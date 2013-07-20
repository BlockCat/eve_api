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

  db_delete('oea_blue_standings')
  ->condition('manual', 0)
  ->execute();

  foreach ($xml->result->children() as $rowset)
  {
    foreach ($rowset->children() as $rows)
    {
      $row = $rows -> attributes();
      db_merge('oea_blue_standings')
        ->key(array('contactID' => (int) $row -> contactID))
        -> fields(array(
        'contactID' => (int) $row -> contactID,
        'contactName' => (string)$row -> contactName,
        'standing' => (string)$row -> standing,
        'manual' => 0,
      )) -> execute();
    }
  }
}
