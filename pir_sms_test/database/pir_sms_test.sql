-- ============================================================
--  PIR + SMS TEST RIG - Database
--  File: database/pir_sms_test.sql
-- ============================================================
--
--  HOW TO IMPORT
--  -------------
--  1. Open XAMPP Control Panel, Start Apache and MySQL.
--  2. Go to  http://localhost/phpmyadmin
--  3. Click the "SQL" tab.
--  4. Paste this whole file in and press "Go".
--
--  This creates a SEPARATE database called `pir_sms_test`.
--  The main ABMDMS database (`motion_monitoring`) is NOT touched.
-- ============================================================


CREATE DATABASE IF NOT EXISTS `pir_sms_test`
    DEFAULT CHARACTER SET utf8mb4
    COLLATE utf8mb4_general_ci;

USE `pir_sms_test`;


-- ------------------------------------------------------------
--  TABLE 1: motion_events
--  One row every time ANY PIR sensor starts or stops seeing
--  movement. The `zone` column says which room it was.
--  Written by api/record_motion.php.
--
--  NOTE: `zone` is a plain VARCHAR on purpose, not an ENUM. Rooms
--  are added and removed in config.php, and an ENUM would mean a
--  schema migration every time. Room D was retired this way with
--  no change to this file at all.
-- ------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `motion_events` (
    `id`          INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `event_type`  VARCHAR(20)  NOT NULL,                          -- MOTION_DETECTED / MOTION_STOPPED
    `zone`        VARCHAR(20)  NOT NULL DEFAULT 'ROOMC',          -- ROOMA / ROOMB / ROOMC (see config.php)
    `source`      VARCHAR(50)  NOT NULL DEFAULT 'ARDUINO_PIR',    -- where the event came from
    `detected_at` DATETIME     NOT NULL,                          -- when it happened (Asia/Manila)
    `created_at`  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_detected_at` (`detected_at`),
    KEY `idx_event_type`  (`event_type`),
    KEY `idx_zone`        (`zone`)          -- the dashboard asks "newest row for room X" a lot
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;


-- ------------------------------------------------------------
--  TABLE 2: sms_events
--  One row every time the Arduino REPORTS the result of an SMS.
--  The Arduino sends the text; PHP only records what happened.
--  Written by api/record_sms.php.
-- ------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `sms_events` (
    `id`        INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `zone`      VARCHAR(20)  NOT NULL,
    `recipient` VARCHAR(20)  NOT NULL DEFAULT '',                 -- phone number that was texted
    `status`    VARCHAR(20)  NOT NULL,                            -- SENT / FAILED / SKIPPED
    `detail`    VARCHAR(100) NOT NULL DEFAULT '',                 -- short reason, e.g. TIMEOUT, COOLDOWN
    `sent_at`   DATETIME     NOT NULL,
    `created_at` TIMESTAMP   NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_sent_at` (`sent_at`),
    KEY `idx_status`  (`status`),
    KEY `idx_zone`    (`zone`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;


-- ------------------------------------------------------------
--  ALREADY IMPORTED THE OLD 1-SENSOR VERSION? Read this.
-- ------------------------------------------------------------
--  CREATE TABLE IF NOT EXISTS does nothing to a table that already
--  exists, so re-running this file will NOT add idx_zone or change
--  the ROOM1 default on an existing rig. And any rows already saved
--  still say zone 'ROOM1', which is no longer an allowed zone - they
--  show up in the history with no room label.
--
--  Pick ONE of the two fixes below and run it in the SQL tab.
--
--  FIX A - keep the old readings (ROOM1 was the Pin 2 sensor,
--          which is now Room C):
--
--      USE `pir_sms_test`;
--      UPDATE motion_events SET zone = 'ROOMC' WHERE zone = 'ROOM1';
--      UPDATE sms_events    SET zone = 'ROOMC' WHERE zone = 'ROOM1';
--      ALTER TABLE motion_events ALTER COLUMN `zone` SET DEFAULT 'ROOMC';
--      ALTER TABLE motion_events ADD KEY `idx_zone` (`zone`);
--      ALTER TABLE sms_events    ADD KEY `idx_zone` (`zone`);
--
--  FIX B - start clean (fine for a test rig; loses the old rows):
--
--      USE `pir_sms_test`;
--      DROP TABLE motion_events;
--      DROP TABLE sms_events;
--      -- then run this whole file again
--
-- ------------------------------------------------------------
--  Check that it worked
-- ------------------------------------------------------------
--  SELECT * FROM motion_events ORDER BY id DESC LIMIT 10;
--  SELECT * FROM sms_events    ORDER BY id DESC LIMIT 10;
--
--  Count events per room:
--  SELECT zone, COUNT(*) FROM motion_events GROUP BY zone;
--
--  To wipe the rig clean before a demo:
--  TRUNCATE TABLE motion_events;
--  TRUNCATE TABLE sms_events;
-- ------------------------------------------------------------
