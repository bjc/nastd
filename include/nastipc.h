/* $Id: nastipc.h,v 1.3 2001/01/19 00:29:26 shmit Exp $ */

#ifndef NASTIPC_H
#define NASTIPC_H

#define NASTHOLE "/tmp/nastd.sock"

/* Protocol section */

/*
 * Send NASTESC to start a new command. The next byte specifies the
 * type of command.
 */
#define NASTCMD		'\xff'

/*
 * Commands, prefixed by NASTCMD.
 */
#define NASTADD		'\x01'
#define NASTDEL		'\x02'
#define NASTUPD		'\x03'
#define NASTGET		'\x04'
#define NASTDIE		'\x0f'

/* Commands for server options. */
#define NASTOPTGET	'\x10'
#define NASTOPTSET	'\x11'

/* Command to get stats. */
#define NASTSTATS	'\x20'

/* Server -> client responses. These can be postfixed with a string. */
#define NASTOK		'\xf1'
#define NASTERR		'\xf2'
#define NASTARG		'\xe0'

/* The quote character, for globbing multiple strings together. */
#define NASTQUOTE	'\xfe'

/*
 * The escape character, to send binary that may be interpereted
 * incorrectly.
 */
#define NASTESC		'\xfd'

/* Item seperator, for returning multiple items in one response. */
#define NASTSEP		'\xfc'

/*
 * The options. Ass more get added to the protocol, just add them here.
 * Do not dupe option numbers. That'd be bad.
 * Use these after the NASTCMD NASTOPTGET sequence. All options require
 * an argument. Most are true/false, but some will require other types.
 * Check nastd.h for option types.
 * (e.g.: NASTCMD NASTOPTGET OPTQCACHE OPTFALSE - don't use the query cache)
 * (e.g.: NASTCMD NASTOPTGET OPTNTHREADS 0x10 - allocate 16 threads)
 */
#define OPTFALSE	'\x00'
#define OPTTRUE		'\x01'

#define OPTQCACHE	'\x01'
#define OPTLOCALDB	'\x02'
#define OPTFALLASYNC	'\x03'
#define OPTALWAYSFALL	'\x04'
#define OPTFAILONCE	'\x05'
#define OPTNTHREADS	'\x06'
#define OPTNOFALLTHROUGH '\x07'

#endif
