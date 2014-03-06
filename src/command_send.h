/***************************************************************************
 *            command_send.h
 *
 *  Fre Jänner 24 08:26:30 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_send(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
