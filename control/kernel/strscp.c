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
#include <linux/list.h>


#endif /* USERSPACE */

enum parse_index {OBJECT, VER_TAG, VERSION, MSG = 3, NONCE = 5, CMD = 6,
                  PROBE = 7, RECORD = 9};
enum type { VERBOSE, ADD_NL, TRIM_TO_NL, UXP_NL, XP_NL, IN_FILE, USAGE };
enum type option_index = USAGE;
enum message_type {EMPTY, REQUEST, REPLY, COMPLETE};
enum message_command {DISCOVERY = 0, OFF = 1, ON = 2, INCREASE = 3, DECREASE = 4,
					  LOW = 5, DEFAULT = 6, HIGH = 7, ADVERSARIAL = 8, RESET = 9, RECORDS = 10};

#define MAX_LINE_LEN 4096
#define MAX_NONCE_SIZE 128
#define MAX_CMD_SIZE 64
#define MAX_ID_SIZE MAX_CMD_SIZE
#define MAX_TOKENS 64
struct jsmn_message;
struct jsmn_session;
static void dump_session(struct jsmn_session *s);
static int dump(const char *js, jsmntok_t *t, size_t count, int indent);

/** bsd queue
 *  in kernel, replace with <linux/list.h>
 **/

#ifdef USERSPACE
/**
 * break the SLIST_HEAD macro into two lines
 * so emacs c-mode indentation doesn't get confused.
 **/
SLIST_HEAD(session_head, jsmn_session) \
h_sessions;

/**
 * for linux kernel compatibility
 **/
struct sock
{
	int socket;
};

#else

LIST_HEAD(h_sessions);

#endif /* kernel space */

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
	size_t count; /* token count */
	struct jsmn_session *s;
	struct sock socket;
};

/**
 * a session may have one request and multiple replies
 **/
