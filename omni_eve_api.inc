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
    $xml = $cached -> data;
  }

  if (empty($username))
  {
    $html = omni_eve_api_curl_http_request($url, $query);

    try
    {
      $xml = new SimpleXMLElement($html -> data);
    }
    catch (Exception $e)
    {
      /**
       * Do something with the error
       */
      $xml = NULL;
    }
  }

  if (isset($xml))
  {
    cache_set($cache_name, $xml, 'cache', strtotime($xml -> cachedUntil));
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

    $api_data['characters'][(string)$row -> characterName] = array_merge(array(
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
    $result = db_select('oea_blue_standings', 'a') -> fields('a', 'standingid') -> condition('contactID', array(
      $character['characterID'],
      $character['corporationID'],
      $character['allianceID']
    ), 'IN') -> condition('standing', 0, '>') -> execute() -> fetchAssoc();
    if ($result -> rowCount())
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
    $result = db_select('oea_blue_standings', 'a') -> fields('a', 'standingid') -> condition('contactID', array(
      $character['characterID'],
      $character['corporationID'],
      $character['allianceID']
    ), 'IN') -> condition('standing', 0, '>') -> execute() -> fetchAssoc();
    if ($result -> rowCount())
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
