noinst_LIBRARIES = libgui.a




libgui_a_SOURCES = audio-settings.c audio-settings.h sample-editor.c	\
sample-editor.h gui.c gui.h waveform.c waveform.h bank-ops.c		\
bank-ops.h sample-selector.c sample-selector.h mastersection.c		\
mastersection.h patchsection.c patchsection.h sampletab.c sampletab.h	\
voicetab.c voicetab.h filtertab.c filtertab.h velocitytab.c		\
velocitytab.h envelopetab.c envelopetab.h lfotab.c lfotab.h		\
pitchtab.c pitchtab.h mod_src_opt.c mod_src_opt.h \
idselector.c idselector.h patchlist.c patchlist.h midisection.c	\
midisection.h channelsection.c channelsection.h

INCLUDES = \
	-l.. \
	@GTK_CFLAGS@ \
	@LIBGNOMECANVAS_CFLAGS@ \
	@PHAT_CFLAGS@

if HAVE_LASH
INCLUDES += @LASH_CFLAGS@
endif

libgui_a_CFLAGS = $(CFLAGS) -I.. -DINSTALLDIR=\"$(datadir)\"
