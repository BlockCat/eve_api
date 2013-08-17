<?php
/**
 * @file
 * Functions related to allowing users to alter the user settings and options.
 */

module_load_include('inc', 'omni_eve_api', 'omni_eve_api.api');
module_load_include('inc', 'omni_eve_api', 'omni_eve_api.cron');

/**
 * Form constructor for the user add API form.
 *
 * @see omni_eve_api_user_add_api_form_validate()
 * @see omni_eve_api_user_add_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_add_api_form($form, &$form_state, $account) {
  if (!isset($form_state['user'])) {
    $form_state['user'] = $account;
  }
  else {
    $account = $form_state['user'];
  }

  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Verify Blue Status'),
    '#description' => t('Please enter your EVE API in order to add it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['enter_api']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['enter_api']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add API Key'),
  );

  $form['#validate'] = array('omni_eve_api_user_add_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_add_api_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_add_api_form().
 *
 * @see omni_eve_api_user_add_api_form_submit()
 */
function omni_eve_api_user_add_api_form_validate($form, &$form_state) {
  $key_id = (int) $form_state['values']['keyID'];
  $v_code = (string) $form_state['values']['vCode'];

  if (empty($key_id) || empty($v_code) || preg_match('/[^a-z0-9]/i', $v_code) || preg_match('/[^0-9]/', $form_state['values']['keyID']) || strlen($v_code) > 64 || strlen($v_code) < 20) {
    form_set_error('keyID', t('Invalid input, please try again.'));
    form_set_error('vCode');
    return;
  }

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyID', t('There was an error with the API.'));
    form_set_error('vCode');
  }
  else {
    $whitelist = array();

    if (!empty($characters)) {
      foreach ($characters['characters'] as $character) {
        $whitelist[] = (int) $character['characterID'];
      }
    }

    $result = db_query('SELECT characterID FROM {oea_whitelist} WHERE deleted = 0 AND characterID IN (:characterIDs)', array(
      ':characterIDs' => $whitelist,
    ));

    if ($result->rowCount()) {
      if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
        form_set_error('keyID', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
        form_set_error('vCode');
      }
    }
    else {
      if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('omni_eve_api_access_mask', 268435455)) != variable_get('omni_eve_api_access_mask', 268435455)) {
        form_set_error('keyID', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
        form_set_error('vCode');
      }
    }

    if ($chars = omni_eve_api_characters_exist($characters)) {
      form_set_error('keyID', t('Characters on this key have already been registered. Characters registered: @chars', array('@chars' => implode(", ", $chars))));
      form_set_error('vCode');
    }
  }
}

/**
 * Form submission handler for omni_eve_api_user_add_api_form().
 *
 * @see omni_eve_api_user_add_api_form_validate()
 */
function omni_eve_api_user_add_api_form_submit($form, &$form_state) {
  $account = $form_state['user'];
  $uid = (int) $account->uid;
  $key_id = (int) $form_state['values']['keyID'];
  $v_code = (string) $form_state['values']['vCode'];

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    drupal_set_message(t('There was an error with the API.'), 'error');
  }
  else {
    $created = date('Y-m-d H:i:s', time());

    $api_id = db_insert('oea_api_keys')->fields(array(
      'uid' => $uid,
      'keyID' => $key_id,
      'vCode' => $v_code,
      'updated' => (string) $created,
      'created' => (string) $created,
    ))->execute();

    foreach ($characters['characters'] as $character) {
      db_merge('oea_characters')->key(array('characterID' => (int) $character['characterID']))->fields(array(
        'characterID' => (int) $character['characterID'],
        'uid' => $uid,
        'apiID' => (int) $api_id,
        'characterName' => (string) $character['characterName'],
        'corporationID' => (int) $character['corporationID'],
        'corporationName' => (string) $character['corporationName'],
        'corporationTicker' => (string) $character['corporationTicker'],
        'allianceID' => (int) $character['allianceID'],
        'allianceName' => (string) $character['allianceName'],
        'allianceTicker' => (string) $character['allianceTicker'],
        'deleted' => 0,
      ))->execute();
    }

    $result = db_query('SELECT c.characterName, c.characterID FROM {oea_characters} c INNER JOIN {users} u ON (c.characterName = u.name) WHERE c.deleted = 0 AND u.uid = :uid', array(
      ':uid' => $uid,
    ));

    if (!$result->rowCount()) {
      try {
        omni_eve_api_jabber_connect();

        $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
          ':username' => (string) omni_eve_api_jabber_format_name($account->name),
        ));

        if ($result->rowCount()) {
          omni_eve_api_jabber_url_query('delete', $account->name);
        }

        omni_eve_api_jabber_disconnect();
      }
      catch (OeaException $e) {
        // Log Error ?
      }

      try {
        $ts3 = omni_eve_api_teamspeak_connection();
        omni_eve_api_teamspeak_user_delete($ts3, $account->teamspeakDBID);
      }
      catch (OeaException $e) {
        // Log Error ?
      }

      db_update('users')->fields(array(
        'teamspeakUID' => 0,
        'teamspeakDBID' => 0,
        'characterID' => 0,
      ))->condition('uid', $uid, '=')->execute();
      $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
      drupal_set_message(t('Please select a new Main Character'));
    }

    drupal_set_message(t('API Key successfully added!'));
  }
}

