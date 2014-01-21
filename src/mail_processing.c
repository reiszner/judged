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
#include "mail_processing.h"
#include "command_become.h"
#include "command_list.h"

void read_command_opt(wchar_t commands[][MSGLEN], wchar_t *lang) {
	wcsncpy(commands[COM_BECOME],    L";become ;",    MSGLEN);
	wcsncpy(commands[COM_BROADCAST], L";broadcast ;", MSGLEN);
	wcsncpy(commands[COM_CLEAR],     L";clear ;",     MSGLEN);
	wcsncpy(commands[COM_CREATE],    L";create ;",    MSGLEN);
	wcsncpy(commands[COM_HELP],      L";help ;",      MSGLEN);
	wcsncpy(commands[COM_HISTORY],   L";history ;",   MSGLEN);
	wcsncpy(commands[COM_LANG],      L";lang ;",      MSGLEN);
	wcsncpy(commands[COM_LIST],      L";list ;",      MSGLEN);
	wcsncpy(commands[COM_MAP],       L";map ;",       MSGLEN);
	wcsncpy(commands[COM_MYPRESS],   L";mypress ;",   MSGLEN);
	wcsncpy(commands[COM_OBSERVE],   L";observe ;",   MSGLEN);
	wcsncpy(commands[COM_PHASE],     L";phase ;",     MSGLEN);
	wcsncpy(commands[COM_PRESS],     L";press ;",     MSGLEN);
	wcsncpy(commands[COM_PROCESS],   L";process ;",   MSGLEN);
	wcsncpy(commands[COM_REGISTER],  L";register ;",  MSGLEN);
	wcsncpy(commands[COM_RESIGN],    L";resign ;",    MSGLEN);
	wcsncpy(commands[COM_RESUME],    L";resume ;",    MSGLEN);
	wcsncpy(commands[COM_ROLLBACK],  L";rollback ;",  MSGLEN);
	wcsncpy(commands[COM_SEND],      L";send ;get ",  MSGLEN);
	wcsncpy(commands[COM_SET],       L";set ;",       MSGLEN);
	wcsncpy(commands[COM_SIGNOFF],   L";signoff ;",   MSGLEN);
	wcsncpy(commands[COM_SIGNON],    L";signon ;",    MSGLEN);
	wcsncpy(commands[COM_SUMMARY],   L";summary ;",   MSGLEN);
	wcsncpy(commands[COM_TERMINATE], L";terminate ;", MSGLEN);
	wcsncpy(commands[COM_VERSION],   L";version ;",   MSGLEN);
	wcsncpy(commands[COM_WHOIS],     L";whois ;",     MSGLEN);
	wcsncpy(commands[COM_WHOGAME],   L";whogame ;",   MSGLEN);
	if (lang && wcslen(lang) > 0) {
		FILE *fp;
		char file[MSGLEN];
		wchar_t *token, *state, buffer[MSGLEN], variable[MSGLEN], *equal;
		int i;

		token = wcstok(lang, L",;", &state);
		while(token != NULL) {
			sprintf(file, "%s/commands.%ls.conf", getenv("JUDGE_DIR"), token);
			if ((fp = fopen(file, "r")) != 0) {
				while (feof(fp) == 0) {
					variable[0] = L'\0';
					fgetws(buffer, MSGLEN, fp);
					for ( i = 0 ; buffer[i] != L'#' && buffer[i] != L'\0' && buffer[i] != L'\n' ; i++ );
					buffer[i] = 0L;
					wcsncpy(variable, buffer, MSGLEN);
					if ((equal = wcschr(variable, L'=')) == NULL) continue;
					*equal = 0L;
					wcs_trim2(variable);
					wcs_lc(variable);
					equal = wcschr(buffer, L'=');
					for (i = 0 ; equal[i+1] != 0L ; i++)
						buffer[i] = equal[i+1];
					buffer[i] = 0L;
					wcs_trim2(buffer);
					wcs_lc(buffer);
//					printf("Variable: '%ls' Wert: '%ls'\n", variable, buffer);
					if      (wcscmp(variable, L"become"    ) == 0) {
						wcsncat(commands[COM_BECOME], buffer, wcslen(buffer));
//						printf("BECOME: '%ls'\n", commands[BECOME]);
					}
					else if (wcscmp(variable, L"broadcast" ) == 0) wcsncat(commands[COM_BROADCAST], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"clear"     ) == 0) wcsncat(commands[COM_CLEAR], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"help"      ) == 0) wcsncat(commands[COM_HELP], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"history"   ) == 0) wcsncat(commands[COM_HISTORY], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"lang"      ) == 0) wcsncat(commands[COM_LANG], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"list"      ) == 0) wcsncat(commands[COM_LIST], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"map"       ) == 0) wcsncat(commands[COM_MAP], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"mypress"   ) == 0) wcsncat(commands[COM_MYPRESS], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"observe"   ) == 0) wcsncat(commands[COM_OBSERVE], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"phase"     ) == 0) wcsncat(commands[COM_PHASE], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"press"     ) == 0) wcsncat(commands[COM_PRESS], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"register"  ) == 0) wcsncat(commands[COM_REGISTER], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"resign"    ) == 0) wcsncat(commands[COM_RESIGN], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"resume"    ) == 0) wcsncat(commands[COM_RESUME], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"send"      ) == 0) wcsncat(commands[COM_SEND], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"set"       ) == 0) wcsncat(commands[COM_SET], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"signon"    ) == 0) wcsncat(commands[COM_SIGNON], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"signoff"   ) == 0) wcsncat(commands[COM_SIGNOFF], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"summary"   ) == 0) wcsncat(commands[COM_SUMMARY], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"version"   ) == 0) wcsncat(commands[COM_VERSION], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"whois"     ) == 0) wcsncat(commands[COM_WHOIS], buffer, wcslen(buffer));
					else if (wcscmp(variable, L"whogame"   ) == 0) wcsncat(commands[COM_WHOGAME], buffer, wcslen(buffer));
				}
				fclose(fp);
			}
			token = wcstok(NULL, L",;", &state);
		}
	}
}



