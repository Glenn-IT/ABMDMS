<?php
/**
 * ============================================================
 * ABMDMS - Arduino Based Motion Detection Monitoring System
 * File: database.php
 * ============================================================
 *
 * This file opens the connection to MySQL.
 *
 * Every page that needs the database does this:
 *
 *     require_once 'database.php';
 *     $db = getDB();
 *
 * We use PDO because it supports "prepared statements", which
 * protect the database from SQL injection attacks.
 * ============================================================
 */

require_once __DIR__ . '/config.php';


/**
 * Returns the database connection.
 *
 * The connection is created the first time this function is
 * called, then reused after that (so we do not open 10 separate
 * connections on one page).
 *
 * @return PDO
 * @throws PDOException if the connection fails
 */
function getDB(): PDO
{
    // 'static' means this variable keeps its value between calls
    static $pdo = null;

    // Already connected? Just hand back the same connection.
    if ($pdo !== null) {
        return $pdo;
    }

    // The DSN is the "address" of the database
    $dsn = 'mysql:host=' . DB_HOST
         . ';port='      . DB_PORT
         . ';dbname='    . DB_NAME
         . ';charset='   . DB_CHARSET;

    $options = [
        // Throw an exception when something goes wrong (easier to debug)
        PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,

        // Return rows as simple named arrays: $row['event_type']
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,

        // Use real prepared statements, not fake ones (safer)
        PDO::ATTR_EMULATE_PREPARES   => false,
    ];

    $pdo = new PDO($dsn, DB_USER, DB_PASS, $options);

    return $pdo;
}


/**
 * A small helper that checks the connection without crashing.
 * Used by the dashboard so it can show a friendly warning
 * instead of a white page full of red error text.
 *
 * @return array{ok: bool, message: string}
 */
function testDBConnection(): array
{
    try {
        getDB()->query('SELECT 1');
        return ['ok' => true, 'message' => 'Database connected.'];

    } catch (PDOException $e) {

        // Log the real technical reason for ourselves...
        error_log('ABMDMS database error: ' . $e->getMessage());

        // ...but show the user a helpful, non-technical hint.
        $hint = 'Cannot connect to MySQL. Open XAMPP Control Panel and make sure '
              . 'MySQL is running, and that you imported database/database.sql '
              . 'in phpMyAdmin.';

        return ['ok' => false, 'message' => $hint];
    }
}
