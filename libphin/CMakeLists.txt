file (GLOB LIBPHIN_SOURCES *.c)

include_directories (
    ${GTK2_INCLUDE_DIRS}
    ${LIBGNOMECANVAS2_INCLUDE_DIRS}
    )

link_directories (
    ${GTK2_LIBRARY_DIRS}
    ${LIBGNOMECANVAS2_LIBRARY_DIRS}
    )

add_definitions (
    ${GTK2_CFLAGS_OTHER}
    ${LIBGNOMECANVAS2_CFLAGS_OTHER}
    )

add_library( phin ${LIBPHIN_SOURCES})

target_link_Libraries(phin ${GTK2_LIBRARIES} ${LIBGNOMECANVAS2_LIBRARIES})
