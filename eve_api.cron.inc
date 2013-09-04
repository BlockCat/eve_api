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
    'time' => 60,
  );
  $queue['eve_api_cron_api_mask_sync'] = array(
    'worker callback' => 'eve_api_cron_api_mask_sync',
    'time' => 60,
  );
  $queue['eve_api_cron_api_alliance_sync'] = array(
    'worker callback' => 'eve_api_cron_api_alliance_sync',
    'time' => 60,
  );
  $queue['eve_api_cron_api_skill_tree'] = array(
    'worker callback' => 'eve_api_cron_api_skill_tree',
    'time' => 60,
  );
  $queue['eve_api_cron_import_character_sheet'] = array(
    'worker callback' => 'eve_api_cron_import_character_sheet',
    'time' => 60,
  );
  $queue['eve_api_cron_import_character_info'] = array(
    'worker callback' => 'eve_api_cron_import_character_info',
    'time' => 60,
  );
  $queue['eve_api_cron_import_skill_in_training'] = array(
    'worker callback' => 'eve_api_cron_import_skill_in_training',
    'time' => 60,
  );
  $queue['eve_api_cron_import_wallet_journal'] = array(
    'worker callback' => 'eve_api_cron_import_wallet_journal',
    'time' => 60,
  );
  $queue['eve_api_cron_import_contracts'] = array(
    'worker callback' => 'eve_api_cron_import_contracts',
    'time' => 60,
  );
  $queue['eve_api_cron_import_mail'] = array(
    'worker callback' => 'eve_api_cron_import_mail',
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
  $account = user_load($uid);

  if ($uid == 0) {
    return FALSE;
  }

  $result = db_query('SELECT apiID, keyID, vCode FROM {eve_api_keys} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));

  $character_found = FALSE;
  $error_found = FALSE;

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $characters = eve_api_update_key($account, $row->keyID, $row->vCode, $row->apiID);
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

  $character_data = eve_api_get_basic_character_info($account->name);

  if (!eve_api_verify_blue($character_data) || !$character_found) {
    $result = db_query('SELECT corporationID, corporationName FROM {eve_api_alliance_corporations}');

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
    $result = db_query('SELECT corporationID, corporationName FROM {eve_api_alliance_corporations}');

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

  db_delete('eve_api_blue_standings')->condition('checked', 1, '=')->condition('manual', 0, '=')->execute();

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

  db_update('eve_api_alliance_corporations')->fields(array('checked' => 1))->condition('manual', 0, '=')->execute();
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

        $result = db_query('SELECT corporationID FROM {eve_api_alliance_corporations} WHERE corporationID = :corporationID AND manual = 0', array(
          ':corporationID' => (int) $row_b->corporationID,
        ));

        if ($result->rowCount()) {
          db_update('eve_api_alliance_corporations')->fields(array('checked' => 0))->condition('corporationID', (int) $row_b->corporationID, '=')->execute();
          continue;
        }

        $query = array('corporationID' => (int) $row_b->corporationID);
        $corp_info = eve_api_get_corporation_sheet_api($query);

        if (isset($corp_info['error'])) {
          db_update('eve_api_alliance_corporations')->fields(array('checked' => 0))->condition('corporationID', (int) $row_b->corporationID, '=')->execute();
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
          'checked' => 0,
          'manual' => 0,
        ))->execute();
      }
    }
  }

  $result = db_query('SELECT corporationID, corporationName FROM {eve_api_alliance_corporations} WHERE checked = 1');

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      if (user_role_load_by_name($row->corporationName) == TRUE) {
        user_role_delete($row->corporationName);
      }
    }
  }

  db_delete('eve_api_alliance_corporations')->condition('checked', 1, '=')->execute();

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
  db_truncate('eve_api_access_mask_groups')->execute();

  foreach ($xml->result->children() as $rowset) {
    $row_a = $rowset->attributes();

    switch ((string) $row_a->name) {
      case 'callGroups':
        foreach ($rowset->children() as $rows) {
          $row_b = $rows->attributes();

          db_insert('eve_api_access_mask_groups')->fields(array(
            'groupID' => (int) $row_b->groupID,
            'name' => (string) $row_b->name,
            'description' => (string) $row_b->description,
          ))->execute();
        }
        break;

      case 'calls':
        foreach ($rowset->children() as $rows) {
          $row_b = $rows->attributes();

          db_insert('eve_api_access_mask')->fields(array(
            'accessmask' => (int) $row_b->accessMask,
            'type' => (string) $row_b->type,
            'name' => (string) $row_b->name,
            'groupID' => (int) $row_b->groupID,
            'description' => (string) $row_b->description,
          ))->execute();
        }
        break;
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

  $api_data = eve_api_get_skill_tree_api();

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

/**
 * Cron that imports the character sheet of a single character.
 *
 * @param array $item
 *   The characterID and API Key and vCode.
 */
function eve_api_cron_import_character_sheet($item) {
  $character_id = (int) $item['characterID'];
  $query = (array) $item['query'];

  $sheet_query = $query + array('characterID' => (int) $character_id);
  $character_sheet = eve_api_get_character_sheet_api($sheet_query);
  $error = 0;

  if (isset($character_sheet['errorCode'])) {
    $error = (int) $character_sheet['errorCode'];
  }

  if (isset($character_sheet['error'])) {
    $error = (int) $character_sheet['error'];

    db_merge('eve_api_character_sheet')->key(array(
      'characterID' => (int) $character_id,
    ))->fields(array(
      'characterID' => (int) $character_id,
      'errorID' => (int) $error,
    ))->execute();

    return FALSE;
  }
  else {
    db_delete('eve_api_character_sheet_attribute_enhancers')->condition('characterID', (int) $character_id, '=')->execute();
    db_delete('eve_api_character_roles')->condition('characterID', (int) $character_id, '=')->execute();
    db_delete('eve_api_character_sheet_titles')->condition('characterID', (int) $character_id, '=')->execute();

    foreach ($character_sheet as $type => $data) {
      switch ((string) $type) {
        case 'skills':
          foreach ($data as $skillsdata) {
            db_merge('eve_api_character_sheet_skills')->key(array(
              'characterID' => (int) $character_id,
              'typeID' => (int) $skillsdata['typeID'],
            ))->fields(array(
              'characterID' => (int) $character_id,
              'typeID' => (int) $skillsdata['typeID'],
              'skillpoints' => (int) $skillsdata['skillpoints'],
              'level' => (int) $skillsdata['level'],
              'published' => (int) $skillsdata['published'],
            ))->execute();
          }
          break;

        case 'certificates':
          foreach ($data as $certificatesdata) {
            db_merge('eve_api_character_sheet_certificates')->key(array(
              'characterID' => (int) $character_id,
              'certificateID' => (int) $certificatesdata['certificateID'],
            ))->fields(array(
              'characterID' => (int) $character_id,
              'certificateID' => (int) $certificatesdata['certificateID'],
            ))->execute();
          }
          break;

        case 'corporationTitles':
          foreach ($data as $titlesdata) {
            db_merge('eve_api_character_sheet_titles')->key(array(
              'characterID' => (int) $character_id,
              'titleID' => (int) $titlesdata['titleID'],
            ))->fields(array(
              'characterID' => (int) $character_id,
              'titleID' => (int) $titlesdata['titleID'],
              'titleName' => (int) $titlesdata['titleName'],
            ))->execute();
          }
          break;

        case 'roles':
          foreach ($data as $rolename => $roledata) {
            foreach ($roledata as $roles) {
              db_merge('eve_api_character_roles')->key(array(
                'characterID' => (int) $character_id,
                'type' => (string) $rolename,
                'roleID' => (int) $roles['roleID'],
              ))->fields(array(
                'characterID' => (int) $character_id,
                'type' => (string) $rolename,
                'roleID' => (int) $roles['roleID'],
                'roleName' => (string) $roles['roleName'],
              ))->execute();
            }
          }
          break;

        case 'attributeEnhancers':
          foreach ($data as $attributename => $attributedata) {
            $query_data = db_insert('eve_api_character_sheet_attribute_enhancers');

            $fields = array(
              'characterID' => (int) $character_id,
              'bonusName' => (string) $attributename,
            );

            foreach ($attributedata as $attribute => $attributes) {
              if (empty($attributes)) {
                continue;
              }

              $fields += array(
                (string) $attribute => (string) $attributes,
              );
            }

            $query_data->fields($fields);
            $query_data->execute();
          }
          break;

        case 'info':
          $query_data = db_merge('eve_api_character_sheet')->key(array(
            'characterID' => (int) $character_id,
          ));

          $fields = array(
            'characterID' => (int) $character_id,
            'errorID' => (int) $error,
          );

          $ignored = array(
            'corporationName',
            'corporationID',
            'allianceName',
            'allianceID',
            'name',
          );

          foreach ($data as $infoname => $infodata) {
            if (in_array($infoname, $ignored)) {
              continue;
            }

            if (empty($infodata)) {
              continue;
            }

            $fields += array(
              (string) $infoname => (string) $infodata,
            );
          }

          $query_data->fields($fields);
          $query_data->execute();
          break;

        default:
          break;
      }
    }
  }

  return TRUE;
}

/**
 * Cron that imports the character info of a single character.
 *
 * @param array $item
 *   The characterID and API Key and vCode.
 */
function eve_api_cron_import_character_info($item) {
  $character_id = (int) $item['characterID'];
  $query = (array) $item['query'];

  $info_query = $query + array('characterID' => (int) $character_id);
  $character_info = eve_api_get_character_info_api($info_query);
  $error = 0;

  if (isset($character_info['errorCode'])) {
    $error = (int) $character_info['errorCode'];
  }

  if (isset($character_info['error'])) {
    $error = (int) $character_info['error'];

    db_merge('eve_api_character_info')->key(array(
      'characterID' => (int) $character_id,
    ))->fields(array(
      'characterID' => (int) $character_id,
      'errorID' => (int) $error,
    ))->execute();

    return FALSE;
  }
  else {
    db_delete('eve_api_character_info_corporation_history')->condition('characterID', (int) $character_id, '=')->execute();

    foreach ($character_info as $type => $data) {
      switch ((string) $type) {
        case 'employmentHistory':
          foreach ($data as $historydata) {
            db_merge('eve_api_character_info_corporation_history')->key(array(
              'characterID' => (int) $character_id,
              'recordID' => (int) $historydata['recordID'],
            ))->fields(array(
              'characterID' => (int) $character_id,
              'recordID' => (int) $historydata['recordID'],
              'corporationID' => (int) $historydata['corporationID'],
              'startDate' => (string) $historydata['startDate'],
            ))->execute();
          }
          break;

        case 'info':
          $query_data = db_merge('eve_api_character_info')->key(array(
            'characterID' => (int) $character_id,
          ));

          $fields = array(
            'characterID' => (int) $character_id,
            'errorID' => (int) $error,
          );

          $ignored = array(
            'characterID',
          );

          foreach ($data as $infoname => $infodata) {
            if (in_array($infoname, $ignored)) {
              continue;
            }

            if (empty($infodata)) {
              continue;
            }

            $fields += array(
              (string) $infoname => (string) $infodata,
            );
          }

          $query_data->fields($fields);
          $query_data->execute();
          break;

        default:
          break;
      }
    }
  }

  return TRUE;
}

/**
 * Cron that imports the skills being trained of a single character.
 *
 * @param array $item
 *   The characterID and API Key and vCode.
 */
function eve_api_cron_import_skill_in_training($item) {
  $character_id = (int) $item['characterID'];
  $query = (array) $item['query'];

  $training_query = $query + array('characterID' => (int) $character_id);
  $training_info = eve_api_get_skill_in_training_api($training_query);
  $error = 0;

  if (isset($training_info['errorCode'])) {
    $error = (int) $training_info['errorCode'];
  }

  if (isset($training_info['error'])) {
    $error = (int) $training_info['error'];

    db_merge('eve_api_skill_training')->key(array(
      'characterID' => (int) $character_id,
    ))->fields(array(
      'characterID' => (int) $character_id,
      'errorID' => (int) $error,
    ))->execute();

    return FALSE;
  }
  else {
    foreach ($training_info as $type => $data) {
      switch ((string) $type) {
        case 'info':
          $query_data = db_merge('eve_api_skill_training')->key(array(
            'characterID' => (int) $character_id,
          ));

          $fields = array(
            'characterID' => (int) $character_id,
            'errorID' => (int) $error,
          );

          $ignored = array(
            'currentTQTime',
          );

          foreach ($data as $infoname => $infodata) {
            if (in_array($infoname, $ignored)) {
              continue;
            }

            if (empty($infodata)) {
              continue;
            }

            $fields += array(
              (string) $infoname => (string) $infodata,
            );
          }

          $query_data->fields($fields);
          $query_data->execute();
          break;

        default:
          break;
      }
    }
  }

  return TRUE;
}

/**
 * Cron that imports the wallet journal of a single character.
 *
 * @param array $item
 *   The characterID and API Key and vCode.
 */
function eve_api_cron_import_wallet_journal($item) {
  $character_id = (int) $item['characterID'];
  $query = (array) $item['query'];

  $row_count = '100';
  $from_id = NULL;

  $wallet_query = $query + array(
    'characterID' => (int) $character_id,
    'rowCount' => $row_count,
  );

  $counter = 100;

  $result = db_query('SELECT MAX(refID) AS maxRefID FROM {eve_api_wallet_journal} WHERE characterID = :characterID', array(
    ':characterID' => (int) $character_id,
  ));

  if (!$result->rowCount()) {
    $max_ref_id = 0;
  }
  else {
    $row = $result->fetchAssoc();
    $max_ref_id = isset($row['maxRefID']) ? (int) $row['maxRefID'] : 0;
  }

  do {
    $wallet_info = eve_api_get_wallet_journal_api($wallet_query);

    if (isset($wallet_info['error'])) {
      return FALSE;
    }
    else {
      $max_ref_id_found = FALSE;
      $count = 0;

      foreach ($wallet_info as $type => $data) {
        switch ((string) $type) {
          case 'wallet_journal':
            foreach ($data as $walletdata) {
              if ($max_ref_id >= (float) $walletdata['refID']) {
                $max_ref_id_found = TRUE;
                continue;
              }

              if (!isset($from_id)) {
                $from_id = (float) $walletdata['refID'];
              }

              if ($from_id >= (float) $walletdata['refID']) {
                $from_id = (float) $walletdata['refID'];
              }

              $query_data = db_merge('eve_api_wallet_journal')->key(array(
                'refID' => (int) $walletdata['refID'],
                'characterID' => (int) $character_id,
              ));

              $fields = array(
                'characterID' => (int) $character_id,
              );

              foreach ($walletdata as $infoname => $infodata) {
                if (empty($infodata)) {
                  continue;
                }

                $fields += array(
                  (string) $infoname => (string) $infodata,
                );
              }

              $query_data->fields($fields);
              $query_data->execute();

              $count++;
            }
            break;

          default:
            break;
        }
      }

      if ($max_ref_id_found || !isset($from_id) || $count != $row_count) {
        break;
      }

      $wallet_query = array(
        'fromID' => $from_id,
      ) + $wallet_query;
    }
  } while ($counter--);

  return TRUE;
}

/**
 * Cron that imports the contracts of a single character.
 *
 * @param array $item
 *   The characterID and API Key and vCode.
 */
function eve_api_cron_import_contracts($item) {
  $character_id = (int) $item['characterID'];
  $query = (array) $item['query'];

  $contract_query = $query + array(
    'characterID' => (int) $character_id,
  );

  $contract_info = eve_api_get_contracts_api($contract_query);

  if (isset($contract_info['error'])) {
    return FALSE;
  }
  else {
    $result = db_query('SELECT MAX(contractID) AS maxContractID FROM {eve_api_character_contracts} WHERE characterID = :characterID', array(
      ':characterID' => (int) $character_id,
    ));

    if (!$result->rowCount()) {
      $max_contract_id = 0;
    }
    else {
      $row = $result->fetchAssoc();
      $max_contract_id = isset($row['maxContractID']) ? (int) $row['maxContractID'] : 0;
    }

    foreach ($contract_info as $type => $data) {
      switch ((string) $type) {
        case 'contracts':
          foreach ($data as $contractdata) {
            if ($max_contract_id >= (int) $contractdata['contractID']) {
              continue;
            }

            $query_data = db_merge('eve_api_character_contracts')->key(array(
              'contractID' => (int) $contractdata['contractID'],
            ));

            $fields = array(
              'characterID' => (int) $character_id,
            );

            foreach ($contractdata as $infoname => $infodata) {
              if (empty($infodata)) {
                continue;
              }

              $fields += array(
                (string) $infoname => (string) $infodata,
              );
            }

            $query_data->fields($fields);
            $query_data->execute();
          }
          break;

        default:
          break;
      }
    }
  }

  return TRUE;
}

/**
 * Cron that imports the mail of a single character.
 *
 * @param array $item
 *   The characterID and API Key and vCode.
 */
function eve_api_cron_import_mail($item) {
  $character_id = (int) $item['characterID'];
  $query = (array) $item['query'];

  $mail_query = $query + array(
    'characterID' => (int) $character_id,
  );

  $counter = 100;

  $result = db_query('SELECT MAX(messageID) AS maxMessageID FROM {eve_api_mail_bodies} WHERE characterID = :characterID', array(
    ':characterID' => (int) $character_id,
  ));

  if (!$result->rowCount()) {
    $max_message_id = 0;
  }
  else {
    $row = $result->fetchAssoc();
    $max_message_id = isset($row['maxMessageID']) ? (int) $row['maxMessageID'] : 0;
  }

  do {
    $mail_info = eve_api_get_mail_messages_api($mail_query);

    if (isset($mail_info['error'])) {
      return FALSE;
    }
    else {
      $max_message_id_found = FALSE;
      $message_ids = array();

      foreach ($mail_info as $type => $data) {
        switch ((string) $type) {
          case 'mail':
            foreach ($data as $maildata) {
              if ($max_message_id >= (int) $maildata['messageID']) {
                $max_message_id_found = TRUE;
                continue;
              }

              $message_ids[] = (int) $maildata['messageID'];

              $query_data = db_merge('eve_api_mail_messages')->key(array(
                'messageID' => (int) $maildata['messageID'],
              ));

              $fields = array(
                'characterID' => (int) $character_id,
              );

              foreach ($maildata as $infoname => $infodata) {
                if (empty($infodata)) {
                  continue;
                }

                $fields += array(
                  (string) $infoname => (string) $infodata,
                );
              }

              $query_data->fields($fields);
              $query_data->execute();
            }
            break;

          default:
            break;
        }
      }

      if (empty($message_ids)) {
        break;
      }

      $body_query = array(
        'ids' => implode(',', $message_ids),
      ) + $mail_query;

      $body_info = eve_api_get_mail_bodies_api($body_query);

      if (isset($body_info['error'])) {
        return FALSE;
      }
      else {
        foreach ($body_info as $type => $data) {
          switch ((string) $type) {
            case 'mail_body':
              foreach ($data as $bodydata) {
                $query_data = db_merge('eve_api_mail_bodies')->key(array(
                  'messageID' => (int) $bodydata['messageID'],
                ));

                $fields = array(
                  'characterID' => (int) $character_id,
                );

                foreach ($bodydata as $infoname => $infodata) {
                  if (empty($infodata)) {
                    continue;
                  }

                  $fields += array(
                    (string) $infoname => (string) $infodata,
                  );
                }

                $query_data->fields($fields);
                $query_data->execute();
              }
              break;

            default:
              break;
          }
        }
      }

      if ($max_message_id_found) {
        break;
      }
    }
  } while ($counter--);

  return TRUE;
}
