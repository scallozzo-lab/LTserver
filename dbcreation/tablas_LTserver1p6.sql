-- =========================================================
-- LTServer DB Ver 1.6 DATABASE
-- PostgreSQL Script

-- VERSION 1.6 ->
--  - Agregada tabla de dispositivos  

-- VERSION 1.5 ->
--  - Se agrega tabla district 
--  - Se modifica tabla zone, se agregan los campos (center_lat, center_lng, default_zoom y created_at) y se referencia a district
--  Nota: La finalidad de estos cambios es poder a futuro definir el districto y asignarle los zones_id correspondientes. 
--  - Se agrega la tabla inspection_events para el recorrido manual
-- 
-- VERSION 1.4 ->  
--   - agregada tabla: zone esto sirve para luego hacer un tablero de comando, historial y estadísticas por zona geográfica con mucha mas precisión. de como estaban antes que era solo un campo de texto en cada tabla. ahora se normaliza y se hace referencia a esta nueva tabla, no se nota la diferencia en esta instancia pero más adelante si.
--   - devstate.zone (VARCHAR) reemplazado por devstate.zone_id (FK -> zone)
--   - alarm.zone (VARCHAR) reemplazado por alarm.zone_id (FK -> zone, SET NULL)
--   - deviceevent.zone (VARCHAR) reemplazado por deviceevent.zone_id (FK -> zone, SET NULL)
--   - energylog.zone (VARCHAR) reemplazado por energylog.zone_id (FK -> zone)
-- =========================================================

DROP TABLE IF EXISTS energylog CASCADE;
DROP TABLE IF EXISTS inspection_events CASCADE;
DROP TABLE IF EXISTS deviceevent CASCADE;
DROP TABLE IF EXISTS alarm CASCADE;
-- Version 1.6
DROP TABLE IF EXISTS devices CASCADE;

DROP TABLE IF EXISTS devstate CASCADE;
DROP TABLE IF EXISTS zone CASCADE;
DROP TABLE IF EXISTS users CASCADE;
DROP TABLE IF EXISTS db_version CASCADE;
DROP TABLE IF EXISTS district CASCADE;


DROP TYPE IF EXISTS light_status CASCADE;
DROP TYPE IF EXISTS lamp_type_enum CASCADE;
DROP TYPE IF EXISTS alarm_type_enum CASCADE;
DROP TYPE IF EXISTS event_type_enum CASCADE;
DROP TYPE IF EXISTS severity_enum CASCADE;

-- =========================================================
-- ENUM TYPES
-- =========================================================

CREATE TYPE light_status AS ENUM (
    'on',
    'off',
    'fault',
    'maintenance',
    'structure'
);

CREATE TYPE lamp_type_enum AS ENUM (
    'LED',
    'HPS',
    'MH'
);

CREATE TYPE alarm_type_enum AS ENUM (
    'lamp_fault',
    'driver_fault',
    'communication_lost',
    'over_temperature',
    'voltage_anomaly',
    'power_outage'
);

CREATE TYPE event_type_enum AS ENUM (
    'power_failure',
    'circuit_disconnected',
    'voltage_spike',
    'overcurrent',
    'communication_lost',
    'lamp_end_of_life',
    'driver_overheat',
    'breaker_trip'
);

CREATE TYPE severity_enum AS ENUM (
    'critical',
    'high',
    'medium',
    'low'
);

-- =========================================================
-- TABLE: db_version
-- =========================================================
CREATE EXTENSION IF NOT EXISTS pgcrypto;

CREATE TABLE db_version (
    id      SERIAL PRIMARY KEY,
    version VARCHAR(10) NOT NULL
);
INSERT INTO db_version (version) VALUES ('1.6');

-- =========================================================
-- TABLE: users
-- =========================================================
CREATE TABLE users (
    id            UUID PRIMARY KEY DEFAULT gen_random_uuid(),
    username      VARCHAR(64) UNIQUE NOT NULL,
    password_hash TEXT NOT NULL,
    role          VARCHAR(32) DEFAULT 'operator',
    created_at    TIMESTAMP NOT NULL DEFAULT NOW()
);

