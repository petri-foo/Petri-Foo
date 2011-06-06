SUBDIRS = gui patch_private

bin_PROGRAMS = petri-foo

petri_foo_SOURCES = 					\
	adsr.c			adsr.h			\
				config.h		\
				control.h		\
	dish_file.c		dish_file.h		\
	driver.c		driver.h		\
	instance.c		instance.h		\
	jackdriver.c		jackdriver.h		\
	lfo.c			lfo.h			\
	maths.c			maths.h			\
	midi.c			midi.h			\
	mixer.c			mixer.h			\
	mod_src.c		mod_src.h		\
	names.c			names.h			\
	patch.c			patch.h			\
	patch_set_and_get.c	patch_set_and_get.h	\
	patch_util.c		patch_util.h		\
	petri-foo.c		petri-foo.h		\
	sample.c		sample.h		\
	sync.c			sync.h			\
	ticks.c			ticks.h
                                                                                                                                                                        
INCLUDES = 			\
	@ALSA_CFLAGS@ 		\
	@JACK_CFLAGS@ 		\
	@LIBSAMPLERATE_CFLAGS@ 	\
	@LIBSNDFILE_CFLAGS@ 	\
	@LIBXML_CFLAGS@ 	\
	@GTK_CFLAGS@

petri_foo_LDADD = 		\
	patch_private/libpatch.a\
	gui/libgui.a 		\
	@PHAT_LIBS@ 		\
	@ALSA_LIBS@ 		\
	@JACK_LIBS@ 		\
	@LIBSAMPLERATE_LIBS@ 	\
	@LIBSNDFILE_LIBS@ 	\
	@LIBXML_LIBS@ 		\
	@GTK_LIBS@ 		\
	@LIBGNOMECANVAS_LIBS@
	
EXTRA_DIST = gprof-helper.c
