
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

#include "db_alarm.h"
#include "db.h"


/*
 * =========================================================
 * alarm_type
 * =========================================================
 */

eDbAlarmType _dbalarm_type_from_string(const char *str)
{
    if (str == NULL)
        return DB_ALARM_LAMP_FAULT;

    if (strcmp(str, "lamp_fault") == 0)
        return DB_ALARM_LAMP_FAULT;

    if (strcmp(str, "driver_fault") == 0)
        return DB_ALARM_DRIVER_FAULT;

    if (strcmp(str, "communication_lost") == 0)
        return DB_ALARM_COMMUNICATION_LOST;

    if (strcmp(str, "over_temperature") == 0)
        return DB_ALARM_OVER_TEMPERATURE;

    if (strcmp(str, "voltage_anomaly") == 0)
        return DB_ALARM_VOLTAGE_ANOMALY;

    if (strcmp(str, "power_outage") == 0)
        return DB_ALARM_POWER_OUTAGE;


    /*
     * Valor por defecto
     */
    return DB_ALARM_LAMP_FAULT;
}


const char *_dbalarm_type_to_string(eDbAlarmType type)
{
    switch (type)
    {
        case DB_ALARM_LAMP_FAULT:
            return "lamp_fault";

        case DB_ALARM_DRIVER_FAULT:
            return "driver_fault";

        case DB_ALARM_COMMUNICATION_LOST:
            return "communication_lost";

        case DB_ALARM_OVER_TEMPERATURE:
            return "over_temperature";

        case DB_ALARM_VOLTAGE_ANOMALY:
            return "voltage_anomaly";

        case DB_ALARM_POWER_OUTAGE:
            return "power_outage";

        default:
            return "lamp_fault";
    }
}


/*
 * =========================================================
 * severity
 * =========================================================
 */

eDbDeviceEventSeverity _dbalarm_severity_from_string(const char *str)
{
    if (str == NULL)
        return DEV_SEVERITY_LOW;


    if (strcmp(str, "critical") == 0)
        return DEV_SEVERITY_CRITICAL;

    if (strcmp(str, "high") == 0)
        return DEV_SEVERITY_HIGH;

    if (strcmp(str, "medium") == 0)
        return DEV_SEVERITY_MEDIUM;

    if (strcmp(str, "low") == 0)
        return DEV_SEVERITY_LOW;


    return DEV_SEVERITY_LOW;
}


const char *_dbalarm_severity_to_string(
    eDbDeviceEventSeverity severity)
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
            return "low";
    }
}


/*
 * =========================================================
 * READ ALARM
 * =========================================================
 */

