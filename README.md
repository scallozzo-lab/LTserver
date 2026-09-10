
# LTserver — Instalación en servidor Ubuntu

Este documento describe el procedimiento para instalar **LTserver** en un servidor Ubuntu desde cero.

Incluye:

* Configuración de acceso SSH mediante clave pública.
* Copia/sincronización de LTserver al servidor.
* Instalación de herramientas de compilación.
* Instalación de `libpq`.
* Instalación y configuración de PostgreSQL 16.
* Creación de la base de datos `lumina`.
* Creación del rol PostgreSQL `scallozzo`.
* Importación de la estructura inicial de la base.
* Compilación y prueba de LTserver.

---

# 1. Configurar acceso SSH

LTserver se administra remotamente mediante SSH.

## 1.1 Verificar la clave SSH en la PC de desarrollo

En la PC:

```bash
ls -la ~/.ssh/
```

La clave pública normalmente será:

```text
~/.ssh/id_ed25519.pub
```

Si todavía no existe una clave:

```bash
ssh-keygen -t ed25519
```

Aceptar las opciones por defecto si no se necesita una configuración especial.

---

## 1.2 Copiar la clave pública al servidor

Desde la PC de desarrollo:

```bash
ssh-copy-id root@IP_SERVIDOR
```

También puede indicarse explícitamente la clave:

```bash
ssh-copy-id -i ~/.ssh/id_ed25519.pub root@IP_SERVIDOR
```

IMPORTANTE: no ejecutar `ssh-copy-id` mediante `sudo`, ya que buscaría las claves SSH del usuario `root` local en lugar de las del usuario actual.

---

## 1.3 Probar acceso

```bash
ssh root@IP_SERVIDOR
```

Si todo está correctamente configurado, no debería solicitar la contraseña del usuario remoto.

---

# 2. Copiar LTserver al servidor

Para una copia simple puede utilizarse `scp`:

```bash
scp -r LTserver root@IP_SERVIDOR:/home/scallozzo/
```

Para desarrollo se recomienda `rsync`, ya que solamente transfiere archivos nuevos o modificados:

```bash
rsync -avz LTserver/ root@IP_SERVIDOR:/home/scallozzo/LTserver/
```

Si se desea mantener una copia espejo:

```bash
rsync -avz --delete LTserver/ root@IP_SERVIDOR:/home/scallozzo/LTserver/
```

ADVERTENCIA: `--delete` elimina en el servidor los archivos que ya no existan en la carpeta local.

---

# 3. Instalar herramientas de compilación

Actualizar la información de paquetes:

```bash
apt update
```

Instalar GCC, G++, Make y herramientas básicas:

```bash
apt install -y build-essential
```

Verificar:

```bash
gcc --version
g++ --version
make --version
```

LTserver fue desarrollado utilizando GCC 11.x en Ubuntu 22.04.

Si se requiere específicamente GCC 11:

```bash
apt install -y gcc-11 g++-11
```

Verificar:

```bash
gcc-11 --version
g++-11 --version
```

---

# 4. Instalar librería de desarrollo PostgreSQL

LTserver utiliza `libpq` para comunicarse con PostgreSQL:

```c
#include <libpq-fe.h>
```

Instalar:

```bash
apt install -y libpq-dev
```

Verificar el header:

```bash
dpkg -L libpq-dev | grep libpq-fe.h
```

Normalmente se encuentra en:

```text
/usr/include/postgresql/libpq-fe.h
```

Para obtener automáticamente las opciones necesarias de compilación:

```bash
pkg-config --cflags --libs libpq
```

Normalmente devuelve parámetros equivalentes a:

```text
-I/usr/include/postgresql -lpq
```

---

# 5. Instalar PostgreSQL 16

La instalación original de LTserver utiliza PostgreSQL 16.

Para comprobar la versión en una instalación existente:

```bash
psql --version
```

Ejemplo utilizado durante la migración:

```text
psql (PostgreSQL) 16.14
```

Instalar PostgreSQL 16:

```bash
apt update
apt install -y postgresql-16 postgresql-client-16
```

