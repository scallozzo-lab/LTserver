#ifndef __LOGDEVICE_H__
    #define __LOGDEVICE_H__

#define _DEVICE_LOG_BASE_DIR    "logs"
#define _DEVICE_LOG_FILENAME    "debug.log"

#ifndef _OPT_LOGDEVICE
    #define _logDevice(...)    ((void)0)
#else
    void _logDevice(const uint8_t *HubID, const char *format, ...);
#endif

#endif