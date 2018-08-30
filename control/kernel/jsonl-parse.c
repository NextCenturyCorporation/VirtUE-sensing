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
#include "controller.h"
#include "controller-linux.h"

#endif /* USERSPACE */


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
void init_jsonl_parser(void)
{
	INIT_LIST_HEAD_RCU(&h_sessions);
}

#endif /* kernel space */


enum parse_index {OBJECT_START, VER_TAG, VERSION, MSG, JSONL_BRACKET, NONCE, CMD,
                  SENSOR, JSONR_BRACKET, OBJECT_END};
enum type { VERBOSE, ADD_NL, TRIM_TO_NL, UXP_NL, XP_NL, IN_FILE, USAGE };
enum type option_index = USAGE;
enum message_type {EMPTY, REQUEST, REPLY, COMPLETE};

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

static int
process_state_request(struct jsmn_message *m, int index);

static int
process_records_request(struct jsmn_message *m, int index);

static int
process_discovery_request(struct jsmn_message *m, int index);

static inline int
process_jsmn_cmd(struct jsmn_message *m, int index);
int (*cmd_table[])(struct jsmn_message *m, int index) =
{
	process_jsmn_cmd, /* connect */
	process_discovery_request, /* DISCOVERY */
	process_state_request, /* OFF */
	process_state_request, /* ON */
	process_state_request, /* INCREASE */
	process_state_request, /* DECREASE */
	process_state_request, /* LOW */
	process_state_request, /* DEFAULT */
	process_state_request, /* HIGH */
	process_state_request, /* ADVERSARIAL */
	process_state_request, /* RESET */
	process_records_request /* RECORDS */
};

int verbose_flag = 0;

void
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
void
free_message(struct jsmn_message *m)
{
	if(m->line)
		kfree(m->line);
 	kfree(m);
	return;
}

struct jsmn_message *
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

void
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
		spin_lock(&sessions_lock);
		list_del_rcu(&s->session_entry);
		spin_unlock(&sessions_lock);

		while (NULL != (m = list_first_or_null_rcu(&s->h_replies,
												   struct jsmn_message,
												   e_messages))) {
			list_del(&m->e_messages);
			free_message(m);
		}
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
 * EXCEPT for the discovery request requires a sensor ID
 **/