Si Ubuntu no encuentra estos paquetes, deberá agregarse previamente el repositorio oficial PostgreSQL PGDG.

Verificar:

```bash
psql --version
```

Comprobar que PostgreSQL esté funcionando:

```bash
systemctl status postgresql
```

Habilitar inicio automático:

```bash
systemctl enable postgresql
```

---

# 6. Ubicación de la configuración PostgreSQL

En PostgreSQL 16 sobre Ubuntu normalmente se encuentra en:

```text
/etc/postgresql/16/main/
```

Los archivos principales son:

```text
postgresql.conf
pg_hba.conf
```

Para obtener la ubicación exacta utilizada por PostgreSQL:

```bash
sudo -u postgres psql -c "SHOW config_file;"
```

Para consultar el archivo de autenticación:

```bash
sudo -u postgres psql -c "SHOW hba_file;"
```

---

# 7. Autenticación local y error "Peer authentication failed"

Si estando conectado como `root` se ejecuta:

```bash
psql -U postgres
```

puede aparecer:

```text
FATAL: Peer authentication failed for user "postgres"
```

Esto es normal cuando PostgreSQL utiliza autenticación `peer` para conexiones mediante socket local.

Para administrar PostgreSQL desde `root`, utilizar:

```bash
sudo -u postgres psql
```

No es necesario modificar la autenticación `peer` para administrar localmente PostgreSQL.

---

# 8. Crear el rol PostgreSQL scallozzo

Entrar como administrador:

```bash
sudo -u postgres psql
```

Crear el usuario:

```sql
CREATE ROLE scallozzo WITH LOGIN PASSWORD 'CAMBIAR_PASSWORD';
```

Opcionalmente, si `scallozzo` será utilizado como usuario administrador del servidor PostgreSQL:

```sql
ALTER ROLE scallozzo WITH SUPERUSER CREATEDB CREATEROLE;
```

Para producción es preferible utilizar los privilegios mínimos necesarios y no hacer que el usuario utilizado por la aplicación sea `SUPERUSER`.

---

# 9. Crear la base de datos Lumina

Desde `psql`:

```sql
CREATE DATABASE lumina OWNER scallozzo;
```

Si la base ya fue creada utilizando `postgres`:

```sql
ALTER DATABASE lumina OWNER TO scallozzo;
```

Entrar en la base:

```text
\c lumina
```

---

# 10. Importar estructura inicial de LTserver

El esquema actual se encuentra en:

```text
LTserver/dbcreation/tablas_LTserver1p6.sql
```

Desde Linux:

```bash
cd /home/scallozzo/LTserver/dbcreation
```

Como la autenticación local utiliza `peer`, importar utilizando el usuario Linux `postgres`:

```bash
sudo -u postgres psql -d lumina -f tablas_LTserver1p6.sql
```

Durante la importación deberían aparecer mensajes como:

```text
CREATE TABLE
CREATE INDEX
INSERT 0 5
INSERT 0 20
...
```

---

# 11. Error "role scallozzo does not exist"

Si durante la importación aparece:

```text
ERROR: role "scallozzo" does not exist
```

significa que el SQL contiene objetos, propietarios o permisos asociados al rol `scallozzo`.

Crear previamente el rol:

```bash
sudo -u postgres psql
```

```sql
CREATE ROLE scallozzo WITH LOGIN PASSWORD 'CAMBIAR_PASSWORD';
```

Luego repetir la importación si fuera necesario.

---

# 12. Convertir scallozzo en propietario de Lumina

Si los objetos fueron creados inicialmente como `postgres`, pueden transferirse a `scallozzo`.

Entrar:

```bash
sudo -u postgres psql -d lumina
```

Ejecutar:

```sql
REASSIGN OWNED BY postgres TO scallozzo;
```

Esto reasigna los objetos pertenecientes a `postgres` dentro de la base actual.

Comprobar tablas y propietarios:

```text
\dt
```

También puede verificarse el esquema:

```text
\dn+
```

Salir:

```text
\q
```

---

# 13. Verificar las tablas de Lumina

Ejecutar:

```bash
sudo -u postgres psql -d lumina -c "\dt"
```

