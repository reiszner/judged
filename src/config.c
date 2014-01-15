/***************************************************************************
 *            config.c
 *
 *  Die Juli 02 01:25:11 2013
 *  Copyright  2013  Sascha Reißner
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include "misc.h"
#include "config.h"

struct Config *read_config(struct Config *config_old, struct Config *params)
{
	FILE *fp;
	int i, j;
	char buffer[1024], variable[1024], string_out[1024], *ptr;
	struct Config config;
	struct passwd *user_ptr;
	struct group *group_ptr;

	if ((fp=fopen(config_old->file, "r")) == NULL) {
		return NULL;
	}

	config.judgeuser[0] = '\0';
	config.judgeuid = -1;
	config.judgegroup[0] = '\0';
	config.judgegid = -1;
	config.judgedir[0] = '\0';
	config.judgecode[0] = '\0';
	config.judgename[0] = '\0';
	config.judgeaddr[0] = '\0';
	config.judgekeeper[0] = '\0';
	config.gateway[0] = '\0';
	config.sendmail[0] = '\0';
	config.pidfile[0] = '\0';
	config.unixsocket[0] = '\0';
	config.inetsocket[0] = '\0';
	config.inetport = 0;
	config.fifofile[0] = '\0';
	config.fifochilds = 3;
	config.loginput = 0;
	config.logoutput = 0;
	config.restart = 0;

	while (feof(fp) == 0) {
		fgets(buffer, 1024, fp );
		for ( i=0 ; buffer[i] != '#' && buffer[i] != '\0' && buffer[i] != '\n' ; i++ );
		buffer[i] = '\0';
		if ((ptr = strpbrk(buffer, "=")) == NULL) continue;
		sscanf(buffer, "%s", variable);
		strtolower(variable);
		for (i=1 ; ptr[i] == ' ' ; i++ );
		ptr+=i;
		for (i=0 ; ptr[i] != '\0' ; i++) buffer[i] = ptr[i];
		buffer[i] = '\0';
		for (i=(strlen(buffer)-1) ; buffer[i] == ' ' ; i--) buffer[i] = '\0';

		if (strncmp("judgeuser",   variable,  9) == 0) sprintf(config.judgeuser, "%s", buffer);
		if (strncmp("judgegroup",  variable, 10) == 0) sprintf(config.judgegroup, "%s", buffer);
		if (strncmp("judgedir",    variable,  8) == 0) sprintf(config.judgedir, "%s", buffer);
		if (strncmp("judgecode",   variable,  9) == 0) sprintf(config.judgecode, "%s", buffer);
		if (strncmp("judgename",   variable,  9) == 0) sprintf(config.judgename, "%s", buffer);
		if (strncmp("judgeaddr",   variable,  9) == 0) sprintf(config.judgeaddr, "%s", buffer);
		if (strncmp("judgekeeper", variable, 11) == 0) sprintf(config.judgekeeper, "%s", buffer);
		if (strncmp("gateway",     variable,  7) == 0) sprintf(config.gateway, "%s", buffer);
		if (strncmp("sendmail",     variable, 8) == 0) sprintf(config.sendmail, "%s", buffer);
		if (strncmp("pid-file",    variable,  8) == 0) sprintf(config.pidfile, "%s", buffer);
		if (strncmp("unix-socket", variable, 11) == 0) sprintf(config.unixsocket, "%s", buffer);
		if (strncmp("inet-socket", variable, 11) == 0) sprintf(config.inetsocket, "%s", buffer);
		if (strncmp("inet-port",   variable,  9) == 0) sscanf(buffer, "%d", &config.inetport);
		if (strncmp("fifo-file",   variable,  9) == 0) sprintf(config.fifofile, "%s", buffer);
		if (strncmp("fifo-childs", variable, 11) == 0) sscanf(buffer, "%d", &config.fifochilds);
		if (strncmp("log-input",   variable,  9) == 0) sscanf(buffer, "%d", &config.loginput);
		if (strncmp("log-output",  variable, 10) == 0) sscanf(buffer, "%d", &config.logoutput);
	}
	fclose(fp);

//	overwrite config with params

	if (params != NULL) {
		if (strlen(params->judgeuser) > 0)   sprintf(config.judgeuser,  "%s", params->judgeuser);
		if (strlen(params->judgegroup) > 0)  sprintf(config.judgegroup, "%s", params->judgegroup);
		if (strlen(params->judgedir) > 0)    sprintf(config.judgedir,   "%s", params->judgedir);
		if (strlen(params->judgecode) > 0)   sprintf(config.judgecode,  "%s", params->judgecode);
		if (strlen(params->judgename) > 0)   sprintf(config.judgename,  "%s", params->judgename);
		if (strlen(params->judgeaddr) > 0)   sprintf(config.judgeaddr,  "%s", params->judgeaddr);
		if (strlen(params->judgekeeper) > 0) sprintf(config.judgekeeper,"%s", params->judgekeeper);
		if (strlen(params->gateway) > 0)     sprintf(config.gateway,    "%s", params->gateway);
		if (strlen(params->sendmail) > 0)    sprintf(config.sendmail,   "%s", params->sendmail);
		if (strlen(params->pidfile) > 0)     sprintf(config.pidfile,    "%s", params->pidfile);
		if (strlen(params->unixsocket) > 0)  sprintf(config.unixsocket, "%s", params->unixsocket);
		if (strlen(params->inetsocket) > 0)  sprintf(config.inetsocket, "%s", params->inetsocket);
		if (params->inetport > 0)            config.inetport = params->inetport;
		if (strlen(params->fifofile) > 0)    sprintf(config.fifofile,   "%s", params->fifofile);
		if (params->fifochilds > 0)          config.fifochilds = params->fifochilds;
		if (params->loginput > -1)           config.loginput = params->loginput;
		if (params->logoutput > -1)          config.logoutput = params->logoutput;
	}

// find correct username and uid

	user_ptr = getpwnam(config.judgeuser);
	if(user_ptr == NULL) {
		i = -1;
		sscanf(config.judgeuser ,"%d", &i);
		user_ptr=getpwuid(i);
		if(user_ptr == NULL) {
			sprintf(string_out, "%s: user or UID '%s' not found. exit.\n", config.judgecode, config.judgeuser);
			output( LOG_ERR, string_out);
			return NULL;
		}
	}
	sscanf(user_ptr->pw_name, "%s", config.judgeuser);
	config.judgeuid = user_ptr->pw_uid;
	config.judgegid = user_ptr->pw_gid;

// find correct groupname and gid

	if (strlen(config.judgegroup) > 0) {
		group_ptr=getgrnam(config.judgegroup);
		if(group_ptr == NULL) {
			i = -1;
			sscanf(config.judgegroup ,"%d", &i);
			group_ptr=getgrgid(i);
		}
	}
	else {
		group_ptr=getgrgid(config.judgegid);
	}

	if(group_ptr == NULL) {
		sprintf(string_out, "%s: group or GID '%s' not found. exit.\n", config.judgecode, config.judgegroup);
		output( LOG_ERR, string_out);
		return NULL;
	}
	sscanf(group_ptr->gr_name, "%s", config.judgegroup);
	config.judgegid = group_ptr->gr_gid;

// find correct judge directory

	if (strlen(config.judgedir) == 0) {
		sscanf(user_ptr->pw_dir, "%s", config.judgedir);
	}

	else if (strncmp("/", config.judgedir,  1) != 0) {
		sprintf(buffer, "%s/%s/", user_ptr->pw_dir, config.judgedir);
		sscanf(buffer, "%s",config.judgedir);
	}

// check if we are runable

	if (config.judgeuid == 0) {
		
		sprintf( string_out, "%s: will not run as user '%s' (UID: %d). exit.\n", config.judgecode, config.judgeuser, config.judgeuid);
		output(LOG_ERR, string_out);
		return NULL;
	}

	if (config.judgegid == 0) {
		sprintf( string_out, "%s: will not run as group '%s' (GID: %d). exit.\n", config.judgecode, config.judgegroup, config.judgegid);
		output ( LOG_ERR, string_out);
		return NULL;
	}

	if (strlen(config.fifofile) == 0) config.fifochilds = 0;
	if (config.fifochilds == 0) config.fifofile[0] = '\0';
	if (strlen(config.inetsocket) == 0) config.inetport = 0;
	if (config.inetport == 0) config.inetsocket[0] = '\0';

	if (strlen(config.unixsocket) == 0 && strlen(config.inetsocket) == 0 && strlen(config.fifofile) == 0 ) {
		sprintf( string_out, "%s: have no canal to communicate. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}

// set enviroment

	if (config.judgeuid > 0) {
		ptr = getenv("JUDGE_UID");
		if (ptr != NULL) sscanf(ptr, "%d", &i);
		if (ptr == NULL || i != config.judgeuid) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set user to '%s' (%d)\n", config.judgecode, config.judgeuser, config.judgeuid);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change user to '%s' (%d)\n", config.judgecode, config.judgeuser, config.judgeuid);
				output(LOG_NOTICE, string_out);
			}
			sprintf(buffer,"%d",config.judgeuid);
			if( setenv("JUDGE_UID", buffer, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_UID\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
			config.restart |= ALL; 
		}
	} else if (config.judgeuid < 0) {
		sprintf( string_out, "%s: no user/UID found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (config.judgegid > 0) {
		ptr = getenv("JUDGE_GID");
		if (ptr != NULL) sscanf(ptr, "%d", &i);
		if (ptr == NULL || i != config.judgegid) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set group to '%s' (%d)\n", config.judgecode, config.judgegroup, config.judgegid);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change group to '%s' (%d)\n", config.judgecode, config.judgegroup, config.judgegid);
				output(LOG_NOTICE, string_out);
			}
			sprintf(buffer,"%d",config.judgegid);
			if( setenv("JUDGE_GID", buffer, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_GID\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
			config.restart |= ALL;
		}
	} else if (config.judgegid < 0) {
		sprintf( string_out, "%s: no group/GID found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (strlen(config.judgedir) > 0) {
		ptr = getenv("JUDGE_DIR");
		if (ptr != NULL) i = strcmp(ptr, config.judgedir);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set directory to '%s'\n", config.judgecode, config.judgedir);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change directory to '%s'\n", config.judgecode, config.judgedir);
				output(LOG_NOTICE, string_out);
			}
			if( setenv("JUDGE_DIR", config.judgedir, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_DIR\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
			if(chdir(config.judgedir) == -1) {
				sprintf( string_out, "%s: couldn't change to directory '%s'.\n", config.judgecode, config.judgedir);
				output(LOG_ERR, string_out);
				return NULL;
			}
		}
	}
	else {
		sprintf( string_out, "%s: no directory found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (strlen(config.judgecode) > 0) {
		ptr = getenv("JUDGE_CODE");
		if (ptr != NULL) i = strcmp(ptr, config.judgecode);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set judgecode to '%s'\n", config.judgecode, config.judgecode);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change judgecode to '%s'\n", config.judgecode, config.judgecode);
				output(LOG_NOTICE, string_out);
			}
			if( setenv("JUDGE_CODE", config.judgecode, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_CODE\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
			config.restart |= ALL;
		}
	}
	else {
		sprintf( string_out, "%s: no judgecode found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (strlen(config.judgename) > 0) {
		ptr = getenv("JUDGE_NAME");
		if (ptr != NULL) i = strcmp(ptr, config.judgename);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set judgename to '%s'\n", config.judgecode, config.judgename);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change judgename to '%s'\n", config.judgecode, config.judgename);
				output(LOG_NOTICE, string_out);
			}
			if( setenv("JUDGE_NAME", config.judgename, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_NAME\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
		}
	}
	else {
		sprintf( string_out, "%s: no judgename found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (strlen(config.judgeaddr) > 0) {
		ptr = getenv("JUDGE_ADDR");
		if (ptr != NULL) i = strcmp(ptr, config.judgeaddr);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set judgeaddr to '%s'\n", config.judgecode, config.judgeaddr);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change judgeaddr to '%s'\n", config.judgecode, config.judgeaddr);
				output(LOG_NOTICE, string_out);
			}
			if( setenv("JUDGE_ADDR", config.judgeaddr, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_ADDR\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
		}
	}
	else {
		sprintf( string_out, "%s: no judgeaddr found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (strlen(config.judgekeeper) > 0) {
		ptr = getenv("JUDGE_KEEPER");
		if (ptr != NULL) i = strcmp(ptr, config.judgekeeper);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set judgekeeper to '%s'\n", config.judgecode, config.judgekeeper);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change judgekeeper to '%s'\n", config.judgecode, config.judgekeeper);
				output(LOG_NOTICE, string_out);
			}
			if( setenv("JUDGE_KEEPER", config.judgekeeper, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_KEEPER\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
		}
	}
	else {
		sprintf( string_out, "%s: no address for judgekeeper found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (strlen(config.gateway) > 0) {
		ptr = getenv("JUDGE_GATEWAY");
		if (ptr != NULL) i = strcmp(ptr, config.gateway);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set gateway to '%s'\n", config.judgecode, config.gateway);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change gateway to '%s'\n", config.judgecode, config.gateway);
				output(LOG_NOTICE, string_out);
			}
			if (setenv("JUDGE_GATEWAY", config.gateway, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_GATEWAY\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
		}
	}
	else {
		sprintf( string_out, "%s: no address for gateway found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	if (strlen(config.sendmail) > 0) {
		ptr = getenv("JUDGE_SENDMAIL");
		if (ptr != NULL) i = strcmp(ptr, config.sendmail);
		if (ptr == NULL || i != 0) {
			if (ptr == NULL) {
				sprintf( string_out, "%s: set sendmail to '%s'\n", config.judgecode, config.sendmail);
				output(LOG_NOTICE, string_out);
			}
			else {
				sprintf( string_out, "%s: change sendmail to '%s'\n", config.judgecode, config.sendmail);
				output(LOG_NOTICE, string_out);
			}
			if (setenv("JUDGE_SENDMAIL", config.sendmail, 1) != 0 ) {
				sprintf( string_out, "%s: couldn't set JUDGE_SENDMAIL\n", config.judgecode);
				output(LOG_ERR, string_out);
				return NULL;
			}
		}
	}
	else {
		sprintf( string_out, "%s: no command for sendmail found in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}



	ptr = getenv("JUDGE_PIDFILE");
	if (ptr == NULL && strlen(config.pidfile) > 0) {
		sprintf( string_out, "%s: set pidfile to '%s'\n", config.judgecode, config.pidfile);
		output(LOG_NOTICE, string_out);
		if (setenv("JUDGE_PIDFILE", config.pidfile, 1) != 0 ) {
			sprintf( string_out, "%s: couldn't set JUDGE_PIDFILE\n", config.judgecode);
			output(LOG_ERR, string_out);
			return NULL;
		}
	}
	else if (ptr != NULL && strlen(config.pidfile) == 0) {
		sscanf(ptr,"%s",config.pidfile);
		sprintf( string_out, "%s: can't run without pidfile! leaf it at '%s'\n", config.judgecode, config.pidfile);
		output(LOG_NOTICE, string_out);
	}
	else if (ptr != NULL && strlen(config.pidfile) > 0) {
		i = strcmp(ptr, config.pidfile);
		if (i != 0) {
			sscanf(ptr,"%s",config.pidfile);
			sprintf( string_out, "%s: pidfile can't changed! leaf it at '%s'\n", config.judgecode, config.pidfile);
			output(LOG_NOTICE, string_out);
		}
	}
	else {
		sprintf( string_out, "%s: no pidfile defined in configfile. exit.\n", config.judgecode);
		output(LOG_ERR, string_out);
		return NULL;
	}

	sprintf( string_out, "%s: restart at fifo = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);





	ptr = getenv("JUDGE_FIFOCHILDS");
	if (ptr != NULL) sscanf(ptr,"%d",&j);
	else j = 0;
	if (config.fifochilds != j) {
		sprintf(buffer,"%d",config.fifochilds);
		if( setenv("JUDGE_FIFOCHILDS", buffer, 1) != 0 ) {
			sprintf( string_out, "%s: couldn't set JUDGE_FIFOCHILDS\n", config.judgecode);
			output(LOG_ERR, string_out);
			return NULL;
		}
		config.restart |= PIPE;
	}


	ptr = getenv("JUDGE_FIFO");
	if (ptr == NULL && strlen(config.fifofile) > 0) {
		sprintf( string_out, "%s: set fifofile to '%s'\n", config.judgecode, config.fifofile);
		output(LOG_NOTICE, string_out);
		config.restart |= PIPE;
	}
	else if (ptr != NULL && strlen(config.fifofile) == 0) {
		sprintf( string_out, "%s: unset fifofile.\n", config.judgecode);
		output(LOG_NOTICE, string_out);
		config.restart |= PIPE;
	}
	else if (ptr != NULL && strlen(config.fifofile) > 0) {
		i = strcmp(ptr, config.fifofile);
		if (i != 0) {
			sprintf( string_out, "%s: change fifofile to '%s'\n", config.judgecode, config.fifofile);
			output(LOG_NOTICE, string_out);
			config.restart |= PIPE;
		}
		else {
			if (config.restart & ALL) config.restart |= PIPE;
		}
	}

	sprintf( string_out, "%s: restart at unix = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);

	ptr = getenv("JUDGE_UNIX");
	if (ptr == NULL && strlen(config.unixsocket) > 0) {
		sprintf( string_out, "%s: set unixsocket to '%s'\n", config.judgecode, config.unixsocket);
		output(LOG_NOTICE, string_out);
		config.restart |= UNIX;
	}
	else if (ptr != NULL && strlen(config.unixsocket) == 0) {
		sprintf( string_out, "%s: unset unixsocket.\n", config.judgecode);
		output(LOG_NOTICE, string_out);
		config.restart |= UNIX;
	}
	else if (ptr != NULL && strlen(config.unixsocket) > 0) {
		i = strcmp(ptr, config.unixsocket);
		if (i != 0) {
			sprintf( string_out, "%s: change unixsocket to '%s'\n", config.judgecode, config.unixsocket);
			output(LOG_NOTICE, string_out);
			config.restart |= UNIX;
		}
		else {
			if (config.restart & ALL) config.restart |= UNIX;
		}
	}

	sprintf( string_out, "%s: restart at inet (token) = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);

	i=0;
	strcpy(buffer, config.inetsocket);
	ptr = strtok(buffer, ", ");
	while(ptr != NULL) {
		sscanf(ptr,"%s",&variable[i*128]);
		ptr = strtok(NULL, ", ");
		i++;
	}
	for (; i < 5 ; i++) variable[i*128] = '\0';

	sprintf( string_out, "%s: restart at inet (old) = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);

	for (i = 0 ; i < 5 ; i++) {
		sprintf(buffer, "JUDGE_INET%d", i);
		if (getenv(buffer) != NULL) {
			for (j = 0 ; j < 5 && strcmp(&variable[j*128], getenv(buffer)) ; j++);
			if (j == 5) {
				unsetenv(buffer);
				config.restart |= (INET << i);
			}
			else variable[j*128] = '\0';
		}
	}

	sprintf( string_out, "%s: restart at inet (new) = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);

	for (i = 0 ; i < 5 ; i++) {
		if (strlen(&variable[i*128]) > 0) {
			for (j=0 ; j < 5 ; j++) {
				sprintf(buffer, "JUDGE_INET%d", j);
				if (getenv(buffer) == NULL) {
					sprintf( string_out, "%s: set %s to '%s' (%d)\n", config.judgecode, buffer, &variable[i*128], (int)strlen(&variable[i*128]));
					output(LOG_NOTICE, string_out);
					if( setenv(buffer, &variable[i*128], 1) != 0 ) {
						sprintf( string_out, "%s: couldn't set %s\n", config.judgecode, buffer);
						output(LOG_ERR, string_out);
						return NULL;
					}
					config.restart |= (INET << j);
					break;
				}
			}
		}
	}

	sprintf( string_out, "%s: restart at port = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);

	ptr = getenv("JUDGE_INETPORT");
	if (ptr != NULL) sscanf(ptr,"%d",&j);
	else j = 0;
	if (config.inetport != j) {
		sprintf(buffer,"%d",config.inetport);
		if( setenv("JUDGE_INETPORT", buffer, 1) != 0 ) {
			sprintf( string_out, "%s: couldn't set JUDGE_INETPORT\n", config.judgecode);
			output(LOG_ERR, string_out);
			return NULL;
		}
		for (i = 0 ; i < 5 ; i++) {
			sprintf(buffer, "JUDGE_INET%d", i);
			if (getenv(buffer)) config.restart |= (INET << i);
		}
	}

	sprintf( string_out, "%s: restart at end1 = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);
	config.restart &= 127;
	sprintf( string_out, "%s: restart at end2 = %d\n", config.judgecode, config.restart);
	output(LOG_NOTICE, string_out);
	sprintf(config.file, "%s", config_old->file);
	memcpy(config_old, &config, sizeof(struct Config));
	return config_old;
}
