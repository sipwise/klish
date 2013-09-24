/*
 * ctype.h
 */
/**
\ingroup lub
\defgroup lub_ctype ctype
@{

\brief The ANSI-C standard <ctype.h> interface works fine for strings 
containing characters encoded with the ASCII 7-bit coding. However 
when you use characters outside this range things start to get ambiguous. 

The default manner in which to represent a string in C is to use a 
"char *". (NB. this is a signed type) The interfaces presented 
in <ctype.h> take signed integers. When a character greater than 128 is
passed as a "char" to isspace() (e.g. the british pound sign "£") then 
a negative value is passed into the function/macro. A typical
implementation (e.g. VxWorks) may use the passed argument as an offset
into a lookup table, negative values in this case cause problems...

This utility provides an interface which avoids this ambiguity by passing 
"char" characters directly rather than converting to "int".

This component currently only contains those operations which are required
by the current CLISH/LUB implementations. It can be extended on an as needed 
basis.

*/
/*---------------------------------------------------------------
 * HISTORY
 * 4-Sep-2006		Graeme McKerrell	
 *    Initial Version
 *---------------------------------------------------------------
 * Copyright (C) 2006 Newport Networks. All Rights Reserved.
 *--------------------------------------------------------------- */
#ifndef _lub_ctype_h
#define _lub_ctype_h

#include "lub/types.h"

#include "lub/c_decl.h"
_BEGIN_C_DECL
/**
 * This operation identifies whether a character is a decimal digit
 * or not.
 *
 * \pre 
 * - none
 * 
 * \return 
 * BOOL_TRUE  - if the character is a decimal digit
 * BOOL_FALSE - if the character is not a decimal digit
 *
 * \post 
 * - none
 */
bool_t lub_ctype_isdigit(
	/** 
         * The character to check
         */
				char c);
/**
 * This operation identifies whether a character is a standard white space
 * character. (space, tab, carriage-return, vertical tab, form-feed)
 *
 * \pre 
 * - none
 * 
 * \return 
 * BOOL_TRUE  - if the character is white space
 * BOOL_FALSE - if the character is not white space
 *
 * \post 
 * - none
 */
bool_t lub_ctype_isspace(
	/** 
         * The character to check
         */
				char c);
/**
 * This operation converts an uppercase letter to the corresponding 
 * lowercase letter.
 *
 * \pre 
 * - none
 * 
 * \return 
 * If the parameter is a character for which lub_ctype_isupper() is true 
 * and there is a corresponding character for which lub_ctype_islower() is true
 * then the corresponding character is returned. Otherwise the parameter is 
 * returned unchanged.
 *
 * \post 
 * - none
 */
char lub_ctype_tolower(
	/** 
         * The character to convert
         */
			      char c);
/**
 * This operation converts a lowercase letter to the corresponding 
 * uppercase letter.
 *
 * \pre 
 * - none
 * 
 * \return 
 * If the parameter is a character for which lub_ctype_islower() is true 
 * and there is a corresponding character for which lub_ctype_isupper() is true
 * then the corresponding character is returned. Otherwise the parameter is 
 * returned unchanged.
 *
 * \post 
 * - none
 */
char lub_ctype_toupper(
	/** 
         * The character to convert
         */
			      char c);

_END_C_DECL
#endif				/* _lub_ctype_h */
/** @} */
