
#ifndef DB_DEVCALENDAR_H
    #define DB_DEVDECALENDAR_H

#include <stdint.h>
#include "db.h"

typedef struct
{
    char light_id[51];

    stCalendarEvent CalendarList[_MAXCALENDARLST];

} stDb_T_devcalendar;


typedef struct
{
    int  count;
    char last_update[32];

} stDb_T_devcalendar_info;

void _InitDb_devcalendar(void);
int _dbread_table_devcalendar(void);
stDb_T_devcalendar *_Stfind_Devcalendar(const char *light_id);

#endif