static int
pre_process_jsmn_request_cmd(struct jsmn_message *m)
{
	uint8_t *c, *name = NULL;
	size_t c_bytes, name_bytes = 0;
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
	if (m->parser.toknext < SENSOR) {
		/**
		 * there is no sensor element, not valid
		 **/
		printk(KERN_DEBUG "received a jsonl request message with no sensor name."
			   " %s:%d\n", __FILE__, __LINE__);
		goto err_out;
	} else {
		name = m->line + m->tokens[SENSOR].start;
		name_bytes = m->tokens[SENSOR].end - m->tokens[SENSOR].start;
		if (name_bytes ==0 || name_bytes > MAX_NAME_SIZE) {
			printk(KERN_DEBUG "json name token is either"
				   "too small or too large for a sensor name or uuid: %ld bytes\n",
				   name_bytes);
			return JSMN_ERROR_INVAL;
		}
	}

	if (m->type == REQUEST) {
        /* copy the command into the session command array */
		memcpy(m->s->cmd, c, c_bytes);
		m->s->cmd[c_bytes] = 0x00;
		if (m->parser.toknext > SENSOR) {
			memcpy(m->s->sensor_name, name, name_bytes);
			m->s->sensor_name[name_bytes] = 0x00;
			printk(KERN_DEBUG "request sensor name: %s\n", m->s->sensor_name);

		}
		ccode = index_command(c, c_bytes);
		if (ccode >= 0 && ccode <= RECORDS ) {
			return cmd_table[ccode](m, ccode);
		}
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


static struct jsmn_message *
allocate_reply_message(struct jsmn_message *request, size_t alloc_buffer)
{
	struct jsmn_message *rply = NULL;

	rply = kzalloc(sizeof(struct jsmn_message), GFP_KERNEL);
	if (!rply) {
		return NULL;
	}
	if(alloc_buffer) {
		rply->line = kzalloc(alloc_buffer, GFP_KERNEL);
		if (!rply->line) {
			goto err_out_rply;
		}
		rply->len = alloc_buffer;
	}

	/**
	 * copy some fields from the request msg to the rply msg
	 **/
	/* session */
	rply->s = request->s;
	/* socket */
	rply->socket = request->socket;
	return rply;

err_out_rply:
	kfree(rply);
	return NULL;
}

/**
 * Records request:
 * find the struct probe, run it, read the flex array, clear each
 * array element after its read.
 **/


static int
write_session_replies(struct jsmn_session *session)
{
	struct jsmn_message *msg = NULL;
	ssize_t ccode;

	rcu_read_lock();
	if(list_empty(&session->h_replies)) {
		ccode = -ENOENT;
		goto err_socket_write;
	}

	list_for_each_entry_rcu(msg, &session->h_replies, e_messages) {
		ccode = k_socket_write(msg->socket,
							   strlen(msg->line) + 1,
							   msg->line,
						 	   0L);
		if (ccode < 0) {
			goto err_socket_write;
		}
	}
	ccode = 0;
err_socket_write:
	rcu_read_unlock();
	return ccode;
}



static int
process_state_request(struct jsmn_message *m, int index)
{
	int ccode = 0, write_ccode = 0;
	struct sensor *sensor_p = NULL;
	struct jsmn_message *s_reply = NULL;

	struct state_request sreq = {
		.cmd = index,
	};

	struct state_reply srep = {
		.cmd = index,
	};

	struct sensor_msg sm = {
		.id = index,
		.ccode = 0,
		.input = &sreq,
		.input_len = sizeof(struct state_request),
		.output = &srep,
		.output_len = sizeof(struct state_reply),
	};

	do {
		ccode = get_sensor(m->s->sensor_name, &sensor_p);
	} while (ccode == -EAGAIN);

	if (ccode < 0) {
		/**
		 * TODO: return an empty discovery response or some json object
		 * that tells the client the target probe was not found.
		 **/
		printk(KERN_DEBUG "could not find a matching sensor, exiting %d\n",
			   ccode);
		return ccode;
	}
	if (!ccode && sensor_p != NULL) {
	    /**
	     * sensor is LOCKED
	     **/
		ccode = sensor_p->message(sensor_p, &sm);
		/* UNLOCK sensor! */
		spin_unlock(&sensor_p->lock);
		if (ccode >= 0) {
			int bytes = 0;
			s_reply = allocate_reply_message(m, m->len * 2);
			if (!s_reply) {
				ccode = -ENOMEM;
				goto err_exit;
			}
			bytes = scnprintf(s_reply->line,
							  s_reply->len - 1,
							  "{%s, reply: [%s, %s]}\n",
							  PROTOCOL_VERSION,
							  s_reply->s->nonce,
							  cmd_strings[srep.state]);
			s_reply->line = krealloc(s_reply->line, bytes + 1, GFP_KERNEL);
			s_reply->len = bytes + 1;
			spin_lock(&s_reply->s->sl);
			list_add_tail_rcu(&s_reply->e_messages,
							  &s_reply->s->h_replies);
			spin_unlock(&s_reply->s->sl);
			write_ccode = write_session_replies(s_reply->s);
		} else {
			goto err_exit;
		}
	}

err_exit:
	free_session(m->s);
	if (ccode < 0)
		return ccode;
	if (write_ccode < 0)
		return write_ccode;
	return COMPLETE;
}


static int
process_records_request(struct jsmn_message *msg, int index)
{
	int ccode = 0, write_ccode = 0;
	struct sensor *sensor_p = NULL;
	struct jsmn_message *r_reply = NULL;

	struct records_request rr = {
		.json_msg = msg,
		.run_probe = 1,
		.clear = 1,
		.index = 0,
		.range = -1
	};

	struct records_reply rp = {};

	struct sensor_msg pm = {
		.id = RECORDS,
		.ccode = 0,
		.input = &rr,
		.input_len = sizeof(struct records_request),
		.output = &rp,
		.output_len = sizeof(struct records_reply),
	};

	get_random_bytes(&rr.nonce, sizeof(uint64_t));

	/**
	 * get_sensor will work with either the sensor name or the sensor
	 * uuid as the key. It first tests the key to see if it is a uuid,
	 * and if so tries to find the sensor by uuid.
	 *
	 * If the uuid search fails, it tries to find the sensor by name.
	 **/
	do {
		ccode = get_sensor(msg->s->sensor_name, &sensor_p);
	} while (ccode == -EAGAIN);

	if (ccode < 0) {
		/**
		 * TODO: return an empty discovery response or some json object
		 * that tells the client the target probe was not found.
		 **/
		printk(KERN_DEBUG "could not find a matching sensor, exiting %d\n",
			   ccode);
		return ccode;
	}

	if (!ccode && sensor_p != NULL) {
        /* send this probe a records request */
		/* will return 0 or error if no record. */
		/* each record is encapsulated in a json object and copied into */
		/* a reply message, and linked to the session */
		do {
			rp.records = kzalloc(CONNECTION_MAX_REPLY, GFP_KERNEL);
			rp.records_len = CONNECTION_MAX_REPLY;
			if (! rp.records) {
				ccode = -ENOMEM;
				break;
			}

			ccode = sensor_p->message(sensor_p, &pm);
			/**
			 * rp.records contains the json record object(s)
			 * rp.records_len contains the length of the object(s)
			 **/
			if (ccode >= 0) {
				r_reply = allocate_reply_message(msg, 0);
				if (r_reply) {
					/**
					 * response was built in sensor_p->message, assign
					 * that buffer to this reply, then
					 * link the reply message to the session
					 **/
					r_reply->line = rp.records;
					r_reply->len  = rp.records_len;
					spin_lock(&r_reply->s->sl);
					list_add_tail_rcu(&r_reply->e_messages,
									  &r_reply->s->h_replies);
					spin_unlock(&r_reply->s->sl);
				}
			}
			/**
			 * re-initialize the request struct to reflect where we are in the loop
			 **/
			rr.run_probe = 0;
			rr.index++;
		} while(!ccode && rp.index != -ENOENT);
		spin_unlock(&sensor_p->lock);
		sensor_p = NULL;
	}

	/**
     * even if one of the records exhuasted memory or
	 * caused an error, we may have been able to link more than one
	 * good record to the session as a reply message.  Try to write
	 * as many records to the socket as we can.
	 *
	 * write_session_replies will return -ENOENT if there are no replies
	 * linked into the session.
	 **/
	write_ccode = write_session_replies(msg->s);

    /**
	 * ERROR CODE returns:
	 * There are two possible error codes, in priority:
	 *
	 * 1. ccode < 0 means we ran out of memory trying to stuff a record
	 * into a json response object
	 *
	 * 2. write_ccode < 0 means we experienced an error writing one record
	 * to the message socket. We may have written one or more records
	 * before experiencing the error.
	 *
	 * if there is an error, return ccode firstly, write_ccode secondly
	 *
	 * If no error, return COMPLETE (a positive enumerated value)
	 **/

	free_session(msg->s);
	if (ccode < 0)
		return ccode;
	if (write_ccode < 0)
		return write_ccode;
	return COMPLETE;
}

static int
process_discovery_request(struct jsmn_message *m, int index)
{

/**
 * TODO: optimize away redundant usage of string library functions
 **/
	struct jsmn_message *reply_msg = NULL;
	uint8_t *probe_ids = NULL;
	uint8_t *r_header = "{" PROTOCOL_VERSION ", \'reply\': [";
	size_t probe_ids_len = 0;
	int ccode = 0;

	assert(m);
	reply_msg = kzalloc(sizeof(struct jsmn_message), GFP_KERNEL);
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
	     **/
		ssize_t orig_len = CONNECTION_MAX_HEADER - 1;

		orig_len = scnprintf(reply_msg->line,
							orig_len,
							"%s\'%s\', \'discovery\', %s]}",
							r_header,
							reply_msg->s->nonce,
							probe_ids);


		reply_msg->line = krealloc(reply_msg->line, orig_len + 1, GFP_KERNEL);

		/**
		 * link the discovery reply message to the session
		 **/
		spin_lock(&reply_msg->s->sl);
		list_add_tail_rcu(&reply_msg->e_messages, &reply_msg->s->h_replies);
		spin_unlock(&reply_msg->s->sl);

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
void
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


struct jsmn_session *
get_session(struct jsmn_message *m)
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
		spin_lock(&sessions_lock);
		list_add_rcu(&ns->session_entry, &h_sessions);
		spin_unlock(&sessions_lock);
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
		list_for_each_entry_rcu(ns, &h_sessions, session_entry){
			if (! memcmp(ns->nonce, n, bytes)) {
				spin_lock(&ns->sl);
				list_add_tail_rcu(&m->e_messages, &ns->h_replies);
				spin_unlock(&ns->sl);
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

static int
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



static int
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

static int
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


void dump_session(struct jsmn_session *s)
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

int dump(const char *js, jsmntok_t *t, size_t count, int indent)
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
				c++; p++;
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

