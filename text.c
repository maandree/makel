/* See LICENSE file for copyright and license details. */
#include "common.h"

#ifdef HAVE_GRAPHEME
#include <grapheme.h>
#endif

static void
print_long_line_tip(enum warning_class class)
{
	printtipf(class, "you can put a <backslash> at the end of the line to continue "
	                 "it on the next line, except in or immediately proceeding an "
	                 "include line");
}


struct line *
load_text_file(int fd, const char *fname, int nest_level, size_t *nlinesp)
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
		lines[i].eof = i + 1 == *nlinesp;
		lines[i].nest_level = nest_level;

		if (lines[i].len + 1 > 2048) {
			/* https://pubs.opengroup.org/onlinepubs/9699919799/basedefs/V1_chap03.html#tag_03_403 */
			warnf_undefined(WC_TEXT, "%s:%zu: line is, including the <newline> character, longer than "
			                         "2048 bytes which causes undefined behaviour as input files are "
			                         "text files and POSIX only guarantees support for lines up to 2048 "
			                         "bytes long including the <newline> character in text files",
			                fname, *nlinesp + 1);
			printinfof(WC_TEXT, "this implementation supports arbitrarily long lines");
			print_long_line_tip(WC_TEXT);
		}
		p += lines[i].len + 1;
	}

	free(buf);
	return lines;
}


void
check_utf8_encoding(struct line *line)
{
#ifdef HAVE_GRAPHEME
	size_t off, r;
	uint_least32_t codepoint;
#if GRAPHEME_INVALID_CODEPOINT == 0xFFFD
	unsigned char invalid_codepoint_encoding[] = {0xEF, 0xBF, 0xBD};
#endif

	for (off = 0; off < line->len; off += r) {
		r = grapheme_decode_utf8(&line->data[off], line->len - off, &codepoint);

		if (codepoint == GRAPHEME_INVALID_CODEPOINT &&
		    (r != ELEMSOF(invalid_codepoint_encoding) ||
		     memcmp(&line->data[off], invalid_codepoint_encoding, r))) {

			warnf_unspecified(WC_ENCODING, "%s:%zu: line contains invalid UTF-8", line->path, line->lineno);
			printinfof(WC_ENCODING, "this implementation will replace it the "
			                        "Unicode replacement character (U+FFFD)");

			line->data = erealloc(line->data, line->len - r + ELEMSOF(invalid_codepoint_encoding));
			memmove(&line->data[off + ELEMSOF(invalid_codepoint_encoding)],
			        &line->data[off + r],
			        line->len - off - r);
			memcpy(&line->data[off], invalid_codepoint_encoding, ELEMSOF(invalid_codepoint_encoding));
			line->len -= r;
			line->len += r = ELEMSOF(invalid_codepoint_encoding);
		}
	}
#endif
}


void
check_column_count(struct line *line)
{
#ifdef HAVE_GRAPHEME
	size_t columns = 0;
	size_t off, r;
	uint_least32_t codepoint;

	if (line->len <= style.max_line_length) /* Column count cannot be more than byte count */
		return;

	for (off = 0; off < line->len; off += r) {
		r = grapheme_decode_utf8(&line->data[off], line->len - off, &codepoint);
		columns += (size_t)abs(wcwidth((wchar_t)codepoint));
	}

	if (columns > style.max_line_length) {
		warnf_style(WC_LONG_LINE, "%s:%zu: line is longer than %zu columns",
		            line->path, line->lineno, columns);
		if (line->len + 1 <= 2048)
			print_long_line_tip(WC_LONG_LINE);
	}
#endif
}


int
is_line_blank(struct line *line)
{
	char *s = line->data;
	while (isspace(*s))
		s++;
	return !*s;
}
