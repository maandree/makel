/* See LICENSE file for copyright and license details. */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <locale.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <wchar.h>

#include "arg.h"


#if defined(__GNUC__)
# pragma GCC diagnostic ignored "-Wsuggest-attribute=format"
# pragma GCC diagnostic ignored "-Wsuggest-attribute=noreturn"
# pragma GCC diagnostic ignored "-Wsuggest-attribute=pure"
# pragma GCC diagnostic ignored "-Wsuggest-attribute=const"
#endif


#define EXIT_STYLE         1
#define EXIT_CONFUSING     2
#define EXIT_WARNING       3
#define EXIT_UNSPECIFIED   4
#define EXIT_NONCONFORMING 5
#define EXIT_UNDEFINED     6
#define EXIT_CRITICAL      7
#define EXIT_ERROR         8


#define ELEMSOF(ARRAY) (sizeof(ARRAY) / sizeof(*(ARRAY)))
#define MAX(A, B) ((A) > (B) ? (A) : (B))


#define LIST_WARNING_CLASSES(X)\
	X(WC_MAKEFILE, "makefile", INFORM)\
	X(WC_EXTRA_MAKEFILE, "extra-makefile", WARN)\
	X(WC_CMDLINE, "cmdline", WARN)\
	X(WC_TEXT, "text", WARN)\
	X(WC_ENCODING, "encoding", WARN)\
	X(WC_LONG_LINE, "long-line", WARN_STYLE)\
	X(WC_NONEMPTY_BLANK, "nonempty-blank", WARN_STYLE)\
	X(WC_LEADING_BAD_SPACE, "leading-bad-space", WARN)\
	X(WC_ILLEGAL_INDENT, "illegal-indent", WARN)\
	X(WC_CONTINUATION_OF_BLANK, "continuation-of-blank", WARN)\
	X(WC_CONTINUATION_TO_BLANK, "continuation-to-blank", WARN)\
	X(WC_EOF_LINE_CONTINUATION, "eof-line-continuation", WARN)\
	X(WC_UNINDENTED_CONTINUATION, "unindented-continuation", WARN)\
	X(WC_SPACELESS_CONTINUATION, "spaceless-continuation", WARN)\
	X(WC_COMMENT_CONTINUATION, "comment-continuation", WARN)


enum action {
	IGNORE,
	INFORM,
	WARN_STYLE,
	WARN
};

enum warning_class {
#define X(ENUM, NAME, ACTION) ENUM,
	LIST_WARNING_CLASSES(X)
#undef X
	NUM_WARNING_CLASS
};

struct warning_class_data {
	const char *name;
	enum action action;
};

enum line_class {
	EMPTY, /* Classified as comment lines in the specification */
	BLANK, /* Classified as comment lines in the specification */
	COMMENT,
	COMMAND_LINE,
	OTHER
};

struct line {
	char *data;
	size_t len;
	const char *path;
	size_t lineno;
	int eof;
	int nest_level;
	char continuation_joiner; /* If '\\', it shall be '\\\n' */
};

enum macro_bracket_style {
	INCONSISTENT,
	ROUND,
	CURLY
};

struct style {
	size_t max_line_length;
	int only_empty_blank_lines;
	enum macro_bracket_style macro_bracket_style;
};


extern int exit_status;
extern struct style style;


/* makefile.c */
int open_default_makefile(const char **pathp);
void cmdline_opt_f(const char *arg, const char **makefile_pathp);
struct line *load_makefile(const char *path, size_t *nlinesp);


/* text.c */
struct line *load_text_file(int fd, const char *fname, int nest_level, size_t *nlinesp);
void check_utf8_encoding(struct line *line);
void check_column_count(struct line *line);
int is_line_blank(struct line *line);


/* ui.c */
extern struct warning_class_data warning_classes[];
void xprintwarningf(enum warning_class class, int severity, const char *fmt, ...);
#define warnf_style(CLASS, ...) xprintwarningf(CLASS, EXIT_STYLE, __VA_ARGS__)
#define warnf_confusing(CLASS, ...) xprintwarningf(CLASS, EXIT_CONFUSING, __VA_ARGS__)
#define warnf_warning(CLASS, ...) xprintwarningf(CLASS, EXIT_WARNING, __VA_ARGS__)
#define warnf_unspecified(CLASS, ...) xprintwarningf(CLASS, EXIT_UNSPECIFIED, __VA_ARGS__)
#define warnf_nonconforming(CLASS, ...) xprintwarningf(CLASS, EXIT_NONCONFORMING, __VA_ARGS__)
#define warnf_undefined(CLASS, ...) xprintwarningf(CLASS, EXIT_UNDEFINED, __VA_ARGS__)
void printinfof(enum warning_class class, const char *fmt, ...);
void printerrorf(const char *fmt, ...);
void printtipf(enum warning_class class, const char *fmt, ...);


/* util.c */
void *erealloc(void *, size_t);
void *ecalloc(size_t, size_t);
void *emalloc(size_t);
void *ememdup(const void *, size_t);
void eprintf(const char *, ...);
