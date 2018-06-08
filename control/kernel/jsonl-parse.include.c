#ifdef USERSPACE

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include <sys/queue.h>
#endif /* USERSPACE */

#include "jsmn/jsmn.h"


#ifdef USERSPACE
typedef char uint8_t;
typedef int spinlock_t;
#define GFP_KERNEL 1
#define krealloc(p, t, f)						\
	realloc((p), (t))
#define kzalloc(s, f) calloc((s), sizeof(uint8_t))
#define kfree(p) free(p)
#define kstrdup(p) strdup(p)
#define printk printf
#define KERN_INFO ""
#else
/** kernel module **/
#include <linux/rculist.h>


#endif /* USERSPACE */

enum parse_index {OBJECT_START, VER_TAG, VERSION, MSG, JSONL_BRACKET, NONCE, CMD,
                  PROBE, JSONR_BRACKET, OBJECT_END};
enum type { VERBOSE, ADD_NL, TRIM_TO_NL, UXP_NL, XP_NL, IN_FILE, USAGE };
enum type option_index = USAGE;
enum message_type {EMPTY, REQUEST, REPLY, COMPLETE};

#define MAX_LINE_LEN CONNECTION_MAX_MESSAGE
/**
 * TODO: do not make MAX_NONCE_SIZE to be the de-facto nonce size
 * allow shorter nonces, allowing the client to choose,
 * up to MAX_NONCE_SIZE
 **/
#define MAX_NONCE_SIZE 32
#define MAX_CMD_SIZE 128
#define MAX_ID_SIZE MAX_CMD_SIZE
#define MAX_TOKENS 64
struct jsmn_message;
struct jsmn_session;
static void dump_session(struct jsmn_session *s);
static int dump(const char *js, jsmntok_t *t, size_t count, int indent);

/** user space makes use of bsd queues </usr/include/sys/queue.h>
 *  in kernel, use instead <linux/list.h>, which has type-checking
 *  built into the macros and inline functions.
 **/

#ifdef USERSPACE
/**
 * break the SLIST_HEAD macro into two lines
 * so emacs c-mode indentation doesn't get confused.
 **/
SLIST_HEAD(session_head, jsmn_session) \
h_sessions;

#else
spinlock_t sessions_lock;
unsigned long sessions_flag;
struct list_head h_sessions;
static inline void init_jsonl_parser(void)
{
	INIT_LIST_HEAD_RCU(&h_sessions);
}

#endif /* kernel space */


/**
 * a jsmn_message is a single jsonl object, read and created by a connection
 * the jsonl control protocol expects messages to be a request and matching
 * replies (one or more).
 *
 * Each new request message automatically creates a new jsmn_session (below).
 * a successful session is one request message and at least one reply
 * message.
 *
 * A jsmn_session provides glue that binds a request to matching responses.
 *
 * The struct connection (defined in controller.h) initializes each jsmn_message
 * with the struct socket that produced the data (read or written).
 *
 * You may always relate a struct connection to a jsmn_message by their having
 * the same struct socket.
 *
 **/

struct jsmn_message
{
#ifdef USERSPACE
	STAILQ_ENTRY(jsmn_message) e_messages;
#else
	struct list_head e_messages;
#endif
	spinlock_t sl;
	jsmn_parser parser;
	jsmntok_t tokens[MAX_TOKENS];
	uint8_t *line;
	size_t len;
	int type;
	int count; /* token count */
	struct jsmn_session *s;
#ifdef USERSPACE
	FILE *file;
#else
	struct socket *socket;
#endif
};

/**
 * a session may have one request and multiple replies
 **/
struct jsmn_session
{
#ifdef USERSPACE
	SLIST_ENTRY(jsmn_session) sessions;
#else
	struct list_head session_entry;
#endif
	spinlock_t sl;
	uint8_t nonce[MAX_NONCE_SIZE];
	struct jsmn_message *req;
#ifdef USERSPACE
	STAILQ_HEAD(replies, jsmn_message) h_replies;
#else
	struct list_head h_replies;
#endif
	uint8_t cmd[MAX_CMD_SIZE];
	uint8_t probe_id[MAX_ID_SIZE];

};

uint8_t *add_nl_at_end(uint8_t *in, int len);

