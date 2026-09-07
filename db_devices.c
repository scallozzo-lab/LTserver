#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

#include "db.h"
#include "db_devices.h"


stDb_T_devices T_devices[_CANT_MAX_EQ];
stSessionData_T_dev SessionData_T_dev[_CANT_MAX_EQ];
stDb_T_devices_info devices_info;


/*
 * ---------------------------------------------------------
 * Cantidad de dispositivos cargados
 * ---------------------------------------------------------
 */
int _dbread_devices_size(void)
{
    int count = 0;

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        if (T_devices[i].light_id[0] != '\0')
            count++;
    }

    return count;
}


/*
 * ---------------------------------------------------------
 * Buscar dispositivo por light_id
 * ---------------------------------------------------------
 */
stDb_T_devices *_Stfind_Devices(const char *light_id)
{
    if (light_id == NULL)
        return NULL;

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        /*
         * Registro válido
         */
        if (T_devices[i].light_id[0] == '\0')
            continue;

        /*
         * Comparamos light_id
         */
        if (strcmp(T_devices[i].light_id, light_id) == 0)
            return &T_devices[i];
    }

    return NULL;
}

/*
 * ---------------------------------------------------------
 * Buscar dispositivo por eqid
 * ---------------------------------------------------------
 */
stDb_T_devices *_Stfind_Devices_ByEqid(const char *eqid, uint8_t type, int *idx)
{
    
    char tmpeqid[18] = {0};
    if(type) sprintf(tmpeqid, "%02X:%02X:%02X:%02X:%02X:%02X",
                                                         eqid[0],
                                                         eqid[1],
                                                         eqid[2],
                                                         eqid[3],
                                                         eqid[4],
                                                         eqid[5],
                                                         eqid[6]);
     
    if (eqid == NULL)
        return NULL;

    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        /*
         * Registro válido
         */
        if (T_devices[i].light_id[0] == '\0')
            continue;

        /*
         * Comparamos eqid
         */
        // Si el type de eqid es binario
        if(type)
        {
            if (strcmp(T_devices[i].eqid, tmpeqid) == 0)
            {
                if(idx) *idx = i;
                return &T_devices[i];
            }   
        }
        else if (strcmp(T_devices[i].eqid, eqid) == 0)
        {    
            if(idx) *idx = i;
            return &T_devices[i];
        }
    }
    return NULL;
}


/*
 * ---------------------------------------------------------
 * Buscar primer dispositivo de una zona
 * ---------------------------------------------------------
 */
stDb_T_devices *_Stfind_Devices_ByZone(int zone_id)
{
    for (int i = 0; i < _CANT_MAX_EQ; i++)
    {
        /*
         * Registro válido
         */
        if (T_devices[i].light_id[0] == '\0')
            continue;

        /*
         * Comparamos zone_id
         */
        if (T_devices[i].zone_id == zone_id)
            return &T_devices[i];
    }

    return NULL;
}

/*
 * ---------------------------------------------------------
 * Lee la tabla devices completa
 * ---------------------------------------------------------
 */
int _dbread_table_devices(void)
{
    int ret = _DB_STS_OK;

    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbread_table_devices] Connection to database failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_table_devices] Connection to database failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }

    /*
     * -----------------------------------------------------
     * Consulta explícita de las columnas
     * -----------------------------------------------------
     *
     * No usamos SELECT *.
     *
     * El orden de las columnas coincide exactamente
     * con el orden utilizado para cargar la estructura.
     */
    PGresult *res = PQexec(conn,
        "SELECT "
        "light_id, "
        "eqid, "
        "linkid, "
        "hubid, "
        "date_time, "
        "devtype, "
        "serial_number, "
        "_name, "
        "model, "
        "address, "
        "address_number, "
        "intersection, "
        "zipcode, "
        "lat, "
        "lng, "
        "description, "
        "zone_id, "
        "map_loc, "
        "map_pag, "
        "is_enabled, "
        "is_linked, "
        "devconfig "
        "FROM devices"
    );


    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbread_table_devices] Query execution failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_table_devices] Query execution failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    int rows = PQntuples(res);

#ifdef _DEBUG_DB_READ
    printf("[_dbread_table_devices] Rows: %d\n", rows);
