#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "storefiles.h"
#include "dsvv.h"


#define MAX_BUFFER_SIZE 1000

stStoreV _StoreV[_MAXSTOREV];



int *_strcpyln(int8_t *p_dest, const int8_t *p_org)
{
    int *ret = 0;

    if(p_dest && p_org)
    {
        while(*p_org != 0 && *p_org != '{')
            *p_dest++ = *p_org++;
        
        p_dest--;
        if(*p_dest == '/') *p_dest = 0;
        ret = p_dest;
    }
    return ret;
}

void _InitStoreFiles(void)
{
    memset(_StoreV, 0, sizeof(_StoreV));
}

uint32_t _Look4Free(uint32_t SrcIp)
{
    for(int idx = 0; idx < _MAXSTOREV; idx++)    
        if(_StoreV[idx].SrcIp == 0)
        {    
            _StoreV[idx].SrcIp = SrcIp;
            return idx + 1;    
        }
    return 0;
}

uint32_t _Look4Pos(uint32_t SrcIp)
{
    for(uint32_t idx = 0; idx < _MAXSTOREV; idx++)    
        if(_StoreV[idx].SrcIp == SrcIp && _StoreV[idx].file)
        {    
            return idx + 1;    
        }
    return 0;
}

void _Look4Release(uint32_t idx)
{
    if(idx < _MAXSTOREV)
    {    
        _StoreV[idx].SrcIp = 0;
        _StoreV[idx].file = 0;
    }
}

void _ProcStFilesTimeout(void)
{
    static uint8_t Div = 0;

    // Cada 1Seg
    if(Div++ >= 100)
    {
        for(uint32_t idx = 0; idx < _MAXSTOREV; idx++)    
        {
            if(_StoreV[idx].SrcIp)
            {
                if(_StoreV[idx].WTimer) _StoreV[idx].WTimer--;
                else
                {    
                    printf("[_ProcStFilesTimeout] Vector _StoreV liberado (%d)\n", idx);
                    _log("[_ProcStFilesTimeout] Vector _StoreV liberado (%d)\n", idx);
                    _Look4Release(idx);
                }
            }    
        }    
        Div = 0;
    }
}
    

