/***************************************************************************
 *            incoming.c
 *
 *  Die October 23 20:02:37 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "ipc.h"
#include "misc.h"
#include "string_wcs.h"
#include "message.h"
#include "whois.h"
#include "incoming.h"
#include "mail_processing.h"



int incoming (int partner) {

	pid_t pid;
	struct message msg;
	char judgedir[255], judgecode[255], gateway[255], temp[MSGLEN], *output_mbs = NULL, *input_mbs = NULL;
	wchar_t temp_wide[MSGLEN];
	int msgid, shmid = 0, flag = 1;
	void *shmdata;
	key_t ipc_key;
	struct timeval idtime;

	gettimeofday(&idtime, NULL);
	setlocale(LC_ALL, "");

	strcpy(judgedir, getenv("JUDGE_DIR"));
	strcpy(judgecode, getenv("JUDGE_CODE"));
	strcpy(gateway, getenv("JUDGE_GATEWAY"));
	sscanf(getenv("JUDGE_IPCKEY"), "%d", &ipc_key);
//	semid = get_semaphore (ipc_key);
	msgid = get_msgqueue (ipc_key);
	pid = getpid();

/* Say partner we are ready */
	send_msg (msgid, 0, partner, L"READY %d\0", pid);

	while (flag) {
		msgrcv(msgid, &msg, MSGLEN, pid, 0);

		if (wcsncmp(L"SHM_ID", msg.text, 6) == 0) {
			swscanf(msg.text + 6, L"%d", &shmid);
			shmdata = shmat(shmid, NULL, 0);
			if (shmdata == (void *) -1) {
				logging(LOG_ERR, L"%s: error while attach shared memory with ID %d\n", judgecode, shmid);
				send_msg (msgid, 0, partner, L"ERROR\0");
				return EXIT_FAILURE;
			}
			send_msg (msgid, 0, partner, L"SHM_OK\0");
		}

		else if (wcsncmp(L"SHM_DATA", msg.text, 8) == 0) {
			input_mbs = resive_sharedmemory(shmdata, msgid, pid, partner);
			if (input_mbs == NULL) {
				logging(LOG_ERR, L"%s: no message recived.\n", judgecode);
			}
		}

		else if (wcsncmp(L"SHM_WAIT", msg.text, 8) == 0) {
			flag = 0;
		}

		else if (wcsncmp(L"PROCESS", msg.text, 7) == 0) {
			return 0;
			flag = 0;
		}

		else {
			logging(LOG_ERR, L"%s: don't understand '%ls'\n", judgecode, msg.text);
			flag = 0;
		}

	}
	if (!input_mbs) return EXIT_FAILURE;
	output_mbs = NULL;






// Auswerten !!!

// we have receive a QUIT
	if (strncmp("From quit\n", input_mbs, 10) == 0) {
		shmdt(shmdata);
		send_msg (msgid, 0, partner, L"SHM_BYE\0");
		send_msg (msgid, 0, 1, L"%s\0", input_mbs);
	}

// we have receive a ATRUN
	else if (strncmp("From atrun ", input_mbs, 11) == 0) {
		shmdt(shmdata);
		send_msg (msgid, 0, partner, L"SHM_BYE\0");
		send_msg (msgid, 0, 1, L"%s\0", input_mbs);
	}

