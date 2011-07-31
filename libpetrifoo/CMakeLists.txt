include_directories ( . )

ADD_SUBDIRECTORY( patch_private )

file (GLOB LIBPETRIFOO_SOURCES *.c)

include_directories (
    ${LIBXML2_INCLUDE_DIRS}
    )

link_directories (
    ${LIBXML2_LIBRARY_DIRS}
    )

add_definitions (
    ${LIBXML2_CFLAGS_OTHER}
    )

add_library( petrifoo ${LIBPETRIFOO_SOURCES})

target_link_Libraries(  petrifoo patch_private
                        ${LIBXML2_LIBRARIES}
                        ${JACK_LIBRARIES}
                        ${SNDFILE_LIBRARIES}
                        ${ALSA_LIBRARIES}
                        ${SAMPLERATE_LIBRARIES}
                    )

