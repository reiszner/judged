/***************************************************************************
 *            mail_processing.h
 *
 *  Mon Jänner 20 03:26:33 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>

#define COM_BECOME 0
#define COM_BROADCAST 1
#define COM_CLEAR 2
#define COM_CREATE 3
#define COM_HELP 4
#define COM_HISTORY 5
#define COM_LANG 6
#define COM_LIST 7
#define COM_MAP 8
#define COM_MYPRESS 9
#define COM_OBSERVE 10
#define COM_PHASE 11
#define COM_PRESS 12
#define COM_PROCESS 13
#define COM_REGISTER 14
#define COM_RESIGN 15
#define COM_RESUME 16
#define COM_ROLLBACK 17
#define COM_SEND 18
#define COM_SET 19
#define COM_SIGNOFF 20
#define COM_SIGNON 21
#define COM_SUMMARY 22
#define COM_TERMINATE 23
#define COM_VERSION 24
#define COM_WHOIS 25
#define COM_WHOGAME 26

#define ALL__COM 27

int mail_processing (struct email_headers_t *, wchar_t *, struct buffer_t *);
