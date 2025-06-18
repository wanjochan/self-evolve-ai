/*
 *  ARM64 register definitions for TinyCC
 *
 *  This file contains the register definitions needed for ARM64 code generation
 *  These definitions are extracted from arm64-gen.c TARGET_DEFS_ONLY section
 *  to make them available during regular compilation.
 */

#ifndef _ARM64_DEFS_H
#define _ARM64_DEFS_H

/* Only define these macros if they're not already defined elsewhere */
#ifndef TREG_R
#define TREG_R(x) (x) // x = 0..18
#endif

#ifndef TREG_R30
#define TREG_R30  19
#endif

#ifndef TREG_F
#define TREG_F(x) (x + 20) // x = 0..7
#endif

/* ARM64-specific register class macros that won't conflict with x86_64 */
#ifndef RC_R
#define RC_R(x) (1 << (2 + (x))) // x = 0..18
#endif

#ifndef RC_R30
#define RC_R30  (1 << 21)
#endif

#ifndef RC_F
#define RC_F(x) (1 << (22 + (x))) // x = 0..7
#endif

/* Define long double helper function tokens if not already defined */
#ifndef TOK___multf3
#define TOK___multf3 TOK___DUMMY("__multf3")
#define TOK___addtf3 TOK___DUMMY("__addtf3")
#define TOK___subtf3 TOK___DUMMY("__subtf3")
#define TOK___divtf3 TOK___DUMMY("__divtf3")
#define TOK___eqtf2 TOK___DUMMY("__eqtf2")
#define TOK___netf2 TOK___DUMMY("__netf2")
#define TOK___lttf2 TOK___DUMMY("__lttf2")
#define TOK___getf2 TOK___DUMMY("__getf2")
#define TOK___letf2 TOK___DUMMY("__letf2")
#define TOK___gttf2 TOK___DUMMY("__gttf2")
#define TOK___floatunditf TOK___DUMMY("__floatunditf")
#define TOK___floatditf TOK___DUMMY("__floatditf")
#define TOK___floatunsitf TOK___DUMMY("__floatunsitf")
#define TOK___floatsitf TOK___DUMMY("__floatsitf")
#define TOK___fixunstfdi TOK___DUMMY("__fixunstfdi")
#define TOK___fixtfdi TOK___DUMMY("__fixtfdi")
#define TOK___fixunstfsi TOK___DUMMY("__fixunstfsi")
#define TOK___fixtfsi TOK___DUMMY("__fixtfsi")
#endif

#endif /* _ARM64_DEFS_H */