-- =========================================================
-- TABLE: district
-- =========================================================
CREATE TABLE district (
    id              SERIAL PRIMARY KEY,

    name            VARCHAR(100) UNIQUE NOT NULL,

    center_lat      DOUBLE PRECISION,
    center_lng      DOUBLE PRECISION,
    default_zoom    INTEGER DEFAULT 14,

    created_at      TIMESTAMP DEFAULT NOW()
);

-- =========================================================
-- TABLE: zone
-- =========================================================
CREATE TABLE zone (
    id              SERIAL PRIMARY KEY,

    district_id     INTEGER NOT NULL,

    name            VARCHAR(50) NOT NULL,
    description     VARCHAR(255),

    center_lat      DOUBLE PRECISION,
    center_lng      DOUBLE PRECISION,
    default_zoom    INTEGER DEFAULT 15,

    created_at      TIMESTAMP DEFAULT NOW(),

    CONSTRAINT uq_zone_district_name
        UNIQUE (district_id, name),

    FOREIGN KEY (district_id)
        REFERENCES district(id)
        ON DELETE CASCADE
);

-- =========================================================
-- TABLE: devices (Version 1.6)
-- Device configuration / network information
-- =========================================================

CREATE TABLE devices (

    light_id VARCHAR(50) NOT NULL UNIQUE,
    
    eqid char(17) NOT NULL,
        -- Dirección de red
        -- Formato: 12:34:56:78:9A:BC

    linkid char(17) NOT NULL,
        -- Dirección del master
        -- Solo si is_linked = true

    hubid char(17) NOT NULL,
        -- ID de la unidad central repetidora

    date_time timestamptz NOT NULL DEFAULT now(),

    devtype int2 NOT NULL,
        -- 0-63   : Luminaria
        -- 64-127 : SIT
        -- 128-255: Sistema de infracciones

    serial_number varchar(32) NOT NULL,

    _name varchar(32) NOT NULL,

    model varchar(16) NOT NULL,

    address varchar(20) NOT NULL,

    address_number varchar(10) NOT NULL,

    intersection varchar(32) NOT NULL,

    zipcode varchar(16) NOT NULL,
 
    lat	DOUBLE PRECISION,
    lng	DOUBLE PRECISION,

    description varchar(32),

    zone_id int4 NOT NULL,
        -- Zona geográfica a la que pertenece
        -- El municipio se obtiene mediante zone.district_id

    map_loc char(32) NOT NULL,
        -- Localidad / Ciudad

    map_pag int2 NOT NULL,
        -- Página dentro del mapa

    is_enabled bool NOT NULL,

    is_linked bool NOT NULL,

    devconfig int NOT NULL,
        -- Configuración del dispositivo (bitcode)
        

    CONSTRAINT pk_devices
        PRIMARY KEY (light_id),

    CONSTRAINT uq_devices_eqid
        UNIQUE (eqid),

    CONSTRAINT fk_devices_zone
        FOREIGN KEY (zone_id)
        REFERENCES zone(id)
        ON DELETE RESTRICT
        
);

-- =========================================================
-- TABLE: devstate
-- Current state of each luminaire (one row per device)
-- =========================================================
CREATE TABLE devstate (
    light_id        VARCHAR(50) PRIMARY KEY,

    zone_id         INTEGER NOT NULL,

    lat             DOUBLE PRECISION,
    lng             DOUBLE PRECISION,

    status          light_status NOT NULL DEFAULT 'off',

    dimming_level   INTEGER CHECK (
                        dimming_level >= 0
                        AND dimming_level <= 100
                    ),

    power_watts     NUMERIC(10,2),
    voltage         NUMERIC(10,2),
    temperature_c   NUMERIC(10,2),
    burn_hours      BIGINT DEFAULT 0,
    last_seen       TIMESTAMP,
    street_name     VARCHAR(255),
    lamp_type       lamp_type_enum NOT NULL DEFAULT 'LED',
    rated_watts     NUMERIC(10,2) DEFAULT 150,
    created_at      TIMESTAMP DEFAULT NOW(),
    updated_at      TIMESTAMP DEFAULT NOW(),

   -- CONSTRAINT fk_devstate_zone
   --     FOREIGN KEY (zone_id)
   --     REFERENCES zone(id)
   --     ON DELETE RESTRICT
        
    CONSTRAINT fk_devstate_device
        FOREIGN KEY (light_id)
        REFERENCES devices(light_id)
        ON DELETE CASCADE
        
);