#endif


    /*
     * -----------------------------------------------------
     * Verificar cantidad máxima
     * -----------------------------------------------------
     */
    if (rows > _CANT_MAX_EQ)
    {
        printf("[_dbread_table_devices] Error! MAX(%d)\n",
               rows);

        _log("[_dbread_table_devices] Error! MAX(%d)\n",
             rows);

        ret = _DB_STS_ERR_STRUCT;
    }
    else
    {
        int cols = PQnfields(res);

        /*
         * devices tiene 22 columnas.
         */
        if (cols != 22)
        {
#ifdef _DEBUG_DB_READ
            printf("[_dbread_table_devices] Error! Columns (%d)\n",
                   cols);
#endif

            _log("[_dbread_table_devices] Error! Columns (%d)\n",
                 cols);

            ret = _DB_STS_ERR_STRUCT;
        }
        else
        {
            /*
             * -------------------------------------------------
             * Limpiamos la estructura antes de cargarla
             * -------------------------------------------------
             */
            memset(T_devices, 0, sizeof(T_devices));


            /*
             * -------------------------------------------------
             * Cargar registros
             * -------------------------------------------------
             */
            for (int i = 0; i < rows; i++)
            {

#ifdef _DEBUG_DB_READ
                printf("\nrow nr:%d\n", i + 1);
#endif


                /*
                 * -------------------------------------------------
                 * 0 - light_id
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 0))
                {
                    strncpy(T_devices[i].light_id,
                            PQgetvalue(res, i, 0),
                            sizeof(T_devices[i].light_id) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 1 - eqid
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 1))
                {
                    strncpy(T_devices[i].eqid,
                            PQgetvalue(res, i, 1),
                            sizeof(T_devices[i].eqid) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 2 - linkid
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 2))
                {
                    strncpy(T_devices[i].linkid,
                            PQgetvalue(res, i, 2),
                            sizeof(T_devices[i].linkid) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 3 - hubid
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 3))
                {
                    strncpy(T_devices[i].hubid,
                            PQgetvalue(res, i, 3),
                            sizeof(T_devices[i].hubid) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 4 - date_time
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 4))
                {
                    strncpy(T_devices[i].date_time,
                            PQgetvalue(res, i, 4),
                            sizeof(T_devices[i].date_time) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 5 - devtype
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 5))
                {
                    T_devices[i].devtype =
                        atoi(PQgetvalue(res, i, 5));
                }


                /*
                 * -------------------------------------------------
                 * 6 - serial_number
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 6))
                {
                    strncpy(T_devices[i].serial_number,
                            PQgetvalue(res, i, 6),
                            sizeof(T_devices[i].serial_number) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 7 - _name
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 7))
                {
                    strncpy(T_devices[i]._name,
                            PQgetvalue(res, i, 7),
                            sizeof(T_devices[i]._name) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 8 - model
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 8))
                {
                    strncpy(T_devices[i].model,
                            PQgetvalue(res, i, 8),
                            sizeof(T_devices[i].model) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 9 - address
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 9))
                {
                    strncpy(T_devices[i].address,
                            PQgetvalue(res, i, 9),
                            sizeof(T_devices[i].address) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 10 - address_number
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 10))
                {
                    strncpy(T_devices[i].address_number,
                            PQgetvalue(res, i, 10),
                            sizeof(T_devices[i].address_number) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 11 - intersection
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 11))
                {
                    strncpy(T_devices[i].intersection,
                            PQgetvalue(res, i, 11),
                            sizeof(T_devices[i].intersection) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 12 - zipcode
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 12))
                {
                    strncpy(T_devices[i].zipcode,
                            PQgetvalue(res, i, 12),
                            sizeof(T_devices[i].zipcode) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 13 - lat
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 13))
                {
                    T_devices[i].lat =
                        atof(PQgetvalue(res, i, 13));
                }


                /*
                 * -------------------------------------------------
                 * 14 - lng
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 14))
                {
                    T_devices[i].lng =
                        atof(PQgetvalue(res, i, 14));
                }


                /*
                 * -------------------------------------------------
                 * 15 - description
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 15))
                {
                    strncpy(T_devices[i].description,
                            PQgetvalue(res, i, 15),
                            sizeof(T_devices[i].description) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 16 - zone_id
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 16))
                {
                    T_devices[i].zone_id =
                        atoi(PQgetvalue(res, i, 16));
                }


                /*
                 * -------------------------------------------------
                 * 17 - map_loc
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 17))
                {
                    strncpy(T_devices[i].map_loc,
                            PQgetvalue(res, i, 17),
                            sizeof(T_devices[i].map_loc) - 1);
                }


                /*
                 * -------------------------------------------------
                 * 18 - map_pag
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 18))
                {
                    T_devices[i].map_pag =
                        atoi(PQgetvalue(res, i, 18));
                }


                /*
                 * -------------------------------------------------
                 * 19 - is_enabled
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 19))
                {
                    T_devices[i].is_enabled =
                        (strcmp(PQgetvalue(res, i, 19), "t") == 0);
                }


                /*
                 * -------------------------------------------------
                 * 20 - is_linked
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 20))
                {
                    T_devices[i].is_linked =
                        (strcmp(PQgetvalue(res, i, 20), "t") == 0);
                }


                /*
                 * -------------------------------------------------
                 * 21 - devconfig
                 * -------------------------------------------------
                 */
                if (!PQgetisnull(res, i, 21))
                {
                    T_devices[i].devconfig =
                        atoi(PQgetvalue(res, i, 21));
                }


#ifdef _DEBUG_DB_READ

                printf("light_id       : %s\n",
                       T_devices[i].light_id);

                printf("eqid           : %s\n",
                       T_devices[i].eqid);

                printf("linkid         : %s\n",
                       T_devices[i].linkid);

                printf("hubid          : %s\n",
                       T_devices[i].hubid);

                printf("date_time      : %s\n",
                       T_devices[i].date_time);

                printf("devtype        : %d\n",
                       T_devices[i].devtype);

                printf("serial_number  : %s\n",
                       T_devices[i].serial_number);

                printf("_name          : %s\n",
                       T_devices[i]._name);

                printf("model          : %s\n",
                       T_devices[i].model);

                printf("address        : %s\n",
                       T_devices[i].address);

                printf("address_number : %s\n",
                       T_devices[i].address_number);

                printf("intersection   : %s\n",
                       T_devices[i].intersection);

                printf("zipcode        : %s\n",
                       T_devices[i].zipcode);

                printf("lat            : %.6f\n",
                       T_devices[i].lat);

                printf("lng            : %.6f\n",
                       T_devices[i].lng);

                printf("description    : %s\n",
                       T_devices[i].description);

                printf("zone_id        : %d\n",
                       T_devices[i].zone_id);

                printf("map_loc        : %s\n",
                       T_devices[i].map_loc);

                printf("map_pag        : %d\n",
                       T_devices[i].map_pag);

                printf("is_enabled     : %s\n",
                       T_devices[i].is_enabled ?
                       "true" : "false");

                printf("is_linked      : %s\n",
                       T_devices[i].is_linked ?
                       "true" : "false");

                printf("devconfig      : %d\n",
                       T_devices[i].devconfig);

#endif

            }
        }
    }


    /*
     * -----------------------------------------------------
     * Clean up
     * -----------------------------------------------------
     */
    PQclear(res);
    PQfinish(conn);

    return ret;
}

