/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/

#include "controller.h"
#include "jsmn/jsmn.h"

/* hold socket JSON probe interface */
extern struct kernel_sensor k_sensor;
static struct connection listener;
static char *socket_name = "/var/run/kernel_sensor";

module_param(socket_name, charp, 0644);

struct connection *
init_connection(struct connection *, uint64_t, void *);

/**
 * sock refers to struct socket,
 * sk refers to struct sock
 * http://haifux.org/hebrew/lectures/217/netLec5.pdf
 **/

static int k_socket_read(struct socket *s, int n, void *in)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int ccode = 0;

	assert(s);
	assert(in);
	assert(n > 0 && n <= CONNECTION_MAX_HEADER);

	memset(&msg, 0x00, sizeof(msg));
	memset(&iov, 0x00, sizeof(iov));
	msg.msg_name = 0;
	msg.msg_namelen = 0;

	iov.iov_base = in;
	iov.iov_len = n;
	msg.msg_iter.iov = &iov;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	/**
	 * sock_recvmsg changed in the kernel - see uname.h
	 **/
	ccode = SOCK_RECVMSG(s, &msg, n, 0);
	set_fs(oldfs);
	return ccode;
}

static int k_socket_write(struct socket *s, int n, void *out)
{
	struct msghdr msg;
	struct iovec iov;
	mm_segment_t oldfs;
	int ccode = 0;

	assert(s);
	assert(out);
	assert(n > 0 && n <= CONNECTION_MAX_HEADER);

	memset(&msg, 0x00, sizeof(msg));
	memset(&iov, 0x00, sizeof(iov));
	msg.msg_name = 0;
	msg.msg_namelen = 0;

	iov.iov_base = out;
	iov.iov_len = n;
	msg.msg_iter.iov = &iov;

	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;

	oldfs = get_fs();
	set_fs(KERNEL_DS);
	ccode = sock_sendmsg(s, &msg);
	set_fs(oldfs);
	return ccode;
}

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

	ccode = k_socket_write(sock, 1, buf);
	ccode = k_socket_read(sock, 1, buf);

	if (! SHOULD_SHUTDOWN ) {
		init_and_queue_work(work, worker, k_read_write);
	}
};


static void k_accept(struct kthread_work *work)
{
	struct connection *new_connection = NULL;
	struct socket *newsock = NULL, *sock = NULL;
	struct kthread_worker *worker = work->worker;
	struct connection *connection =
		container_of(work, struct connection, work);

	assert(connection &&
		   connection->flags);
	assert(__FLAG_IS_SET(connection->flags, PROBE_LISTEN));
	assert(__FLAG_IS_SET(connection->flags, PROBE_HAS_WORK));

	sock = connection->connected;
	if ((sock->ops->accept(sock, newsock, 0)) < 0) {
		SHOULD_SHUTDOWN = 1;
	}

/**
 * create a new struct connection, link it to the kernel sensor
 **/
	if (newsock != NULL && (! SHOULD_SHUTDOWN)) {
		new_connection = kzalloc(sizeof(struct connection), GFP_KERNEL);
		if (new_connection) {
			init_connection(new_connection, PROBE_CONNECT, newsock);
		} else {
			SHOULD_SHUTDOWN = 1;
		}
	}
	if (! SHOULD_SHUTDOWN ) {
		init_and_queue_work(work, worker, k_accept);
	}
	return;
}

static int start_listener(struct connection *c)
{
	struct sockaddr_un addr;
	struct socket *sock = NULL;
	assert(__FLAG_IS_SET(c->flags, PROBE_LISTEN));

	sock_create(AF_UNIX, SOCK_STREAM, 0, &sock);
	if (!sock) {
		c->connected = NULL;
		goto err_exit;
	}
	c->connected = sock;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	strncpy(addr.sun_path, &c->path[0], UNIX_PATH_MAX - 1);

	/* sizeof(address) - 1 is necessary to ensure correct null-termination */
	if (c->connected->ops->bind(sock,(struct sockaddr *)&addr,
								sizeof(addr) -1)) {
		goto err_release;

	}
/* see /usr/include/net/tcp_states.h */
	if (c->connected->ops->listen(sock, TCP_LISTEN)) {
		c->connected->ops->shutdown(sock, RCV_SHUTDOWN);
		goto err_release;
	}

	return 0;
err_release:
	sock_release(sock);
err_exit:
	c->connected = NULL;
	return -ENFILE;
}


static inline void
link_new_connection_work(struct connection *c,
						 void (*f)(struct kthread_work *),
						 uint8_t *d)
{
	if (!SHOULD_SHUTDOWN) {
		llist_add(&c->l_node, &k_sensor.probes);
		CONT_INIT_WORK(&c->work, f);
		__SET_FLAG(c->flags, PROBE_HAS_WORK);
		CONT_INIT_WORKER(&c->worker);
		CONT_QUEUE_WORK(&c->worker, &c->work);
		kthread_run(kthread_worker_fn, &c->worker, d);
	}
}

						 

/**
 * return a newly initialized connnection struct,
 * socket will either be bound and listening, or
 * accepted and connected, according to flags
 **/
struct connection *
init_connection(struct connection *c, uint64_t flags, void *p)
{
	int ccode = 0;
	assert(c);
	assert(p);
	assert(__FLAG_IS_SET(flags, PROBE_LISTEN) ||
		   __FLAG_IS_SET(flags, PROBE_CONNECT));
	assert(! (__FLAG_IS_SET(flags, PROBE_LISTEN) &&
			  __FLAG_IS_SET(flags, PROBE_CONNECT)));


	memset(c, 0x00, sizeof(struct connection));
	c = (struct connection *)init_probe((struct probe *)c,
										"connection", strlen("connection") + 1);
	c->flags = flags;
	if (__FLAG_IS_SET(flags, PROBE_LISTEN)) {
		/**
		 * p is a pointer to a string holding the socket name
		 **/
		memcpy(&c->path, p, UNIX_PATH_MAX);
		if((ccode = start_listener(c))) {
			ccode = -ENFILE;
			goto err_exit;
		}

        /**
		 * the socket is now bound and listening, we don't want to block
		 * here so schedule the accept to happen on a separate kernel thread.
		 * first, link it to the kernel sensor list of probes, then schedule
		 * it as work
		 **/

		link_new_connection_work(c, k_accept, "kcontrol accept");
		
	
	} else { /** 
			  * new sock is accepted and a new 
			  * connection is created, allocated and its probe 
			  * elements are initialfized 
			  **/
		/**
		 * p is a pointer to a connected socket
		 **/
		struct socket *sock = p;
		printk(KERN_INFO "connected socket at %p\n", sock);
		/** now we need to read and write messages **/

		link_new_connection_work(c, k_read_write, "kcontrol read & write");
	}
	return c;

err_exit:

	if (c->connected) {
		c->connected = NULL;
	}
	return ERR_PTR(ccode);
}


static int __init socket_interface_init(void)
{
	DMSG();

	init_connection(&listener, PROBE_LISTEN, socket_name);

	return 0;
}

static void __exit socket_interface_exit(void)
{
	SHOULD_SHUTDOWN = 1;

	return;
}

module_init(socket_interface_init);
module_exit(socket_interface_exit);

MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR(_MODULE_AUTHOR);
MODULE_DESCRIPTION(_MODULE_INFO "interface");
