#ifndef __NETHUBBIN_H__
    #define  __NETHUBBIN_H__

#include <stdint.h>
#include <stddef.h>

#define _MAX_NETHUBBIN_SIZE (64 * 1024)
#define _NETHUBFILE     "./bins/firmware.bin"

#define _IDFLAG "@@ATLTX#" 
#define _FLASH_OFFSET_HEADER	0x010CLU		// Offset donde se encuentra el header de versión (después de la tabla de saltos)
#define  FLASH_MAX_LEN_FW   	49152LU 		//bytes

typedef struct
{
    uint8_t flag;
	char	filename[256];
    uint16_t length;
    uint8_t filebuffer[_MAX_NETHUBBIN_SIZE];
	uint8_t ver[3];
	uint8_t fwtype[3];
	uint16_t totframes;
	uint16_t framesize;
}stNetHUBBin;



typedef struct __attribute__((packed)) {
	uint8_t id[8];
	uint8_t version[2];
	uint8_t rev;
	uint8_t fwtype[3];
	uint8_t lenid;
	uint8_t lenstr[8];
	uint8_t sigid;
	uint8_t sigstr[8];
}stfwverid;

typedef struct
{
    uint64_t id;          // 0 = slot libre
    uint32_t last_seen;
    uint32_t status;
    uint32_t fw_version;
}stFwHubCtrl;


static int hub_count = 0;
int _LoadNetHubBin(void);
stNetHUBBin *_GetNetHubFileInfo(void);
uint16_t _GetTotBinFrames(uint16_t binlen);


#endif