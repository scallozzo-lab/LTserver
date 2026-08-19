#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"

stDb_T_db_version T_db_version;
stDb_T_sector T_sector[_CANT_MAX_EQ];
stDb_T_devices T_devices[_CANT_MAX_EQ];

// Definición de nombres de campos para control de layout de DB
const int8_t* T_muni_names[] = {
                                "id", 
                                "cid", 
                                "pid",
                                "sector_id",
                                "sector_name",
                                "folder_name",
                                "is_public_sector",
                                "is_geopos",
                                "map_file"
                                };


const int8_t* T_dev_names[] = {
                                "id",	
                                "eqid",	
                                "linkid",
                                "hubid",
                                "_uid",
                                "date_time",
                                "devtype",
                                "serial_number",
                                "_name",
                                "model",
                                "address",
                                "address_number",
                                "intersection",
                                "zipcode",
                                "mapposx",
                                "mapposy",
                                "geo_latitude",
                                "geo_longitude",
                                "description",
                                "sector_id",
                                "map_loc",
                                "map_pag",
                                "is_enabled",
                                "is_linked",
                                "devconfig"
};


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
    memset((void*)&T_db_version, 0, sizeof(T_db_version));
    memset((void*)T_sector, 0, sizeof(T_sector));
    memset((void*)T_devices, 0, sizeof(T_devices));
    int r1 = _dbread_table_tableschema_sector();
    int r2 = _dbread_table_tableschema_dev(); 
    if (r1 == _DB_STS_ERR_DBLAYOUT || r2 == _DB_STS_ERR_DBLAYOUT) ret = _DB_STS_ERR_DBLAYOUT;
    else if(r1) ret = r1;
    else if(r2) ret = r2;
    return ret;
}

/*
  Compara el layout de la tabla 'sector'
*/
int _dbread_table_tableschema_sector(void) 
{
    int ret = _DB_STS_OK;
    
    // Connect to the database
    PGconn *conn = PQconnectdb(_DB_ADDRESS);
    if (PQstatus(conn) != CONNECTION_OK) 
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return _DB_STS_ERR_CONN;
    }

    // Execute SQL query
    PGresult *res = PQexec(conn, "SELECT column_name FROM information_schema.columns WHERE table_schema = 'public' AND table_name = 'sector'");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        fprintf(stderr, "Query execution failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return _DB_STS_ERR_CMD;
    }

    // Print the query result
    int rows = PQntuples(res);
    if(rows != _MAXCOLSDBSECTOR)
    {
        printf("[_dbread_table_tableschema_sector] Error! MAX(%d)\r\n", rows);
        _log("[_dbread_table_tableschema_sector] Error! MAX(%d)\r\n", rows);
        ret = _DB_STS_ERR_STRUCT;    
    } 
    else
    {

#ifdef _DEBUG_DB_READ
        printf("ESQUEMA TABLA sector:\n");
#endif        
        int cols = PQnfields(res);
        if(cols == 1)
            for (int i = 0; i < rows; i++) 
            {
                for (int j = 0; j < cols; j++) 
                {
#ifdef _DEBUG_DB_READ
                    printf("%s\t", PQgetvalue(res, i, j));
#endif                
                    if(memcmp(T_muni_names[i], PQgetvalue(res, i, j), strlen(T_muni_names[i])))
                    {
                        printf("\n[_dbread_table_tableschema_sector] Error-> %s-%s\n", T_muni_names[i], PQgetvalue(res, i, j));    
                        _log("[_dbread_table_tableschema_sector] Error-> %s-%s\n", T_muni_names[i], PQgetvalue(res, i, j));    
                        ret = _DB_STS_ERR_DBLAYOUT;
                    } 
                }
#ifdef _DEBUG_DB_READ                
                printf("\n");
#endif            
            }
        else 
        {
            printf("[_dbread_table_tableschema_sector] Error! (%d)\r\n", cols);
            _log("[_dbread_table_tableschema_sector] Error! (%d)\r\n", cols);
            ret = _DB_STS_ERR_STRUCT;
        }
    }       
    
    // Clean up
    PQclear(res);
    PQfinish(conn);

    return ret;
}