/**
 * Form constructor for the user select main character form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see omni_eve_api_user_select_main_character_form_validate()
 * @see omni_eve_api_user_select_main_character_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_select_main_character_form($form, &$form_state, $account) {
  if (!isset($form_state['user'])) {
    $form_state['user'] = $account;
  }
  else {
    $account = $form_state['user'];
  }

  $uid = $account->uid;

  $form['main_character'] = array(
    '#type' => 'fieldset',
    '#title' => t('Valid Characters'),
    '#weight' => 0,
  );

  $form['main_character']['name'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => $account->name,
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );

  $uid_api_list = omni_eve_api_list_api_simple($uid);
  $valid_characters_list = omni_eve_api_list_valid_characters($uid_api_list, $account->name);

  $result = db_query('SELECT characterID FROM {users} WHERE characterID = 0 AND uid = :uid', array(
    ':uid' => $uid,
  ));

  if ($result->rowCount()) {
    $form['main_character']['name']['#default_value'] = '';
    $valid_characters_list = array('' => t('-- Select a New Character --')) + $valid_characters_list;
  }

  $form['main_character']['name']['#options'] = $valid_characters_list;

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['#validate'] = array('omni_eve_api_user_select_main_character_form_validate');
  $form['#submit'] = array('omni_eve_api_user_select_main_character_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_select_main_character_form().
 *
 * @see omni_eve_api_user_select_main_character_form_submit()
 */
function omni_eve_api_user_select_main_character_form_validate($form, &$form_state) {
  $account = $form_state['user'];
  $uid = $account->uid;

  if ($account->name == (string) $form_state['values']['name'] && $account->characterID != 0 && !empty($form_state['values']['name'])) {
    form_set_error('name', t('You have not selected a new character or you are unable to change characters.'));
  }

  $character_name = (string) $form_state['values']['name'];
  $character_data = omni_eve_api_get_character_info($character_name);

  if ($character_data === FALSE || $character_data['uid'] != $uid) {
    form_set_error('name', t('Character does not exist or does not belong to you.'));
  }
}

/**
 * Form submission handler for omni_eve_api_user_select_main_character_form().
 *
 * @see omni_eve_api_user_select_main_character_form_validate()
 */
