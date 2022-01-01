/* See LICENSE file for copyright and license details. */
#include "common.h"

NUSAGE(EXIT_ERROR, "[-f makefile]");


int exit_status = 0;

struct style style = {
	.max_line_length = 120
};


static const char *const default_makefiles[] = {
	"makefile",
	"Makefile"
};


static int
open_default_makefile(const char **pathp)
{
	int fd;
	size_t i;

	/* The specification says that the alternatives “shall be
	 * tried”, but it uses the phrase “if neither … are found”,
	 * implying that make(1) shall fail if a file cannot be
	 * opened and only try the next alternative if it failed
	 * becomes the file does not exist.
	 */

	for (i = 0; i < ELEMSOF(default_makefiles); i++) {
		*pathp = default_makefiles[i];
		fd = open(*pathp, O_RDONLY);
		if (fd >= 0) {
			printinfof(WC_MAKEFILE, "found standard makefile to use: %s", *pathp);
			goto find_existing_fallbacks;
		} else if (errno != ENOENT) {
			eprintf("found standard makefile to use, but failed to open: %s:", *pathp);
		}
	}

	printerrorf("couldn't find any makefile to use, portable "
	            "alternatives are ./makefile and ./Makefile");

find_existing_fallbacks:
	for (i++; i < ELEMSOF(default_makefiles); i++)
		if (!access(default_makefiles[i], F_OK))
			warnf_warning(WC_EXTRA_MAKEFILE, "found additional standard makefile: %s",
			              default_makefiles[i]);

	return fd;
}


static void
cmdline_opt_f(const char *arg, const char **makefile_pathp)
{
	static int warning_emitted = 0;

	if (*makefile_pathp && !warning_emitted) {
		warning_emitted = 1;
		warnf_unspecified(WC_CMDLINE, "the -f option has been specified multiple times, "
		                              "they are processed in order, but the behaviour is "
	                                      "otherwise unspecified");
		printinfof(WC_CMDLINE, "this implementation will use the last "
		                       "option and discard earlier options");
	}

	*makefile_pathp = arg;
}


static struct line *
load_makefile(const char *path, size_t *nlinesp)
{
	struct line *lines;
	int fd;

	if (!path) {
		fd = open_default_makefile(&path);
	} else if (!strcmp(path, "-")) {
		/* “A pathname of '-' shall denote the standard input” */
		fd = dup(STDIN_FILENO);
		if (fd < 0)
			eprintf("dup <stdin>:");
		path = "<stdin>";
	} else {
		fd = open(path, O_RDONLY);
		if (fd < 0)
			eprintf("open %s O_RDONLY:", path);
	}

	lines = load_text_file(fd, path, 0, nlinesp);
	close(fd);
	return lines;
}


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
