file (GLOB TEST_SOURCES test.c)

include_directories (
    ${Petri-Foo_SOURCE_DIR}/libpetrifoo
    ${Petri-Foo_SOURCE_DIR}/libpetrifui
    ${Petri-Foo_SOURCE_DIR}/libphin
    ${GTK2_INCLUDE_DIRS}
    ${LIBGNOMECANVAS2_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIRS}
    )

link_directories (
    ${GTK2_LIBRARY_DIRS}
    )

add_definitions (
    ${GTK2_CFLAGS_OTHER}
    )

add_executable(test ${TEST_SOURCES})

target_link_Libraries( test petrifoo petrifui pthread phin )

