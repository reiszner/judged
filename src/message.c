/***************************************************************************
 *            message.c
 *
 *  Mon Jänner 13 12:41:53 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"


struct email_addr_t *get_email_addr_from(GMimeMessage *message) {

	struct email_addr_t *email_addr;
	const char *temp;
	char name[256], email[256];
	unsigned int i, cnt_n = 0, cnt_a = 0, mark = 0;

	email_addr = malloc(sizeof(struct email_addr_t));
	temp = g_mime_message_get_sender(message);

	for ( i = 0 ; temp[i] != '\0' ; i++ ) {

		if (temp[i] == '\\' && !(mark & 4)) {
			mark |= 4;
			continue;
		}

		if (temp[i] == '<' && !(mark & 1) && !(mark & 4)) {
			mark |= 2;
			cnt_a = 0;
			continue;
		}

		if (temp[i] == '>' && !(mark & 1) && !(mark & 4)) {
			mark &= ~2;
			continue;
		}

		if (temp[i] == '"' && !(mark & 1) && !(mark & 4)) {
			mark |= 1;
			cnt_n = 0;
			continue;
		}

		if (temp[i] == '"' && mark & 1 && !(mark & 4)) {
			mark &= ~1;
			continue;
		}

		if (mark & 2) email[cnt_a++] = temp[i];
		else name[cnt_n++] = temp[i];
		mark &= ~4;
	}

	name[cnt_n] = '\0';
	email[cnt_a] = '\0';

	if (cnt_a == 0) {
		strncpy(email, name, strlen(name) + 1);
		name[0] = '\0';
	}

	mbstowcs( email_addr->name, name, 256);
	mbstowcs( email_addr->email, email, 256);

	return email_addr;
}




int write_message(struct email_headers_t *headers, char *content, struct timeval idtime) {

	char temp1[MSGLEN] = "\0", temp2[MSGLEN] = "\0";
	GMimeMessage *message_out;
	time_t timenow;
	struct tm *sendtime;
	struct timeval idtimenow;

	gettimeofday(&idtimenow, NULL);
	message_out = g_mime_message_new(FALSE);

/* From */
	sprintf(temp1, "%s <%s>", getenv("JUDGE_NAME"), getenv("JUDGE_ADDR"));
	g_mime_message_set_sender(message_out, temp1);

/* To */
	wcstombs(temp1, headers->name, MSGLEN);
	wcstombs(temp2, headers->email, MSGLEN);
	g_mime_message_add_recipient(message_out, GMIME_RECIPIENT_TYPE_TO, temp1, temp2);

/* Subject */
	wcstombs(temp1, headers->subject, MSGLEN);
	g_mime_message_set_subject(message_out, temp1);

/* Date */
	time(&timenow);
	sendtime = localtime(&timenow);
	g_mime_message_set_date(message_out, timenow, ((sendtime->tm_gmtoff / 3600) * 100) + ((sendtime->tm_gmtoff % 3600) * 60 / 3600));

/* ID */
	sprintf(temp1, "%08lX.%05lX.%s", timenow, (long int) (idtimenow.tv_usec-idtime.tv_usec), getenv("JUDGE_ADDR"));
	g_mime_message_set_message_id(message_out, temp1);

/* References */

	if (wcslen(headers->msgid)) {
		wcstombs(temp1, headers->msgid, MSGLEN);
		if (wcslen(headers->references)) {
			wcstombs(temp2, headers->references, MSGLEN);
			strcat(temp2, " <");
			strcat(temp2, temp1);
			strcat(temp2, ">");
		}
		else
			sprintf(temp2, "<%s>", temp1);
		g_mime_object_append_header((GMimeObject *)message_out, "References", temp2);
	}

/* In-Reply-To */
	if (wcslen(headers->msgid)) {
		wcstombs(temp1, headers->msgid, MSGLEN);
		sprintf(temp2, "<%s>", temp1);
		g_mime_object_append_header((GMimeObject *)message_out, "In-Reply-To", temp2);
	}

	add_mime_part(message_out, content);

/* send out per sendmail */
	GMimeStream *stream;
	FILE *fp_mail;

	fp_mail = popen(getenv("JUDGE_SENDMAIL"), "w");

/* create a new stream for writing */
	stream = g_mime_stream_file_new(fp_mail);
	g_mime_stream_file_set_owner((GMimeStreamFile *) stream, FALSE);

/* write the message to the stream */
	g_mime_object_write_to_stream((GMimeObject *)message_out, stream);

/* flush the stream (kinda like fflush() in libc's stdio) */
	g_mime_stream_flush (stream);

	fclose(fp_mail);
	return 0;
}




