# Exports:
#
# Variables:
#  LIBUNICORN_FOUND
#  LIBUNICORN_INCLUDE_DIR
#  LIBUNICORN_LIBRARY
#
# Target:
#  Unicorn::Unicorn
#

find_path(LIBUNICORN_INCLUDE_DIR
          unicorn/unicorn.h
          HINTS $ENV{UNICORNDIR}
          PATH_SUFFIXES include)

find_library(LIBUNICORN_LIBRARY
             NAMES unicorn
             HINTS $ENV{UNICORNDIR})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Unicorn DEFAULT_MSG LIBUNICORN_LIBRARY LIBUNICORN_INCLUDE_DIR)

if (UNICORN_FOUND)
    set(THREADS_PREFER_PTHREAD_FLAG ON)
    find_package(Threads REQUIRED)
    unset(THREADS_PREFER_PTHREAD_FLAG)

    add_library(Unicorn::Unicorn UNKNOWN IMPORTED)
    set_target_properties(Unicorn::Unicorn PROPERTIES
        IMPORTED_LOCATION ${LIBUNICORN_LIBRARY}
        INTERFACE_INCLUDE_DIRECTORIES ${LIBUNICORN_INCLUDE_DIR}
        INTERFACE_LINK_LIBRARIES Threads::Threads
    )
endif()

mark_as_advanced(LIBUNICORN_INCLUDE_DIR LIBUNICORN_LIBRARY)
