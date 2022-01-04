/* See LICENSE file for copyright and license details. */
#include "common.h"


static const char *const default_makefiles[] = {
	"makefile",
	"Makefile"
};


int
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
	/* This serves two purposes: to inform the user that
	 * we are only checking one of the files, and which
	 * would (which is printed earlier), and to information
	 * the user that it can be confusing. It is not common
	 * practice run make(1) to generate ./makefile from
	 * ./Makefile and (either immediately or by running
	 * make(1) again) built the project from ./Makefile,
	 * although that certainly can be useful if there are
	 * parts of the makefile you want to generate, such
	 * as .h file dependencies for .c files in very large
	 * projects that have many .h files and many .c files
	 * that each only depend on a few .h files.
	 */
	for (i++; i < ELEMSOF(default_makefiles); i++)
		if (!access(default_makefiles[i], F_OK))
			warnf_confusing(WC_EXTRA_MAKEFILE,
			                "found additional standard makefile, this be confusing: %s",
			                default_makefiles[i]);

	return fd;
}


void
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


struct line *
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
