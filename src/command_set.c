/***************************************************************************
 *            command_set.c
 *
 *  Fre Jänner 24 08:54:06 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_set.h"

void com_set(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_SET_ADDRESS:
			append_to_message(out, L"SET_ADDRESS gefunden.\n");
			break;

		case MSGIN_SET_DENY:
			append_to_message(out, L"SET_DENY gefunden.\n");
			break;

		default:
			append_to_message(out, L"SET unbekannt.\n");
			break;
	}
	return;
}