function omni_eve_api_user_select_main_character_form_submit($form, &$form_state) {
  $account = $form_state['user'];
  $uid = (int) $account->uid;

  try {
    omni_eve_api_jabber_connect();

    $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
      ':username' => (string) omni_eve_api_jabber_format_name($account->name),
    ));

    if ($result->rowCount()) {
      omni_eve_api_jabber_url_query('delete', $account->name);
    }

    omni_eve_api_jabber_disconnect();
  }
  catch (OeaException $e) {
    // Log Error ?
  }

  $old_character_name = $account->name;
  $old_character_data = omni_eve_api_get_character_info($old_character_name);

  if ($corporation_role = user_role_load_by_name($old_character_data['corporationName'])) {
    user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);

    $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);
  }
  elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2))) {
    user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);
  }

  $default_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
  user_multiple_role_edit(array($uid), 'remove_role', $default_role->rid);

  $character_name = (string) $form_state['values']['name'];
  $character_data = omni_eve_api_get_character_info($character_name);

  $character_is_blue = omni_eve_api_verify_blue($character_data);

  if ($corporation_role = user_role_load_by_name($character_data['corporationName'])) {
    user_multiple_role_edit(array($uid), 'add_role', $corporation_role->rid);

    $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $alliance_role->rid);
  }
  elseif (($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2))) && $character_is_blue) {
    user_multiple_role_edit(array($uid), 'add_role', $blue_role->rid);
  }
  else {
    $default_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $default_role->rid);
  }

  db_update('users')->fields(array('characterID' => (int) $character_data['characterID'], 'name' => (string) $character_data['characterName']))->condition('uid', $uid, '=')->execute();

  $queue = DrupalQueue::get('omni_eve_api_cron_api_user_sync');
  $queue->createItem($uid);

  $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user_sync');
  $queue->createItem(array(
    'uid' => $uid,
    'teamspeakDBID' => $account->teamspeakDBID,
    'runs' => 1,
  ));

  drupal_set_message(t('New main character selected!'));
}

/**
 * Form constructor for the user list API form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see omni_eve_api_user_list_api_form_validate()
 * @see omni_eve_api_user_list_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_list_api_form($form, &$form_state, $account) {
  if (!isset($form_state['user'])) {
    $form_state['user'] = $account;
  }
  else {
    $account = $form_state['user'];
  }

  $uid = (int) $account->uid;
  $api_list = array();

  $header = array(
    'action' => array(
      'data' => t('Action'),
    ),
    'keyID' => array(
      'data' => t('API Key ID'),
      'field' => 'a.keyID',
    ),
    'vCode' => array(
      'data' => t('API Verification Code'),
      'field' => 'a.vCode',
    ),
    'characters' => array(
      'data' => t('Characters'),
    ),
    'updated' => array(
      'data' => t('Last Updated'),
      'field' => 'a.updated',
    ),
    'created' => array(
      'data' => t('Created'),
      'field' => 'a.created',
    ),
    'errorID' => array(
      'data' => t('Status'),
      'field' => 'a.errorID',
    ),
  );

  $result_a = db_select('oea_api_keys', 'a')
    ->fields('a', array(
      'apiID',
      'keyID',
      'vCode',
      'errorID',
      'updated',
      'created',
    ))
    ->extend('PagerDefault')
    ->limit(3)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->condition('uid', $uid, '=')
    ->execute();

  if ($result_a->rowCount()) {
    foreach ($result_a->fetchAll() as $row_a) {
      $api_list[$row_a->apiID] = array(
        'apiID' => check_plain($row_a->apiID),
        'keyID' => check_plain($row_a->keyID),
        'vCode' => check_plain($row_a->vCode),
        'errorID' => check_plain($row_a->errorID),
        'updated' => check_plain($row_a->updated),
        'created' => check_plain($row_a->created),
      );

      $result_b = db_query('SELECT characterID, characterName, corporationID, corporationName, corporationTicker, allianceID, allianceName, allianceTicker FROM {oea_characters} WHERE deleted = 0 AND uid = :uid AND apiID = :apiID', array(
        ':uid' => $uid,
        ':apiID' => (int) $row_a->apiID,
      ));

      if ($result_b->rowCount()) {
        foreach ($result_b->fetchAll() as $row_b) {
          $api_list[$row_a->apiID]['characters'][$row_b->characterID] = array(
            'characterID' => check_plain($row_b->characterID),
            'characterName' => check_plain($row_b->characterName),
            'corporationID' => check_plain($row_b->corporationID),
            'corporationName' => check_plain($row_b->corporationName),
            'corporationTicker' => check_plain($row_b->corporationTicker),
            'allianceID' => check_plain($row_b->allianceID),
            'allianceName' => check_plain($row_b->allianceName),
            'allianceTicker' => check_plain($row_b->allianceTicker),
          );
        }
      }
    }
  }

  $options = array();

  foreach ($api_list as $api_single) {
    $list_characters = '';

    if (isset($api_single['characters'])) {
      foreach ($api_single['characters'] as $characters) {
        $corp_alliance = ($characters['allianceID'] == 0) ? $characters['corporationName'] : $characters['corporationName'] . '/' . $characters['allianceName'];
        $list_characters .= check_plain('[' . $characters['corporationTicker'] . '] ' . $characters['characterName'] . ' (' . $corp_alliance . ')');
        $list_characters .= '<br />';
      }
    }

    $v_code = (strlen($api_single['vCode']) > 13) ? substr($api_single['vCode'], 0, 10) . '...' : $api_single['vCode'];

    $options[$api_single['apiID']] = array(
      'action' => array(
        'data' => array(
          '#type' => 'link',
          '#title' => t('Edit'),
          '#href' => 'user/' . $uid . '/omni_eve_api/update_api_key/' . $api_single['apiID'],
        ),
      ),
      'keyID' => check_plain($api_single['keyID']),
      'vCode' => check_plain($v_code),
      'characters' => $list_characters,
      'updated' => format_date(strtotime($api_single['updated']), 'short'),
      'created' => format_date(strtotime($api_single['created']), 'short'),
      'errorID' => check_plain(omni_eve_api_api_error_msg($api_single['errorID'])),
    );
  }

  $form['list_api']['api_keys'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No API Keys associated with your account.'),
    '#weight' => 0,
  );

  $form['list_api']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
  );

  $form['#validate'] = array('omni_eve_api_user_list_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_list_api_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_list_api_form().
 *
 * @see omni_eve_api_user_list_api_form_submit()
 */
