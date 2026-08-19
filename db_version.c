#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

#include "db_version.h"
#include "db.h"


int _dbread_db_version(stDb_T_db_version *db_version)
{
    if (db_version == NULL)
        return _DB_STS_ERR_STRUCT;


    /*
     * Limpiamos la estructura.
     */
    memset(db_version,
           0,
           sizeof(stDb_T_db_version));


    /*
     * Conexión a PostgreSQL
     */
    PGconn *conn = PQconnectdb(_DB_ADDRESS);

    if (PQstatus(conn) != CONNECTION_OK)
    {
        fprintf(stderr,
                "[_dbread_db_version] Connection failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_db_version] Connection failed: %s\n",
             PQerrorMessage(conn));

        PQfinish(conn);

        return _DB_STS_ERR_CONN;
    }


    /*
     * Como db_version debe contener una única versión,
     * tomamos el registro existente.
     */
    const char *query =
        "SELECT id, version "
        "FROM db_version";


    PGresult *res = PQexec(conn, query);


    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        fprintf(stderr,
                "[_dbread_db_version] Query failed: %s\n",
                PQerrorMessage(conn));

        _log("[_dbread_db_version] Query failed: %s\n",
             PQerrorMessage(conn));

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_CMD;
    }


    /*
     * Verificamos cantidad de registros.
     */
    int rows = PQntuples(res);

    if (rows == 0)
    {
        printf("[_dbread_db_version] No version found\n");

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    if (rows != 1)
    {
        printf("[_dbread_db_version] Error! Multiple versions (%d)\n",
               rows);

        _log("[_dbread_db_version] Error! Multiple versions (%d)\n",
             rows);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * Verificación de columnas.
     */
    int cols = PQnfields(res);

    if (cols != 2)
    {
        printf("[_dbread_db_version] Error! Columns (%d)\n",
               cols);

        _log("[_dbread_db_version] Error! Columns (%d)\n",
             cols);

        PQclear(res);
        PQfinish(conn);

        return _DB_STS_ERR_STRUCT;
    }


    /*
     * id
     */
    if (!PQgetisnull(res, 0, 0))
    {
        db_version->id =
            atoi(PQgetvalue(res, 0, 0));
    }


    /*
     * version
     */
    if (!PQgetisnull(res, 0, 1))
    {
        strncpy(db_version->version,
                PQgetvalue(res, 0, 1),
                sizeof(db_version->version) - 1);

        db_version->version[
            sizeof(db_version->version) - 1
        ] = '\0';
    }


#ifdef _DEBUG_DB_READ

    printf("\n[_dbread_db_version]\n");

    printf("  id      : %d\n",
           db_version->id);

    printf("  version : %s\n",
           db_version->version);

#endif


    /*
     * Clean up
     */
    PQclear(res);
    PQfinish(conn);


    return _DB_STS_OK;
}

#include <stdio.h>
#include <stdlib.h>

#include "db_version.h"

#ifdef _TEST_DB_VERSION
int main(void)
{
    stDb_T_db_version db_version;

    printf("========================================\n");
    printf("       TEST DB VERSION\n");
    printf("========================================\n\n");


    int ret = _dbread_db_version(&db_version);


    printf("\nResultado: %d\n", ret);


    if (ret != _DB_STS_OK)
    {
        printf("Error leyendo db_version\n");
        return EXIT_FAILURE;
    }


    printf("\n----------------------------------------\n");
    printf("DB Version\n");
    printf("----------------------------------------\n");

    printf("ID      : %d\n",
           db_version.id);

    printf("Version : %s\n",
           db_version.version);


    printf("\n========================================\n");
    printf("Fin del test\n");
    printf("========================================\n");


    return EXIT_SUCCESS;
}
#endif
