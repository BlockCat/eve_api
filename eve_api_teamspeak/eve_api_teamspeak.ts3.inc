<?php
/**
 * @file
 * Functions related to connecting to Teamspeak3.
 */

/**
 * Sanitize input variable, used for Teamspeak messages.
 *
 * @param string $input
 *   A dirty string.
 *
 * @return string
 *   A clean string.
 */
function eve_api_teamspeak_clean_input($input) {
  return trim(preg_replace('/ +/', ' ', preg_replace('|[^[:alnum:][:punct:]\s]|', ' ', urldecode(html_entity_decode(strip_tags((string) $input))))));
}

/**
 * Check the TeamSpeak 3 PHP Framework version.
 *
 * @return string
 *   The version of the TeamSpeak 3 PHP Framework.
 */
function eve_api_teamspeak_get_version() {
  $libraries = libraries_get_libraries();

  if (isset($libraries['TeamSpeak3'])) {
    $path = libraries_get_path('TeamSpeak3');

    if (file_exists($path . '/TeamSpeak3.php')) {
      require_once $path . '/TeamSpeak3.php';

      $teamspeak_version = TeamSpeak3::LIB_VERSION;

      return $teamspeak_version;
    }
    else {
      return '0.0.0';
    }
  }
  else {
    return '0.0.0';
  }
}

/**
 * Generates the Teamspeak name as specified in Teamspeak Settings.
 *
 * @param string $character_name
 *   An eve Character Name.
 *
 * @return string
 *   The users Teamspeak display name.
 */
function eve_api_teamspeak_format_name($character_name) {
  // Valid Options:
  // @name = Character Name.
  // @corp = Corp Ticker.
  // @alliance = Alliance Ticker.
  $teamspeak_name = variable_get('eve_api_teamspeak_nickname', '[@corp] @name');

  $character_data = eve_api_teamspeak_get_character_info($character_name);

  $teamspeak_name = str_replace('@alliance', $character_data['allianceTicker'], $teamspeak_name);
  $teamspeak_name = str_replace('@corp', $character_data['corporationTicker'], $teamspeak_name);
  $teamspeak_name = str_replace('@name', $character_name, $teamspeak_name);

  if (strlen($teamspeak_name) > 30) {
    $teamspeak_name = substr($teamspeak_name, 0, 30);
  }

  return $teamspeak_name;
}

/**
 * Test the connection to the Teamspeak 3 Server.
 *
 * @return object
 *   The Teamspeak connection object.
 */
function eve_api_teamspeak_connection($check = FALSE) {
  if ((!variable_get('eve_api_teamspeak_enable', FALSE) || !variable_get('eve_api_teamspeak_framework_installed', FALSE)) && !$check) {
    throw new EveApiException(t('Teamspeak 3 Connection disabled.'));
  }

  // Load the TeamSpeak 3 PHP Framework Library.
  if (($library = libraries_load('TeamSpeak3')) && !empty($library['loaded'])) {
    variable_set('eve_api_teamspeak_framework_installed', TRUE);
  }
  else {
    variable_set('eve_api_teamspeak_framework_installed', FALSE);
    variable_set('eve_api_teamspeak_enable', FALSE);
  }

  try {
    $query = 'serverquery://' . variable_get('eve_api_teamspeak_query_username', 'Username') . ':' . variable_get('eve_api_teamspeak_query_password', 'Password') . '@' . variable_get('eve_api_teamspeak_hostname', '127.0.0.1') . ':' . variable_get('eve_api_teamspeak_query_port', 10011) . '/?server_port=' . variable_get('eve_api_teamspeak_server_port', 9987) . '&blocking=0';
    $ts3 = TeamSpeak3::factory($query);
    $ts3->whoamiSet('client_nickname', 'OEABOT');
    $ts3->setPredefinedQueryName('OEABOT');
  }
  catch (TeamSpeak3_Exception $e) {
    throw new EveApiException(t('Teamspeak 3 Connection failed. Error: @error', array('@error' => check_plain($e->getMessage()))));
  }

  return $ts3;
}

/**
 * Create a Teamspeak 3 Server group on the Teamspeak 3 server.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param string $role
 *   The name of the role.
 * 
 * @return string
 *   The new Teamspeak Group ID.
 */
function eve_api_teamspeak_group_create(&$ts3, $role) {
  try {
    if ($copy_id = eve_api_teamspeak_group_get_id($ts3, 'Default Corporation')) {
      $teamspeakgid = $ts3->serverGroupCopy($copy_id, $role);
      $ts3->serverGroupPermAssign($teamspeakgid, 'b_group_is_permanent', 1);
    }
    else {
      $guest_id = eve_api_teamspeak_group_get_id($ts3, 'Guest');
      $teamspeakgid = $ts3->serverGroupCopy($guest_id, $role);
      $ts3->serverGroupPermAssign($teamspeakgid, 'b_group_is_permanent', 1);
    }
  }
  catch (TeamSpeak3_Exception $e) {
    throw new EveApiException(t('Unable to create new group.'));
  }

  return $teamspeakgid;
}

