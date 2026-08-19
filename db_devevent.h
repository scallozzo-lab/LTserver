#ifndef DB_DEVICEEVENT_H
    #define DB_DEVICEEVENT_H

#include <stdint.h>
#include "db.h"

typedef enum
{
    DEV_EVENT_POWER_FAILURE = 0,
    DEV_EVENT_CIRCUIT_DISCONNECTED,
    DEV_EVENT_VOLTAGE_SPIKE,
    DEV_EVENT_OVERCURRENT,
    DEV_EVENT_COMMUNICATION_LOST,
    DEV_EVENT_LAMP_END_OF_LIFE,
    DEV_EVENT_DRIVER_OVERHEAT,
    DEV_EVENT_BREAKER_TRIP

} eDbDeviceEventType;


typedef enum
{
    DEV_SEVERITY_CRITICAL = 0,
    DEV_SEVERITY_HIGH,
    DEV_SEVERITY_MEDIUM,
    DEV_SEVERITY_LOW

} eDbDeviceEventSeverity;


typedef struct
{
    int64_t id;

    char light_id[51];

    char event_code[51];

    eDbDeviceEventType event_type;

    eDbDeviceEventSeverity severity;

    char message[256];

    int32_t zone_id;

    double theoretical_kw;
    double real_kw;

    double voltage_v;
    double current_ma;

    int resolved;

    char created_at[20];

} stDb_T_deviceevent;



extern enum eDbDeviceEventSeverity;


int _dbread_deviceevent(int64_t id,
                        stDb_T_deviceevent *event);

int _dbinsert_deviceevent(stDb_T_deviceevent *event);

#endif