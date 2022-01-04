/* Trivial code, not subject to copyright, use as you see fit.
 * Reimplementation of 20h's arg.h */

#ifndef ARG_H
#define ARG_H

#include <stddef.h>


extern const char *argv0;


#define ARGBEGIN        do {\
                                char arg_h_flag_, arg_h_break_;\
				if (!argc)\
					break;\
                                argv0 = argv[0];\
                                while (--argc, *++argv && argv[0][0] == '-' && argv[0][1]) {\
					if (argv[0][1] == '-' && !argv[0][2]) {\
                                                argv++;\
                                                argc--;\
                                                break;\
                                        }\
                                        for (arg_h_break_ = 0; !arg_h_break_ && *++*argv;) {\
                                                switch ((arg_h_flag_ = **argv))

#define ARGEND				}\
				}\
			} while (0)


#define FLAG()		(arg_h_flag_)

#define ARG()		(arg_h_break_ = 1, argv[0][1] ? &argv[0][1] :\
			                   argv[1]    ? (argc--, *++argv) :\
			                                (usage(), NULL))

#endif
