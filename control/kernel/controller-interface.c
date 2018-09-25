/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/
#include "uname.h"
#include "controller.h"
#include "jsmn/jsmn.h"
//#include "jsonl-parse.include.c"

/* hold socket JSON probe interface */
extern struct kernel_sensor k_sensor;
extern char *socket_name;
extern char *lockfile_name;
struct connection listener;
EXPORT_SYMBOL(listener);

struct connection *
init_connection(struct connection *, uint64_t, void *);

/**
 * sock refers to struct socket,
 * sk refers to struct sock
 * http://haifux.org/hebrew/lectures/217/netLec5.pdf
 **/
ssize_t
k_socket_read(struct socket *sock,
			  size_t size,
			  void *in,
			  unsigned int flags)
{

	ssize_t res = 0;
	struct msghdr msg = {.msg_flags = flags};
	struct kvec iov = {.iov_base = in, .iov_len = size};

	printk(KERN_DEBUG "k_socket_read sock %p, num bytes to read %ld," \
		   "inbuf %p, flags %x\n",
		   sock, size, in, flags);
again:
	res = kernel_recvmsg(sock, &msg, &iov, 1, size, flags);
	if (res == -EAGAIN)
		goto again;

	return res;
}
STACK_FRAME_NON_STANDARD(k_socket_read);

ssize_t
k_socket_peak(struct socket *sock)
{
	static uint8_t in[CONNECTION_MAX_MESSAGE];

	return k_socket_read(sock, CONNECTION_MAX_MESSAGE,
						 in, MSG_PEEK);
}


ssize_t
k_socket_write(struct socket *sock,
			   size_t size,
			   void *out,
			   unsigned int flags)
{
	ssize_t res = 0;
	struct msghdr msg = {.msg_flags = flags};
	struct kvec iov = {.iov_base = out, .iov_len = size};

again:
	res = kernel_sendmsg(sock, &msg, &iov, 1, size);
	if (res <= 0) {
		printk(KERN_DEBUG "kernel_sendmsg returned err %lx\n", res);
		if (res == -EAGAIN) {
			sleep(1);
			goto again;
		}
	}
	return res;
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
	ccode = k_socket_read(m->socket, m->len, m->line, 0L);
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
		printk(KERN_INFO "kernel sensor error reading from socket\n");
	}
	return ccode;
}



