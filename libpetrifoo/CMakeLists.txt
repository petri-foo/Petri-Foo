include_directories ( . .. )

ADD_SUBDIRECTORY( patch_private )

file (GLOB LIBPETRIFOO_SOURCES *.c)

include_directories(
    ${ALSA_INCLUDE_DIRS}
    )

add_library( petrifoo ${LIBPETRIFOO_SOURCES})

target_link_Libraries(  petrifoo patch_private
                        ${JACK_LIBRARIES}
                        ${SNDFILE_LIBRARIES}
                        ${ALSA_LIBRARIES}
                        ${SAMPLERATE_LIBRARIES}
                    )

