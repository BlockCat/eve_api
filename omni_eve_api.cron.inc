<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

/**
 * Implements hook_cronapi().
 */
function omni_eve_api_cronapi($op, $job = NULL) {
  $items = array();

  $items['omni_eve_api_general_cron'] = array(
    'description' => 'General cron, called once a day.',
    'rule' => '4 0 * * *',
  );

  $items['omni_eve_api_users_cron'] = array(
    'description' => 'Pull API for each user every hour.',
    'rule' => '0 * * * *',
  );

  $items['omni_eve_api_teamspeak_user_cron'] = array(
    'description' => 'Update user permissions in Teamspeak every 5 minutes.',
    'rule' => '*/5 * * * *',
  );

  $items['omni_eve_api_jabber_user_cron'] = array(
    'description' => 'Update user permissions in Jabber every 5 minutes.',
    'rule' => '*/5 * * * *',
  );

  $items['omni_eve_api_teamspeak_name_cron'] = array(
    'description' => 'Check usernames in Teamspeak every minute.',
    'rule' => '* * * * *',
  );

  return $items;
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_general_cron() {
  if (variable_get('omni_eve_api_enable_cron', TRUE) && variable_get('omni_eve_api_enable', FALSE)) {
    $item = NULL;

    // Run cron for standings, but have nothing to pass so passing a NULL
    // variable.
    $queue = DrupalQueue::get('omni_eve_api_cron_get_standings');
    $queue->createQueue();
    $queue->createItem($item);

    // Run cron for corporations in the alliance, but have nothing to pass so
    // passing a NULL variable.
    $queue = DrupalQueue::get('omni_eve_api_cron_get_alliance_corporations');
    $queue->createQueue();
    $queue->createItem($item);

    // Run cron to get latest access mask definitions.
    $queue = DrupalQueue::get('omni_eve_api_cron_get_access_mask');
    $queue->createQueue();
    $queue->createItem($item);
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_users_cron() {
  if (variable_get('omni_eve_api_enable_cron', TRUE) && variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
    $queue->createQueue();
    $result = db_select('users', 'u')->fields('u', array('uid'))->condition('status', 1, '=')->execute();

    foreach ($result->fetchAll() as $item) {
      $queue->createItem($item->uid);
    }
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_teamspeak_user_cron() {
  if (variable_get('omni_eve_api_enable_cron', TRUE) && variable_get('omni_eve_api_ts3_enable', FALSE) && variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_role');
    $queue->createQueue();
    $item = NULL;
    $queue->createItem($item);

    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user');
    $queue->createQueue();
    $result = db_select('users', 'u')->fields('u', array('uid', 'teamspeakdbid'))->condition('status', 1, '=')->condition('teamspeakuid', 0, '!=')->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem(array('uid' => $item->uid, 'teamspeakdbid' => $item->teamspeakdbid));
      }
    }
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_jabber_user_cron() {
  if (variable_get('omni_eve_api_enable_cron', TRUE) && variable_get('omni_eve_api_jabber_enable', FALSE) && variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_jabber_role');
    $queue->createQueue();
    $item = NULL;
    $queue->createItem($item);

    $queue = DrupalQueue::get('omni_eve_api_cron_jabber_user');
    $queue->createQueue();
    $result = db_select('users', 'u')->fields('u', array('uid', 'name'))->condition('status', 1, '=')->condition('characterid', 0, '!=')->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem(array('uid' => $item->uid, 'name' => $item->name));
      }
    }
  }
}

/**
 * Implements hook_cron().
 */
function omni_eve_api_teamspeak_name_cron() {
  if (variable_get('omni_eve_api_enable_cron', TRUE) && variable_get('omni_eve_api_ts3_enable', FALSE) && variable_get('omni_eve_api_enable', FALSE)) {
    $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_name');
    $queue->createQueue();
    $item = NULL;
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
  $queue['omni_eve_api_cron_teamspeak_user'] = array(
    'worker callback' => 'omni_eve_api_cron_teamspeak_user',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_teamspeak_name'] = array(
    'worker callback' => 'omni_eve_api_cron_teamspeak_name',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_teamspeak_role'] = array(
    'worker callback' => 'omni_eve_api_cron_teamspeak_role',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_jabber_user'] = array(
    'worker callback' => 'omni_eve_api_cron_jabber_user',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_jabber_role'] = array(
    'worker callback' => 'omni_eve_api_cron_jabber_role',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_queue_users'] = array(
    'worker callback' => 'omni_eve_api_cron_queue_users',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_get_standings'] = array(
    'worker callback' => 'omni_eve_api_cron_get_standings',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_get_access_mask'] = array(
    'worker callback' => 'omni_eve_api_cron_get_access_mask',
    'time' => 15,
  );
  $queue['omni_eve_api_cron_get_alliance_corporations'] = array(
    'worker callback' => 'omni_eve_api_cron_get_alliance_corporations',
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
function omni_eve_api_cron_teamspeak_user($item) {
  try {
    $roles = user_roles();
    $valid_roles = $user_roles = array();

    $query = db_select('oea_alliance_corporations', 'oea');
    $query->fields('oea', array('corporationname', 'corporationticker'));
    $result = $query->execute();

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

    $uid = $item['uid'];
    $teamspeakdbid = $item['teamspeakdbid'];
    $ts3 = omni_eve_api_teamspeak_connection();

    $query = db_select('users_roles', 'ur');
    $query->join('role', 'r', 'r.rid = ur.rid');
    $query->fields('r', array('rid', 'name'));
    $query->condition('uid', $uid, '=');
    $result = $query->execute();

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
function omni_eve_api_cron_teamspeak_name($item) {
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

      $query = db_select('oea_characters', 'c');
      $query->condition('c.deleted', 0);
      $query->join('users', 'u', 'c.charactername = u.name');
      $query->fields('c', array('charactername'));
      $query->condition('u.teamspeakuid', $teamspeakuid);
      $result = $query->execute();

      if (!$result->rowCount()) {
        $teamspeak_user->message(t('[color=red]Your Teamspeak ID is not registered on our server.[/color]'));
        $teamspeak_user->message(t('[color=red]If you do not register within 10 minutes[/color]'));
        $teamspeak_user->message(t('[color=red]you will be removed from the server.[/color]'));
        $teamspeak_user->message(t('[color=red]To register, go to [url]@homepage[/url][/color]', array('@homepage' => url('user/register', array('absolute' => TRUE)))));

        if (time() >= ($teamspeak_user->client_lastconnected + 600)) {
          omni_eve_api_teamspeak_user_delete($ts3, $teamspeak_user->client_database_id);
          $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('Please Register!'));
        }

        continue;
      }

      $row = $result->fetchAssoc();

      $expected_teamspeaknickname = omni_eve_api_teamspeak_format_name($row['charactername']);

      if ($teamspeaknickname != $expected_teamspeaknickname) {
        $teamspeak_user->message(t('[color=red]Your nickname is incorrect.[/color]'));
        $teamspeak_user->message(t('[color=red]If you do not correct your nickname within 10 minutes[/color]'));
        $teamspeak_user->message(t('[color=red]Nickname expected: @nickname[/color]', array('@nickname' => $expected_teamspeaknickname)));

        if (time() >= ($teamspeak_user->client_lastconnected + 600)) {
          $teamspeak_user->kick(TeamSpeak3::KICK_SERVER, t('You were told, fix your nickname!'));
        }
      }
    }
  }
  catch (OeaException $e) {
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
function omni_eve_api_cron_teamspeak_role($item) {
  try {
    $roles = user_roles();
    $ts3 = omni_eve_api_teamspeak_connection();
    $corp_info = array();

    $query = db_select('oea_alliance_corporations', 'oea');
    $query->fields('oea', array('corporationname', 'corporationticker'));
    $result = $query->execute();

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
function omni_eve_api_cron_jabber_user($item) {
  try {
    $user_roles = array();

    $query = db_select('oea_alliance_corporations', 'oea');
    $query->fields('oea', array('corporationname', 'corporationticker'));
    $result = $query->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $corp_info[$row->corporationname] = $row->corporationticker;
      }
    }

    $uid = $item['uid'];
    $charactername = $item['name'];

    $query = db_select('users_roles', 'ur');
    $query->join('role', 'r', 'r.rid = ur.rid');
    $query->fields('r', array('rid', 'name'));
    $query->condition('uid', $uid, '=');
    $result = $query->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $rname = $row->name;

        foreach ($corp_info as $key => $value) {
          if ($key == $rname) {
            $rname = $value;
          }
        }

        $user_roles[] = omni_eve_api_jabber_format_name($rname);
      }
    }

    $query = array(
      'groups' => implode(",", $user_roles),
    );

    $result = omni_eve_api_jabber_url_query('update', $charactername, $query);
  }
  catch (OeaException $e) {
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
function omni_eve_api_cron_jabber_role($item) {
  try {
    $roles = user_roles();
    $corp_info = array();

    $query = db_select('oea_alliance_corporations', 'oea');
    $query->fields('oea', array('corporationname', 'corporationticker'));
    $result = $query->execute();

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
function omni_eve_api_cron_queue_users($item) {
  $uid = $item;
  $all_characters = array();

  $query = db_select('oea_api_keys', 'a');
  $query->fields('a', array('apiid', 'keyid', 'vcode'));
  $query->condition('drupalid', $uid, '=');
  $result = $query->execute();

  $updated = date('Y-m-d H:i:s', time());

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $error = 0;

      $query = array(
        'keyid' => $row->keyid,
        'vcode' => $row->vcode,
      );

      $characters = omni_eve_api_get_character_api($query);

      if (isset($characters['error']) && $characters['error'] != 403) {
        $new_api_key_id = db_merge('oea_api_keys')->key(array('apiid' => $row->apiid))->fields(array(
          'apiid' => $row->apiid,
          'errorid' => $characters['error'],
          'updated' => $updated,
        ))->execute();

        db_update('oea_characters')->fields(array('errorid' => $characters['error']))->condition('apiid', $row->apiid, '=')->execute();

        continue;
      }
      elseif (isset($characters['error']) && $characters['error'] == 403) {
        $new_api_key_id = db_merge('oea_api_keys')->key(array('apiid' => $row->apiid))->fields(array(
          'apiid' => $row->apiid,
          'errorid' => $characters['error'],
          'updated' => $updated,
        ))->execute();

        db_update('oea_characters')->fields(array('errorid' => $characters['error'], 'deleted' => 1))->condition('apiid', $row->apiid, '=')->execute();

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
        if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('omni_eve_api_access_mask', 268435455)) != variable_get('omni_eve_api_access_mask', 268435455)) {
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
    ))->condition('deleted', 0, '=')->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($corporation_role = user_role_load_by_name($row->corporationname)) {
          user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);
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
      $row = db_select('users', 'u')->fields('u', array('teamspeakdbid'))->condition('uid', $uid, '=')->execute()->fetchAssoc();
      $ts3 = omni_eve_api_teamspeak_connection();
      omni_eve_api_teamspeak_user_delete($ts3, $row['teamspeakdbid']);
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    try {
      omni_eve_api_jabber_connect();
      $row = db_select('users', 'u')->fields('u', array('name'))->condition('uid', $uid, '=')->execute()->fetchAssoc();

      $result = db_select('ofUser', 'u')->fields('u', array(
        'username',
      ))->condition('username', omni_eve_api_jabber_format_name($row['name']), '=')->execute();

      if ($result->rowCount()) {
        omni_eve_api_jabber_url_query('delete', $account->name);
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
    return FALSE;
  }
  else {
    $row = db_select('users', 'u')->fields('u', array('name'))->condition('uid', $uid, '=')->execute()->fetchAssoc();
    $character_data = omni_eve_api_get_character_info($row['name']);

    if ($corporation_role = user_role_load_by_name($character_data['corporationName'])) {
      $result = db_select('oea_alliance_corporations', 'ac')->fields('ac', array(
        'corporationid',
        'corporationname',
      ))->condition('deleted', 0, '=')->execute();

      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $row) {
          if (($corporation_role_2 = user_role_load_by_name($row->corporationname)) && $row->corporationid != $character_data['corporationID']) {
            user_multiple_role_edit(array($uid), 'remove_role', $corporation_role_2->rid);
          }
        }
      }

      user_multiple_role_edit(array($uid), 'add_role', $corporation_role->rid);

      $blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);

      $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', $alliance_role->rid);
    }
    else {
      $result = db_select('oea_alliance_corporations', 'ac')->fields('ac', array(
        'corporationid',
        'corporationname',
      ))->condition('deleted', 0, '=')->execute();

      if ($result->rowCount()) {
        foreach ($result->fetchAll() as $row) {
          if ($corporation_role = user_role_load_by_name($row->corporationname)) {
            user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);
          }
        }
      }

      $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
      user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);

      $blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', $blue_role->rid);
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
function omni_eve_api_cron_get_standings($item = NULL) {
  $query = array(
    'keyid' => variable_get('omni_eve_api_corp_keyid', ''),
    'vcode' => variable_get('omni_eve_api_corp_vcode', ''),
  );
  $xml = omni_eve_api_get_xml('ContactList', $query);

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>Error: ' . $xml . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
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
function omni_eve_api_cron_get_alliance_corporations($item = NULL) {
  $xml = omni_eve_api_get_xml('AllianceList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>Error: ' . $xml . '</pre>', NULL, WATCHDOG_WARNING);
    return array('error' => $xml);
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

  $result = db_select('oea_alliance_corporations', 'ac')->fields('ac', array(
    'corporationid',
    'corporationname',
  ))->condition('deleted', 1, '=')->execute();

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
function omni_eve_api_cron_get_access_mask($item = NULL) {
  $xml = omni_eve_api_get_xml('callList');

  if (!isset($xml->cachedUntil)) {
    watchdog('eve api', '<pre>Error: ' . $xml . '</pre>', NULL, WATCHDOG_WARNING);
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