/**
 * Get the Teamspeak 3 unique group ID.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param string $role
 *   The name of the role.
 * 
 * @return string
 *   The Teamspeak Group ID.
 */
function eve_api_teamspeak_group_get_id(&$ts3, $role) {
  try {
    $teamspeakgid = $ts3->serverGroupGetByName($role);
  }
  catch (TeamSpeak3_Exception $e) {
    $teamspeakgid = FALSE;
  }

  return $teamspeakgid;
}

/**
 * Get a list of current Teampspeak Server groups.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * 
 * @return array
 *   Returns a list of server groups available.
 */
function eve_api_teamspeak_get_groups(&$ts3) {
  try {
    $teamspeak_data = $ts3->serverGroupList();
  }
  catch (TeamSpeak3_Exception $e) {
    return FALSE;
  }

  return $teamspeak_data;
}

/**
 * Retrieve list of users currently connected to Teamspeak.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * 
 * @return array
 *   Returns an array filled with TeamSpeak3_Node_Client objects.
 */
function eve_api_teamspeak_user_list(&$ts3) {
  try {
    $teamspeak_user_list = $ts3->clientList();
  }
  catch (TeamSpeak3_Exception $e) {
    throw new EveApiException(t('Unable to get list of active users. Error: @error', array('@error' => check_plain($e->getMessage()))));
  }

  return $teamspeak_user_list;
}

/**
 * Retrieve info on user currently connected to Teamspeak.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param int $teamspeak_dbid
 *   The Teamspeak unique database id of a user.
 * 
 * @return array
 *   Returns a list of properties from the database for the client specified.
 */
function eve_api_teamspeak_user_info(&$ts3, $teamspeak_dbid) {
  try {
    $teamspeak_user_info = $ts3->clientInfoDb($teamspeak_dbid);
  }
  catch (TeamSpeak3_Exception $e) {
    throw new EveApiException(t('Unable to get user info. Error: @error', array('@error' => check_plain($e->getMessage()))));
  }

  return $teamspeak_user_info;
}

/**
 * Get the Teamspeak 3 users Unique ID.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param string $teamspeak_name
 *   The expected Teamspeak 3 user name.
 * 
 * @return string
 *   The users unique teamspeak id.
 */
function eve_api_teamspeak_user_by_name(&$ts3, $teamspeak_name) {
  try {
    $teamspeak_data = $ts3->clientGetByName($teamspeak_name);
  }
  catch (TeamSpeak3_Exception $e) {
    throw new EveApiException(t('Unable to find user.'));
  }

  return $teamspeak_data;
}

/**
 * Delete's the user from the Teamspeak Database.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param int $teamspeak_dbid
 *   The database ID for the teamspeak user.
 * 
 * @return bool
 *   Success or failure.
 */
function eve_api_teamspeak_user_delete(&$ts3, $teamspeak_dbid) {
  try {
    $ts3->clientDeleteDb($teamspeak_dbid);
  }
  catch (TeamSpeak3_Exception $e) {
    return FALSE;
  }

  return TRUE;
}

/**
 * Get a list of groups the user is in.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param int $teamspeak_dbid
 *   The database ID for the teamspeak user.
 * 
 * @return array
 *   Returns an array containing the names and IDs of all server groups
 *   the client specified with $teamspeak_dbid is is currently residing in.
 */
function eve_api_teamspeak_user_get_groups(&$ts3, $teamspeak_dbid) {
  try {
    $teamspeak_data = $ts3->clientGetServerGroupsByDbid($teamspeak_dbid);
  }
  catch (TeamSpeak3_Exception $e) {
    return FALSE;
  }

  return $teamspeak_data;
}

/**
 * Delete's the user from the Teamspeak group.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param int $teamspeakgid
 *   The group ID for the teamspeak group.
 * @param int $teamspeak_dbid
 *   The database ID for the teamspeak user.
 * 
 * @return bool
 *   Success or failure.
 */
function eve_api_teamspeak_user_remove_group(&$ts3, $teamspeakgid, $teamspeak_dbid) {
  try {
    $ts3->serverGroupClientDel($teamspeakgid, $teamspeak_dbid);
  }
  catch (TeamSpeak3_Exception $e) {
    return FALSE;
  }

  return TRUE;
}

/**
 * Add's the user to the Teamspeak group.
 *
 * @param object $ts3
 *   The Teamspeak connection object.
 * @param int $teamspeakgid
 *   The group ID for the teamspeak group.
 * @param int $teamspeak_dbid
 *   The database ID for the teamspeak user.
 * 
 * @return bool
 *   Success or failure.
 */
function eve_api_teamspeak_user_add_group(&$ts3, $teamspeakgid, $teamspeak_dbid) {
  try {
    $ts3->serverGroupClientAdd($teamspeakgid, $teamspeak_dbid);
  }
  catch (TeamSpeak3_Exception $e) {
    return FALSE;
  }

  return TRUE;
}
