/***************************************************************************
 *            command_become.c
 *
 *  Mon Jänner 20 13:28:41 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_become.h"

void com_become(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	if (!game) {
		append_to_message(out, L"Sie sind in keinem Spiel eingeloggt.\n");
		append_to_message(out, L"Brauche zuerst ein SIGNON.\n");
		return;
	}

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_BECOME_MASTER:
			append_to_message(out, L"BECOME_MASTER gefunden.\n");
			break;

		default:
			append_to_message(out, L"BECOME unbekannt.\n");
			break;
	}
	return;
}
