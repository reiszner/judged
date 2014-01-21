/***************************************************************************
 *            string_wcs.h
 *
 *  Mon Jänner 13 07:56:34 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include <wctype.h>
#include <stdlib.h>

wchar_t *wcs_lc(wchar_t *);
int wcs_trim(wchar_t *, wchar_t *);
int wcs_trim2(wchar_t *);
unsigned long wcs_to_mbs_len(char **, wchar_t *);
