/***************************************************************************
 *            command_broadcast.h
 *
 *  Fre Jänner 24 08:09:40 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_broadcast(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
