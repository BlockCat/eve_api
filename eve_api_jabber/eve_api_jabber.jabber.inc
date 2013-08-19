<?php
/**
 * @file
 * Functions related to connecting to Jabber.
 */

/**
 * Test the connection to the Jabber Server.
 */
function eve_api_jabber_test_connection() {
  try {
    $result = db_query('SELECT username FROM {ofUser} LIMIT 1');
    unset($result);
  }
  catch (PDOException $e) {
    throw new EveApiException(check_plain($e->getMessage()));
  }
}

/**
 * Connect to the Jabber Database.
 */
function eve_api_jabber_connect($check = FALSE) {
  if (!variable_get('eve_api_jabber_enable', FALSE) && !$check) {
    throw new EveApiException(t('Jabber Connection disabled.'));
  }

  // Set the Jabber Database for access on the fly.
  $jabber_database = array(
    'host' => variable_get('eve_api_jabber_hostname', '127.0.0.1'),
    'database' => variable_get('eve_api_jabber_database', 'Jabber'),
    'username' => variable_get('eve_api_jabber_username', 'Username'),
    'password' => variable_get('eve_api_jabber_password', 'Password'),
    'driver' => 'mysql',
  );

  Database::addConnectionInfo('Jabber', 'default', $jabber_database);

  try {
    db_set_active('Jabber');
  }
  catch (PDOException $e) {
    db_set_active();
    throw new EveApiException(check_plain($e->getMessage()));
  }
}

/**
 * Disconnect from the Jabber Database.
 */
function eve_api_jabber_disconnect() {
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
function eve_api_jabber_format_name($name) {
  $name = strtolower($name);
  $name = str_replace(" ", "_", $name);
  $name = str_replace("'", "_", $name);

  if (strlen($name) > 50) {
    $name = substr($name, 0, 50);
  }

  return check_plain($name);
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
function eve_api_jabber_format_display_name($name) {
  $name = str_replace("'", "_", $name);

  if (strlen($name) > 50) {
    $name = substr($name, 0, 50);
  }

  return $name;
}

/**
 * Checks to see if exists and Inserts a new Group if it does not exist.
 *
 * @param string $name
 *   The group name to add.
 * @param string $description
 *   The description of the group.
 *
 * @return bool
 *   Returns a true on success.
 */
function eve_api_jabber_group_add($name, $description = '') {
  if (!variable_get('eve_api_jabber_enable', FALSE)) {
    throw new EveApiException(t('Jabber Connection disabled.'));
  }

  try {
    eve_api_jabber_connect();

    $clean_name = (string) eve_api_jabber_format_name($name);

    $result = db_query('SELECT groupName FROM {ofGroup} WHERE groupName = :groupName', array(
      ':groupName' => $clean_name,
    ));

    if ($result->rowCount()) {
      eve_api_jabber_disconnect();

      return TRUE;
    }

    db_merge('ofGroup')->key(array('groupName' => $clean_name))->fields(array(
      'groupName' => $clean_name,
      'description' => (string) $description,
    ))->execute();

    db_insert('ofGroupProp')->fields(array(
      'groupName' => $clean_name,
      'name' => 'sharedRoster.showInRoster',
      'propValue' => 'everybody',
    ))->execute();

    db_insert('ofGroupProp')->fields(array(
      'groupName' => $clean_name,
      'name' => 'sharedRoster.displayName',
      'propValue' => (string) $name,
    ))->execute();

    db_insert('ofGroupProp')->fields(array(
      'groupName' => $clean_name,
      'name' => 'sharedRoster.groupList',
      'propValue' => '',
    ))->execute();

    eve_api_jabber_disconnect();
  }
  catch (PDOException $e) {
    eve_api_jabber_disconnect();
    throw new EveApiException(check_plain($e->getMessage()));
  }

  return TRUE;
}

/**
 * Prepares a URL query to do various tasks through the Jabber User Service.
 *
 * @param string $type
 *   What we are doing.
 * @param string $character_name
 *   The login name of the user we are modifying..
 * @param array $query
 *   (optional) Additional options as needed for each $type.
 *
 * @return mixed
 *   Returns a true on success, false on failure or message.
 */
function eve_api_jabber_url_query($type, $character_name = '', array $query = array()) {
  if (!variable_get('eve_api_jabber_enable', FALSE) && $type != 'check') {
    throw new EveApiException(t('Jabber Connection disabled.'));
  }

  try {
    switch ($type) {
      case 'enable':
      case 'disable':
      case 'delete':
        if (empty($character_name)) {
          return FALSE;
        }

        $query = array();
        $default_query = array(
          'type' => $type,
          'secret' => variable_get('eve_api_jabber_secret', 'Password'),
          'username' => eve_api_jabber_format_name($character_name),
        );
        break;

      case 'add':
      case 'update':
      case 'add_roster':
      case 'update_roster':
      case 'delete_roster':
        if (empty($query) || empty($character_name)) {
          return FALSE;
        }

        $default_query = array(
          'type' => $type,
          'secret' => variable_get('eve_api_jabber_secret', 'Password'),
          'username' => eve_api_jabber_format_name($character_name),
        );
        break;

      case 'check':
        $query = array();
        $default_query = array(
          'secret' => variable_get('eve_api_jabber_secret', 'Password'),
        );
        break;

      default:
        return FALSE;
    }

    $query = array_merge($query, $default_query);
    $query = drupal_http_build_query($query);

    $url = variable_get('eve_api_jabber_url', 'http://localhost:9090') . '/plugins/userService/userservice?' . $query;
    $result = eve_api_jabber_url_send_query($url);

    if ($result == 'ok') {
      return TRUE;
    }
    else {
      return $result;
    }
  }
  catch (PDOException $e) {
    db_set_active();
    throw new EveApiException(check_plain($e->getMessage()));
  }
}

/**
 * Sends the URL query to do various tasks through the Jabber User Service.
 *
 * @param string $url
 *   URL to send to the Jabber User Service.
 *
 * @return mixed
 *   Returns the XML object on success, NULL on error.
 */
function eve_api_jabber_url_send_query($url) {
  $has_curl = function_exists('curl_init');
  $open_basedir = ini_get('open_basedir');

  if (!$has_curl || $open_basedir) {
    $options = array(
      'method' => 'GET',
    );

    $html = drupal_http_request($url, $options);
  }
  else {
    $html = eve_api_curl_http_request($url, array(), 'GET');
  }

  if (!empty($html->error)) {
    drupal_set_message(t('EVE API returned an error with the HTTP request: @error', array('@error' => check_plain($html->error))), 'error');
  }

  if ($html->code == 200) {
    try {
      $xml = @new SimpleXMLElement($html->data);
    }
    catch (Exception $e) {
      drupal_set_message(t('EVE API returned an error with the data retrieved: @error', array('@error' => check_plain($e->getMessage())), 'error'));
      $xml = NULL;
    }
  }
  else {
    $xml = NULL;
  }

  return $xml;
}
