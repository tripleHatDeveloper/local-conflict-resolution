#ifndef _OPTS_H
#define _OPTS_H

#ifdef MAIN
#define DECL
#else
#define DECL extern
#endif

DECL int Opt_Min;
DECL int Opt_All;
DECL int Opt_Verb;
DECL int Opt_Time;
DECL int Opt_FullRem;

#endif
