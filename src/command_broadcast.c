/***************************************************************************
 *            command_broadcast.c
 *
 *  Fre Jänner 24 08:09:40 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_broadcast.h"

void com_broadcast(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_BROADCAST_END:
			append_to_message(out, L"BROADCAST_END gefunden.\n");
			break;

		default:
			append_to_message(out, L"BROADCAST unbekannt.\n");
			break;
	}
	return;
}