int _CreateFolder(uint8_t *ppath)
{
    struct stat info;
    
    // Check if the directory exists
    if(stat(ppath, &info)) 
    {
#ifdef _DEBUG_RCSERVER
        printf("path %s\n", ppath);
#endif
        if(S_ISDIR(info.st_mode)) 
        {
  
            printf("create dir 1\n");
            if(mkdir(ppath, 0777))
                return 1;
            else return 0;
  
        }
        else 
        {
#ifdef _DEBUG_RCSERVER
            printf("create mkdir\n");
#endif
            if(mkdir(ppath, 0777))
                return 1;
            else return 0;
        }
    } 
    // Sino el directorio ya existe
    else
        return 0;
}


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
                    uint16_t *pFileCrc) 
{
    uint32_t idx = 0;
    uint8_t CFilename[_MAXTMPFNAME];

    // Si es el primer bloque y NO es reintento, genera el archivo y almacena el handler 
    if((Boffset == 0) && (_Look4Pos(Srcip) == 0) )
    {
        if(Srcip == 0) return ST_ERR_SRCIP;
        // Si hay espacio disponible
        if(idx = _Look4Free(Srcip))
        {
            // Establece path absoluto al archivo 
            if(PName)
            {
                memset(CFilename, 0, sizeof(CFilename));

#ifdef _LOCAL_PATH_VIDEOS                
                strcpy(CFilename, _LOCAL_PATH_VIDEOS);
                if(_CreateFolder(CFilename))
                {    
                    _Look4Release(idx - 1);
                    return ST_ERR_TARGET_CREATE;
                }
                CFilename[strlen(CFilename)] = '/';
#endif                
                
                // Agrega el path configurado en el municipio correspondiente (ignora /{year})
                _strcpyln(&CFilename[strlen(CFilename)], PName);

                // Agrega el path configurado en el municipio correspondiente
                //strcpy(&CFilename[strlen(CFilename)], PName);

                if(_CreateFolder(CFilename))
                {    
                    _Look4Release(idx - 1);
                    return ST_ERR_TARGET_CREATE;
                }
                CFilename[strlen(CFilename)] = '/';
                CFilename[strlen(CFilename)] = '_';
                // extrae el año del nombre de archivo
                memcpy(&CFilename[strlen(CFilename)], &FName[3], 4);

                if(_CreateFolder(CFilename))
                {
                    _Look4Release(idx - 1);
                    return ST_ERR_TARGET_CREATE;
                }
                
                CFilename[strlen(CFilename)] = '/';
                // copia el nombre de archivo completo
                strcpy(&CFilename[strlen(CFilename)], FName);

#ifdef _DEBUG_RCSERVER
                printf("[_StoreFile] path completo->%s\n", CFilename);
                _log("[_StoreFile] path completo->%s\n", CFilename);
#endif                
                
                // Open the binary file in write mode
                FILE *file = fopen(CFilename, "wb");
                if (file == NULL) 
                {
                    perror("[_StoreFile] Error opening file\n");
                    _log("[_StoreFile] Error opening file\n");
                    _Look4Release(idx - 1);
                    return ST_ERR_OFILE;
                }
                else 
                {    
                    _StoreV[idx - 1].file = file; 
                    _StoreV[idx - 1].TxfrId = TxfrId;
                    _StoreV[idx - 1].BOffset = 0xfffffffflu;
                    _StoreV[idx - 1].BlockNr = 0;
                    _StoreV[idx - 1].FileCRC = 0;
                    _StoreV[idx - 1].Status = 0;
                    memset(_StoreV[idx - 1].CFileName, 0, sizeof(_StoreV[idx - 1].CFileName));
                    strcpy(_StoreV[idx - 1].CFileName, CFilename);
                    _StoreV[idx - 1].WTimer = _MAXTIMEOUT;
                }
            }
            else
            {    
                _Look4Release(idx - 1);
                return ST_ERR_TARGET_FOLDER;
            }
        }
        // Sino error, sin espacio
        else return ST_ERR_NOHANDLER_FREE;
    }
    // Sino busca la posición almacenada
    else 
    {
        idx = _Look4Pos(Srcip);
        if(idx == 0)    
        {
            printf("[_StoreFile] Error _Look4Pos ip:%X\n", Srcip);
            _log("[_StoreFile] Error _Look4Pos ip:%X\n", Srcip);
            return ST_ERR_IPLOOKUP;
        }
    }

    if((Boffset != _StoreV[idx - 1].BOffset) /*&& ((_StoreV[idx - 1].BlockNr) != RxBlockNr)*/ )
    {
        if(_StoreV[idx - 1].TxfrId == TxfrId)
        {           
            uint16_t fcrc = CalcCrc16(inBuffer, Buflen, _StoreV[idx - 1].FileCRC);
            if(RxBCrc == fcrc)
            {
                _StoreV[idx - 1].BOffset = Boffset;
                _StoreV[idx - 1].BlockNr = RxBlockNr;
                _StoreV[idx - 1].FileCRC = fcrc;
            }
            if(pBlockNr) *pBlockNr = _StoreV[idx - 1].BlockNr;
            if(pFileCrc) *pFileCrc = _StoreV[idx - 1].FileCRC; 
            
            if(RxBCrc != fcrc)
            {
                printf("[_StoreFile] ERROR FileCRC %04X-%04X\n", RxBCrc, fcrc);
                _log("[_StoreFile] ERROR FileCRC %04X-%04X\n", RxBCrc, fcrc);
                return ST_ERR_BLOCKCRC;
            }
    

#ifdef _AUTODECRYPTION            
            if(Boffset == 0)
            {
                if(Buflen >= _MAXLGVIDEO2ENC)
                {    
                    if(*((unsigned int*)inBuffer) != _ENCFLAGVAL)
                    {
                        printf("[_StoreFile] ERROR Archivo %s No está encriptado\n", FName);
                        _log("[_StoreFile] ERROR Archivo %s No está encriptado\n", FName);
                    }
                    else
                    {
                        _EncriptDecript(inBuffer, _MAXLGVIDEO2ENC, 0);
                        _StoreV[idx - 1].Status |= SF_STS_DECRYP_DONE;
                    }
                }
                else
                {
                    printf("[_StoreFile] ERROR InBuffer < _MAXLGVIDEO2ENC\n");
                    _log("[_StoreFile] ERROR InBuffer < _MAXLGVIDEO2ENC\n");      
                }
            }
#endif            
            // Recarga el timer
            _StoreV[idx - 1].WTimer = _MAXTIMEOUT;
        
            // Write the contents of the buffer to the file
            size_t bytes_written = fwrite(inBuffer, sizeof(unsigned char), Buflen, _StoreV[idx - 1].file);
            if (bytes_written != Buflen) 
            {
                perror("Error writing to file");
                _log("Error writing to file");
                fclose(_StoreV[idx - 1].file);
                _Look4Release(idx - 1);
                return ST_ERR_WRITEERR;
            }
            // Sino si es ultimo bloque, libera todo
            else if(lblock == LBLOCK_CLOSE)
            {                
                // Close the file
                fclose(_StoreV[idx - 1].file);
                //_Look4Release(idx - 1);

#ifdef _AUTODECRYPTION
                if(_StoreV[idx - 1].Status & SF_STS_DECRYP_DONE)
                {
                    memset(CFilename, 0, sizeof(CFilename));            
                    int ptrpos = strcopys(CFilename, _StoreV[idx - 1].CFileName, '.');
		            memcpy(&CFilename[ptrpos], _VIDEOFILE_OUTPUTEXT, 4);
                    printf("old name %s new name %s\n", _StoreV[idx - 1].CFileName, CFilename);
                    rename(_StoreV[idx - 1].CFileName, CFilename);
                }
#endif        
            }
        }
        else 
        {    
            printf("[_StoreFile] TxfrId perdida %X\n\r", _StoreV[idx - 1].TxfrId);
            _log("[_StoreFile] TxfrId perdida %X\n\r", _StoreV[idx - 1].TxfrId);
            // Close the file
            fclose(_StoreV[idx - 1].file);
            _Look4Release(idx - 1);
            return ST_ERR_TXFRID_LOST;                   
        }
    }
    else
    {    
        if(pBlockNr) *pBlockNr = _StoreV[idx - 1].BlockNr;
        if(pFileCrc) *pFileCrc = _StoreV[idx - 1].FileCRC; 
        printf("[_StoreFile] Reintento offset %X\n\r", Boffset);
        _log("[_StoreFile] Reintento offset %X\n\r", Boffset);
        //printf("BlockNr %d - RxBlockNr %d -\n\r", _StoreV[idx - 1].BlockNr, RxBlockNr);
    }
    return ST_STS_OK;
}