-- =========================================================
-- TABLE: alarm
-- Active or historical alarms
-- =========================================================
CREATE TABLE alarm (
    id               BIGSERIAL PRIMARY KEY,

    light_id         VARCHAR(50) NOT NULL,
    street_name      VARCHAR(255),
    zone_id          INTEGER,
    alarm_type       alarm_type_enum NOT NULL,
    severity         severity_enum NOT NULL,
    message          TEXT,
    acknowledged     BOOLEAN DEFAULT FALSE,
    acknowledged_by  VARCHAR(100),
    acknowledged_at  TIMESTAMP,
    resolved         BOOLEAN DEFAULT FALSE,
    resolved_at      TIMESTAMP,
    created_at       TIMESTAMP DEFAULT NOW(),

    CONSTRAINT fk_alarm_light
        FOREIGN KEY (light_id)
        REFERENCES devstate(light_id)
        ON DELETE CASCADE,

    CONSTRAINT fk_alarm_zone
        FOREIGN KEY (zone_id)
        REFERENCES zone(id)
        ON DELETE SET NULL
);

-- =========================================================
-- TABLE: deviceevent
-- Historical device/system events
-- =========================================================
CREATE TABLE deviceevent (
    id              BIGSERIAL PRIMARY KEY,

    light_id        VARCHAR(50) NOT NULL,
    event_code      VARCHAR(50),
    event_type      event_type_enum NOT NULL,
    severity        severity_enum NOT NULL,
    message         TEXT,
    zone_id         INTEGER,
    theoretical_kw  NUMERIC(10,3),
    real_kw         NUMERIC(10,3),
    voltage_v       NUMERIC(10,2),
    current_ma      NUMERIC(10,2),
    resolved        BOOLEAN DEFAULT FALSE,
    created_at      TIMESTAMP DEFAULT NOW(),

    CONSTRAINT fk_event_light
        FOREIGN KEY (light_id)
        REFERENCES devstate(light_id)
        ON DELETE CASCADE,

    CONSTRAINT fk_event_zone
        FOREIGN KEY (zone_id)
        REFERENCES zone(id)
        ON DELETE SET NULL
);

-- =========================================================
-- TABLE: energylog
-- Aggregated energy statistics per zone
-- =========================================================
CREATE TABLE energylog (
    id            BIGSERIAL PRIMARY KEY,

    zone_id       INTEGER NOT NULL,

    recorded_at   TIMESTAMP NOT NULL,
    total_kwh     NUMERIC(12,3),
    active_lights INTEGER,
    avg_dimming   NUMERIC(5,2),
    faults_count  INTEGER,
    co2_saved_kg  NUMERIC(12,3),

    CONSTRAINT fk_energylog_zone
        FOREIGN KEY (zone_id)
        REFERENCES zone(id)
        ON DELETE RESTRICT
);

-- =========================================================
-- TABLE: inspection_events
-- Aggregated data from manual inspections
-- =========================================================
CREATE TABLE inspection_events
(
    id SERIAL PRIMARY KEY,
    light_id VARCHAR(50) NOT NULL UNIQUE,
    event_type VARCHAR(30) NOT NULL,
    latitude DOUBLE PRECISION NOT NULL,
    longitude DOUBLE PRECISION NOT NULL,
    accuracy_m DOUBLE PRECISION,
    comments TEXT,
    event_time TIMESTAMP DEFAULT NOW()
);

-- =========================================================
-- INDEXES
-- =========================================================
CREATE INDEX idx_zone_district_id    ON zone(district_id);
CREATE INDEX idx_devstate_zone_id    ON devstate(zone_id);
-- Version 1.6
CREATE INDEX "IX_devices_zone_id"    ON devices USING btree (zone_id);

