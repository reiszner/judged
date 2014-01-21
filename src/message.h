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
