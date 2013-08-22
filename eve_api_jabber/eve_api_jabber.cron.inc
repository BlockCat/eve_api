<?php
/**
 * @file
 * Functions related to controlling and adding cron jobs.
 */

/**
 * Implements hook_cron().
 */
function eve_api_jabber_cron() {
  if (!variable_get('eve_api_enable_cron', TRUE)) {
    return;
  }

  if (variable_get('eve_api_enable', FALSE) && variable_get('eve_api_jabber_enable', FALSE)) {
    $interval = variable_get('eve_api_jabber_role_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_jabber_role_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_jabber_cron_role_sync');
      $queue->createItem(1);

      variable_set('eve_api_jabber_role_cron_next', time() + $interval);
    }
  }

  if (variable_get('eve_api_enable', FALSE) && variable_get('eve_api_jabber_enable', FALSE)) {
    $interval = variable_get('eve_api_jabber_user_cron_time', 5 * 60);

    if (time() >= variable_get('eve_api_jabber_user_cron_next', 0)) {
      $queue = DrupalQueue::get('eve_api_jabber_cron_user_sync');
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
 * Implements hook_cron_queue_info().
 *
 * Set up ten queues that all send items to the same callback function.
 */
function eve_api_jabber_cron_queue_info() {
  $queue = array();
  $queue['eve_api_jabber_cron_user_sync'] = array(
    'worker callback' => 'eve_api_jabber_cron_user_sync',
    'time' => 15,
  );
  $queue['eve_api_jabber_cron_role_sync'] = array(
    'worker callback' => 'eve_api_jabber_cron_role_sync',
    'time' => 15,
  );
  return $queue;
}

/**
 * Update user permissions in Jabber based on roles.
 *
 * @param mixed $item
 *   whatever was added to the queue via queue->createItem()
 */
function eve_api_jabber_cron_user_sync($item) {
  if ($item['runs'] == 3) {
    watchdog('eve_api_jabber', 'Cron Update Jabber User Roles task has failed 2 times in a row, task was removed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    return FALSE;
  }

  try {
    $user_roles = $corp_info = array();

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
    watchdog('eve_api_jabber', 'Cron Update Jabber User Roles task has failed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    $item['runs'] = $item['runs'] + 1;
    $queue = DrupalQueue::get('eve_api_jabber_cron_user_sync');
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
function eve_api_jabber_cron_role_sync($item) {
  if ($item == 3) {
    watchdog('eve_api_jabber', 'Cron Sync Jabber Roles task has failed 2 times in a row, task was removed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
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
    watchdog('eve_api_jabber', 'Cron Sync Jabber Roles task has failed. Data: ' . drupal_http_build_query($item), NULL, WATCHDOG_WARNING);
    $item = $item + 1;
    $queue = DrupalQueue::get('eve_api_jabber_cron_role_sync');
    $queue->createItem($item);

    return FALSE;
  }

  return TRUE;
}
