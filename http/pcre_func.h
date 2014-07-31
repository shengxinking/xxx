/**
 *	@file	pcre_fun.h
 *
 *	@brief	the protection Macro, API define.
 *
 *
 */

#ifndef PCRE_H
#define	PCRE_H

/**********************************************************
 *	PCRE functions                                    *
 *********************************************************/
#include "pcre.h"
#define SIGNATURE_EXP_TYPE_NO_MATCH             0
#define SIGNATURE_EXP_TYPE_MATCH                1

/**
 *	Compile the PCRE expression into binary data
 *
 * 	Return pcre object if success, NULL on error.
 */
extern pcre *
pcre_compile_exp(const char *pattern, int options);

/**
 *	Study the PCRE object and save into pcre_extra
 *	object.
 *
 *	Return pcre_extra object if success, NULL on error.
 */
extern pcre_extra *
pcre_study_exp(pcre *re, int options);

/**
 *
 */
extern int
pcre_study_exp2(pcre *re, pcre_extra **extra);


extern int
pcre_match_exp(pcre *exp, pcre_extra *extra, char *buf,
		int len, int *begin, int *end);

extern int
pcre_match_exp2(pcre *exp, pcre_extra *extra, int index,
		char *buf, int len, int *begin, int *end);



#endif	/* endof PROTUTIL_H */


