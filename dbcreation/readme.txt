Para crear al base datos:

psql -U postgres

CREATE DATABASE ltdb;

\q

psql -U postgres -d ltdb -f tablas_ltserver1p0.sql

Luego para verificar la base de datos, ejecutar: 
psql -U postgres 
\l
\c ltdb
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
postgres=# \c ltdb
You are now connected to database "ltdb" as user "postgres".
ltdb=# \dt
           List of relations
 Schema |    Name    | Type  |  Owner   
--------+------------+-------+----------
 public | db_version | table | postgres
 public | devices    | table | postgres
 public | sector     | table | postgres
(3 rows)

ltdb=# 



















