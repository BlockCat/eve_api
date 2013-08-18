<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

/**
 * Implements hook_cron().
 */
function eve_api_api_alliance_fetch_cron() {
  $interval = variable_get('eve_api_api_alliance_fetch_cron_time', 1440 * 60);

  if (time() >= variable_get('eve_api_api_alliance_fetch_cron_next', 0) && date('G') == 2) {
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_fetch');
    $queue->createItem(1);

    variable_set('eve_api_api_alliance_fetch_cron_next', time() + $interval);
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_api_alliance_sync_cron() {
  $interval = variable_get('eve_api_api_alliance_sync_cron_time', 1440 * 60);

  if (time() >= variable_get('eve_api_api_alliance_sync_cron_next', 0) && date('G') == 3) {
    $queue = DrupalQueue::get('eve_api_cron_api_alliance_sync');
    $queue->createItem(1);

    variable_set('eve_api_api_alliance_sync_cron_next', time() + $interval);
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_api_mask_cron() {
  $interval = variable_get('eve_api_api_mask_cron_time', 1440 * 60);

  if (time() >= variable_get('eve_api_api_mask_cron_next', 0) && date('G') == 1) {
    $queue = DrupalQueue::get('eve_api_cron_api_mask_sync');
    $queue->createItem(1);

    variable_set('eve_api_api_mask_cron_next', time() + $interval);
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_api_users_cron() {
  if (variable_get('eve_api_enable', FALSE)) {
    $interval = variable_get('eve_api_api_users_cron_time', 60 * 60);

    if (time() >= variable_get('eve_api_api_users_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_cron_api_user_sync');

      $result = db_query('SELECT DISTINCT uid FROM {eve_api_characters}');

      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $item) {
          $queue->createItem($item->uid);
        }
      }

      variable_set('eve_api_api_users_cron_next', time() + $interval);
    }
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_teamspeak_role_cron() {
  if (variable_get('eve_api_ts3_enable', FALSE) && variable_get('eve_api_enable', FALSE)) {
    $interval = variable_get('eve_api_teamspeak_role_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_teamspeak_role_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_cron_teamspeak_role_sync');
      $queue->createItem(1);

      variable_set('eve_api_teamspeak_role_cron_next', time() + $interval);
    }
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_teamspeak_user_cron() {
  if (variable_get('eve_api_ts3_enable', FALSE) && variable_get('eve_api_enable', FALSE)) {
    $interval = variable_get('eve_api_teamspeak_user_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_teamspeak_user_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_cron_teamspeak_user_sync');
      $result = db_query('SELECT uid, teamspeakDBID, status FROM {users} WHERE characterID != 0 AND teamspeakUID != 0');

      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $item) {
          $queue->createItem(array(
            'uid' => $item->uid,
            'teamspeakDBID' => $item->teamspeakDBID,
            'status' => $item->status,
            'runs' => 1,
          ));
        }
      }

      variable_set('eve_api_teamspeak_user_cron_next', time() + $interval);
    }
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_jabber_role_cron() {
  if (variable_get('eve_api_jabber_enable', FALSE) && variable_get('eve_api_enable', FALSE)) {
    $interval = variable_get('eve_api_jabber_role_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_jabber_role_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_cron_jabber_role_sync');
      $queue->createItem(1);

      variable_set('eve_api_jabber_role_cron_next', time() + $interval);
    }
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_jabber_user_cron() {
  if (variable_get('eve_api_jabber_enable', FALSE) && variable_get('eve_api_enable', FALSE)) {
    $interval = variable_get('eve_api_jabber_user_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_jabber_user_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_cron_jabber_user_sync');
      $result = db_query('SELECT uid, name, status FROM {users} WHERE characterID != 0');

      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $item) {
          $queue->createItem(array(
            'uid' => $item->uid,
            'name' => $item->name,
            'status' => $item->status,
            'runs' => 1,
          ));
        }
      }

      variable_set('eve_api_jabber_user_cron_next', time() + $interval);
    }
  }
}

/**
 * Implements hook_cron().
 */
function eve_api_teamspeak_name_cron() {
  if (variable_get('eve_api_ts3_enable', FALSE) && variable_get('eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('eve_api_cron_teamspeak_user_check');
    $queue->createItem(1);
  }
}

/**
 * Update user permissions in Teamspeak based on roles.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_cron_teamspeak_user_sync($item) {
  if ($item['runs'] == 11) {
    watchdog('eve_api', 'Cron Update Teamspeak User Permissions task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $roles = user_roles();
    $valid_roles = $user_roles = array();

    $result = db_query('SELECT corporationName, corporationTicker FROM {eve_api_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationName] = $row->corporationTicker;
      }
    }

    foreach ($roles as $rid => $rname) {
      if ($rid == 1 || $rid == 2) {
        continue;
      }

      foreach ($corp_info as $key => $value) {
        if ($key == $rname) {
          $rname = $value;
        }
      }

      $valid_roles[$rname] = $rid;
    }

    $uid = (int) $item['uid'];
    $teamspeak_dbid = (int) $item['teamspeakDBID'];
    $ts3 = eve_api_teamspeak_connection();

    $result = db_query('SELECT r.rid, r.name FROM {users_roles} ur INNER JOIN {role} r ON (r.rid = ur.rid) WHERE ur.uid = :uid', array(
      ':uid' => $uid,
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $rname = $row->name;

        foreach ($corp_info as $key => $value) {
          if ($key == $rname) {
            $rname = $value;
          }
        }

        $user_roles[$rname] = $row->rid;
        $teamspeakgid = eve_api_teamspeak_group_get_id($ts3, $rname);
        eve_api_teamspeak_user_add_group($ts3, $teamspeakgid, $teamspeak_dbid);
      }
    }

    $roles = eve_api_teamspeak_user_get_groups($ts3, $teamspeak_dbid);

    foreach ($roles as $rid => $rname) {
      if (!isset($user_roles[(string) $rname['name']]) && (string) $rname['name'] != 'Guest' && isset($valid_roles[(string) $rname['name']])) {
        eve_api_teamspeak_user_remove_group($ts3, $rid, $teamspeak_dbid);
      }
    }
  }
  catch (EveApiException $e) {
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('eve_api_cron_teamspeak_user_sync');
    $queue->createItem($item);

    return FALSE;
  }

  return TRUE;
}

/**
 * Check each user on Teamspeak to ensure the Unique ID matches the character.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_cron_teamspeak_user_check($item) {
  if ($item == 11) {
    watchdog('eve_api', 'Cron Check Teamspeak Names task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $ts3 = eve_api_teamspeak_connection();
    $teamspeak_user_list = eve_api_teamspeak_user_list($ts3);

    foreach ($teamspeak_user_list as $teamspeak_user) {
      if ($teamspeak_user->client_type == 1) {
        continue;
      }

      if (in_array(variable_get('eve_api_ts3_bypass_group', 0), explode(',', (string) $teamspeak_user->client_servergroups))) {
        continue;
      }

      $teamspeak_uid = (string) $teamspeak_user->client_unique_identifier;
      $teamspeaknickname = (string) $teamspeak_user->client_nickname;

      $result = db_query('SELECT c.characterName FROM {eve_api_characters} c INNER JOIN {users} u ON (c.characterName = u.name) WHERE c.deleted = 0 AND u.teamspeakUID = :teamspeakUID', array(
        ':teamspeakUID' => $teamspeak_uid,
      ));

      if (!$result->rowCount()) {
        if (variable_get('eve_api_ts3_cron_reg_message_enable', FALSE)) {
          $teamspeak_user->message('[color=red]' . t('Your Teamspeak ID is not registered on our server.') . '[/color]');

          if (variable_get('eve_api_ts3_cron_reg_kick_enable', FALSE) || variable_get('eve_api_ts3_cron_reg_ban_enable', FALSE)) {
            $teamspeak_user->message('[color=red]' . t('If you do not register within @time minutes', array('@time' => variable_get('eve_api_ts3_cron_reg_time', 10))) . '[/color]');
            $teamspeak_user->message('[color=red]' . t('you will be removed from the server.'));
          }

          $teamspeak_user->message('[color=red]' . t('To register, go to [url]@homepage[/url]', array('@homepage' => url('user/register', array('absolute' => TRUE)))) . '[/color]');

          $message = variable_get('eve_api_ts3_cron_reg_message', '');

          if (!empty($message)) {
            $teamspeak_user->message('[color=red]' . $message . '[/color]');
          }
        }

        if (variable_get('eve_api_ts3_cron_reg_poke_enable', FALSE)) {
          $message = t('Please register');

          if (variable_get('eve_api_ts3_cron_reg_kick_enable', FALSE) || variable_get('eve_api_ts3_cron_reg_ban_enable', FALSE)) {
            $message .= t(' within @time minutes or you will be removed from the server', array('@time' => variable_get('eve_api_ts3_cron_reg_time', 10)));
          }

          $message .= '!';

          $teamspeak_user->poke($message);
          $teamspeak_user->poke(t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));

          $message = variable_get('eve_api_ts3_cron_reg_poke', '');

          if (!empty($message)) {
            $teamspeak_user->poke($message);
          }
        }

        if (variable_get('eve_api_ts3_cron_reg_kick_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_ts3_cron_reg_time', 10)))) {
            $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));
            eve_api_teamspeak_user_delete($ts3, $teamspeak_user->client_database_id);
          }
        }

        if (variable_get('eve_api_ts3_cron_reg_ban_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_ts3_cron_reg_time', 10)))) {
            $teamspeak_user->ban((60 * variable_get('eve_api_ts3_cron_reg_ban_time', 10)), t('You should have registered at @homepage  You are banned for @time minutes.', array(
              '@homepage' => url('user/register', array('absolute' => TRUE)),
              '@time' => (variable_get('eve_api_ts3_cron_reg_ban_time', 10)),
            )));
            eve_api_teamspeak_user_delete($ts3, $teamspeak_user->client_database_id);
          }
        }

        continue;
      }

      $row = $result->fetchAssoc();

      $expected_teamspeaknickname = eve_api_teamspeak_format_name($row['characterName']);

      if ($teamspeaknickname != $expected_teamspeaknickname) {
        if (variable_get('eve_api_ts3_cron_user_message_enable', FALSE)) {
          $teamspeak_user->message('[color=red]' . t('Your nickname is incorrect.') . '[/color]');

          if (variable_get('eve_api_ts3_cron_user_kick_enable', FALSE) || variable_get('eve_api_ts3_cron_user_ban_enable', FALSE)) {
            $teamspeak_user->message('[color=red]' . t('If you do not fix your nickname within @time minutes', array('@time' => variable_get('eve_api_ts3_cron_user_time', 10))) . '[/color]');
            $teamspeak_user->message('[color=red]' . t('you will be removed from the server.'));
          }

          $teamspeak_user->message('[color=red]' . t('Nickname expected: @nickname', array('@nickname' => $expected_teamspeaknickname)) . '[/color]');

          $message = eve_api_clean_input(variable_get('eve_api_ts3_cron_user_message', ''));

          if (!empty($message)) {
            $teamspeak_user->message('[color=red]' . $message . '[/color]');
          }
        }

        if (variable_get('eve_api_ts3_cron_user_poke_enable', FALSE)) {
          $message = t('Your nickname is incorrect');

          if (variable_get('eve_api_ts3_cron_user_kick_enable', FALSE) || variable_get('eve_api_ts3_cron_user_ban_enable', FALSE)) {
            $message .= t(', within @time minutes or you will be removed from the server', array('@time' => variable_get('eve_api_ts3_cron_user_time', 10)));
          }

          $message .= '!';

          $teamspeak_user->poke($message);
          $teamspeak_user->poke(t('Nickname expected: @nickname', array('@nickname' => $expected_teamspeaknickname)));

          $message = eve_api_clean_input(variable_get('eve_api_ts3_cron_user_poke', ''));

          if (!empty($message)) {
            $teamspeak_user->poke($message);
          }
        }

        if (variable_get('eve_api_ts3_cron_user_kick_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_ts3_cron_user_time', 10)))) {
            $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));
          }
        }

        if (variable_get('eve_api_ts3_cron_user_ban_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_ts3_cron_user_time', 10)))) {
            $teamspeak_user->ban((60 * variable_get('eve_api_ts3_cron_user_ban_time', 10)), t('You should have fixed your nickname, you are banned for @time minutes.', array(
              '@time' => (variable_get('eve_api_ts3_cron_user_ban_time', 10)),
            )));
          }
        }
      }
    }
  }
  catch (EveApiException $e) {
    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_cron_teamspeak_user_check');
    $queue->createItem($item);

    return FALSE;
  }

  return TRUE;
}

/**
 * Sync Teamspeak Permissions with Drupal roles.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_cron_teamspeak_role_sync($item) {
  if ($item == 11) {
    watchdog('eve_api', 'Cron Sync Teamspeak Roles task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $roles = user_roles();
    $ts3 = eve_api_teamspeak_connection();
    $corp_info = array();

    $result = db_query('SELECT corporationName, corporationTicker FROM {eve_api_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationName] = $row->corporationTicker;
      }
    }

    foreach ($roles as $rid => $rname) {
      if ($rid == 1 || $rid == 2) {
        continue;
      }

      foreach ($corp_info as $key => $value) {
        if ($key == $rname) {
          $rname = $value;
        }
      }

      if (eve_api_teamspeak_group_get_id($ts3, $rname)) {
        continue;
      }

      eve_api_teamspeak_group_create($ts3, $rname);
    }
  }
  catch (EveApiException $e) {
    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_cron_teamspeak_role_sync');
    $queue->createItem($item);

    return FALSE;
  }

  return TRUE;
}

/**
 * Update user permissions in Jabber based on roles.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_cron_jabber_user_sync($item) {
  if ($item['runs'] == 11) {
    watchdog('eve_api', 'Cron Update Jabber User Roles task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $user_roles = array();

    $result = db_query('SELECT corporationName, corporationTicker FROM {eve_api_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationName] = $row->corporationTicker;
      }
    }

    $uid = (int) $item['uid'];
    $character_name = (string) $item['name'];

    $result = db_query('SELECT r.rid, r.name FROM {users_roles} ur INNER JOIN {role} r ON (r.rid = ur.rid) WHERE ur.uid = :uid', array(
      ':uid' => $uid,
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($row->rid == variable_get('eve_api_unverified_role', 2)) {
          $result = eve_api_jabber_url_query('delete', $character_name);
          return TRUE;
        }

        $rname = $row->name;

        foreach ($corp_info as $key => $value) {
          if ($key == $rname) {
            $rname = $value;
          }
        }

        $user_roles[] = eve_api_jabber_format_name($rname);
      }

      $query = array(
        'groups' => implode(",", $user_roles),
      );

      $result = eve_api_jabber_url_query('update', $character_name, $query);
    }
    else {
      $result = eve_api_jabber_url_query('delete', $character_name);
    }
  }
  catch (EveApiException $e) {
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('eve_api_cron_jabber_user_sync');
    $queue->createItem($item);

    return FALSE;
  }

  return TRUE;
}

/**
 * Sync Jabber Permissions with Drupal roles.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_cron_jabber_role_sync($item) {
  if ($item == 11) {
    watchdog('eve_api', 'Cron Sync Jabber Roles task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $roles = user_roles();
    $corp_info = array();

    $result = db_query('SELECT corporationName, corporationTicker FROM {eve_api_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationName] = $row->corporationTicker;
      }
    }

    foreach ($roles as $rid => $rname) {
      if ($rid == 1 || $rid == 2) {
        continue;
      }

      $description = $rname;

      foreach ($corp_info as $key => $value) {
        if ($key == $rname) {
          $rname = $value;
        }
      }

      eve_api_jabber_group_add($rname, $description);
    }
  }
  catch (EveApiException $e) {
    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_cron_jabber_role_sync');
    $queue->createItem($item);

    return FALSE;
  }

  return TRUE;
}

/**
 * Verify each characters API.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_cron_api_user_sync($item) {
  $uid = (int) $item;
  $all_characters = array();

  $result = db_query('SELECT name, teamspeakDBID FROM {users} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));
  $account = $result->fetchAssoc();

  $result = db_query('SELECT apiID, keyID, vCode FROM {eve_api_api_keys} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));

  $updated = date('Y-m-d H:i:s', time());

  $character_found = FALSE;

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $error = 0;

      $query = array(
        'keyID' => $row->keyID,
        'vCode' => $row->vCode,
      );

      $characters = eve_api_get_character_api($query);

      if (isset($characters['error']) && $characters['error'] != 403) {
        db_merge('eve_api_api_keys')->key(array('apiID' => (int) $row->apiID))->fields(array(
          'apiID' => (int) $row->apiID,
          'errorID' => (int) $characters['error'],
          'updated' => (string) $updated,
        ))->execute();

        db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error']))->condition('apiID', (int) $row->apiID, '=')->execute();

        continue;
      }
      elseif (isset($characters['error']) && $characters['error'] == 403) {
        db_merge('eve_api_api_keys')->key(array('apiID' => (int) $row->apiID))->fields(array(
          'apiID' => (int) $row->apiID,
          'errorID' => (int) $characters['error'],
          'updated' => (string) $updated,
        ))->execute();

        db_update('eve_api_characters')->fields(array('errorID' => (int) $characters['error'], 'deleted' => 1))->condition('apiID', (int) $row->apiID, '=')->execute();

        continue;
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

      if ($result->rowCount()) {
        if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
          $error = 1;
        }
      }
      else {
        if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
          $error = 1;
        }
      }

      db_merge('eve_api_api_keys')->key(array('apiID' => (int) $row->apiID))->fields(array(
        'apiID' => (int) $row->apiID,
        'errorID' => (int) $error,
        'updated' => (string) $updated,
      ))->execute();

      db_update('eve_api_characters')->fields(array('errorID' => (int) $error, 'deleted' => 1))->condition('apiID', (int) $row->apiID, '=')->execute();

      foreach ($characters['characters'] as $character) {
        if ($account['name'] == (string) $character['characterName']) {
          $character_found = TRUE;
        }

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
      }

      $all_characters = array_replace_recursive($characters, $all_characters);
    }
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

    $default_role = user_role_load(variable_get('eve_api_unverified_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $default_role->rid);

    try {
      $ts3 = eve_api_teamspeak_connection();
      eve_api_teamspeak_user_delete($ts3, $account['teamspeakDBID']);
    }
    catch (EveApiException $e) {
      // Log Error ?
    }

    try {
      eve_api_jabber_connect();

      $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
        ':username' => (string) eve_api_jabber_format_name($account['name']),
      ));

      if ($result->rowCount()) {
        eve_api_jabber_url_query('delete', $account['name']);
      }

      eve_api_jabber_disconnect();
    }
    catch (EveApiException $e) {
      // Log Error ?
    }

    db_update('users')->fields(array(
      'teamspeakUID' => 0,
      'teamspeakDBID' => 0,
      'characterID' => 0,
      ))->condition('uid', $uid, '=')->execute();
  }
  else {
    $character_data = eve_api_get_character_info($account['name']);

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

      $blue_role = user_role_load(variable_get('eve_api_blue_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', $blue_role->rid);
    }
  }

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
  if ($item == 11) {
    watchdog('eve_api', 'Cron Standings task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
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
  if ($item == 11) {
    watchdog('eve_api', 'Cron Import Alliance API task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
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
  if ($item == 11) {
    watchdog('eve_api', 'Cron Access Mask task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
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
