<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

/**
 * Implements hook_cron().
 */
function eve_api_teamspeak_cron() {
  if (variable_get('eve_api_enable', FALSE) && variable_get('eve_api_teamspeak_enable', FALSE)) {
    $interval = variable_get('eve_api_teamspeak_role_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_teamspeak_role_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_teamspeak_cron_role_sync');
      $queue->createItem(1);

      variable_set('eve_api_teamspeak_role_cron_next', time() + $interval);
    }
  }

  if (variable_get('eve_api_enable', FALSE) && variable_get('eve_api_teamspeak_enable', FALSE)) {
    $interval = variable_get('eve_api_teamspeak_user_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_teamspeak_user_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_teamspeak_cron_user_sync');
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

  if (variable_get('eve_api_enable', FALSE) && variable_get('eve_api_teamspeak_enable', FALSE)) {
    $queue = DrupalQueue::get('eve_api_teamspeak_cron_user_check');
    $queue->createItem(1);
  }
}

/**
 * Implements hook_cron_queue_info().
 *
 * Set up ten queues that all send items to the same callback function.
 */
function eve_api_teamspeak_cron_queue_info() {
  $queue = array();
  $queue['eve_api_teamspeak_cron_user_sync'] = array(
    'worker callback' => 'eve_api_teamspeak_cron_user_sync',
    'time' => 15,
  );
  $queue['eve_api_teamspeak_cron_user_check'] = array(
    'worker callback' => 'eve_api_teamspeak_cron_user_check',
    'time' => 15,
  );
  $queue['eve_api_teamspeak_cron_role_sync'] = array(
    'worker callback' => 'eve_api_teamspeak_cron_role_sync',
    'time' => 15,
  );
  return $queue;
}

