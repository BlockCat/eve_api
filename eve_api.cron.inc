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

  $interval = variable_get('eve_api_api_alliance_fetch_cron_time', 1440 * 60);

  if (time() >= variable_get('eve_api_api_alliance_fetch_cron_next', 0) && date('G') == 2) {
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_fetch');
    $queue->createItem(1);

    variable_set('eve_api_api_alliance_fetch_cron_next', time() + $interval);
  }

  $interval = variable_get('eve_api_api_alliance_sync_cron_time', 1440 * 60);

  if (time() >= variable_get('eve_api_api_alliance_sync_cron_next', 0) && date('G') == 3) {
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_sync');
    $queue->createItem(1);

    variable_set('eve_api_api_alliance_sync_cron_next', time() + $interval);
  }

  $interval = variable_get('eve_api_api_mask_cron_time', 1440 * 60);

  if (time() >= variable_get('eve_api_api_mask_cron_next', 0) && date('G') == 1) {
    $queue = DrupalQueue::get('eve_api_cron_api_mask_sync');
    $queue->createItem(1);

    variable_set('eve_api_api_mask_cron_next', time() + $interval);
  }

  if (variable_get('eve_api_enable', FALSE)) {
    $interval = variable_get('eve_api_api_users_cron_time', 60 * 60);

    if (time() >= variable_get('eve_api_api_users_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_cron_api_user_sync');
      $result = db_query('SELECT DISTINCT uid FROM {eve_api_characters}');

      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $item) {
          $queue->createItem(array(
            'uid' => $item->uid,
            'runs' => 1,
          ));
        }
      }

      variable_set('eve_api_api_users_cron_next', time() + $interval);
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
    'time' => 15,
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
  return $queue;
}

