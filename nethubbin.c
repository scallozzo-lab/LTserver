#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "rcserver.h"
#include "nethubbin.h"
#include "db.h"

static stNetHUBBin NetHUBBin = {0};
static stFwHubCtrl FwHubCtrl[_CANT_MAX_HUB] = {0};
    
static uint32_t hex8_to_u32(const uint8_t *s)
{
    uint32_t v = 0;

    for (int i = 0; i < 8; i++)
    {
        uint8_t c = s[i];
        uint8_t n;

        if (c >= '0' && c <= '9')      n = c - '0';
        else if (c >= 'A' && c <= 'F') n = c - 'A' + 10;
        else if (c >= 'a' && c <= 'f') n = c - 'a' + 10;
        else return 0; // error

        v = (v << 4) | n;
    }

    return v;
}

// ---------- CRC32 ----------
static uint32_t crc32_calc(const uint8_t *data, size_t len, uint32_t seed)
{
    uint32_t crc = seed; //0xFFFFFFFF 
    for (size_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++)
            crc = (crc & 1) ? (crc >> 1) ^ 0xEDB88320 : crc >> 1;
    }
    return ~crc;
}
    
static
int load_binary_file(const char *path, stNetHUBBin *out_file)
{
    if (!path || !out_file)
        return -1;

    FILE *fp = fopen(path, "rb");
    if (!fp)
        return -2;

    // Obtener tamaño
    if (fseek(fp, 0, SEEK_END) != 0)
    {
        fclose(fp);
        return -3;
    }

    long size = ftell(fp);
    if (size < 0 || size > _MAX_NETHUBBIN_SIZE)
    {
        fclose(fp);
        return -4;
    }

    rewind(fp);

    // Completar estructura
    memset(out_file, 0, sizeof(stNetHUBBin));

    // Leer archivo completo
    size_t read_bytes = fread(NetHUBBin.filebuffer, 1, size, fp);
    fclose(fp);

    strncpy(out_file->filename, path, sizeof(out_file->filename) - 1);
    out_file->length = (uint32_t)size;
 
    return 0;
}

uint16_t _GetTotBinFrames(uint16_t binlen)
{
   return (binlen + _MAXFRAMEFWUPDATE - 1) / _MAXFRAMEFWUPDATE;
}            

int _LoadNetHubBin(void)
{
    uint8_t tmpbin[_MAX_NETHUBBIN_SIZE];

    if (load_binary_file(_NETHUBFILE, &NetHUBBin) == 0)
    {
        _log("[_LoadNetHubBin] Archivo: %s\n", NetHUBBin.filename);
        _log("[_LoadNetHubBin] Tamaño: %u bytes\n", NetHUBBin.length);
        
        // Copia a buffer local solo para comprar sobreescribir campos y comparar CRC
        memcpy(tmpbin, NetHUBBin.filebuffer, sizeof(tmpbin));

        stfwverid *pst = (stfwverid *)&tmpbin[_FLASH_OFFSET_HEADER];

        // Si se encuentra el header de versión:
        if(!memcmp(pst->id, _IDFLAG, sizeof(pst->id)))
        {
            uint32_t binlen = hex8_to_u32(pst->lenstr);
            uint32_t bincrc = hex8_to_u32(pst->sigstr);           
            
            _log("[_LoadNetHubBin] FW.Header->\n");
            _log("len %08X\n", binlen);
            _log("signature %08X\n", bincrc);
            
            memset(pst->lenstr, '_', sizeof(pst->lenstr));
            memset(pst->sigstr, '_', sizeof(pst->sigstr));
            
            uint32_t crc = crc32_calc(tmpbin, NetHUBBin.length - 4, 0xFFFFFFFFLU);
            
            _log("FW.Header crc calc %08X\n", crc);
            if(crc == bincrc) 
            {
                NetHUBBin.ver[0] = pst->version[0];
                NetHUBBin.ver[1] = pst->version[1];
                NetHUBBin.ver[2] = pst->rev;
                NetHUBBin.fwtype[0] = pst->fwtype[0];
                NetHUBBin.fwtype[1] = pst->fwtype[1];
                NetHUBBin.fwtype[2] = pst->fwtype[2];
                NetHUBBin.totframes = _GetTotBinFrames(binlen);
                NetHUBBin.framesize = _MAXFRAMEFWUPDATE;
                NetHUBBin.flag = true;

                _log("[_LoadNetHubBin] Firmware NetHUB V%c.%c (%c%c%c) OK\n", 
                                                                                        pst->version[0], 
                                                                                        pst->version[1], 
                                                                                        pst->fwtype[0], 
                                                                                        pst->fwtype[1], 
                                                                                        pst->fwtype[2]);
                return 1;
            }
            else 
            {
                _log("[_LoadNetHubBin] Firmware NetHUB Error de CRC %08X-%08X\n", crc, bincrc);
                return 0; 
            }
        }
        else
        {    
            _log("[_LoadNetHubBin] Warning FW.Header NO PRESENTE. Versión inválida!\n");
            return 0;
        }
    }
    else
    {
        _log("[_LoadNetHubBin] ERROR carga binario %s\n", _NETHUBFILE);
    }
    return 0;
}

stNetHUBBin *_GetNetHubFileInfo(void)
{
    if(NetHUBBin.flag == true)
        return &NetHUBBin;
    else return 0;
}

stFwHubCtrl *_Create_Hub(uint64_t id)
{
    for (int i = 0; i < _CANT_MAX_HUB; i++)
    {
        if (FwHubCtrl[i].id == 0)
        {
            FwHubCtrl[i].id = id;
            FwHubCtrl[i].last_seen = time(NULL);
            return &FwHubCtrl[i];
        }
    }
    return NULL; // tabla llena
}

stFwHubCtrl *_Find_Hub(uint64_t id)
{
    for (int i = 0; i < _CANT_MAX_HUB; i++)
    {
        if (FwHubCtrl[i].id == id)
            return &FwHubCtrl[i];
    }
    return NULL;
}
