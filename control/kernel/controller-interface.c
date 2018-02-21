/*******************************************************************************
 * in-virtue kernel controller
 * Published under terms of the Gnu Public License v2, 2017
 ******************************************************************************/

#include "controller.h"



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


/**
 * JSON tokens are limited to TOKEN_ARRAY_SIZE, defined in
 * controller.h: 190, which is approximately PAGE_SIZE / sizeof(jsmntok_t),
 * usually equal to
 **/
static struct flex_array *token_storage;
/* pre-allocate tokens */
static inline int
pre_alloc_tokens(struct flex_array *ts, int data_size, int array_size)
{
	ts = flex_array_alloc(data_size, array_size, GFP_KERNEL);
	if (!ts)
		return -ENOMEM;

	if (flex_array_prealloc(ts, 0, TOKEN_ARRAY_SIZE, GFP_KERNEL | __GFP_ZERO)) {
		flex_array_free(ts);
		ts = NULL;
		return -ENOMEM;
	}
	return 0;
}


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

	ccode = k_socket_read(sock, 1, buf);
	ccode = k_socket_write(sock, 1, buf);

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
	pre_alloc_tokens(token_storage, TOKEN_DATA_SIZE, TOKEN_ARRAY_SIZE);
	init_connection(&listener, PROBE_LISTEN, socket_name);

	return 0;
}

static void __exit socket_interface_exit(void)
{
	SHOULD_SHUTDOWN = 1;

	return;
}



/*****************************************************************/
/*****************************************************************/


/**
 * Allocates a fresh unused token from the token pull.
 */
static jsmntok_t *
jsmn_alloc_token (jsmn_parser * parser, jsmntok_t * tokens, size_t num_tokens)
{
  jsmntok_t *tok;
  if (parser->toknext >= num_tokens)
    {
      return NULL;
    }
  tok = &tokens[parser->toknext++];
  tok->start = tok->end = -1;
  tok->size = 0;
#ifdef JSMN_PARENT_LINKS
  tok->parent = -1;
#endif
  return tok;
}

/**
 * Fills token type and boundaries.
 */
static void
jsmn_fill_token (jsmntok_t * token, jsmntype_t type, int start, int end)
{
  token->type = type;
  token->start = start;
  token->end = end;
  token->size = 0;
}

/**
 * Fills next available token with JSON primitive.
 */
static int
jsmn_parse_primitive (jsmn_parser * parser, const char *js,
		      size_t len, jsmntok_t * tokens, size_t num_tokens)
{
  jsmntok_t *token;
  int start;

  start = parser->pos;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
      switch (js[parser->pos])
	{
#ifndef JSMN_STRICT
	  /*
	   * In strict mode primitive must be followed by "," or "}" or
	   * "]"
	   */
	case ':':
#endif
	case '\t':
	case '\r':
	case '\n':
	case ' ':
	case ',':
	case ']':
	case '}':
	  goto found;
	}
      if (js[parser->pos] < 32 || js[parser->pos] >= 127)
	{
	  parser->pos = start;
	  return JSMN_ERROR_INVAL;
	}
    }
#ifdef JSMN_STRICT
  /*
   * In strict mode primitive must be followed by a comma/object/array
   */
  parser->pos = start;
  return JSMN_ERROR_PART;
#endif

found:
  if (tokens == NULL)
    {
      parser->pos--;
      return 0;
    }
  token = jsmn_alloc_token (parser, tokens, num_tokens);
  if (token == NULL)
    {
      parser->pos = start;
      return JSMN_ERROR_NOMEM;
    }
  jsmn_fill_token (token, JSMN_PRIMITIVE, start, parser->pos);
#ifdef JSMN_PARENT_LINKS
  token->parent = parser->toksuper;
#endif
  parser->pos--;
  return 0;
}

/**
 * Fills next token with JSON string.
 */