int _dbread_alarm(int64_t id,
                  stDb_T_alarm *alarm)
{
    /*
     * Validación
     */
    if (alarm == NULL)
        return _DB_STS_ERR_STRUCT;


    /*
     * Limpiar estructura
     */
    memset(alarm,
           0,
           sizeof(stDb_T_alarm));


    /*
     * -----------------------------------------------------
     * Conexión PostgreSQL
     * -----------------------------------------------------
     */

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbread_alarm] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_alarm] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    /*
     * -----------------------------------------------------
     * ID
     * -----------------------------------------------------
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
     * -----------------------------------------------------
     * Query
     * -----------------------------------------------------
     */

    const char *query =
        "SELECT "
        "id, "
        "light_id, "
        "street_name, "
        "zone_id, "
        "alarm_type, "
        "severity, "
        "message, "
        "acknowledged, "
        "acknowledged_by, "
        "acknowledged_at, "
        "resolved, "
        "resolved_at, "
        "created_at "
        "FROM alarm "
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
     * -----------------------------------------------------
     * Verificar resultado
     * -----------------------------------------------------
     */

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbread_alarm] Query failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_alarm] Query failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    /*
     * -----------------------------------------------------
     * Verificar cantidad de registros
     * -----------------------------------------------------
     */

    int rows = PQntuples(res);

    if (rows == 0)
    {
        printf("[_dbread_alarm] Alarm not found: %lld\n",
               (long long)id);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    if (rows != 1)
    {
        printf("[_dbread_alarm] Unexpected rows: %d\n",
               rows);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * -----------------------------------------------------
     * Verificar cantidad de columnas
     * -----------------------------------------------------
     *
     * alarm tiene 13 columnas.
     */

    int cols = PQnfields(res);

    if (cols != 13)
    {
        printf("[_dbread_alarm] Error! Columns (%d)\n",
               cols);

        _log("[_dbread_alarm] Error! Columns (%d)\n",
             cols);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * -----------------------------------------------------
     * Cargar estructura
     * -----------------------------------------------------
     */


    /*
     * 0 - id
     */
    if (!PQgetisnull(res, 0, 0))
    {
        alarm->id =
            atoll(PQgetvalue(res, 0, 0));
    }


    /*
     * 1 - light_id
     */
    if (!PQgetisnull(res, 0, 1))
    {
        strncpy(alarm->light_id,
                PQgetvalue(res, 0, 1),
                sizeof(alarm->light_id) - 1);
    }


    /*
     * 2 - street_name
     */
    if (!PQgetisnull(res, 0, 2))
    {
        strncpy(alarm->street_name,
                PQgetvalue(res, 0, 2),
                sizeof(alarm->street_name) - 1);
    }


    /*
     * 3 - zone_id
     */
    if (!PQgetisnull(res, 0, 3))
    {
        alarm->zone_id =
            atoi(PQgetvalue(res, 0, 3));
    }


    /*
     * 4 - alarm_type
     */
    if (!PQgetisnull(res, 0, 4))
    {
        alarm->alarm_type =
            _dbalarm_type_from_string(
                PQgetvalue(res, 0, 4));
    }


    /*
     * 5 - severity
     */
    if (!PQgetisnull(res, 0, 5))
    {
        alarm->severity =
            _dbalarm_severity_from_string(
                PQgetvalue(res, 0, 5));
    }


    /*
     * 6 - message
     */
    if (!PQgetisnull(res, 0, 6))
    {
        strncpy(alarm->message,
                PQgetvalue(res, 0, 6),
                sizeof(alarm->message) - 1);
    }


    /*
     * 7 - acknowledged
     */
    if (!PQgetisnull(res, 0, 7))
    {
        alarm->acknowledged =
            (strcmp(PQgetvalue(res, 0, 7), "t") == 0);
    }


    /*
     * 8 - acknowledged_by
     */
    if (!PQgetisnull(res, 0, 8))
    {
        strncpy(alarm->acknowledged_by,
                PQgetvalue(res, 0, 8),
                sizeof(alarm->acknowledged_by) - 1);
    }


    /*
     * 9 - acknowledged_at
     *
     * Puede ser NULL.
     */
    if (!PQgetisnull(res, 0, 9))
    {
        strncpy(alarm->acknowledged_at,
                PQgetvalue(res, 0, 9),
                sizeof(alarm->acknowledged_at) - 1);
    }


    /*
     * 10 - resolved
     */
    if (!PQgetisnull(res, 0, 10))
    {
        alarm->resolved =
            (strcmp(PQgetvalue(res, 0, 10), "t") == 0);
    }


    /*
     * 11 - resolved_at
     *
     * Puede ser NULL.
     */
    if (!PQgetisnull(res, 0, 11))
    {
        strncpy(alarm->resolved_at,
                PQgetvalue(res, 0, 11),
                sizeof(alarm->resolved_at) - 1);
    }


    /*
     * 12 - created_at
     */
    if (!PQgetisnull(res, 0, 12))
    {
        strncpy(alarm->created_at,
                PQgetvalue(res, 0, 12),
                sizeof(alarm->created_at) - 1);
    }


    /*
     * -----------------------------------------------------
     * Debug
     * -----------------------------------------------------
     */

#ifdef _DEBUG_DB_READ

    printf("\n[_dbread_alarm]\n");

    printf("  id               : %lld\n",
           (long long)alarm->id);

    printf("  light_id         : %s\n",
           alarm->light_id);

    printf("  street_name      : %s\n",
           alarm->street_name);

    printf("  zone_id          : %d\n",
           alarm->zone_id);

    printf("  alarm_type       : %s\n",
           _dbalarm_type_to_string(alarm->alarm_type));

    printf("  severity         : %s\n",
           _dbalarm_severity_to_string(alarm->severity));

    printf("  message          : %s\n",
           alarm->message);

    printf("  acknowledged     : %d\n",
           alarm->acknowledged);

    printf("  acknowledged_by  : %s\n",
           alarm->acknowledged_by);

    printf("  acknowledged_at  : %s\n",
           alarm->acknowledged_at);

    printf("  resolved         : %d\n",
           alarm->resolved);

    printf("  resolved_at      : %s\n",
           alarm->resolved_at);

    printf("  created_at       : %s\n",
           alarm->created_at);

#endif


    /*
     * -----------------------------------------------------
     * Clean up
     * -----------------------------------------------------
     */

    PQclear(res);
    PQfinish(conn);


    return _DB_STS_OK;
}

int _dbinsert_alarm(stDb_T_alarm *alarm)
{
    if (alarm == NULL)
        return _DB_STS_ERR_STRUCT;


    /*
     * -----------------------------------------------------
     * Conexión PostgreSQL
     * -----------------------------------------------------
     */

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbinsert_alarm] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbinsert_alarm] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    /*
     * -----------------------------------------------------
     * Convertir valores a string
     * -----------------------------------------------------
     */

    char zone_id_str[16];
    char acknowledged_str[8];
    char resolved_str[8];

    snprintf(zone_id_str,
             sizeof(zone_id_str),
             "%d",
             alarm->zone_id);

    snprintf(acknowledged_str,
             sizeof(acknowledged_str),
             "%s",
             alarm->acknowledged ? "true" : "false");

    snprintf(resolved_str,
             sizeof(resolved_str),
             "%s",
             alarm->resolved ? "true" : "false");


    /*
     * -----------------------------------------------------
     * Parámetros
     *
     *  1 light_id
     *  2 street_name
     *  3 zone_id
     *  4 alarm_type
     *  5 severity
     *  6 message
     *  7 acknowledged
     *  8 acknowledged_by
     *  9 acknowledged_at
     * 10 resolved
     * 11 resolved_at
     *
     * created_at NO se envía.
     * PostgreSQL utiliza DEFAULT NOW().
     * -----------------------------------------------------
     */

    const char *paramValues[11];

    paramValues[0] = alarm->light_id;
    paramValues[1] = alarm->street_name;
    paramValues[2] = zone_id_str;

    paramValues[3] =
        _dbalarm_type_to_string(alarm->alarm_type);

    paramValues[4] =
        _dbalarm_severity_to_string(alarm->severity);

    paramValues[5] = alarm->message;

    paramValues[6] = acknowledged_str;

    paramValues[7] = alarm->acknowledged_by;


    /*
     * acknowledged_at
     *
     * Si está vacío mandamos NULL.
     */

    if (alarm->acknowledged_at[0] != '\0')
        paramValues[8] = alarm->acknowledged_at;
    else
        paramValues[8] = NULL;


    paramValues[9] = resolved_str;


    /*
     * resolved_at
     *
     * Si está vacío mandamos NULL.
     */

    if (alarm->resolved_at[0] != '\0')
        paramValues[10] = alarm->resolved_at;
    else
        paramValues[10] = NULL;


    /*
     * -----------------------------------------------------
     * Query
     * -----------------------------------------------------
     */

    const char *query =
        "INSERT INTO alarm ("
        "light_id, "
        "street_name, "
        "zone_id, "
        "alarm_type, "
        "severity, "
        "message, "
        "acknowledged, "
        "acknowledged_by, "
        "acknowledged_at, "
        "resolved, "
        "resolved_at"
        ") "
        "VALUES ("
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


    /*
     * -----------------------------------------------------
     * Ejecutar
     * -----------------------------------------------------
     */

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
     * -----------------------------------------------------
     * Verificar resultado
     * -----------------------------------------------------
     */

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbinsert_alarm] INSERT failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbinsert_alarm] INSERT failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    /*
     * -----------------------------------------------------
     * Obtener ID generado
     * -----------------------------------------------------
     */

    if (PQntuples(res) != 1 ||
        PQnfields(res) != 1)
    {
        printf("[_dbinsert_alarm] Invalid RETURNING result\n");

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    int64_t generated_id =
        atoll(PQgetvalue(res, 0, 0));

    alarm->id = generated_id;

    /*
     * -----------------------------------------------------
     * Actualizar ID en estructura
     * -----------------------------------------------------
     */

    /*
     * El parámetro es const, por lo tanto no podemos
     * escribir alarm->id.
     *
     * El ID queda disponible en generated_id.
     */


#ifdef _DEBUG_DB_WRITE

    printf("\n[_dbinsert_alarm]\n");

    printf("  id               : %lld\n",
           (long long)generated_id);

    printf("  light_id         : %s\n",
           alarm->light_id);

    printf("  street_name      : %s\n",
           alarm->street_name);

    printf("  zone_id          : %d\n",
           alarm->zone_id);

    printf("  alarm_type       : %s\n",
           _dbalarm_type_to_string(alarm->alarm_type));

    printf("  severity         : %s\n",
           _dbalarm_severity_to_string(alarm->severity));

    printf("  message          : %s\n",
           alarm->message);

    printf("  acknowledged     : %d\n",
           alarm->acknowledged);

    printf("  acknowledged_by  : %s\n",
           alarm->acknowledged_by);

    printf("  acknowledged_at  : %s\n",
           alarm->acknowledged_at);

    printf("  resolved         : %d\n",
           alarm->resolved);

    printf("  resolved_at      : %s\n",
           alarm->resolved_at);

#endif


    /*
     * -----------------------------------------------------
     * Clean up
     * -----------------------------------------------------
     */

    PQclear(res);
    PQfinish(conn);


    return _DB_STS_OK;
}