CREATE INDEX idx_devstate_status     ON devstate(status);

CREATE INDEX idx_alarm_light_id      ON alarm(light_id);
CREATE INDEX idx_alarm_zone_id       ON alarm(zone_id);
CREATE INDEX idx_alarm_resolved      ON alarm(resolved);
CREATE INDEX idx_alarm_acknowledged  ON alarm(acknowledged);
CREATE INDEX idx_alarm_created_at    ON alarm(created_at);

CREATE INDEX idx_deviceevent_light_id   ON deviceevent(light_id);
CREATE INDEX idx_deviceevent_zone_id    ON deviceevent(zone_id);
CREATE INDEX idx_deviceevent_created_at ON deviceevent(created_at);
CREATE INDEX idx_deviceevent_resolved   ON deviceevent(resolved);

CREATE INDEX idx_energylog_zone_id    ON energylog(zone_id);
CREATE INDEX idx_energylog_recorded_at ON energylog(recorded_at);

-- =========================================================
-- SAMPLE DATA - ZONE
-- =========================================================

--INSERT INTO zone (name, description) VALUES
--('Zone-A', 'Main Street corridor'),
--('Zone-B', 'Oak Avenue sector'),
--('Zone-C', 'Pine Street sector'),
--('Zone-D', 'Maple Road sector'),
--('Zone-E', 'Liberty Avenue sector');

-- =========================================================
-- DISTRICTS
-- =========================================================

INSERT INTO district (
    name,
    center_lat,
    center_lng,
    default_zoom
)
VALUES
('Villa Ballester', -34.5495, -58.5580, 14),
('San Martín',      -34.5750, -58.5350, 14);

-- =========================================================
-- ZONES - VILLA BALLESTER
-- district_id = 1
-- =========================================================

INSERT INTO zone (
    district_id,
    name,
    description,
    center_lat,
    center_lng,
    default_zoom
)
VALUES
(1, 'Norte',  'Zona norte de Villa Ballester',  -34.5430, -58.5580, 15),
(1, 'Sur',    'Zona sur de Villa Ballester',    -34.5565, -58.5580, 15),
(1, 'Este',   'Zona este de Villa Ballester',   -34.5495, -58.5510, 15),
(1, 'Oeste',  'Zona oeste de Villa Ballester',  -34.5495, -58.5660, 15),
(1, 'Centro', 'Zona centro de Villa Ballester', -34.5495, -58.5580, 16);

-- =========================================================
-- ZONES - SAN MARTÍN
-- district_id = 2
-- =========================================================

INSERT INTO zone (
    district_id,
    name,
    description,
    center_lat,
    center_lng,
    default_zoom
)
VALUES
(2, 'Norte',  'Zona norte de San Martín',  -34.5680, -58.5350, 15),
(2, 'Sur',    'Zona sur de San Martín',    -34.5820, -58.5350, 15),
(2, 'Este',   'Zona este de San Martín',   -34.5750, -58.5280, 15),
(2, 'Oeste',  'Zona oeste de San Martín',  -34.5750, -58.5430, 15),
(2, 'Centro', 'Zona centro de San Martín', -34.5750, -58.5350, 16);


-- =========================================================
-- SAMPLE DATA - DEVICES
-- =========================================================

