/***************************************************************************
 *            command_register.h
 *
 *  Fre Jänner 24 08:19:10 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_register(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
