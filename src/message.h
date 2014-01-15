/***************************************************************************
 *            message.h
 *
 *  Mon Jänner 13 12:41:53 2014
 *  Copyright  2014  
 *  <user@host>
 ****************************************************************************/

#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <glib.h>
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
	wchar_t subject[1024];
	wchar_t msgid[1024];
	wchar_t references[1024];
	wchar_t gateway[1024];
};

struct ampersand_t {
	wchar_t *tag;
	wchar_t *sign;
};

char *message_body;
wchar_t *input_wide;

struct email_addr_t *get_email_addr_from(GMimeMessage *);
int write_message(struct email_headers_t *, char *);
void add_mime_part(GMimeMessage *, char *);
void *capture_message(GMimeObject *);
void count_foreach_callback(GMimeObject *, GMimeObject *, gpointer);
int read_message(struct email_headers_t*, char *);
int html_to_plain(wchar_t *, wchar_t *);
wchar_t *append_to_message(wchar_t *, wchar_t *, int *);
