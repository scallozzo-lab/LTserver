Para crear al base datos:

psql -U postgres

CREATE DATABASE lumina;

\q

psql -U postgres -d lumina -f tablas_ltserver1p6.sql

Luego para verificar la base de datos, ejecutar: 
psql -U postgres 
\l
\c lumina
\dt

Dar acceso a todo:

GRANT SELECT ON TABLE db_version TO scallozzo;
GRANT SELECT ON TABLE sector TO scallozzo;
GRANT SELECT ON TABLE devices TO scallozzo;
GRANT SELECT ON TABLE state TO scallozzo;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE sector TO scallozzo;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE devices TO scallozzo;
GRANT SELECT, INSERT, UPDATE, DELETE ON TABLE state TO scallozzo;

--------------------------------------------------
postgres=# \l
postgres=# \l
postgres=# \c lumina
You are now connected to database "lumina" as user "postgres".
lumina=# \dt
           List of relations
 Schema |    Name    | Type  |  Owner   
--------+------------+-------+----------
 public | db_version | table | postgres
 public | devices    | table | postgres
 public | sector     | table | postgres
(3 rows)

lumina=# 



















