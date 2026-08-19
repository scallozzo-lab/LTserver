rm rcserver
gcc -g rcserver.c aux.c storefiles.c db.c dsvv.c loop2app.c nethubbin.c db_devstate.c db_devevent.c db_version.c db_alarm.c -o rcserver -I/usr/include/postgresql/ -lpq -Xlinker -Map=rcserver.map -Wl,--print-memory-usage
./rcserver