function omni_eve_api_user_list_api_form_validate($form, &$form_state) {
  if (!is_array($form_state['values']['api_keys']) || !count(array_filter($form_state['values']['api_keys']))) {
    form_set_error('', t('No items selected.'));
  }
}

/**
 * Form submission handler for omni_eve_api_user_list_api_form().
 *
 * @see omni_eve_api_user_list_api_form_validate()
 */
function omni_eve_api_user_list_api_form_submit($form, &$form_state) {
  $account = $form_state['user'];
  $uid = (int) $account->uid;

  foreach ((array) $form_state['values']['api_keys'] as $api_id) {
    if ($api_id == 0 && !is_int($api_id)) {
      continue;
    }

    db_delete('oea_api_keys')->condition('apiID', (int) $api_id)->execute();
    db_update('oea_characters')->fields(array('deleted' => 1))->condition('apiID', (int) $api_id)->execute();
  }

  $result = db_query('SELECT c.characterName, c.characterID FROM {oea_characters} c INNER JOIN {users} u ON (c.characterName = u.name) WHERE c.deleted = 0 AND u.uid = :uid', array(
    ':uid' => $uid,
  ));

  if (!$result->rowCount()) {
    $result = db_query('SELECT corporationID, corporationName FROM {oea_alliance_corporations} WHERE deleted = 0');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($corporation_role = user_role_load_by_name($row->corporationName)) {
          user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);
        }
      }
    }

    $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);

    $blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2));
    user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);

    $unverified_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $unverified_role->rid);

    try {
      omni_eve_api_jabber_connect();

      $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
        ':username' => (string) omni_eve_api_jabber_format_name($account->name),
      ));

      if ($result->rowCount()) {
        omni_eve_api_jabber_url_query('delete', $account->name);
      }

      omni_eve_api_jabber_disconnect();
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    db_update('users')->fields(array(
      'characterID' => 0,
    ))->condition('uid', $uid, '=')->execute();
    $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
    drupal_set_message(t('Please select a new Main Character'));
  }

  $queue = DrupalQueue::get('omni_eve_api_cron_api_user_sync');
  $queue->createItem($uid);

  $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user_sync');
  $result = db_query('SELECT teamspeakDBID FROM {users} WHERE uid = :uid', array(
    'uid' => $uid,
  ));
  $row = $result->fetchAssoc();
  $queue->createItem(array(
    'uid' => $uid,
    'teamspeakDBID' => $row['teamspeakDBID'],
    'runs' => 1,
  ));

  drupal_set_message(t('API keys successfully deleted!'));
}