/**
 * Verify each characters API.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_cron_api_user_sync($item) {
  if ($item['runs'] == 3) {
    watchdog('eve_api', 'Cron update User API task has failed 2 times in a row, task was removed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  $uid = (int) $item['uid'];
  $all_characters = array();

  $account = user_load($uid);

  $result = db_query('SELECT apiID, keyID, vCode FROM {eve_api_api_keys} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));

  $updated = date('Y-m-d H:i:s', time());

  $character_found = FALSE;
  $error_found = FALSE;

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $error = 0;

      $query = array(
        'keyID' => $row->keyID,
        'vCode' => $row->vCode,
      );

      $characters = eve_api_get_character_api($query);

      if (isset($characters['error'])) {
        switch ($characters['error']) {
          case 403:
            db_merge('eve_api_api_keys')->key(array('apiID' => (int) $row->apiID))->fields(array(
              'apiID' => (int) $row->apiID,
              'errorID' => (int) $characters['error'],
              'updated' => (string) $updated,
            ))->execute();

            db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error'], 'deleted' => 1))->condition('apiID', (int) $row->apiID, '=')->execute();
            db_update('eve_api_characters_roles')->fields(array('errorID' => (int) $characters['error'], 'deleted' => 1))->condition('apiID', (int) $row->apiID, '=')->execute();

            continue;
            break;

          default:
            db_merge('eve_api_api_keys')->key(array('apiID' => (int) $row->apiID))->fields(array(
              'apiID' => (int) $row->apiID,
              'errorID' => (int) $characters['error'],
              'updated' => (string) $updated,
            ))->execute();

            db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error']))->condition('apiID', (int) $row->apiID, '=')->execute();
            db_update('eve_api_characters_roles')->fields(array('errorID' => (int) $characters['error']))->condition('apiID', (int) $row->apiID, '=')->execute();

            $error_found = TRUE;
            continue;
        }
      }

      $whitelist = array();

      if (!empty($characters)) {
        foreach ($characters['characters'] as $character) {
          $whitelist[] = (int) $character['characterID'];
        }
      }

      $result = db_query('SELECT characterID FROM {eve_api_whitelist} WHERE deleted = 0 AND characterID IN (:characterIDs)', array(
        ':characterIDs' => $whitelist,
      ));

      $allow_expires = variable_get('eve_api_require_expires', FALSE) ? FALSE : !empty($characters['expires']);
      $allow_type = variable_get('eve_api_require_type', TRUE) ? $characters['type'] != 'Account' : FALSE;

      if ($result->rowCount()) {
        if ($allow_expires || ($characters['accessMask'] & 8388680) != 8388680) {
          $error = 1;
        }
      }
      else {
        if ($allow_expires || $allow_type || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
          $error = 1;
        }
      }

      db_merge('eve_api_api_keys')->key(array('apiID' => (int) $row->apiID))->fields(array(
        'apiID' => (int) $row->apiID,
        'errorID' => (int) $error,
        'updated' => (string) $updated,
      ))->execute();

      db_update('eve_api_characters')->fields(array('errorID' => (int) $error, 'deleted' => 1))->condition('apiID', (int) $row->apiID, '=')->execute();
      db_update('eve_api_characters_roles')->fields(array('errorID' => (int) $error, 'deleted' => 1))->condition('apiID', (int) $row->apiID, '=')->execute();

      if ($error == 1) {
        continue;
      }

      foreach ($characters['characters'] as $character) {
        db_merge('eve_api_characters')->key(array('characterID' => (int) $character['characterID']))->fields(array(
          'characterID' => (int) $character['characterID'],
          'uid' => (int) $uid,
          'apiID' => (int) $row->apiID,
          'characterName' => (string) $character['characterName'],
          'corporationID' => (int) $character['corporationID'],
          'corporationName' => (string) $character['corporationName'],
          'corporationTicker' => (string) $character['corporationTicker'],
          'allianceID' => (int) $character['allianceID'],
          'allianceName' => (string) $character['allianceName'],
          'allianceTicker' => (string) $character['allianceTicker'],
          'deleted' => 0,
          'errorID' => (int) $error,
        ))->execute();

        if ($account->name == (string) $character['characterName']) {
          $character_found = TRUE;
        }

        $sheet_query = $query + array('characterID' => (int) $character['characterID']);

        $character_sheet = eve_api_get_character_sheet($sheet_query);

        if (!empty($character_sheet['corporationRoles'])) {
          foreach ($character_sheet['corporationRoles'] as $roles) {
            db_merge('eve_api_characters_roles')->key(array(
              'characterID' => (int) $character['characterID'],
              'type' => 'corporationRoles',
              'roleID' => (int) $roles['roleID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'uid' => (int) $uid,
              'apiID' => (int) $row->apiID,
              'type' => 'corporationRoles',
              'roleID' => (int) $roles['roleID'],
              'roleName' => (string) $roles['roleName'],
              'deleted' => 0,
              'errorID' => (int) $error,
              ))->execute();
          }
        }

        if (!empty($character_sheet['corporationRolesAtHQ'])) {
          foreach ($character_sheet['corporationRolesAtHQ'] as $roles) {
            db_merge('eve_api_characters_roles')->key(array(
              'characterID' => (int) $character['characterID'],
              'type' => 'corporationRolesAtHQ',
              'roleID' => (int) $roles['roleID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'uid' => (int) $uid,
              'apiID' => (int) $row->apiID,
              'type' => 'corporationRolesAtHQ',
              'roleID' => (int) $roles['roleID'],
              'roleName' => (string) $roles['roleName'],
              'deleted' => 0,
              'errorID' => (int) $error,
            ))->execute();
          }
        }

        if (!empty($character_sheet['corporationRolesAtBase'])) {
          foreach ($character_sheet['corporationRolesAtBase'] as $roles) {
            db_merge('eve_api_characters_roles')->key(array(
              'characterID' => (int) $character['characterID'],
              'type' => 'corporationRolesAtBase',
              'roleID' => (int) $roles['roleID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'uid' => (int) $uid,
              'apiID' => (int) $row->apiID,
              'type' => 'corporationRolesAtBase',
              'roleID' => (int) $roles['roleID'],
              'roleName' => (string) $roles['roleName'],
              'deleted' => 0,
              'errorID' => (int) $error,
            ))->execute();
          }
        }

        if (!empty($character_sheet['corporationRolesAtOther'])) {
          foreach ($character_sheet['corporationRolesAtOther'] as $roles) {
            db_merge('eve_api_characters_roles')->key(array(
              'characterID' => (int) $character['characterID'],
              'type' => 'corporationRolesAtOther',
              'roleID' => (int) $roles['roleID'],
            ))->fields(array(
              'characterID' => (int) $character['characterID'],
              'uid' => (int) $uid,
              'apiID' => (int) $row->apiID,
              'type' => 'corporationRolesAtOther',
              'roleID' => (int) $roles['roleID'],
              'roleName' => (string) $roles['roleName'],
              'deleted' => 0,
              'errorID' => (int) $error,
            ))->execute();
          }
        }
      }
        
      $all_characters = array_replace_recursive($characters, $all_characters);
    }
  }

  if ($error_found) {
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('eve_api_cron_api_user_sync');
    $queue->createItem($item);
    return FALSE;
  }

  if (!eve_api_verify_blue($all_characters) || !$character_found) {
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
    $character_data = eve_api_get_character_info($account->name);

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
 * @return mixed
 *   A complex array if an error is generated, or TRUE if no error.
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
 * @return mixed
 *   A complex array if an error is generated, or TRUE if no error.
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
 * @return mixed
 *   A complex array if an error is generated, or TRUE if no error.
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
