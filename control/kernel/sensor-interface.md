# Sensor (Controller) Interface Code Explanation

## Initialization

The interface code is organized into a Linux kernel module, and as such it has declared initialization and exit routines.

The initialization routine is `socket_interface_init`. It pre-allocates memory for JSON tokens and then starts a kernel thread that creates, opens, and listens on a domain socket:
```c
static int __init socket_interface_init(void)
{
	DMSG();
	pre_alloc_tokens(token_storage, TOKEN_DATA_SIZE, TOKEN_ARRAY_SIZE);
	init_connection(&listener, PROBE_LISTEN, socket_name);

	return 0;
}

```

`init_connection` is a complex function that has two different modes: to _create a listening thread_ and to _create a communications thread using a connected socket_.

The initialization call uses the flag `PROBE_LISTEN` which instructs `init_connection` to create a socket, then start a kernel thread to listen on that socket for incoming connections.

### `init_connection PROBE_LISTEN`

When called with `PROBE_LISTEN`, `init_connection` does the following:

* initializes a `struct connection`, which contains all the necessary elements to run as a kernel thread and to reschedule itself as needed.

* calls `start_listener`, which creates, binds, and listens on a unix domain socket. At this point, the `struct connection` has, in addition to the kernel thread elements, an actively listening domain socket.

* calls `link_new_connection_work` which starts a new kernel thread and links `struct connection` to the parent `struct kernel_sensor` using the parent's lockless linked list.


### `k_accept`

The listening socket is serviced by a kernel thread that runs the `k_accept` function. When scheduled, this kernel thread:

* calls the kernel routine `sock->ops->accept`, which returns a new, connected socket.

* creates a new `struct connection`, attaches the new `struct socket` to the connection

* calls `init_connection` using the `PROBE_CONNECT` flag.

The call to `init_connection` seems to be re-entrant, but technically it is not because the caller is a new, different kernel thread that is servicing the `accepted` socket. The flow is re-entrant but the second entry is by a new, different kernel thread. The original caller, which used the `PROBE_LISTEN` flag, is still listening on the original socket and being regularly scheduled by the kernel.

### `init_connection PROBE_CONNECT`

The new accepted socket connection is embodied in a `struct connection` and contains an anymnous `struct probe`, so technically it is running as a probe, although it exists to read and write the connected socket rather than to probe the kernel. But it uses the same resources as a probe uses - kernel threads, linked lists, and spinlocks, etc.

`init_connection PROBE_CONNECT` takes the new accepted connection and:

* links the new connection to the parent `struct kernel sensor` using the parents lockless linked list.


```c
/**
* p is a pointer to a connected socket
**/
struct socket *sock = p;
printk(KERN_INFO "connected socket at %p\n", sock);
/** now we need to read and write messages **/

link_new_connection_work(c, k_read_write, "kcontrol read & write");
```
The call to `link_new_connection_work` scheduled a new kernel thread that calls `k_read_write`


### The Sensor Protocol

The accepted connection engages in the Sensor Protocol as a _server_. When the client closes the protocol, the connection kills itself.

### `k_read_write`

`k_read_write` engages in the JSONL Probe protocol.

```c
static void k_read_write(struct kthread_work *work)
{
	int ccode = 0;
	uint8_t buf [CONNECTION_MAX_HEADER + 1];


	struct socket *sock = NULL;
	struct kthread_worker *worker = work->worker;
	struct connection *connection =
		container_of(work, struct connection, work);

	assert(connection &&
		   connection->flags);
	assert(__FLAG_IS_SET(connection->flags, PROBE_CONNECT));
	assert(__FLAG_IS_SET(connection->flags, PROBE_HAS_WORK));

	sock = connection->connected;

	ccode = k_socket_read(sock, 1, buf);
	ccode = k_socket_write(sock, 1, buf);

	if (! SHOULD_SHUTDOWN ) {
		init_and_queue_work(work, worker, k_read_write);
	}
};
```