static inline int index_command(uint8_t *cmd, int bytes)
{
	static uint8_t *table[] = {
		"connect",
		"discovery",
		"off",
		"on",
		"increase",
		"decrease",
		"low",
		"default",
		"high",
		"adversarial",
		"reset",
		"records"
	};
	static int length[] = {1, 2, 2, 2, 1, 3, 1, 3, 1, 1, 3, 3};
	int i = 0;

	for(i = 0; i < NUM_COMMANDS; i++) {
		if (! memcmp(cmd, table[i], (length[i] < bytes) ? length[i] : bytes))
			return i;
	}
	return -JSMN_ERROR_INVAL;
}

/**
 * @brief requirements for cmd_table message parsing functions
 *
 * int (cmd_table[])(struct jsmn_message *m, int index);
 *
 * @return zero or COMPLETE upon success, < 0 if an error
 *        - if message is attached to a session, must call free_session
 *        - otherwise, call free_message
 *
 *        - if returns < 0, the caller will end up destroying the connection
 *        - if returns 0, connection will stay open.
 *
 *       the connection struct is not visible to this level of the parser,
 *       it correlates messages to an accepted socket, and can be re-used for
 *       multiple request-reply messages pairs.
 *
 *       either way (<0  or 0), the session and message(s) need to be freed.
 *
 *       free_session will un-allocate the session and free both messages
 *       attached to it (request and reply)
 *
 *       request messages not yet attached to a session can be freed by calling
 *       free_message.
 **/


static inline int
process_records_request(struct jsmn_message *m, int index);

static int
process_discovery_request(struct jsmn_message *m, int index);

static inline int
process_jsmn_cmd(struct jsmn_message *m, int index);
int (*cmd_table[])(struct jsmn_message *m, int index) =
{
	process_jsmn_cmd, /* connect */
	process_discovery_request, /* DISCOVERY */
	process_jsmn_cmd, /* OFF */
	process_jsmn_cmd, /* ON */
	process_jsmn_cmd, /* INCREASE */
	process_jsmn_cmd, /* DECREASE */
	process_jsmn_cmd, /* LOW */
	process_jsmn_cmd, /* DEFAULT */
	process_jsmn_cmd, /* HIGH */
	process_jsmn_cmd, /* ADVERSARIAL */
	process_jsmn_cmd, /* RESET */
	process_records_request /* RECORDS */
};

int verbose_flag = 0;
static inline void
free_session(struct jsmn_session *s);

/**
 * TODO: replace this for each type of response
 **/
static inline int
process_single_response(struct jsmn_session *s)
{
	if (verbose_flag)
		dump_session(s);
	free_session(s);
	return COMPLETE;
}


const uint8_t escape [] = {0x5c, 0x00};


/**
 * free_message - and don't free the socket (kernel space)
 * or close the file (user space). The other end of the
 * connection may write or read using this socket (file)
 **/
static inline void
free_message(struct jsmn_message *m)
{

	printk(KERN_DEBUG "m: %p, m->line: %p\n", m, m->line);

	if(m->line)
		kfree(m->line);


 	kfree(m);


	return;
}

static inline struct jsmn_message *
new_message(uint8_t *line, size_t len)
{
	struct jsmn_message *m;

	assert(line && line[len] == 0x00);

	if (!line || len > MAX_LINE_LEN || line[len] != 0x00) {
		return NULL;
	}
	m = kzalloc(sizeof(struct jsmn_message), GFP_KERNEL);
	if (!m) {
		return NULL;
	}
	m->line = line;
	m->len = len;
	return m;
}

static inline void
free_session(struct jsmn_session *s)
{

#ifdef USERSPACE
	while (! STAILQ_EMPTY(&s->h_replies)) {
		struct jsmn_message *m = STAILQ_FIRST(&s->h_replies);
		STAILQ_REMOVE_HEAD(&s->h_replies, e_messages);
		free_message(m);
	}
	SLIST_REMOVE(&h_sessions, s, jsmn_session, sessions);
#else
	{
		struct jsmn_message *m;
		spin_lock_irqsave(&sessions_lock, sessions_flag);
		rcu_read_lock();
		list_del_rcu(&s->session_entry);
		while (NULL != (m = list_first_or_null_rcu(&s->h_replies,
										   struct jsmn_message,
										   e_messages))) {
			printk(KERN_DEBUG "free reply message: %p\n", m);
			list_del_rcu(&m->e_messages);
			synchronize_rcu();
			free_message(m);

		}
		rcu_read_unlock();
		spin_unlock_irqrestore(&sessions_lock, sessions_flag);
		if (s->req) {

			free_message(s->req);

		}
		kfree(s);

	}
#endif
}