/**
 * Update user permissions in Teamspeak based on roles.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_teamspeak_cron_user_sync($item) {
  if ($item['runs'] == 11) {
    watchdog('eve_api_teamspeak', 'Cron Update Teamspeak User Permissions task has failed 10 times in a row, task was removed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
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
    watchdog('eve_api_teamspeak', 'Cron Update Teamspeak User Permissions task has failed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('eve_api_teamspeak_cron_user_sync');
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
function eve_api_teamspeak_cron_user_check($item) {
  if ($item == 11) {
    watchdog('eve_api_teamspeak', 'Cron Check Teamspeak Names task has failed 10 times in a row, task was removed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  try {
    $ts3 = eve_api_teamspeak_connection();
    $teamspeak_user_list = eve_api_teamspeak_user_list($ts3);

    foreach ($teamspeak_user_list as $teamspeak_user) {
      if ($teamspeak_user->client_type == 1) {
        continue;
      }

      if (in_array(variable_get('eve_api_teamspeak_bypass_group', 0), explode(',', (string) $teamspeak_user->client_servergroups))) {
        continue;
      }

      $teamspeak_uid = (string) $teamspeak_user->client_unique_identifier;
      $teamspeaknickname = (string) $teamspeak_user->client_nickname;

      $result = db_query('SELECT c.characterName FROM {eve_api_characters} c INNER JOIN {users} u ON (c.characterName = u.name) WHERE c.deleted = 0 AND u.teamspeakUID = :teamspeakUID', array(
        ':teamspeakUID' => $teamspeak_uid,
      ));

      if (!$result->rowCount()) {
        if (variable_get('eve_api_teamspeak_reg_message_enable', FALSE)) {
          $teamspeak_user->message('[color=red]' . t('Your Teamspeak ID is not registered on our server.') . '[/color]');

          if (variable_get('eve_api_teamspeak_reg_kick_enable', FALSE) || variable_get('eve_api_teamspeak_reg_ban_enable', FALSE)) {
            $teamspeak_user->message('[color=red]' . t('If you do not register within @time minutes', array('@time' => variable_get('eve_api_teamspeak_reg_time', 10))) . '[/color]');
            $teamspeak_user->message('[color=red]' . t('you will be removed from the server.'));
          }

          $teamspeak_user->message('[color=red]' . t('To register, go to [url]@homepage[/url]', array('@homepage' => url('user/register', array('absolute' => TRUE)))) . '[/color]');

          $message = variable_get('eve_api_teamspeak_reg_message', '');

          if (!empty($message)) {
            $teamspeak_user->message('[color=red]' . $message . '[/color]');
          }
        }

        if (variable_get('eve_api_teamspeak_reg_poke_enable', FALSE)) {
          $message = t('Please register');

          if (variable_get('eve_api_teamspeak_reg_kick_enable', FALSE) || variable_get('eve_api_teamspeak_reg_ban_enable', FALSE)) {
            $message .= t(' within @time minutes or you will be removed from the server', array('@time' => variable_get('eve_api_teamspeak_reg_time', 10)));
          }

          $message .= '!';

          $teamspeak_user->poke($message);
          $teamspeak_user->poke(t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));

          $message = variable_get('eve_api_teamspeak_reg_poke', '');

          if (!empty($message)) {
            $teamspeak_user->poke($message);
          }
        }

        if (variable_get('eve_api_teamspeak_reg_kick_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_teamspeak_reg_time', 10)))) {
            $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));
          }
        }

        if (variable_get('eve_api_teamspeak_reg_ban_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_teamspeak_reg_time', 10)))) {
            $teamspeak_user->ban((60 * variable_get('eve_api_teamspeak_reg_ban_time', 10)), t('You should have registered at @homepage  You are banned for @time minutes.', array(
              '@homepage' => url('user/register', array('absolute' => TRUE)),
              '@time' => (variable_get('eve_api_teamspeak_reg_ban_time', 10)),
            )));
          }
        }

        continue;
      }

      $row = $result->fetchAssoc();

      $expected_teamspeaknickname = eve_api_teamspeak_format_name($row['characterName']);

      if ($teamspeaknickname != $expected_teamspeaknickname) {
        if (variable_get('eve_api_teamspeak_user_message_enable', FALSE)) {
          $teamspeak_user->message('[color=red]' . t('Your nickname is incorrect.') . '[/color]');

          if (variable_get('eve_api_teamspeak_user_kick_enable', FALSE) || variable_get('eve_api_teamspeak_user_ban_enable', FALSE)) {
            $teamspeak_user->message('[color=red]' . t('If you do not fix your nickname within @time minutes', array('@time' => variable_get('eve_api_teamspeak_user_time', 10))) . '[/color]');
            $teamspeak_user->message('[color=red]' . t('you will be removed from the server.'));
          }

          $teamspeak_user->message('[color=red]' . t('Nickname expected: @nickname', array('@nickname' => $expected_teamspeaknickname)) . '[/color]');

          $message = eve_api_teamspeak_clean_input(variable_get('eve_api_teamspeak_user_message', ''));

          if (!empty($message)) {
            $teamspeak_user->message('[color=red]' . $message . '[/color]');
          }
        }

        if (variable_get('eve_api_teamspeak_user_poke_enable', FALSE)) {
          $message = t('Your nickname is incorrect');

          if (variable_get('eve_api_teamspeak_user_kick_enable', FALSE) || variable_get('eve_api_teamspeak_user_ban_enable', FALSE)) {
            $message .= t(', within @time minutes or you will be removed from the server', array('@time' => variable_get('eve_api_teamspeak_user_time', 10)));
          }

          $message .= '!';

          $teamspeak_user->poke($message);
          $teamspeak_user->poke(t('Nickname expected: @nickname', array('@nickname' => $expected_teamspeaknickname)));

          $message = eve_api_teamspeak_clean_input(variable_get('eve_api_teamspeak_user_poke', ''));

          if (!empty($message)) {
            $teamspeak_user->poke($message);
          }
        }

        if (variable_get('eve_api_teamspeak_user_kick_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_teamspeak_user_time', 10)))) {
            $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('To register, go to @homepage', array('@homepage' => url('user/register', array('absolute' => TRUE)))));
          }
        }

        if (variable_get('eve_api_teamspeak_user_ban_enable', FALSE)) {
          if (time() >= ($teamspeak_user->client_lastconnected + (60 * variable_get('eve_api_teamspeak_user_time', 10)))) {
            $teamspeak_user->ban((60 * variable_get('eve_api_teamspeak_user_ban_time', 10)), t('You should have fixed your nickname, you are banned for @time minutes.', array(
              '@time' => (variable_get('eve_api_teamspeak_user_ban_time', 10)),
            )));
          }
        }
      }
    }
  }
  catch (EveApiException $e) {
    watchdog('eve_api_teamspeak', 'Cron Check Teamspeak Names task has failed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_teamspeak_cron_user_check');
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
function eve_api_teamspeak_cron_role_sync($item) {
  if ($item == 11) {
    watchdog('eve_api_teamspeak', 'Cron Sync Teamspeak Roles task has failed 10 times in a row, task was removed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
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
    watchdog('eve_api_teamspeak', 'Cron Sync Teamspeak Roles task has failed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_teamspeak_cron_role_sync');
    $queue->createItem($item);

    return FALSE;
  }

  return TRUE;
}
