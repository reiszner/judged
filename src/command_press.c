/***************************************************************************
 *            comand_press.c
 *
 *  Fre Jänner 24 07:47:23 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "command_press.h"

void com_press(struct message_proc_t *input, wchar_t word[][MSGLEN], Game *game, Whois *whois, struct buffer_t *out) {
	int option;

	option = take_option(input, word, MSGIN__WORDS);
	switch (option) {

		case MSGIN_PRESS_TO:
			append_to_message(out, L"PRESS_TO gefunden.\n");
			break;

		case MSGIN_PRESS_ALL:
			append_to_message(out, L"PRESS_ALL gefunden.\n");
			break;

		case MSGIN_PRESS_BUT:
			append_to_message(out, L"PRESS_BUT gefunden.\n");
			break;

		case MSGIN_PRESS_END:
			append_to_message(out, L"PRESS_END gefunden.\n");
			break;

		default:
			append_to_message(out, L"PRESS unbekannt.\n");
			break;
	}
	return;
}