/**
 * assumes: we have already attached the message to
 * a session.
 * every message requires a command, and every message
 * EXCEPT for the discovery request requires a probe ID
 **/

static inline int
pre_process_jsmn_request_cmd(struct jsmn_message *m)
{
	uint8_t *c, *id = NULL;
	size_t c_bytes, id_bytes = 0;
	int ccode;

	assert(m && m->s);

	if (m->type == REPLY) {
		printk(KERN_DEBUG "pre-process request cmd received a reply message"
			   " %s:%d\n", __FILE__, __LINE__);
		return JSMN_ERROR_INVAL;
	}
	/* m->type is either REQUEST or REPLY */
    /* set up to copy or compare the session command */
	c = m->line + m->tokens[CMD].start;
	c_bytes = m->tokens[CMD].end - m->tokens[CMD].start;
	if (c_bytes <=0 || c_bytes >= MAX_CMD_SIZE)
		return JSMN_ERROR_INVAL;
	if (m->parser.toknext < PROBE) {
		/**
		 * there is no probe element, not valid
		 **/
		printk(KERN_DEBUG "received a jsonl request message with no probe id."
			   " %s:%d\n", __FILE__, __LINE__);
		goto err_out;
	} else {
		id = m->line + m->tokens[PROBE].start;
		id_bytes = m->tokens[PROBE].end - m->tokens[PROBE].start;
		if (id_bytes ==0 || id_bytes > MAX_CMD_SIZE) {
			printk(KERN_DEBUG "pre_process_jsmn_command: json command token is either"
				   "too small or too large for any valid commands: %ld bytes\n",
				   id_bytes);
			return JSMN_ERROR_INVAL;
		}
	}

	if (m->type == REQUEST) {
        /* copy the command into the session command array */
		memcpy(m->s->cmd, c, c_bytes);
		m->s->cmd[c_bytes] = 0x00;
		if (m->parser.toknext > PROBE) {
			memcpy(m->s->probe_id, id, id_bytes);
			m->s->probe_id[id_bytes] = 0x00;
		}
		ccode = index_command(c, c_bytes);
		if (ccode >= 0 && ccode <= RECORDS )
			return cmd_table[ccode](m, ccode);
	}

err_out:
	printk(KERN_DEBUG "invalid message type %s:%d\n", __FILE__, __LINE__);
	return JSMN_ERROR_INVAL;
}


/**
 * Process Request Messages
 * message is guaranteed to be connected to a session,
 * is typed as either a request or reply
 * nonce is msg->s->nonce
 * cmd is msg->s->cmd
 * probe is msg->s->probe_id
 *
 * - allocate a reply message struct, copy the struct, link to session
 * - session is allocated and attached to message but not locked.
 * - build the json reply message in the 'line' field
 * - link the reply to the session reply list
 * - write the reply to line to the socket
 * - free the session
 *
 * For Records replies:
 *   - link each reply to the session reply list (at least one,
 *     likely more than one)
 *   - call send_session_replies to write the replies to the socket
 *   - free the session
 **/

/**
 * Note:
 * Individual elements in the array can be cleared with::
 *  int flex_array_clear(struct flex_array *array, unsigned int element_nr);
 **/

/**
 * Records request:
 * find the struct probe, run it, read the flex array, clear each
 * array element after its read.
 **/

static int
process_records_request(struct jsmn_message *msg, int index)
{
	int ccode = 0;
	struct probe *probe_p = NULL;


	do {
		ccode = get_probe(msg->s->probe_id, &probe_p);
	} while (ccode == -EAGAIN);

	if (!ccode && probe_p != NULL) {
		uint8_t *buf = NULL;
		ssize_t len;

        /* send this probe a records request */
		probe_p->rcv_msg_from_probe(probe_p, RECORDS, (void **)&buf, &len);

		goto out_unlock;
	}
out_unlock:
	spin_unlock(&probe_p->lock);
	return ccode;
}


