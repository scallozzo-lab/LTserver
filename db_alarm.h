#ifndef DB_ALARM_H
    #define DB_ALARM_H

#include <stdint.h>
#include "db.h"
#include "db_devevent.h"
/*
 * Tipo de alarma
 */
typedef enum
{
    DB_ALARM_LAMP_FAULT = 0,
    DB_ALARM_DRIVER_FAULT,
    DB_ALARM_COMMUNICATION_LOST,
    DB_ALARM_OVER_TEMPERATURE,
    DB_ALARM_VOLTAGE_ANOMALY,
    DB_ALARM_POWER_OUTAGE

} eDbAlarmType;


/*
 * Estructura alarm
 */
typedef struct
{
    int64_t id;

    char light_id[51];

    char street_name[256];

    int zone_id;

    eDbAlarmType alarm_type;

    eDbDeviceEventSeverity severity;

    char message[512];

    uint8_t acknowledged;

    char acknowledged_by[101];

    char acknowledged_at[32];

    uint8_t resolved;

    char resolved_at[32];

    char created_at[32];

} stDb_T_alarm;


/*
 * Lectura de una alarma por ID
 */
int _dbread_alarm(int64_t id,
                  stDb_T_alarm *alarm);

                  
int _dbinsert_alarm(stDb_T_alarm *alarm);


/*
 * Conversión ENUM PostgreSQL -> ENUM C
 */
eDbAlarmType _dbalarm_type_from_string(const char *str);


/*
 * Conversión ENUM C -> PostgreSQL
 */
const char *_dbalarm_type_to_string(eDbAlarmType type);

#endif

