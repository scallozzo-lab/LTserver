#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "rcserver.h"
#include "logdevice.h"

#ifdef _OPT_LOGDEVICE
void _logDevice(const uint8_t *HubID, const char *format, ...)
{
    char hubid[13];
    char dirpath[256];
    char filepath[512];

    if (HubID == NULL || format == NULL)
        return;

    // ---------------------------------------------------------
    // Convertir HubID[6] -> "01A43200FF27"
    // ---------------------------------------------------------
    snprintf(hubid,
             sizeof(hubid),
             "%02X%02X%02X%02X%02X%02X",
             HubID[0],
             HubID[1],
             HubID[2],
             HubID[3],
             HubID[4],
             HubID[5]);

    // ---------------------------------------------------------
    // Crear directorio base "logs"
    // ---------------------------------------------------------
    if (mkdir(_DEVICE_LOG_BASE_DIR, 0755) != 0)
    {
        if (errno != EEXIST)
        {
            printf("[_logDevice] Error creating directory %s\n",
                   _DEVICE_LOG_BASE_DIR);
            return;
        }
    }

    // ---------------------------------------------------------
    // Crear directorio del Hub
    // logs/01A43200FF27
    // ---------------------------------------------------------
    snprintf(dirpath,
             sizeof(dirpath),
             "%s/%s",
             _DEVICE_LOG_BASE_DIR,
             hubid);

    if (mkdir(dirpath, 0755) != 0)
    {
        if (errno != EEXIST)
        {
            printf("[_logDevice] Error creating directory %s\n",
                   dirpath);
            return;
        }
    }

    // ---------------------------------------------------------
    // logs/01A43200FF27/debug.log
    // ---------------------------------------------------------
    snprintf(filepath,
             sizeof(filepath),
             "%s/%s",
             dirpath,
             _DEVICE_LOG_FILENAME);

    FILE *file = fopen(filepath, "a");

    if (file == NULL)
    {
        printf("[_logDevice] Error opening file %s\n", filepath);
        return;
    }

    // ---------------------------------------------------------
    // Timestamp
    // ---------------------------------------------------------
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[80];

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    strftime(buffer,
             sizeof(buffer),
             "%Y-%m-%d %H:%M:%S",
             timeinfo);

    fprintf(file, "[%s] ", buffer);

    // ---------------------------------------------------------
    // Mensaje
    // ---------------------------------------------------------
    va_list args;

    va_start(args, format);
    vfprintf(file, format, args);
    va_end(args);

    fclose(file);
}
#endif