static int
process_discovery_request(struct jsmn_message *m, int index)
{

/**
 * TODO: optimize away redundant usage of string library functions
 **/
	struct jsmn_message *reply_msg = NULL;
	uint8_t *probe_ids = NULL;
	uint8_t *r_header = "{" PROTOCOL_VERSION ", reply: [";
	size_t probe_ids_len = 0;
	int ccode = 0;

	assert(m);
	printk(KERN_DEBUG "allocating a reply message size %ld\n",
		   sizeof(struct jsmn_message));

	reply_msg = kzalloc(sizeof(struct jsmn_message), GFP_KERNEL);
	printk(KERN_DEBUG "reply_msg, kzalloc returned: %p\n", reply_msg);


	if (!reply_msg) {
		ccode = -ENOMEM;

		goto out_session;
	}
	reply_msg->type = REPLY;

	/**
	 * copy some fields from the request msg to the rply msg
	 **/
	/* session */
	reply_msg->s = m->s;
	/* socket */
	reply_msg->socket = m->socket;

	/**
	 * build the discovery buffer
	 * '[probe ids]'
	 **/
	if (build_discovery_buffer(&probe_ids, &probe_ids_len) < 0) {
		kfree(reply_msg);
		ccode = -ENFILE;
		goto out_session;
	}

	/**
	 * allocate the message buffer (line), MAX_HEADER, to be realloc'd
	 * when we know the actual size.
	 * TODO: start with a smaller buffer and realloc bigger
	 **/
	reply_msg->line = kzalloc(CONNECTION_MAX_HEADER, GFP_KERNEL);
	if (!reply_msg->line) {

		ccode = -ENOMEM;
		goto out_reply_msg;
	} else {
		/**
	     * build the JSONL buffer
	     * '{Virtue-protocol-verion: 0.1, reply: [nonce, discovery, [probe ids]] }\n'
	     **/
		unsigned long replies_flag;
		size_t remaining = CONNECTION_MAX_HEADER - 1;
		strncat(reply_msg->line, r_header, remaining);
		remaining -= strlen(reply_msg->line);
		if (remaining <= probe_ids_len + 4) {
			goto out_reply_msg;
		}

		strncat(reply_msg->line, reply_msg->s->nonce, remaining);
		remaining -= strlen(reply_msg->line);
		if (remaining <= probe_ids_len + 4) {
			goto out_reply_msg;
		}

		strncat(reply_msg->line, ", discovery, ", remaining);
		remaining -= strlen(reply_msg->line);
		if (remaining <= probe_ids_len + 4) {
			goto out_reply_msg;
		}

		strncat(reply_msg->line, probe_ids, remaining);
		remaining -= strlen(reply_msg->line);
		if (remaining <= 4) {
			goto out_reply_msg;
		}

		strncat(reply_msg->line, "] }", remaining);
		reply_msg->line = add_nl_at_end(reply_msg->line, strlen(reply_msg->line));
		/**
		 * link the discovery reply message to the session
		 **/
		spin_lock_irqsave(&reply_msg->s->sl, replies_flag);
		list_add_tail_rcu(&reply_msg->e_messages, &reply_msg->s->h_replies);
		spin_unlock_irqrestore(&reply_msg->s->sl, replies_flag);

		/**
		 * write the reply message to the socket, then free the session
		 **/
		ccode = k_socket_write(reply_msg->socket,
							   strlen(reply_msg->line) + 1,
							   reply_msg->line,
							   0L);
		goto out_session;
	}

out_reply_msg:
	free_message(reply_msg);
out_session:
	free_session(m->s);
	return COMPLETE;
}

/**
 * message is guaranteed to be connected to a session,
 * is typed as either a request or reply
 **/
static inline int
process_jsmn_cmd(struct jsmn_message *m, int index)
{

	if (m->type == REPLY) {
		if (index != RECORDS) {
			return process_single_response(m->s);
		} else {
			return JSMN_ERROR_INVAL;
		}
	}
	return 0;
}


/**
 * garbage collection of sessions
 * every new session has one request message
 * a session is discriminated by the value of request->socket.socket
 * each socket may have exactly one pending request. If a new request
 * comes on an existing socket, the existing request->session is
 * garbage collected and a new session is established.
 *
 * a single request may elicit multiple replies, so this test
 * is not symmetric - e.g., we don't garbage-collect a session
 * after the first reply comes in on session->request->socket.socket
 *
 **/
