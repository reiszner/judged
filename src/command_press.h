/***************************************************************************
 *            comand_press.h
 *
 *  Fre Jänner 24 07:47:23 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>
#include "master.h"
#include "whois.h"

void com_press(struct message_proc_t *, wchar_t [][MSGLEN], Game *, Whois *, struct buffer_t *);
