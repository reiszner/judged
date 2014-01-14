/***************************************************************************
 *            string_wcs.c
 *
 *  Mon Jänner 13 07:56:34 2014
 *  Copyright  2014  
 *  <user@host>
 ****************************************************************************/

#include "string_wcs.h"

wchar_t *wcs_lc(wchar_t *text) {
	unsigned int i;
	for ( i = 0 ; text[i] != '\0' ; i++ )
		if (iswupper(text[i])) text[i] = towlower(text[i]);
	return text;
}



wchar_t *wcs_trim(wchar_t *text) {
	unsigned int i, j;
	for (i = wcslen(text) - 1 ; iswspace(text[i]) ; i--) text[i] = 0L;
	for (i = 0 ; iswspace(text[i]) ; i++);
	for (j = 0 ; iswcntrl(text[i]) ; i++ , j++) text[j] = text[i];
	text[j] = 0L;
	return text;
}