static inline void
garbage_collect_session(struct jsmn_message *m)
{
	struct jsmn_session *s;
	assert(m && m->type == REQUEST);
	if (!m || m->type != REQUEST) {
		return;
	}
#ifdef USERSPACE
	SLIST_FOREACH(s, &h_sessions, sessions)
	{
		if (s && s->req && s->req->file == m->file)

		{
			free_session(s); /* removes s from h_sessions */
			return;
		}
	}
#else
	{
		rcu_read_lock();
		list_for_each_entry_rcu(s, &h_sessions, session_entry) {
			if (s && s->req && s->req->socket == m->socket){
				rcu_read_unlock();
				/* free session will handle locking and rcu garbage
				   collection. */
				free_session(s);
				return;
			}
		}
		rcu_read_unlock();
	}

#endif
}


static inline struct jsmn_session *get_session(struct jsmn_message *m)
{
	uint8_t *n;
	size_t bytes;
	struct jsmn_session *ns;

	n = m->line  + m->tokens[NONCE].start;
	bytes = m->tokens[NONCE].end - m->tokens[NONCE].start;

	if (bytes <=0 || bytes > MAX_NONCE_SIZE)
		return NULL;

	if (m->type == REQUEST) {
		garbage_collect_session(m);

/* new session */
		ns = kzalloc(sizeof(*ns), GFP_KERNEL);
		memcpy(ns->nonce, n, bytes);
		ns->req = m;
		m->s = ns;
#ifdef USERSPACE
		STAILQ_INIT(&ns->h_replies);
		SLIST_INSERT_HEAD(&h_sessions, ns, sessions);
#else
		INIT_LIST_HEAD_RCU(&ns->h_replies);
		spin_lock_irqsave(&sessions_lock, sessions_flag);
		rcu_read_lock();
		list_add_rcu(&ns->session_entry, &h_sessions);
		rcu_read_unlock();
		spin_unlock_irqrestore(&sessions_lock, sessions_flag);
#endif
		return ns;
	} else {
#ifdef USERSPACE
		SLIST_FOREACH(ns, &h_sessions, sessions)
		{
			if (! memcmp(ns->nonce, n, bytes)) {
				STAILQ_INSERT_TAIL(&ns->h_replies, m, e_messages);
			}
		}
#else
		rcu_read_lock();
		list_for_each_entry_rcu(ns, &h_sessions, session_entry) {
			if (! memcmp(ns->nonce, n, bytes)) {
				unsigned long replies_flag;
				spin_lock_irqsave(&ns->sl, replies_flag);
				list_add_tail_rcu(&m->e_messages, &ns->h_replies);
				spin_unlock_irqrestore(&ns->sl, replies_flag);
			}
			rcu_read_unlock();
		}
#endif
		m->s = ns;
		return ns;
	} /* m->type  == response */

	return NULL;
}


/**
 * in version 0.1, message must be either "request" or "reply"
 *
 **/

static inline int
check_protocol_message(struct jsmn_message *m)
{
	uint8_t *messages[] = {"request", "reply"};
	uint8_t *msg;
	size_t bytes;

	msg = m->line + m->tokens[MSG].start;
	bytes = m->tokens[MSG].end - m->tokens[MSG].start;
	assert(bytes == strlen(messages[0]) ||
		   bytes == strlen(messages[1]));
	m->type = JSMN_ERROR_INVAL;
	if (bytes == strlen(messages[0]) && ! memcmp(msg, messages[0], bytes)) {
		m->type = REQUEST;
	}

	if (bytes == strlen(messages[1]) && ! memcmp(msg, messages[1], bytes)) {
		m->type = REPLY;
	}

	return m->type;
}