int _dbread_table_tableschema_dev(void) 
{
    int ret = _DB_STS_OK;

    // Connect to the database
    PGconn *conn = PQconnectdb(_DB_ADDRESS);
    if (PQstatus(conn) != CONNECTION_OK) 
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return _DB_STS_ERR_CONN;
    }

    // Execute SQL query
    PGresult *res = PQexec(conn, "SELECT column_name FROM information_schema.columns WHERE table_schema = 'public' AND table_name = 'devices'");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        fprintf(stderr, "Query execution failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return _DB_STS_ERR_CMD;
    }

    // Print the query result
    int rows = PQntuples(res);
    if(rows != _MAXCOLSDBDEVICES)
    {
        printf("[_dbread_table_tableschema_devices] Error! MAX(%d)\r\n", rows);
        _log("[_dbread_table_tableschema_devices] Error! MAX(%d)\r\n", rows);
        ret = _DB_STS_ERR_STRUCT;    
    } 
    else
    {

#ifdef _DEBUG_DB_READ
        printf("ESQUEMA TABLA devices:\n");
#endif        
        int cols = PQnfields(res);
        if(cols == 1)
            for (int i = 0; i < rows; i++) 
            {
                for (int j = 0; j < cols; j++) 
                {
#ifdef _DEBUG_DB_READ
                    printf("%s\t", PQgetvalue(res, i, j));
#endif                
                    if(memcmp(T_dev_names[i], PQgetvalue(res, i, j), strlen(T_dev_names[i])))
                    {
                        printf("\n[_dbread_table_tableschema_dev] Error-> %s-%s\n", T_dev_names[i], PQgetvalue(res, i, j));    
                        _log("[_dbread_table_tableschema_dev] Error-> %s-%s\n", T_dev_names[i], PQgetvalue(res, i, j));    
                        ret = _DB_STS_ERR_DBLAYOUT;
                    } 
                }
#ifdef _DEBUG_DB_READ                
                printf("\n");
#endif            
            }
        else 
        {
            printf("[_dbread_table_tableschema_dev] Error! (%d)\r\n", cols);
            _log("[_dbread_table_tableschema_dev] Error! (%d)\r\n", cols);
            ret = _DB_STS_ERR_STRUCT;
        }
    }       
    
    // Clean up
    PQclear(res);
    PQfinish(conn);

    return ret;
}


int _dbread_table_version(void) 
{
    int ret = 0;
    // Connect to the database
    PGconn *conn = PQconnectdb(_DB_ADDRESS);
    if (PQstatus(conn) != CONNECTION_OK) 
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return _DB_STS_ERR_CONN;
    }

    // Execute SQL query
    PGresult *res = PQexec(conn, "SELECT * FROM db_version");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        fprintf(stderr, "Query execution failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return _DB_STS_ERR_CMD;
    }

    // Print the query result
    int rows = PQntuples(res);
    if(rows == 1)
    {
        int cols = PQnfields(res);
        if(cols == _MAXCOLSDBVERSION)
            for (int i = 0; i < rows; i++) 
            {
                for (int j = 0; j < cols; j++) 
                {
#ifdef _DEBUG_DB_READ
                    printf("%s\t", PQgetvalue(res, i, j));
#endif                
                }
#ifdef _DEBUG_DB_READ                
                printf("\n");
#endif            
            }
        else 
        {
            printf("[_dbread_table_version] Error! (%d)\r\n", cols);
            ret = _DB_STS_ERR_STRUCT;
        }
    }       
    else
    {
        printf("[_dbread_table_version] Error! = Multi-Version (%d)\r\n", rows);
        ret = _DB_STS_ERR_STRUCT;
    }
    // Clean up
    PQclear(res);
    PQfinish(conn);

    return ret;
}

