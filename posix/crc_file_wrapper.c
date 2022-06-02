#include "crc_file_wrapper.h"

#if (REDCONF_API_POSIX == 1) && (REDCONF_ATTRIBUTES_MAX > 0)

#include <redmisc.h>
#include <redutils.h>

#define BUFFER_SIZE 4096

static int32_t crc_file_wrapper_compute_crc(int32_t iFildes, uint32_t *pulCRC) {
  int64_t llOffset = 0;

  /* initialiaze the CRC to 0 */
  *pulCRC = 0;

  /* reset file offset to file start */
  llOffset = red_lseek(iFildes, 0, RED_SEEK_SET);

  if (llOffset == 0) {
    int32_t iLength = 0;
    uint8_t abBuffer[BUFFER_SIZE];

    /* compute the CRC on the all file */
    while ((iLength = red_read(iFildes, abBuffer, sizeof(abBuffer))) > 0) {
      *pulCRC = RedCrc32Update(*pulCRC, abBuffer, (uint32_t)iLength);
    }
  }

  return (int32_t)llOffset;
}


int32_t crc_file_wrapper_stat(const char *pszPath, REDSTAT *pStat) {
  int32_t iRet = red_stat(pszPath, pStat);

  if ((iRet != -1) && RED_S_ISREG(pStat->st_mode)) {
    iRet = crc_file_wrapper_check(pszPath);
  }

  return iRet;
}

int32_t crc_file_wrapper_open(const char *pszPath, uint32_t ulOpenMode) {
  if (ulOpenMode & RED_O_WRONLY) {
    /* In order to compute the CRC on the file data we need to be able
     * to read them
     */
    ulOpenMode = (ulOpenMode & ~RED_O_WRONLY) | RED_O_RDWR;
  }

  return red_open(pszPath, ulOpenMode);
}

int32_t crc_file_wrapper_close(int32_t iFildes) { return red_close(iFildes); }

int32_t crc_file_wrapper_read(int32_t iFildes, void *pBuffer,
                              uint32_t ulLength) {
  return red_read(iFildes, pBuffer, ulLength);
}

int32_t crc_file_wrapper_write(int32_t iFildes, const void *pBuffer,
                               uint32_t ulLength) {
  REDSTAT Stat;
  int64_t llOffset = -1;
  int32_t iLength = -1;
  int32_t iRet = red_fstat(iFildes, &Stat);

  if (iRet != -1) {
    /* retrieve current file offset */
    iRet = llOffset = red_lseek(iFildes, 0, RED_SEEK_CUR);
  }

  if (iRet != -1) {
    /* write the data */
    iLength = red_write(iFildes, pBuffer, ulLength);
  }

  if (iLength > 0) {
    /* We wrote 1 byte or more, so the CRC needs to be changed */
    uint32_t ulCRCattribute = 0;

    if ((uint64_t)llOffset == Stat.st_size) {
      /* retreive the curent CRC */
      iRet = red_fgetxattr(iFildes, FILE_CRC_ATTRIBUTE, &ulCRCattribute);

      if (iRet != -1) {
        /* Add the written data segment to the CRC */
        ulCRCattribute =
            RedCrc32Update(ulCRCattribute, pBuffer, (uint32_t)iLength);
      }
    } else {
      /* recompute the whole CRC */
      iRet = crc_file_wrapper_compute_crc(iFildes, &ulCRCattribute);
    }

    if (iRet != -1) {
      /* associate the new CRC to the file */
      iRet = red_fsetxattr(iFildes, FILE_CRC_ATTRIBUTE, ulCRCattribute);
    }

    llOffset = llOffset + (int64_t)iLength;

    /* Restore the file offset */
    if (llOffset != red_lseek(iFildes, llOffset, RED_SEEK_SET)) {
      iRet = -1;
    }

    REDASSERT(iRet != -1);
  }

  return iLength;
}

int32_t crc_file_wrapper_fsync(int32_t iFildes) { return red_fsync(iFildes); }

int32_t crc_file_wrapper_lseek(int32_t iFildes, int64_t llOffset,
                               REDWHENCE whence) {
  return red_lseek(iFildes, llOffset, whence);
}

int32_t crc_file_wrapper_ftruncate(int32_t iFildes, uint64_t ullSize) {
  uint32_t ulCRCcomputed = 0;
  int64_t llOffset = -1;
  int32_t iRet = red_ftruncate(iFildes, ullSize);

  if (iRet != -1) {
    /* retrieve current file offset */
    iRet = llOffset = red_lseek(iFildes, 0, RED_SEEK_CUR);
  }

  if (iRet != -1) {
    /* compute the new CRC */
    iRet = crc_file_wrapper_compute_crc(iFildes, &ulCRCcomputed);
  }

  if (iRet != -1) {
    /* associate the new CRC to the file */
    iRet = red_fsetxattr(iFildes, FILE_CRC_ATTRIBUTE, ulCRCcomputed);
  }

  if (llOffset != -1) {
    /* Restore the file offset */
    if (llOffset != red_lseek(iFildes, llOffset, RED_SEEK_SET)) {
      iRet = -1;
    }
  }

  return iRet;
}

int32_t crc_file_wrapper_fstat(int32_t iFildes, REDSTAT *pStat) {
  return red_fstat(iFildes, pStat);
}

int32_t crc_file_wrapper_check(const char *pszPath) {
  REDSTAT Stat;
  int32_t iFildes = -1;
  int32_t iRet = iFildes = red_open(pszPath, RED_O_RDONLY);

  if (iRet != -1) {
    iRet = red_fstat(iFildes, &Stat);
  }

  if ((iRet != -1) && RED_S_ISREG(Stat.st_mode)) {
    uint32_t ulCRCattribute = 0;
    uint32_t ulCRCcomputed = 0;

    /* retreive the CRC attribute */
    iRet = red_fgetxattr(iFildes, FILE_CRC_ATTRIBUTE, &ulCRCattribute);

    if (iRet != -1) {
      /* compute the CRC on the file */
      iRet = crc_file_wrapper_compute_crc(iFildes, &ulCRCcomputed);
    }

    if (iRet != -1) {
      if (ulCRCattribute == ulCRCcomputed) {
        /* The 2 CRC are matching */
        iRet = 0;
      } else {
        iRet = -1;
      }
    }
  }

  if (iFildes != -1) {
    /* close the file */
    int32_t iRetclose = red_close(iFildes);

    REDASSERT(iRetclose != -1);

    (void)iRetclose;
  }

  return iRet;
}

int32_t crc_file_wrapper_fix(const char *pszPath) {
  REDSTAT Stat;
  int32_t iFildes = -1;
  int32_t iRet = iFildes = red_open(pszPath, RED_O_RDONLY);

  if (iRet != -1) {
    iRet = red_fstat(iFildes, &Stat);
  }

  if ((iRet != -1) && RED_S_ISREG(Stat.st_mode)) {
    uint32_t ulCRCcomputed = 0;

    /* compute the file CRC */
    iRet = crc_file_wrapper_compute_crc(iFildes, &ulCRCcomputed);

    if (iRet != -1) {
      /* associate the new CRC to the file */
      iRet = red_fsetxattr(iFildes, FILE_CRC_ATTRIBUTE, ulCRCcomputed);
    }
  }

  if (iFildes != -1) {
    /* close the file */
    int32_t iRetclose = red_close(iFildes);

    REDASSERT(iRetclose != -1);

    (void)iRetclose;
  }

  return iRet;
}

#endif /* (REDCONF_API_POSIX == 1) && (REDCONF_ATTRIBUTES_MAX > 0) */
