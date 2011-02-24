SUBDIRS = gui

bin_PROGRAMS = petri-foo

petri_foo_SOURCES = 					\
	adsr.c			adsr.h			\
	beef.c			beef.h			\
				config.h		\
				control.h		\
	driver.c		driver.h		\
	instance.c		instance.h		\
	jackdriver.c		jackdriver.h		\
	lfo.c			lfo.h			\
	maths.c			maths.h			\
	midi.c			midi.h			\
	mixer.c			mixer.h			\
	patch.c			patch.h			\
	patch_set_and_get.c	patch_set_and_get.h	\
	patch_util.c		patch_util.h		\
	petri-foo.c		petri-foo.h		\
	sample.c		sample.h		\
	sync.c			sync.h			\
	ticks.c			ticks.h			\
	patch_private/patch_defs.c

                                                                                                                                                                        
if WITH_LASH                                                                                                                                                            
petri_foo_SOURCES += 					\
	lashdriver.c 		lashdriver.h                                                                                                                           
endif

INCLUDES = 			\
	@ALSA_CFLAGS@ 		\
	@JACK_CFLAGS@ 		\
	@LIBSAMPLERATE_CFLAGS@ 	\
	@LIBSNDFILE_CFLAGS@ 	\
	@LIBXML_CFLAGS@ 	\
	@LASH_CFLAGS@ 		\
	@GTK_CFLAGS@

petri_foo_LDADD = 		\
	gui/libgui.a 		\
	@PHAT_LIBS@ 		\
	@ALSA_LIBS@ 		\
	@JACK_LIBS@ 		\
	@LIBSAMPLERATE_LIBS@ 	\
	@LIBSNDFILE_LIBS@ 	\
	@LIBXML_LIBS@ 		\
	@LASH_LIBS@ 		\
	@GTK_LIBS@ 		\
	@LIBGNOMECANVAS_LIBS@
	
EXTRA_DIST = gprof-helper.c
