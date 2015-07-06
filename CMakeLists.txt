cmake_minimum_required(VERSION 2.6)

if (COMMAND CMAKE_POLICY)
  cmake_policy (SET CMP0011 NEW)
endif (COMMAND CMAKE_POLICY)

project(Petri-Foo)

set(Petri-Foo_VERSION_MAJOR 1)
set(Petri-Foo_VERSION_MINOR 87)
set(BINDIR  "bin" CACHE STRING "Where to install binaries")
set(DATADIR "share/petri-foo" CACHE STRING "Where to install data files")
set(APPLICATIONS_DIR "share/applications" CACHE STRING "Where to install desktop files")
set(PIXMAPS_DIR "share/pixmaps" CACHE STRING "Where to install icons")
set(MIME_DIR "share/mime" CACHE STRING "Where MIME definitions are located")
set(UpdateMime "ON" CACHE BOOL "Update Mime Database")

set(ARCHIVE_NAME petri-foo-0.${Petri-Foo_VERSION_MAJOR}.${Petri-Foo_VERSION_MINOR})

# two #defines to aid stripping path info from debug message output:
# see config.h.in
set(SRC_DIR $PROJECT_SOURCE_DIR)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH
  ${CMAKE_SOURCE_DIR}/cmake/Modules
)

string( LENGTH "${PROJECT_SOURCE_DIR}" SRC_DIR_STRLEN)
include (CheckCSourceCompiles)
INCLUDE (CheckIncludeFiles)

mark_as_advanced(EXECUTABLE_OUTPUT_PATH)
mark_as_advanced(LIBRARY_OUTPUT_PATH)
mark_as_advanced(CMAKE_BUILD_TYPE)
mark_as_advanced(CMAKE_INSTALL_PREFIX)

option (BuildForDebug "Include gdb debugging support" OFF)
option (GtkDeprecatedChecks "Check against use of API deprecated in GTK3" OFF)

if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sandbox )
    option (BuildSandbox "Build sandbox test code " OFF)
endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sandbox )
 

# DEBUG
set (BuildOptionsDebug "-O0 -ggdb" CACHE STRING "Debug build flags")

# RELEASE
set (BuildOptionsRelease "-O2" CACHE STRING "Release build flags")

# additional compiler flags
add_definitions(-Wall -Wextra)

if (BuildForDebug)
    set (CMAKE_BUILD_TYPE "Debug")
    set (DEBUG 1)
    set (CMAKE_C_FLAGS_DEBUG ${BuildOptionsDebug})
    message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_C_FLAGS_DEBUG}")
else (BuildForDebug)
    set (CMAKE_BUILD_TYPE "Release")
    set (DEBUG 0)
    set (CMAKE_C_FLAGS_RELEASE ${BuildOptionsRelease})
    message (STATUS "Building for ${CMAKE_BUILD_TYPE}, flags: ${CMAKE_C_FLAGS_RELEASE}")
endif (BuildForDebug)

if (GtkDeprecatedChecks)
    add_definitions(
        -DGDK_PIXBUF_DISABLE_DEPRECATED
        -DGTK_DISABLE_DEPRECATED
        -DGDK_DISABLE_DEPRECATED
        -DGSEAL_ENABLE
        -DGTK_DISABLE_SINGLE_INCLUDES)
    message (STATUS "Building with GTK_DISABLE_DEPRECATED...")
endif (GtkDeprecatedChecks)


include(FindPkgConfig)


# JACK
pkg_check_modules(JACK REQUIRED jack>=0.120.0)
if (JACK_FOUND)
  message(STATUS "Found jack ${JACK_VERSION}")
else (JACK_FOUND)
  message(FATAL_ERROR "Jack >= 0.120.0 not found.")
endif (JACK_FOUND)


include (FindALSA)

# SNDFILE
pkg_check_modules (SNDFILE REQUIRED sndfile)
if (SNDFILE_FOUND)
    message(STATUS "Found sndfile ${SNDFILE_VERSION}")