INSERT INTO devices (
    light_id,
    eqid,
    linkid,
    hubid,
    date_time,
    devtype,
    serial_number,
    _name,
    model,
    address,
    address_number,
    intersection,
    zipcode,
    lat,
    lng,
    description,
    zone_id,
    map_loc,
    map_pag,
    is_enabled,
    is_linked,
    devconfig
)
VALUES
('SL-001','01:02:03:04:05:06','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000001','Luminaria 001','LT-LED150','Av. San Martin','100','Av. San Martin y Calle 1','1650',-34.6037,-58.3816,'Luminaria LED',1,'Villa Ballester',1,TRUE,FALSE,0),
('SL-002','00:00:00:00:00:02','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000002','Luminaria 002','LT-LED150','Av. San Martin','200','Av. San Martin y Calle 2','1650',-34.6040,-58.3820,'Luminaria LED',1,'Villa Ballester',1,TRUE,FALSE,0),
('SL-003','00:00:00:00:00:03','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000003','Luminaria 003','LT-LED150','Av. San Martin','300','Av. San Martin y Calle 3','1650',-34.6050,-58.3830,'Luminaria LED',2,'Villa Ballester',1,TRUE,FALSE,0),
('SL-004','00:00:00:00:00:04','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000004','Luminaria 004','LT-LED150','Av. San Martin','400','Av. San Martin y Calle 4','1650',-34.6060,-58.3840,'Luminaria LED',2,'Villa Ballester',1,TRUE,FALSE,0),
('SL-005','00:00:00:00:00:05','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000005','Luminaria 005','LT-LED150','Av. San Martin','500','Av. San Martin y Calle 5','1650',-34.6070,-58.3850,'Luminaria LED',3,'Villa Ballester',1,TRUE,FALSE,0),
('SL-006','00:00:00:00:00:06','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000006','Luminaria 006','LT-LED150','Av. San Martin','600','Av. San Martin y Calle 6','1650',-34.6080,-58.3860,'Luminaria LED',3,'Villa Ballester',1,TRUE,FALSE,0),
('SL-007','00:00:00:00:00:07','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000007','Luminaria 007','LT-LED150','Av. San Martin','700','Av. San Martin y Calle 7','1650',-34.6090,-58.3870,'Luminaria LED',4,'Villa Ballester',1,TRUE,FALSE,0),
('SL-008','00:00:00:00:00:08','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000008','Luminaria 008','LT-LED150','Av. San Martin','800','Av. San Martin y Calle 8','1650',-34.6100,-58.3880,'Luminaria LED',4,'Villa Ballester',1,TRUE,FALSE,0),
('SL-009','00:00:00:00:00:09','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000009','Luminaria 009','LT-LED150','Av. San Martin','900','Av. San Martin y Calle 9','1650',-34.6110,-58.3890,'Luminaria LED',5,'Villa Ballester',1,TRUE,FALSE,0),
('SL-010','00:00:00:00:00:0A','00:00:00:00:00:00','00:00:00:00:01:00',NOW(),0,'SN-000010','Luminaria 010','LT-LED150','Av. San Martin','1000','Av. San Martin y Calle 10','1650',-34.6120,-58.3900,'Luminaria LED',5,'Villa Ballester',1,TRUE,FALSE,0),
('SL-011','00:00:00:00:00:0B','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000011','Luminaria 011','LT-LED150','Calle Alvear','1100','Calle Alvear y Calle 11','1650',-36.5481,-58.5582,'Luminaria LED',1,'Villa Ballester',1,TRUE,FALSE,0),
('SL-012','00:00:00:00:00:0C','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000012','Luminaria 012','LT-LED150','Calle Alvear','1200','Calle Alvear y Calle 12','1650',-36.5490,-58.5590,'Luminaria LED',1,'Villa Ballester',1,TRUE,FALSE,0),
('SL-013','00:00:00:00:00:0D','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000013','Luminaria 013','LT-LED150','Boulevard Ballester','1300','Boulevard Ballester y Calle 13','1650',-36.5502,-58.5601,'Luminaria LED',2,'Villa Ballester',1,TRUE,FALSE,0),
('SL-014','00:00:00:00:00:0E','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000014','Luminaria 014','LT-LED150','Boulevard Ballester','1400','Boulevard Ballester y Calle 14','1650',-36.5510,-58.5615,'Luminaria LED',2,'Villa Ballester',1,TRUE,FALSE,0),
('SL-015','00:00:00:00:00:0F','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000015','Luminaria 015','LT-LED150','Calle Lacroze','1500','Calle Lacroze y Calle 15','1650',-36.5524,-58.5620,'Luminaria LED',3,'Villa Ballester',1,TRUE,FALSE,0),
('SL-016','00:00:00:00:00:10','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000016','Luminaria 016','LT-LED150','Calle Lacroze','1600','Calle Lacroze y Calle 16','1650',-36.5535,-58.5633,'Luminaria LED',3,'Villa Ballester',1,TRUE,FALSE,0),
('SL-017','00:00:00:00:00:11','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000017','Luminaria 017','LT-LED150','Avenida Márquez','1700','Avenida Márquez y Calle 17','1650',-36.5541,-58.5640,'Luminaria LED',4,'Villa Ballester',1,TRUE,FALSE,0),
('SL-018','00:00:00:00:00:12','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000018','Luminaria 018','LT-LED150','Avenida Márquez','1800','Avenida Márquez y Calle 18','1650',-36.5550,-58.5651,'Luminaria LED',4,'Villa Ballester',1,TRUE,FALSE,0),
('SL-019','00:00:00:00:00:13','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000019','Luminaria 019','LT-LED150','Calle Independencia','1900','Calle Independencia y Calle 19','1650',-36.5563,-58.5664,'Luminaria LED',5,'Villa Ballester',1,TRUE,FALSE,0),
('SL-020','00:00:00:00:00:14','00:00:00:00:00:00','00:00:00:00:02:00',NOW(),0,'SN-000020','Luminaria 020','LT-LED150','Calle Independencia','2000','Calle Independencia y Calle 20','1650',-36.5571,-58.5670,'Luminaria LED',5,'Villa Ballester',1,TRUE,FALSE,0);

