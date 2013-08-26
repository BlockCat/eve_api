<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

/**
 * Implements hook_cron().
 */
function eve_api_cron() {
  if (!variable_get('eve_api_enable_cron', TRUE)) {
    return;
  }

  $interval = variable_get('eve_api_cron_api_alliance_fetch_time', 1439 * 60);

  if (time() >= variable_get('eve_api_cron_api_alliance_fetch_next', 0) && date('G') == 2 && date('i') >= 12) {
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_fetch');
    $queue->createItem(1);

    variable_set('eve_api_cron_api_alliance_fetch_next', time() + $interval);
  }

  $interval = variable_get('eve_api_cron_api_alliance_sync_time', 1439 * 60);

  if (time() >= variable_get('eve_api_cron_api_alliance_sync_next', 0) && date('G') == 3 && date('i') >= 12) {
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_sync');
    $queue->createItem(1);

    variable_set('eve_api_cron_api_alliance_sync_next', time() + $interval);
  }

  $interval = variable_get('eve_api_cron_api_mask_time', 1439 * 60);

  if (time() >= variable_get('eve_api_cron_api_mask_next', 0) && date('w') == 0 && date('G') == 1 && date('i') >= 12) {
    $queue = DrupalQueue::get('eve_api_cron_api_mask_sync');
    $queue->createItem(1);

    variable_set('eve_api_cron_api_mask_next', time() + $interval);
  }

  if (variable_get('eve_api_enable', FALSE)) {
    $interval = variable_get('eve_api_cron_api_users_time', 59 * 60);

    if (time() >= variable_get('eve_api_cron_api_users_next', 0) && date('i') >= 18) {
      $queue = DrupalQueue::get('eve_api_cron_api_user_sync');
      $result = db_query('SELECT DISTINCT uid FROM {users} WHERE uid != 0');

      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $item) {
          $queue->createItem(array(
            'uid' => $item->uid,
            'runs' => 1,
          ));
        }
      }

      variable_set('eve_api_cron_api_users_next', time() + $interval);
    }
  }
}

/**
 * Implements hook_cron_queue_info().
 *
 * Set up ten queues that all send items to the same callback function.
 */
