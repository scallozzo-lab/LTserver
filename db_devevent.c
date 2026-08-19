#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "db_devevent.h"

const char *_DbDeviceEventTypeToString(eDbDeviceEventType type)
{
    switch (type)
    {
        case DEV_EVENT_POWER_FAILURE:
            return "power_failure";

        case DEV_EVENT_CIRCUIT_DISCONNECTED:
            return "circuit_disconnected";

        case DEV_EVENT_VOLTAGE_SPIKE:
            return "voltage_spike";

        case DEV_EVENT_OVERCURRENT:
            return "overcurrent";

        case DEV_EVENT_COMMUNICATION_LOST:
            return "communication_lost";

        case DEV_EVENT_LAMP_END_OF_LIFE:
            return "lamp_end_of_life";

        case DEV_EVENT_DRIVER_OVERHEAT:
            return "driver_overheat";

        case DEV_EVENT_BREAKER_TRIP:
            return "breaker_trip";

        default:
            return NULL;
    }
}

const char *_DbDeviceEventSeverityToString(eDbDeviceEventSeverity severity)
{
    switch (severity)
    {
        case DEV_SEVERITY_CRITICAL:
            return "critical";

        case DEV_SEVERITY_HIGH:
            return "high";

        case DEV_SEVERITY_MEDIUM:
            return "medium";

        case DEV_SEVERITY_LOW:
            return "low";

        default:
            return NULL;
    }
}

eDbDeviceEventType _DbDeviceEventTypeFromString(const char *str)
{
    if (str == NULL)
        return DEV_EVENT_POWER_FAILURE;

    if (strcmp(str, "power_failure") == 0)
        return DEV_EVENT_POWER_FAILURE;

    if (strcmp(str, "circuit_disconnected") == 0)
        return DEV_EVENT_CIRCUIT_DISCONNECTED;

    if (strcmp(str, "voltage_spike") == 0)
        return DEV_EVENT_VOLTAGE_SPIKE;

    if (strcmp(str, "overcurrent") == 0)
        return DEV_EVENT_OVERCURRENT;

    if (strcmp(str, "communication_lost") == 0)
        return DEV_EVENT_COMMUNICATION_LOST;

    if (strcmp(str, "lamp_end_of_life") == 0)
        return DEV_EVENT_LAMP_END_OF_LIFE;

    if (strcmp(str, "driver_overheat") == 0)
        return DEV_EVENT_DRIVER_OVERHEAT;

    if (strcmp(str, "breaker_trip") == 0)
        return DEV_EVENT_BREAKER_TRIP;

    return DEV_EVENT_POWER_FAILURE;
}


eDbDeviceEventSeverity _DbDeviceEventSeverityFromString(const char *str)
{
    if (str == NULL)
        return DEV_SEVERITY_CRITICAL;

    if (strcmp(str, "critical") == 0)
        return DEV_SEVERITY_CRITICAL;

    if (strcmp(str, "high") == 0)
        return DEV_SEVERITY_HIGH;

    if (strcmp(str, "medium") == 0)
        return DEV_SEVERITY_MEDIUM;

    if (strcmp(str, "low") == 0)
        return DEV_SEVERITY_LOW;

    return DEV_SEVERITY_CRITICAL;
}