int _dbread_table_sector(void) 
{
    int ret = 0;
    
    // Connect to the database
    PGconn *conn = PQconnectdb(_DB_ADDRESS);
    if (PQstatus(conn) != CONNECTION_OK) 
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    // Execute SQL query
    PGresult *res = PQexec(conn, "SELECT * FROM sector");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        fprintf(stderr, "Query execution failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return _DB_STS_ERR_CMD;
    }

    // Print the query result
    int rows = PQntuples(res);
    if(rows > _CANT_MAX_EQ)
    {
        printf("[_dbread_table_sector] Error! MAX(%d)\r\n", rows);
        ret = 1;    
    } 
    else
    {
        int cols = PQnfields(res);
        if(cols == _MAXCOLSDBSECTOR)
            for (int i = 0; i < rows; i++) 
            {
                for (int j = 0; j < cols; j++) 
                {
#ifdef _DEBUG_DB_READ
                    printf("%s\t", PQgetvalue(res, i, j));
#endif
                    switch(j)
                    {
                        case 0:
                            T_sector[i].id = atoi(PQgetvalue(res, i, j));
                        break;
                        
                        case 1:
                            T_sector[i].cid = atoi(PQgetvalue(res, i, j));
                        break;

                        case 2:
                            T_sector[i].pid = atoi(PQgetvalue(res, i, j));
                        break;

                        case 3:
                            T_sector[i].sector_id = atoi(PQgetvalue(res, i, j));
                        break;
                        
                        // sector_name
                        case 4:
                            strncpy(T_sector[i].sector_name, PQgetvalue(res, i, j), sizeof(T_sector[i].sector_name) - 1); 
                        break;

                        // folder_name
                        case 5:
                            strncpy(T_sector[i].folder_name, PQgetvalue(res, i, j), sizeof(T_sector[i].folder_name) - 1); 
                        break;

                        // is_public_sector
                        case 6:
                            T_sector[i].is_public_sector = atoi(PQgetvalue(res, i, j));
                        break;
                        
                        // is_geopos
                        case 7:
                            T_sector[i].is_geopos = atoi(PQgetvalue(res, i, j));
                        break;

                        // map_file
                        case 8:
                            strncpy(T_sector[i].map_file, PQgetvalue(res, i, j), sizeof(T_sector[i].map_file) - 1); 
                        break;

                        default:
                            printf("[_dbread_table_sector] Warning: Unhandled col %d\n",j);
                    }
                }
#ifdef _DEBUG_DB_READ
                printf("\n");
#endif            
            }
        else 
        {
            printf("[_dbread_table_sector] Error! (%d)\r\n", cols);
            ret = 1;
        }    
    }
    // Clean up
    PQclear(res);
    PQfinish(conn);

    return ret;
}

