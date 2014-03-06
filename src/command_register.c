/***************************************************************************
 *            command_register.c
 *
 *  Fre Jänner 24 08:19:10 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_register.h"

void com_register(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_REGISTER_END:
			append_to_message(out, L"REGISTER_END gefunden.\n");
			break;

		default:
			append_to_message(out, L"REGISTER unbekannt.\n");
			break;
	}
	return;
}
