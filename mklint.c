/* See LICENSE file for copyright and license details. */
#include "common.h"

NUSAGE(EXIT_ERROR, "[-f makefile]");


int exit_status = 0;

struct style style = {
	.max_line_length = 120,
	.only_empty_blank_lines = 1
};


static void
set_line_continuation_joiner(struct line *line)
{
	if (line->len && line->data[line->len - 1] == '\\') {
		line->data[--line->len] = '\0';
		/* Doesn't matter here if the first non-white space is # */
		line->continuation_joiner = line->data[0] == '\t' ? '\t' : ' ';
	} else {
		line->continuation_joiner = '\0';
	}
}


static void
check_line_continuations(struct line *lines, size_t nlines)
{
	size_t i, cont_from = 0;

	for (i = 0; i < nlines; i++) {
		set_line_continuation_joiner(&lines[i]);

		if (lines[i].continuation_joiner &&
		    (!i || !lines[i - 1].continuation_joiner) &&
		    is_line_blank(&lines[i])) {
			warnf_confusing(WC_CONTINUATION_OF_BLANK,
			                "%s:%zu: initial line continuation on otherwise blank line, can cause confusion",
			                lines[i].path, lines[i].lineno);
		}

		if (!lines[i].continuation_joiner &&
		    i && lines[i - 1].continuation_joiner) {
			warnf_confusing(WC_CONTINUATION_TO_BLANK,
			                "%s:%zu: terminal line continuation to blank line, can cause confusion",
			                lines[i].path, lines[i].lineno);
		}

		if (!lines[i].continuation_joiner && lines[i].eof) {
			warnf_unspecified(WC_EOF_LINE_CONTINUATION,
			                  "%s:%zu: line continuation at end of file, causes unspecified behaviour%s",
			                  lines[i].path, lines[i].lineno,
			                  (!lines[i].nest_level ? "" :
			                   ", it is especially problematic in an included line"));
			printinfof(WC_EOF_LINE_CONTINUATION, "this implementation will remove the line continuation");
			lines[i].continuation_joiner = 0;
		}

		if (i && lines[i - 1].continuation_joiner && lines[i].len) {
			if (!isspace(lines[i].data[0])) {
				if (lines[cont_from].len && !isspace(lines[cont_from].data[lines[cont_from].len - 1])) {
					warnf_confusing(WC_SPACELESS_CONTINUATION,
					                "%s:%zu,%zu: <backslash> is proceeded by a non-white space "
					                "character at the same time as the next line%s begins with "
					                "a non-white space character, this can cause confusion as "
					                "the make utility will add a whitespace",
					                lines[cont_from].path, lines[cont_from].lineno,
					                lines[i].lineno, i == cont_from + 1 ? "" :
					                ", that consist of not only a <backslash>,");
				}
				warnf_confusing(WC_UNINDENTED_CONTINUATION,
				                "%s:%zu: continuation of line is not indented, can cause confusion",
				                lines[i].path, lines[i].lineno);
			}
			cont_from = i;
		} else if (lines[i].continuation_joiner) {
			cont_from = i;
		}
	}
}



static enum line_class
classify_line(struct line *line)
{
	int warned_bad_space = 0;
	char *s;

	if (!line->len)
		return EMPTY;

	s = line->data;

	while (isspace(*s)) {
		if (!warned_bad_space && !isblank(*s)) {
			warned_bad_space = 1;
			warnf_undefined(WC_LEADING_BAD_SPACE,
			                "%s:%zu: line contains leading white space other than"
			                "<space> and <tab>, which causes undefined behaviour",
			                line->path, line->lineno);
			/* TODO what do we do here? */
		}
		s++;
	}

	if (*s == '#') {
		if (line->data[0] != '#') {
			/* TODO should not apply if command line */
			warnf_undefined(WC_ILLEGAL_INDENT,
			                "%s:%zu: comment has leading white space, which is not legal",
			                line->path, line->lineno);
			printinfof(WC_ILLEGAL_INDENT, "this implementation will recognise it as a comment line");
		}
		return COMMENT;

	} else if (!*s) {
		return BLANK;

	} else if (line->data[0] == '\t') {
		return COMMAND_LINE;

	} else {
		return OTHER;
	}
}


int
main(int argc, char *argv[])
{
	const char *path = NULL;
	struct line *lines;
	size_t nlines;
	size_t i;

	libsimple_default_failure_exit = EXIT_ERROR;

	/* make(1) shall support mixing of options and operands (up to --) */
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

	check_line_continuations(lines, nlines);

	for (i = 0; i < nlines; i++) {
		switch (classify_line(&lines[i])) {
		case EMPTY:
			break;

		case BLANK:
			if (style.only_empty_blank_lines) {
				warnf_style(WC_NONEMPTY_BLANK, "%s:%zu: line is blank but not empty",
				            lines[i].path, lines[i].lineno);
			}
			break;

		case COMMENT:
			break;

		case COMMAND_LINE:
		case OTHER:
			break;

		default:
			abort();
		}
	}

	free(lines);
	return exit_status;
}
