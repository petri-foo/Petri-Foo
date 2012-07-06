file (GLOB PETRI_FOO_SOURCES *.c)

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

if (HAVE_LIBLO)
    link_libraries(lo)
endif (HAVE_LIBLO)

add_definitions (
    ${GTK2_CFLAGS_OTHER}
    )

add_executable(petri-foo ${PETRI_FOO_SOURCES})


target_link_Libraries(petri-foo petrifoo petrifui pthread phin )

install (TARGETS petri-foo DESTINATION ${BINDIR})
