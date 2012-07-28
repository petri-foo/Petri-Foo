file (GLOB LIBPHIN_SOURCES *.c)

include_directories (
    ${GTK2_INCLUDE_DIRS}
    ${LIBGNOMECANVAS2_INCLUDE_DIRS}
    )


add_library( phin STATIC ${LIBPHIN_SOURCES})

SET(CMAKE_INSTALL_LIBDIR lib CACHE PATH "Output directory for libraries")

target_link_Libraries(phin ${GTK2_LIBRARIES} ${LIBGNOMECANVAS2_LIBRARIES})
