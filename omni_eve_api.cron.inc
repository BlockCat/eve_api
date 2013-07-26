<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

/**
 * Implements hook_cron().
 */
function omni_eve_api_cron() {
  if (variable_get('omni_eve_api_enable_cron', TRUE)) {
    // Queue each user in the database to check the API's for blue status and
    // other checks
    $queue = drupal_queue_get('omni_eve_api_queue_api');
    $queue->createQueue();
    $result = db_select('users', 'u')->fields('u', array(
      'uid',
      'charactername',
      'characterid',
    ))->condition('status', 1, '=')->execute();
    while ($item = $result->fetchAssoc()) {
      $queue->createItem($item);
    }

    $item = NULL;

    // Run cron for standings, but have nothing to pass so passing a NULL
    // variable.
    $queue = drupal_queue_get('omni_eve_api_get_standings');
    $queue->createQueue();
    $queue->createItem($item);

    // Run cron for corporations in the alliance, but have nothing to pass so
    // passing a NULL variable.
    $queue = drupal_queue_get('omni_eve_api_get_alliance_corporations');
    $queue->createQueue();
    $queue->createItem($item);
  }
}

/**
 * Implements hook_cron_queue_info().
 *
 * Set up ten queues that all send items to the same callback function.
 */
function omni_eve_api_cron_queue_info() {
  $queue = array();
  $queue['omni_eve_api_queue_api'] = array(
    'worker callback' => 'omni_eve_api_queue_api',
    'time' => 15,
  );
  $queue['omni_eve_api_get_standings'] = array(
    'worker callback' => 'omni_eve_api_get_standings',
    'time' => 15,
  );
  $queue['omni_eve_api_get_alliance_corporations'] = array(
    'worker callback' => 'omni_eve_api_get_alliance_corporations',
    'time' => 60,
  );
  return $queue;
}

/**
 * Queue worker callback: receives an item from the queue and performs
 * some mysterious processing task based on the item data.
 *
 * @param mixed $item
 * whatever was added to the queue via queue->createItem()
 *
 * @todo Check each characters current API's and update changes and delete
 * characters that are no longer valid.  Verify current Main Character is still
 * valid for each account.  If the main character is no longer valid, remove
 * roles.
 */
function omni_eve_api_queue_api($item) {
}

/**
 * Queue worker callback: This worker is only ever called once, but because it
 * can take a while, it is run in parallel.
 *
 * @param mixed $item
 * whatever was added to the queue via queue->createItem()
 *
 * @todo Error check for $xml
 */
function omni_eve_api_get_standings($item = NULL) {
  $query = array(
    'keyid' => variable_get('omni_eve_api_corp_keyid'),
    'vcode' => variable_get('omni_eve_api_corp_vcode'),
  );
  $xml = omni_eve_api_get_xml('standings', $query);

  if (!isset($xml->result)) {
    return NULL;
  }

  db_update('oea_blue_standings')->fields(array('checked' => 1))->condition('manual ', '0', '=')->execute();

  foreach ($xml->result->children() as $rowset) {
    foreach ($rowset->children() as $rows) {
      $row = $rows->attributes();
      db_merge('oea_blue_standings')->key(array('contactID' => (int) $row->contactID))->fields(array(
        'contactID' => (int) $row->contactID,
        'contactName' => (string) $row->contactName,
        'standing' => (string) $row->standing,
        'manual' => 0,
        'checked' => 0,
      ))->execute();
    }
  }

  db_update('oea_blue_standings')->fields(array('deleted' => 1, ))->condition('checked ', '1', '=')->condition('manual ', '0', '=')->execute();
}

/**
 * Queue worker callback: This worker is only ever called once, but because it
 * can take a while, it is run in parallel.
 *
 * @param mixed $item
 * whatever was added to the queue via queue->createItem()
 *
 * @todo Error check for $xml
 */
function omni_eve_api_get_alliance_corporations($item = NULL) {
  $xml = omni_eve_api_get_xml('alliances');

  if (!isset($xml->result->rowset->row)) {
    return NULL;
  }

  db_update('oea_alliance_corporations')->fields(array('deleted' => 1))->condition('manual ', '0', '=')->execute();
  db_truncate('oea_alliance_info')->execute();

  foreach ($xml->result->rowset->row as $row) {
    db_insert('oea_alliance_info')->fields(array(
      'allianceid' => (int) $row['allianceID'],
      'allianceticker' => (string) $row['shortName'],
      'alliancename' => (string) $row['name'],
    ))->execute();

    if ((int) $row['allianceID'] == variable_get('omni_eve_api_allianceID')) {
      foreach ($row->rowset->row as $row2) {
        $query = array('corporationID' => (int) $row2['corporationID']);
        $corp_info = omni_eve_api_get_corporation_info($query);
        db_merge('oea_alliance_corporations')->key(array('corporationid' => (int) $row2['corporationID']))->fields(array(
          'corporationid' => (int) $row2['corporationID'],
          'corporationname' => (string) $corp_info['corporationName'],
          'deleted' => 0,
          'manual' => 0,
        ))->execute();
      }
    }
  }

  $result = db_select('oea_alliance_corporations', 'ac')->fields('ac', array(
    'corporationid',
    'corporationname',
  ))->condition('deleted ', '1', '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      if (user_role_load_by_name($row->corporationname) == TRUE) {
        $user_role = new stdClass();
        $user_role->name = $row->corporationname;
        user_role_delete($user_role);
      }
    }
  }

  $result = db_select('oea_alliance_corporations', 'ac')->fields('ac', array(
    'corporationid',
    'corporationname',
  ))->condition('deleted ', '0', '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      if (user_role_load_by_name($row->corporationname) == FALSE) {
        $user_role = new stdClass();
        $user_role->name = $row->corporationname;
        if (!empty($row->corporationname)) {
          user_role_save($user_role);
        }
      }
    }
  }

  db_delete('oea_alliance_corporations')->condition('deleted ', '1', '=')->execute();
}
