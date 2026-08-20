#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "db_devices.h"

stDb_T_sector T_sector[_CANT_MAX_EQ];
//stDb_T_devices T_devices[_CANT_MAX_EQ];

void Hex2Bin(uint8_t *pdat, uint8_t *pdest, uint8_t len)
{
  uint8_t localbuf[255];
  uint8_t *pl = localbuf; 
  uint8_t tlen = len;
   
  if(len <= (sizeof(localbuf) / 2))
  {
    memset(localbuf,0,sizeof(localbuf));
      
    while(len--)
    {
      uint8_t lh = *pdat++;
      uint8_t lb = *pdat++;
      
      if(lh >= 'A' && lh <= 'F') lh -= 0x37;
      else lh -= 0x30;
      if(lb >= 'A' && lb <= 'F') lb -= 0x37;
      else lb -= 0x30;
      
      *pl++ = (lh<<4) + lb;
    }
    memcpy(pdest, localbuf, tlen/2);		
  }
  else printf("[Hex2Bin] Trama fuera de Rango len:%d\n\r",len);
}

int _Init_dbread(void)
{
    int ret = _DB_STS_OK;
    _InitDb_devices();
    _InitDb_devstate();
    ret = _dbread_bulk();
}



int _dbread_bulk(void)
{
    int ret = 0;
    
    //ret |= _dbread_table_sector(); 
    ret |= _dbread_table_devices();
    ret |= _dbread_table_devstate();
    return ret;
}

stDb_T_devices *_Stfind_Eqid(uint8_t *peqid, uint8_t eqlen)
{
    for(int x = 0; x < _CANT_MAX_EQ; x++)
        if(!memcmp(peqid, T_devices[x].eqid, eqlen))
            return &T_devices[x];
    
    return 0;
}

stDb_T_devices *_Stfind_Hubid(uint8_t *peqid, uint8_t eqlen)
{
    for(int x = 0; x < _CANT_MAX_EQ; x++)
        if(!memcmp(peqid, T_devices[x].eqid, eqlen))
            return &T_devices[x];
    
    return 0;
}


uint8_t *_Findtargetfolder(uint8_t *peqid, uint8_t eqlen)
{

    stDb_T_devices *pdev = _Stfind_Eqid(peqid, eqlen);

    if(pdev)
    {
        //printf("pdev->id %d\n", pdev->id);
        //printf("muni id %d\n", pdev->municipality_id);
        //stDb_T_municipalities *pmun = _Stfind_muni(pdev->municipality_id);
        /*
        if(pmun)
        {    
            printf("pmun->%d\n", pmun->id);
            return pmun->folder_name;
        }
        else*/ return 0;
    }
    else 
        return 0;
}

int8_t _CheckEq(uint8_t *peqid, uint8_t eqlen)
{
    stDb_T_devices * peq = _Stfind_Eqid(peqid, eqlen);
    if(peq) return SS_STS_ONLINE;
    else return SS_STS_ERR_NO_ALTA;
}

int8_t _CheckHubId(uint8_t *peqid, uint8_t eqlen)
{
    stDb_T_devices * peq = _Stfind_Hubid(peqid, eqlen);
    if(peq) return SS_STS_ONLINE;
    else return SS_STS_ERR_NO_ALTA;
}

/*
int main(void) 
{
    _dbread_table_version();
    _dbread_table_muni();
    _dbread_table_devices();
    printf("\n:->");
    for(int x=0;x<5;x++)
        if(T_devices[x].id)
        {
            for(int y=0;y<10;y++)
                printf("%02X", T_devices[x].eqid[y]);    
        }  

    printf("\n\n\n");

    for(int x=0;x<5;x++)
        if(T_municipalities[x].id)
        {
            printf("id:%d", T_municipalities[x].id);    
            printf("name:%s", T_municipalities[x].name);    
            printf("folder:%s", T_municipalities[x].folder_name);    
            printf("\n");
        }  

    return 0;
}*/

