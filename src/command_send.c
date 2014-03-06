/***************************************************************************
 *            command_send.c
 *
 *  Fre Jänner 24 08:26:30 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_send.h"

void com_send(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_SEND_DEDICATION:
			append_to_message(out, L"SEND_DEDICATION gefunden.\n");
			break;

		case MSGIN_SEND_LOGFILE:
			append_to_message(out, L"SEND_LOGFILE gefunden.\n");
			break;

		case MSGIN_SEND_PACKAGE:
			append_to_message(out, L"SEND_PACKAGE gefunden.\n");
			break;

		case MSGIN_SEND_RESULT:
			append_to_message(out, L"SEND_RESULT gefunden.\n");
			break;

		default:
			append_to_message(out, L"SEND unbekannt.\n");
			break;
	}
	return;
}
