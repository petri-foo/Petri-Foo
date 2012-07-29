
file (GLOB LIBPETRIFUI_SOURCES *.c)

include_directories (
    ${Petri-Foo_SOURCE_DIR}/libpetrifoo
    ${LIBXML2_INCLUDE_DIR}
    ${OPENSSL_INCLUDE_DIR}
    )


add_definitions (
    ${LIBXML2_DEFINITIONS}
    )

add_library( petrifui ${LIBPETRIFUI_SOURCES})

target_link_Libraries(  petrifui
                        petrifoo
                        ${LIBXML2_LIBRARIES}
                        ${OPENSSL_LIBRARIES}
                    )
