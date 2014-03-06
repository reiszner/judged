/***************************************************************************
 *            mail_processing.c
 *
 *  Mon Jänner 20 03:26:33 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "string_wcs.h"
#include "message.h"
#include "whois.h"
#include "master.h"
#include "mail_processing.h"
#include "command_become.h"
#include "command_list.h"
#include "command_press.h"
#include "command_broadcast.h"
#include "command_register.h"
#include "command_send.h"
#include "command_whogame.h"
#include "command_generic.h"
#include "command_set.h"

void read_words(wchar_t words[][MSGLEN], wchar_t *lang) {
	int i;
	for ( i = 0 ; i < MSGIN__WORDS ; i++ ) words[i][0] = 0L;

	if (lang && wcslen(lang) > 0) {
		FILE *fp;
		char file[MSGLEN];
		wchar_t *token, *state, buffer[MSGLEN], variable[MSGLEN], *equal;
		int i;

		token = wcstok(lang, L" ,;", &state);
		while(token != NULL) {
			sprintf(file, "%s/commands.%ls.conf", getenv("JUDGE_DIR"), token);
			if ((fp = fopen(file, "r")) != 0) {
				while (feof(fp) == 0) {
					variable[0] = L'\0';
					fgetws(buffer, MSGLEN, fp);
					for ( i = 0 ; buffer[i] != L'#' && buffer[i] != L'\0' && buffer[i] != L'\n' ; i++ );
					buffer[i] = L'\0';

					wcsncpy(variable, buffer, MSGLEN);
					if ((equal = wcschr(variable, L'=')) == NULL) continue;
					*equal = L'\0';
					wcs_trim2(variable);
					wcs_uc(variable);

					for (i = 0 ; equal[i+1] != L'\0' ; i++) buffer[i] = equal[i+1];
					buffer[i] = L'\0';
					wcs_trim2(buffer);
					wcs_lc(buffer);

					if      (!wcscmp(variable, L"BECOME"          )) append_option(words[MSGIN_BECOME],      buffer);
					else if (!wcscmp(variable, L"BROADCAST"       )) append_option(words[MSGIN_BROADCAST],   buffer);
					else if (!wcscmp(variable, L"CLEAR"           )) append_option(words[MSGIN_CLEAR],       buffer);
					else if (!wcscmp(variable, L"CREATE"          )) append_option(words[MSGIN_CREATE],      buffer);
					else if (!wcscmp(variable, L"HELP"            )) append_option(words[MSGIN_HELP],        buffer);
					else if (!wcscmp(variable, L"HISTORY"         )) append_option(words[MSGIN_HISTORY],     buffer);
					else if (!wcscmp(variable, L"LANG"            )) append_option(words[MSGIN_LANG],        buffer);
					else if (!wcscmp(variable, L"LIST"            )) append_option(words[MSGIN_LIST],        buffer);
					else if (!wcscmp(variable, L"MAP"             )) append_option(words[MSGIN_MAP],         buffer);
					else if (!wcscmp(variable, L"MYPRESS"         )) append_option(words[MSGIN_MYPRESS],     buffer);
					else if (!wcscmp(variable, L"OBSERVE"         )) append_option(words[MSGIN_OBSERVE],     buffer);
					else if (!wcscmp(variable, L"PHASE"           )) append_option(words[MSGIN_PHASE],       buffer);
					else if (!wcscmp(variable, L"PRESS"           )) append_option(words[MSGIN_PRESS],       buffer);
					else if (!wcscmp(variable, L"PROCESS"         )) append_option(words[MSGIN_PROCESS],     buffer);
					else if (!wcscmp(variable, L"REGISTER"        )) append_option(words[MSGIN_REGISTER],    buffer);
					else if (!wcscmp(variable, L"RESIGN"          )) append_option(words[MSGIN_RESIGN],      buffer);
					else if (!wcscmp(variable, L"RESUME"          )) append_option(words[MSGIN_RESUME],      buffer);
					else if (!wcscmp(variable, L"ROLLBACK"        )) append_option(words[MSGIN_ROLLBACK],    buffer);
					else if (!wcscmp(variable, L"SEND"            )) append_option(words[MSGIN_SEND],        buffer);
					else if (!wcscmp(variable, L"SET"             )) append_option(words[MSGIN_SET],         buffer);
					else if (!wcscmp(variable, L"SIGNOFF"         )) append_option(words[MSGIN_SIGNOFF],     buffer);
					else if (!wcscmp(variable, L"SIGNON"          )) append_option(words[MSGIN_SIGNON],      buffer);
					else if (!wcscmp(variable, L"SUMMARY"         )) append_option(words[MSGIN_SUMMARY],     buffer);
					else if (!wcscmp(variable, L"TERMINATE"       )) append_option(words[MSGIN_TERMINATE],   buffer);
					else if (!wcscmp(variable, L"VERSION"         )) append_option(words[MSGIN_VERSION],     buffer);
					else if (!wcscmp(variable, L"WHOIS"           )) append_option(words[MSGIN_WHOIS],       buffer);
					else if (!wcscmp(variable, L"WHOGAME"         )) append_option(words[MSGIN_WHOGAME],     buffer);

					else if (!wcscmp(variable, L"BECOME_MASTER"   )) append_option(words[MSGIN_BECOME_MASTER],   buffer);

					else if (!wcscmp(variable, L"BROADCAST_END"   )) append_option(words[MSGIN_BROADCAST_END],   buffer);

					else if (!wcscmp(variable, L"GENERIC_FROM"    )) append_option(words[MSGIN_GENERIC_FROM],    buffer);
					else if (!wcscmp(variable, L"GENERIC_TO"      )) append_option(words[MSGIN_GENERIC_TO],      buffer);
					else if (!wcscmp(variable, L"GENERIC_FOR"     )) append_option(words[MSGIN_GENERIC_FOR],     buffer);
					else if (!wcscmp(variable, L"GENERIC_LINES"   )) append_option(words[MSGIN_GENERIC_LINES],   buffer);

					else if (!wcscmp(variable, L"LIST_FULL"       )) append_option(words[MSGIN_LIST_FULL],       buffer);

					else if (!wcscmp(variable, L"PRESS_TO"        )) append_option(words[MSGIN_PRESS_TO],        buffer);
					else if (!wcscmp(variable, L"PRESS_ALL"       )) append_option(words[MSGIN_PRESS_ALL],       buffer);
					else if (!wcscmp(variable, L"PRESS_BUT"       )) append_option(words[MSGIN_PRESS_BUT],       buffer);
					else if (!wcscmp(variable, L"PRESS_END"       )) append_option(words[MSGIN_PRESS_END],       buffer);

					else if (!wcscmp(variable, L"REGISTER_END"    )) append_option(words[MSGIN_REGISTER_END],    buffer);

					else if (!wcscmp(variable, L"SEND_DEDICATION" )) append_option(words[MSGIN_SEND_DEDICATION], buffer);
					else if (!wcscmp(variable, L"SEND_LOGFILE"    )) append_option(words[MSGIN_SEND_LOGFILE],    buffer);
					else if (!wcscmp(variable, L"SEND_PACKAGE"    )) append_option(words[MSGIN_SEND_PACKAGE],    buffer);
					else if (!wcscmp(variable, L"SEND_RESULT"     )) append_option(words[MSGIN_SEND_RESULT],     buffer);

					else if (!wcscmp(variable, L"SET_ADDRESS"     )) append_option(words[MSGIN_SET_ADDRESS],     buffer);
					else if (!wcscmp(variable, L"SET_DENY"        )) append_option(words[MSGIN_SET_DENY],        buffer);

					else if (!wcscmp(variable, L"WHOGAME_FULL"    )) append_option(words[MSGIN_WHOGAME_FULL],    buffer);
				}
				fclose(fp);
			}
			token = wcstok(NULL, L" ,;", &state);
		}
	}
	append_option(words[MSGIN_BECOME],      L";become ;");
	append_option(words[MSGIN_BROADCAST],   L";broadcast ;");
	append_option(words[MSGIN_CLEAR],       L";clear ;");
	append_option(words[MSGIN_CREATE],      L";create ;");
	append_option(words[MSGIN_HELP],        L";help ;");
	append_option(words[MSGIN_HISTORY],     L";history ;");
	append_option(words[MSGIN_LANG],        L";lang ;");
	append_option(words[MSGIN_LIST],        L";list ;");
	append_option(words[MSGIN_MAP],         L";map ;");
	append_option(words[MSGIN_MYPRESS],     L";mypress ;");
	append_option(words[MSGIN_OBSERVE],     L";observe ;");
	append_option(words[MSGIN_PHASE],       L";phase ;");
	append_option(words[MSGIN_PRESS],       L";press ;");
	append_option(words[MSGIN_PROCESS],     L";process ;");
	append_option(words[MSGIN_REGISTER],    L";register ;");
	append_option(words[MSGIN_RESIGN],      L";resign ;");
	append_option(words[MSGIN_RESUME],      L";resume ;");
	append_option(words[MSGIN_ROLLBACK],    L";rollback ;");
	append_option(words[MSGIN_SEND],        L";send ;get ");
	append_option(words[MSGIN_SET],         L";set ;");
	append_option(words[MSGIN_SIGNOFF],     L";signoff ;");
	append_option(words[MSGIN_SIGNON],      L";signon ;");
	append_option(words[MSGIN_SUMMARY],     L";summary ;");
	append_option(words[MSGIN_TERMINATE],   L";terminate ;");
	append_option(words[MSGIN_VERSION],     L";version ;");
	append_option(words[MSGIN_WHOIS],       L";whois ;");
	append_option(words[MSGIN_WHOGAME],     L";whogame ;");

	append_option(words[MSGIN_BECOME_MASTER],   L";master ;");

	append_option(words[MSGIN_BROADCAST_END],   L";endbroadcast ;endpress ;");

	append_option(words[MSGIN_GENERIC_FROM],    L";from ;");
	append_option(words[MSGIN_GENERIC_TO],      L";to ;");
	append_option(words[MSGIN_GENERIC_FOR],     L";for ;");
	append_option(words[MSGIN_GENERIC_LINES],   L";lines ;");

	append_option(words[MSGIN_LIST_FULL],       L";full ;");

	append_option(words[MSGIN_PRESS_TO],        L";to ;");
	append_option(words[MSGIN_PRESS_ALL],       L";all ;");
	append_option(words[MSGIN_PRESS_BUT],       L";but ;");
	append_option(words[MSGIN_PRESS_END],       L";endpress ;endbroadcast ;");

	append_option(words[MSGIN_REGISTER_END],    L";endregister ;end ;");

	append_option(words[MSGIN_SEND_DEDICATION], L";dedication ;");
	append_option(words[MSGIN_SEND_LOGFILE],    L";logfile ;");
	append_option(words[MSGIN_SEND_PACKAGE],    L";package ;");
	append_option(words[MSGIN_SEND_RESULT],     L";result ;");

	append_option(words[MSGIN_SET_ADDRESS],     L";address ;");
	append_option(words[MSGIN_SET_DENY],        L";deny ;");

	append_option(words[MSGIN_WHOGAME_FULL],    L";full ;");
}



int mail_processing (struct email_headers_t *headers, wchar_t *in, struct buffer_t *out) {
	struct message_proc_t input;
	Whois *whois;
	Game *game = NULL;
	wchar_t word[MSGIN__WORDS][MSGLEN];
	int option;

	input.now = NULL;
	input.next = in;

	whois = get_whois_by_email(headers->email);
	read_words(word, whois->language);

	while (input.next != NULL) {
		input.pos = 0;
		read_body(&input, L";\n");
		if (wcslen(input.line) == 0) continue;
		wcsncat(input.linelc, L" ", 1);
		option = take_option(&input, word, MSGIN__WORDS);
		switch (option) {

			case MSGIN_BECOME:
				com_become(&input, word, game, whois, out);
				break;

			case MSGIN_BROADCAST:
				com_broadcast(&input, word, game, whois, out);
				break;

			case MSGIN_CLEAR:
				append_to_message(out, L"CLEAR gefunden!\n");
//				com_clear(&input, game, whois, out);
				break;

			case MSGIN_CREATE:
				append_to_message(out, L"CREATE gefunden!\n");
//				com_create(&input, game, whois, out);
				break;

			case MSGIN_HELP:
				append_to_message(out, L"HELP gefunden!\n");
//				com_help(&input, game, whois, out);
				break;

			case MSGIN_HISTORY:
				append_to_message(out, L"HISTORY gefunden!\n");
//				com_history(&input, game, whois, out);
				break;

			case MSGIN_LANG:
				append_to_message(out, L"LANG gefunden!\n");
//				com_lang(&input, game, whois, out);
				break;

			case MSGIN_LIST:
				com_list(&input, word, game, whois, out);
				break;

			case MSGIN_MAP:
				append_to_message(out, L"MAP gefunden!\n");
//				com_map(&input, game, whois, out);
				break;

			case MSGIN_MYPRESS:
				append_to_message(out, L"MYPRESS gefunden!\n");
//				com_mypress(&input, game, whois, out);
				break;

			case MSGIN_OBSERVE:
				append_to_message(out, L"OBSERVE gefunden!\n");
//				com_observe(&input, game, whois, out);
				break;

			case MSGIN_PHASE:
				append_to_message(out, L"PHASE gefunden!\n");
//				com_phase(&input, game, whois, out);
				break;

			case MSGIN_PRESS:
				com_press(&input, word, game, whois, out);
				break;

			case MSGIN_PROCESS:
				append_to_message(out, L"PROCESS gefunden!\n");
//				com_process(&input, game, whois, out);
				break;

			case MSGIN_REGISTER:
				com_register(&input, word, game, whois, out);
				break;

			case MSGIN_RESIGN:
				append_to_message(out, L"RESIGN gefunden!\n");
//				com_resign(&input, game, whois, out);
				break;

			case MSGIN_RESUME:
				append_to_message(out, L"RESUME gefunden!\n");
//				com_resume(&input, game, whois, out);
				break;

			case MSGIN_ROLLBACK:
				append_to_message(out, L"ROLLBACK gefunden!\n");
//				com_rollback(&input, game, whois, out);
				break;

			case MSGIN_SEND:
				com_send(&input, word, game, whois, out);
				break;

			case MSGIN_SET:
				com_set(&input, word, game, whois, out);
				break;

			case MSGIN_SIGNOFF:
				append_to_message(out, L"SIGNOFF gefunden!\n");
//				com_signoff(&input, game, whois, out);
				break;

			case MSGIN_SIGNON:
				append_to_message(out, L"SIGNON gefunden!\n");
//				com_signon(&input, game, whois, out);
				break;

			case MSGIN_SUMMARY:
				append_to_message(out, L"SUMMARY gefunden!\n");
//				com_summary(&input, game, whois, out);
				break;

			case MSGIN_TERMINATE:
				append_to_message(out, L"TERMINATE gefunden!\n");
//				com_terminate(&input, game, whois, out);
				break;

			case MSGIN_VERSION:
				append_to_message(out, L"VERSION gefunden!\n");
//				com_version(&input, game, whois, out);
				break;

			case MSGIN_WHOIS:
				append_to_message(out, L"WHOIS gefunden!\n");
//				com_whois(&input, game, whois, out);
				break;

			case MSGIN_WHOGAME:
				com_whogame(&input, word, game, whois, out);
				break;

			default:
				append_to_message(out, L"unbekannter Befehl:\n");
				break;
		}

		append_to_message(out, input.line);
		append_to_message(out, L"\n");
	}

	if(whois) {
		free(whois);
		whois = NULL;
	}

	return 0;
}
