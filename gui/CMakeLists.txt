file (GLOB PETRI_FOO_SOURCES *.c)

include_directories (
    ${Petri-Foo_SOURCE_DIR}/libpetrifoo
    ${Petri-Foo_SOURCE_DIR}/libpetrifui
    ${Petri-Foo_SOURCE_DIR}/libphin
    ${GTK2_INCLUDE_DIRS}
	${GOOCANVAS_INCLUDE_DIRS}
    ${LIBXML2_INCLUDE_DIR}
    )

add_executable(petri-foo ${PETRI_FOO_SOURCES})

target_link_Libraries(petri-foo petrifoo petrifui pthread phin ${LIBLO_LIBRARIES})

install (TARGETS petri-foo DESTINATION ${BINDIR})
