#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "db_devstate.h"


stDb_T_devstate T_devstate[_CANT_MAX_EQ];
stDb_T_devstate_info devstate_info;


int _dbread_devstate_size(void)
{
    int count = 0;

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        if (T_devstate[i].light_id[0] != '\0')
            count++;
    }

    return count;
}

stDb_T_devstate *_Stfind_Devstate(const char *light_id)
{
    if (light_id == NULL)
        return NULL;

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        /*
         * Registro válido
         */
        if (T_devstate[i].light_id[0] == '\0')
            continue;

        /*
         * Comparamos el light_id
         */
        if (strcmp(T_devstate[i].light_id, light_id) == 0)
            return &T_devstate[i];
    }

    return NULL;
}


stDb_T_devstate *_Stfind_Devstate_ByZone(int zone_id)
{
    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        /*
         * Registro válido
         */
        if (T_devstate[i].light_id[0] == '\0')
            continue;

        /*
         * Comparamos el zone_id
         */
        if (T_devstate[i].zone_id == zone_id)
            return &T_devstate[i];
    }

    return NULL;
}

/*
 * Lee la tabla devstate completa
 *
 * Requiere que stDb_T_devstate tenga:
 *
 *     stRgbGroup rgb[3];
 *
 * y que stRgbGroup sea:
 *
 *     typedef struct
 *     {
 *         bool    enable;
 *         uint8_t r;
 *         uint8_t g;
 *         uint8_t b;
 *     } stRgbGroup;
 */
int _dbread_table_devstate(void)
{
    int ret = _DB_STS_OK;

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbread_table_devstate] Connection to database failed: %s",
                PQerrorMessage(conn));

        _log("[_dbread_table_devstate] Connection to database failed: %s",
             PQerrorMessage(conn));

        PQfinish(conn);
        return _DB_STS_ERR_CONN;
    }

    /*
     * Consulta explícita de las columnas.
     * No usamos SELECT * para mantener fijo el orden.
     */
    PGresult *res = PQexec(conn,
        "SELECT "
        "light_id, "          /*  0 */
        "zone_id, "           /*  1 */
        "lat, "               /*  2 */
        "lng, "               /*  3 */
        "status, "            /*  4 */
        "dimming_level, "     /*  5 */

        "rgbg1_enable, "      /*  6 */
        "rgbg1_r, "           /*  7 */
        "rgbg1_g, "           /*  8 */
        "rgbg1_b, "           /*  9 */

        "rgbg2_enable, "      /* 10 */
        "rgbg2_r, "           /* 11 */
        "rgbg2_g, "           /* 12 */
        "rgbg2_b, "           /* 13 */

        "rgbg3_enable, "      /* 14 */
        "rgbg3_r, "           /* 15 */
        "rgbg3_g, "           /* 16 */
        "rgbg3_b, "           /* 17 */

        "power_watts, "       /* 18 */
        "voltage, "           /* 19 */
        "temperature_c, "     /* 20 */
        "burn_hours, "        /* 21 */
        "last_seen, "         /* 22 */
        "street_name, "       /* 23 */
        "lamp_type, "         /* 24 */
        "rated_watts, "       /* 25 */
        "created_at, "        /* 26 */
        "updated_at "         /* 27 */
        "FROM devstate"
    );

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbread_table_devstate] Query execution failed: %s",
                PQerrorMessage(conn));

        _log("[_dbread_table_devstate] Query execution failed: %s",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }

    int rows = PQntuples(res);

#ifdef _DEBUG_DB_READ
    printf("[_dbread_table_devstate] Rows: %d\n", rows);
