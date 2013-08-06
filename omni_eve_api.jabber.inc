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

/**
 * Format the Jabber group or login name.
 *
 * @param string $name
 *   The name to clean.
 *
 * @return string
 *   The cleaned Jabber group or login name.
 */
function omni_eve_api_jabber_format_name($name) {
  $name = strtolower($name);
  $name = str_replace(" ", "_", $name);
  $name = str_replace("'","_", $name);

  if (strlen($name) > 50) {
    $name = substr($name, 0, 50);
  }

  return $name;
}

/**
 * Format the Jabber display name.
 *
 * @param string $name
 *   The name to clean.
 *
 * @return string
 *   The users Jabber display name.
 */
function omni_eve_api_jabber_format_display_name($name) {
  $name = str_replace("'","_", $name);

  if (strlen($name) > 50) {
    $name = substr($name, 0, 50);
  }

  return $name;
}

function omni_eve_api_jabber_user_update($charactername, $email, $groups, $password = '') {
  $query = array(
    'name' => omni_eve_api_jabber_format_display_name($charactername),
    'email' => $email,
    'groups' => implode(",", $groups),
  );

  if (!empty($password)) {
    $password = array(
      'password' => $password,
    );

    $query = array_merge($query, $password);
  }

  $result = omni_eve_api_jabber_query('update', $charactername, $query);
}

function omni_eve_api_jabber_user_add($charactername, $email, $groups, $password) {
  $query = array(
    'name' => omni_eve_api_jabber_format_display_name($charactername),
    'password' => $password,
    'email' => $email,
    'groups' => implode(",", $groups),
  );

  $result = omni_eve_api_jabber_query('add', $charactername, $query);
}

function omni_eve_api_jabber_user_delete($charactername) {
  $result = omni_eve_api_jabber_query('add', $charactername);
}

function omni_eve_api_jabber_user_disable($charactername) {
  $result = omni_eve_api_jabber_query('disable', $charactername);
}

function omni_eve_api_jabber_user_enable($charactername) {
  $result = omni_eve_api_jabber_query('enable', $charactername);
}

function omni_eve_api_jabber_query($type, $charactername, array $query = array()) {
  switch ($type) {
    case 'add':
    case 'update':
    case 'add_roster':
    case 'update_roster':
    case 'delete_roster':
      if (empty($query)) {
        return FALSE;
      }
      break;

    case 'enable':
    case 'disable':
    case 'delete':
      $query = array();
      break;

    default:
      return FALSE;
      break;
  }

  $default_query = array(
    'type' => $type,
    'secret' => variable_get('omni_eve_api_jabber_secret', 'Password'),
    'username' => omni_eve_api_jabber_format_name($charactername),
  );

  $query = array_merge($query, $default_query);

  $query = drupal_http_build_query($query);

  $url = variable_get('omni_eve_api_jabber_url', 'http://localhost:9090') . '/plugins/userService/userservice?' . $query;
  $result = omni_eve_api_jabber_send_query($url);

  if ($result == 'ok') {
    return TRUE;
  }
  else {
    return $result;
  }
}

function omni_eve_api_jabber_send_query($url) {
  $html = omni_eve_api_curl_http_request($url, array(), 'GET');

  if (!empty($html->error)) {
    drupal_set_message(t('EVE API returned an error with the HTTP request: @error', array('@error' => $html->error)), 'error');
  }

  if ($html->code == 200) {
    try {
      $xml = @new SimpleXMLElement($html->data);
    }
    catch (Exception $e) {
      drupal_set_message(t('EVE API returned an error with the data retrieved: @error', array('@error' => $e->getMessage()), 'error'));
      $xml = NULL;
    }
  }
  else {
    $xml = NULL;
  }

  return $xml;
}
