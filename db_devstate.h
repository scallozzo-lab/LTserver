#ifndef __DB_DEVSTATE_H__
    #define  __DB_DEVSTATE_H__

#include <stdint.h>
#include "db.h"

#define _DB_DEVSTATE_NOCHANGE   0
#define _DB_DEVSTATE_CHANGED    1

typedef struct
{
    bool     enable;

    uint8_t  r;
    uint8_t  g;
    uint8_t  b;

} stRgbGroup;

typedef struct
{
    char     light_id[51];

    int32_t  zone_id;

    double   lat;
    double   lng;

    char     status[16];

    int32_t  dimming_level;

    /*
     * RGB Groups
     */
    stRgbGroup rgb[3];
    int32_t devtype;
    int32_t mode;
    char    device_date_time[32];
    int32_t auto_program;

    double   power_watts;
    double   voltage;
    double   temperature_c;

    int64_t  burn_hours;

    char     last_seen[20];

    char     street_name[256];

    char     lamp_type[32];

    double   rated_watts;

    char     created_at[20];
    char     updated_at[20];

} stDb_T_devstate;


typedef struct
{
    int count;
    char last_update[32];

} stDb_T_devstate_info;


/*
 * Consulta COUNT(*) y MAX(updated_at)
 * y determina si devstate cambió.
 */
int _dbcheck_devstate(stDb_T_devstate_info *info);


extern stDb_T_devstate T_devstate[_CANT_MAX_EQ];
extern stDb_T_devstate_info devstate_info;


int _dbread_devstate_size(void);
int _dbread_table_devstate(void);
int _dbwrite_devstate(stDb_T_devstate *pdev);
int _dbcheck_devstate(stDb_T_devstate_info *info);
void _InitDb_devstate(void);
stDb_T_devstate *_Stfind_Devstate(const char *light_id);

#endif