void add_mime_part (GMimeMessage *message, char *message_out) {
	GMimeDataWrapper *content;
	GMimePart *mime_part;
	GMimeStream *stream;
	GMimeContentType *mime_type;

/* create the new part that we are going to add... */
	mime_part = g_mime_part_new();
	mime_type = g_mime_content_type_new("text", "plain");
	g_mime_content_type_set_parameter(mime_type, "charset", "UTF-8");
	g_mime_object_set_content_type( (GMimeObject *)mime_part, mime_type);

/* create the parts' content stream */
	stream = g_mime_stream_mem_new_with_buffer (message_out, strlen(message_out));

/*
create the content object - since the stream is not base64 or QP encoded or
anything, we can use GMIME_CONTENT_ENCODING_DEFAULT as the encoding type
(_DEFAULT is the same as saying "nothing specified")
*/
	content = g_mime_data_wrapper_new_with_stream (stream, GMIME_CONTENT_ENCODING_DEFAULT);
	g_object_unref (stream);

/* set the content object on the new mime part */
	g_mime_part_set_content_object (mime_part, content);
	g_object_unref (content);

/* if we want, we can tell GMime that the content should be base64 encoded when written to disk... */
	g_mime_part_set_content_encoding (mime_part, g_mime_part_get_best_content_encoding (mime_part, GMIME_ENCODING_CONSTRAINT_7BIT));

/* the "polite" way to modify a mime structure that we didn't
 create is to create a new toplevel multipart/mixed part and
 add the previous toplevel part as one of the subparts as
 well as our text part that we just created... */

/* create a multipart/mixed part */
//	multipart = g_mime_multipart_new_with_subtype ("mixed");

/* add our new text part to it */
//	g_mime_multipart_add (multipart, (GMimeObject *) mime_part);
//	g_object_unref (mime_part);

/* now append the message's toplevel part to our multipart */
//	g_mime_multipart_add (multipart, message->mime_part);

/* now replace the message's toplevel mime part with our new multipart */
	g_mime_message_set_mime_part(message, (GMimeObject *)mime_part);
//	g_object_unref (multipart);
}



void *capture_message (GMimeObject *message) {

	GMimeStream *wstream, *fstream;
	GMimeDataWrapper *wrapper;
	GMimeFilter *filter;
	char *output;
	int length;

	wrapper = g_mime_part_get_content_object((GMimePart *)message);

/* create a new stream for writing to output */
	wstream = g_mime_stream_mem_new ();
	g_mime_stream_mem_set_owner ((GMimeStreamMem *) wstream, FALSE);

/* create a filtered stream */
	fstream = g_mime_stream_filter_new (wstream);

/* filter for charset */
	if (g_mime_content_type_get_parameter(g_mime_object_get_content_type(message),"charset") != NULL ) {
		filter = g_mime_filter_charset_new(g_mime_content_type_get_parameter(g_mime_object_get_content_type(message),"charset"), "UTF-8");
		g_mime_stream_filter_add ((GMimeStreamFilter *)fstream, filter);
		g_object_unref (filter);
	}

/* filter for CRLF */
	filter = g_mime_filter_crlf_new(FALSE, FALSE);
	g_mime_stream_filter_add ((GMimeStreamFilter *)fstream, filter);
	g_object_unref (filter);

/* write the message to the stream */
	g_mime_data_wrapper_write_to_stream (wrapper, fstream);
	g_object_unref (wrapper);
	g_mime_stream_reset(wstream);

//	wprintf(L"Lang: %ld\n", g_mime_stream_length(wstream));
	if ( (length = g_mime_stream_length(wstream)) > 0 ) {
		length++;
		output = calloc(length, sizeof(char));
		length = g_mime_stream_read(wstream, output, length);
		output[length] = '\0';
	}

	g_object_unref (wstream);
	g_object_unref (fstream);
	return output;
}