/**
 * Form constructor for the user update API form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 * @param string $api_id
 *   The unique apiID that is being updated.
 *
 * @see omni_eve_api_user_update_api_form_validate()
 * @see omni_eve_api_user_update_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_update_api_form($form, &$form_state, $account, $api_id) {
  if (!isset($form_state['user'])) {
    $form_state['user'] = $account;
  }
  else {
    $account = $form_state['user'];
  }

  $uid = (int) $account->uid;

  $result = db_query('SELECT keyID, vCode FROM {oea_api_keys} WHERE apiID = :apiID', array(
    ':apiID' => (int) $api_id,
  ));

  if (!$result->rowCount()) {
    drupal_goto('user/' . $uid . '/omni_eve_api');
  }

  $row = $result->fetchAssoc();

  $form['update_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update API'),
    '#description' => t('Please enter your EVE API in order to update it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['update_api']['apiID'] = array(
    '#value' => $api_id,
    '#type' => 'hidden',
  );

  $form['update_api']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#default_value' => isset($form_state['values']['keyID']) ? (int) $form_state['values']['keyID'] : $row['keyID'],
    '#weight' => 0,
  );

  $form['update_api']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#default_value' => isset($form_state['values']['vCode']) ? (string) $form_state['values']['vCode'] : $row['vCode'],
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update API Key'),
  );

  $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api';
  $form['#validate'] = array('omni_eve_api_user_update_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_update_api_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_update_api_form().
 *
 * @see omni_eve_api_user_update_api_form_submit()
 */
function omni_eve_api_user_update_api_form_validate($form, &$form_state) {
  $key_id = (int) $form_state['values']['keyID'];
  $v_code = (string) $form_state['values']['vCode'];

  if (empty($key_id) || empty($v_code) || preg_match('/[^a-z0-9]/i', $v_code) || preg_match('/[^0-9]/', $form_state['values']['keyID']) || strlen($v_code) > 64 || strlen($v_code) < 20) {
    form_set_error('keyID', t('Invalid input, please try again.'));
    form_set_error('vCode');
    return;
  }

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyID', t('There was an error with the API.'));
    form_set_error('vCode');
  }
  else {
    $form_state['values']['characters'] = $characters;

    $whitelist = array();

    if (!empty($characters)) {
      foreach ($characters['characters'] as $character) {
        $whitelist[] = (int) $character['characterID'];
      }
    }

    $result = db_query('SELECT characterID FROM {oea_whitelist} WHERE deleted = 0 AND characterID IN (:characterIDs)', array(
      ':characterIDs' => $whitelist,
    ));

    if ($result->rowCount()) {
      if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
        form_set_error('keyID', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
        form_set_error('vCode');
      }
    }
    else {
      if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('omni_eve_api_access_mask', 268435455)) != variable_get('omni_eve_api_access_mask', 268435455)) {
        form_set_error('keyID', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
        form_set_error('vCode');
      }
    }
  }
}

/**
 * Form submission handler for omni_eve_api_user_update_api_form().
 *
 * @see omni_eve_api_user_update_api_form_validate()
 */
