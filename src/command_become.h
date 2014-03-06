/***************************************************************************
 *            command_become.h
 *
 *  Mon Jänner 20 13:28:41 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_become(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