void count_foreach_callback (GMimeObject *parent, GMimeObject *part, gpointer user_data) {
	int *count = user_data;
	wchar_t *message_temp = NULL;

/* 'part' points to the current part node that
 * g_mime_message_foreach() is iterating over */

/* find out what class 'part' is... */
	if (GMIME_IS_MESSAGE_PART (part)) {

/* message/rfc822 or message/news */
		GMimeMessage *message;

/* g_mime_message_foreach() won't descend into
child message parts, so if we want to count any
subparts of this child message, we'll have to call
g_mime_message_foreach() again here. */

		message = g_mime_message_part_get_message ((GMimeMessagePart *) part);
		g_mime_message_foreach (message, count_foreach_callback, count);

	} else if (GMIME_IS_MESSAGE_PARTIAL (part)) {
/* message/partial */

/* this is an incomplete message part, probably a
 large message that the sender has broken into
 smaller parts and is sending us bit by bit. we
 could save some info about it so that we could
 piece this back together again once we get all the
 parts? */

	} else if (GMIME_IS_MULTIPART (part)) {
/* multipart/mixed, multipart/alternative,
 * multipart/related, multipart/signed,
 * multipart/encrypted, etc... */

/*
		printf("------------------------------\n");
		printf("Part is Multipart!!!\n");
		printf("Type      : '%s'\n",g_mime_content_type_to_string(g_mime_object_get_content_type(part)));
		printf("Media-Type: '%s'\n",g_mime_content_type_get_media_type (g_mime_object_get_content_type(part)));
		printf("Sub-Type  : '%s'\n",g_mime_content_type_get_media_subtype (g_mime_object_get_content_type(part)));
		printf("Preface   : '%s'\n",g_mime_multipart_get_preface((GMimeMultipart *)part));
		printf("Postface  : '%s'\n",g_mime_multipart_get_postface((GMimeMultipart *)part));
		printf("Anzahl    : %d\n",  g_mime_multipart_get_count((GMimeMultipart *)part));
		printf("Index     : %d\n",  g_mime_multipart_index_of((GMimeMultipart *)parent, part));
		printf("ID: '%s'\n\n",g_mime_object_get_content_id(part));
		printf("------------------------------\n\n");
*/

/* we'll get to finding out if this is a
 * signed/encrypted multipart later... */

	} else if (GMIME_IS_PART (part)) {
/* a normal leaf part, could be text/plain or
 * image/jpeg etc */

/*
		printf("------------------------------\n");
		printf("Type      : '%s'\n",g_mime_content_type_to_string(g_mime_object_get_content_type(part)));
		printf("Media-Type: '%s'\n",g_mime_content_type_get_media_type (g_mime_object_get_content_type(part)));
		printf("Sub-Type  : '%s'\n",g_mime_content_type_get_media_subtype (g_mime_object_get_content_type(part)));
		printf("Header    : '%s'\n",g_mime_object_get_headers (part));
*/
		if (g_mime_content_type_is_type(g_mime_object_get_content_type(part), "text", "*")) {
/*
			printf("Parameter : '%s'\n",g_mime_content_type_get_parameter(g_mime_object_get_content_type(part),"charset"));
			printf("ID        : '%s'\n",g_mime_object_get_content_id(part));
			printf("Content-Description : '%s'\n",g_mime_part_get_content_description ((GMimePart *)part));
			printf("Content-ID          : '%s'\n",g_mime_part_get_content_id ((GMimePart *)part));
			printf("Content-Location    : '%s'\n",g_mime_part_get_content_location ((GMimePart *)part));
			printf("Content-Encoding    : '%s'\n",g_mime_content_encoding_to_string(g_mime_part_get_content_encoding ((GMimePart *)part)));
			printf("Content-Best        : '%s'\n",g_mime_content_encoding_to_string(g_mime_part_get_best_content_encoding ((GMimePart *)part, GMIME_ENCODING_CONSTRAINT_7BIT)));
*/
			if (g_mime_content_type_is_type(g_mime_object_get_content_type(part), "text", "plain")) {
				if (message_body == NULL) {
					message_body = capture_message(part);
					input_wide = calloc(strlen(message_body) + 1, sizeof(wchar_t));
					mbstowcs(input_wide, message_body, strlen(message_body) + 1);
				}
			}

			if (g_mime_content_type_is_type(g_mime_object_get_content_type(part), "text", "html")) {
				if (message_body == NULL) {
					message_body = capture_message(part);
					message_temp = calloc(strlen(message_body) + 1, sizeof(wchar_t));
					input_wide = calloc(strlen(message_body) + 1, sizeof(wchar_t));
					mbstowcs(message_temp, message_body, strlen(message_body) + 1);
					html_to_plain(input_wide, message_temp);
					free(message_temp);
					message_temp = NULL;
				}
			}

		}

	} else {
		g_assert_not_reached ();
	}
}



