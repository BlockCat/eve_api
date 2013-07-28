<?php
/**
 * @file
 * Functions related to retrieving and manipulating data from the EVE API.
 */

/**
 * Function to retrieve the XML.
 *
 * @param string $type
 *   Indicate what URL to use to retrieve the XML.
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return mixed
 *   Returns a valid XML object or ERROR information.
 */
function omni_eve_api_get_xml($type, $query = array()) {
  $url = 'https://api.eveonline.com';

  switch ($type) {
    case 'ContactList':
      $url .= "/corp/ContactList.xml.aspx";
      break;

    case 'AllianceList':
      $url .= "/eve/AllianceList.xml.aspx";
      break;

    case 'CorporationSheet':
      $url .= "/corp/CorporationSheet.xml.aspx";
      break;

    case 'CharacterSheet':
      $url .= "/char/CharacterSheet.xml.aspx";
      break;

    case 'FacWarStats':
      $url .= "/char/FacWarStats.xml.aspx";
      break;

    case 'CharacterID':
      $url .= "/eve/CharacterID.xml.aspx";
      break;

    case 'CharacterName':
      $url .= "/eve/CharacterName.xml.aspx";
      break;

    case 'callList':
      $url .= "/api/callList.xml.aspx";
      break;

    default:
    case 'APIKeyInfo':
      $url .= "/account/APIKeyInfo.xml.aspx";
      break;
  }

  $cache_name = 'xml' . $type . str_replace('=', '', str_replace('&', '', drupal_http_build_query($query)));

  if ($cached = cache_get($cache_name, 'cache')) {
    $html = $cached->data;
  }

  $error = NULL;

  if (empty($username)) {
    $html = omni_eve_api_curl_http_request($url, $query);

    if ($html->code >= 400) {
      drupal_set_message(t('EVE API returned an error with the HTTP request: @error', array('@error' => $html->error)), 'error');
      $error = $html;
      $html = $xml = NULL;
    }
    elseif ($html->code != 200) {
      drupal_set_message(t('EVE API returned an error with the HTTP request: @error', array('@error' => $html->error)), 'error');
      $error = $html;
      $html = $xml = NULL;
    }
  }

  if (isset($html)) {
    try {
      $xml = new SimpleXMLElement($html->data);
    }
    catch (Exception $e) {
      drupal_set_message(t('EVE API returned an error with the data retrieved: @error', array('@error' => $e->getMessage()), 'error'));
      $xml = NULL;
    }

    if ($xml->error) {
      drupal_set_message(t('EVE API returned an error with the API data: @error', array('@error' => $xml->error)), 'error');
      $xml = NULL;
    }
  }

  if (isset($xml) && isset($xml->cachedUntil)) {
    cache_set($cache_name, $html, 'cache', strtotime($xml->cachedUntil));
    return $xml;
  }
  else {
    return $error;
  }
}

/**
 * Function to retrieve individual characters from the database.
 *
 * @param string $charactername
 *   Unique character name used to retrieve information about the character
 *
 * @return array
 *   A complex array of a single character information.
 */
function omni_eve_api_get_character_info($charactername) {
  $row = db_select('oea_characters', 'c')->fields('c', array(
    'charactername',
    'characterid',
    'corporationname',
    'corporationid',
    'corporationticker',
    'allianceid',
    'alliancename',
    'allianceticker',
  ))->condition('charactername', $charactername, '=')->execute()->fetchAssoc();

  $api_data = array(
    'characterName' => $row['charactername'],
    'characterID' => $row['characterid'],
    'corporationName' => $row['corporationname'],
    'corporationID' => $row['corporationid'],
    'corporationTicker' => $row['corporationticker'],
    'allianceID' => $row['allianceid'],
    'allianceName' => $row['alliancename'],
    'allianceTicker' => $row['allianceticker'],
  );

  return $api_data;
}

/**
 * Function to retrieve individual characters API info.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A complex array of multiple character information.
 */