#endif

    if (rows > _CANT_MAX_EQ)
    {
        printf("[_dbread_table_devstate] Error! MAX(%d)\r\n", rows);

        _log("[_dbread_table_devstate] Error! MAX(%d)\r\n", rows);

        ret = _DB_STS_ERR_STRUCT;
    }
    else
    {
        int cols = PQnfields(res);

        /*
         * 16 columnas originales + 12 columnas RGB = 28.
         */
        if (cols != 28)
        {
#ifdef _DEBUG_DB_READ
            printf("[_dbread_table_devstate] Error! Columns (%d)\r\n", cols);
#endif
            _log("[_dbread_table_devstate] Error! Columns (%d)\r\n", cols);

            ret = _DB_STS_ERR_STRUCT;
        }
        else
        {
            memset(T_devstate, 0, sizeof(T_devstate));

            for (int i = 0; i < rows; i++)
            {
#ifdef _DEBUG_DB_READ
                printf("\nrow nr:%d\n", i + 1);
#endif

                /* light_id */
                if (!PQgetisnull(res, i, 0))
                {
                    strncpy(T_devstate[i].light_id,
                            PQgetvalue(res, i, 0),
                            sizeof(T_devstate[i].light_id) - 1);
                }

                /* zone_id */
                if (!PQgetisnull(res, i, 1))
                {
                    T_devstate[i].zone_id =
                        atoi(PQgetvalue(res, i, 1));
                }

                /* lat */
                if (!PQgetisnull(res, i, 2))
                {
                    T_devstate[i].lat =
                        atof(PQgetvalue(res, i, 2));
                }

                /* lng */
                if (!PQgetisnull(res, i, 3))
                {
                    T_devstate[i].lng =
                        atof(PQgetvalue(res, i, 3));
                }

                /* status */
                if (!PQgetisnull(res, i, 4))
                {
                    strncpy(T_devstate[i].status,
                            PQgetvalue(res, i, 4),
                            sizeof(T_devstate[i].status) - 1);
                }

                /* dimming_level */
                if (!PQgetisnull(res, i, 5))
                {
                    T_devstate[i].dimming_level =
                        atoi(PQgetvalue(res, i, 5));
                }

                /*
                 * RGB GROUP 1
                 */
                if (!PQgetisnull(res, i, 6))
                {
                    const char *v = PQgetvalue(res, i, 6);

                    T_devstate[i].rgb[0].enable =
                        (v[0] == 't' || v[0] == 'T' || v[0] == '1');
                }

                if (!PQgetisnull(res, i, 7))
                {
                    T_devstate[i].rgb[0].r =
                        (uint8_t)atoi(PQgetvalue(res, i, 7));
                }

                if (!PQgetisnull(res, i, 8))
                {
                    T_devstate[i].rgb[0].g =
                        (uint8_t)atoi(PQgetvalue(res, i, 8));
                }

                if (!PQgetisnull(res, i, 9))
                {
                    T_devstate[i].rgb[0].b =
                        (uint8_t)atoi(PQgetvalue(res, i, 9));
                }

                /*
                 * RGB GROUP 2
                 */
                if (!PQgetisnull(res, i, 10))
                {
                    const char *v = PQgetvalue(res, i, 10);

                    T_devstate[i].rgb[1].enable =
                        (v[0] == 't' || v[0] == 'T' || v[0] == '1');
                }

                if (!PQgetisnull(res, i, 11))
                {
                    T_devstate[i].rgb[1].r =
                        (uint8_t)atoi(PQgetvalue(res, i, 11));
                }

                if (!PQgetisnull(res, i, 12))
                {
                    T_devstate[i].rgb[1].g =
                        (uint8_t)atoi(PQgetvalue(res, i, 12));
                }

                if (!PQgetisnull(res, i, 13))
                {
                    T_devstate[i].rgb[1].b =
                        (uint8_t)atoi(PQgetvalue(res, i, 13));
                }

                /*
                 * RGB GROUP 3
                 */
                if (!PQgetisnull(res, i, 14))
                {
                    const char *v = PQgetvalue(res, i, 14);

                    T_devstate[i].rgb[2].enable =
                        (v[0] == 't' || v[0] == 'T' || v[0] == '1');
                }

                if (!PQgetisnull(res, i, 15))
                {
                    T_devstate[i].rgb[2].r =
                        (uint8_t)atoi(PQgetvalue(res, i, 15));
                }

                if (!PQgetisnull(res, i, 16))
                {
                    T_devstate[i].rgb[2].g =
                        (uint8_t)atoi(PQgetvalue(res, i, 16));
                }

                if (!PQgetisnull(res, i, 17))
                {
                    T_devstate[i].rgb[2].b =
                        (uint8_t)atoi(PQgetvalue(res, i, 17));
                }

                /* power_watts */
                if (!PQgetisnull(res, i, 18))
                {
                    T_devstate[i].power_watts =
                        atof(PQgetvalue(res, i, 18));
                }

                /* voltage */
                if (!PQgetisnull(res, i, 19))
                {
                    T_devstate[i].voltage =
                        atof(PQgetvalue(res, i, 19));
                }

                /* temperature_c */
                if (!PQgetisnull(res, i, 20))
                {
                    T_devstate[i].temperature_c =
                        atof(PQgetvalue(res, i, 20));
                }

                /* burn_hours */
                if (!PQgetisnull(res, i, 21))
                {
                    T_devstate[i].burn_hours =
                        atoll(PQgetvalue(res, i, 21));
                }

                /* last_seen */
                if (!PQgetisnull(res, i, 22))
                {
                    strncpy(T_devstate[i].last_seen,
                            PQgetvalue(res, i, 22),
                            sizeof(T_devstate[i].last_seen) - 1);
                }

                /* street_name */
                if (!PQgetisnull(res, i, 23))
                {
                    strncpy(T_devstate[i].street_name,
                            PQgetvalue(res, i, 23),
                            sizeof(T_devstate[i].street_name) - 1);
                }

                /* lamp_type */
                if (!PQgetisnull(res, i, 24))
                {
                    strncpy(T_devstate[i].lamp_type,
                            PQgetvalue(res, i, 24),
                            sizeof(T_devstate[i].lamp_type) - 1);
                }

                /* rated_watts */
                if (!PQgetisnull(res, i, 25))
                {
                    T_devstate[i].rated_watts =
                        atof(PQgetvalue(res, i, 25));
                }

                /* created_at */
                if (!PQgetisnull(res, i, 26))
                {
                    strncpy(T_devstate[i].created_at,
                            PQgetvalue(res, i, 26),
                            sizeof(T_devstate[i].created_at) - 1);
                }

                /* updated_at */
                if (!PQgetisnull(res, i, 27))
                {
                    strncpy(T_devstate[i].updated_at,
                            PQgetvalue(res, i, 27),
                            sizeof(T_devstate[i].updated_at) - 1);
                }

#ifdef _DEBUG_DB_READ
                printf("light_id       : %s\n", T_devstate[i].light_id);
                printf("zone_id        : %d\n", T_devstate[i].zone_id);
                printf("lat            : %.6f\n", T_devstate[i].lat);
                printf("lng            : %.6f\n", T_devstate[i].lng);
                printf("status         : %s\n", T_devstate[i].status);
                printf("dimming_level  : %d\n", T_devstate[i].dimming_level);

                for (int g = 0; g < 3; g++)
                {
                    printf("rgbg%d          : EN=%d R=%u G=%u B=%u\n",
                           g + 1,
                           T_devstate[i].rgb[g].enable ? 1 : 0,
                           (unsigned)T_devstate[i].rgb[g].r,
                           (unsigned)T_devstate[i].rgb[g].g,
                           (unsigned)T_devstate[i].rgb[g].b);
                }

                printf("power_watts    : %.2f\n", T_devstate[i].power_watts);
                printf("voltage        : %.2f\n", T_devstate[i].voltage);
                printf("temperature_c  : %.2f\n", T_devstate[i].temperature_c);
                printf("burn_hours     : %lld\n",
                       (long long)T_devstate[i].burn_hours);
                printf("last_seen      : %s\n", T_devstate[i].last_seen);
                printf("street_name    : %s\n", T_devstate[i].street_name);
                printf("lamp_type      : %s\n", T_devstate[i].lamp_type);
                printf("rated_watts    : %.2f\n", T_devstate[i].rated_watts);
                printf("created_at     : %s\n", T_devstate[i].created_at);
                printf("updated_at     : %s\n", T_devstate[i].updated_at);
#endif
            }
        }
    }

    PQclear(res);
    PQfinish(conn);

    return ret;
}