int read_message(struct email_headers_t *headers, char *input_buffer) {
	int count;
	GMimeMessage *message;
	GMimeParser *parser;
	GMimeStream *stream;
	struct email_addr_t *email_addr_from;

/* init the gmime library */
	g_mime_init (0);

/* parse the message */
/* create a stream to read from the file descriptor */
	stream = g_mime_stream_mem_new_with_buffer (input_buffer, strlen(input_buffer)+1);
/* create a new parser object to parse the stream */
	parser = g_mime_parser_new_with_stream (stream);
/* unref the stream (parser owns a ref, so this object does not actually get free'd until we destroy the parser) */
	g_object_unref (stream);
/* parse the message from the stream */
	message = g_mime_parser_construct_message (parser);
/* free the parser (and the stream) */
	g_object_unref (parser);
	count = 0;
	g_mime_message_foreach(message, count_foreach_callback, &count);

	/* From */
	if ((email_addr_from = get_email_addr_from(message))) {
		wcsncpy(headers->name , email_addr_from->name , MSGLEN);
		wcsncpy(headers->email, email_addr_from->email, MSGLEN);
	}

/* Subject */
	if (g_mime_message_get_subject(message))
		mbstowcs(headers->subject, g_mime_message_get_subject(message), MSGLEN);

/* Message-ID */
	if (g_mime_message_get_message_id(message))
		mbstowcs(headers->msgid, g_mime_message_get_message_id(message), MSGLEN);

/* References */
	if (g_mime_object_get_header((GMimeObject *)message,"references"))
		mbstowcs(headers->references, g_mime_object_get_header((GMimeObject *)message,"references"), MSGLEN);

/* special headers */
	if (g_mime_object_get_header((GMimeObject *)message,"x-judgecode"))
		mbstowcs(headers->gateway, g_mime_object_get_header((GMimeObject *)message,"x-judgecode"), MSGLEN);

	return 0;
}



int html_to_plain(wchar_t *plain, wchar_t *html) {

	struct ampersand_t ampersand[]={
#include "ampersand.code"
	};

	wchar_t *character, tag[32];
	unsigned long int html_offset = 0, plain_offset = 0, tag_offset, tag_length, jump = 0, nl = 0, quote = 0, i, j;

	while ( html[html_offset] != '\0') {

		if (html_offset >= wcslen(html))
			break;

		character = html + html_offset;

//		wprintf(L"Text-Offset: %d --- HTML-Offset: %d --- Zeichen: '%lc' --- Wert: %ld\n", plain_offset, html_offset, character[0], character[0]);

		if (character[0] == '\n' || character[0] == '\t' ) {
			html_offset++;
			continue;
		}

		else if (character[0] == '<') {
			tag_offset = html_offset + 1;
			tag_length = 0;
//			wprintf(L"Tag found at %d!\n", tag_offset);

//  search tag-keyword
			while (html[tag_offset + tag_length] != ' ' && html[tag_offset + tag_length] != '>' && html[tag_offset + tag_length] != '\n' ) tag_length++;

//  take tag-keyword as lowercase
//			wprintf(L"Tag_Offset: %d --- Tag_Length: %d\n", tag_offset, tag_length);
			wcsncpy(tag, html + tag_offset, tag_length);
			tag[tag_length] = '\0';
			wcs_lc(tag);
//			wprintf(L"Tag: '%ls'\n", tag);

//  move html_offset at end of tag
//			html_offset = tag_offset + tag_length;
			html_offset += tag_length;
			while (html[html_offset] != '>') html_offset++;

//  unset character because we have a html-tag
			character = NULL;

//			wprintf(L"Tag: %ls\n", tag);

			if (wcslen(tag) == 1 && wcsncmp(tag, L"p", 1) == 0)
				character = L"\n";

			else if (wcslen(tag) == 2) {
				if (wcsncmp(tag, L"/p", 2) == 0)
					character = L"\n";

				else if (wcsncmp(tag, L"br", 2) == 0)
					character = L"\n";
			}

			if (wcslen(tag) == 3) {
				if (wcsncmp(tag, L"/td", 3) == 0) {
					wcsncpy(plain + plain_offset, L" ", 1);
					plain_offset++;
				}

				else if (wcsncmp(tag, L"/tr", 3) == 0) {
					if (nl == 0) character = L"\n";
				}

				else if (wcsncmp(tag, L"div", 3) == 0) {
					if (nl == 0) character = L"\n";
				}
			}

			if (wcslen(tag) == 4) {
				if (wcsncmp(tag, L"body", 4) == 0)
				plain_offset = 0;

				else if (wcsncmp(tag, L"/div", 4) == 0)
					if (nl == 0) character = L"\n";
			}

			if (wcslen(tag) == 5) {
				if (wcsncmp(tag, L"style", 5) == 0)
					jump = 1;

				else if (wcsncmp(tag, L"/body", 5) == 0)
					jump = 1;

				else if (wcsncmp(tag, L"table", 5) == 0)
					if (nl == 0) character = L"\n";
			}

			if (wcslen(tag) == 6) {
				if (wcsncmp(tag, L"/style", 6) == 0)
					jump = 0;

				else if (wcsncmp(tag, L"/table", 6) == 0)
					if (nl == 0) character = L"\n";
			}

			if (wcslen(tag) == 10 && wcsncmp(tag, L"blockquote", 10) == 0) {
				quote++;
				character = L"\n";
			}

			if (wcslen(tag) == 11 && wcsncmp(tag, L"/blockquote", 11) == 0) {
				quote--;
				character = L"\n";
			}

		}

		else if (character[0] == '&') {
			tag_offset = html_offset + 1;
			for ( tag_length = 0 ; html[tag_offset + tag_length] != ';' && tag_length < 10 ; tag_length++ );

			character = html + tag_offset + tag_length;

			if (character[0] == ';') {
				wcsncpy(tag, html + tag_offset, tag_length);
				tag[tag_length] = '\0';

				if (tag[0] == '#') {
					tag[0] = '0';
					if (tag[1] == 'x' || tag[1] == 'X' ) swscanf(tag, L"%x" ,&i);
					else swscanf(tag + 1, L"%d" ,&i);
					if (i > 31) plain[plain_offset++] = i;
				} else {
					for ( i = 0 ; i < (sizeof(ampersand) / sizeof(struct ampersand_t)) ; i++ ) {
						if (wcsncmp(tag, ampersand[i].tag, wcslen(ampersand[i].tag)) == 0) {
							character = ampersand[i].sign;
							break;
						}
					}
					if (i < (sizeof(ampersand) / sizeof(struct ampersand_t))) {
						wcsncpy(plain + plain_offset, ampersand[i].sign, wcslen(ampersand[i].sign));
						plain_offset += wcslen(ampersand[i].sign);
					}
					else {
						wcsncpy(plain + plain_offset, tag, tag_length);
						plain_offset += tag_length;
					}
				}
				character = NULL;
				html_offset += tag_length + 1;
			} else {
				character = L"&";
			}
		}

		if (jump == 0 && character != NULL) {
			if (character[0] != '\n') {
				if (nl > 0) {
					for (i = 0 ; i < nl && i < 2 ; i++ ) {
						plain[plain_offset++] = '\n';
						for (j = 0 ; j < quote ; j++ ) {
							plain[plain_offset++] = '>';
							plain[plain_offset++] = ' ';
						}
					}
					nl = 0;
				}
				plain[plain_offset++] = character[0];
			} else {
				nl++;
			}
		}

		html_offset++;
	}
	plain[plain_offset++] = '\n';
	plain[plain_offset++] = '\0';
	return plain_offset;
}