/*
 * ---------------------------------------------------------
 * Verifica si la tabla devices cambió
 * ---------------------------------------------------------
 *
 * Comparamos:
 *
 *   1. Cantidad de registros
 *   2. MAX(date_time)
 *
 * Si cualquiera de los dos cambia, consideramos que
 * la tabla fue modificada.
 *
 * ---------------------------------------------------------
 */
int _dbcheck_devices(stDb_T_devices_info *info)
{
    if (info == NULL)
        return _DB_STS_ERR_STRUCT;


    PGconn *conn = PQconnectdb(_DB_ADDRESS);


    /*
     * -----------------------------------------------------
     * Verificar conexión
     * -----------------------------------------------------
     */

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbcheck_devices] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbcheck_devices] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    /*
     * -----------------------------------------------------
     * Consulta
     * -----------------------------------------------------
     */

    const char *query =
        "SELECT COUNT(*), "
        "       MAX(date_time) "
        "FROM devices";


    PGresult *res = PQexec(conn, query);


    /*
     * -----------------------------------------------------
     * Verificar resultado
     * -----------------------------------------------------
     */

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbcheck_devices] Query failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbcheck_devices] Query failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    /*
     * -----------------------------------------------------
     * Verificar estructura del resultado
     * -----------------------------------------------------
     */

    if (PQntuples(res) != 1 ||
        PQnfields(res) != 2)
    {
        printf("[_dbcheck_devices] Invalid result\n");

        _log("[_dbcheck_devices] Invalid result\n");

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * -----------------------------------------------------
     * COUNT(*)
     * -----------------------------------------------------
     */

    int db_count = 0;


    if (!PQgetisnull(res, 0, 0))
    {
        db_count =
            atoi(PQgetvalue(res, 0, 0));
    }


    /*
     * -----------------------------------------------------
     * MAX(date_time)
     * -----------------------------------------------------
     */

    char db_last_update[32];

    memset(db_last_update,
           0,
           sizeof(db_last_update));


    if (!PQgetisnull(res, 0, 1))
    {
        strncpy(db_last_update,
                PQgetvalue(res, 0, 1),
                sizeof(db_last_update) - 1);

        db_last_update[
            sizeof(db_last_update) - 1
        ] = '\0';
    }


#ifdef _DEBUG_DB_READ

    printf("\n[_dbcheck_devices]\n");

    printf("  DB count       : %d\n",
           db_count);

    printf("  DB last_update : %s\n",
           db_last_update);

    printf("  RAM count      : %d\n",
           info->count);

    printf("  RAM last_update: %s\n\n",
           info->last_update);

#endif


    /*
     * -----------------------------------------------------
     * Comparar contra el estado conocido en RAM
     * -----------------------------------------------------
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
     * -----------------------------------------------------
     * Si hubo cambios, actualizar información RAM
     * -----------------------------------------------------
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

        printf("  --> DEVICES CHANGED\n");

#endif
    }


    /*
     * -----------------------------------------------------
     * Clean up
     * -----------------------------------------------------
     */

    PQclear(res);
    PQfinish(conn);


    return changed ?
           _DB_STS_CHANGED :
           _DB_STS_OK;
}

