/***************************************************************************
 *            command_whogame.h
 *
 *  Fre Jänner 24 08:39:34 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_whogame(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