function omni_eve_api_get_character_api($query) {
  $xml = omni_eve_api_get_xml('APIKeyInfo', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>' . print_r($xml, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  $api_data = array();
  $api_info = $xml->result->key->attributes();
  $api_data['accessMask'] = (int) $api_info->accessMask;
  $api_data['type'] = (string) $api_info->type;
  $api_data['expires'] = ((string) $api_info->expires) ? TRUE : FALSE;

  foreach ($xml->result->key->rowset->children() as $rows) {
    $row = $rows->attributes();

    $query = array('corporationID' => (int) $row->corporationID);
    $corp_info = omni_eve_api_get_corporation_info($query);

    if (isset($corp_info['error'])) {
      return array('error' => $corp_info);
    }
    else {
      $api_data['characters'][(int) $row->characterID] = array_merge(array(
        'characterName' => (string) $row->characterName,
        'characterID' => (int) $row->characterID,
        'corporationName' => (string) $row->corporationName,
        'corporationID' => (int) $row->corporationID,
      ), $corp_info);
    }
  }

  return $api_data;
}

/**
 * Function to retrieve the alliance tags.
 *
 * @param int $allianceid
 *   The Unique ID for an alliance.
 *
 * @return string
 *   The Alliance ticker for the $allianceid.
 */
function _omni_eve_api_get_alliance_ticker($allianceid) {
  $row = db_select('oea_alliance_info', 'a')->fields('a', array('allianceticker'))->condition('allianceid', $allianceid, '=')->execute()->fetchAssoc();

  return $row['allianceticker'];
}

/**
 * Function to retrieve corporation API Info.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A complex array of corporate information.
 */
function omni_eve_api_get_corporation_info($query) {
  $xml = omni_eve_api_get_xml('CorporationSheet', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>' . print_r($xml, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  $api_data = array(
    'corporationID' => (string) $xml->result->corporationID,
    'corporationName' => (string) $xml->result->corporationName,
    'corporationTicker' => (string) $xml->result->ticker,
    'allianceID' => (int) $xml->result->allianceID,
    'allianceName' => (string) $xml->result->allianceName,
    'allianceTicker' => _omni_eve_api_get_alliance_ticker((int) $xml->result->allianceID),
  );

  return $api_data;
}

/**
 * Function to retrieve character names from character ids.
 *
 * @param array $query
 *   The URL variables to pass through the URL.
 *
 * @return array
 *   A complex array of corporate information.
 */
function omni_eve_api_get_character_name($query) {
  $xml = omni_eve_api_get_xml('CharacterName', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>' . print_r($xml, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  $api_data = array();

  foreach ($xml->result->rowset->children() as $rows) {
    $row = $rows->attributes();

    $api_data[(int) $row->characterID] = (string) $row->name;
  }

  return $api_data;
}

/**
 * Function to compare the API against the Alliance Blue List.
 *
 * @param array $characters
 *   A complex array full of character information.
 *
 * @return bool
 *   Indicates a character is blue to the alliance.
 */
function omni_eve_api_verify_blue($characters) {
  if (!isset($characters['characters'])) {
    return FALSE;
  }

  foreach ($characters['characters'] as $character) {
    $result = db_select('oea_blue_standings', 'a')->fields('a', array('contactID'))->condition('contactID', array(
      $character['characterID'],
      $character['corporationID'],
      $character['allianceID'],
    ), 'IN')->condition('standing', 0, '>')->condition('deleted', 0, '=')->execute();
    if ($result->rowCount()) {
      return TRUE;
    }

    if ($character['allianceID'] == variable_get('omni_eve_api_allianceID', 0) || $character['corporationID'] == variable_get('omni_eve_api_corporationID', 0)) {
      return TRUE;
    }

    $result = db_select('oea_alliance_corporations', 'c')->fields('c', array('corporationid'))->condition('corporationid', $character['corporationID'], '=')->execute();
    if ($result->rowCount()) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
 * Function to generate an array of Blue Characters to the Alliance.
 *
 * @param array $characters
 *   A complex array full of character information.
 *
 * @return array
 *   A simple array of characters blue to the alliance.
 */
function omni_eve_api_list_valid_characters($characters) {
  $chars = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $result = db_select('oea_blue_standings', 'a')->fields('a', array('contactID'))->condition('contactID', array(
        $character['characterID'],
        $character['corporationID'],
        $character['allianceID'],
      ), 'IN')->condition('standing', 0, '>')->condition('deleted', 0, '=')->execute();
      if ($result->rowCount()) {
        $chars[$character['characterName']] = $character['characterName'];
        continue;
      }
      $result = db_select('oea_alliance_corporations', 'c')->fields('c', array('corporationid'))->condition('corporationid', array($character['corporationID']), 'IN')->execute();
      if ($result->rowCount()) {
        $chars[$character['characterName']] = $character['characterName'];
      }
    }
  }

  if (empty($chars)) {
    $chars[''] = 'No Characters Found';
  }

  return $chars;
}

/**
 * Function to check if the Characters already exist in the DB.
 *
 * @param array $characters
 *   A complex array full of character information.
 *
 * @return mixed
 *   A simple array of characters found or a bool FALSE.
 */
function omni_eve_api_characters_exist($characters) {
  $chars = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $result = db_select('oea_characters', 'c')->fields('c', array(
        'apiid',
        'characterid',
        'drupalid',
      ))->condition('characterid', $character['characterID'], '=')->condition('deleted', 0, '=')->execute();
      if ($result->rowCount()) {
        $chars[] = $character['characterName'];
      }
    }
  }

  if (empty($chars)) {
    $chars = FALSE;
  }

  return $chars;
}

