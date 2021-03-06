/***************************************************************************
 *            command_list.c
 *
 *  Die Jänner 21 03:11:04 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_list.h"

void com_list(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_LIST_FULL:
			append_to_message(out, L"LIST_FULL gefunden.\n");
			break;

		default:
			append_to_message(out, L"LIST unbekannt.\n");
			break;
	}
	return;
}