static int
jsmn_parse_string (jsmn_parser * parser, const char *js,
		   size_t len, jsmntok_t * tokens, size_t num_tokens)
{
  jsmntok_t *token;

  int start = parser->pos;

  parser->pos++;

  /*
   * Skip starting quote
   */
  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
      char c = js[parser->pos];

      /*
       * Quote: end of string
       */
      if (c == '\"')
	{
	  if (tokens == NULL)
	    {
	      return 0;
	    }
	  token = jsmn_alloc_token (parser, tokens, num_tokens);
	  if (token == NULL)
	    {
	      parser->pos = start;
	      return JSMN_ERROR_NOMEM;
	    }
	  jsmn_fill_token (token, JSMN_STRING, start + 1, parser->pos);
#ifdef JSMN_PARENT_LINKS
	  token->parent = parser->toksuper;
#endif
	  return 0;
	}

      /*
       * Backslash: Quoted symbol expected
       */
      if (c == '\\' && parser->pos + 1 < len)
	{
	  int i;
	  parser->pos++;
	  switch (js[parser->pos])
	    {
	      /*
	       * Allowed escaped symbols
	       */
	    case '\"':
	    case '/':
	    case '\\':
	    case 'b':
	    case 'f':
	    case 'r':
	    case 'n':
	    case 't':
	      break;
	      /*
	       * Allows escaped symbol \uXXXX
	       */
	    case 'u':
	      parser->pos++;
	      for (i = 0;
		   i < 4 && parser->pos < len && js[parser->pos] != '\0'; i++)
		{
		  /*
		   * If it isn't a hex character we have an error
		   */
		  if (!((js[parser->pos] >= 48 && js[parser->pos] <= 57) ||	/* 0-9
										 */
			(js[parser->pos] >= 65 && js[parser->pos] <= 70) ||	/* A-F
										 */
			(js[parser->pos] >= 97 && js[parser->pos] <= 102)))
		    {		/* a-f
				 */
		      parser->pos = start;
		      return JSMN_ERROR_INVAL;
		    }
		  parser->pos++;
		}
	      parser->pos--;
	      break;
	      /*
	       * Unexpected symbol
	       */
	    default:
	      parser->pos = start;
	      return JSMN_ERROR_INVAL;
	    }
	}
    }
  parser->pos = start;
  return JSMN_ERROR_PART;
}

/**
 * Parse JSON string and fill tokens.
 */
