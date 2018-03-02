#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include <errno.h>
#include "jsmn/jsmn.h"

typedef char uint8_t;
#define GFP_KERNEL 1
#define krealloc(p, t, f)						\
	realloc((p), (t))
#define kzalloc(s, f) calloc((s), sizeof(uint8_t))
#define printk printf
#define KERN_INFO ""

const uint8_t escape [] = {0x5c, 0x00};
int verbose_flag = 0;

static inline void *__krealloc(void *buf, size_t s) 
{
	void *p = krealloc(buf, s, GFP_KERNEL);
	if (!p) {
		free(buf);
		printk("realloc(): errno=%d\n", ENOMEM);
	}
	return p;
}


/*
 * An example of reading JSON from stdin and printing its content to stdout.
 * The output looks like YAML, but I'm not sure if it's really compatible.
 */

static int dump(const char *js, jsmntok_t *t, size_t count, int indent) {
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

uint8_t *empty(uint8_t *in, int len)
{
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


enum type { VERBOSE, ADD_NL, TRIM_TO_NL, UXP_NL, XP_NL, IN_FILE, USAGE };
uint8_t *input_string = NULL, *output_string = NULL, *in_file_name  = NULL;
enum type option_index = USAGE;
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
			FILE *in_file;
			ssize_t nread, len = 0;
			uint8_t *line = NULL;

			in_file_name = strdup(optarg);
			in_file = fopen(in_file_name, "r");
			if (in_file == NULL) {
				perror("fopen");
				exit(EXIT_FAILURE);
			}
		again:
			while((nread = getline(&line, &len, in_file)) != -1) {
				jsmn_parser p;
				jsmntok_t *tok;
				size_t tokcount = 0x10;

				jsmn_init(&p);
				tok = kzalloc(sizeof(jsmntok_t) * tokcount, GFP_KERNEL);
				if (tok == NULL) {
					fprintf(stderr, "kzalloc(): errno %d\n", errno);
					exit(EXIT_FAILURE);
				}
				int ccode = jsmn_parse(&p, line, len, tok, tokcount);
				if (ccode < 0) {
					if (ccode == JSMN_ERROR_NOMEM) {
						tokcount *= 2;
						tok = __krealloc(tok, sizeof(*tok) * tokcount);
						if (!tok) {
							exit(EXIT_FAILURE);
						}
						goto again;
					}
				} else if (verbose_flag){
					dump(line, tok, p.toknext, 0);
				}
				free(line);
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
