# TODO:
# create usable libtool .la file

CMAKE_MINIMUM_REQUIRED(VERSION 3.0.2)

LIST(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/libzip")

PROJECT(libzip C)

OPTION(ENABLE_COMMONCRYPTO "Enable use of CommonCrypto" ON)
OPTION(ENABLE_GNUTLS "Enable use of GnuTLS" ON)
OPTION(ENABLE_MBEDTLS "Enable use of mbed TLS" ON)
OPTION(ENABLE_OPENSSL "Enable use of OpenSSL" ON)
OPTION(ENABLE_WINDOWS_CRYPTO "Enable use of Windows cryptography libraries" ON)

OPTION(ENABLE_BZIP2 "Enable use of BZip2" OFF)
OPTION(ENABLE_LZMA "Enable use of LZMA" OFF)

INCLUDE(CheckFunctionExists)
INCLUDE(CheckIncludeFiles)
INCLUDE(CheckSymbolExists)
INCLUDE(CheckTypeSize)
INCLUDE(CheckCSourceRuns)
INCLUDE(CheckCSourceCompiles)
INCLUDE(CheckStructHasMember)
INCLUDE(TestBigEndian)
INCLUDE(GNUInstallDirs)
IF(ENABLE_COMMONCRYPTO)
  CHECK_INCLUDE_FILES(CommonCrypto/CommonCrypto.h COMMONCRYPTO_FOUND)
ELSE()
  SET(COMMONCRYPTO_FOUND FALSE)
ENDIF()
IF(ENABLE_GNUTLS)
  INCLUDE(FindNettle)
  INCLUDE(FindGnuTLS)
ELSE()
  SET(GNUTLS_FOUND FALSE)
ENDIF()
IF(ENABLE_MBEDTLS)
  FIND_PATH(MBEDTLS_INCLUDE_DIR mbedtls/aes.h)
  FIND_LIBRARY(MBEDTLS_LIBRARIES NAMES mbedcrypto)
ELSE()
  SET(MBEDTLS_LIBRARIES FALSE)
ENDIF()
IF(ENABLE_OPENSSL)
  INCLUDE(FindOpenSSL)
ELSE()
  SET(OPENSSL_FOUND FALSE)
ENDIF()
IF(WIN32)
  IF(ENABLE_WINDOWS_CRYPTO)
    SET(WINDOWS_CRYPTO_FOUND TRUE)
  ENDIF()
ELSE()
  SET(WINDOWS_CRYPTO_FOUND FALSE)
ENDIF()

OPTION(BUILD_SHARED_LIBS "Build shared libraries" ON)
OPTION(SHARED_LIB_VERSIONNING "Add SO version in .so build" ON)

SET(PACKAGE "libzip")
SET(PACKAGE_NAME ${PACKAGE})
SET(PACKAGE_VERSION_MAJOR "1")
SET(PACKAGE_VERSION_MINOR "5")
SET(PACKAGE_VERSION_MICRO "2a")
#SET(VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}")
SET(VERSION "${PACKAGE_VERSION_MAJOR}.${PACKAGE_VERSION_MINOR}.${PACKAGE_VERSION_MICRO}")
SET(PACKAGE_VERSION ${VERSION})
SET(LIBZIP_VERSION ${PACKAGE_VERSION})
SET(LIBZIP_VERSION_MAJOR ${PACKAGE_VERSION_MAJOR})
SET(LIBZIP_VERSION_MINOR ${PACKAGE_VERSION_MINOR})
SET(LIBZIP_VERSION_MICRO ${PACKAGE_VERSION_MICRO})
SET(PACKAGE_STRING "${PACKAGE_NAME} ${PACKAGE_VERSION}")

SET(ARCHIVE_NAME ${PACKAGE_NAME}-${PACKAGE_VERSION})
IF(NOT TARGET dist)
ADD_CUSTOM_TARGET(dist
  COMMAND git config tar.tar.xz.command "xz -c"
  COMMAND git archive --prefix=${ARCHIVE_NAME}/ -o ${ARCHIVE_NAME}.tar.gz HEAD
  COMMAND git archive --prefix=${ARCHIVE_NAME}/ -o ${ARCHIVE_NAME}.tar.xz HEAD
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
ADD_CUSTOM_TARGET(distcheck
  COMMAND chmod -R u+w ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest 2>/dev/null || true
  COMMAND rm -rf ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest
  COMMAND cmake -E tar xf ${ARCHIVE_NAME}.tar.gz
  COMMAND chmod -R u-w ${ARCHIVE_NAME}
  COMMAND mkdir ${ARCHIVE_NAME}-build
  COMMAND mkdir ${ARCHIVE_NAME}-dest
  COMMAND cd ${ARCHIVE_NAME}-build && cmake -DCMAKE_INSTALL_PREFIX=../${ARCHIVE_NAME}-dest ../${ARCHIVE_NAME}
  COMMAND cd ${ARCHIVE_NAME}-build && make -j4
  COMMAND cd ${ARCHIVE_NAME}-build && make test
  COMMAND cd ${ARCHIVE_NAME}-build && make install
#  COMMAND cd ${ARCHIVE_NAME}-build && make uninstall
#  COMMAND if [ `find ${ARCHIVE_NAME}-dest ! -type d | wc -l` -ne 0 ]; then echo leftover files in ${ARCHIVE_NAME}-dest; false; fi
  COMMAND cd ${ARCHIVE_NAME}-build && make clean
  COMMAND chmod -R u+w ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest
  COMMAND rm -rf ${ARCHIVE_NAME} ${ARCHIVE_NAME}-build ${ARCHIVE_NAME}-dest
  COMMAND echo "${ARCHIVE_NAME}.tar.gz is ready for distribution."
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
ADD_DEPENDENCIES(distcheck dist)
ENDIF(NOT TARGET dist)

IF(BUILD_SHARED_LIBS)
  SET(HAVE_SHARED TRUE)
ELSE()
  SET(ZIP_STATIC TRUE)
ENDIF()

# Checks

CHECK_FUNCTION_EXISTS(_chmod HAVE__CHMOD)
CHECK_FUNCTION_EXISTS(_close HAVE__CLOSE)
CHECK_FUNCTION_EXISTS(_dup HAVE__DUP)
CHECK_FUNCTION_EXISTS(_fdopen HAVE__FDOPEN)
CHECK_FUNCTION_EXISTS(_fileno HAVE__FILENO)
CHECK_FUNCTION_EXISTS(_open HAVE__OPEN)
CHECK_FUNCTION_EXISTS(_setmode HAVE__SETMODE)
CHECK_FUNCTION_EXISTS(_snprintf HAVE__SNPRINTF)
CHECK_FUNCTION_EXISTS(_strdup HAVE__STRDUP)
CHECK_FUNCTION_EXISTS(_stricmp HAVE__STRICMP)
CHECK_FUNCTION_EXISTS(_strtoi64 HAVE__STRTOI64)
CHECK_FUNCTION_EXISTS(_strtoui64 HAVE__STRTOUI64)
CHECK_FUNCTION_EXISTS(_unlink HAVE__UNLINK)
CHECK_FUNCTION_EXISTS(arc4random HAVE_ARC4RANDOM)
CHECK_FUNCTION_EXISTS(clonefile HAVE_CLONEFILE)
CHECK_FUNCTION_EXISTS(explicit_bzero HAVE_EXPLICIT_BZERO)
CHECK_FUNCTION_EXISTS(explicit_memset HAVE_EXPLICIT_MEMSET)
CHECK_FUNCTION_EXISTS(fileno HAVE_FILENO)
CHECK_FUNCTION_EXISTS(fseeko HAVE_FSEEKO)
CHECK_FUNCTION_EXISTS(ftello HAVE_FTELLO)
CHECK_FUNCTION_EXISTS(getprogname HAVE_GETPROGNAME)
CHECK_FUNCTION_EXISTS(localtime_r HAVE_LOCALTIME_R)
CHECK_FUNCTION_EXISTS(open HAVE_OPEN)
CHECK_FUNCTION_EXISTS(setmode HAVE_SETMODE)
CHECK_FUNCTION_EXISTS(snprintf HAVE_SNPRINTF)
CHECK_FUNCTION_EXISTS(strcasecmp HAVE_STRCASECMP)
CHECK_FUNCTION_EXISTS(strdup HAVE_STRDUP)
CHECK_FUNCTION_EXISTS(stricmp HAVE_STRICMP)
CHECK_FUNCTION_EXISTS(strtoll HAVE_STRTOLL)
CHECK_FUNCTION_EXISTS(strtoull HAVE_STRTOULL)

CHECK_INCLUDE_FILES("sys/types.h;sys/stat.h;fts.h" HAVE_FTS_H)
CHECK_INCLUDE_FILES(stdbool.h HAVE_STDBOOL_H)
CHECK_INCLUDE_FILES(strings.h HAVE_STRINGS_H)
CHECK_INCLUDE_FILES(unistd.h HAVE_UNISTD_H)

CHECK_INCLUDE_FILES(inttypes.h HAVE_INTTYPES_H_LIBZIP)
CHECK_INCLUDE_FILES(stdint.h HAVE_STDINT_H_LIBZIP)
CHECK_INCLUDE_FILES(sys/types.h HAVE_SYS_TYPES_H_LIBZIP)

# TODO: fix test
# this test does not find __progname even when it exists
#CHECK_SYMBOL_EXISTS(__progname stdlib.h HAVE___PROGNAME)

CHECK_TYPE_SIZE(__int8 __INT8_LIBZIP)
CHECK_TYPE_SIZE(int8_t INT8_T_LIBZIP)
CHECK_TYPE_SIZE(uint8_t UINT8_T_LIBZIP)
CHECK_TYPE_SIZE(__int16 __INT16_LIBZIP)
CHECK_TYPE_SIZE(int16_t INT16_T_LIBZIP)
CHECK_TYPE_SIZE(uint16_t UINT16_T_LIBZIP)
CHECK_TYPE_SIZE(__int32 __INT32_LIBZIP)
CHECK_TYPE_SIZE(int32_t INT32_T_LIBZIP)
CHECK_TYPE_SIZE(uint32_t UINT32_T_LIBZIP)
CHECK_TYPE_SIZE(__int64 __INT64_LIBZIP)
CHECK_TYPE_SIZE(int64_t INT64_T_LIBZIP)
CHECK_TYPE_SIZE(uint64_t UINT64_T_LIBZIP)
CHECK_TYPE_SIZE("short" SHORT_LIBZIP)
CHECK_TYPE_SIZE("int" INT_LIBZIP)
CHECK_TYPE_SIZE("long" LONG_LIBZIP)
CHECK_TYPE_SIZE("long long" LONG_LONG_LIBZIP)
CHECK_TYPE_SIZE("off_t" SIZEOF_OFF_T)
CHECK_TYPE_SIZE("size_t" SIZE_T_LIBZIP)
CHECK_TYPE_SIZE("ssize_t" SSIZE_T_LIBZIP)

CHECK_C_SOURCE_COMPILES("#include <sys/ioctl.h>
#include <linux/fs.h>
int main(int argc, char *argv[]) { unsigned long x = FICLONERANGE; }" HAVE_FICLONERANGE)

CHECK_C_SOURCE_COMPILES("
int foo(char * _Nullable bar);
int main(int argc, char *argv[]) { }" HAVE_NULLABLE)

TEST_BIG_ENDIAN(WORDS_BIGENDIAN)

#FIND_PACKAGE(ZLIB 1.1.2 REQUIRED)
INCLUDE_DIRECTORIES(../zlib/zlib)
SET(CMAKE_REQUIRED_INCLUDES ../zlib/zlib)

IF(ENABLE_BZIP2)
  FIND_PACKAGE(BZip2)
  IF(BZIP2_FOUND)
    SET (HAVE_LIBBZ2 1)

    INCLUDE_DIRECTORIES(${BZIP2_INCLUDE_DIR})
    SET (OPTIONAL_LIBRARY ${OPTIONAL_LIBRARY} ${BZIP2_LIBRARIES})
  ELSE()
    MESSAGE(WARNING "-- bzip2 library not found; bzip2 support disabled")
  ENDIF(BZIP2_FOUND)
ENDIF(ENABLE_BZIP2)

IF(ENABLE_LZMA)
  FIND_PACKAGE(LibLZMA)
  IF(LIBLZMA_FOUND)
    SET (HAVE_LIBLZMA 1)

    INCLUDE_DIRECTORIES(${LIBLZMA_INCLUDE_DIR})
    SET (OPTIONAL_LIBRARY ${OPTIONAL_LIBRARY} ${LIBLZMA_LIBRARY})
  ELSE()
    MESSAGE(WARNING "-- lzma library not found; lzma support disabled")
  ENDIF(LIBLZMA_FOUND)
ENDIF(ENABLE_LZMA)


IF (COMMONCRYPTO_FOUND)
  SET (HAVE_CRYPTO 1)
  SET (HAVE_COMMONCRYPTO 1)
ELSEIF (WINDOWS_CRYPTO_FOUND)
  SET (HAVE_CRYPTO 1)
  SET (HAVE_WINDOWS_CRYPTO 1)
ELSEIF (GNUTLS_FOUND AND NETTLE_FOUND)
  SET (HAVE_CRYPTO 1)
  SET (HAVE_GNUTLS 1)
  INCLUDE_DIRECTORIES(${GNUTLS_INCLUDE_DIR} ${NETTLE_INCLUDE_DIR})
  SET (OPTIONAL_LIBRARY ${OPTIONAL_LIBRARY} ${GNUTLS_LIBRARY} ${NETTLE_LIBRARY})
ELSEIF (OPENSSL_FOUND)
  SET (HAVE_CRYPTO 1)
  SET (HAVE_OPENSSL 1)
  INCLUDE_DIRECTORIES(${OPENSSL_INCLUDE_DIR})
  SET (OPTIONAL_LIBRARY ${OPTIONAL_LIBRARY} ${OPENSSL_LIBRARIES})
ELSEIF (MBEDTLS_LIBRARIES)
  SET (HAVE_CRYPTO 1)
  SET (HAVE_MBEDTLS 1)
  INCLUDE_DIRECTORIES(${MBEDTLS_INCLUDE_DIR})
  SET (OPTIONAL_LIBRARY ${OPTIONAL_LIBRARY} ${MBEDTLS_LIBRARIES})
ENDIF()

IF (NOT HAVE_CRYPTO)
  MESSAGE(WARNING "-- neither Common Crypto, GnuTLS, mbed TLS, OpenSSL, nor Windows Cryptography found; AES support disabled")
ENDIF()

IF(MSVC)
ADD_DEFINITIONS("-D_CRT_SECURE_NO_WARNINGS")
ADD_DEFINITIONS("-D_CRT_NONSTDC_NO_DEPRECATE")
ENDIF(MSVC)

if(WIN32)
  if(HAVE_WINDOWS_CRYPTO)
    SET (OPTIONAL_LIBRARY ${OPTIONAL_LIBRARY} bcrypt)
  endif()
  if(CMAKE_SYSTEM_NAME MATCHES WindowsPhone OR CMAKE_SYSTEM_NAME MATCHES WindowsStore)
    ADD_DEFINITIONS(-DMS_UWP)
  else(CMAKE_SYSTEM_NAME MATCHES WindowsPhone OR CMAKE_SYSTEM_NAME MATCHES WindowsStore)
    SET (OPTIONAL_LIBRARY ${OPTIONAL_LIBRARY} advapi32)
  endif(CMAKE_SYSTEM_NAME MATCHES WindowsPhone OR CMAKE_SYSTEM_NAME MATCHES WindowsStore)
endif(WIN32)

ADD_DEFINITIONS("-DHAVE_CONFIG_H")

# rpath handling: use rpath in installed binaries
IF(NOT CMAKE_SYSTEM_NAME MATCHES Linux)
	SET(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
	SET(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
ENDIF()

# fixed size integral types

IF(HAVE_INTTYPES_H_LIBZIP)
  SET(LIBZIP_TYPES_INCLUDE "#define __STDC_FORMAT_MACROS 1
#include <inttypes.h>")
ELSEIF(HAVE_STDINT_H_LIBZIP)
  SET(LIBZIP_TYPES_INCLUDE "#include <stdint.h>")
ELSEIF(HAVE_SYS_TYPES_H_LIBZIP)
  SET(LIBZIP_TYPES_INCLUDE "#include <sys/types.h>")
ENDIF()

IF(HAVE_INT8_T_LIBZIP)
  SET(ZIP_INT8_T int8_t)
ELSEIF(HAVE___INT8_LIBZIP)
  SET(ZIP_INT8_T __int8)
ELSE()
  SET(ZIP_INT8_T "signed char")
ENDIF()

IF(HAVE_UINT8_T_LIBZIP)
  SET(ZIP_UINT8_T uint8_t)
ELSEIF(HAVE___INT8_LIBZIP)
  SET(ZIP_UINT8_T "unsigned __int8")
ELSE()
  SET(ZIP_UINT8_T "unsigned char")
ENDIF()

IF(HAVE_INT16_T_LIBZIP)
  SET(ZIP_INT16_T int16_t)
ELSEIF(HAVE___INT16_LIBZIP)
  SET(INT16_T_LIBZIP __int16)
ELSEIF(SHORT_LIBZIP EQUAL 2)
  SET(INT16_T_LIBZIP short)
ENDIF()

IF(HAVE_UINT16_T_LIBZIP)
  SET(ZIP_UINT16_T uint16_t)
ELSEIF(HAVE___INT16_LIBZIP)
  SET(UINT16_T_LIBZIP "unsigned __int16")
ELSEIF(SHORT_LIBZIP EQUAL 2)
  SET(UINT16_T_LIBZIP "unsigned short")
ENDIF()

IF(HAVE_INT32_T_LIBZIP)
  SET(ZIP_INT32_T int32_t)
ELSEIF(HAVE___INT32_LIBZIP)
  SET(ZIP_INT32_T __int32)
ELSEIF(INT_LIBZIP EQUAL 4)
  SET(ZIP_INT32_T int)
ELSEIF(LONG_LIBZIP EQUAL 4)
  SET(ZIP_INT32_T long)
ENDIF()

IF(HAVE_UINT32_T_LIBZIP)
SET(ZIP_UINT32_T uint32_t)
ELSEIF(HAVE___INT32_LIBZIP)
SET(ZIP_UINT32_T "unsigned __int32")
ELSEIF(INT_LIBZIP EQUAL 4)
SET(ZIP_UINT32_T "unsigned int")
ELSEIF(LONG_LIBZIP EQUAL 4)
SET(ZIP_UINT32_T "unsigned long")
ENDIF()

IF(HAVE_INT64_T_LIBZIP)
  SET(ZIP_INT64_T int64_t)
ELSEIF(HAVE___INT64_LIBZIP)
  SET(ZIP_INT64_T __int64)
ELSEIF(LONG_LIBZIP EQUAL 8)
  SET(ZIP_INT64_T long)
ELSEIF(LONG_LONG_LIBZIP EQUAL 8)
  SET(ZIP_INT64_T "long long")
ENDIF()

IF(HAVE_UINT64_T_LIBZIP)
  SET(ZIP_UINT64_T uint64_t)
ELSEIF(HAVE___INT64_LIBZIP)
  SET(ZIP_UINT64_T "unsigned __int64")
ELSEIF(LONG_LIBZIP EQUAL 8)
  SET(ZIP_UINT64_T "unsigned long")
ELSEIF(LONG_LONG_LIBZIP EQUAL 8)
  SET(ZIP_UINT64_T "unsigned long long")
ENDIF()

IF(HAVE_NULLABLE)
  SET(ZIP_NULLABLE_DEFINES)
ELSE()
  SET(ZIP_NULLABLE_DEFINES "#define _Nullable
#define _Nonnull")
ENDIF()

# write out config file
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/libzip/cmake-config.h.in ${CMAKE_CURRENT_BINARY_DIR}/libzip/config.h)
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/libzip/cmake-zipconf.h.in ${CMAKE_CURRENT_BINARY_DIR}/libzip/zipconf.h)

# installation
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libzip/zipconf.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
INSTALL(FILES libzip/lib/zip.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

SET(CMAKE_C_VISIBILITY_PRESET hidden)

ADD_LIBRARY(zip
  libzip/lib/zip_add.c
  libzip/lib/zip_add_dir.c
  libzip/lib/zip_add_entry.c
  libzip/lib/zip_algorithm_deflate.c
  libzip/lib/zip_buffer.c
  libzip/lib/zip_close.c
  libzip/lib/zip_delete.c
  libzip/lib/zip_dir_add.c
  libzip/lib/zip_dirent.c
  libzip/lib/zip_discard.c
  libzip/lib/zip_entry.c
  libzip/lib/zip_err_str.c
  libzip/lib/zip_error.c
  libzip/lib/zip_error_clear.c
  libzip/lib/zip_error_get.c
  libzip/lib/zip_error_get_sys_type.c
  libzip/lib/zip_error_strerror.c
  libzip/lib/zip_error_to_str.c
  libzip/lib/zip_extra_field.c
  libzip/lib/zip_extra_field_api.c
  libzip/lib/zip_fclose.c
  libzip/lib/zip_fdopen.c
  libzip/lib/zip_file_add.c
  libzip/lib/zip_file_error_clear.c
  libzip/lib/zip_file_error_get.c
  libzip/lib/zip_file_get_comment.c
  libzip/lib/zip_file_get_external_attributes.c
  libzip/lib/zip_file_get_offset.c
  libzip/lib/zip_file_rename.c
  libzip/lib/zip_file_replace.c
  libzip/lib/zip_file_set_comment.c
  libzip/lib/zip_file_set_encryption.c
  libzip/lib/zip_file_set_external_attributes.c
  libzip/lib/zip_file_set_mtime.c
  libzip/lib/zip_file_strerror.c
  libzip/lib/zip_filerange_crc.c
  libzip/lib/zip_fopen.c
  libzip/lib/zip_fopen_encrypted.c
  libzip/lib/zip_fopen_index.c
  libzip/lib/zip_fopen_index_encrypted.c
  libzip/lib/zip_fread.c
  libzip/lib/zip_fseek.c
  libzip/lib/zip_ftell.c
  libzip/lib/zip_get_archive_comment.c
  libzip/lib/zip_get_archive_flag.c
  libzip/lib/zip_get_encryption_implementation.c
  libzip/lib/zip_get_file_comment.c
  libzip/lib/zip_get_name.c
  libzip/lib/zip_get_num_entries.c
  libzip/lib/zip_get_num_files.c
  libzip/lib/zip_hash.c
  libzip/lib/zip_io_util.c
  libzip/lib/zip_libzip_version.c
  libzip/lib/zip_memdup.c
  libzip/lib/zip_name_locate.c
  libzip/lib/zip_new.c
  libzip/lib/zip_open.c
  libzip/lib/zip_progress.c
  libzip/lib/zip_rename.c
  libzip/lib/zip_replace.c
  libzip/lib/zip_set_archive_comment.c
  libzip/lib/zip_set_archive_flag.c
  libzip/lib/zip_set_default_password.c
  libzip/lib/zip_set_file_comment.c
  libzip/lib/zip_set_file_compression.c
  libzip/lib/zip_set_name.c
  libzip/lib/zip_source_accept_empty.c
  libzip/lib/zip_source_begin_write.c
  libzip/lib/zip_source_begin_write_cloning.c
  libzip/lib/zip_source_buffer.c
  libzip/lib/zip_source_call.c
  libzip/lib/zip_source_close.c
  libzip/lib/zip_source_commit_write.c
  libzip/lib/zip_source_compress.c
  libzip/lib/zip_source_crc.c
  libzip/lib/zip_source_error.c
  libzip/lib/zip_source_filep.c
  libzip/lib/zip_source_free.c
  libzip/lib/zip_source_function.c
  libzip/lib/zip_source_get_compression_flags.c
  libzip/lib/zip_source_is_deleted.c
  libzip/lib/zip_source_layered.c
  libzip/lib/zip_source_open.c
  libzip/lib/zip_source_pkware.c
  libzip/lib/zip_source_read.c
  libzip/lib/zip_source_remove.c
  libzip/lib/zip_source_rollback_write.c
  libzip/lib/zip_source_seek.c
  libzip/lib/zip_source_seek_write.c
  libzip/lib/zip_source_stat.c
  libzip/lib/zip_source_supports.c
  libzip/lib/zip_source_tell.c
  libzip/lib/zip_source_tell_write.c
  libzip/lib/zip_source_window.c
  libzip/lib/zip_source_write.c
  libzip/lib/zip_source_zip.c
  libzip/lib/zip_source_zip_new.c
  libzip/lib/zip_stat.c
  libzip/lib/zip_stat_index.c
  libzip/lib/zip_stat_init.c
  libzip/lib/zip_strerror.c
  libzip/lib/zip_string.c
  libzip/lib/zip_unchange.c
  libzip/lib/zip_unchange_all.c
  libzip/lib/zip_unchange_archive.c
  libzip/lib/zip_unchange_data.c
  libzip/lib/zip_utf-8.c
)

IF(WIN32)
  target_sources(zip PRIVATE
    libzip/lib/zip_source_win32handle.c
    libzip/lib/zip_source_win32utf8.c
    libzip/lib/zip_source_win32w.c
  )
  IF(CMAKE_SYSTEM_NAME MATCHES WindowsPhone OR CMAKE_SYSTEM_NAME MATCHES WindowsStore)
  ELSE()
    target_sources(zip PRIVATE libzip/lib/zip_source_win32a.c)
  ENDIF()
ELSE()
  target_sources(zip PRIVATE
    libzip/lib/zip_mkstempm.c
    libzip/lib/zip_source_file.c
    libzip/lib/zip_random_unix.c
  )
ENDIF()

IF(HAVE_LIBBZ2)
  target_sources(zip PRIVATE libzip/lib/zip_algorithm_bzip2.c)
ENDIF()

IF(HAVE_LIBLZMA)
  target_sources(zip PRIVATE libzip/lib/zip_algorithm_xz.c)
ENDIF()

IF(HAVE_COMMONCRYPTO)
  target_sources(zip PRIVATE libzip/lib/zip_crypto_commoncrypto.c)
ELSEIF(HAVE_WINDOWS_CRYPTO)
  target_sources(zip PRIVATE libzip/lib/zip_crypto_win.c)
ELSEIF(HAVE_GNUTLS)
  target_sources(zip PRIVATE libzip/lib/zip_crypto_gnutls.c)
ELSEIF(HAVE_OPENSSL)
  target_sources(zip PRIVATE libzip/lib/zip_crypto_openssl.c)
ELSEIF(HAVE_MBEDTLS)
  target_sources(zip PRIVATE libzip/lib/zip_crypto_mbedtls.c)
ENDIF()

IF(HAVE_CRYPTO)
  target_sources(zip PRIVATE
    libzip/lib/zip_winzip_aes.c
    libzip/lib/zip_source_winzip_aes_decode.c
    libzip/lib/zip_source_winzip_aes_encode.c
  )
ENDIF()

target_include_directories(zip
PUBLIC
  libzip/lib
  ${CMAKE_CURRENT_BINARY_DIR}/libzip
)

# pkgconfig file
SET(prefix ${CMAKE_INSTALL_PREFIX})
SET(exec_prefix \${prefix})
SET(bindir \${exec_prefix}/${CMAKE_INSTALL_BINDIR})
SET(libdir \${exec_prefix}/${CMAKE_INSTALL_LIBDIR})
SET(includedir \${prefix}/${CMAKE_INSTALL_INCLUDEDIR})
IF(CMAKE_SYSTEM_NAME MATCHES BSD)
  SET(PKG_CONFIG_RPATH "-Wl,-R\${libdir}")
ENDIF(CMAKE_SYSTEM_NAME MATCHES BSD)
get_target_property(LIBS_PRIVATE zip LINK_LIBRARIES)
foreach(LIB ${LIBS_PRIVATE})
  if(LIB MATCHES "^/")
    get_filename_component(LIB ${LIB} NAME_WE)
    string(REGEX REPLACE "^lib" "" LIB ${LIB})
  endif()
  set(LIBS "${LIBS} -l${LIB}")
endforeach()
CONFIGURE_FILE(libzip/libzip.pc.in libzip/libzip.pc @ONLY)
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/libzip.pc DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

ADD_CUSTOM_TARGET(update_zip_err_str
  COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/libzip/lib/make_zip_err_str.sh ${CMAKE_CURRENT_SOURCE_DIR}/libzip/lib/zip.h ${CMAKE_CURRENT_SOURCE_DIR}/libzip/lib/zip_err_str.c
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/libzip/lib/zip.h ${CMAKE_CURRENT_SOURCE_DIR}/libzip/lib/make_zip_err_str.sh
)

IF(SHARED_LIB_VERSIONNING)
SET_TARGET_PROPERTIES(zip PROPERTIES VERSION 5.0 SOVERSION 5)
ENDIF()

TARGET_LINK_LIBRARIES(zip ${ZLIB_LIBRARIES} ${OPTIONAL_LIBRARY})
INSTALL(TARGETS zip
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

