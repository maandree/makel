/* See LICENSE file for copyright and license details. */
#include "common.h"

NUSAGE(EXIT_ERROR, "[-f makefile]");


int exit_status = 0;

struct style style = {
	.max_line_length = 120
};


int
main(int argc, char *argv[])
{
	const char *path = NULL;
	struct line *lines;
	size_t nlines;
	size_t i;

	libsimple_default_failure_exit = EXIT_ERROR;

	/* make(1) shall support mixing of options and operands (uptil --) */
	ARGBEGIN {
	case 'f':
		cmdline_opt_f(ARG(), &path);
		break;

	default:
		usage();
	} ARGEND;

	if (argc)
		usage();

	setlocale(LC_ALL, ""); /* Required by wcwidth(3) */

	lines = load_makefile(path, &nlines);

	for (i = 0; i < nlines; i++) {
		check_utf8_encoding(&lines[i]);
		check_column_count(&lines[i]);
	}

	free(lines);
	return exit_status;
}
