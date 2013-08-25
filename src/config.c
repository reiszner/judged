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
	char buffer[1024], variable[512], *ptr;
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
	config->restart = 0;

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
		if(user_ptr == NULL) {
			syslog( LOG_NOTICE, "%s: user or UID '%s' not found. exit.\n", config->judgecode, config->judgeuser);
			return NULL;
		}
	}
	sscanf(user_ptr->pw_name, "%s", config->judgeuser);
	config->judgeuid = user_ptr->pw_uid;
	config->judgegid = user_ptr->pw_gid;

// find correct groupname and gid

	if (strlen(config->judgegroup) > 0) {
		group_ptr=getgrnam(config->judgegroup);
		if(group_ptr == NULL) {
			i = -1;
			sscanf(config->judgegroup ,"%d", &i);
			group_ptr=getgrgid(i);
		}
	}
	else {
		group_ptr=getgrgid(config->judgegid);
	}

	if(group_ptr == NULL) {
		syslog( LOG_NOTICE, "%s: group or GID '%s' not found. exit.\n", config->judgecode, config->judgegroup);
		return NULL;
	}
	sscanf(group_ptr->gr_name, "%s", config->judgegroup);
	config->judgegid = group_ptr->gr_gid;

// find correct judge directory

	if (strlen(config->judgedir) == 0) {
		sscanf(user_ptr->pw_dir, "%s", config->judgedir);
	}

	else if (strncmp("/", config->judgedir,  1) != 0) {
		sprintf(buffer, "%s/%s/", user_ptr->pw_dir, config->judgedir);
		sscanf(buffer, "%s",config->judgedir);
	}

// check if we are runable

	if (config->judgeuid == 0) {
		syslog( LOG_NOTICE, "%s: will not run as user '%s' (UID: %d). exit.\n", config->judgecode, config->judgeuser, config->judgeuid);
		return NULL;
	}

	if (config->judgegid == 0) {
		syslog( LOG_NOTICE, "%s: will not run as group '%s' (GID: %d). exit.\n", config->judgecode, config->judgegroup, config->judgegid);
		return NULL;
	}

	if (strlen(config->fifofile) == 0 && config->fifochilds > 0)
		config->fifochilds = 0;
	
	if (strlen(config->fifofile) > 0 && config->fifochilds == 0)
		config->fifofile[0] = '\0';

	if (strlen(config->inetsocket) == 0 && config->inetport > 0)
		config->inetport = 0;
	
	if (strlen(config->inetsocket) > 0 && config->inetport == 0)
		config->inetsocket[0] = '\0';

	if (strlen(config->unixsocket) == 0 && strlen(config->inetsocket) == 0 && strlen(config->fifofile) == 0 ) {
		syslog( LOG_NOTICE, "%s: have no canal to communicate. exit.\n", config->judgecode);
		return NULL;
	}

