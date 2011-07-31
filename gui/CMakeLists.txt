include_directories(${Petri-Foo_SOURCE_DIR}/libpetrifoo)
include_directories(${Petri-Foo_SOURCE_DIR}/libphin)

file (GLOB PETRI_FOO_SOURCES *.c)

include_directories (
    ${GTK2_INCLUDE_DIRS}
    ${LIBGNOMECANVAS2_INCLUDE_DIRS}
    )

#link_directories (
#    ${GTK2_LIBRARY_DIRS}
#    )

add_definitions (
    ${GTK2_CFLAGS_OTHER}
    )

add_executable(petri-foo ${PETRI_FOO_SOURCES})


# DEBUG
set (BuildOptionsDebug
    "-O0 -ggdb -Wall -Wextra -DGTK_DISABLE_DEPRECATED -DGDK_DISABLE_DEPRECATED -DGSEAL_ENABLE"
        CACHE STRING "Debug build flags")


target_link_Libraries(petri-foo phin petrifoo)
