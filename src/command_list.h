/***************************************************************************
 *            command_list.h
 *
 *  Die Jänner 21 03:11:04 2014
 *  Copyright  2014  
 *  <reiszner@novaplan.at>
 ****************************************************************************/

#include <wchar.h>

#define LIST_FULL 0
#define ALL__LIST 1

void com_list(struct message_proc_t *, wchar_t *, struct buffer_t *);
