/*
 * dump.h
 */
/**
\ingroup lub
\defgroup lub_dump dump
@{

\brief This utility provides a simple hierachical debugging mechanism.

By indenting and undenting the output, printing nested debug messages is made
easy.
*/
#ifndef _lub_dump_h
#define _lub_dump_h

#define LUB_DUMP_NULL "(null)"
#define LUB_DUMP_STR(str) ( str ? str : LUB_DUMP_NULL )
#define LUB_DUMP_BOOL(val) ( val ? "true" : "false" )

#include <stdarg.h>
/*=====================================
 * DUMP INTERFACE
 *===================================== */
/**
 * This operation behaves identically to the standard printf() function
 * with the exception that the offset at the begining of the line is 
 * determined by the current indent settings.
 * \pre 
 * - none
 *
 * \return
 * The number of characters sent to stdout.
 *
 * \post
 * - The formatted message will be sent to stdout.
 */
 /*lint -esym(534,lub_dump_printf) Ignoring return value of function */
int lub_dump_printf(
	    /**
             * printf-like format string
             */
			   const char *fmt, ...
    );
/**
 * This operation indicates that the offset for messages should be increased by
 * one level.
 *
 * \pre 
 * - none
 *
 * \post
 * - An indentation divider will be sent to stdout to emphasise the change
 *   in offset.
 * - Subsequent calls to lub_dump_printf() will output at this new offset.
 * - Client may call lub_undent() to restore offset.
 */
void lub_dump_indent(void);
/**
 * This operation indicates that the offset for messages should be decreased by
 * one level.
 *
 * \pre 
 * - lub_dump_indent() should have been called at least one more time than
 *   this function.
 *
 * \post
 * - An indentation divider will be sent to stdout to emphasise the change
 *   in offset.
 * - Subsequent calls to lub_dump_printf() will output at this new offset.
 */
void lub_dump_undent(void);

#endif				/* _lub_dump_h */
/** @} lub_dump */