#ifdef _TEST_DB_ALARM_READ

    #include <stdio.h>
    #include <stdlib.h>

    #include "db_alarm.h"

    int main(void)
    {
        stDb_T_alarm alarm;

        /*
        * ID de alarma a buscar
        */
        int64_t alarm_id = 1;


        printf("========================================\n");
        printf("          TEST DB ALARM\n");
        printf("========================================\n\n");


        printf("Buscando alarma ID: %lld\n",
            (long long)alarm_id);


        int ret = _dbread_alarm(alarm_id, &alarm);


        printf("\nResultado: %d\n", ret);


        if (ret != _DB_STS_OK)
        {
            printf("Error leyendo alarma\n");

            return EXIT_FAILURE;
        }


        printf("\n----------------------------------------\n");
        printf("Alarma encontrada\n");
        printf("----------------------------------------\n");


        printf("  id               : %lld\n",
            (long long)alarm.id);

        printf("  light_id         : %s\n",
            alarm.light_id);

        printf("  street_name      : %s\n",
            alarm.street_name);

        printf("  zone_id          : %d\n",
            alarm.zone_id);

        printf("  alarm_type       : %s\n",
            _dbalarm_type_to_string(alarm.alarm_type));

        printf("  severity         : %d\n",
            alarm.severity);

        printf("  message          : %s\n",
            alarm.message);

        printf("  acknowledged     : %d\n",
            alarm.acknowledged);

        printf("  acknowledged_by  : %s\n",
            alarm.acknowledged_by);

        printf("  acknowledged_at  : %s\n",
            alarm.acknowledged_at);

        printf("  resolved         : %d\n",
            alarm.resolved);

        printf("  resolved_at      : %s\n",
            alarm.resolved_at);

        printf("  created_at       : %s\n",
            alarm.created_at);


        printf("\n========================================\n");
        printf("Fin del test\n");
        printf("========================================\n");


        return EXIT_SUCCESS;
    }

