<?php
/**
 * @file
 * Functions related to connecting to Teamspeak3.
 */

/**
 * Generates the Teamspeak name as specified in Teamspeak Settings.
 *
 * @param array $account
 *   A simple user object.
 *
 * @return string
 *   The users Teamspeak display name.
 */
function omni_eve_api_teamspeak_format_name($account) {
  // Valid Options:
  // @name = Character Name
  // @corp = Corp Ticker
  // @alliance = Alliance Ticker
  $teamspeak_name = variable_get('omni_eve_api_ts3_nickname', '[@corp] @name');

  $charactername = $account->charactername;

  $character_data = omni_eve_api_get_character_info($charactername);

  $teamspeak_name = str_replace('@alliance', $character_data['allianceTicker'], $teamspeak_name);
  $teamspeak_name = str_replace('@corp', $character_data['corporationTicker'], $teamspeak_name);
  $teamspeak_name = str_replace('@name', $charactername, $teamspeak_name);

  if (strlen($teamspeak_name) > 30) {
    $teamspeak_name = substr($teamspeak_name, 0, 30);
  }

  return $teamspeak_name;
}

function omni_eve_api_teamspeak_connection() {
  if (variable_get('omni_eve_api_ts3_enable', FALSE) === FALSE) {
    throw new Exception(t('Teamspeak 3 Connection disabled.'));
  }

  try {
    $query = 'serverquery://' . variable_get('omni_eve_api_ts3_query_username', 'Username') . ':' . variable_get('omni_eve_api_ts3_query_password', 'Password') . '@' . variable_get('omni_eve_api_ts3_host', '127.0.0.1') . ':' . variable_get('omni_eve_api_ts3_query_port', 10011) . '/?server_port=' . variable_get('omni_eve_api_ts3_server_port', 9987) . '&blocking=0';
    $ts3 = TeamSpeak3::factory($query);
  }
  catch (TeamSpeak3_Exception $e){
    throw new Exception(t('Teamspeak 3 Connection failed. Error: @error', array('@error' => $e->getMessage())));
    $ts3 = NULL;
  }

  return $ts3;
}

function omni_eve_api_teamspeak_get_by_name($ts3, $teamspeak_name) {
  try {
    $teamspeak_data = $ts3->clientGetByName($teamspeak_name);
    $teamspeakuid = $teamspeak_data->client_unique_identifier;
  }
  catch (TeamSpeak3_Exception $e){
    throw new Exception(t('Unable to find user.'));
    $teamspeakuid = NULL;
  }

  return $teamspeakuid;
}
