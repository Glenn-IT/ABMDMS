-- ============================================================
-- ABMDMS - Arduino Based Motion Detection Monitoring System
-- File: database/database.sql
-- ============================================================
--
-- HOW TO USE THIS FILE
-- --------------------
-- 1. Open XAMPP Control Panel and start Apache + MySQL.
-- 2. Go to http://localhost/phpmyadmin
-- 3. Click the "Import" tab at the top.
-- 4. Click "Choose File" and select this file (database.sql).
-- 5. Scroll down and click "Import".
--
-- That is all. This file creates the database, the table,
-- and the indexes automatically. You do NOT need to create
-- the database by hand first.
-- ============================================================


-- ------------------------------------------------------------
-- STEP 1: Create the database (only if it does not exist yet)
-- ------------------------------------------------------------
CREATE DATABASE IF NOT EXISTS `motion_monitoring`
    DEFAULT CHARACTER SET utf8mb4
    COLLATE utf8mb4_general_ci;


-- ------------------------------------------------------------
-- STEP 2: Use that database for everything below
-- ------------------------------------------------------------
USE `motion_monitoring`;


-- ------------------------------------------------------------
-- STEP 3: Create the table that stores every motion event
-- ------------------------------------------------------------
CREATE TABLE IF NOT EXISTS `motion_logs` (

    -- Automatic ID number: 1, 2, 3, 4 ...
    `id` INT UNSIGNED NOT NULL AUTO_INCREMENT,

    -- What happened: 'MOTION_DETECTED' or 'MOTION_STOPPED'
    `event_type` VARCHAR(20) NOT NULL,

    -- Which zone it happened in: 'ROOMA', 'ROOMB', or 'ROOMC'
    `zone` VARCHAR(20) NOT NULL DEFAULT 'ROOMC',

    -- Where it came from: 'ARDUINO_PIR' or 'SIMULATOR'
    `source` VARCHAR(50) NOT NULL DEFAULT 'ARDUINO_PIR',

    -- The date and time the motion actually happened
    `detected_at` DATETIME NOT NULL,

    -- The date and time the row was saved (filled in automatically)
    `created_at` TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,

    -- The id column is the primary key (the unique row identifier)
    PRIMARY KEY (`id`),

    -- Indexes make searching and sorting much faster
    KEY `idx_detected_at` (`detected_at`),
    KEY `idx_event_type`  (`event_type`),
    KEY `idx_zone`        (`zone`)

) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;


-- ============================================================
-- UPGRADE: already imported this file before the 3-zone change?
-- ------------------------------------------------------------
-- CREATE TABLE IF NOT EXISTS above does NOT add columns to a table
-- that already exists. If your `motion_logs` table was created
-- before this update, run the two lines below ONCE in the
-- phpMyAdmin "SQL" tab instead of re-importing this whole file.
-- ============================================================

-- ALTER TABLE `motion_logs` ADD COLUMN `zone` VARCHAR(20) NOT NULL DEFAULT 'ROOMC' AFTER `event_type`;
-- ALTER TABLE `motion_logs` ADD KEY `idx_zone` (`zone`);


-- ============================================================
-- OPTIONAL: sample row
-- ------------------------------------------------------------
-- The lines below are turned OFF (commented out with --).
-- If you want one example row in the table so the dashboard is
-- not empty, delete the two dashes at the start of the INSERT
-- line, then import the file again.
-- ============================================================

-- INSERT INTO `motion_logs` (`event_type`, `zone`, `source`, `detected_at`) VALUES ('MOTION_DETECTED', 'ROOMA', 'ARDUINO_PIR', NOW());


-- ============================================================
-- USEFUL COMMANDS (paste these into the phpMyAdmin "SQL" tab)
-- ------------------------------------------------------------
-- See everything that was recorded:
--     SELECT * FROM motion_logs ORDER BY id DESC;
--
-- Count all motion detections in one zone:
--     SELECT COUNT(*) FROM motion_logs WHERE event_type = 'MOTION_DETECTED' AND zone = 'ROOMA';
--
-- Delete all records and start over from id 1:
--     TRUNCATE TABLE motion_logs;
-- ============================================================
