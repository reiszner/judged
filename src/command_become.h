/***************************************************************************
 *            command_become.h
 *
 *  Mon Jänner 20 13:28:41 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>

#define BECOME_MASTER 0
#define ALL__BECOME 1

void com_become(struct message_proc_t *, wchar_t *, struct buffer_t *);
