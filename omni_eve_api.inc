<?php

/**
 * Function to retrieve the XML
 */
function omni_eve_api_get_xml(&$xml, $type, $query = array())
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
    'data' => array('query' => $query),
    'timeout' => 60
  );

  $html = curl_http_request($url, $options);

  try
  {
    $xml = new SimpleXMLElement($html);
  }
  catch (Exception $e)
  {
    /**
     * Do something with the error
     */
  }
}

/**
 * Function to retrieve the XML
 */
function omni_eve_api_get_standings(&$xml, $keyid, $vcode)
{
  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );
  omni_eve_api_get_xml($xml, 'standings', $query);

  if (!empty($xml -> result -> rowset[0]))
  {
    foreach ($xml->result->rowset[0] as $s)
    {
      $cstandings[(string)$s["contactID"]] = array(
        (string)$s["contactName"],
        (string)$s["standing"]
      );
    }
  }

  if (!empty($xml -> result -> rowset[1]))
  {
    foreach ($xml->result->rowset[1] as $s)
    {
      $astandings[(string)$s["contactID"]] = array(
        (string)$s["contactName"],
        (string)$s["standing"]
      );
    }
  }

  $count = 0;

  if (!empty($cstandings))
  {
    foreach ($cstandings as $i => $c)
    {
      if ($c[1] > 0)
      {
        $blues[$i][0] = $c[0];
        $blues[$i][1] = $c[1];
        $blues[$i][2] = 0;
        $count++;
      }
      elseif ($c[1] < 0)
      {
        $reds[$i][0] = $c[0];
        $reds[$i][1] = $c[1];
        $reds[$i][2] = 0;
        $count++;
      }
    }
  }

  if (!empty($astandings))
  {
    foreach ($astandings as $i => $a)
    {
      if ($a[1] > 0)
      {
        $blues[$i][0] = $a[0];
        $blues[$i][2] = $a[1];
        $count++;

        if (!isset($blues[$i][1]))
        {
          $blues[$i][1] = 0;
        }
      }
      elseif ($a[1] < 0)
      {
        $reds[$i][0] = $a[0];
        $reds[$i][2] = $a[1];
        $count++;

        if (!isset($reds[$i][1]))
        {
          $reds[$i][1] = 0;
        }
      }
    }
  }

  return array(
    $blues,
    $reds,
    $count,
  );
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
  omni_eve_api_get_xml($xml, 'keyinfo', $query);

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
function omni_eve_api_valid_characters(&$xml)
{
  return array(
    'Character 1' => t('Plugin 1'),
    'Character 2' => t('Plugin 2'),
    $xml => t($xml),
  );
}
