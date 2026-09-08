#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "db_devstate.h"
#include "db_devcalendar.h"

stDb_T_devcalendar T_devcalendar[_CANT_MAX_EQ];
stDb_T_devcalendar_info devcalendar_info;


int _dbread_devcalendar_size(void)
{
    int count = 0;

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        if (T_devcalendar[i].light_id[0] != '\0')
            count++;
    }

    return count;
}

stDb_T_devcalendar *_Stfind_Devcalendar(const char *light_id)
{
    if (light_id == NULL)
        return NULL;

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        if (T_devcalendar[i].light_id[0] == '\0')
            continue;

        if (strcmp(T_devcalendar[i].light_id, light_id) == 0)
            return &T_devcalendar[i];
    }

    return NULL;
}


int _dbread_table_devcalendar(void)
{
    int ret = _DB_STS_OK;

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbread_table_devcalendar] Connection to database failed: %s",
                PQerrorMessage(conn));

        _log("[_dbread_table_devcalendar] Connection to database failed: %s",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    PGresult *res = PQexec(conn,
        "SELECT "
        "light_id, "          /*  0 */
        "event_id, "          /*  1 */
        "enabled, "           /*  2 */
        "start_hour, "        /*  3 */
        "start_minute, "      /*  4 */
        "end_hour, "          /*  5 */
        "end_minute, "        /*  6 */
        "days_mask, "         /*  7 */
        "action, "            /*  8 */
        "rgbg1_r, "           /*  9 */
        "rgbg1_g, "           /* 10 */
        "rgbg1_b, "           /* 11 */
        "rgbg2_r, "           /* 12 */
        "rgbg2_g, "           /* 13 */
        "rgbg2_b, "           /* 14 */
        "rgbg3_r, "           /* 15 */
        "rgbg3_g, "           /* 16 */
        "rgbg3_b, "           /* 17 */
        "dimming "            /* 18 */
        "FROM devcalendar "
        "ORDER BY light_id, event_id"
    );


    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbread_table_devcalendar] Query execution failed: %s",
                PQerrorMessage(conn));

        _log("[_dbread_table_devcalendar] Query execution failed: %s",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    int rows = PQntuples(res);
    int cols = PQnfields(res);


#ifdef _DEBUG_DB_READ
    printf("[_dbread_table_devcalendar] Rows: %d\n", rows);
#endif


    if (cols != 19)
    {
#ifdef _DEBUG_DB_READ
        printf("[_dbread_table_devcalendar] Error! Columns (%d)\n", cols);
#endif

        _log("[_dbread_table_devcalendar] Error! Columns (%d)\n", cols);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    memset(T_devcalendar, 0, sizeof(T_devcalendar));

    int dev_index = -1;
    char last_light_id[51];

    memset(last_light_id, 0, sizeof(last_light_id));


    for (int i = 0; i < rows; i++)
    {
        const char *light_id = PQgetvalue(res, i, 0);

        /*
         * Nuevo dispositivo
         */
        if (strcmp(last_light_id, light_id) != 0)
        {
            dev_index++;

            if (dev_index >= _CANT_MAX_EQ)
            {
                printf("[_dbread_table_devcalendar] Error! MAX devices\n");

                _log("[_dbread_table_devcalendar] Error! MAX devices\n");

                ret = _DB_STS_ERR_STRUCT;
                break;
            }

            strncpy(T_devcalendar[dev_index].light_id,
                    light_id,
                    sizeof(T_devcalendar[dev_index].light_id) - 1);

            strncpy(last_light_id,
                    light_id,
                    sizeof(last_light_id) - 1);
        }


        int event_id = atoi(PQgetvalue(res, i, 1));

        if (event_id < 0 || event_id >= _MAXCALENDARLST)
        {
            printf("[_dbread_table_devcalendar] Invalid event_id: %d\n",
                   event_id);

            continue;
        }


        stCalendarEvent *pevent =
            &T_devcalendar[dev_index].CalendarList[event_id];


        /*
         * enabled
         */
        if (!PQgetisnull(res, i, 2))
        {
            const char *v = PQgetvalue(res, i, 2);

            pevent->enabled =
                (v[0] == 't' || v[0] == 'T' || v[0] == '1');
        }


        if (!PQgetisnull(res, i, 3))
            pevent->start_hour =
                (uint8_t)atoi(PQgetvalue(res, i, 3));

        if (!PQgetisnull(res, i, 4))
            pevent->start_minute =
                (uint8_t)atoi(PQgetvalue(res, i, 4));

        if (!PQgetisnull(res, i, 5))
            pevent->end_hour =
                (uint8_t)atoi(PQgetvalue(res, i, 5));

        if (!PQgetisnull(res, i, 6))
            pevent->end_minute =
                (uint8_t)atoi(PQgetvalue(res, i, 6));

        if (!PQgetisnull(res, i, 7))
            pevent->days_mask =
                (uint8_t)atoi(PQgetvalue(res, i, 7));

        if (!PQgetisnull(res, i, 8))
            pevent->action =
                (uint8_t)atoi(PQgetvalue(res, i, 8));


        /*
         * RGB GROUP 1
         */
        if (!PQgetisnull(res, i, 9))
            pevent->r_g1 =
                (uint8_t)atoi(PQgetvalue(res, i, 9));

        if (!PQgetisnull(res, i, 10))
            pevent->g_g1 =
                (uint8_t)atoi(PQgetvalue(res, i, 10));

        if (!PQgetisnull(res, i, 11))
            pevent->b_g1 =
                (uint8_t)atoi(PQgetvalue(res, i, 11));


        /*
         * RGB GROUP 2
         */
        if (!PQgetisnull(res, i, 12))
            pevent->r_g2 =
                (uint8_t)atoi(PQgetvalue(res, i, 12));

        if (!PQgetisnull(res, i, 13))
            pevent->g_g2 =
                (uint8_t)atoi(PQgetvalue(res, i, 13));

        if (!PQgetisnull(res, i, 14))
            pevent->b_g2 =
                (uint8_t)atoi(PQgetvalue(res, i, 14));


        /*
         * RGB GROUP 3
         */
        if (!PQgetisnull(res, i, 15))
            pevent->r_g3 =
                (uint8_t)atoi(PQgetvalue(res, i, 15));

        if (!PQgetisnull(res, i, 16))
            pevent->g_g3 =
                (uint8_t)atoi(PQgetvalue(res, i, 16));

        if (!PQgetisnull(res, i, 17))
            pevent->b_g3 =
                (uint8_t)atoi(PQgetvalue(res, i, 17));


        if (!PQgetisnull(res, i, 18))
            pevent->dimming =
                (uint8_t)atoi(PQgetvalue(res, i, 18));


#ifdef _DEBUG_DB_READ

        printf("\nCalendar: %s Event: %d\n",
               T_devcalendar[dev_index].light_id,
               event_id);

        printf("enabled      : %d\n", pevent->enabled);
        printf("start        : %02u:%02u\n",
               pevent->start_hour,
               pevent->start_minute);

        printf("end          : %02u:%02u\n",
               pevent->end_hour,
               pevent->end_minute);

        printf("days_mask    : %02X\n", pevent->days_mask);
        printf("action       : %u\n", pevent->action);

        printf("RGB G1       : %u %u %u\n",
               pevent->r_g1,
               pevent->g_g1,
               pevent->b_g1);

        printf("RGB G2       : %u %u %u\n",
               pevent->r_g2,
               pevent->g_g2,
               pevent->b_g2);

        printf("RGB G3       : %u %u %u\n",
               pevent->r_g3,
               pevent->g_g3,
               pevent->b_g3);

        printf("dimming      : %u\n",
               pevent->dimming);

#endif
    }


    PQclear(res);
    PQfinish(conn);

    return ret;
}

int _dbwrite_devcalendar(stDb_T_devcalendar *pdev)
{
    if (pdev == NULL)
        return _DB_STS_ERR_STRUCT;


    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbwrite_devcalendar] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbwrite_devcalendar] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    /*
     * Los 5 eventos forman una sola configuración.
     * Usamos transacción.
     */
    PGresult *res = PQexec(conn, "BEGIN");

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }

    PQclear(res);


    const char *query =
        "INSERT INTO devcalendar ("
        "light_id, "
        "event_id, "
        "enabled, "
        "start_hour, "
        "start_minute, "
        "end_hour, "
        "end_minute, "
        "days_mask, "
        "action, "
        "rgbg1_r, rgbg1_g, rgbg1_b, "
        "rgbg2_r, rgbg2_g, rgbg2_b, "
        "rgbg3_r, rgbg3_g, rgbg3_b, "
        "dimming, "
        "updated_at"
        ") VALUES ("
        "$1,$2,$3,$4,$5,$6,$7,$8,$9,"
        "$10,$11,$12,"
        "$13,$14,$15,"
        "$16,$17,$18,"
        "$19,NOW()"
        ") "
        "ON CONFLICT (light_id, event_id) "
        "DO UPDATE SET "
        "enabled = EXCLUDED.enabled, "
        "start_hour = EXCLUDED.start_hour, "
        "start_minute = EXCLUDED.start_minute, "
        "end_hour = EXCLUDED.end_hour, "
        "end_minute = EXCLUDED.end_minute, "
        "days_mask = EXCLUDED.days_mask, "
        "action = EXCLUDED.action, "
        "rgbg1_r = EXCLUDED.rgbg1_r, "
        "rgbg1_g = EXCLUDED.rgbg1_g, "
        "rgbg1_b = EXCLUDED.rgbg1_b, "
        "rgbg2_r = EXCLUDED.rgbg2_r, "
        "rgbg2_g = EXCLUDED.rgbg2_g, "
        "rgbg2_b = EXCLUDED.rgbg2_b, "
        "rgbg3_r = EXCLUDED.rgbg3_r, "
        "rgbg3_g = EXCLUDED.rgbg3_g, "
        "rgbg3_b = EXCLUDED.rgbg3_b, "
        "dimming = EXCLUDED.dimming, "
        "updated_at = NOW()";


    for (int i = 0; i < _MAXCALENDARLST; i++)
    {
        stCalendarEvent *pevent =
            &pdev->CalendarList[i];


        char event_id[8];
        char enabled[8];

        char start_hour[8];
        char start_minute[8];
        char end_hour[8];
        char end_minute[8];

        char days_mask[8];
        char action[8];

        char r_g1[8], g_g1[8], b_g1[8];
        char r_g2[8], g_g2[8], b_g2[8];
        char r_g3[8], g_g3[8], b_g3[8];

        char dimming[8];


        snprintf(event_id, sizeof(event_id), "%d", i);

        snprintf(enabled, sizeof(enabled),
                 "%s",
                 pevent->enabled ? "true" : "false");

        snprintf(start_hour, sizeof(start_hour),
                 "%u", pevent->start_hour);

        snprintf(start_minute, sizeof(start_minute),
                 "%u", pevent->start_minute);

        snprintf(end_hour, sizeof(end_hour),
                 "%u", pevent->end_hour);

        snprintf(end_minute, sizeof(end_minute),
                 "%u", pevent->end_minute);

        snprintf(days_mask, sizeof(days_mask),
                 "%u", pevent->days_mask);

        snprintf(action, sizeof(action),
                 "%u", pevent->action);


        snprintf(r_g1, sizeof(r_g1), "%u", pevent->r_g1);
        snprintf(g_g1, sizeof(g_g1), "%u", pevent->g_g1);
        snprintf(b_g1, sizeof(b_g1), "%u", pevent->b_g1);

        snprintf(r_g2, sizeof(r_g2), "%u", pevent->r_g2);
        snprintf(g_g2, sizeof(g_g2), "%u", pevent->g_g2);
        snprintf(b_g2, sizeof(b_g2), "%u", pevent->b_g2);

        snprintf(r_g3, sizeof(r_g3), "%u", pevent->r_g3);
        snprintf(g_g3, sizeof(g_g3), "%u", pevent->g_g3);
        snprintf(b_g3, sizeof(b_g3), "%u", pevent->b_g3);

        snprintf(dimming, sizeof(dimming),
                 "%u", pevent->dimming);


        const char *paramValues[] =
        {
            pdev->light_id,
            event_id,
            enabled,

            start_hour,
            start_minute,
            end_hour,
            end_minute,

            days_mask,
            action,

            r_g1,
            g_g1,
            b_g1,

            r_g2,
            g_g2,
            b_g2,

            r_g3,
            g_g3,
            b_g3,

            dimming
        };


        res = PQexecParams(
            conn,
            query,
            19,
            NULL,
            paramValues,
            NULL,
            NULL,
            0
        );


        if (PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            fprintf(stderr,
                    "[_dbwrite_devcalendar] Query failed: %s\n",
                    PQerrorMessage(conn));

            _log("[_dbwrite_devcalendar] Query failed: %s\n",
                 PQerrorMessage(conn));

            PQclear(res);

            res = PQexec(conn, "ROLLBACK");
            PQclear(res);

            PQfinish(conn);

            return _DB_STS_ERR_CMD;
        }

        PQclear(res);
    }


    res = PQexec(conn, "COMMIT");

    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }

    PQclear(res);