-- =========================================================
-- SAMPLE DATA - DEVSTATE
-- =========================================================

INSERT INTO devstate (
    light_id, zone_id, lat, lng, status, dimming_level,
    power_watts, voltage, temperature_c, burn_hours,
    last_seen, street_name, lamp_type, rated_watts
)
VALUES
('SL-001',1,-34.6037,-58.3816,'on',80,120.5,220,45.2,12000,NOW(),'Main Street','LED',150),
('SL-002',1,-34.6040,-58.3820,'off',0,0,219,39.0,9500,NOW(),'Main Street','LED',150),
('SL-003',2,-34.6050,-58.3830,'fault',50,98.2,210,70.5,18000,NOW(),'Oak Avenue','HPS',250),
('SL-004',2,-34.6060,-58.3840,'maintenance',20,60.0,215,55.1,14500,NOW(),'Oak Avenue','MH',400),
('SL-005',3,-34.6070,-58.3850,'on',100,149.0,221,42.3,5000,NOW(),'Pine Street','LED',150),
('SL-006',3,-34.6080,-58.3860,'structure',75,112.4,220,44.0,6200,NOW(),'Pine Street','LED',150),
('SL-007',4,-34.6090,-58.3870,'fault',30,70.1,198,81.5,21000,NOW(),'Maple Road','HPS',250),
('SL-008',4,-34.6100,-58.3880,'off',0,0,220,33.0,7600,NOW(),'Maple Road','LED',150),
('SL-009',5,-34.6110,-58.3890,'on',65,105.0,222,47.0,8900,NOW(),'Liberty Ave','LED',150),
('SL-010',5,-34.6120,-58.3900,'maintenance',10,30.0,214,50.0,13200,NOW(),'Liberty Ave','MH',400),

('SL-011',1,-36.5481,-58.5582,'on',90,135.0,221,43.1,7200,NOW(),'Calle Alvear','LED',150),
('SL-012',1,-36.5490,-58.5590,'off',0,0,220,35.5,9800,NOW(),'Calle Alvear','LED',150),
('SL-013',2,-36.5502,-58.5601,'fault',40,82.5,205,76.8,19500,NOW(),'Boulevard Ballester','HPS',250),
('SL-014',2,-36.5510,-58.5615,'structure',15,42.0,214,58.0,16300,NOW(),'Boulevard Ballester','MH',400),
('SL-015',3,-36.5524,-58.5620,'on',100,151.0,222,41.7,4300,NOW(),'Calle Lacroze','LED',150),
('SL-016',3,-36.5535,-58.5633,'structure',70,108.2,220,46.2,6100,NOW(),'Calle Lacroze','LED',150),
('SL-017',4,-36.5541,-58.5640,'fault',25,65.4,199,83.3,22800,NOW(),'Avenida Márquez','HPS',250),
('SL-018',4,-36.5550,-58.5651,'off',0,0,221,32.1,8700,NOW(),'Avenida Márquez','LED',150),
('SL-019',5,-36.5563,-58.5664,'on',60,97.8,223,48.6,9100,NOW(),'Calle Independencia','LED',150),
('SL-020',5,-36.5571,-58.5670,'maintenance',20,55.0,216,53.2,14800,NOW(),'Calle Independencia','MH',400);
-- =========================================================
-- SAMPLE DATA - ALARM
-- =========================================================