struct jsmn_session
{
#ifdef USERSPACE
	SLIST_ENTRY(jsmn_session) sessions;
#else
	struct list_head sessions;
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

static inline int index_command(uint8_t *cmd, int bytes)
{
	static uint8_t *table[] = {
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
	static int length[] = {2, 2, 2, 2, 3, 2, 3, 2, 2, 3, 3};
	int i = 0;

	for(i = 0; i < 11; i++) {
		if (! memcmp(cmd, table[i], (length[i] < bytes) ? length[i] : bytes))
			return i;
	}
	return -JSMN_ERROR_INVAL;
}


static inline int
process_records_cmd(struct jsmn_message *m, int index);

static inline int
process_jsmn_cmd(struct jsmn_message *m, int index);
int (*cmd_table[])(struct jsmn_message *m, int index) =
{
	process_jsmn_cmd, /* DISCOVERY */
	process_jsmn_cmd, /* OFF */
	process_jsmn_cmd, /* ON */
	process_jsmn_cmd, /* INCREASE */
	process_jsmn_cmd, /* DECREASE */
	process_jsmn_cmd, /* LOW */
	process_jsmn_cmd, /* DEFAULT */
	process_jsmn_cmd, /* HIGH */
	process_jsmn_cmd, /* ADVERSARIAL */
	process_jsmn_cmd, /* RESET */
	process_records_cmd /* RECORDS */
};

int verbose_flag = 0;
static inline void
free_session(struct jsmn_session *s);

static inline int
process_single_response(struct jsmn_session *s)
{
	if (verbose_flag)
		dump_session(s);
	free_session(s);
	return COMPLETE;
}


const uint8_t escape [] = {0x5c, 0x00};

static inline void *__krealloc(void *buf, size_t s)
{
	void *p = krealloc(buf, s, GFP_KERNEL);
	if (!p) {
		free(buf);
		printk("realloc(): errno=%d\n", ENOMEM);
	}
	return p;
}


static inline void
free_message(struct jsmn_message *m)
{
	return;
	if(m->line)
		kfree(m->line);
	kfree(m);
	return;
}


static inline void
free_session(struct jsmn_session *s)
{
	if (s->req)
		free_message(s->req);
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
		while ((m = list_first_entry_or_null(&s->h_replies,
											 jsmn_message,
											 h_replies))) {
			list_del(&m->e_messages);
		}
	}
	list_del(&s->sessions);
#endif
	kfree(s);
}



/**
 * assumes: we have already attached the message to
 * a session
 **/
static inline int
process_records_cmd(struct jsmn_message *m, int index)
{
	/**
	 * every record command must be targeted to a probe id
	 * the id may be a wildcard, we assume the command
	 * and id are both stored in the message struct
	 **/

	if (m->type == REQUEST) {
		/* here is where to call in to the probe */
		printk(KERN_INFO "Dispatching a record request to %s, %s\n",
			   m->s->probe_id, m->s->nonce);
		return 0;
	} else if (m->type == REPLY) {
		int bytes;
		uint8_t *r;
		if (m->parser.toknext <= RECORD || m->parser.toknext >= MAX_TOKENS) {
			printk(KERN_INFO "parser did not find a response record %s, %s\n",
				   m->s->probe_id, m->s->nonce);
			return process_single_response(m->s);
		}

		r = m->line + m->tokens[RECORD].start;
		bytes = m->tokens[RECORD].end - m->tokens[RECORD].start;
		printk(KERN_INFO "Received a record from %s, %s, %.*s\n",
			   m->s->probe_id, m->s->nonce, bytes, r);

	}

	return 0;

}



/**
 * assumes: we have already attached the message to
 * a session.
 * every message requires a command, and every message
 * EXCEPT for the discovery request requires a probe ID
 **/

static inline int
pre_process_jsmn_cmd(struct jsmn_message *m)
{
	uint8_t *string, *c, *id;
	size_t c_bytes, id_bytes;
	int ccode;

	assert(m && m->s);

	/* m->type is either REQUEST or REPLY */
    /* set up to copy or compare the session command */
	c = m->line + m->tokens[CMD].start;
	c_bytes = m->tokens[CMD].end - m->tokens[CMD].start;
	if (c_bytes <=0 || c_bytes >= MAX_CMD_SIZE)
		return JSMN_ERROR_INVAL;
	if (m->parser.toknext <= PROBE) {
		/**
		 * there is no probe element, only valid
		 * for a discovery request message.
		 **/
	} else {
		id = m->line + m->tokens[PROBE].start;
		id_bytes = m->tokens[PROBE].end - m->tokens[PROBE].start;
		if (id_bytes <=0 || id_bytes >= MAX_CMD_SIZE)
			return JSMN_ERROR_INVAL;
	}


	if (m->type == REQUEST) {
        /* copy the command into the session command array */
		memcpy(m->s->cmd, c, c_bytes);
		m->s->cmd[c_bytes] = 0x00;
		if (m->parser.toknext > PROBE) {
			memcpy(m->s->probe_id, id, id_bytes);
			m->s->probe_id[id_bytes] = 0x00;
		}

	} else if (m->type == REPLY) {
		/* m is linked into the session tail q */
		/* nonce and command match */
		/* replies must have an id */
        /* a discovery reply may have multiple probe id strings */
		/* if a reply, nonce, session, and cmd must be identical */
		if (m->parser.toknext <= PROBE) {
			return JSMN_ERROR_INVAL;
		}
	}

	ccode = index_command(c, c_bytes);
	if (ccode >= 0 && ccode <= RECORDS )
		return cmd_table[ccode](m, ccode);
	return ccode;
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
#else
		list_for_each_entry(s, &h_sessions, sessions)
#endif
	{

		if (s && s->req && s->req->socket.socket == m->socket.socket) {
			free_session(s); /* removes s from h_sessions */
			return;
		}
	}
}


static inline struct jsmn_session *get_session(struct jsmn_message *m)
{
	uint8_t *n;
	size_t bytes;
	int i;
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
		INIT_LIST_HEAD(&ns->h_replies);
		list_add(ns, &h_sessions);
#endif
		return ns;
	} else {
#ifdef USERSPACE
		SLIST_FOREACH(ns, &h_sessions, sessions)
#else
			list_for_each_entry(ns, &h_sessions, sessions)
#endif
		{

			if (! memcmp(ns->nonce, n, bytes)) {
#ifdef USERSPACE
				STAILQ_INSERT_TAIL(&ns->h_replies, m, e_messages);
#else
				list_add_tail(m, &ns->h_replies);
#endif
				m->s = ns;
				return ns;
			}
		}
	}
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
	if (bytes != strlen(prot_tag))
		return JSMN_ERROR_INVAL;
	ccode = memcmp(prot_tag, start, bytes);
	if (ccode) {
		printk(KERN_INFO "message tag value is unexpected\n");
		return JSMN_ERROR_INVAL;
	}

	start = m->line + version.start;
	bytes = version.end - version.start;
	if (bytes != strlen(ver_tag))
		return JSMN_ERROR_INVAL;
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
	struct jsmn_session *message_session = NULL;

	assert(m);
	jsmn_init(&m->parser);
	m->count = jsmn_parse(&m->parser,
						  m->line,
						  m->len,
						  m->tokens,
						  MAX_TOKENS);
	if (m->count < 0 ) {
		printk(KERN_INFO "failed to parse JSON: %d\n", m->count);
		return m->count;
	}
	assert(m->line && m->line[m->len] == 0x00);
	if (validate_message_tokens(m)) {
		printk(KERN_INFO "each message must be a well-formed JSON object" \
			   " %d\n", m->count);
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
		case 4:
		{
			/* opening left bracket of message content object */

			break;
		}
		case NONCE:
		{
			m->s = get_session(m);
			break;
		}
		case CMD:
		{
			ccode = pre_process_jsmn_cmd(m);

			if (!ccode || ccode == COMPLETE)
				return 0;
			else
				return JSMN_ERROR_INVAL;
		}

		default:
			return JSMN_ERROR_INVAL;
		}
	}

	return m->count;
}


