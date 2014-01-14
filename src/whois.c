/***************************************************************************
 *            whois.c
 *
 *  Mon Jänner 13 07:47:38 2014
 *  Copyright  2014  
 *  <user@host>
 ****************************************************************************/

#include "string_wcs.h"
#include "whois.h"


struct whois_t *get_whois_by_email(wchar_t *email) {
	struct whois_t *whois;
	FILE *whois_fp;
	char temp[1024];

	whois = malloc(sizeof(struct whois_t));
	memset(whois, 0, sizeof(struct whois_t));

	sprintf(temp, "%s/dip.whois", getenv("JUDGE_DIR"));
	if((whois_fp = fopen(temp, "r")) == NULL) return NULL;

	while (whois && !(wcsstr(whois->email, email)))
		whois = read_whois(whois_fp);

	fclose(whois_fp);
	return whois;
}



struct whois_t *get_whois_by_id(int uid) {
	struct whois_t *whois;
	FILE *whois_fp;
	char temp[1024];

	whois = malloc(sizeof(struct whois_t));
	memset(whois, 0, sizeof(struct whois_t));

	sprintf(temp, "%s/dip.whois", getenv("JUDGE_DIR"));
	if((whois_fp = fopen(temp, "r")) == NULL) return NULL;

	while (whois && whois->uid != uid)
		whois = read_whois(whois_fp);

	fclose(whois_fp);
	return whois;
}



int update_whois(struct whois_t *update) {
	struct whois_t *whois;
	FILE *whois_fp, *whoisb_fp;
	char temp[1024], temp2[1024];

	whois = malloc(sizeof(struct whois_t));
	memset(whois, 0, sizeof(struct whois_t));

	sprintf(temp, "%s/dip.whois", getenv("JUDGE_DIR"));
	sprintf(temp2, "%s/dip.whois.bak", getenv("JUDGE_DIR"));
	rename(temp, temp2);
	if((whoisb_fp = fopen(temp2, "r")) == NULL) return -1;
	if((whois_fp = fopen(temp, "w")) == NULL) return -1;

	while (whois) {
		whois = read_whois(whoisb_fp);

		if (whois->uid == update->uid)
			write_whois(update, whois_fp);
		else
			write_whois(whois, whois_fp);

	}
	fclose(whois_fp);
	fclose(whoisb_fp);
	return 0;
}



struct whois_t *read_whois(FILE *fp) {
	struct whois_t *whois;
	wchar_t line[1024], linelc[1024];

	whois = malloc(sizeof(struct whois_t));
	memset(whois, 0, sizeof(struct whois_t));

	while (fgetws(line, 1024, fp)) {
		wcsncpy(linelc, line, 1024);
		wcs_lc(linelc);

		if (wcsncmp(linelc, L"-", 1) == 0) break;

		else if (wcsncmp(linelc, L"user: ", 6) == 0)
			swscanf(line+6, L"%d", &whois->uid);

		else if (wcsncmp(linelc, L"remind: ", 8) == 0)
			swscanf(line+8, L"%d", &whois->remind);

		else if (wcsncmp(linelc, L"package: ", 9) == 0)
			swscanf(line+9, L"%d", &whois->package);

		else if (wcsncmp(linelc, L"postalcode: ", 12) == 0)
			swscanf(line+12, L"%d", &whois->postalcode);

		else if (wcsncmp(linelc, L"country: ", 9) == 0)
			swscanf(line+9, L"%s", wcs_trim(whois->country));

		else if (wcsncmp(linelc, L"name: ", 6) == 0)
			swscanf(line+6, L"%s", wcs_trim(whois->name));

		else if (wcsncmp(linelc, L"email: ", 7) == 0)
			swscanf(line+7, L"%s", wcs_trim(whois->email));

		else if (wcsncmp(linelc, L"level: ", 7) == 0)
			swscanf(line+7, L"%s", wcs_trim(whois->level));

		else if (wcsncmp(linelc, L"language: ", 10) == 0)
			swscanf(line+10, L"%s", wcs_trim(whois->language));

		else if (wcsncmp(linelc, L"timezone: ", 10) == 0)
			swscanf(line+10, L"%s", wcs_trim(whois->timezone));

		else if (wcsncmp(linelc, L"birth: ", 7) == 0)
			swscanf(line+7, L"%s", wcs_trim(whois->birthday));

		else if (wcsncmp(linelc, L"address: ", 9) == 0)
			swscanf(line+9, L"%s", wcs_trim(whois->address));

		else if (wcsncmp(linelc, L"site: ", 6) == 0)
			swscanf(line+6, L"%s", wcs_trim(whois->site));

		else if (wcsncmp(linelc, L"phone: ", 7) == 0)
			swscanf(line+7, L"%s", wcs_trim(whois->phone));

		else if (wcsncmp(linelc, L"link: ", 6) == 0)
			swscanf(line+6, L"%s", wcs_trim(whois->link));

		else if (wcsncmp(linelc, L"source: ", 8) == 0)
			swscanf(line+8, L"%s", wcs_trim(whois->source));

		else if (wcsncmp(linelc, L"ip-address: ", 12) == 0)
			swscanf(line+12, L"%s", wcs_trim(whois->ip_address));

		else if (wcsncmp(linelc, L"interests: ", 11) == 0)
			swscanf(line+11, L"%s", wcs_trim(whois->interests));

		else if (wcsncmp(linelc, L"sex: ", 5) == 0)
			swscanf(line+5, L"%d", &whois->sex);

	}
	if (whois->uid)
		return whois;
	else
		return NULL;
}



int write_whois(struct whois_t *whois, FILE *fp) {
	fwprintf(fp, L"User: %d\n", whois->uid);
	fwprintf(fp, L"Remind: %d\n", whois->remind);
	fwprintf(fp, L"Package: %d\n", whois->package);
	fwprintf(fp, L"Postalcode: %d\n", whois->postalcode);
	fwprintf(fp, L"Country: %s\n", whois->country);
	fwprintf(fp, L"Name: %s\n", whois->name);
	fwprintf(fp, L"Email: %s\n", whois->email);
	fwprintf(fp, L"Level: %s\n", whois->level);
	fwprintf(fp, L"Language: %s\n", whois->language);
	fwprintf(fp, L"Timezone: %s\n", whois->timezone);
	fwprintf(fp, L"Birthday: %s\n", whois->birthday);
	fwprintf(fp, L"Address: %s\n", whois->address);
	fwprintf(fp, L"Site: %s\n", whois->site);
	fwprintf(fp, L"Phone: %s\n", whois->phone);
	fwprintf(fp, L"Link: %s\n", whois->link);
	fwprintf(fp, L"Source: %s\n", whois->source);
	fwprintf(fp, L"IP-Address: %s\n", whois->ip_address);
	fwprintf(fp, L"Interests: %s\n", whois->interests);
	fwprintf(fp, L"Sex: %d\n", whois->sex);
	fputws(L"-----\n", fp);
	return 0;
}
