noinst_LIBRARIES = libgui.a

libgui_a_SOURCES = 					\
	audio-settings.c	audio-settings.h	\
	bank-ops.c		bank-ops.h		\
	basic_combos.c		basic_combos.h		\
	channelsection.c	channelsection.h	\
	envelopetab.c		envelopetab.h		\
	gui.c			gui.h			\
	idselector.c		idselector.h		\
	lfotab.c		lfotab.h		\
	mastersection.c		mastersection.h		\
	midisection.c		midisection.h		\
	mod_section.c		mod_section.h		\
	mod_src_gui.c		mod_src_gui.h		\
	paramtab.c		paramtab.h		\
	patchlist.c		patchlist.h		\
	patchsection.c		patchsection.h		\
	sample-editor.c		sample-editor.h		\
	sample-selector.c	sample-selector.h	\
	sampletab.c		sampletab.h		\
	voicetab.c		voicetab.h		\
	waveform.c		waveform.h

INCLUDES = 			\
	-l.. 			\
	@GTK_CFLAGS@ 		\
	@LIBGNOMECANVAS_CFLAGS@ \
	@PHAT_CFLAGS@

libgui_a_CFLAGS = $(CFLAGS) -I.. -DINSTALLDIR=\"$(datadir)\"