static void dump_session(struct jsmn_session *s)
{
	struct jsmn_message *reply;

	assert(s && s->req);
	dump(s->req->line, s->req->tokens, s->req->parser.toknext, 0);
#ifdef USERSPACE
	STAILQ_FOREACH(reply, &s->h_replies, e_messages)
#else
	list_for_each_entry(reply, &s->h_replies, e_messages)
#endif
	{
		dump(reply->line, reply->tokens, reply->parser.toknext, 0);
	}
	return;
}


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
		printf("%.*s", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_STRING) {
		printf("'%.*s'", t->end - t->start, js+t->start);
		return 1;
	} else if (t->type == JSMN_OBJECT) {
		printf("\n");
		j = 0;
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent; k++) printf("  ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf(": ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf("\n");
		}
		return j+1;
	} else if (t->type == JSMN_ARRAY) {
		j = 0;
		printf("\n");
		for (i = 0; i < t->size; i++) {
			for (k = 0; k < indent-1; k++) printf("  ");
			printf("   - ");
			j += dump(js, t+1+j, count-j, indent+1);
			printf("\n");
		}
		return j+1;
	}
	return 0;
}

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


/**
 * trim the string to first un-escaped newline
 **/
uint8_t *trim_to_nl(uint8_t *in, int len)
{
	int count = 0;
	uint8_t *c, *out, *end = in + len;
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

#ifdef USERSPACE

uint8_t *empty(uint8_t *in, int len)
{
	assert(0);
	return NULL;
}

uint8_t *(*function_table[])(uint8_t *, int) =
{
	empty,
	add_nl_at_end,
	trim_to_nl,
	unescape_newlines,
	escape_newlines
};
char *program_name;
void usage(void)
{
	printf("%s --verbose <test>,", program_name);
	printf (" where <test> is one of:\n");
	printf ("\t --add-nl <string> --trim-to-nl <string> " \
			"--unescape-nl <string> --escape-nl <string>\n");
	printf ("\t --file <filename>\n");
	exit(0);
}


uint8_t *input_string = NULL, *output_string = NULL, *in_file_name  = NULL;

static inline int
get_options (int argc, char **argv)
{
	if (argc < 2)
		usage ();
	while (1) {
		int c;
		static struct option long_options[] = {
			{"verbose", no_argument, &verbose_flag, 1},
			{"add-nl", required_argument, 0, 'a'},
			{"trim-to-nl", required_argument, 0, 't'},
			{"unescape-nl", required_argument, 0, 'u'},
			{"escape-nl", required_argument, 0, 'x'},
			{"in-file", required_argument, 0, 'i'},
			{"help", no_argument, NULL, 0},
			{0, 0, 0, 0}
		};

		c = getopt_long_only (argc, argv, "va:t:u:e:h",
							  long_options, (int *)&option_index);
		if (c == -1)
			break;

		switch(option_index) {
		case VERBOSE:
		{
			break;
		}

		case ADD_NL:		/* add a newline */
		case TRIM_TO_NL:		/* trim to the first newline */
		case UXP_NL:		/* un-escape escaped newlines */
		case XP_NL:		{   /* escape newlines  */
			int input_len, output_len;
			input_string = strdup(optarg);
			input_len = strlen(input_string);
			output_string = function_table[option_index](input_string,
														 input_len);

			output_len = strlen(output_string);
			if (verbose_flag) {
				printf("option: %d\ninput %d: %s\noutput %d:%s\n",
					   option_index,
					   input_len, optarg,
					   output_len, output_string);
			}
			break;
		}
		case IN_FILE:
		{
			/**
			 * TODO: make message tokens realloc'd but keep the MAX_TOKENS
			 *       limit, otherwise vulnerable to a DOS attack from a message
			 *       like '{[[[[[[[[[[[[[[[[[[[[ ... ]]]]]]]]]]]]]]]]]]]]}'
			 *       that would cause the parser to realloc tokens and exhuast
			 *       memory
			 * TODO: garbage-collect partial sessions (req only)
			 * TODO: clean up replies with no matching request
			 **/
			FILE *in_file;
			ssize_t nread, len = 0;
			uint8_t *line = NULL;
#ifdef USERSPACE
			SLIST_INIT(&h_sessions);
#else
			INIT_LIST_HEAD(&h_sessions);
#endif
			in_file_name = strdup(optarg);
			in_file = fopen(in_file_name, "r");
			if (in_file == NULL) {
				perror("fopen");
				exit(EXIT_FAILURE);
			}


			while((nread = getline(&line, &len, in_file)) != -1) {
				struct jsmn_message *this_msg =
					kzalloc(sizeof(struct jsmn_message), GFP_KERNEL);
				if (!this_msg) {
					exit(EXIT_FAILURE);
				}
				this_msg->line = line;
				this_msg->len = strlen(line);
				/* the following call will free a request/response pair and its
				 * session structure.
				 */
				parse_json_message(this_msg);
				line = NULL;
				len = 0;
			}
			fclose(in_file);
			exit(EXIT_SUCCESS);
			break;
		}
		case USAGE:		/* help */
		default:
			usage();
		}
	}

}

int main(int argc, uint8_t **argv)
{
	int i;
	program_name = strdup(argv[0]);
	return get_options(argc, argv);
}

#endif /* USERSPACE */
