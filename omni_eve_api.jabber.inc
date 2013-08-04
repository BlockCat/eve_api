<?php
/**
 * @file
 * Functions related to connecting to Jabber.
 */

if (variable_get('omni_eve_api_jabber_enable', FALSE)) {
  $jabber_database = array(
    'host' => variable_get('omni_eve_api_jabber_host', '127.0.0.1'),
    'database' => variable_get('omni_eve_api_jabber_database', 'Jabber'),
    'username' => variable_get('omni_eve_api_jabber_username', 'Username'),
    'password' => variable_get('omni_eve_api_jabber_password', 'Password'),
    'driver' => 'mysql',
  );

  Database::addConnectionInfo('Jabber', 'default', $jabber_database);
}

/**
 * Test the connection to the Jabber Server.
 */
function omni_eve_api_jabber_test_connection() {
  try {
    $result = db_select('ofUser', 'u')
      ->fields('u', array(
        'username',
      ))
      ->range(0,1)
      ->execute();
  }
  catch (PDOException $e) {
    throw new Exception($e->getMessage());
  }
}

/**
 * Connect to the Jabber Database.
 */
function omni_eve_api_jabber_connect() {
  try {
    db_set_active('Jabber');
  }
  catch (PDOException $e) {
    db_set_active();
    throw new Exception($e->getMessage());
  }
}

/**
 * Disconnect from the Jabber Database.
 */
function omni_eve_api_jabber_disconnect() {
  db_set_active();
}
