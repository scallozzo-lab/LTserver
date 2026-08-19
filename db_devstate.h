#ifndef __DB_DEVSTATE_H__
    #define  __DB_DEVSTATE_H__

#include <stdint.h>
#include "db.h"


typedef struct
{
    char     light_id[51];

    int32_t  zone_id;

    double   lat;
    double   lng;

    char     status[16];

    int32_t  dimming_level;

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


#define _DB_DEVSTATE_NOCHANGE   0
#define _DB_DEVSTATE_CHANGED    1

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

int _dbread_devstate_size(void);
int _dbread_table_devstate(void);
int _dbwrite_devstate(stDb_T_devstate *pdev);
int _dbcheck_devstate(stDb_T_devstate_info *info);
void _InitDb_devstate(void);

#endif