int append_to_message(struct buffer_t *message, wchar_t *line) {
	wchar_t *newbuffer = NULL;

	if (message->buffer == NULL) {
		message->buffer = malloc(sizeof(wchar_t) * BUFFER_SIZE * ++message->blocks);
	}
	else if((wcslen(message->buffer) + wcslen(line)) > (BUFFER_SIZE * message->blocks)) {
		newbuffer = realloc(message->buffer, sizeof(wchar_t) * BUFFER_SIZE * ++message->blocks);
		if (newbuffer) message->buffer = newbuffer;
		return -1;
	}
	wcscat(message->buffer, line);
	return 0;
}

void read_body(struct message_proc_t *body, wchar_t *lineend) {
	wchar_t ch;
	*body->line = 0L;
	*body->linelc = 0L;

	if (body->next) {
		if (body->now)
			body->now = body->next + 1;
		else
			body->now = body->next;
	} else {
		body->now = NULL;
		return;
	}

	body->next = wcspbrk(body->now, lineend);
	if (body->next) {
		ch = *body->next;
		*body->next = 0L;
	}

	wcs_trim(body->line, body->now);
	wcscpy(body->linelc, body->line);
	wcs_lc(body->linelc);

	if (body->next)
		*body->next = ch;

	return;
}



int take_option(struct message_proc_t *check, wchar_t options[][MSGLEN], int max) {
	wchar_t *token, *state, temp[MSGLEN];
	int i;
	printf("pruefe: '%ls'\n", check->line);
	for (i = 0 ; i < max ; i++) {
		wcsncpy(temp, options[i], MSGLEN);
		token = wcstok(temp, L";", &state);
		printf("Line: '%ls' --> pos: %d\n", check->linelc + check->pos, check->pos);
		while(token != NULL) {
			printf("Token: '%ls'\n", token);
			if (wcsncmp(check->linelc + check->pos, token, wcslen(token)) == 0) {
				check->pos += wcslen(token);
				break;
			}
			token = wcstok(NULL, L";", &state);
		}
		if (token) break;
	}
	return i;
}