INSERT INTO alarm (
    light_id, street_name, zone_id, alarm_type, severity, message,
    acknowledged, acknowledged_by, acknowledged_at, resolved, resolved_at
)
VALUES
('SL-003','Oak Avenue',2,'lamp_fault','high','Lamp failure detected',TRUE,'admin',NOW(),FALSE,NULL),
('SL-004','Oak Avenue',2,'driver_fault','critical','LED driver malfunction',TRUE,'operator1',NOW(),FALSE,NULL),
('SL-007','Maple Road',4,'over_temperature','critical','Temperature exceeded threshold',FALSE,NULL,NULL,FALSE,NULL),
('SL-001','Main Street',1,'voltage_anomaly','medium','Voltage fluctuation detected',TRUE,'admin',NOW(),TRUE,NOW()),
('SL-005','Pine Street',3,'communication_lost','high','No communication with node',FALSE,NULL,NULL,FALSE,NULL),
('SL-006','Pine Street',3,'power_outage','critical','Power outage detected',TRUE,'tech1',NOW(),FALSE,NULL),
('SL-002','Main Street',1,'lamp_fault','low','Low brightness warning',TRUE,'admin',NOW(),TRUE,NOW()),
('SL-008','Maple Road',4,'driver_fault','medium','Driver response timeout',FALSE,NULL,NULL,FALSE,NULL),
('SL-009','Liberty Ave',5,'over_temperature','high','Internal temperature high',TRUE,'operator2',NOW(),FALSE,NULL),
('SL-010','Liberty Ave',5,'communication_lost','medium','Packet timeout',FALSE,NULL,NULL,FALSE,NULL),

('SL-011','Calle Alvear',1,'voltage_anomaly','medium','Se detectó fluctuación de voltaje',TRUE,'admin',NOW(),TRUE,NOW()),
('SL-012','Calle Alvear',1,'lamp_fault','low','Advertencia de baja luminosidad',TRUE,'operador1',NOW(),FALSE,NULL),
('SL-013','Boulevard Ballester',2,'lamp_fault','high','Falla detectada en la lámpara',TRUE,'admin',NOW(),FALSE,NULL),
('SL-014','Boulevard Ballester',2,'driver_fault','critical','Mal funcionamiento del driver LED',TRUE,'tecnico1',NOW(),FALSE,NULL),
('SL-015','Calle Lacroze',3,'communication_lost','medium','Pérdida de comunicación con el nodo',FALSE,NULL,NULL,FALSE,NULL),
('SL-016','Calle Lacroze',3,'power_outage','critical','Se detectó un corte de energía',TRUE,'operador2',NOW(),FALSE,NULL),
('SL-017','Avenida Márquez',4,'over_temperature','critical','La temperatura superó el umbral permitido',FALSE,NULL,NULL,FALSE,NULL),
('SL-018','Avenida Márquez',4,'driver_fault','medium','Tiempo de respuesta del driver excedido',FALSE,NULL,NULL,FALSE,NULL),
('SL-019','Calle Independencia',5,'over_temperature','high','Temperatura interna elevada',TRUE,'admin',NOW(),FALSE,NULL),
('SL-020','Calle Independencia',5,'communication_lost','medium','Timeout en la recepción de paquetes',FALSE,NULL,NULL,FALSE,NULL);

-- =========================================================
-- SAMPLE DATA - DEVICEEVENT
-- =========================================================