// we have receive a message
	else {

		struct email_headers_t *email_headers;
		email_headers = calloc(1, sizeof(struct email_headers_t));
		read_message(email_headers, input_mbs);

// we have a message, read it!!!
		if (input_wide) {

			struct buffer_t output_wide;
			output_wide.buffer = NULL;
			output_wide.blocks = 0;
			output_wide.next = NULL;
			output_wide.prev = NULL;







// this is only for testcases
			append_to_message(&output_wide, L"Nachricht erhalten.\n\n");
/*
			if (whois && whois->uid > 0) {

				wcsncpy(email_headers->name, whois->name, wcslen(whois->name));
				wcsncpy(email_headers->email, whois->email, wcslen(whois->email));
				if (wcsncmp(email_headers->subject, L"Re: ", 4) != 0) {
					wcsncpy(temp_wide, email_headers->subject, wcslen(email_headers->subject) + 1);
					wcsncpy(email_headers->subject, L"Re: \0", 5);
					wcsncat(email_headers->subject, temp_wide, wcslen(temp_wide) + 1);
				}

				output_wide = append_to_message(output_wide, L"Was ich über dich weis:\n\n", &out_blocks);
				swprintf(temp_wide, MSGLEN, L"User: %ld\n", whois->uid);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Remind: %d\n", whois->remind);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Postalcode: %d\n", whois->postalcode);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Country: %ls\n", whois->country);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Name: %ls\n", whois->name);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Email: %ls\n", whois->email);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Level: %ls\n", whois->level);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Language: %ls\n", whois->language);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Timezone: %ls\n", whois->timezone);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Birthday: %ls\n", whois->birthday);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Address: %ls\n", whois->address);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Site: %ls\n", whois->site);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Phone: %ls\n", whois->phone);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Link: %ls\n", whois->link);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Source: %ls\n", whois->source);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"IP-Address: %ls\n", whois->ip_address);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Interests: %ls\n", whois->interests);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
				swprintf(temp_wide, MSGLEN, L"Sex: %d\n", whois->sex);
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
			} else {

				swprintf(temp_wide, MSGLEN, L"Keine Information gefunden.\n\n");
				output_wide = append_to_message(output_wide, temp_wide, &out_blocks);
			}
*/
			swprintf(temp_wide, MSGLEN, L"\nerhaltene Nachricht:\n--------------------\n\n");
			append_to_message(&output_wide, temp_wide);

			mail_processing (email_headers, input_wide, &output_wide);

			if (input_wide) {
				free(input_wide);
				input_wide = NULL;
			}

			wcs_to_mbs_len(&output_mbs, output_wide.buffer);
			printf("---> Wide: %ld / Multi: %ld\n", wcslen(output_wide.buffer), strlen(output_mbs));

			free(output_wide.buffer);
			output_wide.buffer = NULL;

			logging(LOG_ERR, L"%s: schreibe Nachricht ...\n", judgecode);
			write_message(email_headers, output_mbs, idtime);


// this is action for dip
/*
			FILE *fp_dipc;
			time_t now;
			int semid, res;

			semaphore_operation (semid, 0, LOCK);
			sprintf(temp, "%s/dip -q", judgedir);
			fp_dipc = popen(temp, "w");
			if(fp_dipc == NULL) {
				sprintf(string_out, "%s: No conection to dip\n", judgecode);
				output(LOG_NOTICE, string_out);
			}
			else {
				res = fwrite(input_mbs, sizeof(wchar_t), strlen(input_mbs), fp_dipc);
				if (res != strlen(input_mbs)) {
					sprintf( string_out, "%s: write to dip has a problem - %d\n", judgecode, res);
					output(LOG_NOTICE, string_out);
				}
				pclose(fp_dipc);
			}
			semaphore_operation (semid, 0, UNLOCK);
			time(&now);
			send_msg (msgid, 0, 1, L"From atrun %ld\0", now + 61);
*/

// Abmelden
			shmdt(shmdata);
			send_msg (msgid, 0, partner, L"SHM_BYE\0");

// Aufräumen
			if (output_mbs) {
				free(output_mbs);
				output_mbs = NULL;
			}



// we have no plaintext and no HTML !!!
		} else {
			input_wide = calloc(MSGLEN, sizeof(wchar_t));
			swprintf(input_wide, MSGLEN, L"Sorry, could not find Plaintext or HTML in your Email.");
			output_mbs = calloc(MSGLEN, sizeof(char));
			wcstombs(output_mbs, input_wide, MSGLEN);
			free(input_wide);
			input_wide = NULL;

/* send answer per fifo or socket */
			if (wcslen(email_headers->gateway)) {
				wcstombs(temp, email_headers->gateway, MSGLEN);
				send_msg (msgid, 0, partner, L"SHM_ANSWER %s\0", temp);

				flag = send_sharedmemory(shmdata, output_mbs, msgid, pid, partner);
				if (flag) {
					logging(LOG_ERR, L"%s: send per sharedmemory apported.\n", judgecode);
				}
				shmdt(shmdata);
				send_msg (msgid, 0, partner, L"SHM_BYE\0");
			}

/* send answer per email */
			else {
				write_message(email_headers, output_mbs, idtime);
			}
			free(output_mbs);
			output_mbs = NULL;
			return -1;
		}


		if (email_headers) free(email_headers);

	}
	if (input_mbs) free(input_mbs);
	return 0;
}
/*
				if (loginput > 0) {
					time(&now);
					sprintf(temp, "%s/log/%ld-%d.log", judgedir, now, pid);
					syslog( LOG_NOTICE, "%s: write to logfile '%s'.\n", judgecode, temp);
					if((fp_temp = fopen(temp,"a")) == NULL) {
						syslog( LOG_NOTICE, "%s: can't create logfile '%s'.\n", judgecode, temp);
					}
					else {
						res = fwrite(msginput_mbs, sizeof(char), strlen(msginput_mbs), fp_temp);
						if ( res != strlen(msginput_mbs))
							syslog( LOG_NOTICE, "%s: write to logfile has a problem - %d\n", judgecode, res);
						fclose(fp_temp);
					}
				}
*/