static inline int
check_protocol_version(struct jsmn_message *m)
{
	uint8_t *start;
	size_t bytes;
	int ccode;
	jsmntok_t tag, version;


	static uint8_t *prot_tag = "Virtue-protocol-version";
	static uint8_t *ver_tag = "0.1";

	tag = m->tokens[VER_TAG];
	version = m->tokens[VER_TAG + 1];

	start = m->line  + tag.start;
	bytes = tag.end - tag.start;
	if (bytes != strlen(prot_tag)) {
		printk(KERN_DEBUG "bytes: %ld; tag len: %ld\n", bytes, strlen(prot_tag));


		return JSMN_ERROR_INVAL;
	}

	ccode = memcmp(prot_tag, start, bytes);
	if (ccode) {

		printk(KERN_INFO "message tag value is unexpected\n");
		return JSMN_ERROR_INVAL;
	}

	start = m->line + version.start;
	bytes = version.end - version.start;
	if (bytes != strlen(ver_tag)) {

		return JSMN_ERROR_INVAL;
	}

	ccode = memcmp(ver_tag, start, bytes);
	if (ccode) {

		printk(KERN_INFO "Protocol version value is unexpected\n");
		return JSMN_ERROR_INVAL;
	}
	return 0;
}

static inline int
validate_message_tokens(struct jsmn_message *m)
{
	int i = 0, len;

	assert(m);
	if (!m->line ||
		m->len >= MAX_LINE_LEN ||
		m->line[m->len] != 0x00) {


		return -EINVAL;
	}

	if (m->count < 1 ||
		m->count > MAX_TOKENS ||
		m->tokens[0].type != JSMN_OBJECT) {

		return -JSMN_ERROR_INVAL;
	}
	for (i = 0, len = m->len; i < m->count; i++) {
		if (m->tokens[i].start > len ||
			m->tokens[i].end > len ||
			m->tokens[i].end - m->tokens[i].start < 0) {

			return -JSMN_ERROR_INVAL;
		}
	}
	return 0;
}

/**
 * each message must a well-formed JSON object
 **/
int
parse_json_message(struct jsmn_message *m)
{
	int i = 0, ccode = 0;

	assert(m);
	if (!m->count) {
		jsmn_init(&m->parser);
		m->count = jsmn_parse(&m->parser,
							  m->line,
							  m->len,
							  m->tokens,
							  MAX_TOKENS);
	}

	printk(KERN_DEBUG "token count for msg: %d\n", m->count);

	if (m->count < 0 ) {
		printk(KERN_INFO "failed to parse JSON: %d\n", (int)m->count);
		return m->count;
	}
	assert(m->line && m->line[m->len] == 0x00);

	if (validate_message_tokens(m)) {
		printk(KERN_INFO "each message must be a well-formed JSON object" \
			   " %d\n", (int)m->count);
		return m->count;
	}

	/* count holds the number of tokens in the string */
	for (i = 1; i < m->count; i++) {
		/**
		 * we always expect tok[1].type to be JSON_STRING, and
		 * to be "Virtue-protocol-version", likewise tok[2].type to be
		 * JSON_PRIMITIVE, and should be equal to the current version "0.1"
		 **/

		switch (i) {
		case OBJECT_START:
		case OBJECT_END:
		{
			break;
		}

		case VER_TAG:
		{
			if (check_protocol_version(m)) {

				return JSMN_ERROR_INVAL;
			}
			i  = VERSION; /* we validated the tag and value, so increment the index */
			break;
		}

		case MSG:
		{
			/**
			 * this should always be the command, REQUEST or REPLY
			 */
			m->type = check_protocol_message(m);
			if (m->type == JSMN_ERROR_INVAL) {

				return m->type;
			}

			break;
		}
		case JSONL_BRACKET:
		case JSONR_BRACKET:
		{
			break;
		}

		case NONCE:
		{
			if (!(m->s = get_session(m))) {

				printk(KERN_INFO "unable to find session corresponding to message:");
				dump(m->line, m->tokens, m->parser.toknext, 0);
				free_message(m);
				return JSMN_ERROR_INVAL;
			}
			break;
		}
		case CMD:
		{
			/**
			 * pre_process_jsmn_cmd(m):
			 *
			 **/

			ccode = pre_process_jsmn_request_cmd(m);

			if (!ccode || ccode == COMPLETE)
				return 0;
			else {
				return JSMN_ERROR_INVAL;
			}
		}
		default:
			return JSMN_ERROR_INVAL;

		}
	}

	return m->count;
}
STACK_FRAME_NON_STANDARD(parse_json_message);


static void dump_session(struct jsmn_session *s)
{
	struct jsmn_message *reply;

	assert(s && s->req);
	dump(s->req->line, s->req->tokens, s->req->parser.toknext, 0);
#ifdef USERSPACE
	STAILQ_FOREACH(reply, &s->h_replies, e_messages)
#else
	rcu_read_lock();
	list_for_each_entry_rcu(reply, &s->h_replies, e_messages)
#endif
	{
		dump(reply->line, reply->tokens, reply->parser.toknext, 0);
	}
#ifndef USERSPACE
	rcu_read_unlock();
#endif
	return;
}
STACK_FRAME_NON_STANDARD(dump_session);


