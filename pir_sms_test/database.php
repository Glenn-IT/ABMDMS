<?php
/**
 * ============================================================
 * PIR + SMS TEST RIG
 * File: database.php
 * ============================================================
 *
 * Opens the connection to MySQL. Every page that needs the
 * database does this:
 *
 *     require_once 'database.php';
 *     $db = getDB();
 *
 * We use PDO with prepared statements, which is what protects
 * the database from SQL injection.
 * ============================================================
 */

require_once __DIR__ . '/config.php';


/**
 * Returns the database connection.
 * Created on the first call, reused after that.
 *
 * @return PDO
 * @throws PDOException if the connection fails
 */
function getDB(): PDO
{
    static $pdo = null;

    if ($pdo !== null) {
        return $pdo;
    }

    $dsn = 'mysql:host=' . DB_HOST
         . ';port='      . DB_PORT
         . ';dbname='    . DB_NAME
         . ';charset='   . DB_CHARSET;

    $options = [
        PDO::ATTR_ERRMODE            => PDO::ERRMODE_EXCEPTION,
        PDO::ATTR_DEFAULT_FETCH_MODE => PDO::FETCH_ASSOC,
        PDO::ATTR_EMULATE_PREPARES   => false,
    ];

    $pdo = new PDO($dsn, DB_USER, DB_PASS, $options);

    return $pdo;
}


/**
 * Checks the connection without crashing, so the dashboard can
 * show a friendly warning instead of a white page of red text.
 *
 * @return array{ok: bool, message: string}
 */
function testDBConnection(): array
{
    try {
        getDB()->query('SELECT 1');
        return ['ok' => true, 'message' => 'Database connected.'];

    } catch (PDOException $e) {

        error_log('PIR_SMS_TEST database error: ' . $e->getMessage());

        $hint = 'Cannot connect to MySQL. Open the XAMPP Control Panel, make sure '
              . 'MySQL is running, then import pir_sms_test/database/pir_sms_test.sql '
              . 'in phpMyAdmin.';

        return ['ok' => false, 'message' => $hint];
    }
}