int _dbread_deviceevent(int64_t id,
                        stDb_T_deviceevent *event)
{
    if (event == NULL)
        return _DB_STS_ERR_STRUCT;


    /*
     * Limpiamos la estructura
     */
    memset(event, 0, sizeof(stDb_T_deviceevent));


    /*
     * -------------------------------------------------
     * Conexión
     * -------------------------------------------------
     */

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbread_deviceevent] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_deviceevent] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    /*
     * -------------------------------------------------
     * ID
     * -------------------------------------------------
     */

    char id_str[32];

    snprintf(id_str,
             sizeof(id_str),
             "%lld",
             (long long)id);


    const char *paramValues[] =
    {
        id_str
    };


    /*
     * -------------------------------------------------
     * Query
     * -------------------------------------------------
     */

    const char *query =
        "SELECT "
        "id, "
        "light_id, "
        "event_code, "
        "event_type, "
        "severity, "
        "message, "
        "zone_id, "
        "theoretical_kw, "
        "real_kw, "
        "voltage_v, "
        "current_ma, "
        "resolved, "
        "created_at "
        "FROM deviceevent "
        "WHERE id = $1";


    PGresult *res = PQexecParams(
        conn,
        query,
        1,
        NULL,
        paramValues,
        NULL,
        NULL,
        0
    );


    /*
     * -------------------------------------------------
     * Verificar resultado
     * -------------------------------------------------
     */

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbread_deviceevent] Query failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_deviceevent] Query failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    int rows = PQntuples(res);


    /*
     * ID inexistente
     */
    if (rows == 0)
    {
        printf("[_dbread_deviceevent] Event not found: %lld\n",
               (long long)id);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * No debería ocurrir porque id es PRIMARY KEY
     */
    if (rows != 1)
    {
        printf("[_dbread_deviceevent] Unexpected rows: %d\n",
               rows);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * -------------------------------------------------
     * Verificar columnas
     * -------------------------------------------------
     */

    int cols = PQnfields(res);

    if (cols != 13)
    {
        printf("[_dbread_deviceevent] Error! Columns (%d)\n",
               cols);

        _log("[_dbread_deviceevent] Error! Columns (%d)\n",
             cols);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * -------------------------------------------------
     * Cargar estructura
     * -------------------------------------------------
     */

    /* id */
    if (!PQgetisnull(res, 0, 0))
    {
        event->id =
            atoll(PQgetvalue(res, 0, 0));
    }


    /* light_id */
    if (!PQgetisnull(res, 0, 1))
    {
        strncpy(event->light_id,
                PQgetvalue(res, 0, 1),
                sizeof(event->light_id) - 1);
    }


    /* event_code */
    if (!PQgetisnull(res, 0, 2))
    {
        strncpy(event->event_code,
                PQgetvalue(res, 0, 2),
                sizeof(event->event_code) - 1);
    }


    /*
     * event_type
     *
     * PostgreSQL -> ENUM C
     */
    if (!PQgetisnull(res, 0, 3))
    {
        event->event_type =
            _DbDeviceEventTypeFromString(
                PQgetvalue(res, 0, 3));
    }


    /*
     * severity
     *
     * PostgreSQL -> ENUM C
     */
    if (!PQgetisnull(res, 0, 4))
    {
        event->severity =
            _DbDeviceEventSeverityFromString(
                PQgetvalue(res, 0, 4));
    }


    /* message */
    if (!PQgetisnull(res, 0, 5))
    {
        strncpy(event->message,
                PQgetvalue(res, 0, 5),
                sizeof(event->message) - 1);
    }


    /* zone_id */
    if (!PQgetisnull(res, 0, 6))
    {
        event->zone_id =
            atoi(PQgetvalue(res, 0, 6));
    }


    /* theoretical_kw */
    if (!PQgetisnull(res, 0, 7))
    {
        event->theoretical_kw =
            atof(PQgetvalue(res, 0, 7));
    }


    /* real_kw */
    if (!PQgetisnull(res, 0, 8))
    {
        event->real_kw =
            atof(PQgetvalue(res, 0, 8));
    }


    /* voltage_v */
    if (!PQgetisnull(res, 0, 9))
    {
        event->voltage_v =
            atof(PQgetvalue(res, 0, 9));
    }


    /* current_ma */
    if (!PQgetisnull(res, 0, 10))
    {
        event->current_ma =
            atof(PQgetvalue(res, 0, 10));
    }


    /* resolved */
    if (!PQgetisnull(res, 0, 11))
    {
        event->resolved =
            (strcmp(PQgetvalue(res, 0, 11), "t") == 0);
    }


    /* created_at */
    if (!PQgetisnull(res, 0, 12))
    {
        strncpy(event->created_at,
                PQgetvalue(res, 0, 12),
                sizeof(event->created_at) - 1);
    }


#ifdef _DEBUG_DB_READ

    printf("\n[_dbread_deviceevent]\n");

    printf("  id             : %lld\n",
           (long long)event->id);

    printf("  light_id       : %s\n",
           event->light_id);

    printf("  event_code     : %s\n",
           event->event_code);

    printf("  event_type     : %s\n",
           _DbDeviceEventTypeToString(event->event_type));

    printf("  severity       : %s\n",
           _DbDeviceEventSeverityToString(event->severity));

    printf("  message        : %s\n",
           event->message);

    printf("  zone_id        : %d\n",
           event->zone_id);

    printf("  theoretical_kw : %.3f\n",
           event->theoretical_kw);

    printf("  real_kw        : %.3f\n",
           event->real_kw);

    printf("  voltage_v      : %.2f\n",
           event->voltage_v);

    printf("  current_ma     : %.2f\n",
           event->current_ma);

    printf("  resolved       : %d\n",
           event->resolved);

    printf("  created_at     : %s\n",
           event->created_at);

#endif


    /*
     * -------------------------------------------------
     * Liberar recursos
     * -------------------------------------------------
     */

    PQclear(res);
    PQfinish(conn);

    return _DB_STS_OK;
}


int _dbinsert_deviceevent(stDb_T_deviceevent *event)
{
    if (event == NULL)
        return _DB_STS_ERR_STRUCT;


    /*
     * -------------------------------------------------
     * Convertir ENUM de C a texto PostgreSQL
     * -------------------------------------------------
     */

    const char *event_type =
        _DbDeviceEventTypeToString(event->event_type);

    const char *severity =
        _DbDeviceEventSeverityToString(event->severity);


    /*
     * Verificar que los valores sean válidos
     */
    if ((event_type == NULL) || (severity == NULL))
    {
        printf("[_dbinsert_deviceevent] Invalid event type/severity\n");

        _log("[_dbinsert_deviceevent] Invalid event type/severity\n");

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * -------------------------------------------------
     * Conexión a PostgreSQL
     * -------------------------------------------------
     */

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbinsert_deviceevent] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbinsert_deviceevent] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    /*
     * -------------------------------------------------
     * Conversión de valores numéricos
     * -------------------------------------------------
     */

    char zone_id[16];
    char theoretical_kw[32];
    char real_kw[32];
    char voltage_v[32];
    char current_ma[32];


    snprintf(zone_id,
             sizeof(zone_id),
             "%d",
             event->zone_id);

    snprintf(theoretical_kw,
             sizeof(theoretical_kw),
             "%.3f",
             event->theoretical_kw);

    snprintf(real_kw,
             sizeof(real_kw),
             "%.3f",
             event->real_kw);

    snprintf(voltage_v,
             sizeof(voltage_v),
             "%.2f",
             event->voltage_v);

    snprintf(current_ma,
             sizeof(current_ma),
             "%.2f",
             event->current_ma);


    /*
     * BOOLEAN PostgreSQL
     */
    const char *resolved =
        event->resolved ? "true" : "false";


    /*
     * -------------------------------------------------
     * Parámetros
     *
     * $1  light_id
     * $2  event_code
     * $3  event_type
     * $4  severity
     * $5  message
     * $6  zone_id
     * $7  theoretical_kw
     * $8  real_kw
     * $9  voltage_v
     * $10 current_ma
     * $11 resolved
     * -------------------------------------------------
     */

    const char *paramValues[] =
    {
        event->light_id,
        event->event_code,
        event_type,
        severity,
        event->message,
        zone_id,
        theoretical_kw,
        real_kw,
        voltage_v,
        current_ma,
        resolved
    };


    /*
     * -------------------------------------------------
     * INSERT
     *
     * id         -> BIGSERIAL, PostgreSQL lo genera
     * created_at -> DEFAULT NOW()
     *
     * RETURNING id nos devuelve el ID generado.
     * -------------------------------------------------
     */

    const char *query =
        "INSERT INTO deviceevent "
        "("
        "light_id, "
        "event_code, "
        "event_type, "
        "severity, "
        "message, "
        "zone_id, "
        "theoretical_kw, "
        "real_kw, "
        "voltage_v, "
        "current_ma, "
        "resolved"
        ") "
        "VALUES "
        "("
        "$1, "
        "$2, "
        "$3, "
        "$4, "
        "$5, "
        "$6, "
        "$7, "
        "$8, "
        "$9, "
        "$10, "
        "$11"
        ") "
        "RETURNING id";


    PGresult *res = PQexecParams(
        conn,
        query,
        11,
        NULL,
        paramValues,
        NULL,
        NULL,
        0
    );


    /*
     * -------------------------------------------------
     * Verificar resultado
     * -------------------------------------------------
     */

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbinsert_deviceevent] INSERT failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbinsert_deviceevent] INSERT failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    /*
     * -------------------------------------------------
     * Obtener ID generado
     * -------------------------------------------------
     */

    if (PQntuples(res) != 1)
    {
        printf("[_dbinsert_deviceevent] Invalid returned rows\n");

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    event->id =
        atoll(PQgetvalue(res, 0, 0));


#ifdef _DEBUG_DB_READ

    printf("\n[_dbinsert_deviceevent] Event inserted\n");

    printf("  id             : %lld\n",
           (long long)event->id);

    printf("  light_id       : %s\n",
           event->light_id);

    printf("  event_code     : %s\n",
           event->event_code);

    printf("  event_type     : %s\n",
           event_type);

    printf("  severity       : %s\n",
           severity);

    printf("  message        : %s\n",
           event->message);

#endif


    /*
     * -------------------------------------------------
     * Liberar recursos
     * -------------------------------------------------
     */

    PQclear(res);
    PQfinish(conn);

    return _DB_STS_OK;
}


