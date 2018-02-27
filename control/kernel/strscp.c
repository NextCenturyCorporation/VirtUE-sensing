#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>
#include "jsmn/jsmn.h"

#define GFP_KERNEL 1
#define krealloc(p, t, f)					\
				 realloc((p), (t))
typedef char uint8_t;
const uint8_t escape [] = {0x5c, 0x00};

int verbose_flag = 0;

uint8_t *add_nl_at_end(uint8_t *in, int len)
{
	uint8_t *c, *end = in + len;
	assert(*end == 0);
	c = krealloc(in, len + 1, GFP_KERNEL);
	assert(c);
	*(c + len) = 0x0a;
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
			count = (c - in) + 1;
			out = krealloc(in, count, GFP_KERNEL);
			return out;
		}
	}
	return in;
}


uint8_t *unescape_newlines(uint8_t *in, int len)
{
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
				}
			}
			in++;
		}
	} while (in);
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
	while (c) {
		count++;
		c = strpbrk(++c, nl);
	}
	if (count) {
		c = krealloc (in,  len + count, GFP_KERNEL);
        if (c) {
			*(c + (len + count)) = 0x00;
			p = c;
			while (*p && count) {
				if (*p != 0x0a && *p != 0x0d) {
					*c++ = *p++;
				} else {
					*c++ = 0x5c;
					*c++ = *p++;
					count--;
				}
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
	exit(0);
}


enum type { VERBOSE, ADD_NL, TRIM_TO_NL, UXP_NL, XP_NL, USAGE };
uint8_t *input_string = NULL, *output_string = NULL;
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
