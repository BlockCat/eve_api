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

  $options = array(
    'method' => 'POST',
    'data' => $query,
    'timeout' => 60
  );

  $html = curl_http_request($url, $options);

  try
  {
    $xml = new SimpleXMLElement($html->data);
  }
  catch (Exception $e)
  {
    /**
     * Do something with the error
     */
  }

  return $xml;
}

/**
 * Function to verify API is valid
 */
function omni_eve_api_get_api_status(&$xml, $keyid, $vcode)
{
  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );
  $xml = omni_eve_api_get_xml('keyinfo', $query);

  $api_data = array();
  $api_info = $xml -> result -> key -> attributes();
  $api_data['api_mask'] = (int)$api_info -> accessMask;
  $api_data['api_type'] = (string)$api_info -> type;
  $api_data['api_expires'] = ((string)$api_info -> expires) ? true : false;

  foreach ($xml->result->key->rowset->children() as $rows)
  {
    $row = $rows -> attributes();
    $api_data['characters'][] = array(
      'characterName' => (string)$row -> characterName,
      'characterID' => (int)$row -> characterID,
      'corporationName' => (string)$row -> corporationName,
      'corporationID' => (int)$row -> corporationID
    );
  }

  return $api_data;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_verify_blue_status(&$xml)
{
  return NULL;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_valid_characters(&$xml, $characters)
{
  $chars = array();

  foreach ($characters['characters'] as $character)
  {
    $chars[$character['characterName']] = $character['characterName'];
  }

  return $chars;
}
