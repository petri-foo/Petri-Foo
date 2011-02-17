SUBDIRS = gui

bin_PROGRAMS = petri-foo

petri_foo_SOURCES = alsadriver.c jackdriver.c mixer.c mixer.h sample.c	\
sample.h driver.c driver.h midi.c midi.h patch.c patch.h petri-foo.c	\
petri-foo.h beef.c beef.h maths.c maths.h ticks.c ticks.h adsr.c adsr.h	\
lfo.c lfo.h sync.c sync.h control.h instance.c instance.h \
patch_util.c patch_util.h
                                                                                                                                                                        
if WITH_LASH                                                                                                                                                            
petri_foo_SOURCES += lashdriver.c lashdriver.h                                                                                                                           
endif

INCLUDES = \
	@ALSA_CFLAGS@ \
	@JACK_CFLAGS@ \
	@LIBSAMPLERATE_CFLAGS@ \
	@LIBSNDFILE_CFLAGS@ \
	@LIBXML_CFLAGS@ \
	@LASH_CFLAGS@ \
	@GTK_CFLAGS@


petri_foo_LDADD = \
	gui/libgui.a \
	@PHAT_LIBS@ \
	@ALSA_LIBS@ \
	@JACK_LIBS@ \
	@LIBSAMPLERATE_LIBS@ \
	@LIBSNDFILE_LIBS@ \
	@LIBXML_LIBS@ \
	@LASH_LIBS@ \
	@GTK_LIBS@ \
	@LIBGNOMECANVAS_LIBS@
	



EXTRA_DIST = gprof-helper.c
