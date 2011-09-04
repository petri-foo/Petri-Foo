
file (GLOB LIBPETRIFUI_SOURCES *.c)

include_directories (
    ${Petri-Foo_SOURCE_DIR}/libpetrifoo
    ${LIBXML2_INCLUDE_DIRS}
    )

link_directories (
    ${LIBXML2_LIBRARY_DIRS}
    )

add_definitions (
    ${LIBXML2_CFLAGS_OTHER}
    )

add_library( petrifui ${LIBPETRIFUI_SOURCES})

target_link_Libraries(  petrifui
                        petrifoo
                        ${LIBXML2_LIBRARIES}
                    )
