/***************************************************************************
 *            command_whogame.c
 *
 *  Fre Jänner 24 08:39:34 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_whogame.h"

void com_whogame(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_WHOGAME_FULL:
			append_to_message(out, L"WHOGAME_FULL gefunden.\n");
			break;

		default:
			append_to_message(out, L"WHOGAME unbekannt.\n");
			break;
	}
	return;
}
