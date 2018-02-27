#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <getopt.h>

#define GFP_KERNEL 1
#define krealloc(p, t, f)					\
				 realloc((p), (t))
#define DMSG printf

char *test1 = "this is terminated by a new line\n trailing";
typedef char uint8_t;
const uint8_t escape [] = {0x5c, 0x00};

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

void usage(void)
{
	DMSG("help\n");
}


enum type { ADD_NL, TRIM_TO_NL, UXP_NL, XP_NL };
int add_flag, trim_flag, uxp_flag, xp_flag;


static inline void
get_options (int argc, char **argv)
{
	while (1)
    {
		if (argc < 2)
			usage ();

		int c;
		static struct option long_options[] = {
			{"dummy-for-short-option", no_argument, NULL, 0},
			{"add-nl", required_argument, &add_flag, 1},
			{"trim-to-nl", no_argument, &trim_flag, 1},
			{"unescape-nl", required_argument, &uxp_flag, 1},
			{"escape-nl", required_argument, &xp_flag, 1},
			{"help", no_argument, NULL, 0},
			{0, 0, 0, 0}
		};
		int option_index = 0;
		c = getopt_long_only (argc, argv, "ila:f:u:s:dh",
							  long_options, &option_index);
		if (c == -1)
		{
			break;
		}

    restart_long:
		switch (option_index)
		{
			DMSG("selected option %s\n", long_options[option_index].name);

		case 0:
			switch (c)
			{
			case 'a':
				option_index = 1;
				add_flag = 1;
				goto restart_long;
			case 't':
				option_index = 2;
				trim_flag = 1;
				goto restart_long;
			case 'u':
				option_index = 3;
				uxp_flag = 1;
				goto restart_long;
			case 'e':
				option_index = 4;
				xp_flag = 1;
				goto restart_long;
			default:
				option_index = 5;
				usage ();
			}
		case 1:		/* info */
		{

		}
		break;

		case 2:		/* list */

			break;
		case 3:		/* find */
		{
			break;
		}
		case 4:		/* apply */
		{
			break;
		}
		case 5:		/* remove */
		default:
			break;
		}
    }
}

int main(int argc, uint8_t **argv)
{
	enum type test;
}