INSERT INTO deviceevent (
    light_id, event_code, event_type, severity, message,
    zone_id, theoretical_kw, real_kw, voltage_v, current_ma, resolved
)
VALUES
('SL-001','EV-001','power_failure','critical','Unexpected power loss',1,0.150,0.000,0,0,FALSE),
('SL-002','EV-002','communication_lost','medium','Device offline',1,0.150,0.010,220,50,FALSE),
('SL-003','EV-003','voltage_spike','high','High voltage detected',2,0.250,0.310,260,900,FALSE),
('SL-004','EV-004','driver_overheat','critical','Driver overheating',2,0.400,0.420,215,1200,FALSE),
('SL-005','EV-005','overcurrent','high','Current exceeded limit',3,0.150,0.190,220,1500,TRUE),
('SL-006','EV-006','breaker_trip','medium','Breaker disconnected',3,0.150,0.000,0,0,TRUE),
('SL-007','EV-007','lamp_end_of_life','low','Lamp lifetime reached',4,0.250,0.240,219,850,FALSE),
('SL-008','EV-008','circuit_disconnected','high','Open circuit detected',4,0.150,0.000,0,0,FALSE),
('SL-009','EV-009','voltage_spike','medium','Transient voltage spike',5,0.150,0.170,245,780,TRUE),
('SL-010','EV-010','communication_lost','medium','No response from controller',5,0.400,0.020,210,40,FALSE),

('SL-011','EV-011','power_failure','critical','Pérdida inesperada de energía',1,0.150,0.000,0,0,FALSE),
('SL-012','EV-012','communication_lost','medium','Dispositivo fuera de línea',1,0.150,0.012,220,55,FALSE),
('SL-013','EV-013','voltage_spike','high','Se detectó sobretensión',2,0.250,0.320,262,910,FALSE),
('SL-014','EV-014','driver_overheat','critical','Sobrecalentamiento del driver',2,0.400,0.430,216,1210,FALSE),
('SL-015','EV-015','overcurrent','high','La corriente excedió el límite permitido',3,0.150,0.195,221,1510,TRUE),
('SL-016','EV-016','breaker_trip','medium','Interruptor automático desconectado',3,0.150,0.000,0,0,TRUE),
('SL-017','EV-017','lamp_end_of_life','low','La lámpara alcanzó el fin de su vida útil',4,0.250,0.245,219,860,FALSE),
('SL-018','EV-018','circuit_disconnected','high','Se detectó circuito abierto',4,0.150,0.000,0,0,FALSE),
('SL-019','EV-019','voltage_spike','medium','Pico transitorio de tensión detectado',5,0.150,0.172,246,790,TRUE),
('SL-020','EV-020','communication_lost','medium','Sin respuesta del controlador',5,0.400,0.021,211,42,FALSE);

-- =========================================================
-- SAMPLE DATA - ENERGYLOG
-- (usa zone_id referenciando la tabla zone)
-- =========================================================

INSERT INTO energylog (
    zone_id, recorded_at, total_kwh, active_lights,
    avg_dimming, faults_count, co2_saved_kg
)
VALUES
(1, NOW() - INTERVAL '10 hours', 1250.500, 45, 78.5, 2, 320.2),
(2, NOW() - INTERVAL '9 hours',   980.200, 38, 70.0, 4, 250.0),
(3, NOW() - INTERVAL '8 hours',  1430.800, 52, 88.2, 1, 410.5),
(4, NOW() - INTERVAL '7 hours',   870.100, 31, 60.4, 5, 210.8),
(5, NOW() - INTERVAL '6 hours',  1115.600, 40, 74.0, 3, 298.1),
(1, NOW() - INTERVAL '5 hours',  1275.300, 46, 80.0, 1, 325.4),
(2, NOW() - INTERVAL '4 hours',  1002.900, 39, 72.5, 2, 255.0),
(3, NOW() - INTERVAL '3 hours',  1455.400, 53, 90.1, 0, 420.0),
(4, NOW() - INTERVAL '2 hours',   910.700, 33, 64.3, 4, 220.7),
(5, NOW() - INTERVAL '1 hour',   1148.200, 41, 76.8, 2, 305.6);
