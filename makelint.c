/* See LICENSE file for copyright and license details. */
#include "common.h"

NUSAGE(EXIT_ERROR, "[-f makefile]");



int exit_status = 0;

static const char *default_makefiles[] = {
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
	for (; i < ELEMSOF(default_makefiles); i++)
		if (!access(default_makefiles[i], F_OK))
			warnf_warning(WC_EXTRA_MAKEFILE, "found additional standard makefile: %s", *pathp);

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
load_file(int fd, const char *fname, size_t *nlinesp)
{
	struct line *lines;
	char *buf = NULL, *p;
	size_t size = 0;
	size_t len = 0;
	size_t i;
	ssize_t r;

	/* getline(3) may seem like the best way to read line by line,
	 * however, it may terminate before the end of the line is
	 * reached, which we would have to deal with, additionally,
	 * we want to check for null bytes. Therefore we will keep
	 * this simple and use read(3) and scan manually; and as a
	 * bonus we can leave the file descriptor open, and let the
	 * caller than created it close it.
	 */

	i = 0;
	*nlinesp = 0;
	for (;;) {
		if (len == size)
			buf = erealloc(buf, size += 2048);
		r = read(fd, &buf[len], size - len);
		if (r > 0)
			len += (size_t)r;
		else if (!r)
			break;
		else if (errno == EINTR)
			continue;
		else
			eprintf("read %s:", fname);

		for (; i < len; i++) {
			if (buf[i] == '\n') {
				*nlinesp += 1;
				buf[i] = '\0';
			} else if (buf[i] == '\0') {
				/* https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_403 */
				warnf_undefined(WC_TEXT, "%s:%zu: file contains a NUL byte, this is disallowed, because "
				                         "input files are text files, and causes undefined behaviour",
				                fname, *nlinesp + 1);
				/* make(1) should probably just abort */
				printinfof(WC_TEXT, "this implementation will replace it with a <space>");
				buf[i] = ' ';
			}
		}
	}

	if (len && buf[len - 1] != '\0') { /* LF has been converted to NUL above */
		/* https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_403 */
		warnf_undefined(WC_TEXT, "%s:%zu: is non-empty but does not end with a <newline>, which is "
		                         "required because input files are text files, and omission of it "
		                         "causes undefined behaviour",
		                fname, *nlinesp + 1);
		/* make(1) should probably just abort */
		printinfof(WC_TEXT, "this implementation will add the missing <newline>");
		buf = erealloc(buf, len + 1);
		buf[len++] = '\0';
		*nlinesp += 1;
	}

	lines = *nlinesp ? ecalloc(*nlinesp, sizeof(*lines)) : NULL;
	for (p = buf, i = 0; i < *nlinesp; i++) {
		lines[i].lineno = i + 1;
		lines[i].path = fname;
		lines[i].len = strlen(p);
		lines[i].data = ememdup(p, lines[i].len + 1);
		if (lines[i].len + 1 > 2048) {
			/* https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_403 */
			warnf_undefined(WC_TEXT, "%s:%zu: line is, including the <newline> character, longer than "
			                         "2048 bytes which causes undefined behaviour as input files are "
			                         "text files and POSIX only guarantees support for lines up to 2048 "
			                         "bytes long including the <newline> character in text files",
			                fname, *nlinesp + 1);
		}
		p += lines[i].len + 1;
	}

	free(buf);
	return lines;
}


int
main(int argc, char *argv[])
{
	const char *path = NULL;
	int fd;
	struct line *lines;
	size_t nlines;

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

	lines = load_file(fd, path, &nlines);
	close(fd);

	free(lines);
	return exit_status;
}
