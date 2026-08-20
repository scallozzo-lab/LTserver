#ifndef DB_DEVICES_H
    #define DB_DEVICES_H

#include <stdbool.h>
#include "db.h"

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


extern stDb_T_devices T_devices[_CANT_MAX_EQ];


int _dbread_devices_size(void);

stDb_T_devices *_Stfind_Devices(const char *light_id);

stDb_T_devices *_Stfind_Devices_ByEqid(const char *eqid);

stDb_T_devices *_Stfind_Devices_ByZone(int zone_id);

int _dbread_table_devices(void);

void _InitDb_devices(void);


#endif