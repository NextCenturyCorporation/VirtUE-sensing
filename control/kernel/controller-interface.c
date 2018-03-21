/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
#include "controller.h"
#include "jsmn/jsmn.h"
#include "jsmn/jsmn.c"
#include "jsonl-parse.include.c"

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


#pragma GCC diagnostic ignored "-Wunused-function"

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

/**
 * @brief read_parse_message - read a message from the connected
 * domain socket and see if it can be parsed as a JSON object.
 *
 * struct message is assumed to be partially initialized:
 * m->spinlock
 * m->line points to a buffer
 * m->len is the size of the buffer, but will get re-initialized to
 *        the number of bytes read
 * everything else will be initialized by the function
 *
 * if json_parse returns JSMN_ERROR_PART we need to realloc
 * a larger buffer and continue reading, but we set a hard limit
 * at CONNECTION_MAX_MESSAGE, by default 4k.
 * the initial buffer size is expected to be CONNECTION_MAX_HEADER,
 * by default 1K.
 *
 * It is not expected that the realloc loop will run (JSMN_ERROR_PART)
 * in version 0.1 of the kernel sensor protocol, because the linux kernel
 * is mostly likely to be acting as a server of the kernel sensor protocol,
 * and request messages are expected to be smaller than CONNECTION_MAX_HEADER,
 * so it would be good to re-visit the definition of CONNECTION_MAX_HEADER,
 * and even the use of krealloc below.
 **/
static int read_parse_message(struct jsmn_message *m)
{
	int ccode = 0, len_save = 0, bytes_read = 0;
	void *line_save = NULL;
	assert(m && m->line && m->len <= CONNECTION_MAX_MESSAGE);
	assert(m->socket);
	len_save = m->len;

again:
	ccode = k_socket_read(m->socket, m->len, m->line);
	if (ccode > 0) {
		bytes_read += ccode;
		/* make sure that the read buffer is terminated with a zero */
		line_save = m->line;
		m->line[bytes_read] = 0x00;
		m->len = bytes_read;
		jsmn_init(&m->parser);
		m->count = jsmn_parse(&m->parser,
							  m->line,
							  m->len,
							  m->tokens,
							  MAX_TOKENS);
		if (m->count == JSMN_ERROR_PART && len_save < CONNECTION_MAX_MESSAGE) {
            /* it may be valid to realloc and try again */
			DMSG();
			printk(KERN_INFO "kernel sensor read part of a JSON object, " \
				   "attempting to realloc and read the remainder\n");
			m->line = krealloc(m->line, CONNECTION_MAX_MESSAGE, GFP_KERNEL);
			if (m->line) {
				len_save = CONNECTION_MAX_MESSAGE;
				line_save = m->line;
				m->len = CONNECTION_MAX_MESSAGE - (1 + bytes_read);
				*(m->line + CONNECTION_MAX_MESSAGE) = 0x00;
				m->line += bytes_read;
				goto again;
			}
		}
		/* set ccode to the number of tokens */
		ccode = m->count;
	} else {
		DMSG();
		printk(KERN_INFO "kernel sensor error reading from socket\n");
	}
	return ccode;
}

/**
 * @brief k_read_write kernel thread that reads and writes the JSON
 * kernel sensor protocol over a connected socket.
 *
 * If the socket presents a valid JSON object that is also a
 * sensor protocol message, create struct jsmn_message and dispatch
 * that message to the json parser.
 *
 * The json parser will create a struct jsmn_session which it will use
 * to organize the response message and eventually to write that response
 * back to the client over the same socket.
 *
 * Te json parser will dispatch the message to the correct
 * sensor.probe, which is expected to generate a response.
 *
 **/

static void
k_read_write(struct kthread_work *work)
{
	int ccode = 0;
	/**
	 * allocate these buffers dynamically so this function can
	 * be re-entrant. Also avoid allocation on the stack.
	 * these buffers are 1k each. if we need more space, the
	 * message handlers will need to realloc the buf for more space
	 **/
	uint8_t *read_buf = NULL;

	struct jsmn_message *m = NULL;
	struct socket *sock = NULL;
	struct kthread_worker *worker = work->worker;
	struct connection *connection =
		container_of(work, struct connection, work);

	assert(connection &&
		   connection->flags);
	assert(__FLAG_IS_SET(connection->flags, PROBE_CONNECT));
	assert(__FLAG_IS_SET(connection->flags, PROBE_HAS_WORK));

	sock = connection->connected;
	read_buf = kzalloc(CONNECTION_MAX_HEADER, GFP_KERNEL);
	if (read_buf == NULL) {
/**
 * running as a kernel thread, can't return an error code, so we need
 * to log this error info
 **/
		DMSG();
		printk(KERN_INFO "kernel sensor error - no memory, kernel thread exiting\n");
		return;
	}

again:
	ccode = k_socket_read(sock, CONNECTION_MAX_HEADER - 1, read_buf);
	if (ccode < 0){
		if (ccode == -EAGAIN) {
			goto again;
		}
		__CLEAR_FLAG(connection->flags, PROBE_CONNECT);
		goto err_out1;
	}
	m = new_message(read_buf, CONNECTION_MAX_HEADER);
	if(!m) {
		goto err_out1;
	}

	m->socket = sock;
	m->count = read_parse_message(m);
	if (m->count < 0) {
    /* for some reason, didn't read a valid json object */
		DMSG();
		printk(KERN_INFO "kernel sensor error reading a valid JSON object, " \
			   "connection is being closed\n");
		goto err_out0;
	}

	/**
	 * following call, if successful, will dispatch to the
	 * correct message handler, which will call into the correct
	 * probe, which will usually result in a write to this socket.
	 *
	 * parse_json_message accepts responsibility for freeing
	 * resources when it returns >= 0.
	 **/
	ccode = parse_json_message(m);
	if (ccode < 0) {
		DMSG();
		printk(KERN_INFO "kernel sensor error parsing a protocol message, " \
			   "connection is being closed\n");
		goto err_out0;
	}

 	if (! SHOULD_SHUTDOWN ) {
		/* do it all again */
 		init_and_queue_work(work, worker, k_read_write);
 	}
	return;

err_out0:
	free_message(m);
err_out1:
	if (read_buf) kfree(read_buf);
	kfree(connection->_destroy(connection));
	return;
}

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
 * tear down the connection but don't free the connection
 * memory. do free resources, struct sock.
 **/
static inline void *destroy_connection(struct connection *c)
{
	/* destroy the probe resources */
	c->destroy((struct probe *)c);
	if (c->connected) {
		sock_release(c->connected);
	}
	memset(c, 0x00, sizeof(*c));
	return c;
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
	c->_destroy = destroy_connection;

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
	INIT_LIST_HEAD(&h_sessions);
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