// set enviroment

	if (config->judgeuid > 0) {
		ptr = getenv("JUDGE_UID");
		if (ptr != NULL) sscanf(ptr, "%d", &i);
		if (ptr == NULL || i != config->judgeuid) {
			if (ptr == NULL) syslog( LOG_NOTICE, "%s: set user to '%s' (%d)\n", config->judgecode, config->judgeuser, config->judgeuid);
			else syslog( LOG_NOTICE, "%s: change user to '%s' (%d)\n", config->judgecode, config->judgeuser, config->judgeuid);
			sprintf(buffer,"%d",config->judgeuid);
			if( setenv("JUDGE_UID", buffer, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set JUDGE_UID\n", config->judgecode);
				return NULL;
			}
			config->restart |= ALL; 
		}
	} else if (config->judgeuid < 0) {
		syslog( LOG_NOTICE, "%s: no user/UID found in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	if (config->judgegid > 0) {
		ptr = getenv("JUDGE_GID");
		if (ptr != NULL) sscanf(ptr, "%d", &i);
		if (ptr == NULL || i != config->judgegid) {
			if (ptr == NULL) syslog( LOG_NOTICE, "%s: set group to '%s' (%d)\n", config->judgecode, config->judgegroup, config->judgegid);
			else syslog( LOG_NOTICE, "%s: change group to '%s' (%d)\n", config->judgecode, config->judgegroup, config->judgegid);
			sprintf(buffer,"%d",config->judgegid);
			if( setenv("JUDGE_GID", buffer, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set JUDGE_GID\n", config->judgecode);
				return NULL;
			}
			config->restart |= ALL;
		}
	} else if (config->judgegid < 0) {
		syslog( LOG_NOTICE, "%s: no group/GID found in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	if (strlen(config->judgedir) > 0) {
		ptr = getenv("JUDGE_DIR");
		if (ptr != NULL) i = strcmp(ptr, config->judgedir);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) syslog( LOG_NOTICE, "%s: set directory to '%s'\n", config->judgecode, config->judgedir);
			else syslog( LOG_NOTICE, "%s: change directory to '%s'\n", config->judgecode, config->judgedir);
			if( setenv("JUDGE_DIR", config->judgedir, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set JUDGE_DIR\n", config->judgecode);
				return NULL;
			}
			if(chdir(config->judgedir) == -1) {
				syslog( LOG_NOTICE, "%s: couldn't change to directory '%s'.\n", config->judgecode, config->judgedir);
				return NULL;
			}
			config->restart |= ALL;
		}
	}
	else {
		syslog( LOG_NOTICE, "%s: no directory found in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	if (strlen(config->judgecode) > 0) {
		ptr = getenv("JUDGE_CODE");
		if (ptr != NULL) i = strcmp(ptr, config->judgecode);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) syslog( LOG_NOTICE, "%s: set judgecode to '%s'\n", config->judgecode, config->judgecode);
			else syslog( LOG_NOTICE, "%s: change judgecode to '%s'\n", config->judgecode, config->judgecode);
			if( setenv("JUDGE_CODE", config->judgecode, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set JUDGE_CODE\n", config->judgecode);
				return NULL;
			}
			config->restart |= ALL;
		}
	}
	else {
		syslog( LOG_NOTICE, "%s: no judgecode found in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	if (strlen(config->ourselves) > 0) {
		ptr = getenv("JUDGE_SELVE");
		if (ptr != NULL) i = strcmp(ptr, config->ourselves);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) syslog( LOG_NOTICE, "%s: set ourselves to '%s'\n", config->judgecode, config->ourselves);
			else syslog( LOG_NOTICE, "%s: change ourselves to '%s'\n", config->judgecode, config->ourselves);
			if( setenv("JUDGE_SELVE", config->ourselves, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set JUDGE_SELVE\n", config->judgecode);
				return NULL;
			}
			config->restart |= ALL;
		}
	}
	else {
		syslog( LOG_NOTICE, "%s: no address for ourselve found in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	if (strlen(config->judgekeeper) > 0) {
		ptr = getenv("JUDGE_KEEPER");
		if (ptr != NULL) i = strcmp(ptr, config->judgekeeper);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) syslog( LOG_NOTICE, "%s: set judgekeeper to '%s'\n", config->judgecode, config->judgekeeper);
			else syslog( LOG_NOTICE, "%s: change judgekeeper to '%s'\n", config->judgecode, config->judgekeeper);
			if( setenv("JUDGE_KEEPER", config->judgekeeper, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set JUDGE_KEEPER\n", config->judgecode);
				return NULL;
			}
			config->restart |= ALL;
		}
	}
	else {
		syslog( LOG_NOTICE, "%s: no address for judgekeeper found in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	if (strlen(config->gateway) > 0) {
		ptr = getenv("JUDGE_GATEWAY");
		if (ptr != NULL) i = strcmp(ptr, config->gateway);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) syslog( LOG_NOTICE, "%s: set gateway to '%s'\n", config->judgecode, config->gateway);
			else syslog( LOG_NOTICE, "%s: change gateway to '%s'\n", config->judgecode, config->gateway);
			if (setenv("JUDGE_GATEWAY", config->gateway, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set JUDGE_GATEWAY\n", config->judgecode);
				return NULL;
			}
			config->restart |= ALL;
		}
	}
	else {
		syslog( LOG_NOTICE, "%s: no address for gateway found in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	ptr = getenv("JUDGE_PIDFILE");
	if (ptr == NULL && strlen(config->pidfile) > 0) {
		syslog( LOG_NOTICE, "%s: set pidfile to '%s'\n", config->judgecode, config->pidfile);
		if (setenv("JUDGE_PIDFILE", config->pidfile, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_PIDFILE\n", config->judgecode);
			return NULL;
		}
	}
	else if (ptr != NULL && strlen(config->pidfile) == 0) {
		sscanf(ptr,"%s",config->pidfile);
		syslog( LOG_NOTICE, "%s: can't run without pidfile! leaf it at '%s'\n", config->judgecode, config->pidfile);
	}
	else if (ptr != NULL && strlen(config->pidfile) > 0) {
		i = strcmp(ptr, config->pidfile);
		if (i != 0) {
			sscanf(ptr,"%s",config->pidfile);
			syslog( LOG_NOTICE, "%s: pidfile can't changed! leaf it at '%s'\n", config->judgecode, config->pidfile);
		}
	}
	else {
		syslog( LOG_NOTICE, "%s: no pidfile defined in configfile. exit.\n", config->judgecode);
		return NULL;
	}



	ptr = getenv("JUDGE_FIFO");
	if (ptr == NULL && strlen(config->fifofile) > 0) {
		syslog( LOG_NOTICE, "%s: set fifofile to '%s'\n", config->judgecode, config->fifofile);
		config->restart |= PIPE;
	}
	else if (ptr != NULL && strlen(config->fifofile) == 0) {
		syslog( LOG_NOTICE, "%s: unset fifofile.\n", config->judgecode);
		config->restart |= PIPE;
	}
	else if (ptr != NULL && strlen(config->fifofile) > 0) {
		i = strcmp(ptr, config->fifofile);
		if (i != 0) {
			syslog( LOG_NOTICE, "%s: change fifofile to '%s'\n", config->judgecode, config->fifofile);
			config->restart |= PIPE;
		}
	}
/*
		sprintf(buffer,"%d",config->fifochilds);
		if( setenv("JUDGE_FIFOCHILDS", buffer, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_FIFOCHILDS\n", config->judgecode);
			return NULL;
		}
		if( setenv("JUDGE_FIFO", config->fifofile, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_FIFO\n", config->judgecode);
			return NULL;
		}
*/



	ptr = getenv("JUDGE_UNIX");
	if (ptr == NULL && strlen(config->unixsocket) > 0) {
		syslog( LOG_NOTICE, "%s: set unixsocket to '%s'\n", config->judgecode, config->unixsocket);
		config->restart |= UNIX;
	}
	else if (ptr != NULL && strlen(config->unixsocket) == 0) {
		syslog( LOG_NOTICE, "%s: unset unixsocket.\n", config->judgecode);
		config->restart |= UNIX;
	}
	else if (ptr != NULL && strlen(config->unixsocket) > 0) {
		i = strcmp(ptr, config->unixsocket);
		if (i != 0) {
			syslog( LOG_NOTICE, "%s: change unixsocket to '%s'\n", config->judgecode, config->unixsocket);
			config->restart |= UNIX;
		}
	}
/*
	if( setenv("JUDGE_UNIX", config->unixsocket, 1) != 0 ) {
		syslog( LOG_NOTICE, "%s: couldn't set JUDGE_UNIX\n", config->judgecode);
		return NULL;
	}
*/



	if (strlen(config->inetsocket) > 0 && config->inetport > 0) {
		ptr = strtok(config->inetsocket, ", ");
		while(ptr != NULL) {
			for (i = 0 ; i < 5 ; i++) {
				sprintf(buffer, "JUDGE_INET%d", i);
				if ( strcmp(ptr, getenv(buffer)) == 0 ) break;
			}



			
			if( setenv(buffer, ptr, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set %s\n", config->judgecode, buffer);
				return NULL;
			}


			ptr = strtok(NULL, ", ");
		}
	}

	if (strlen(config->inetsocket) > 0 && config->inetport > 0) {
		i=0;
		ptr = strtok(config->inetsocket, ", ");
		while(ptr != NULL) {
			sprintf(buffer, "JUDGE_INET%d", i++);
			if( setenv(buffer, ptr, 1) != 0 ) {
				syslog( LOG_NOTICE, "%s: couldn't set %s\n", config->judgecode, buffer);
				return NULL;
			}
			ptr = strtok(NULL, ", ");
		}
		sprintf(buffer,"%d",config->inetport);
		if( setenv("JUDGE_INETPORT", buffer, 1) != 0 ) {
			syslog( LOG_NOTICE, "%s: couldn't set JUDGE_INETPORT\n", config->judgecode);
			return NULL;
		}
	}

	return config;
}
