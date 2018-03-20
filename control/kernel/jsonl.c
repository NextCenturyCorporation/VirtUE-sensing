#include "jsonl-parse.include.c"

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

			FILE *in_file;
			ssize_t nread, len = 0;
			uint8_t *line = NULL;
			SLIST_INIT(&h_sessions);
			in_file_name = strdup(optarg);
			in_file = fopen(in_file_name, "r");
			if (in_file == NULL) {
				perror("fopen");
				exit(EXIT_FAILURE);
			}


			while((nread = getline(&line, &len, in_file)) != -1) {
				struct jsmn_message *this_msg = new_message(line, strlen(line));
				if (!this_msg) {
					exit(EXIT_FAILURE);
				}
				/* the following call will free a request/response pair and its
				 * session structure.
				 */
				this_msg->file = in_file;
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