#ifdef _DEBUG_DB_READ
    printf("[_dbwrite_devcalendar] Updated: %s\n",
           pdev->light_id);
#endif


    PQfinish(conn);

    return _DB_STS_OK;
}


int _dbcheck_devcalendar(stDb_T_devcalendar_info *info)
{
    if (info == NULL)
        return _DB_STS_ERR_STRUCT;


    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbcheck_devcalendar] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbcheck_devcalendar] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    const char *query =
        "SELECT COUNT(*), "
        "       MAX(updated_at) "
        "FROM devcalendar";


    PGresult *res = PQexec(conn, query);


    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbcheck_devcalendar] Query failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbcheck_devcalendar] Query failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    int db_count = 0;

    char db_last_update[32];

    memset(db_last_update, 0, sizeof(db_last_update));


    if (!PQgetisnull(res, 0, 0))
        db_count = atoi(PQgetvalue(res, 0, 0));


    if (!PQgetisnull(res, 0, 1))
    {
        strncpy(db_last_update,
                PQgetvalue(res, 0, 1),
                sizeof(db_last_update) - 1);
    }


#ifdef _DEBUG_DB_READ

    printf("\n[_dbcheck_devcalendar]\n");
    printf("  DB count       : %d\n", db_count);
    printf("  DB last_update : %s\n", db_last_update);
    printf("  RAM count      : %d\n", info->count);
    printf("  RAM last_update: %s\n", info->last_update);

#endif


    int changed = 0;


    if (db_count != info->count)
    {
        changed = 1;
    }
    else if (strcmp(db_last_update,
                    info->last_update) != 0)
    {
        changed = 1;
    }


    if (changed)
    {
        info->count = db_count;

        strncpy(info->last_update,
                db_last_update,
                sizeof(info->last_update) - 1);

        info->last_update[
            sizeof(info->last_update) - 1
        ] = '\0';


#ifdef _DEBUG_DB_READ
        printf("  --> DEVCALENDAR CHANGED\n");
#endif
    }


    PQclear(res);
    PQfinish(conn);


    return changed ?
           _DB_DEVSTATE_CHANGED :
           _DB_DEVSTATE_NOCHANGE;
}


void _InitDb_devcalendar(void)
{
    memset(&devcalendar_info, 0, sizeof(devcalendar_info));

    _dbread_table_devcalendar();

    /*
     * Forzamos que el primer _dbcheck_devcalendar()
     * sincronice count y last_update.
     */
    devcalendar_info.count = -1;
}