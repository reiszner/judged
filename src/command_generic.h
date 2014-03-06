/***************************************************************************
 *            command_generic.h
 *
 *  Fre Jänner 24 08:44:20 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_generic(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