int _dbread_table_devices(void) 
{
    int ret = 0;
    
    // Connect to the database
    PGconn *conn = PQconnectdb(_DB_ADDRESS);
    if (PQstatus(conn) != CONNECTION_OK) 
    {
        fprintf(stderr, "Connection to database failed: %s", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    // Execute SQL query
    PGresult *res = PQexec(conn, "SELECT * FROM devices");
    if (PQresultStatus(res) != PGRES_TUPLES_OK) 
    {
        fprintf(stderr, "Query execution failed: %s", PQerrorMessage(conn));
        PQclear(res);
        PQfinish(conn);
        return 1;
    }

    // Print the query result
    int rows = PQntuples(res);
       if(rows > _CANT_MAX_EQ)
    {
        printf("[_dbread_table_devices] Error! MAX(%d)\r\n", rows);
        ret = 1;    
    } 
    else
    {
        int cols = PQnfields(res);
        if(cols == _MAXCOLSDBDEVICES)
            for (int i = 0; i < rows; i++) 
            {
#ifdef _DEBUG_DB_READ
                printf("row nr:%d\n",i + 1);
#endif
                for (int j = 0; j < cols; j++) 
                {
#ifdef _DEBUG_DB_READ
                    printf("(col) %d (%s) -> %s\n", j, T_dev_names[j], PQgetvalue(res, i, j));
#endif                
                    switch(j)
                    {
                        // id
                        case 0:
                        T_devices[i].id = atoi(PQgetvalue(res, i, j));
                        break;
                        
                        // eqid
                        // linkid
                        // hubid
                        case 1:
                        case 2:
                        case 3:
                        {
                            uint8_t tmpcad[32], tmpcad2[32];
                            memset(tmpcad, 0, sizeof(tmpcad));
                            memset(tmpcad2, 0, sizeof(tmpcad2));
                            memcpy(tmpcad, PQgetvalue(res, i, j), sizeof(tmpcad));
                        
                            for(int x = 0, i = 0;x < sizeof(tmpcad);x++)
                                if(tmpcad[x] != ':') tmpcad2[i++] = tmpcad[x];
                            
                            if(j == 1)
                                Hex2Bin(tmpcad2, T_devices[i].eqid, sizeof(T_devices[i].eqid) * 2);
                            else if(j == 2)
                                Hex2Bin(tmpcad2, T_devices[i].linkid, sizeof(T_devices[i].linkid) * 2);
                            else if(j == 3)
                                Hex2Bin(tmpcad2, T_devices[i].hubid, sizeof(T_devices[i].hubid) * 2);
                        }
                        break;
                        
                        // _uid
                        case 4:
                            strncpy(T_devices[i]._iud, PQgetvalue(res, i, j), sizeof(T_devices[i]._iud) - 1);
                        break;

                        // date_time (limitado a 19 caracteres "2025-07-12 22:24:56")
                        case 5:
                            strncpy(T_devices[i].creation_date, PQgetvalue(res, i, j), sizeof(T_devices[i].creation_date) - 1);
                        break;

                        // devtype
                        case 6:
                            T_devices[i].devtype = atoi(PQgetvalue(res, i, j));
                        break;

                        // serial_number
                        case 7:
                            strncpy(T_devices[i].serial_number, PQgetvalue(res, i, j), sizeof(T_devices[i].serial_number) - 1);
                        break;

                        // _name
                        case 8:
                            strncpy(T_devices[i]._name, PQgetvalue(res, i, j), sizeof(T_devices[i]._name) - 1);
                        break;

                        // model
                        case 9:
                            strncpy(T_devices[i].model, PQgetvalue(res, i, j), sizeof(T_devices[i].model) - 1);   
                        break;

                        // address
                        case 10:
                            strncpy(T_devices[i].address, PQgetvalue(res, i, j), sizeof(T_devices[i].address) - 1);     
                        break;

                        // address_number
                        case 11:
                            strncpy(T_devices[i].address_number, PQgetvalue(res, i, j), sizeof(T_devices[i].address_number) - 1);     
                        break;

                        // intersection
                        case 12:
                            strncpy(T_devices[i].intersection, PQgetvalue(res, i, j), sizeof(T_devices[i].intersection) - 1);      
                        break;

                        // zipcode
                        case 13:
                            strncpy(T_devices[i].zipcode, PQgetvalue(res, i, j), sizeof(T_devices[i].zipcode) - 1);         
                        break;

                        // mapposx
                        case 14:
                            T_devices[i].mapposx = atoi(PQgetvalue(res, i, j));
                        break;

                        // mapposy
                        case 15:
                            T_devices[i].mapposy = atoi(PQgetvalue(res, i, j));
                        break;

                        // geo_latitude
                        case 16:
                            T_devices[i].geo_latitude = atof(PQgetvalue(res, i, j));
                        break;

                        // geo_longitude
                        case 17:
                            T_devices[i].geo_longitude = atof(PQgetvalue(res, i, j));
                        break;

                        // description
                        case 18:
                            strncpy(T_devices[i].description, PQgetvalue(res, i, j), sizeof(T_devices[i].description) - 1);         
                        break;

                        // sector_id
                        case 19:
                            T_devices[i].sector_id = atoi(PQgetvalue(res, i, j));
                        break;

                        // map_loc
                        case 20:
                            strncpy(T_devices[i].map_loc, PQgetvalue(res, i, j), sizeof(T_devices[i].map_loc) - 1);           
                        break;

                        // map_pag
                        case 21:
                            T_devices[i].map_pag = atoi(PQgetvalue(res, i, j));
                        break;

                        // is_enabled
                        case 22:
                            T_devices[i].is_enabled = atoi(PQgetvalue(res, i, j));
                        break;

                        // is_linked
                        case 23:
                            T_devices[i].is_linked = atoi(PQgetvalue(res, i, j));
                        break;

                        // devconfig
                        case 24:
                            T_devices[i].devconfig = atoi(PQgetvalue(res, i, j));
                        break;

                        default:
                            printf("[_dbread_table_devices] Warning: Unhandled col %d\n",j);

                    }       
                }
#ifdef _DEBUG_DB_READ
                printf("\n");
#endif            
            }
        else 
        {
            printf("[_dbread_table_devices] Error! (%d)\r\n", cols);
            ret = 1;
        }    
    }
    // Clean up
    PQclear(res);
    PQfinish(conn);

    return ret;
}

int _dbread_bulk(void)
{
    int ret = 0;
    
    ret |= _dbread_table_version();
    ret |= _dbread_table_sector(); 
    ret |= _dbread_table_devices();
   
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

/*
stDb_T_municipalities *_Stfind_muni(uint32_t mid)
{
    for(int x = 0; x < _CANT_MAX_EQ; x++)
        if((T_municipalities[x].id == mid) && mid)
            return &T_municipalities[x];
    
    return 0;
}
*/

uint8_t *_Findtargetfolder(uint8_t *peqid, uint8_t eqlen)
{

    stDb_T_devices *pdev = _Stfind_Eqid(peqid, eqlen);

    if(pdev)
    {
        printf("pdev->id %d\n", pdev->id);
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

