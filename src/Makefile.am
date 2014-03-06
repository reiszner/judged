## Process this file with automake to produce Makefile.in

## Created by Anjuta

AM_CPPFLAGS = \
	-DPACKAGE_LOCALE_DIR=\""$(localedir)"\" \
	-DPACKAGE_SRC_DIR=\""$(srcdir)"\" \
	-DPACKAGE_DATA_DIR=\""$(pkgdatadir)"\" \
	$(GLIB_CFLAGS) \
	$(gmime-2.6_CFLAGS) \
	$(GMIME_CFLAGS)

AM_CFLAGS =\
	 -Wall\
	 -g

bin_PROGRAMS = judged \
	judge-fifo \
	judge-sock \
	judge-deliver

judge_fifo_SOURCES =  \
	judge-fifo.c \
	fifo.c \
	fifo.h \
	child.c \
	child.h \
	ipc.c \
	ipc.h \
	misc.c \
	misc.h

judged_SOURCES =  \
	config.c \
	config.h \
	ipc.c \
	ipc.h \
	misc.c \
	misc.h \
	judged.c \
	incoming.c \
	incoming.h \
	whois.c \
	whois.h \
	string_wcs.c \
	string_wcs.h \
	message.c \
	message.h \
	mail_processing.c \
	mail_processing.h \
	command_become.c \
	command_become.h \
	command_list.c \
	command_list.h \
	command_press.c \
	command_press.h \
	command_broadcast.c \
	command_broadcast.h \
	command_register.c \
	command_register.h \
	command_send.c \
	command_send.h \
	command_whogame.c \
	command_whogame.h \
	command_generic.c \
	command_generic.h \
	command_set.c \
	command_set.h \
	master.c \
	master.h \
	fifo.c \
	fifo.h

judged_LDADD = $(GLIB_LIBS) \
	$(gmime-2.6_LIBS) \
	$(GMIME_LIBS) 

judge_sock_SOURCES =  \
	ipc.c \
	ipc.h \
	judge-sock.c \
	misc.c \
	misc.h \
	socket.c \
	socket.h

judge_deliver_SOURCES =  \
	judge-deliver.c

