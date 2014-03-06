/***************************************************************************
 *            whois.h
 *
 *  Mon Jänner 13 07:47:38 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>

#ifndef _WHOIS_

typedef struct whois_t {
	int uid;
	int remind;
	int package;
	int postalcode;
	int power_real;
	int power_now;
	wchar_t country[256];
	wchar_t name[256];
	wchar_t email[256];
	wchar_t level[256];
	wchar_t language[256];
	wchar_t timezone[256];

	wchar_t birthday[256];
	wchar_t address[256];
	wchar_t site[256];
	wchar_t phone[256];
	wchar_t link[256];
	wchar_t source[256];
	wchar_t ip_address[256];
	wchar_t interests[256];
	int sex;
} Whois;



Whois *get_whois_by_uid(int);
Whois *get_whois_by_email(wchar_t *);
int update_whois(Whois *);
Whois *read_whois(FILE *);
int write_whois(Whois *, FILE *);

#define _WHOIS_
#endif