int mail_processing (struct email_headers_t *headers, wchar_t *in, struct buffer_t *out) {
	struct message_proc_t input;
	struct whois_t *whois;
	wchar_t command_opt[ALL__COM][MSGLEN];
	int option;
	void *game = NULL;

//	input = malloc(sizeof(struct message_proc_t));
	input.now = NULL;
	input.next = in;

	whois = get_whois_by_email(headers->email);
	read_command_opt(command_opt, whois->language);

	while (input.next != NULL) {
		input.pos = 0;
		read_body(&input, L";\n");
		if (wcslen(input.line) == 0) continue;
		wcsncat(input.linelc, L" ", 1);
		option = take_option(&input, command_opt, ALL__COM);
		switch (option) {

			case COM_BECOME:
				append_to_message(out, L"BECOME gefunden!\n");
				if (game) com_become(&input, whois->language, out);
				else append_to_message(out, L"Sie sind in keinem Spiel angemeldet!\n");
				break;

			case COM_BROADCAST:
				append_to_message(out, L"BROADCAST gefunden!\n");
				break;

			case COM_CLEAR:
				append_to_message(out, L"CLEAR gefunden!\n");
				break;

			case COM_CREATE:
				append_to_message(out, L"CREATE gefunden!\n");
				break;

			case COM_HELP:
				append_to_message(out, L"HELP gefunden!\n");
				break;

			case COM_HISTORY:
				append_to_message(out, L"HISTORY gefunden!\n");
				break;

			case COM_LANG:
				append_to_message(out, L"LANG gefunden!\n");
				break;

			case COM_LIST:
				if (game) com_list(&input, whois->language, out);
				else append_to_message(out, L"Sie sind in keinem Spiel angemeldet!\n");
				break;

			case COM_MAP:
				append_to_message(out, L"MAP gefunden!\n");
				break;

			case COM_MYPRESS:
				append_to_message(out, L"MYPRESS gefunden!\n");
				break;

			case COM_OBSERVE:
				append_to_message(out, L"OBSERVE gefunden!\n");
				break;

			case COM_PHASE:
				append_to_message(out, L"PHASE gefunden!\n");
				break;

			case COM_PRESS:
				append_to_message(out, L"PRESS gefunden!\n");
				break;

			case COM_PROCESS:
				append_to_message(out, L"PROCESS gefunden!\n");
				break;

			case COM_REGISTER:
				append_to_message(out, L"REGISTER gefunden!\n");
				break;

			case COM_RESIGN:
				append_to_message(out, L"RESIGN gefunden!\n");
				break;

			case COM_RESUME:
				append_to_message(out, L"RESUME gefunden!\n");
				break;

			case COM_ROLLBACK:
				append_to_message(out, L"ROLLBACK gefunden!\n");
				break;

			case COM_SEND:
				append_to_message(out, L"SEND gefunden!\n");
				break;

			case COM_SET:
				append_to_message(out, L"SET gefunden!\n");
				break;

			case COM_SIGNOFF:
				append_to_message(out, L"SIGNOFF gefunden!\n");
				break;

			case COM_SIGNON:
				append_to_message(out, L"SIGNON gefunden!\n");
				break;

			case COM_SUMMARY:
				append_to_message(out, L"SUMMARY gefunden!\n");
				break;

			case COM_TERMINATE:
				append_to_message(out, L"TERMINATE gefunden!\n");
				break;

			case COM_VERSION:
				append_to_message(out, L"VERSION gefunden!\n");
				break;

			case COM_WHOIS:
				append_to_message(out, L"WHOIS gefunden!\n");
				break;

			case COM_WHOGAME:
				append_to_message(out, L"WHOGAME gefunden!\n");
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