/**
 * Function that retrieves all API's associated with a user id.
 *
 * @param int $uid
 *   A unique Drupal User ID.
 *
 * @return mixed
 *   A comlex array of character and api key information.
 */
function omni_eve_api_list_api($uid) {
  $api_list = array();
  $api_list['full'] = $api_list['simple'] = array();

  $result_a = db_select('oea_api_keys', 'a')->fields('a', array(
    'apiid',
    'keyid',
    'vcode',
    'errorid',
    'updated',
    'created',
  ))->condition('drupalid', $uid, '=')->execute();

  if ($result_a->rowCount()) {
    foreach ($result_a->fetchAll() as $row_a) {
      $api_list['full'][$row_a->apiid] = array(
        'apiid' => $row_a->apiid,
        'keyid' => $row_a->keyid,
        'vcode' => $row_a->vcode,
        'errorid' => $row_a->errorid,
        'updated' => $row_a->updated,
        'created' => $row_a->created,
      );

      if ($row_a->errorid != 0) {
        continue;
      }

      $result_b = db_select('oea_characters', 'c')->fields('c', array(
        'characterid',
        'charactername',
        'corporationid',
        'corporationname',
        'corporationticker',
        'allianceid',
        'alliancename',
        'allianceticker',
      ))->condition('drupalid', $uid, '=')->condition('deleted', 0, '=')->condition('apiid', $row_a->apiid, '=')->execute();

      if ($result_b->rowCount()) {
        foreach ($result_b->fetchAll() as $row_b) {
          $api_list['full'][$row_a->apiid]['characters'][$row_b->characterid] = array(
            'characterid' => $row_b->characterid,
            'charactername' => $row_b->charactername,
            'corporationid' => $row_b->corporationid,
            'corporationname' => $row_b->corporationname,
            'corporationticker' => $row_b->corporationticker,
            'allianceid' => $row_b->allianceid,
            'alliancename' => $row_b->alliancename,
            'allianceticker' => $row_b->allianceticker,
          );
          $api_list['simple']['characters'][$row_b->characterid] = array(
            'characterID' => $row_b->characterid,
            'characterName' => $row_b->charactername,
            'corporationID' => $row_b->corporationid,
            'corporationName' => $row_b->corporationname,
            'corporationTicker' => $row_b->corporationticker,
            'allianceID' => $row_b->allianceid,
            'allianceName' => $row_b->alliancename,
            'allianceTicker' => $row_b->allianceticker,
          );
        }
      }
    }
  }

  return $api_list;
}
