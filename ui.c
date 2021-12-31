/* See LICENSE file for copyright and license details. */
#include "common.h"


struct warning_class_data warning_classes[] = {
#define X(ENUM, NAME, ACTION) {NAME, ACTION},
	LIST_WARNING_CLASSES(X)
#undef X
	{NULL, 0}
};


static void
vxprintwarningf(enum warning_class class, int severity, const char *fmt, va_list ap)
{
	if (warning_classes[class].action != IGNORE) {
		fprintf(stderr, "%s: [%s] ", argv0,
		        warning_classes[class].action == INFORM ? "info" : "warn");
		vfprintf(stderr, fmt, ap);
		fprintf(stderr, " (-w%s)\n", warning_classes[class].name);
		if (warning_classes[class].action != INFORM)
			exit_status = MAX(exit_status, severity);
	}
}


void
xprintwarningf(enum warning_class class, int severity, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vxprintwarningf(class, severity, fmt, ap);
	va_end(ap);
}


void
printerrorf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "%s: [error] ", argv0);
	vfprintf(stderr, fmt, ap);
	fprintf(stderr, "\n");
	va_end(ap);
	exit(EXIT_CRITICAL);
}