#ifdef _TEST_DB_DEVEVENT

int main(void)
{
    stDb_T_deviceevent event;


    int ret = _dbread_deviceevent(35, &event);

    if (ret == _DB_STS_OK)
    {
        printf("Evento encontrado\n");
        printf("ID       : %lld\n", (long long)event.id);
        printf("Luminaria: %s\n", event.light_id);
        printf("Tipo     : %s\n", _DbDeviceEventTypeToString(event.event_type));
        printf("Severidad: %s\n", _DbDeviceEventSeverityToString(event.severity));
        printf("Mensaje  : %s\n", event.message);
    }
    else
    {
        printf("No se pudo leer el evento\n");
    }

    memset(&event, 0, sizeof(event));

    strcpy(event.light_id, "111");
    strcpy(event.event_code, "OVERTEMP");

    event.event_type = DEV_EVENT_OVERCURRENT;
    event.severity = DEV_SEVERITY_HIGH;

    strcpy(event.message,
        "Temperatura de luminaria superior al limite");

    event.zone_id = 5;

    event.theoretical_kw = 0.150;
    event.real_kw = 0.132;

    event.voltage_v = 24.00;
    event.current_ma = 5500.00;

    event.resolved = 0;


    ret = _dbinsert_deviceevent(&event);

    if (ret == _DB_STS_OK)
    {
        printf("Evento creado correctamente\n");
        printf("ID asignado por PostgreSQL: %lld\n",
            (long long)event.id);
    }
    else
    {
        printf("Error creando evento\n");
    }
}
#endif