int
jsmn_parse (jsmn_parser * parser, const char *js, size_t len,
	    jsmntok_t * tokens, unsigned int num_tokens)
{
  int r;
  int i;
  jsmntok_t *token;
  int count = parser->toknext;

  for (; parser->pos < len && js[parser->pos] != '\0'; parser->pos++)
    {
      char c;
      jsmntype_t type;

      c = js[parser->pos];
      switch (c)
	{
	case '{':
	case '[':
	  count++;
	  if (tokens == NULL)
	    {
	      break;
	    }
	  token = jsmn_alloc_token (parser, tokens, num_tokens);
	  if (token == NULL)
	    return JSMN_ERROR_NOMEM;
	  if (parser->toksuper != -1)
	    {
	      tokens[parser->toksuper].size++;
#ifdef JSMN_PARENT_LINKS
	      token->parent = parser->toksuper;
#endif
	    }
	  token->type = (c == '{' ? JSMN_OBJECT : JSMN_ARRAY);
	  token->start = parser->pos;
	  parser->toksuper = parser->toknext - 1;
	  break;
	case '}':
	case ']':
	  if (tokens == NULL)
	    break;
	  type = (c == '}' ? JSMN_OBJECT : JSMN_ARRAY);
#ifdef JSMN_PARENT_LINKS
	  if (parser->toknext < 1)
	    {
	      return JSMN_ERROR_INVAL;
	    }
	  token = &tokens[parser->toknext - 1];
	  for (;;)
	    {
	      if (token->start != -1 && token->end == -1)
		{
		  if (token->type != type)
		    {
		      return JSMN_ERROR_INVAL;
		    }
		  token->end = parser->pos + 1;
		  parser->toksuper = token->parent;
		  break;
		}
	      if (token->parent == -1)
		{
		  if (token->type != type || parser->toksuper == -1)
		    {
		      return JSMN_ERROR_INVAL;
		    }
		  break;
		}
	      token = &tokens[token->parent];
	    }
#else
	  for (i = parser->toknext - 1; i >= 0; i--)
	    {
	      token = &tokens[i];
	      if (token->start != -1 && token->end == -1)
		{
		  if (token->type != type)
		    {
		      return JSMN_ERROR_INVAL;
		    }
		  parser->toksuper = -1;
		  token->end = parser->pos + 1;
		  break;
		}
	    }
	  /*
	   * Error if unmatched closing bracket
	   */
	  if (i == -1)
	    return JSMN_ERROR_INVAL;
	  for (; i >= 0; i--)
	    {
	      token = &tokens[i];
	      if (token->start != -1 && token->end == -1)
		{
		  parser->toksuper = i;
		  break;
		}
	    }
#endif
	  break;
	case '\"':
	  r = jsmn_parse_string (parser, js, len, tokens, num_tokens);
	  if (r < 0)
	    return r;
	  count++;
	  if (parser->toksuper != -1 && tokens != NULL)
	    tokens[parser->toksuper].size++;
	  break;
	case '\t':
	case '\r':
	case '\n':
	case ' ':
	  break;
	case ':':
	  parser->toksuper = parser->toknext - 1;
	  break;
	case ',':
	  if (tokens != NULL && parser->toksuper != -1 &&
	      tokens[parser->toksuper].type != JSMN_ARRAY &&
	      tokens[parser->toksuper].type != JSMN_OBJECT)
	    {
#ifdef JSMN_PARENT_LINKS
	      parser->toksuper = tokens[parser->toksuper].parent;
#else
	      for (i = parser->toknext - 1; i >= 0; i--)
		{
		  if (tokens[i].type == JSMN_ARRAY
		      || tokens[i].type == JSMN_OBJECT)
		    {
		      if (tokens[i].start != -1 && tokens[i].end == -1)
			{
			  parser->toksuper = i;
			  break;
			}
		    }
		}
#endif
	    }
	  break;
#ifdef JSMN_STRICT
	  /*
	   * In strict mode primitives are: numbers and booleans
	   */
	case '-':
	case '0':
	case '1':
	case '2':
	case '3':
	case '4':
	case '5':
	case '6':
	case '7':
	case '8':
	case '9':
	case 't':
	case 'f':
	case 'n':
	  /*
	   * And they must not be keys of the object
	   */
	  if (tokens != NULL && parser->toksuper != -1)
	    {
	      jsmntok_t *t = &tokens[parser->toksuper];
	      if (t->type == JSMN_OBJECT ||
		  (t->type == JSMN_STRING && t->size != 0))
		{
		  return JSMN_ERROR_INVAL;
		}
	    }
#else
	  /*
	   * In non-strict mode every unquoted value is a primitive
	   */
	default:
#endif
	  r = jsmn_parse_primitive (parser, js, len, tokens, num_tokens);
	  if (r < 0)
	    return r;
	  count++;
	  if (parser->toksuper != -1 && tokens != NULL)
	    tokens[parser->toksuper].size++;
	  break;

#ifdef JSMN_STRICT
	  /*
	   * Unexpected char in strict mode
	   */
	default:
	  return JSMN_ERROR_INVAL;
#endif
	}
    }

  if (tokens != NULL)
    {
      for (i = parser->toknext - 1; i >= 0; i--)
	{
	  /*
	   * Unmatched opened object or array
	   */
	  if (tokens[i].start != -1 && tokens[i].end == -1)
	    {
	      return JSMN_ERROR_PART;
	    }
	}
    }

  return count;
}

/**
 * Creates a new parser based over a given  buffer with an array of tokens
 * available.
 */
void
jsmn_init (jsmn_parser * parser)
{
  parser->pos = 0;
  parser->toknext = 0;
  parser->toksuper = -1;
}


module_init(socket_interface_init);
module_exit(socket_interface_exit);

MODULE_LICENSE(_MODULE_LICENSE);
MODULE_AUTHOR(_MODULE_AUTHOR);
MODULE_DESCRIPTION(_MODULE_INFO "interface");
