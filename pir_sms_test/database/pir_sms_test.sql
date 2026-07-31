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
--  One row every time the PIR sensor starts or stops seeing
--  movement. Written by api/record_motion.php.
-- ------------------------------------------------------------

CREATE TABLE IF NOT EXISTS `motion_events` (
    `id`          INT UNSIGNED NOT NULL AUTO_INCREMENT,
    `event_type`  VARCHAR(20)  NOT NULL,                          -- MOTION_DETECTED / MOTION_STOPPED
    `zone`        VARCHAR(20)  NOT NULL DEFAULT 'ROOM1',          -- only one zone in this rig
    `source`      VARCHAR(50)  NOT NULL DEFAULT 'ARDUINO_PIR',    -- where the event came from
    `detected_at` DATETIME     NOT NULL,                          -- when it happened (Asia/Manila)
    `created_at`  TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `idx_detected_at` (`detected_at`),
    KEY `idx_event_type`  (`event_type`)
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
    KEY `idx_status`  (`status`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;


-- ------------------------------------------------------------
--  Check that it worked
-- ------------------------------------------------------------
--  SELECT * FROM motion_events ORDER BY id DESC LIMIT 10;
--  SELECT * FROM sms_events    ORDER BY id DESC LIMIT 10;
--
--  To wipe the rig clean before a demo:
--  TRUNCATE TABLE motion_events;
--  TRUNCATE TABLE sms_events;
-- ------------------------------------------------------------
