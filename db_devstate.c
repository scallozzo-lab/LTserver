#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>
#include "db.h"
#include "db_devstate.h"


stDb_T_devstate T_devstate[_CANT_MAX_EQ];


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

/*
 * Lee la tabla devstate completa
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
     *
     * No usamos SELECT * para que el orden de los campos
     * que cargamos quede perfectamente definido.
     */
    PGresult *res = PQexec(conn,
        "SELECT "
        "light_id, "
        "zone_id, "
        "lat, "
        "lng, "
        "status, "
        "dimming_level, "
        "power_watts, "
        "voltage, "
        "temperature_c, "
        "burn_hours, "
        "last_seen, "
        "street_name, "
        "lamp_type, "
        "rated_watts, "
        "created_at, "
        "updated_at "
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


    /*
     * Verificamos que la cantidad de dispositivos
     * no supere el espacio disponible.
     */
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
         * devstate tiene 16 columnas.
         */
        if (cols != 16)
        {

#ifdef _DEBUG_DB_READ
            printf("[_dbread_table_devstate] Error! Columns (%d)\r\n", cols);
#endif
            _log("[_dbread_table_devstate] Error! Columns (%d)\r\n", cols);

            ret = _DB_STS_ERR_STRUCT;
        }
        else
        {
            /*
             * Limpiamos la estructura antes de cargarla.
             */
            memset(T_devstate, 0, sizeof(T_devstate));


            for (int i = 0; i < rows; i++)
            {

#ifdef _DEBUG_DB_READ
                printf("\nrow nr:%d\n", i + 1);
#endif

                /*
                 * -------------------------------------------------
                 * light_id
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 0))
                {
                    strncpy(T_devstate[i].light_id,
                            PQgetvalue(res, i, 0),
                            sizeof(T_devstate[i].light_id) - 1);
                }


                /*
                 * -------------------------------------------------
                 * zone_id
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 1))
                {
                    T_devstate[i].zone_id =
                        atoi(PQgetvalue(res, i, 1));
                }


                /*
                 * -------------------------------------------------
                 * lat
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 2))
                {
                    T_devstate[i].lat =
                        atof(PQgetvalue(res, i, 2));
                }


                /*
                 * -------------------------------------------------
                 * lng
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 3))
                {
                    T_devstate[i].lng =
                        atof(PQgetvalue(res, i, 3));
                }


                /*
                 * -------------------------------------------------
                 * status
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 4))
                {
                    strncpy(T_devstate[i].status,
                            PQgetvalue(res, i, 4),
                            sizeof(T_devstate[i].status) - 1);
                }


                /*
                 * -------------------------------------------------
                 * dimming_level
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 5))
                {
                    T_devstate[i].dimming_level =
                        atoi(PQgetvalue(res, i, 5));
                }


                /*
                 * -------------------------------------------------
                 * power_watts
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 6))
                {
                    T_devstate[i].power_watts =
                        atof(PQgetvalue(res, i, 6));
                }


                /*
                 * -------------------------------------------------
                 * voltage
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 7))
                {
                    T_devstate[i].voltage =
                        atof(PQgetvalue(res, i, 7));
                }


                /*
                 * -------------------------------------------------
                 * temperature_c
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 8))
                {
                    T_devstate[i].temperature_c =
                        atof(PQgetvalue(res, i, 8));
                }


                /*
                 * -------------------------------------------------
                 * burn_hours
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 9))
                {
                    T_devstate[i].burn_hours =
                        atoll(PQgetvalue(res, i, 9));
                }


                /*
                 * -------------------------------------------------
                 * last_seen
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 10))
                {
                    strncpy(T_devstate[i].last_seen,
                            PQgetvalue(res, i, 10),
                            sizeof(T_devstate[i].last_seen) - 1);
                }


                /*
                 * -------------------------------------------------
                 * street_name
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 11))
                {
                    strncpy(T_devstate[i].street_name,
                            PQgetvalue(res, i, 11),
                            sizeof(T_devstate[i].street_name) - 1);
                }


                /*
                 * -------------------------------------------------
                 * lamp_type
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 12))
                {
                    strncpy(T_devstate[i].lamp_type,
                            PQgetvalue(res, i, 12),
                            sizeof(T_devstate[i].lamp_type) - 1);
                }


                /*
                 * -------------------------------------------------
                 * rated_watts
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 13))
                {
                    T_devstate[i].rated_watts =
                        atof(PQgetvalue(res, i, 13));
                }


                /*
                 * -------------------------------------------------
                 * created_at
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 14))
                {
                    strncpy(T_devstate[i].created_at,
                            PQgetvalue(res, i, 14),
                            sizeof(T_devstate[i].created_at) - 1);
                }


                /*
                 * -------------------------------------------------
                 * updated_at
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 15))
                {
                    strncpy(T_devstate[i].updated_at,
                            PQgetvalue(res, i, 15),
                            sizeof(T_devstate[i].updated_at) - 1);
                }


#ifdef _DEBUG_DB_READ
                printf("light_id       : %s\n", T_devstate[i].light_id);
                printf("zone_id        : %d\n", T_devstate[i].zone_id);
                printf("lat            : %.6f\n", T_devstate[i].lat);
                printf("lng            : %.6f\n", T_devstate[i].lng);
                printf("status         : %s\n", T_devstate[i].status);
                printf("dimming_level  : %d\n", T_devstate[i].dimming_level);
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


    /*
     * Clean up
     */
    PQclear(res);
    PQfinish(conn);

    return ret;
}


#ifdef _TEST_DB_DEVSTATE

    int main(void)
    {
        printf("========================================\n");
        printf("       TEST DB DEVSTATE\n");
        printf("========================================\n\n");

        int ret = _dbread_table_devstate();

        printf("\nResultado: %d\n", ret);

        if (ret != _DB_STS_OK)
        {
            printf("Error leyendo tabla devstate\n");
            return EXIT_FAILURE;
        }

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

        int tsize = _dbread_devstate_size();

        printf("\n========================================\n");
        printf("Total de elementos cargados %d\n", tsize);
        printf("Fin del test\n");
        printf("========================================\n");

        return EXIT_SUCCESS;
    }

#endif