#endif




#ifdef _TEST_DB_ALARM_INSERT

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "db_alarm.h"
#include "db.h"


int main(void)
{
    stDb_T_alarm alarm;


    /*
     * -----------------------------------------------------
     * Inicializar estructura
     * -----------------------------------------------------
     */

    memset(&alarm, 0, sizeof(stDb_T_alarm));


    /*
     * -----------------------------------------------------
     * Datos de prueba
     * -----------------------------------------------------
 */

    strcpy(alarm.light_id,
           "SL-001");

    strcpy(alarm.street_name,
           "Av. San Martin");

    alarm.zone_id = 1;

    alarm.alarm_type =
        DB_ALARM_LAMP_FAULT;

    alarm.severity =
        DEV_SEVERITY_CRITICAL;

    strcpy(alarm.message,
           "Lamp failure detected");


    /*
     * Al crear una alarma nueva:
     *
     * acknowledged = FALSE
     * resolved     = FALSE
     *
     * acknowledged_at = NULL
     * resolved_at     = NULL
     */

    alarm.acknowledged = 0;
    alarm.resolved = 0;


    /*
     * -----------------------------------------------------
     * Mostrar datos antes del INSERT
     * -----------------------------------------------------
     */

    printf("========================================\n");
    printf("       TEST DB ALARM INSERT\n");
    printf("========================================\n\n");


    printf("----------------------------------------\n");
    printf("Nueva alarma\n");
    printf("----------------------------------------\n");


    printf("  light_id         : %s\n",
           alarm.light_id);

    printf("  street_name      : %s\n",
           alarm.street_name);

    printf("  zone_id          : %d\n",
           alarm.zone_id);

    printf("  alarm_type       : %s\n",
           _dbalarm_type_to_string(
               alarm.alarm_type));

    printf("  severity         : %s\n",
           _dbalarm_severity_to_string(
               alarm.severity));

    printf("  message          : %s\n",
           alarm.message);

    printf("  acknowledged     : %d\n",
           alarm.acknowledged);

    printf("  resolved         : %d\n",
           alarm.resolved);


    /*
     * -----------------------------------------------------
     * INSERT
     * -----------------------------------------------------
     */

    printf("\n----------------------------------------\n");
    printf("Insertando en PostgreSQL...\n");
    printf("----------------------------------------\n");


    int ret = _dbinsert_alarm(&alarm);


    printf("\nResultado escritura: %d\n",
           ret);


    /*
     * -----------------------------------------------------
     * Resultado
     * -----------------------------------------------------
     */

    if (ret != _DB_STS_OK)
    {
        printf("ERROR insertando alarma\n");

        return EXIT_FAILURE;
    }


    printf("\n----------------------------------------\n");
    printf("Alarma insertada correctamente\n");
    printf("----------------------------------------\n");


    printf("ID generado: %lld\n",
           (long long)alarm.id);


    printf("\n========================================\n");
    printf("Fin del test\n");
    printf("========================================\n");


    return EXIT_SUCCESS;
}

#endif