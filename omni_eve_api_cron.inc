<?php

/**
 * Function to retrieve the standings for a cron job
 */
function omni_eve_api_get_standings_cron()
{
  $query = array(
    'keyid' => variable_get('omni_eve_api_corp_keyid'),
    'vcode' => variable_get('omni_eve_api_corp_vcode'),
  );
  $xml = omni_eve_api_get_xml('standings', $query);

  if (is_null($xml))
  {
    return NULL;
  }

  db_delete('oea_blue_standings') -> condition('manual', 0) -> execute();

  foreach ($xml->result->children() as $rowset)
  {
    foreach ($rowset->children() as $rows)
    {
      $row = $rows -> attributes();
      db_merge('oea_blue_standings') -> key(array('contactID' => (int)$row -> contactID)) -> fields(array(
        'contactID' => (int)$row -> contactID,
        'contactName' => (string)$row -> contactName,
        'standing' => (string)$row -> standing,
        'manual' => 0,
      )) -> execute();
    }
  }
}

/**
 * Function to retrieve the corporations in the alliance cron job
 */
function omni_eve_api_alliance_corporations_cron()
{
  $xml = omni_eve_api_get_xml('alliances');

  if (is_null($xml))
  {
    return NULL;
  }

  db_delete('oea_alliance_corporations') -> condition('manual', 0) -> execute();

  foreach ($xml->result->rowset->row as $row)
  {
    if ((int)$row['allianceID'] == variable_get('omni_eve_api_allianceID'))
    {
      foreach ($row->rowset->row as $row2)
      {
        $query = array('corporationID' => (int)$row2['corporationID'], );
        $corp_info = omni_eve_api_get_corporation_info($query);
        db_merge('oea_alliance_corporations') -> key(array('corporationid' => (int)$row2['corporationID'])) -> fields(array(
          'corporationid' => (int)$row2['corporationID'],
          'corporationname' => (string)$corp_info['corporationName'],
          'manual' => 0,
        )) -> execute();
      }
    }
  }
}