function omni_eve_api_user_update_api_form_submit($form, &$form_state) {
  $account = $form_state['user'];
  $uid = (int) $account->uid;

  $key_id = (int) $form_state['values']['keyID'];
  $v_code = (string) $form_state['values']['vCode'];
  $api_id = (int) $form_state['values']['apiID'];
  $characters = (array) $form_state['values']['characters'];

  $updated = date('Y-m-d H:i:s', time());

  db_update('oea_characters')->fields(array('deleted' => 1))->condition('apiID', $api_id)->execute();

  db_merge('oea_api_keys')->key(array('apiID' => $api_id))->fields(array(
    'apiID' => $api_id,
    'uid' => $uid,
    'keyID' => $key_id,
    'vCode' => $v_code,
    'updated' => (string) $updated,
    'errorID' => 0,
  ))->execute();

  foreach ($characters['characters'] as $character) {
    db_merge('oea_characters')->key(array('characterID' => (int) $character['characterID']))->fields(array(
      'characterID' => (int) $character['characterID'],
      'uid' => $uid,
      'apiID' => $api_id,
      'characterName' => (string) $character['characterName'],
      'corporationID' => (int) $character['corporationID'],
      'corporationName' => (string) $character['corporationName'],
      'corporationTicker' => (string) $character['corporationTicker'],
      'allianceID' => (int) $character['allianceID'],
      'allianceName' => (string) $character['allianceName'],
      'allianceTicker' => (string) $character['allianceTicker'],
      'errorID' => 0,
      'deleted' => 0,
    ))->execute();
  }

  $result = db_query('SELECT c.characterName, c.characterID FROM {oea_characters} c INNER JOIN {users} u ON (c.characterName = u.name) WHERE c.deleted = 0 AND u.uid = :uid', array(
    ':uid' => $uid,
  ));

  if (!$result->rowCount()) {
    try {
      omni_eve_api_jabber_connect();

      $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
        ':username' => (string) omni_eve_api_jabber_format_name($account->name),
      ));

      if ($result->rowCount()) {
        omni_eve_api_jabber_url_query('delete', $account->name);
      }

      omni_eve_api_jabber_disconnect();
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    try {
      $ts3 = omni_eve_api_teamspeak_connection();
      omni_eve_api_teamspeak_user_delete($ts3, $account->teamspeakDBID);
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    db_update('users')->fields(array(
      'teamspeakUID' => 0,
      'teamspeakDBID' => 0,
      'characterID' => 0,
    ))->condition('uid', $uid, '=')->execute();
    $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
    drupal_set_message(t('Please select a new Main Character'));
  }

  drupal_set_message(t('API Key successfully updated!'));
}

/**
 * Form constructor for the user teamspeak registration form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see omni_eve_api_user_teamspeak_form_validate()
 * @see omni_eve_api_user_teamspeak_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_teamspeak_form($form, &$form_state, $account) {
  if (!isset($form_state['user'])) {
    $form_state['user'] = $account;
  }
  else {
    $account = $form_state['user'];
  }

  $uid = $account->uid;

  $form['teamspeak'] = array(
    '#type' => 'fieldset',
    '#title' => t('Register your Teamspeak Identity to your User Account'),
    '#weight' => 0,
  );

  $teamspeak_port_check = variable_get('omni_eve_api_ts3_server_port', 9987);
  $teamspeak_password_check = variable_get('omni_eve_api_ts3_server_password', '');

  $form['teamspeak']['howto'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => t('In order to register with Teamspeak, you must first connect to our Teamspeak.<br />If you do not have Teamspeak already installed, please download and install Teamspeak 3 from <a href="@teamspeak_url" target="_blank">here</a>.<br />Please use the following settings to connect:<br /><br />Address - @teamspeak_address<br />@teamspeak_password', array(
      '@teamspeak_url' => 'http://www.teamspeak.com/?page=downloads',
      '@teamspeak_address' => empty($teamspeak_port_check) ? variable_get('omni_eve_api_ts3_server_address', 'teamspeak.com') : variable_get('omni_eve_api_ts3_server_address', 'teamspeak.com') . ':' . variable_get('omni_eve_api_ts3_server_port', 9987),
      '@teamspeak_password' => empty($teamspeak_password_check) ? '' : t('Password') . ' - ' . variable_get('omni_eve_api_ts3_server_password', ''),
    )),
    '#suffix' => '</p>',
    '#weight' => 0,
  );

  $form['teamspeak']['teamspeak_name'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => ($account->characterID != 0) ? t('Teamspeak Name: @teamspeak', array('@teamspeak' => omni_eve_api_teamspeak_format_name($account->name))) : t('Teamspeak Name: <a href="@url">Please select a main character.</a>', array('@url' => url('user/' . $uid . '/omni_eve_api/select_main_character'))),
    '#suffix' => '</p>',
    '#weight' => 10,
  );

  $form['teamspeak']['teamspeak_uid'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => !empty($account->teamspeakUID) ? t('Teamspeak Unique ID: @teamspeak', array('@teamspeak' => $account->teamspeakUID)) : t('Teamspeak Unique ID: You are not registered.'),
    '#suffix' => '</p>',
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Register'),
  );

  if ($account->characterID == 0) {
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('omni_eve_api_user_teamspeak_form_validate');
  $form['#submit'] = array('omni_eve_api_user_teamspeak_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_teamspeak_form().
 *
 * @see omni_eve_api_user_teamspeak_form_submit()
 */