/*
 * An example of reading JSON from stdin and printing its content to stdout.
 * The output looks like YAML, but I'm not sure if it's really compatible.
 */

static int dump(const char *js, jsmntok_t *t, size_t count, int indent)
{
	int i, j, k;
	if (count == 0) {
		return 0;
	}
	if (t->type == JSMN_PRIMITIVE) {
		printk(KERN_INFO "%.*s", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_STRING) {
		printk(KERN_INFO "'%.*s'", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		printk(KERN_INFO "\n");
		j = 0;
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent; k++) printk(KERN_INFO "  ");
			j += dump(js, t+1+j, count-j, indent+1);
			printk(KERN_INFO ": ");
			j += dump(js, t+1+j, count-j, indent+1);
			printk(KERN_INFO "\n");
		}
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		printk(KERN_INFO "\n");
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent-1; k++) printk(KERN_INFO "  ");
			printk(KERN_INFO "   - ");
			j += dump(js, t+1+j, count-j, indent+1);
			printk(KERN_INFO "\n");
		}
		return j+1;
	}
	return 0;
}
STACK_FRAME_NON_STANDARD(dump);

uint8_t *add_nl_at_end(uint8_t *in, int len)
{
	uint8_t *c, *end = in + len;
	assert(*end == 0);
	c = krealloc(in, len + (2 * sizeof(uint8_t)), GFP_KERNEL);
	assert(c);
	*(c + len) = 0x0a;
	*(c + len + 1) = 0x00;
	return c;
}
STACK_FRAME_NON_STANDARD(add_nl_at_end);


/**
 * trim the string to first un-escaped newline
 **/
uint8_t *trim_to_nl(uint8_t *in, int len)
{
	uint8_t *c, *end = in + len;
	assert(*end == 0);
	c = strchr(in, 0x0a);
	while (c && c > in) {
		if ( *(c - 1) == 0x5c) {
			c++;
			c = strchr(c, 0x0a);
		} else {
			*c = 0;
			return krealloc(in, c - in, GFP_KERNEL);
		}
	}
	return in;
}
STACK_FRAME_NON_STANDARD(trim_to_nl);



uint8_t *unescape_newlines(uint8_t *in, int len)
{
	int count = 0;
	uint8_t *save = in;
	uint8_t *end = in + len;
	assert(*end == 0);

	/* assumes: in is null-terminated */
	do {
		in = strpbrk(in, escape);
		if (in) {
			if ( (in + 1) != NULL) {
				if ( *(in + 1) == 0x0a || *(in + 1) == 0x0d ) {
					/* shorten the string */
					uint8_t *p = in + 1;
					uint8_t *s = in;
					while (*p) {
						*s = *p;
						p++;
						s++;
					}
					/* re-terminate the shorter string */
					*s = *p;
					count++;
				}
			}
			in++;
		}
	} while (in);
	if (count) {
		return krealloc(save, len - count, GFP_KERNEL);
	}
	return save;
}
STACK_FRAME_NON_STANDARD(unescape_newlines);

/**
 * really slow and crude, but let's only allocate new memory
 * if we need to
 **/

uint8_t *escape_newlines(uint8_t *in, int len)
{
	int count = 0;
	uint8_t *c, *p, *end = in + len;
	static const uint8_t nl [] = {0x0d, 0x0a, 0x00};
	assert(*end == 0);

	c = strpbrk(in, nl);
	while (c && c < end) {
		count++;
		c = strpbrk(++c, nl);
	}
	if (count) {
		c = in = krealloc (in,  len + count + 1, GFP_KERNEL);
		assert(c);
		*(c + len + count) = 0x00;
		p = c;
		while (*p && count) {
			if (*p != 0x0a && *p != 0x0d) {
				*c++ = *p++;
			} else {
				*(p + 1) = *p;
				p += 2;
				*c = 0x5c;
				c += 2;
				count--;
			}
		}
	}
	return in;
}
STACK_FRAME_NON_STANDARD(escape_newlines);