function eve_api_cron_queue_info() {
  $queue = array();
  $queue['eve_api_cron_api_user_sync'] = array(
    'worker callback' => 'eve_api_cron_api_user_sync',
    'time' => 60,
  );
  $queue['eve_api_cron_api_alliance_fetch'] = array(
    'worker callback' => 'eve_api_cron_api_alliance_fetch',
    'time' => 15,
  );
  $queue['eve_api_cron_api_mask_sync'] = array(
    'worker callback' => 'eve_api_cron_api_mask_sync',
    'time' => 15,
  );
  $queue['eve_api_cron_api_alliance_sync'] = array(
    'worker callback' => 'eve_api_cron_api_alliance_sync',
    'time' => 60,
  );
  $queue['eve_api_cron_api_skill_tree'] = array(
    'worker callback' => 'eve_api_cron_api_skill_tree',
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
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_cron_api_user_sync($item) {
  if ($item['runs'] == 3) {
    watchdog('eve_api', 'Cron update User API task has failed 2 times in a row, task was removed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  $uid = (int) $item['uid'];
  $all_characters = array();

  $account = user_load($uid);

  $result = db_query('SELECT apiID, keyID, vCode FROM {eve_api_keys} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));

  $updated = date('Y-m-d H:i:s', time());

  $character_found = FALSE;
  $error_found = FALSE;

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $characters = eve_api_update_character('update', $account, $row->keyID, $row->vCode, $row->apiID);
      $error_found = ($error_found == TRUE) ? $error_found : $characters['error_found'];
      $character_found = ($character_found == TRUE) ? $character_found : $characters['character_found'];
    }
  }

  if ($error_found) {
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('eve_api_cron_api_user_sync');
    $queue->createItem($item);
    return FALSE;
  }

  $character_data = eve_api_get_character_info($account->name);

  if (!eve_api_verify_blue($character_data) || !$character_found) {
    $result = db_query('SELECT corporationID, corporationName FROM {eve_api_alliance_corporations} WHERE deleted = 0');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($corporation_role = user_role_load_by_name($row->corporationName)) {
          user_multiple_role_edit(array($uid), 'remove_role', (int) $corporation_role->rid);
        }
      }
    }

    $blue_role = user_role_load(variable_get('eve_api_blue_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);

    $alliance_role = user_role_load(variable_get('eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);

    $director_role = user_role_load(variable_get('eve_api_director_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $director_role->rid);

    $ceo_role = user_role_load(variable_get('eve_api_ceo_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $ceo_role->rid);

    $default_role = user_role_load(variable_get('eve_api_unverified_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $default_role->rid);

    module_invoke_all('eve_api_user_delete', $account);

    db_update('users')->fields(array(
        'characterID' => 0,
      ))
      ->condition('uid', $uid, '=')
      ->execute();
  }
  else {
    $result = db_query('SELECT corporationID, corporationName FROM {eve_api_alliance_corporations} WHERE deleted = 0');

    if ($corporation_role = user_role_load_by_name($character_data['corporationName'])) {
      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $row) {
          if (($corporation_role_2 = user_role_load_by_name($row->corporationName)) && $row->corporationID != $character_data['corporationID']) {
            user_multiple_role_edit(array($uid), 'remove_role', $corporation_role_2->rid);
          }
        }
      }

      user_multiple_role_edit(array($uid), 'add_role', $corporation_role->rid);

      if (eve_api_character_is_director($character_data['characterID'])) {
        $director_role = user_role_load(variable_get('eve_api_director_role', 2));
        user_multiple_role_edit(array($uid), 'add_role', $director_role->rid);
      }
      else {
        $director_role = user_role_load(variable_get('eve_api_director_role', 2));
        user_multiple_role_edit(array($uid), 'remove_role', $director_role->rid);
      }

      if (eve_api_character_is_ceo($character_data['characterID'])) {
        $ceo_role = user_role_load(variable_get('eve_api_ceo_role', 2));
        user_multiple_role_edit(array($uid), 'add_role', $ceo_role->rid);
      }
      else {
        $ceo_role = user_role_load(variable_get('eve_api_ceo_role', 2));
        user_multiple_role_edit(array($uid), 'remove_role', $ceo_role->rid);
      }

      $default_role = user_role_load(variable_get('eve_api_unverified_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $default_role->rid);

      $blue_role = user_role_load(variable_get('eve_api_blue_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);

      $alliance_role = user_role_load(variable_get('eve_api_alliance_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', $alliance_role->rid);
    }
    else {
      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $row) {
          if ($corporation_role = user_role_load_by_name($row->corporationName)) {
            user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);
          }
        }
      }

      $default_role = user_role_load(variable_get('eve_api_unverified_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $default_role->rid);

      $alliance_role = user_role_load(variable_get('eve_api_alliance_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);

      $director_role = user_role_load(variable_get('eve_api_director_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $director_role->rid);

      $ceo_role = user_role_load(variable_get('eve_api_ceo_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $ceo_role->rid);

      $blue_role = user_role_load(variable_get('eve_api_blue_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', $blue_role->rid);
    }
  }

  module_invoke_all('eve_api_user_update', $account);

  return TRUE;
}

/**
 * Get current alliance standings.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_cron_api_alliance_fetch($item = NULL) {
  if ($item == 3) {
    watchdog('eve_api', 'Cron Standings task has failed 2 times in a row, task was removed.', NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  $key_id = variable_get('eve_api_corp_keyid', '');
  $v_code = variable_get('eve_api_corp_vcode', '');

  if (empty($key_id) || empty($v_code)) {
    return FALSE;
  }

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $xml = eve_api_get_xml('ContactList', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Alliance Contact List API Data. (' . drupal_http_build_query($query) . ') Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);

    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_fetch');
    $queue->createItem($item);

    return FALSE;
  }

  db_update('eve_api_blue_standings')->fields(array('checked' => 1))->condition('manual', 0, '=')->execute();

  foreach ($xml->result->children() as $rowset) {
    foreach ($rowset->children() as $rows) {
      $row = $rows->attributes();

      db_merge('eve_api_blue_standings')->key(array('contactID' => (int) $row->contactID))->fields(array(
        'contactID' => (int) $row->contactID,
        'contactName' => (string) $row->contactName,
        'standing' => (float) $row->standing,
        'manual' => 0,
        'checked' => 0,
      ))->execute();
    }
  }

  db_update('eve_api_blue_standings')->fields(array('deleted' => 1))->condition('checked', 1, '=')->condition('manual', 0, '=')->execute();

  return TRUE;
}

/**
 * Get current corporations in the alliance.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_cron_api_alliance_sync($item = NULL) {
  if ($item == 3) {
    watchdog('eve_api', 'Cron Import Alliance API task has failed 2 times in a row, task was removed.', NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  $xml = eve_api_get_xml('AllianceList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Alliance List API Data. Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);

    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_sync');
    $queue->createItem($item);

    return FALSE;
  }

  db_update('eve_api_alliance_corporations')->fields(array('deleted' => 1))->condition('manual', 0, '=')->execute();
  db_truncate('eve_api_alliance_info')->execute();

  foreach ($xml->result->rowset->children() as $rows_a) {
    $row_a = $rows_a->attributes();

    db_insert('eve_api_alliance_info')->fields(array(
      'allianceID' => (int) $row_a->allianceID,
      'allianceTicker' => (string) $row_a->shortName,
      'allianceName' => (string) $row_a->name,
    ))->execute();

    if ((int) $row_a->allianceID == variable_get('eve_api_allianceID', 0)) {
      foreach ($rows_a->rowset->children() as $rows_b) {
        $row_b = $rows_b->attributes();

        $result = db_query('SELECT corporationID FROM {eve_api_alliance_corporations} WHERE corporationID = :corporationID AND deleted = 0', array(
          ':corporationID' => (int) $row_b->corporationID,
        ));

        if ($result->rowCount()) {
          continue;
        }

        $query = array('corporationID' => (int) $row_b->corporationID);
        $corp_info = eve_api_get_corporation_info($query);

        if (isset($corp_info['error'])) {
          continue;
        }

        if (!($role = user_role_load_by_name($corp_info['corporationName']))) {
          $user_role = new stdClass();
          $user_role->name = $corp_info['corporationName'];

          if (!empty($corp_info['corporationName'])) {
            user_role_save($user_role);
          }

          $role = user_role_load_by_name($corp_info['corporationName']);
        }

        db_merge('eve_api_alliance_corporations')->key(array('corporationID' => (int) $row_b->corporationID))->fields(array(
          'corporationID' => (int) $row_b->corporationID,
          'rid' => (int) $role->rid,
          'corporationName' => (string) $corp_info['corporationName'],
          'corporationTicker' => (string) $corp_info['corporationTicker'],
          'ceoID' => (int) $corp_info['ceoID'],
          'ceoName' => (string) $corp_info['ceoName'],
          'deleted' => 0,
          'manual' => 0,
        ))->execute();
      }
    }
  }

  $result = db_query('SELECT corporationID, corporationName FROM {eve_api_alliance_corporations} WHERE deleted = 1');

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      if (user_role_load_by_name($row->corporationName) == TRUE) {
        $user_role = new stdClass();
        $user_role->name = $row->corporationName;
        user_role_delete($user_role);
      }
    }
  }

  db_delete('eve_api_alliance_corporations')->condition('deleted', 1, '=')->execute();

  if (!variable_get('eve_api_enable', FALSE)) {
    variable_set('eve_api_enable', TRUE);
    menu_rebuild();
  }

  return TRUE;
}

/**
 * Get the latest access mask from the EVE API.
 * 
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_cron_api_mask_sync($item = NULL) {
  if ($item == 3) {
    watchdog('eve_api', 'Cron Access Mask task has failed 2 times in a row, task was removed.', NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  $xml = eve_api_get_xml('callList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve_api', 'Failed getting Access Mask API Data. Error: ' . (int) $xml, NULL, WATCHDOG_WARNING);

    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_cron_api_mask_sync');
    $queue->createItem($item);

    return FALSE;
  }

  db_truncate('eve_api_access_mask')->execute();

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

      db_insert('eve_api_access_mask')->fields(array(
        'accessmask' => (int) $row_b->accessMask,
        'type' => (string) $row_b->type,
        'name' => (string) $row_b->name,
        'description' => (string) $row_b->description,
      ))->execute();
    }
  }

  return TRUE;
}

/**
 * Get the latest skill tree from the EVE API.
 * 
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 *
 * @return bool
 *   TRUE or FALSE.
 */
function eve_api_cron_api_skill_tree($item = NULL) {
  if ($item == 3) {
    watchdog('eve_api', 'Cron Skill Tree task has failed 2 times in a row, task was removed.', NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  $api_data = eve_api_get_skill_tree();

  if (isset($api_data['error'])) {
    watchdog('eve_api', 'Failed getting Skill Tree API Data. Error: ' . (int) $api_data['error'], NULL, WATCHDOG_WARNING);

    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_cron_api_skill_tree');
    $queue->createItem($item);

    return FALSE;
  }

  db_truncate('eve_api_sheet_groups')->execute();
  db_truncate('eve_api_sheet_types')->execute();

  foreach ($api_data['group'] as $rows) {
    db_insert('eve_api_sheet_groups')->fields(array(
      'groupID' => (int) $rows['groupID'],
      'groupName' => (string) $rows['groupName'],
    ))->execute();
  }

  foreach ($api_data['type'] as $rows) {
    db_insert('eve_api_sheet_types')->fields(array(
      'groupID' => (int) $rows['groupID'],
      'typeID' => (int) $rows['typeID'],
      'typeName' => (string) $rows['typeName'],
    ))->execute();
  }

  return TRUE;
}
