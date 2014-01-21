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

void read_become_opt(wchar_t becomes[][MSGLEN], wchar_t *lang) {
	wcsncpy(becomes[BECOME_MASTER],    L";master ;",    MSGLEN);
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
					if (wcscmp(variable, L"become_master"    ) == 0) wcsncat(becomes[BECOME_MASTER], buffer, wcslen(buffer));
				}
				fclose(fp);
			}
			token = wcstok(NULL, L",;", &state);
		}
	}
}



void com_become(struct message_proc_t *input, wchar_t *language, struct buffer_t *out) {
	wchar_t become_opt[ALL__BECOME][MSGLEN];
	int option;

	read_become_opt(become_opt, language);
	option = take_option(input, become_opt, ALL__BECOME);
	switch (option) {

		case BECOME_MASTER:
			append_to_message(out, L"MASTER gefunden!\n");
			break;

		default:
			append_to_message(out, L"unbekannter Become:\n");
			break;
	}
	return;
}