int _dbwrite_devstate(stDb_T_devstate *pdev)
{
    if (pdev == NULL)
        return _DB_STS_ERR_STRUCT;

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbwrite_devstate] Connection to database failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbwrite_devstate] Connection to database failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    char zone_id[16];
    char lat[32];
    char lng[32];
    char dimming_level[16];
    char power_watts[32];
    char voltage[32];
    char temperature_c[32];
    char burn_hours[32];
    char rated_watts[32];


    snprintf(zone_id, sizeof(zone_id),
             "%d", pdev->zone_id);

    snprintf(lat, sizeof(lat),
             "%.10f", pdev->lat);

    snprintf(lng, sizeof(lng),
             "%.10f", pdev->lng);

    snprintf(dimming_level, sizeof(dimming_level),
             "%d", pdev->dimming_level);

    snprintf(power_watts, sizeof(power_watts),
             "%.2f", pdev->power_watts);

    snprintf(voltage, sizeof(voltage),
             "%.2f", pdev->voltage);

    snprintf(temperature_c, sizeof(temperature_c),
             "%.2f", pdev->temperature_c);

    snprintf(burn_hours, sizeof(burn_hours),
             "%lld",
             (long long)pdev->burn_hours);

    snprintf(rated_watts, sizeof(rated_watts),
             "%.2f", pdev->rated_watts);


    /*
     * Parámetros:
     *
     * $1  zone_id
     * $2  lat
     * $3  lng
     * $4  status
     * $5  dimming_level
     * $6  power_watts
     * $7  voltage
     * $8  temperature_c
     * $9  burn_hours
     * $10 street_name
     * $11 lamp_type
     * $12 rated_watts
     * $13 light_id
     */

    const char *paramValues[] =
    {
        zone_id,
        lat,
        lng,
        pdev->status,
        dimming_level,
        power_watts,
        voltage,
        temperature_c,
        burn_hours,
        pdev->street_name,
        pdev->lamp_type,
        rated_watts,
        pdev->light_id
    };


    const char *query =
        "UPDATE devstate SET "
        "zone_id = $1, "
        "lat = $2, "
        "lng = $3, "
        "status = $4, "
        "dimming_level = $5, "
        "power_watts = $6, "
        "voltage = $7, "
        "temperature_c = $8, "
        "burn_hours = $9, "
        "street_name = $10, "
        "lamp_type = $11, "
        "rated_watts = $12, "
        "last_seen = NOW(), "
        "updated_at = NOW() "
        "WHERE light_id = $13";


    PGresult *res = PQexecParams(
        conn,
        query,
        13,
        NULL,
        paramValues,
        NULL,
        NULL,
        0
    );


    if (PQresultStatus(res) != PGRES_COMMAND_OK)
    {
        fprintf(stderr,
                "[_dbwrite_devstate] Query execution failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbwrite_devstate] Query execution failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    /*
     * Verificar que realmente se haya actualizado
     * un registro.
     */
    if (PQcmdTuples(res) == NULL ||
        atoi(PQcmdTuples(res)) != 1)
    {
        printf("[_dbwrite_devstate] Device not found: %s\n",
               pdev->light_id);

        _log("[_dbwrite_devstate] Device not found: %s\n",
             pdev->light_id);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


#ifdef _DEBUG_DB_READ

    printf("[_dbwrite_devstate] Updated: %s\n",
           pdev->light_id);

#endif


    PQclear(res);
    PQfinish(conn);

    return _DB_STS_OK;
}


int _dbcheck_devstate(stDb_T_devstate_info *info)
{
    if (info == NULL)
        return _DB_STS_ERR_STRUCT;

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbcheck_devstate] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbcheck_devstate] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    const char *query =
        "SELECT COUNT(*), "
        "       MAX(updated_at) "
        "FROM devstate";


    PGresult *res = PQexec(conn, query);


    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbcheck_devstate] Query failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbcheck_devstate] Query failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    if (PQntuples(res) != 1 ||
        PQnfields(res) != 2)
    {
        printf("[_dbcheck_devstate] Invalid result\n");

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * ---------------------------------------------
     * COUNT(*)
     * ---------------------------------------------
     */

    int db_count = 0;

    if (!PQgetisnull(res, 0, 0))
    {
        db_count = atoi(PQgetvalue(res, 0, 0));
    }


    /*
     * ---------------------------------------------
     * MAX(updated_at)
     * ---------------------------------------------
     */

    char db_last_update[32];

    memset(db_last_update, 0, sizeof(db_last_update));

    if (!PQgetisnull(res, 0, 1))
    {
        strncpy(db_last_update,
                PQgetvalue(res, 0, 1),
                sizeof(db_last_update) - 1);
    }


#ifdef _DEBUG_DB_READ

    printf("\n[_dbcheck_devstate]\n");
    printf("  DB count       : %d\n", db_count);
    printf("  DB last_update : %s\n", db_last_update);
    printf("  RAM count      : %d\n", info->count);
    printf("  RAM last_update: %s\n", info->last_update);

#endif


    /*
     * ---------------------------------------------
     * Comparar contra el estado conocido en RAM
     * ---------------------------------------------
     */

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


    /*
     * Si hubo cambios, actualizamos la información
     * conocida por RAM.
     */
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

        printf("  --> DEVSTATE CHANGED\n");

#endif

    }


    PQclear(res);
    PQfinish(conn);


    return changed ?
           _DB_DEVSTATE_CHANGED :
           _DB_DEVSTATE_NOCHANGE;
}