function omni_eve_api_user_teamspeak_form_validate($form, &$form_state) {
  $account = $form_state['user'];
  $uid = $account->uid;

  if ($account->characterID == 0) {
    drupal_goto('user/' . $uid . '/omni_eve_api/select_main_character');
  }

  try {
    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_name = omni_eve_api_teamspeak_format_name($account->name);
    $teamspeak_data = omni_eve_api_teamspeak_user_by_name($ts3, $teamspeak_name);
    unset($teamspeak_data);
  }
  catch (OeaException $e) {
    form_set_error('teamspeak', check_plain($e->getMessage()));
  }
}

/**
 * Form submission handler for omni_eve_api_user_teamspeak_form().
 *
 * @see omni_eve_api_user_teamspeak_form_validate()
 */
function omni_eve_api_user_teamspeak_form_submit($form, &$form_state) {
  $account = $form_state['user'];
  $uid = (int) $account->uid;

  try {
    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_name = omni_eve_api_teamspeak_format_name($account->name);
    $teamspeak_data = omni_eve_api_teamspeak_user_by_name($ts3, $teamspeak_name);
    $teamspeak_uid = (string) $teamspeak_data->client_unique_identifier;
    $teamspeak_dbid = (int) $teamspeak_data->client_database_id;

    db_update('users')->fields(array(
        'teamspeakUID' => $teamspeak_uid,
        'teamspeakDBID' => $teamspeak_dbid,
      ))->condition('uid', $uid, '=')->execute();

    omni_eve_api_cron_teamspeak_user_sync(array(
      'uid' => $uid,
      'teamspeakDBID' => $teamspeak_dbid,
      'runs' => 1,
    ));

    // Need to assign roles.
    drupal_set_message(t('Teamspeak access granted.'));
  }
  catch (OeaException $e) {
    drupal_set_message(check_plain($e->getMessage()), 'error');
  }
}

/**
 * Form constructor for the user jabber registration form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see omni_eve_api_user_jabber_form_validate()
 * @see omni_eve_api_user_jabber_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_jabber_form($form, &$form_state, $account) {
  if (!isset($form_state['user'])) {
    $form_state['user'] = $account;
  }
  else {
    $account = $form_state['user'];
  }

  $uid = $account->uid;

  $form['jabber'] = array(
    '#type' => 'fieldset',
    '#title' => t('Register for a Jabber Account.'),
    '#weight' => 0,
  );

  $form['jabber']['howto'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => t('If you do not have a Jabber Client already installed, you can download and install Trillian from <a href="@jabber_url" target="_blank">here</a>.', array(
      '@jabber_url' => 'https://www.trillian.im/download/',
    )),
    '#suffix' => '</p>',
    '#weight' => 0,
  );

  $form['jabber']['jabber_username'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => ($account->characterID != 0) ? t('Jabber Username: @jabber', array('@jabber' => omni_eve_api_jabber_format_name($account->name) . '@' . variable_get('omni_eve_api_jabber_domain', 'yourdomain.com'))) : t('Jabber Username: <a href="@url">Please select a main character.</a>', array('@url' => url('user/' . $uid . '/omni_eve_api/select_main_character'))),
    '#suffix' => '</p>',
    '#weight' => 10,
  );

  if ($account->characterID != 0) {
    $form['jabber']['jabber_password'] = array(
      '#type' => 'textfield',
      '#title' => t('Set Password'),
      '#description' => t('You can set or change your Jabber password.'),
      '#maxlength' => 50,
      '#weight' => 20,
      '#default_value' => '',
    );
  }

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Register'),
  );

  if ($account->characterID == 0) {
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('omni_eve_api_user_jabber_form_validate');
  $form['#submit'] = array('omni_eve_api_user_jabber_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_jabber_form().
 *
 * @see omni_eve_api_user_jabber_form_submit()
 */
