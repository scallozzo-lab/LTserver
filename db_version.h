#ifndef DB_VERSION_H
    #define DB_VERSION_H

#include <stdint.h>
#include "db.h"

typedef struct
{
    int id;
    char version[11];

} stDb_T_db_version;


/*
 * Lee la versión de la base de datos.
 */
int _dbread_db_version(stDb_T_db_version *db_version);

#endif