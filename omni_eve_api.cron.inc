<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

/**
 * Implements hook_cron().
 */
function omni_eve_api_cron($force = FALSE) {
  if (variable_get('omni_eve_api_enable_cron', TRUE)) {
    $short_threshold = variable_get('omni_eve_api_cron_short_threshold', '3600');
    $short_last_run = variable_get('omni_eve_api_cron_short_last_run', '0');
    $long_threshold = variable_get('omni_eve_api_cron_long_threshold', '86400');
    $long_last_run = variable_get('omni_eve_api_cron_long_last_run', '0');

    // Queue each user in the database to check the API's for blue status and
    // other checks.
    if ((time() - $short_last_run) > $short_threshold || $force) {
      // Set start time.
      variable_set('omni_eve_api_cron_short_last_run', time());

      $queue = DrupalQueue::get('omni_eve_api_queue_api');
      $queue->createQueue();
      $result = db_select('users', 'u')->fields('u', array(
        'uid',
        'charactername',
        'characterid',
      ))->condition('status', 1, '=')->execute();

      while ($item = $result->fetchAssoc()) {
        $queue->createItem($item);
      }
    }

    if ((time() - $long_last_run) > $long_threshold || $force) {
      // Set start time.
      variable_set('omni_eve_api_cron_long_last_run', time());

      $item = NULL;

      // Run cron for standings, but have nothing to pass so passing a NULL
      // variable.
      $queue = DrupalQueue::get('omni_eve_api_get_standings');
      $queue->createQueue();
      $queue->createItem($item);

      // Run cron for corporations in the alliance, but have nothing to pass so
      // passing a NULL variable.
      $queue = DrupalQueue::get('omni_eve_api_get_alliance_corporations');
      $queue->createQueue();
      $queue->createItem($item);

      // Run cron to get latest access mask definitions.
      $queue = DrupalQueue::get('omni_eve_api_get_access_mask');
      $queue->createQueue();
      $queue->createItem($item);
    }
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
  $queue['omni_eve_api_get_access_mask'] = array(
    'worker callback' => 'omni_eve_api_get_access_mask',
    'time' => 15,
  );
  $queue['omni_eve_api_get_alliance_corporations'] = array(
    'worker callback' => 'omni_eve_api_get_alliance_corporations',
    'time' => 60,
  );
  return $queue;
}

/**
 * Verify each characters API.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @todo Check each characters current API's and update changes and delete
 * characters that are no longer valid.  Verify current Main Character is still
 * valid for each account.  If the main character is no longer valid, remove
 * roles.
 */
function omni_eve_api_queue_api($item) {
  $uid = $item;
  $all_characters = array();

  $result = db_select('oea_api_keys', 'a')->fields('a', array(
    'apiid',
    'keyid',
    'vcode',
  ))->condition('drupalid', $uid, '=')->execute();

  $updated = date('Y-m-d H:i:s', time());

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $error = 0;

      $query = array(
        'keyid' => $row->keyid,
        'vcode' => $row->vcode,
      );

      $characters = omni_eve_api_get_character_api($query);

      if (isset($characters['error'])) {
        $new_api_key_id = db_merge('oea_api_keys')->key(array('apiid' => $row->apiid))->fields(array(
          'apiid' => $row->apiid,
          'errorid' => 2,
          'updated' => $updated,
        ))->execute();

        continue;
      }

      $whitelist_characters = array();

      if (!empty($characters)) {
        foreach ($characters['characters'] as $character) {
          $whitelist_characters[] = $character['characterID'];
        }
      }

      $result = db_select('oea_whitelist', 'w')->fields('w', array('characterid'))->condition('characterid', $whitelist_characters, 'IN')->condition('deleted', 0, '=')->execute();

      if ($result->rowCount()) {
        if ($characters['expires'] || $characters['accessMask'] != 8388680) {
          drupal_set_message(t('Invalid API Mask/Type/Expiry, characters disabled.'), 'status');
          $error = 1;
        }
      }
      else {
        if ($characters['expires'] || $characters['type'] != 'Account' || $characters['accessMask'] != variable_get('omni_eve_api_access_mask', '268435455')) {
          drupal_set_message(t('Invalid API Mask/Type/Expiry, characters disabled.'), 'status');
          $error = 1;
        }
      }

      $new_api_key_id = db_merge('oea_api_keys')->key(array('apiid' => $row->apiid))->fields(array(
        'apiid' => $row->apiid,
        'errorid' => $error,
        'updated' => $updated,
      ))->execute();

      db_update('oea_characters')->fields(array('errorid' => $error, 'deleted' => 1))->condition('apiid', $row->apiid, '=')->execute();

      foreach ($characters['characters'] as $character) {
        db_merge('oea_characters')->key(array('characterid' => $character['characterID']))->fields(array(
          'characterid' => $character['characterID'],
          'drupalid' => $uid,
          'apiid' => $row->apiid,
          'charactername' => $character['characterName'],
          'corporationid' => $character['corporationID'],
          'corporationname' => $character['corporationName'],
          'corporationticker' => $character['corporationTicker'],
          'allianceid' => $character['allianceID'],
          'alliancename' => $character['allianceName'],
          'allianceticker' => $character['allianceTicker'],
          'deleted' => 0,
          'errorid' => $error,
        ))->execute();
      }

      $all_characters = array_replace_recursive($characters, $all_characters);
    }
  }

  if (!omni_eve_api_verify_blue($all_characters)) {
    $result = db_select('oea_alliance_corporations', 'ac')->fields('ac', array(
      'corporationid',
      'corporationname',
    ))->condition('deleted', '0', '=')->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($corporation_role = user_role_load_by_name($row->corporationname)) {
          user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);
        }
      }
    }

    if ($alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', '2'))) {
      user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);
    }

    if ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', '2'))) {
      user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);
    }

    db_update('users')->fields(array('charactername' => '', 'characterid' => 0))->condition('uid', $uid, '=')->execute();
    return FALSE;
  }
  else {
    $row = db_select('users', 'u')->fields('a', array('charactername'))->condition('uid', $uid, '=')->execute()->fetchAssoc();
    $character_data = omni_eve_api_get_character_info($row['charactername']);

    $update_rid_2 = NULL;

    if ($corporation_role = user_role_load_by_name($character_data['corporationName'])) {
      $update_rid = $corporation_role->rid;

      if ($alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', '2'))) {
        $update_rid_2 = $alliance_role->rid;
      }
    }
    elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', '2'))) {
      $update_rid = $blue_role->rid;
    }
    else {
      $default_role = user_role_load_by_name('authenticated user');
      $update_rid = $default_role->rid;
    }

    user_multiple_role_edit(array($uid), 'add_role', $update_rid);

    if (isset($update_rid_2)) {
      user_multiple_role_edit(array($uid), 'add_role', $update_rid_2);
    }

    return TRUE;
  }
}

