<?php

/**
 * Function to retrieve the XML
 */
function omni_eve_api_get_xml($type, $query = array())
{
  $url = 'https://api.eveonline.com';

  if ($type == 'standings')
  {
    $url .= "/corp/ContactList.xml.aspx";
  }
  elseif ($type == 'alliances')
  {
    $url .= "/eve/AllianceList.xml.aspx";
  }
  elseif ($type == 'corp')
  {
    $url .= "/corp/CorporationSheet.xml.aspx";
  }
  elseif ($type == 'charsheet')
  {
    $url .= "/char/CharacterSheet.xml.aspx";
  }
  elseif ($type == 'facwar')
  {
    $url .= "/char/FacWarStats.xml.aspx";
  }
  elseif ($type == 'find')
  {
    $url .= "/eve/CharacterID.xml.aspx";
  }
  elseif ($type == 'name')
  {
    $url .= "/eve/CharacterName.xml.aspx";
  }
  elseif ($type == 'calllist')
  {
    $url .= "/api/callList.xml.aspx";
  }
  else
  {
    $url .= "/account/APIKeyInfo.xml.aspx";
  }

  $cache_name = 'xml' . $type . str_replace('=', '', str_replace('&', '', drupal_http_build_query($query)));

  if ($cached = cache_get($cache_name, 'cache'))
  {
    $html = $cached -> data;
  }

  if (empty($username))
  {
    $html = omni_eve_api_curl_http_request($url, $query);

    if ($html -> code == 403 || $html -> code == 400)
    {
      $html = $xml = NULL;
    }
    elseif ($html -> code != 200)
    {
      drupal_set_message('omni_eve_api: EVE API returned an error with the HTTP request ' . $html -> code . '. Full Error: ' . $html -> error, 'error');
      $html = $xml = NULL;
    }
  }

  if (isset($html))
  {
    try
    {
      $xml = new SimpleXMLElement($html -> data);
    }
    catch (Exception $e)
    {
      drupal_set_message('omni_eve_api: Unable to process XML. Full Error: ' . $e -> getMessage(), 'error');
      $xml = NULL;
    }

    if ($xml -> error)
    {
      drupal_set_message('omni_eve_api: EVE API returned an error with the API data. Error Code: ' . $xml -> error['code'] . ' Error Message:' . $xml -> error, 'error');
      $xml = NULL;
    }
  }

  if (isset($xml))
  {
    cache_set($cache_name, $html, 'cache', strtotime($xml -> cachedUntil));
  }

  return $xml;
}

/**
 * Function to retrieve individual characters API info
 */
function omni_eve_api_get_character_api($query)
{
  $xml = omni_eve_api_get_xml('keyinfo', $query);

  if (is_null($xml))
  {
    return NULL;
  }

  $api_data = array();
  $api_info = $xml -> result -> key -> attributes();
  $api_data['accessMask'] = (int)$api_info -> accessMask;
  $api_data['type'] = (string)$api_info -> type;
  $api_data['expires'] = ((string)$api_info -> expires) ? true : false;

  foreach ($xml->result->key->rowset->children() as $rows)
  {
    $row = $rows -> attributes();

    $query = array('corporationID' => (int)$row -> corporationID, );

    $api_data['characters'][(int)$row -> characterID] = array_merge(array(
      'characterName' => (string)$row -> characterName,
      'characterID' => (int)$row -> characterID,
      'corporationName' => (string)$row -> corporationName,
      'corporationID' => (int)$row -> corporationID
    ), omni_eve_api_get_corporation_info($query));
  }

  return $api_data;
}

/**
 * Function to retrieve corporation info
 */
function omni_eve_api_get_corporation_info($query)
{
  $xml = omni_eve_api_get_xml('corp', $query);

  if (is_null($xml))
  {
    return NULL;
  }

  $api_data = array(
    'corporationID' => (string)$xml -> result -> corporationID,
    'corporationName' => (string)$xml -> result -> corporationName,
    'ticker' => (string)$xml -> result -> ticker,
    'allianceID' => (int)$xml -> result -> allianceID,
    'allianceName' => (string)$xml -> result -> allianceName,
  );

  return $api_data;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_verify_blue($characters)
{
  foreach ($characters['characters'] as $character)
  {
    $result = db_select('oea_blue_standings', 'a') -> fields('a', array('contactID')) -> condition('contactID', array(
      $character['characterID'],
      $character['corporationID'],
      $character['allianceID']
    ), 'IN') -> condition('standing', 0, '>') -> execute();
    if ($result -> fetchField())
    {
      return true;
    }

    if ($character['allianceID'] == variable_get('omni_eve_api_allianceID') || $character['corporationID'] == variable_get('omni_eve_api_corporationID'))
    {
      return true;
    }

    $result = db_select('oea_alliance_corporations', 'c') -> fields('c', array('corporationid')) -> condition('corporationid', array($character['corporationID'], ), 'IN') -> execute();
    if ($result -> fetchField())
    {
      return true;
    }
  }

  return false;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_valid_characters($characters)
{
  $chars = array();

  foreach ($characters['characters'] as $character)
  {
    $result = db_select('oea_blue_standings', 'a') -> fields('a', array('contactID')) -> condition('contactID', array(
      $character['characterID'],
      $character['corporationID'],
      $character['allianceID']
    ), 'IN') -> condition('standing', 0, '>') -> execute();
    if ($result -> fetchField())
    {
      $chars[$character['characterName']] = $character['characterName'];
      continue;
    }
    $result = db_select('oea_alliance_corporations', 'c') -> fields('c', array('corporationid')) -> condition('corporationid', array($character['corporationID'], ), 'IN') -> execute();
    if ($result -> fetchField())
    {
      $chars[$character['characterName']] = $character['characterName'];
    }
  }

  if (empty($chars))
  {
    $chars['none'] = 'No Characters Found';
  }

  return $chars;
}

/**
 * Function to compare the Characters retrieved against the current registered characters
 */
function omni_eve_api_characters_exist($characters)
{
  $chars = array();

  foreach ($characters['characters'] as $character)
  {
    $result = db_select('oea_characters', 'c') -> fields('c', array(
      'accountid',
      'characterid',
      'drupalid'
    )) -> condition('characterid ', array($character['characterID'], ), 'IN') -> execute();
    if ($result -> fetchField())
    {
      $chars[] = $character['characterName'];
    }
  }

  if (empty($chars))
  {
    $chars = FALSE;
  }

  return $chars;
}
