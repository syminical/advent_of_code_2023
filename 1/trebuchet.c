#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <ctype.h>
#include <regex.h>
#include <string.h>



#define IN_FILE_NAME		"example_input.txt"
#define MAX_LINE_LEN		77
#define WTOD_MAP_N			9
#define WTOD_MAP_MAX_LEN	5



const char * const WTOD_MAP[WTOD_MAP_N][WTOD_MAP_MAX_LEN+1] = {
	{"one", "o1e"},
	{"two", "t2o"},
	{"three", "t3e"},
	{"four", "f4r"},
	{"five", "f5e"},
	{"six", "s6x"},
	{"seven", "s7n"},
	{"eight", "e8t"},
	{"nine", "n9e"},
};



/* file io */
void 			open_files (FILE **in_file);
void 			close_files (FILE **in_file);
size_t			f_get_line	(FILE **in_file, char *line_buffer, size_t max_len);
/* input processing */
void			str_convert_words_to_digits (regex_t *regex, char *line);
bool 			str_extract_number (char line[], unsigned long *number);
unsigned		str_replace_word_digit (char line[], size_t start, size_t len);



int main (size_t arg_count, char *args[arg_count])
{
	unsigned long sum = 0;

	FILE *in_file;
	open_files(&in_file);

	regex_t regex;
	int re_compile_failed = regcomp(
		&regex,
		"(one|two|three|four|five|six|seven|eight|nine)",
		REG_EXTENDED
	);
	if (re_compile_failed) {
		printf("Could not compile regex!");
		exit(1);
	}

	size_t line_len;
	unsigned long number;
	char line[MAX_LINE_LEN+1];
	for (
		size_t line_n = 1;
			(line_len=f_get_line(&in_file, line, MAX_LINE_LEN)) != 0
		;++line_n
	) {
		printf("%zu: %s", line_n, line);
		str_convert_words_to_digits(&regex, line);
		printf("   - %s", line);
		if (str_extract_number(line, &number)) {
			sum += number;
		} else {
			printf("   - <ERROR! | Number Not Found>");
		}
	}

	printf("\nThe sum is: %zu\n", sum);

	close_files(&in_file);

	exit(0);
}


/* potentially unsafe, may not be completely correct */
void str_convert_words_to_digits (regex_t *regex, char line[]) {
	int re_error;
	size_t matching_start_offset = 0;
	const size_t max_matches = 2;
	regmatch_t matches[max_matches];
	while (true) {
		/* success is 0, fail is > 0 */
		re_error = regexec(
			regex,
			line+matching_start_offset,
			max_matches,
			matches,
			0
		);
		if (re_error == REG_NOMATCH)
			break;
		else if (re_error) {
			char msg_buf[100];
			regerror(re_error, regex, msg_buf, sizeof(msg_buf));
			fprintf(stderr, "Regex match failed: %s | \"%s\"\n", msg_buf, line+matching_start_offset);
			break;
		} else {
			/*
			   - choosing matches[1] over [0] is a bit mystical, but I think it's correct
			   		- limiting matches to [0] and [1], instead of also [2] for example, is also mysterious
						- perhaps this is useful if there are nested subgroups in the regex
						- I think <regex.h> is bugged if you allow matches to have extra slots.
							- it should be rm_so = rm_eo = -1 and obviously unused, but it's inconsistent
								- you randomly get garbage like rm_so = rm_eo = matches[0].rm_eo-1 (USELESS AND BAD)
			   - [0] has the nasty habit of being more than desired, and [1] seems to clamp down on the subgroups
					- e.g. "one" instead of "mone" from "mone231"
			*/
			unsigned adjusted_match_end = str_replace_word_digit(
				line,
				matching_start_offset+matches[1].rm_so,
				matching_start_offset+matches[1].rm_eo
			);
			matching_start_offset = adjusted_match_end-1;
		}
	}
}


/* potentially unsafe, may not be completely correct */
unsigned str_replace_word_digit(char line[], size_t start, size_t end)
{
	size_t line_len = strlen(line)+1;
	size_t match_len = end - start;
	char match_buffer[WTOD_MAP_MAX_LEN];
	/* isolate the matched substring from the line */
	strncpy(match_buffer, line+start, match_len);
	/* find a word we can decode then replace it */
	for (size_t i=0; i < WTOD_MAP_N; ++i) {
		/* if the strings match, attempt the replace */
		if (!strcmp(WTOD_MAP[i][0], match_buffer)) {
			size_t j;
			/* copy match_buffer into line over the digit word */
			for (j=0; WTOD_MAP[i][1][j] != '\0' && j < match_len; ++j)
				line[start+j] = WTOD_MAP[i][1][j];
			/* move the rest of line up to close the gap */
			if (j != end) {
				//printf("%s, %s, %d, %d\n", line+start+j, line+end, end, line_len-end);
				memcpy(line+start+j, line+end, line_len-end);
				/* return the correct offset for the new matching window start */
				return start+j;
			}
		}
	}
	return end;
}


/*
	= args:
		- line, the reference string
		- number, the value gets replaced
	= return: true if successful false if failed
		- disregard the value number if false returned
*/
bool str_extract_number (char line[], unsigned long *number)
{
	char c;
	unsigned first_digit;
	unsigned last_digit;
	bool has_first_digit = false;
	for (size_t i=0; (c=line[i]) != '\0'; ++i) {
		/* filter out noise */
		if (!isdigit(c))
			continue;

		if (!has_first_digit) {
			first_digit = (unsigned)c;
			last_digit = (unsigned)c;
			has_first_digit = true;
		} else
			last_digit = (unsigned)c;
	}

	if (has_first_digit) {
		char num_str[] = {first_digit, last_digit};
		*number = strtoul(num_str, NULL, 0);
		printf("   = %zu\n\n", *number);
		return true;
	}
	*number = 0;
	return false;
}


/*
   - read a line from in_file into s limited by max_len
   - includes trailing '\n'
   = return functional length of s (not including the terminating '\0')
*/
size_t f_get_line (FILE **in_file, char *line_buffer, size_t max_len)
{
	int c;
	size_t i;
	for (
		i=0;
			i < max_len
			&& (c=fgetc(*in_file)) != EOF
			&& c != '\n'
		;++i
	) {
		line_buffer[i] = c;
	}
	if (c == '\n') {
		line_buffer[i] = c;
		++i;
	}
	line_buffer[i] = '\0';
	return i;
}


/*
	- check if the file exists
	- open the file stream for in_file
*/
void open_files (FILE **in_file)
{
	/* open in_file as "r" and make sure it exists */
	*in_file = fopen(IN_FILE_NAME, "r");
	if (*in_file == NULL) {
		printf("%s does not exist!\n", IN_FILE_NAME);
		exit(1);
	}
}


/* realistically this is not needed */
void close_files (FILE **in_file)
{
	// good-boy bloat :)
	fclose(*in_file);
}
