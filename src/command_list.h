/***************************************************************************
 *            command_list.h
 *
 *  Die Jänner 21 03:11:04 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_list(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