/*
 * ---------------------------------------------------------
 * Inicialización
 * ---------------------------------------------------------
 */
void _InitDb_devices(void)
{
    memset(T_devices, 0, sizeof(T_devices));
    memset(&devices_info, 0, sizeof(devices_info));
    memset(&SessionData_T_dev, 0, sizeof(SessionData_T_dev));

    _dbread_table_devices();

    devices_info.count =
        _dbread_devices_size();
}


#ifdef _TEST_DB_DEVICES

    #include <stdio.h>
    #include <stdlib.h>

    int main(void)
    {
        printf("========================================\n");
        printf("        TEST DB DEVICES\n");
        printf("========================================\n\n");


        /*
        * -------------------------------------------------
        * Leer tabla devices
        * -------------------------------------------------
        */

        int ret = _dbread_table_devices();

        printf("\nResultado lectura: %d\n", ret);

        if (ret != _DB_STS_OK)
        {
            printf("Error leyendo tabla devices\n");
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
            * light_id es PRIMARY KEY.
            *
            * Lo utilizamos para determinar si el
            * registro es válido.
            */
            if (T_devices[i].light_id[0] != '\0')
            {
                printf("\nRegistro %d\n", i);

                printf("  light_id       : %s\n",
                    T_devices[i].light_id);

                printf("  eqid           : %s\n",
                    T_devices[i].eqid);

                printf("  linkid         : %s\n",
                    T_devices[i].linkid);

                printf("  hubid          : %s\n",
                    T_devices[i].hubid);

                printf("  date_time      : %s\n",
                    T_devices[i].date_time);

                printf("  devtype        : %d\n",
                    T_devices[i].devtype);

                printf("  serial_number  : %s\n",
                    T_devices[i].serial_number);

                printf("  _name          : %s\n",
                    T_devices[i]._name);

                printf("  model          : %s\n",
                    T_devices[i].model);

                printf("  address        : %s\n",
                    T_devices[i].address);

                printf("  address_number : %s\n",
                    T_devices[i].address_number);

                printf("  intersection   : %s\n",
                    T_devices[i].intersection);

                printf("  zipcode        : %s\n",
                    T_devices[i].zipcode);

                printf("  lat            : %.6f\n",
                    T_devices[i].lat);

                printf("  lng            : %.6f\n",
                    T_devices[i].lng);

                printf("  description    : %s\n",
                    T_devices[i].description);

                printf("  zone_id        : %d\n",
                    T_devices[i].zone_id);

                printf("  map_loc        : %s\n",
                    T_devices[i].map_loc);

                printf("  map_pag        : %d\n",
                    T_devices[i].map_pag);

                printf("  is_enabled     : %s\n",
                    T_devices[i].is_enabled ?
                    "true" : "false");

                printf("  is_linked      : %s\n",
                    T_devices[i].is_linked ?
                    "true" : "false");

                printf("  devconfig      : %d\n",
                    T_devices[i].devconfig);
            }
        }


        /*
        * -------------------------------------------------
        * Cantidad de elementos
        * -------------------------------------------------
        */

        int tsize = _dbread_devices_size();

        printf("\n========================================\n");
        printf("Total de elementos cargados: %d\n", tsize);
        printf("========================================\n");


        /*
        * -------------------------------------------------
        * Buscar dispositivo por light_id
        * -------------------------------------------------
        */

        const char *test_light_id = "SL-001";

        printf("\n----------------------------------------\n");
        printf("Buscando por light_id: %s\n",
            test_light_id);
        printf("----------------------------------------\n");


        stDb_T_devices *pdev =
            _Stfind_Devices(test_light_id);


        if (pdev == NULL)
        {
            printf("Dispositivo NO encontrado: %s\n",
                test_light_id);

            return EXIT_FAILURE;
        }


        /*
        * -------------------------------------------------
        * Mostrar dispositivo encontrado
        * -------------------------------------------------
        */

        printf("\nDispositivo encontrado\n");

        printf("  light_id       : %s\n",
            pdev->light_id);

        printf("  eqid           : %s\n",
            pdev->eqid);

        printf("  serial_number  : %s\n",
            pdev->serial_number);

        printf("  _name          : %s\n",
            pdev->_name);

        printf("  model          : %s\n",
            pdev->model);

        printf("  address        : %s\n",
            pdev->address);

        printf("  address_number : %s\n",
            pdev->address_number);

        printf("  zone_id        : %d\n",
            pdev->zone_id);

        printf("  map_loc        : %s\n",
            pdev->map_loc);

        printf("  is_enabled     : %s\n",
            pdev->is_enabled ?
            "true" : "false");

        printf("  is_linked      : %s\n",
            pdev->is_linked ?
            "true" : "false");


        /*
        * -------------------------------------------------
        * Buscar por eqid
        * -------------------------------------------------
        */

        printf("\n----------------------------------------\n");
        printf("Buscando por eqid: %s\n",
            pdev->eqid);
        printf("----------------------------------------\n");


        stDb_T_devices *pdev_eqid =
            _Stfind_Devices_ByEqid(pdev->eqid);

        if (pdev_eqid == NULL)
        {
            printf("Dispositivo NO encontrado por eqid\n");

            return EXIT_FAILURE;
        }


        printf("\nDispositivo encontrado por eqid\n");

        printf("  eqid           : %s\n",
            pdev_eqid->eqid);

        printf("  light_id       : %s\n",
            pdev_eqid->light_id);

        printf("  _name          : %s\n",
            pdev_eqid->_name);


        /*
        * -------------------------------------------------
        * Buscar por zona
        * -------------------------------------------------
        */

        int test_zone_id = 1;

        printf("\n----------------------------------------\n");
        printf("Buscando dispositivo en zone_id: %d\n",
            test_zone_id);
        printf("----------------------------------------\n");


        stDb_T_devices *pdev_zone =
            _Stfind_Devices_ByZone(test_zone_id);


        if (pdev_zone == NULL)
        {
            printf("No se encontraron dispositivos en zona %d\n",
                test_zone_id);
        }
        else
        {
            printf("\nPrimer dispositivo encontrado en la zona\n");

            printf("  light_id       : %s\n",
                pdev_zone->light_id);

            printf("  eqid           : %s\n",
                pdev_zone->eqid);

            printf("  _name          : %s\n",
                pdev_zone->_name);

            printf("  zone_id        : %d\n",
                pdev_zone->zone_id);
        }


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