void _InitDb_devstate(void)
{
    memset(&devstate_info, 0, sizeof(devstate_info));
    _dbread_table_devstate();
    devstate_info.count = _dbread_devstate_size();
}


#ifdef _TEST_DB_DEVSTATE

int main(void)
{
    printf("========================================\n");
    printf("       TEST DB DEVSTATE\n");
    printf("========================================\n\n");


    /*
     * -------------------------------------------------
     * Leer tabla devstate
     * -------------------------------------------------
     */
    int ret = _dbread_table_devstate();

    printf("\nResultado lectura: %d\n", ret);

    if (ret != _DB_STS_OK)
    {
        printf("Error leyendo tabla devstate\n");
        return EXIT_FAILURE;
    }


    /*
     * -------------------------------------------------
     * Mostrar registros cargados
     * -------------------------------------------------
     */
    printf("\n----------------------------------------\n");
    printf("Registros cargados\n");
    printf("----------------------------------------\n");

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        /*
         * light_id es PRIMARY KEY, por lo tanto
         * podemos utilizarlo para detectar registros válidos.
         */
        if (T_devstate[i].light_id[0] != '\0')
        {
            printf("\nRegistro %d\n", i);

            printf("  light_id      : %s\n",
                T_devstate[i].light_id);

            printf("  zone_id       : %d\n",
                T_devstate[i].zone_id);

            printf("  lat           : %.6f\n",
                T_devstate[i].lat);

            printf("  lng           : %.6f\n",
                T_devstate[i].lng);

            printf("  status        : %s\n",
                T_devstate[i].status);

            printf("  dimming       : %d %%\n",
                T_devstate[i].dimming_level);

            printf("  power         : %.2f W\n",
                T_devstate[i].power_watts);

            printf("  voltage       : %.2f V\n",
                T_devstate[i].voltage);

            printf("  temperature   : %.2f C\n",
                T_devstate[i].temperature_c);

            printf("  burn_hours    : %lld\n",
                (long long)T_devstate[i].burn_hours);

            printf("  last_seen     : %s\n",
                T_devstate[i].last_seen);

            printf("  street_name   : %s\n",
                T_devstate[i].street_name);

            printf("  lamp_type     : %s\n",
                T_devstate[i].lamp_type);

            printf("  rated_watts   : %.2f W\n",
                T_devstate[i].rated_watts);

            printf("  created_at    : %s\n",
                T_devstate[i].created_at);

            printf("  updated_at    : %s\n",
                T_devstate[i].updated_at);
        }
    }


    /*
     * -------------------------------------------------
     * Cantidad de elementos
     * -------------------------------------------------
     */
    int tsize = _dbread_devstate_size();

    printf("\n========================================\n");
    printf("Total de elementos cargados: %d\n", tsize);
    printf("========================================\n");


    /*
     * -------------------------------------------------
     * Buscar dispositivo
     * -------------------------------------------------
     */

    const char *test_light_id = "111";

    printf("\n----------------------------------------\n");
    printf("Buscando dispositivo: %s\n", test_light_id);
    printf("----------------------------------------\n");


    stDb_T_devstate *pdev =
        _Stfind_Devstate(test_light_id);


    if (pdev == NULL)
    {
        printf("Dispositivo NO encontrado: %s\n",
               test_light_id);

        return EXIT_FAILURE;
    }


    /*
     * -------------------------------------------------
     * Mostrar valores actuales
     * -------------------------------------------------
     */

    printf("\nDispositivo encontrado\n");

    printf("  light_id      : %s\n", pdev->light_id);
    printf("  status        : %s\n", pdev->status);
    printf("  dimming       : %d %%\n", pdev->dimming_level);
    printf("  power         : %.2f W\n", pdev->power_watts);
    printf("  voltage       : %.2f V\n", pdev->voltage);
    printf("  temperature   : %.2f C\n", pdev->temperature_c);


    /*
     * -------------------------------------------------
     * Modificar estructura
     * -------------------------------------------------
     */

    printf("\n----------------------------------------\n");
    printf("Modificando dispositivo...\n");
    printf("----------------------------------------\n");

    strcpy(pdev->status, "on");

    pdev->dimming_level = 80;

    pdev->power_watts = 120.50;

    pdev->voltage = 24.00;

    pdev->temperature_c = 42.50;


    /*
     * -------------------------------------------------
     * Mostrar nuevos valores
     * -------------------------------------------------
     */

    printf("\nNuevos valores:\n");

    printf("  light_id      : %s\n", pdev->light_id);
    printf("  status        : %s\n", pdev->status);
    printf("  dimming       : %d %%\n", pdev->dimming_level);
    printf("  power         : %.2f W\n", pdev->power_watts);
    printf("  voltage       : %.2f V\n", pdev->voltage);
    printf("  temperature   : %.2f C\n", pdev->temperature_c);


    /*
     * -------------------------------------------------
     * Actualizar PostgreSQL
     * -------------------------------------------------
     */

    printf("\n----------------------------------------\n");
    printf("Actualizando PostgreSQL...\n");
    printf("----------------------------------------\n");

    ret = _dbwrite_devstate(pdev);

    printf("\nResultado escritura: %d\n", ret);


    if (ret != _DB_STS_OK)
    {
        printf("ERROR actualizando devstate\n");
        return EXIT_FAILURE;
    }


    printf("\nDispositivo actualizado correctamente.\n");


    /*
     * -------------------------------------------------
     * Fin
     * -------------------------------------------------
     */

    printf("\n========================================\n");
    printf("Fin del test\n");
    printf("========================================\n");

    return EXIT_SUCCESS;
}

#endif