else (SNDFILE_FOUND)
    message(FATAL_ERROR "sndfile not found")
endif (SNDFILE_FOUND)


# SAMPLERATE
pkg_check_modules (SAMPLERATE REQUIRED samplerate)
if (SAMPLERATE_FOUND)
    message(STATUS "Found secret rabbit code ${SAMPLERATE_VERSION}")
else (SAMPLERATE_FOUND)
    message(FATAL_ERROR "secret rabbit code not found")
endif (SAMPLERATE_FOUND)


# GLIB2
pkg_check_modules (GLIB2 REQUIRED glib-2.0>=2.16)
if (GLIB2_FOUND)
  message(STATUS "Found glib2 ${GLIB2_VERSION}")
else (GLIB2_FOUND)
  message(FATAL_ERROR "glib2 was not found.")
endif (GLIB2_FOUND)


include(FindGTK2)


# LIBGNOMECANVAS2
pkg_check_modules (LIBGNOMECANVAS2 REQUIRED libgnomecanvas-2.0)
if (LIBGNOMECANVAS2_FOUND)
  message(STATUS "Found libgnomecanvas2 ${LIBGNOMECANVAS2_VERSION}")
else (LIBGNOMECANVAS2_FOUND)
  message(FATAL_ERROR "libgnomecanvas2 was not found.")
endif (LIBGNOMECANVAS2_FOUND)


include(FindLibXml2)


# LIBLO
pkg_check_modules (LIBLO liblo>=0.26)
if (LIBLO_FOUND)
    message(STATUS "Found liblo ${LIBLO_VERSION}, support for NSM will be built")
    set (HAVE_LIBLO 1)
else (LIBLO_FOUND)
    message(STATUS "liblo was not found, support for NSM will not be built")
    set (HAVE_LIBLO 0)
endif (LIBLO_FOUND)


# Check for CAIRO_OPERATOR_HSL_LUMINOSITY
check_c_source_compiles (
    "#include <cairo/cairo.h>
    int main(int argc, char **argv)
    {
        cairo_operator_t op = CAIRO_OPERATOR_HSL_LUMINOSITY;
        return 0;
    }" HAVE_CAIRO_OPERATOR_HSL
)

CHECK_INCLUDE_FILES (malloc.h HAVE_MALLOC_H)
CHECK_INCLUDE_FILES (jack/session.h HAVE_JACK_SESSION_H)

find_package(GCrypt 1.5.0 REQUIRED)
CONFIGURE_FILE( ${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
                ${CMAKE_CURRENT_SOURCE_DIR}/config.h )

link_libraries(m ${LIBLO_LIBRARIES})


include_directories ( . )

# Install targets

ADD_SUBDIRECTORY( libpetrifoo )
ADD_SUBDIRECTORY( libpetrifui )
ADD_SUBDIRECTORY( libphin )
ADD_SUBDIRECTORY( gui )


if (BuildSandbox)
    if (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sandbox )
        message( STATUS "Will build sandbox test target" )
        ADD_SUBDIRECTORY( sandbox )
    endif (IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/sandbox )
endif (BuildSandbox)

#ADD_SUBDIRECTORY( pixmaps )

install (DIRECTORY pixmaps/ DESTINATION ${DATADIR}/pixmaps
          FILES_MATCHING PATTERN "*.png")
install (FILES petri-foo.xml DESTINATION ${MIME_DIR}/packages)
install (FILES pixmaps/petri-foo.png DESTINATION ${PIXMAPS_DIR})
install (FILES petri-foo.desktop DESTINATION ${APPLICATIONS_DIR})

if (UpdateMime)
    install (CODE
        "EXEC_PROGRAM ( \"update-mime-database ${CMAKE_INSTALL_PREFIX}/${MIME_DIR}\" ) "
    )
endif (UpdateMime)

add_custom_target(dist
    COMMAND git archive --prefix=${ARCHIVE_NAME}/ HEAD
        | bzip2 > ${CMAKE_BINARY_DIR}/${ARCHIVE_NAME}.tar.bz2
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

