/***************************************************************************
 *            string_wcs.c
 *
 *  Mon Jänner 13 07:56:34 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"

wchar_t *wcs_lc(wchar_t *text) {
	unsigned int i;
	for ( i = 0 ; text[i] != '\0' ; i++ )
		if (iswupper(text[i])) text[i] = towlower(text[i]);
	return text;
}



int wcs_trim(wchar_t *dst, wchar_t *src) {
	unsigned int i, j;

	for (i = 0 ; iswspace(src[i]) ; i++ );
	for (j = 0 ; !iswcntrl(src[i]) ; i++, j++) dst[j] = src[i];
	dst[j] = 0L;
	if (wcslen(dst) < 1) return -1;
//	for (j = wcslen(dst) - 1 ; iswspace(dst[j]) ; j--) dst[j] = 0L;
	return 0;
}



int wcs_trim2(wchar_t *text) {
	unsigned int i, j;
	for (i = 0 ; iswspace(text[i]) ; i++ );
	for (j = 0 ; !iswcntrl(text[i]) ; i++, j++) text[j] = text[i];
	text[j] = 0L;
	if (wcslen(text) < 1) return -1;
	for (j = wcslen(text) - 1 ; iswspace(text[j]) ; j--) text[j] = 0L;
	return 0;
}



unsigned long wcs_to_mbs_len(char **mbs, wchar_t *wcs) {
	unsigned long i, j;

	for( i = j = 0 ; wcs[i] != 0L ; i++ ) {
		if (wcs[i] > 0x007F ) {
			if (wcs[i] > 0x07FF ) {
				if (wcs[i] > 0xFFFF ) j+=3;
				else j+=2;
			}
			else j++;
		}
		j++;
	}
	j++;

	*mbs = malloc(sizeof(char) * j);
	wcstombs(*mbs, wcs, j);
	return j;
}
