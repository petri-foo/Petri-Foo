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

add_library( phin STATIC ${LIBPHIN_SOURCES})
SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")
install(TARGETS phin DESTINATION ${CMAKE_INSTALL_LIBDIR})

target_link_Libraries(phin ${GTK2_LIBRARIES} ${LIBGNOMECANVAS2_LIBRARIES})
