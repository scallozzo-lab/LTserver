#ifndef __STOREFILES__H__
    #define __STOREFILES__H__

#include <stdio.h>
#include "rcserver.h"

#define _MAXSTOREV      100
#define _MAXTMPFNAME    256
#define _MAXTIMEOUT     30        // Base 1Seg (Max 255)

typedef struct
{
    uint32_t SrcIp;
    FILE *file;    
    uint32_t BOffset;
    uint32_t TxfrId;
    uint32_t BlockNr;
    uint16_t FileCRC;
    uint8_t  Status;
    uint8_t  CFileName[_MAXTMPFNAME];
    uint8_t WTimer;
}stStoreV;


typedef enum
{
    ST_STS_OK   = 0,
    ST_ERR_OFILE,
    ST_ERR_IPLOOKUP,
    ST_ERR_WRITEERR,
    ST_ERR_NOHANDLER_FREE,
    ST_ERR_TARGET_FOLDER,
    ST_ERR_TARGET_CREATE,
    ST_ERR_TXFRID_LOST,
    ST_ERR_SRCIP,
    ST_ERR_BLOCKCRC
}estorefiles;

typedef enum
{
    SF_STS_DECRYP_DONE  = BIT0,
    SF_STS_res1         = BIT1,
    SF_STS_res2         = BIT2,
    SF_STS_res3         = BIT3,
    SF_STS_res4         = BIT4,
    SF_STS_res5         = BIT5,
    SF_STS_res6         = BIT6,
    SF_STS_res7         = BIT7
}esfilesstatus;

void _InitStoreFiles(void);
uint8_t _StoreFile(uint8_t *FName, 
                    uint8_t *PName, 
                    uint8_t *inBuffer, 
                    uint16_t Buflen, 
                    uint32_t Boffset, 
                    uint8_t lblock, 
                    uint32_t RxBlockNr,
                    uint32_t Srcip, 
                    uint32_t TxfrId,
                    uint16_t RxBCrc, 
                    uint32_t *pBlockNr, 
                    uint16_t *pFileCrc); 

void _ProcStFilesTimeout(void);

#endif    
