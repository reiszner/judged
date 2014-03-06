/***************************************************************************
 *            command_generic.c
 *
 *  Fre Jänner 24 08:44:20 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_generic.h"

void com_generic(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_GENERIC_FROM:
			append_to_message(out, L"GENERIC_FROM gefunden.\n");
			break;

		case MSGIN_GENERIC_TO:
			append_to_message(out, L"GENERIC_TO gefunden.\n");
			break;

		case MSGIN_GENERIC_FOR:
			append_to_message(out, L"GENERIC_FOR gefunden.\n");
			break;

		case MSGIN_GENERIC_LINES:
			append_to_message(out, L"GENERIC_LINES gefunden.\n");
			break;

		default:
			append_to_message(out, L"GENERIC unbekannt.\n");
			break;
	}
	return;
}