static void
k_echo_server(struct kthread_work *work)
{

	int ccode = 0;
	/**
	 * allocate these buffers dynamically so this function can
	 * be re-entrant. Also avoid allocation on the stack.
	 * these buffers are 1k each. if we need more space, the
	 * message handlers will need to realloc the buf for more space
	 **/
	ssize_t read_size = 0;
	uint8_t *read_buf = NULL;
	uint8_t echo[] = { 'e', 'c', 'h', 'o', 0x00 };
	uint8_t discover[] = { 'd', 'i', 's', 'c', 'o', 'v', 'e', 'r', 0x00 };
	uint8_t session[] = "{Virtue-protocol-version: 0.1}\n";

	struct socket *sock = NULL;
	struct jsmn_message *m = NULL;

	struct kthread_worker *worker = work->worker;
	struct connection *connection =
		container_of(work, struct connection, work);

	assert(connection &&
		   connection->flags &&
		   connection->connected);
	assert(__FLAG_IS_SET(connection->flags, SENSOR_CONNECT));
	assert(__FLAG_IS_SET(connection->flags, SENSOR_HAS_WORK));
	ccode = down_interruptible(&connection->s_lock);
	if (ccode)
		goto close_out;

	sock = connection->connected;
	read_size = k_socket_peak(sock);
	if (read_size >= 5) {
		read_buf = kzalloc(read_size + 1, GFP_KERNEL);
		if (!read_buf) {

			printk(KERN_DEBUG "k_socket read unable to allocate read buffer: " \
				   "%d bytes\n", ccode);
			__CLEAR_FLAG(connection->flags, SENSOR_CONNECT);
			goto close_out;

		}
		ccode = k_socket_read(sock, read_size, read_buf, 0L);
		printk(KERN_DEBUG "k_socket_read %d\n", ccode);
		if (ccode <= 0) {

			printk(KERN_DEBUG "k_socket_read returned error: %d\n", ccode);
			__CLEAR_FLAG(connection->flags, SENSOR_CONNECT);
			goto close_out;
		}
	} else {
		goto close_out;
	}

	if (!memcmp(read_buf, echo, sizeof(echo))) {
		uint8_t *response = VERSION_STRING;
		printk(KERN_DEBUG "k_socket_write echo: %s\n", response);
		ccode = k_socket_write(sock, strlen(response) + 1, response, 0L);
		printk(KERN_DEBUG "k_socket_write echo return code: %d\n", ccode);

	} else if (!memcmp(read_buf, discover, sizeof(discover))){
		uint8_t *response = NULL;
		size_t len = 0;
		ccode = build_discovery_buffer(&response, &len);
		if (!ccode && response && len) {
			printk(KERN_DEBUG "k_socket_write discover: %s\n", response);
			ccode = k_socket_write(sock, len, response, 0L);
			kfree(response);
			printk(KERN_DEBUG "k_socket_write discover return code: %d\n", ccode);
		}

	} else if (!memcmp(read_buf, session, sizeof(session))) {
		uint8_t *response = SESSION_RESPONSE;
		printk(KERN_DEBUG "k_socket_write session: %s\n", response);
		ccode = k_socket_write(sock, strlen(response) + 1, response, 0L);
		printk(KERN_DEBUG "k_socket_write session return code: %d\n", ccode);

    } else {
/**
 * call the json parser
 * ccode contains the bytes read - a good value for message->len
 */
		size_t len = ccode;
		printk(KERN_DEBUG "socket read: %s\n", read_buf);
		m = new_message(read_buf, len);
		if(!m) {
			goto close_out;
		}

		m->socket = sock;
		m->count = parse_json_message(m);
/**
 * TODO: if parse_json_message returns COMPLETE, close connection
 *       else if it returns 0, keep connection open
 **/
		if (m->count < 0) {

			/* for some reason, didn't read a valid json object */
			printk(KERN_DEBUG "kernel sensor error reading a valid JSON object, " \
				   "connection is being closed\n");
			up(&connection->s_lock);
			goto close_out;
		}
	}

 	if (! atomic64_read(&SHOULD_SHUTDOWN)) {
		/* do it all again */
 		init_and_queue_work(work, worker, k_echo_server);
 	}
	up(&connection->s_lock);
	return;

close_out:
	spin_lock(&k_sensor.lock);
	list_del_rcu(&connection->l_node);
	spin_unlock(&k_sensor.lock);
	synchronize_rcu();
	kfree(connection->_destroy(connection));


	return;
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
 * The json parser will dispatch the message to the correct
 * sensor.probe, which is expected to generate a response.
 *
 **/

static void
k_read_write(struct kthread_work *work)
{
	int ccode = 0;
	unsigned long flags = MSG_PEEK;
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
	assert(__FLAG_IS_SET(connection->flags, SENSOR_CONNECT));
	assert(__FLAG_IS_SET(connection->flags, SENSOR_HAS_WORK));

	sock = connection->connected;
	read_buf = kzalloc(CONNECTION_MAX_HEADER, GFP_KERNEL);
	if (read_buf == NULL) {
/**
 * running as a kernel thread, can't return an error code to the caller,
 * so we need to log this error info.
 **/
		printk(KERN_INFO "kernel sensor error - no memory, kernel thread exiting\n");
		return;
	}

again:
	ccode = k_socket_read(sock, CONNECTION_MAX_HEADER - 1, read_buf, flags);
	printk(KERN_DEBUG "k_socket_read MSG_PEEK returned %d\n", ccode);

	if (ccode < 0){

		if (ccode == -EAGAIN) {
			goto again;
		}
		__CLEAR_FLAG(connection->flags, SENSOR_CONNECT);
		goto err_out1;
	}
	if (flags == MSG_PEEK) {

		flags = 0L;
		goto again;
	}

	m = new_message(read_buf, CONNECTION_MAX_HEADER);
	if(!m) {
		goto err_out1;
	}

	m->socket = sock;
	m->count = read_parse_message(m);
	if (m->count < 0) {
		/* for some reason, didn't read a valid json object */
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
	 * resources when it returns >= 0.fpu
	 **/
	ccode = parse_json_message(m);
	if (ccode < 0) {
		printk(KERN_INFO "kernel sensor error parsing a protocol message, " \
			   "connection is being closed\n");
		goto err_out0;
	}

 	if (! atomic64_read(&SHOULD_SHUTDOWN)) {
		/* do it all again */
 		init_and_queue_work(work, worker, k_read_write);
 	}
	return;

err_out0:
	free_message(m);
err_out1:
	if (read_buf) kfree(read_buf);
	spin_lock(&k_sensor.lock);
	list_del_rcu(&connection->l_node);
	spin_unlock(&k_sensor.lock);
	synchronize_rcu();
	kfree(connection->_destroy(connection));
	return;
}
STACK_FRAME_NON_STANDARD(k_read_write);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-function"

/**
 * NOTE: k_poll is unused. It is intended to be a replacement for
 * k_accept, which works perfectly fine. k_poll would be slightly more
 * efficient and responsive, but more complex. It isn't tested.
 *
 * -mdd
 **/


static void k_poll(struct kthread_work *work)
{

	struct connection *new_connection = NULL;
	struct socket *newsock = NULL;
	struct kthread_worker *worker = work->worker;
	struct connection *connection =
		container_of(work, struct connection, work);

	if (down_interruptible(&connection->s_lock)) {
		goto close_out_reschedule;
	}

/**
 * 0. get a struct file * on the socket, socket->file
 * 1. file->private_data = struct socket
 * 2. struct sock sock = socket->sk
 * 3. init a poll_table
 * 4. poll_initwait init a struct poll_wqueues
 * 5. poll_wait(file, sk_sleep(sk), poll_table *)
 * 6. poll_freewait
 *
 * static inline void
 * poll_wait(struct file * filp, wait_queue_head_t * wait_address, poll_table *p)
 * {
 *	if (p && p->_qproc && wait_address)
 *		p->_qproc(filp, wait_address, p);
 * }
 *
 * static inline void sock_poll_wait(struct file *filp,
 *		wait_queue_head_t *wait_address, poll_table *p)
 * poll table has the struct file * in it.
 **/
	if (connection->connected) {
		poll_table p_table;
		struct poll_wqueues wq;
		struct socket *socket = connection->connected;
		struct file *f = socket->file;

		f->private_data = socket;
		init_poll_funcptr(&p_table, poll_wait);
		poll_initwait(&wq);


		/** calls unix_poll_wait **/
		while (! atomic64_read(&SHOULD_SHUTDOWN)) {
			int ccode = 0;
			uint8_t read_buf[] = {0,0,0,0};
			ktime_t to = (NSEC_PER_SEC / 2);
			u64 slack = 0;

			poll_schedule_timeout(&wq,
								  TASK_INTERRUPTIBLE,
								  &to,
								  slack);
			sock_poll_wait(f, sk_sleep(socket->sk), &p_table);
			/**
			 * peek to see if there is data to read
			 **/
peek_again:
			ccode = k_socket_read(socket, 1, read_buf, MSG_PEEK);
			if (!ccode) {
				continue;
			}
			if (unlikely(ccode < 0)) {
				if (ccode == -EAGAIN) {
					goto peek_again;
				}
				__CLEAR_FLAG(connection->flags, SENSOR_LISTEN);
				goto close_out_quit;
			}

			if (! atomic64_read(&SHOULD_SHUTDOWN)) {
				if (unlikely((ccode = kernel_accept(connection->connected,
													&newsock,
													0L)) < 0))
				{
					printk(KERN_DEBUG "k_accept returned error %d, exiting\n",
						   ccode);
					goto close_out_quit;
				}
			}
/**
 * create a new struct connection, link it to the kernel sensor
 **/
			if (newsock != NULL && (! atomic64_read(&SHOULD_SHUTDOWN))) {
				new_connection = kzalloc(sizeof(struct connection), GFP_KERNEL);
				if (new_connection) {
					init_connection(new_connection, SENSOR_CONNECT, newsock);
				} else {
					atomic64_set(&SHOULD_SHUTDOWN, 1);
				}
			}
		}
	}


close_out_reschedule:
	if (! atomic64_read(&SHOULD_SHUTDOWN)) {
		init_and_queue_work(work, worker, k_poll);
	}
close_out_quit:
	up(&connection->s_lock);
	return;
}

#pragma GCC diagnostic pop

static void k_accept(struct kthread_work *work)
{
	int ccode = 0;

	struct connection *new_connection = NULL;
	struct socket *newsock = NULL;
	struct kthread_worker *worker = work->worker;
	struct connection *connection =
		container_of(work, struct connection, work);

	/**
	 * the unix socket implementation masks out MSG_DONTWAIT,
	 * and then uses O_NONBLOCK internally when handling queued skbs.
	 * the result is that unix accept always blocks.
	 **/

	if (down_interruptible(&connection->s_lock))
		goto close_out_reschedule;

	if (! atomic64_read(&SHOULD_SHUTDOWN)) {
		if ((ccode = kernel_accept(connection->connected,
								   &newsock,
								   0L)) < 0)
		{
			printk(KERN_DEBUG "k_accept returned error %d, exiting\n",
				   ccode);
			goto close_out_quit;
		}
	}
/**
 * create a new struct connection, link it to the kernel sensor
 **/
	if (newsock != NULL && (! atomic64_read(&SHOULD_SHUTDOWN))) {

		new_connection = kzalloc(sizeof(struct connection), GFP_KERNEL);
		if (new_connection) {
			init_connection(new_connection, SENSOR_CONNECT, newsock);
		} else {
			atomic64_set(&SHOULD_SHUTDOWN, 1);
		}
	}
close_out_reschedule:
	if (! atomic64_read(&SHOULD_SHUTDOWN)) {
		init_and_queue_work(work, worker, k_accept);
	}
close_out_quit:
	up(&connection->s_lock);

	return;
}
STACK_FRAME_NON_STANDARD(k_accept);

#pragma GCC diagnostic push
#ifdef FRAME_CHECKING
#pragma GCC diagnostic ignored "-Wstringop-truncation"
#endif
static int start_listener(struct connection *c)
{
	struct sockaddr_un addr = {.sun_family = AF_UNIX};
	struct socket *sock = NULL;

	assert(__FLAG_IS_SET(c->flags, SENSOR_LISTEN));
	SOCK_CREATE_KERN(&init_net, AF_UNIX, SOCK_STREAM, 0, &sock);

	if (!sock) {
		c->connected = NULL;
		goto err_exit;
	}
	c->connected = sock;
	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	memcpy(addr.sun_path, &c->path[0], sizeof(addr.sun_path));

	/* sizeof(address) - 1 is necessary to ensure correct null-termination */
	if (kernel_bind(sock,(struct sockaddr *)&addr,
					sizeof(addr) -1)) {
		goto err_release;

	}
/* see /usr/include/net/tcp_states.h */
	if (kernel_listen(sock, TCP_LISTEN)) {
		kernel_sock_shutdown(sock, RCV_SHUTDOWN | SEND_SHUTDOWN);
		goto err_release;
	}

	return 0;
err_release:

	sock_release(sock);
err_exit:
	c->connected = NULL;
	return -ENFILE;
}
STACK_FRAME_NON_STANDARD(start_listener);
#pragma GCC diagnostic pop



static inline void
link_new_connection_work(struct connection *c,
						 struct list_head *l,
						 void (*f)(struct kthread_work *),
						 uint8_t *d)
{

	if (! atomic64_read(&SHOULD_SHUTDOWN)) {

		spin_lock(&k_sensor.lock);
		list_add_rcu(&c->l_node, l);
		spin_unlock(&k_sensor.lock);

		CONT_INIT_WORK(&c->work, f);
		__SET_FLAG(c->flags, SENSOR_HAS_WORK);

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
	if (down_interruptible(&c->s_lock))
		return c;

	if (c->connected) {
		kernel_sock_shutdown(c->connected, SHUT_RDWR);
		sock_release(c->connected);
		c->connected = NULL;
	}
	up(&c->s_lock);
	c->destroy((struct sensor *)c);
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

	assert(c != NULL);
	assert(p != NULL);
	assert(__FLAG_IS_SET(flags, SENSOR_LISTEN) ||
		   __FLAG_IS_SET(flags, SENSOR_CONNECT));
	assert(! (__FLAG_IS_SET(flags, SENSOR_LISTEN) &&
			  __FLAG_IS_SET(flags, SENSOR_CONNECT)));

	memset(c, 0x00, sizeof(struct connection));
	c = (struct connection *)init_sensor((struct sensor *)c,
										"connection", strlen("connection") + 1);
	sema_init(&c->s_lock, 1);

/**
 * TODO: I should set the flags individually, or mask the flags param;
 * I'm assuming that listen and connect are the only two flags set in the flags param
 **/
	/**
	 * TODO: does sock_release free socket memory?
	 **/
	c->flags |= flags;
	c->_destroy = destroy_connection;

	if (__FLAG_IS_SET(flags, SENSOR_LISTEN)) {
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
		 * first, link it to the kernel sensor list of connections, then schedule
		 * it as work
		 **/

		link_new_connection_work(c,
								 &k_sensor.listeners,
								 k_accept,
								 "kcontrol accept");


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
		c->connected = sock;
		link_new_connection_work(c,
								 &k_sensor.connections,
								 k_echo_server,
								 "kcontrol read & write");
	}


	return c;

err_exit:

	if (c->connected) {
		c->connected = NULL;
	}
	return ERR_PTR(ccode);
}
STACK_FRAME_NON_STANDARD(init_connection);

static void
awaken_accept_thread(void)
{
	struct sockaddr_un addr;
	struct socket *sock = NULL;
	size_t path_len, addr_len;
	int ccode = 0;

	SOCK_CREATE_KERN(&init_net, AF_UNIX, SOCK_STREAM, 0, &sock);
	if (!sock)
		return;


	memset(&addr, 0, sizeof(addr));
	addr.sun_family = AF_UNIX;
	path_len = strlen(socket_name);
	addr_len = sizeof(addr.sun_path) - 1;

	memcpy(addr.sun_path,
		   socket_name,
		   (path_len < addr_len) ? path_len : addr_len);
	ccode = kernel_connect(sock, (struct sockaddr *)&addr, sizeof(addr.sun_path), 0L);
	if (! ccode) {

		kernel_sock_shutdown(sock, SHUT_RDWR);
		sock_release(sock);
	}
	/**
	 * wait until the listener has exited
	 **/
	if (! down_interruptible(&listener.s_lock))
		up(&listener.s_lock);
	return;
}

static int
unlink_sock_name(char *sock_name, char *lock_name)
{
	struct path name_path = {.mnt = 0};
	struct file *lock_file = NULL;
	struct file_lock l = {
		.fl_flags = FL_FLOCK,
		.fl_type = F_WRLCK,
	};
	int need_lock = 0;
	int ccode = kern_path(sock_name, LOOKUP_FOLLOW, &name_path);
	if (ccode) {
       /**
        * its not an error if we can't get the path, it probably means
        * the socket name does not need to be unlinked, perhaps it has not
        * been created yet.
		*
		* but, continue onward and try to get the lock file, so another instance
		* (in the future) will not unlink the sock name while we are using it.
        **/
		;
	}

	if (lock_name) {
		/**
		 * open the lock file, create it if necessary
		 **/
		lock_file = filp_open(lock_name, O_RDONLY | O_CREAT, S_IRUSR | S_IWUSR);
		if (IS_ERR(lock_file) || ! lock_file) {
			printk(KERN_DEBUG "error opening or creating the socket lock\n");
			ccode = -ENFILE;
			goto exit;
		}

		/**
		 * POSIX protocol says this lock will be released if the module
		 * crashes or exits
		 **/
		need_lock = vfs_lock_file(lock_file, F_SETLK, &l, NULL);
	}

	if (!need_lock && !ccode) {
		ccode = vfs_unlink(name_path.dentry->d_parent->d_inode,
						   name_path.dentry,
						   NULL);
	}
exit:
	return ccode;
}


int
socket_interface_init(void)
{
	init_jsonl_parser();
	unlink_sock_name(socket_name, lockfile_name);
	init_connection(&listener, SENSOR_LISTEN, socket_name);
	return 0;
}

void
socket_interface_exit(void)
{
	atomic64_set(&SHOULD_SHUTDOWN, 1);
	awaken_accept_thread();
	unlink_sock_name(socket_name, NULL);
	return;
}