/**
 * Get current alliance standings.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @return mixed
 *   A complex array if an error is generated, or TRUE if no error.
 */
function omni_eve_api_get_standings($item = NULL) {
  $query = array(
    'keyid' => variable_get('omni_eve_api_corp_keyid', ''),
    'vcode' => variable_get('omni_eve_api_corp_vcode', ''),
  );
  $xml = omni_eve_api_get_xml('ContactList', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>' . print_r($xml, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  db_update('oea_blue_standings')->fields(array('checked' => 1))->condition('manual', '0', '=')->execute();

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

  db_update('oea_blue_standings')->fields(array('deleted' => 1))->condition('checked', '1', '=')->condition('manual', '0', '=')->execute();

  return TRUE;
}

/**
 * Get current corporations in the alliance.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @return mixed
 *   A complex array if an error is generated, or TRUE if no error.
 */
function omni_eve_api_get_alliance_corporations($item = NULL) {
  $xml = omni_eve_api_get_xml('AllianceList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>' . print_r($xml, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  db_update('oea_alliance_corporations')->fields(array('deleted' => 1))->condition('manual', '0', '=')->execute();
  db_truncate('oea_alliance_info')->execute();

  foreach ($xml->result->rowset->children() as $rows_a) {
    $row_a = $rows_a->attributes();

    db_insert('oea_alliance_info')->fields(array(
      'allianceid' => (int) $row_a->allianceID,
      'allianceticker' => (string) $row_a->shortName,
      'alliancename' => (string) $row_a->name,
    ))->execute();

    if ((int) $row_a->allianceID == variable_get('omni_eve_api_allianceID', '0')) {
      foreach ($rows_a->rowset->children() as $rows_b) {
        $row_b = $rows_b->attributes();

        $query = array('corporationID' => (int) $row_b->corporationID);
        $corp_info = omni_eve_api_get_corporation_info($query);

        if (isset($corp_info['error'])) {
          continue;
        }

        db_merge('oea_alliance_corporations')->key(array('corporationid' => (int) $row_b->corporationID))->fields(array(
          'corporationid' => (int) $row_b->corporationID,
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
  ))->condition('deleted', '1', '=')->execute();

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
  ))->condition('deleted', '0', '=')->execute();

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

  db_delete('oea_alliance_corporations')->condition('deleted', '1', '=')->execute();

  return TRUE;
}

/**
 * Get the latest access mask from the EVE API.
 * 
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @return mixed
 *   A complex array if an error is generated, or TRUE if no error.
 */
function omni_eve_api_get_access_mask($item = NULL) {
  $xml = omni_eve_api_get_xml('callList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>' . print_r($xml, TRUE) . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
  }

  db_truncate('oea_access_mask')->execute();

  foreach ($xml->result->children() as $rowset) {
    $row_a = $rowset->attributes();

    if ($row_a->name == 'callGroups') {
      continue;
    }

    foreach ($rowset->children() as $rows) {
      $row_b = $rows->attributes();

      if ($row_b->type == 'Corporation') {
        continue;
      }

      db_insert('oea_access_mask')->fields(array(
        'accessmask' => $row_b->accessMask,
        'type' => $row_b->type,
        'name' => $row_b->name,
        'description' => $row_b->description,
      ))->execute();
    }
  }

  return TRUE;
}
