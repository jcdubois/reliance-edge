#ifndef CRCFILEWRAPPER_H
#define CRCFILEWRAPPER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <redconf.h>

#if (REDCONF_API_POSIX == 1) && (REDCONF_ATTRIBUTES_MAX > 0)

#include <redposix.h>

#define FILE_CRC_ATTRIBUTE 0

int32_t crc_file_wrapper_stat(const char *pszPath, REDSTAT *pStat);

int32_t crc_file_wrapper_open(const char *pszPath, uint32_t ulOpenMode);

int32_t crc_file_wrapper_close(int32_t iFildes);

int32_t crc_file_wrapper_read(int32_t iFildes, void *pBuffer,
                              uint32_t ulLength);

int32_t crc_file_wrapper_lseek(int32_t iFildes, int64_t llOffset,
                               REDWHENCE whence);

int32_t crc_file_wrapper_fstat(int32_t iFildes, REDSTAT *pStat);

int32_t crc_file_wrapper_check(const char *pszPath);

#if REDCONF_READ_ONLY == 0
int32_t crc_file_wrapper_write(int32_t iFildes, const void *pBuffer,
                               uint32_t ulLength);

int32_t crc_file_wrapper_fsync(int32_t iFildes);

#if REDCONF_API_POSIX_FTRUNCATE == 1
int32_t crc_file_wrapper_ftruncate(int32_t iFildes, uint64_t ullSize);
#endif

int32_t crc_file_wrapper_fix(const char *pszPath);
#endif

#endif /* (REDCONF_API_POSIX == 1) && (REDCONF_ATTRIBUTES_MAX > 0) */

#ifdef __cplusplus
}
#endif

#endif /* CRCFILEWRAPPER_H */
