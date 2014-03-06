/***************************************************************************
 *            message.h
 *
 *  Mon Jänner 13 12:41:53 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <glib.h>
#include <sys/time.h>
#include <gmime/gmime.h>

#define MSGLEN 1024
#define BUFFER_SIZE 1024


#define MSGIN_BECOME        0
#define MSGIN_BROADCAST     1
#define MSGIN_CLEAR         2
#define MSGIN_CREATE        3
#define MSGIN_HELP          4
#define MSGIN_HISTORY       5
#define MSGIN_LANG          6
#define MSGIN_LIST          7
#define MSGIN_MAP           8
#define MSGIN_MYPRESS       9
#define MSGIN_OBSERVE      10
#define MSGIN_PHASE        11
#define MSGIN_PRESS        12
#define MSGIN_PROCESS      13
#define MSGIN_REGISTER     14
#define MSGIN_RESIGN       15
#define MSGIN_RESUME       16
#define MSGIN_ROLLBACK     17
#define MSGIN_SEND         18
#define MSGIN_SET          19
#define MSGIN_SIGNOFF      20
#define MSGIN_SIGNON       21
#define MSGIN_SUMMARY      22
#define MSGIN_TERMINATE    23
#define MSGIN_VERSION      24
#define MSGIN_WHOIS        25
#define MSGIN_WHOGAME      26

#define MSGIN_BECOME_MASTER   100

#define MSGIN_BROADCAST_END   200

#define MSGIN_GENERIC_FROM    300
#define MSGIN_GENERIC_TO      301
#define MSGIN_GENERIC_FOR     302
#define MSGIN_GENERIC_LINES   303

#define MSGIN_LIST_FULL       400

#define MSGIN_PRESS_TO        500
#define MSGIN_PRESS_ALL       501
#define MSGIN_PRESS_BUT       502
#define MSGIN_PRESS_END       503

#define MSGIN_REGISTER_END    600

#define MSGIN_SEND_DEDICATION 700
#define MSGIN_SEND_LOGFILE    701
#define MSGIN_SEND_PACKAGE    702
#define MSGIN_SEND_RESULT     703

#define MSGIN_SET_ADDRESS     800
#define MSGIN_SET_DENY        801

#define MSGIN_WHOGAME_FULL    900

#define MSGIN__WORDS          901

struct email_addr_t {
	wchar_t name[256];
	wchar_t email[256];
};

struct email_headers_t {
	wchar_t name[256];
	wchar_t email[256];
	wchar_t subject[MSGLEN];
	wchar_t msgid[MSGLEN];
	wchar_t references[MSGLEN];
	wchar_t gateway[MSGLEN];
};

struct ampersand_t {
	wchar_t *tag;
	wchar_t *sign;
};

struct message_proc_t {
	wchar_t *now;
	wchar_t *next;
	int pos;
	wchar_t line[MSGLEN];
	wchar_t linelc[MSGLEN];
};

struct buffer_t {
	wchar_t *buffer;
	int blocks;
	struct buffer_t *prev;
	struct buffer_t *next;
};

char *message_body;
wchar_t *input_wide;

struct email_addr_t *get_email_addr_from(GMimeMessage *);
int write_message(struct email_headers_t *, char *, struct timeval);
void add_mime_part(GMimeMessage *, char *);
void *capture_message(GMimeObject *);
void count_foreach_callback(GMimeObject *, GMimeObject *, gpointer);
int read_message(struct email_headers_t*, char *);
int html_to_plain(wchar_t *, wchar_t *);
int append_to_message(struct buffer_t *, wchar_t *);
void read_body(struct message_proc_t *, wchar_t *);
int take_option(struct message_proc_t *, wchar_t options[][MSGLEN], int);
void append_option(wchar_t *, wchar_t *);
