<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

module_load_include('inc', 'omni_eve_api', 'omni_eve_api.api');

/**
 * Implements hook_cron().
 */
function omni_eve_api_general_cron($force = FALSE) {
  if (variable_get('omni_eve_api_enable', FALSE) || $force) {
    $queue = DrupalQueue::get('omni_eve_api_cron_api_alliance_fetch');
    $queue->createItem(1);

    $queue = DrupalQueue::get('omni_eve_api_cron_api_alliance_sync');
    $queue->createItem(1);

    $queue = DrupalQueue::get('omni_eve_api_cron_api_mask_sync');
    $queue->createItem(1);
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_users_cron() {
  if (variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_api_user_sync');

    $result = db_query('SELECT DISTINCT uid FROM {oea_characters}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem($item->uid);
      }
    }
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_teamspeak_user_cron() {
  if (variable_get('omni_eve_api_ts3_enable', FALSE) && variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_role_sync');
    $queue->createItem(1);

    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user_sync');
    $result = db_query('SELECT uid, teamspeakdbid, status FROM {users} WHERE characterid != 0 AND teamspeakuid != 0');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem(array(
          'uid' => $item->uid,
          'teamspeakdbid' => $item->teamspeakdbid,
          'status' => $item->status,
          'runs' => 1,
        ));
      }
    }
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_jabber_user_cron() {
  if (variable_get('omni_eve_api_jabber_enable', FALSE) && variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_jabber_role_sync');
    $queue->createItem(1);

    $queue = DrupalQueue::get('omni_eve_api_cron_jabber_user_sync');
    $result = db_query('SELECT uid, name, status FROM {users} WHERE characterid != 0');

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
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_teamspeak_name_cron() {
  if (variable_get('omni_eve_api_ts3_enable', FALSE) && variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user_check');
    $queue->createItem(1);
  }
}

/**
 * Implements hook_cron_queue_info().
 *
 * Set up ten queues that all send items to the same callback function.
 */
function omni_eve_api_cron_queue_info() {
  $queue = array();
  $queue['omni_eve_api_cron_teamspeak_user_sync'] = array(
    'worker callback' => 'omni_eve_api_cron_teamspeak_user_sync',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_teamspeak_user_check'] = array(
    'worker callback' => 'omni_eve_api_cron_teamspeak_user_check',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_teamspeak_role_sync'] = array(
    'worker callback' => 'omni_eve_api_cron_teamspeak_role_sync',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_jabber_user_sync'] = array(
    'worker callback' => 'omni_eve_api_cron_jabber_user_sync',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_jabber_role_sync'] = array(
    'worker callback' => 'omni_eve_api_cron_jabber_role_sync',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_api_user_sync'] = array(
    'worker callback' => 'omni_eve_api_cron_api_user_sync',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_api_alliance_fetch'] = array(
    'worker callback' => 'omni_eve_api_cron_api_alliance_fetch',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_api_mask_sync'] = array(
    'worker callback' => 'omni_eve_api_cron_api_mask_sync',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_api_alliance_sync'] = array(
    'worker callback' => 'omni_eve_api_cron_api_alliance_sync',
    'time' => 60,
  );
  return $queue;
}

/**
 * Update user permissions in Teamspeak based on roles.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function omni_eve_api_cron_teamspeak_user_sync($item) {
  if ($item['runs'] == 11) {
    watchdog('eve api', 'Cron Update Teamspeak User Permissions task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $roles = user_roles();
    $valid_roles = $user_roles = array();

    $result = db_query('SELECT corporationname, corporationticker FROM {oea_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationname] = $row->corporationticker;
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
    $teamspeakdbid = (int) $item['teamspeakdbid'];
    $ts3 = omni_eve_api_teamspeak_connection();

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
        $teamspeakgid = omni_eve_api_teamspeak_group_get_id($ts3, $rname);
        omni_eve_api_teamspeak_user_add_group($ts3, $teamspeakgid, $teamspeakdbid);
      }
    }

    $roles = omni_eve_api_teamspeak_user_get_groups($ts3, $teamspeakdbid);

    foreach ($roles as $rid => $rname) {
      if (!isset($user_roles[(string) $rname['name']]) && (string) $rname['name'] != 'Guest' && isset($valid_roles[(string) $rname['name']])) {
        omni_eve_api_teamspeak_user_remove_group($ts3, $rid, $teamspeakdbid);
      }
    }
  }
  catch (OeaException $e) {
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user_sync');
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
function omni_eve_api_cron_teamspeak_user_check($item) {
  if ($item == 11) {
    watchdog('eve api', 'Cron Check Teamspeak Names task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_user_list = omni_eve_api_teamspeak_user_list($ts3);

    foreach ($teamspeak_user_list as $teamspeak_user) {
      if ($teamspeak_user->client_type == 1) {
        continue;
      }

      if (in_array(variable_get('omni_eve_api_ts3_bypass_group', 0), explode(',', (string) $teamspeak_user->client_servergroups))) {
        continue;
      }

      $teamspeakuid = (string) $teamspeak_user->client_unique_identifier;
      $teamspeaknickname = (string) $teamspeak_user->client_nickname;

      $result = db_query('SELECT c.charactername FROM {oea_characters} c INNER JOIN {users} u ON (c.charactername = u.name) WHERE c.deleted = 0 AND u.teamspeakuid = :teamspeakuid', array(
        ':teamspeakuid' => $teamspeakuid,
      ));

      if (!$result->rowCount()) {
        if (variable_get('omni_eve_api_ts3_cron_reg_message_enable', FALSE)) {
          $teamspeak_user->message('[color=red]' . t('Your Teamspeak ID is not registered on our server.') . '[/color]');

          if (variable_get('omni_eve_api_ts3_cron_reg_kick_enable', FALSE) || variable_get('omni_eve_api_ts3_cron_reg_ban_enable', FALSE)) {
            $teamspeak_user->message('[color=red]' . t('If you do not register within @time minutes', array('@time' => variable_get('omni_eve_api_ts3_cron_reg_time', 10))) . '[/color]');
            $teamspeak_user->message('[color=red]' . t('you will be removed from the server.'));
          }

          $teamspeak_user->message('[color=red]' . t('To register, go to [url]@homepage[/url]', array('@homepage' => url('user/register', array('absolute' => TRUE)))) . '[/color]');

          if (!empty(variable_get('omni_eve_api_ts3_cron_reg_message', ''))) {
            $teamspeak_user->message('[color=red]' . variable_get('omni_eve_api_ts3_cron_reg_message', '') . '[/color]');
          }
        }

        if (variable_get('omni_eve_api_ts3_cron_reg_poke_enable', FALSE)) {
          $message = t('Please register');

          if (variable_get('omni_eve_api_ts3_cron_reg_kick_enable', FALSE) || variable_get('omni_eve_api_ts3_cron_reg_ban_enable', FALSE)) {
            $message .= t(' within @time minutes or you will be removed from the server', array('@time' => variable_get('omni_eve_api_ts3_cron_reg_time', 10)));
          }

          $message .= '!';

          $teamspeak_user->poke($message);
          $teamspeak_user->poke(t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));

          if (!empty(variable_get('omni_eve_api_ts3_cron_reg_poke', ''))) {
            $teamspeak_user->poke(variable_get('omni_eve_api_ts3_cron_reg_poke', ''));
          }
        }

        if (variable_get('omni_eve_api_ts3_cron_reg_kick_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('omni_eve_api_ts3_cron_reg_time', 10)))) {
            $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));
            omni_eve_api_teamspeak_user_delete($ts3, $teamspeak_user->client_database_id);
          }          
        }

        if (variable_get('omni_eve_api_ts3_cron_reg_ban_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('omni_eve_api_ts3_cron_reg_time', 10)))) {
            $teamspeak_user->ban((60 * variable_get('omni_eve_api_ts3_cron_reg_ban_time', 10)), t('You should have registered at @homepage  You are banned for @time minutes.', array(
              '@homepage' => url('user/register', array('absolute' => TRUE)),
              '@time' => (variable_get('omni_eve_api_ts3_cron_reg_ban_time', 10)),
            )));
            omni_eve_api_teamspeak_user_delete($ts3, $teamspeak_user->client_database_id);
          } 
        }

        continue;
      }

      $row = $result->fetchAssoc();

      $expected_teamspeaknickname = omni_eve_api_teamspeak_format_name($row['charactername']);

      if ($teamspeaknickname != $expected_teamspeaknickname) {
        if (variable_get('omni_eve_api_ts3_cron_user_message_enable', FALSE)) {
          $teamspeak_user->message('[color=red]' . t('Your nickname is incorrect.') . '[/color]');

          if (variable_get('omni_eve_api_ts3_cron_user_kick_enable', FALSE) || variable_get('omni_eve_api_ts3_cron_user_ban_enable', FALSE)) {
            $teamspeak_user->message('[color=red]' . t('If you do not fix your nickname within @time minutes', array('@time' => variable_get('omni_eve_api_ts3_cron_user_time', 10))) . '[/color]');
            $teamspeak_user->message('[color=red]' . t('you will be removed from the server.'));
          }

          $teamspeak_user->message('[color=red]' . t('Nickname expected: @nickname', array('@nickname' => $expected_teamspeaknickname)) . '[/color]');

          if (!empty(variable_get('omni_eve_api_ts3_cron_user_message', ''))) {
            $teamspeak_user->message('[color=red]' . variable_get('omni_eve_api_ts3_cron_user_message', '') . '[/color]');
          }
        }

        if (variable_get('omni_eve_api_ts3_cron_user_poke_enable', FALSE)) {
          $message = t('Your nickname is incorrect');

          if (variable_get('omni_eve_api_ts3_cron_user_kick_enable', FALSE) || variable_get('omni_eve_api_ts3_cron_user_ban_enable', FALSE)) {
            $message .= t(', within @time minutes or you will be removed from the server', array('@time' => variable_get('omni_eve_api_ts3_cron_user_time', 10)));
          }

          $message .= '!';

          $teamspeak_user->poke($message);
          $teamspeak_user->poke(t('Nickname expected: @nickname', array('@nickname' => $expected_teamspeaknickname)));

          if (!empty(variable_get('omni_eve_api_ts3_cron_user_poke', ''))) {
            $teamspeak_user->poke(variable_get('omni_eve_api_ts3_cron_user_poke', ''));
          }
        }

        if (variable_get('omni_eve_api_ts3_cron_user_kick_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('omni_eve_api_ts3_cron_user_time', 10)))) {
            $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));
          }          
        }

        if (variable_get('omni_eve_api_ts3_cron_user_ban_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('omni_eve_api_ts3_cron_user_time', 10)))) {
            $teamspeak_user->ban((60 * variable_get('omni_eve_api_ts3_cron_user_ban_time', 10)), t('You should have fixed your nickname, you are banned for @time minutes.', array(
              '@time' => (variable_get('omni_eve_api_ts3_cron_user_ban_time', 10)),
            )));
          } 
        }
      }
    }
  }
  catch (OeaException $e) {
    $item = $item + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user_check');
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
function omni_eve_api_cron_teamspeak_role_sync($item) {
  if ($item == 11) {
    watchdog('eve api', 'Cron Sync Teamspeak Roles task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $roles = user_roles();
    $ts3 = omni_eve_api_teamspeak_connection();
    $corp_info = array();

    $result = db_query('SELECT corporationname, corporationticker FROM {oea_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationname] = $row->corporationticker;
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

      if (omni_eve_api_teamspeak_group_get_id($ts3, $rname)) {
        continue;
      }

      omni_eve_api_teamspeak_group_create($ts3, $rname);
    }
  }
  catch (OeaException $e) {
    $item = $item + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_role_sync');
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
function omni_eve_api_cron_jabber_user_sync($item) {
  if ($item['runs'] == 11) {
    watchdog('eve api', 'Cron Update Jabber User Roles task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $user_roles = array();

    $result = db_query('SELECT corporationname, corporationticker FROM {oea_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationname] = $row->corporationticker;
      }
    }

    $uid = (int) $item['uid'];
    $charactername = (string) $item['name'];

    $result = db_query('SELECT r.rid, r.name FROM {users_roles} ur INNER JOIN {role} r ON (r.rid = ur.rid) WHERE ur.uid = :uid', array(
      ':uid' => $uid,
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($row->rid == variable_get('omni_eve_api_unverified_role', 2)) {
          $result = omni_eve_api_jabber_url_query('delete', $charactername);
          return TRUE;
        }

        $rname = $row->name;

        foreach ($corp_info as $key => $value) {
          if ($key == $rname) {
            $rname = $value;
          }
        }

        $user_roles[] = omni_eve_api_jabber_format_name($rname);
      }

      $query = array(
        'groups' => implode(",", $user_roles),
      );

      $result = omni_eve_api_jabber_url_query('update', $charactername, $query);
    }
    else {
      $result = omni_eve_api_jabber_url_query('delete', $charactername);
    }
  }
  catch (OeaException $e) {
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_jabber_user_sync');
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
function omni_eve_api_cron_jabber_role_sync($item) {
  if ($item == 11) {
    watchdog('eve api', 'Cron Sync Jabber Roles task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  try {
    $roles = user_roles();
    $corp_info = array();

    $result = db_query('SELECT corporationname, corporationticker FROM {oea_alliance_corporations}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationname] = $row->corporationticker;
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

      omni_eve_api_jabber_group_add($rname, $description);
    }
  }
  catch (OeaException $e) {
    $item = $item + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_jabber_role_sync');
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
function omni_eve_api_cron_api_user_sync($item) {
  $uid = (int) $item;
  $all_characters = array();

  $result = db_query('SELECT name, teamspeakdbid FROM {users} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));
  $account = $result->fetchAssoc();

  $result = db_query('SELECT apiid, keyid, vcode FROM {oea_api_keys} WHERE uid = :uid', array(
    ':uid' => $uid,
  ));

  $updated = date('Y-m-d H:i:s', time());

  $character_found = FALSE;

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $error = 0;

      $query = array(
        'keyid' => $row->keyid,
        'vcode' => $row->vcode,
      );

      $characters = omni_eve_api_get_character_api($query);

      if (isset($characters['error']) && $characters['error'] != 403) {
        $new_api_key_id = db_merge('oea_api_keys')->key(array('apiid' => (int) $row->apiid))->fields(array(
          'apiid' => (int) $row->apiid,
          'errorid' => (int) $characters['error'],
          'updated' => (string) $updated,
        ))->execute();

        db_update('oea_characters')->fields(array('errorid' => (int) $characters['error']))->condition('apiid', (int) $row->apiid, '=')->execute();

        continue;
      }
      elseif (isset($characters['error']) && $characters['error'] == 403) {
        $new_api_key_id = db_merge('oea_api_keys')->key(array('apiid' => (int) $row->apiid))->fields(array(
          'apiid' => (int) $row->apiid,
          'errorid' => (int) $characters['error'],
          'updated' => (string) $updated,
        ))->execute();

        db_update('oea_characters')->fields(array('errorid' => (int) $characters['error'], 'deleted' => 1))->condition('apiid', (int) $row->apiid, '=')->execute();

        continue;
      }

      $whitelist_characters = array();

      if (!empty($characters)) {
        foreach ($characters['characters'] as $character) {
          $whitelist_characters[] = (int) $character['characterID'];
        }
      }

      $result = db_query('SELECT characterid FROM {oea_whitelist} WHERE deleted = 0 AND characterid IN (:characterids)', array(
        ':characterids' => $whitelist_characters,
      ));

      if ($result->rowCount()) {
        if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
          $error = 1;
        }
      }
      else {
        if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('omni_eve_api_access_mask', 268435455)) != variable_get('omni_eve_api_access_mask', 268435455)) {
          $error = 1;
        }
      }

      $new_api_key_id = db_merge('oea_api_keys')->key(array('apiid' => (int) $row->apiid))->fields(array(
        'apiid' => (int) $row->apiid,
        'errorid' => (int) $error,
        'updated' => (string) $updated,
      ))->execute();

      db_update('oea_characters')->fields(array('errorid' => (int) $error, 'deleted' => 1))->condition('apiid', (int) $row->apiid, '=')->execute();

      foreach ($characters['characters'] as $character) {
        if ($account['name'] == (string) $character['characterName']) {
          $character_found = TRUE;
        }

        db_merge('oea_characters')->key(array('characterid' => (int) $character['characterID']))->fields(array(
          'characterid' => (int) $character['characterID'],
          'uid' => (int) $uid,
          'apiid' => (int) $row->apiid,
          'charactername' => (string) $character['characterName'],
          'corporationid' => (int) $character['corporationID'],
          'corporationname' => (string) $character['corporationName'],
          'corporationticker' => (string) $character['corporationTicker'],
          'allianceid' => (int) $character['allianceID'],
          'alliancename' => (string) $character['allianceName'],
          'allianceticker' => (string) $character['allianceTicker'],
          'deleted' => 0,
          'errorid' => (int) $error,
        ))->execute();
      }

      $all_characters = array_replace_recursive($characters, $all_characters);
    }
  }

  if (!omni_eve_api_verify_blue($all_characters) || !$character_found) {
    $result = db_query('SELECT corporationid, corporationname FROM {oea_alliance_corporations} WHERE deleted = 0');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($corporation_role = user_role_load_by_name($row->corporationname)) {
          user_multiple_role_edit(array($uid), 'remove_role', (int) $corporation_role->rid);
        }
      }
    }

    $blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);

    $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);

    $default_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $default_role->rid);

    try {
      $ts3 = omni_eve_api_teamspeak_connection();
      omni_eve_api_teamspeak_user_delete($ts3, $account['teamspeakdbid']);
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    try {
      omni_eve_api_jabber_connect();

      $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
        ':username' => (string) omni_eve_api_jabber_format_name($account['name']),
      ));

      if ($result->rowCount()) {
        omni_eve_api_jabber_url_query('delete', $account['name']);
      }

      omni_eve_api_jabber_disconnect();
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    db_update('users')->fields(array(
      'teamspeakuid' => 0,
      'teamspeakdbid' => 0,
      'characterid' => 0,
      ))->condition('uid', $uid, '=')->execute();
  }
  else {
    $character_data = omni_eve_api_get_character_info($account['name']);

    $result = db_query('SELECT corporationid, corporationname FROM {oea_alliance_corporations} WHERE deleted = 0');

    if ($corporation_role = user_role_load_by_name($character_data['corporationName'])) {
      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $row) {
          if (($corporation_role_2 = user_role_load_by_name($row->corporationname)) && $row->corporationid != $character_data['corporationID']) {
            user_multiple_role_edit(array($uid), 'remove_role', $corporation_role_2->rid);
          }
        }
      }

      user_multiple_role_edit(array($uid), 'add_role', $corporation_role->rid);

      $default_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $default_role->rid);

      $blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);

      $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', $alliance_role->rid);
    }
    else {
      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $row) {
          if ($corporation_role = user_role_load_by_name($row->corporationname)) {
            user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);
          }
        }
      }

      $default_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $default_role->rid);

      $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);

      $blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2));
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
function omni_eve_api_cron_api_alliance_fetch($item = NULL) {
  if ($item == 11) {
    watchdog('eve api', 'Cron Standings task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  $query = array(
    'keyid' => variable_get('omni_eve_api_corp_keyid', ''),
    'vcode' => variable_get('omni_eve_api_corp_vcode', ''),
  );
  $xml = omni_eve_api_get_xml('ContactList', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>Failed getting Alliance Contact List API Data. (' . drupal_http_build_query($query) . ') Error: ' . $xml . '</pre>', NULL, WATCHDOG_WARNING);

    $item = $item + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_api_alliance_fetch');
    $queue->createItem($item);

    return FALSE;
  }

  db_update('oea_blue_standings')->fields(array('checked' => 1))->condition('manual', 0, '=')->execute();

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

  db_update('oea_blue_standings')->fields(array('deleted' => 1))->condition('checked', 1, '=')->condition('manual', 0, '=')->execute();

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
function omni_eve_api_cron_api_alliance_sync($item = NULL) {
  if ($item == 11) {
    watchdog('eve api', 'Cron Import Alliance API task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  $xml = omni_eve_api_get_xml('AllianceList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>Failed getting Alliance List API Data. (' . drupal_http_build_query($query) . ') Error: ' . $xml . '</pre>', NULL, WATCHDOG_WARNING);

    $item = $item + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_api_alliance_sync');
    $queue->createItem($item);

    return FALSE;
  }

  db_update('oea_alliance_corporations')->fields(array('deleted' => 1))->condition('manual', 0, '=')->execute();
  db_truncate('oea_alliance_info')->execute();

  foreach ($xml->result->rowset->children() as $rows_a) {
    $row_a = $rows_a->attributes();

    db_insert('oea_alliance_info')->fields(array(
      'allianceid' => (int) $row_a->allianceID,
      'allianceticker' => (string) $row_a->shortName,
      'alliancename' => (string) $row_a->name,
    ))->execute();

    if ((int) $row_a->allianceID == variable_get('omni_eve_api_allianceID', 0)) {
      foreach ($rows_a->rowset->children() as $rows_b) {
        $row_b = $rows_b->attributes();

        $query = array('corporationID' => (int) $row_b->corporationID);
        $corp_info = omni_eve_api_get_corporation_info($query);

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

        db_merge('oea_alliance_corporations')->key(array('corporationid' => (int) $row_b->corporationID))->fields(array(
          'corporationid' => (int) $row_b->corporationID,
          'rid' => (int) $role->rid,
          'corporationname' => (string) $corp_info['corporationName'],
          'corporationticker' => (string) $corp_info['corporationTicker'],
          'deleted' => 0,
          'manual' => 0,
        ))->execute();
      }
    }
  }

  $result = db_query('SELECT corporationid, corporationname FROM {oea_alliance_corporations} WHERE deleted = 1');

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      if (user_role_load_by_name($row->corporationname) == TRUE) {
        $user_role = new stdClass();
        $user_role->name = $row->corporationname;
        user_role_delete($user_role);
      }
    }
  }

  db_delete('oea_alliance_corporations')->condition('deleted', 1, '=')->execute();

  if (!variable_get('omni_eve_api_enable', FALSE)) {
    variable_set('omni_eve_api_enable', TRUE);
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
function omni_eve_api_cron_api_mask_sync($item = NULL) {
  if ($item == 11) {
    watchdog('eve api', 'Cron Access Mask task has failed 10 times in a row, task was removed.', NULL, WATCHDOG_EMERGENCY);
    return FALSE;
  }

  $xml = omni_eve_api_get_xml('callList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>Failed getting Access Mask API Data. (' . drupal_http_build_query($query) . ') Error: ' . $xml . '</pre>', NULL, WATCHDOG_WARNING);

    $item = $item + 1;
    $queue = DrupalQueue::get('omni_eve_api_cron_api_mask_sync');
    $queue->createItem($item);

    return FALSE;
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
        'accessmask' => (int) $row_b->accessMask,
        'type' => (string) $row_b->type,
        'name' => (string) $row_b->name,
        'description' => (string) $row_b->description,
      ))->execute();
    }
  }

  return TRUE;
}