function omni_eve_api_user_jabber_form_validate($form, &$form_state) {
  $account = $form_state['user'];
  $uid = $account->uid;

  if ($account->characterID == 0) {
    drupal_goto('user/' . $uid . '/omni_eve_api/select_main_character');
  }

  try {
    omni_eve_api_jabber_connect();

    $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
      ':username' => (string) omni_eve_api_jabber_format_name($account->name),
    ));

    if (!$result->rowCount() && empty($form_state['values']['jabber_password'])) {
      form_set_error('jabber_password', t('You are registering for the first time, you must enter a password.'));
    }

    omni_eve_api_jabber_disconnect();
  }
  catch (OeaException $e) {
    form_set_error('jabber_password', t('There was an error connecting with the Jabber Server, please try again later.'));
  }
}

/**
 * Form submission handler for omni_eve_api_user_jabber_form().
 *
 * @see omni_eve_api_user_jabber_form_validate()
 */
function omni_eve_api_user_jabber_form_submit($form, &$form_state) {
  $account = $form_state['user'];

  $roles = user_roles();
  $user_roles = $all_roles = array();

  $result = db_query('SELECT corporationName, corporationTicker FROM {oea_alliance_corporations}');

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

    $all_roles[$rname] = $description;

    $user_roles[] = omni_eve_api_jabber_format_name($rname);
  }

  try {
    omni_eve_api_jabber_connect();

    $query = array(
      'name' => omni_eve_api_jabber_format_display_name($account->name),
      'email' => $account->mail,
      'groups' => implode(",", $user_roles),
    );

    $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
      ':username' => (string) omni_eve_api_jabber_format_name($account->name),
    ));

    if ($result->rowCount()) {
      $type = 'update';
    }
    else {
      $type = 'add';
    }

    foreach ($all_roles as $name => $description) {
      omni_eve_api_jabber_group_add($name, $description);
    }

    omni_eve_api_jabber_disconnect();

    if (!empty($form_state['values']['jabber_password'])) {
      $password = array(
        'password' => (string) $form_state['values']['jabber_password'],
      );

      $query = array_merge($query, $password);
    }

    $result = omni_eve_api_jabber_url_query($type, $account->name, $query);

    if ($result === TRUE) {
      if ($type = 'update') {
        drupal_set_message(t('Your Jabber account was updated.'));
      }
      else {
        drupal_set_message(t('Your Jabber account was created.'));
      }

      foreach ($all_roles as $name => $description) {
        $query = array(
          'name' => omni_eve_api_jabber_format_name($name),
          'item_jid' => omni_eve_api_jabber_format_name($name) . '@broadcast.' . variable_get('omni_eve_api_jabber_domain', 'yourdomain.com'),
          'subscription' => 3,
        );

        omni_eve_api_jabber_url_query('add_roster', $account->name, $query);

        $query_add = array(
          'groups' => omni_eve_api_jabber_format_name($name),
        );

        $query = array_merge($query, $query_add);

        omni_eve_api_jabber_url_query('update_roster', $account->name, $query);
      }
    }
    else {
      drupal_set_message(t('There was an error updateing your Jabber account, please try again later.'));
    }
  }
  catch (OeaException $e) {
    drupal_set_message(t('There was an error updateing your Jabber account, please try again later.'));
  }
}
