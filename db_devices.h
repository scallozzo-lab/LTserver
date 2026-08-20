#ifndef __DB_DEVICES_H__
    #define __DB_DEVICES_H__

#include <stdbool.h>
//#include "db.h"

#define _DB_DEVICES_CHANGED   1
#define _DB_DEVICES_NOCHANGE  0

typedef struct
{
    char light_id[51];

    char eqid[18];
    char linkid[18];
    char hubid[18];

    char date_time[32];

    int16_t devtype;

    char serial_number[33];

    char _name[33];

    char model[17];

    char address[21];

    char address_number[11];

    char intersection[33];

    char zipcode[17];

    double lat;
    double lng;

    char description[33];

    int32_t zone_id;

    char map_loc[33];

    int16_t map_pag;

    bool is_enabled;
    bool is_linked;

    int devconfig;

} stDb_T_devices;


typedef struct
{
    int count;
    char last_update[32];

} stDb_T_devices_info;


extern stDb_T_devices T_devices[];
extern stDb_T_devices_info devices_info;


int _dbread_devices_size(void);
int _dbcheck_devices(stDb_T_devices_info *info);

stDb_T_devices *_Stfind_Devices(const char *light_id);

//stDb_T_devices *_Stfind_Devices_ByEqid(const char *eqid);
stDb_T_devices *_Stfind_Devices_ByEqid(const char *eqid, uint8_t type);

stDb_T_devices *_Stfind_Devices_ByZone(int zone_id);

int _dbread_table_devices(void);

void _InitDb_devices(void);


#endif