Deberían aparecer las tablas correspondientes al esquema LTserver, incluyendo las utilizadas por la plataforma, por ejemplo:

```text
devices
devstate
devcalendar
zone
alarm
energylog
deviceevent
inspection_events
users
```

La lista exacta depende de la versión del esquema instalada.

---

# 14. Configuración de conexión utilizada por LTserver

Actualmente LTserver utiliza una cadena de conexión `libpq` similar a:

```c
#define _DB_ADDRESS "host=localhost dbname=lumina user=postgres password=lumina"
```

Al especificar:

```text
host=localhost
```

la conexión se realiza mediante TCP/IP y no mediante el socket Unix utilizado por una llamada como:

```bash
psql -U postgres
```

Por este motivo las reglas aplicables de `pg_hba.conf` son las correspondientes a conexiones `host`.

Para probar exactamente el mismo tipo de conexión:

```bash
psql -h localhost -U postgres -d lumina
```

PostgreSQL solicitará la contraseña.

---

# 15. Configuración recomendada para LTserver

Es preferible que LTserver no utilice directamente el usuario administrativo `postgres`.

Una configuración más limpia es utilizar:

```c
#define _DB_ADDRESS "host=localhost dbname=lumina user=scallozzo password=CAMBIAR_PASSWORD"
```

Antes de cambiar LTserver, comprobar manualmente:

```bash
psql -h localhost -U scallozzo -d lumina
```

Si la conexión funciona y el usuario posee permisos sobre las tablas necesarias, LTserver podrá utilizar esa misma configuración.

---

# 16. Comprobar PostgreSQL

Estado del servicio:

```bash
systemctl status postgresql
```

Ver versión real del servidor:

```bash
sudo -u postgres psql -c "SHOW server_version;"
```

Ver puerto configurado:

```bash
sudo -u postgres psql -c "SHOW port;"
```

Verificar que PostgreSQL esté escuchando:

```bash
ss -lntp | grep 5432
```

---

# 17. Compilar LTserver

Ir al directorio correspondiente:

```bash
cd /home/scallozzo/LTserver
```

Si el proyecto utiliza Makefile:

```bash
make
```

Si durante la compilación aparece:

```text
fatal error: libpq-fe.h: No such file or directory
```

comprobar que esté instalado:

```bash
apt install -y libpq-dev
```

y que el Makefile incluya correctamente los flags de PostgreSQL.

Por ejemplo:

```makefile
CFLAGS += -I/usr/include/postgresql
LDLIBS += -lpq
```

o utilizar:

```bash
pkg-config --cflags --libs libpq
```

---

# 18. Prueba final

Antes de iniciar LTserver comprobar:

```bash
systemctl status postgresql
```

Probar conexión:

```bash
psql -h localhost -U scallozzo -d lumina
```

Comprobar tablas:

```sql
\dt
```

Salir:

```text
\q
```

Finalmente iniciar LTserver según el ejecutable generado:

```bash
./LTserver
```

Revisar el log de inicio y comprobar que la conexión con PostgreSQL se establezca correctamente.

---

# 19. Resumen de instalación

En una instalación nueva, el orden recomendado es:

```text
1. Configurar SSH.
2. Copiar LTserver.
3. Instalar build-essential.
4. Instalar libpq-dev.
5. Instalar PostgreSQL 16.
6. Crear el rol scallozzo.
7. Crear la base lumina.
8. Importar tablas_LTserver1p6.sql.
9. Verificar propietarios y permisos.
10. Configurar la conexión de LTserver.
11. Compilar LTserver.
12. Probar conexión con PostgreSQL.
13. Iniciar LTserver.
```

## Datos principales

```text
Proyecto:       LTserver
Base de datos:  lumina
PostgreSQL:     16.x
DB Owner:       scallozzo
DB Port:        5432 (default)
DB Host:        localhost
Schema inicial: tablas_LTserver1p6.sql
```

## Nota de seguridad

No almacenar contraseñas reales en este README ni subirlas al repositorio.

Si actualmente `_DB_ADDRESS` contiene la contraseña directamente en el código fuente, se recomienda posteriormente mover las credenciales a configuración externa o variables de entorno.
