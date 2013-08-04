/***************************************************************************
 *            config.c
 *
 *  Die Juli 02 01:25:11 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "config.h"

struct Config *read_config(struct Config *config)
{
	FILE *fp;
	int i;
	char buffer[1024], variable[512];
	struct passwd *user_ptr;
	struct group *group_ptr;

	if ((fp=fopen(config->file, "r")) == NULL) {
		return NULL;
	}

	config->judgeuser[0] = '\0';
	config->judgeuid = -1;
	config->judgegroup[0] = '\0';
	config->judgegid = -1;
	config->judgedir[0] = '\0';
	config->judgecode[0] = '\0';
	config->ourselves[0] = '\0';
	config->judgekeeper[0] = '\0';
	config->gateway[0] = '\0';
	config->pidfile[0] = '\0';
	config->unixsocket[0] = '\0';
	config->inetsocket[0] = '\0';
	config->inetport = 0;
	config->fifofile[0] = '\0';
	config->fifochilds = 3;
	config->loginput = 0;
	config->logoutput = 0;

	while (feof(fp) == 0) {
		fgets(buffer, 1024, fp );
		if (buffer[0] == '#') continue;
		if (buffer[0] == '\n') continue;

		for ( i=0 ; buffer[i] != '=' ; i++ );
		strncpy(variable,buffer,i);
		variable[i]='\0';
		strtolower(variable);
//		printf("found: '%s'\n",variable);
		if (strncmp("judgeuser",   variable,  9) == 0) sscanf(&buffer[i+1],"%s",config->judgeuser);
		if (strncmp("judgegroup",  variable, 10) == 0) sscanf(&buffer[i+1],"%s",config->judgegroup);
		if (strncmp("judgedir",    variable,  8) == 0) sscanf(&buffer[i+1],"%s",config->judgedir);
		if (strncmp("judgecode",   variable,  9) == 0) sscanf(&buffer[i+1],"%s",config->judgecode);
		if (strncmp("ourselves",   variable,  9) == 0) sscanf(&buffer[i+1],"%s",config->ourselves);
		if (strncmp("judgekeeper", variable, 11) == 0) sscanf(&buffer[i+1],"%s",config->judgekeeper);
		if (strncmp("gateway",     variable,  7) == 0) sscanf(&buffer[i+1],"%s",config->gateway);
		if (strncmp("pid-file",    variable,  8) == 0) sscanf(&buffer[i+1],"%s",config->pidfile);
		if (strncmp("unix-socket", variable, 11) == 0) sscanf(&buffer[i+1],"%s",config->unixsocket);
		if (strncmp("inet-socket", variable, 11) == 0) sscanf(&buffer[i+1],"%s",config->inetsocket);
		if (strncmp("inet-port",   variable,  9) == 0) sscanf(&buffer[i+1],"%d",&config->inetport);
		if (strncmp("fifo-file",   variable,  9) == 0) sscanf(&buffer[i+1],"%s",config->fifofile);
		if (strncmp("fifo-childs", variable, 11) == 0) sscanf(&buffer[i+1],"%d",&config->fifochilds);
		if (strncmp("log-input",   variable,  9) == 0) sscanf(&buffer[i+1],"%d",&config->loginput);
		if (strncmp("log-output",  variable, 10) == 0) sscanf(&buffer[i+1],"%d",&config->logoutput);
	}
	fclose(fp);

// find correct username and uid

	user_ptr=getpwnam(config->judgeuser);
	if(user_ptr == NULL) {
		i = -1;
		sscanf(config->judgeuser ,"%d", &i);
		user_ptr=getpwuid(i);
	}
	if(user_ptr == NULL) {
		printf("User or UID '%s' not found.\n", config->judgeuser);
		return NULL;
	}
	sscanf(user_ptr->pw_name, "%s", config->judgeuser);
	config->judgeuid = user_ptr->pw_uid;
	config->judgegid = user_ptr->pw_gid;

	if (strlen(config->judgegroup) > 0) {

// find correct groupname and gid

		group_ptr=getgrnam(config->judgegroup);
		if(group_ptr == NULL) {
			i = -1;
			sscanf(config->judgegroup ,"%d", &i);
			group_ptr=getgrgid(i);
		}
		if(group_ptr == NULL) {
			printf("Group or GID '%s' not found.\n", config->judgegroup);
			return NULL;
		}
		sscanf(group_ptr->gr_name, "%s", config->judgegroup);
		config->judgegid = group_ptr->gr_gid;

	}
	else {
		group_ptr=getgrgid(config->judgegid);
		if(group_ptr == NULL) {
			printf("Group or GID '%s' not found.\n", config->judgegroup);
			return NULL;
		}
		sscanf(group_ptr->gr_name, "%s", config->judgegroup);
		config->judgegid = group_ptr->gr_gid;
	}

	if (strlen(config->judgedir) == 0) {
		sscanf(user_ptr->pw_dir, "%s", config->judgedir);
	}

	else if (strncmp("/", config->judgedir,  1) != 0) {
		sprintf(buffer, "%s/%s/", user_ptr->pw_dir, config->judgedir);
		sscanf(buffer, "%s",config->judgedir);
	}

	return config;
}
