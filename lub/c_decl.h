/* 
 * c_decl.h 
 * 
 * a simple set of macros to ease declaration of C interfaces.
 */
 /**
\ingroup lub
\defgroup lub_c_decl C linkage macros
@{
These two macros are used to simplify the declaration of C-linkage code.
Rather than worry about preprocessor directives similar to
\code
#ifdef __cplusplus
extern "C" {
#endif 

int foobar(void);

#ifdef __cplusplus
}
#endif
\endcode
you simply need to use the _BEGIN_C_DECL and _END_C_DECL macros instead.

\code
#include "lub/c_decl.h"
_BEGIN_C_DECL

int foobar(void);

_END_C_DECL
\endcode
*/

#ifndef _lub_c_decl_h
#define _lub_c_decl_h

#ifdef __cplusplus
#define _BEGIN_C_DECL extern "C" {
#define _END_C_DECL   }
#else				/* not __cplusplus */
#define _BEGIN_C_DECL
#define _END_C_DECL
#endif				/* not __cplusplus */

/** @} */
#endif				/* _lub_c_decl_h */
