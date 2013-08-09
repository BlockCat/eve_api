<?php
/**
 * @file
 * Functions related to allowing users to alter the user settings and options.
 */

/**
 * Form constructor for the user add API form.
 *
 * @see omni_eve_api_user_add_api_form_validate()
 * @see omni_eve_api_user_add_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_add_api_form($form, &$form_state, $account) {
  $uid = $account->uid;

  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Verify Blue Status'),
    '#description' => t('Please enter your EVE API in order to add it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['enter_api']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['enter_api']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
  );

  $form['enter_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyid', t('There was an error with the API.'));
    form_set_error('vcode');
  }
  else {
    $whitelist_characters = array();

    if (!empty($characters)) {
      foreach ($characters['characters'] as $character) {
        $whitelist_characters[] = $character['characterID'];
      }
    }

    $result = db_select('oea_whitelist', 'w')->fields('w', array('characterid'))->condition('characterid', $whitelist_characters, 'IN')->condition('deleted', 0, '=')->execute();
    if ($result->rowCount()) {
      if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
        form_set_error('keyid', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
        form_set_error('vcode');
      }
    }
    else {
      if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('omni_eve_api_access_mask', 268435455)) != variable_get('omni_eve_api_access_mask', 268435455)) {
        form_set_error('keyid', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
        form_set_error('vcode');
      }
    }

    if ($chars = omni_eve_api_characters_exist($characters)) {
      form_set_error('keyid', t('Characters on this key have already been registered. Characters registered: @chars', array('@chars' => implode(", ", $chars))));
      form_set_error('vcode');
    }
  }
}

/**
 * Form submission handler for omni_eve_api_user_add_api_form().
 *
 * @see omni_eve_api_user_add_api_form_validate()
 */
function omni_eve_api_user_add_api_form_submit($form, &$form_state) {
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);
  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    drupal_set_message(t('There was an error with the API.'), 'error');
  }
  else {
    $created = date('Y-m-d H:i:s', time());

    $new_api_key_id = db_insert('oea_api_keys')->fields(array(
      'drupalid' => $uid,
      'keyid' => $keyid,
      'vcode' => $vcode,
      'updated' => $created,
      'created' => $created,
    ))->execute();

    foreach ($characters['characters'] as $character) {
      db_merge('oea_characters')->key(array('characterid' => $character['characterID']))->fields(array(
        'characterid' => $character['characterID'],
        'drupalid' => $uid,
        'apiid' => $new_api_key_id,
        'charactername' => $character['characterName'],
        'corporationid' => $character['corporationID'],
        'corporationname' => $character['corporationName'],
        'corporationticker' => $character['corporationTicker'],
        'allianceid' => $character['allianceID'],
        'alliancename' => $character['allianceName'],
        'allianceticker' => $character['allianceTicker'],
        'deleted' => 0,
      ))->execute();
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
  $uid = $account->uid;
  $uid_api_list = omni_eve_api_list_api_simple($uid);

  $form['main_character'] = array(
    '#type' => 'fieldset',
    '#title' => t('Valid Characters'),
    '#weight' => 0,
  );

  $form['main_character']['name'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => $account->name,
    '#options' => omni_eve_api_list_valid_characters($uid_api_list, $account->name),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );

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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  if ($account->name == $form_state['values']['name'] && $account->characterid != 0 && !empty($form_state['values']['name'])) {
    form_set_error('name', t('You have not selected a different character or you are unable to change characters.'));
  }
  $charactername = $form_state['values']['name'];
  $character_data = omni_eve_api_get_character_info($charactername);

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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  try {
    omni_eve_api_jabber_connect();
    $result = db_select('ofUser', 'u')->fields('u', array(
      'username',
    ))->condition('username', omni_eve_api_jabber_format_name($account->name), '=')->execute();

    if ($result->rowCount()) {
      omni_eve_api_jabber_url_query('delete', $account->name);
    }

    omni_eve_api_jabber_disconnect();
  }
  catch (OeaException $e) {
    // Log Error ?
  }

  $old_charactername = $account->name;
  $old_character_data = omni_eve_api_get_character_info($old_charactername);
  $charactername = $form_state['values']['name'];
  $character_data = omni_eve_api_get_character_info($charactername);

  $character_is_blue = FALSE;

  $result = db_select('oea_blue_standings', 'a')->fields('a', array('contactID'))->condition('contactID', array(
    $character_data['characterID'],
    $character_data['corporationID'],
    $character_data['allianceID'],
  ), 'IN')->condition('standing', 0, '>')->condition('deleted', 0, '=')->execute();
  if ($result->rowCount()) {
    $character_is_blue = TRUE;
  }

  if ($character['allianceID'] == variable_get('omni_eve_api_allianceID', 0) || $character['corporationID'] == variable_get('omni_eve_api_corporationID', 0)) {
    $character_is_blue = TRUE;
  }

  $result = db_select('oea_alliance_corporations', 'c')->fields('c', array('corporationid'))->condition('corporationid', $character_data['corporationID'], '=')->execute();
  if ($result->rowCount()) {
    $character_is_blue = TRUE;
  }

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

  db_update('users')->fields(array('characterid' => $character_data['characterID'], 'name' => $character_data['characterName']))->condition('uid', $uid, '=')->execute();

  if ($corporation_role = user_role_load_by_name($character_data['corporationName'])) {
    user_multiple_role_edit(array($uid), 'add_role', $corporation_role->rid);

    $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $alliance_role->rid);
  }
  elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2)) && $character_is_blue) {
    user_multiple_role_edit(array($uid), 'add_role', $blue_role->rid);
  }
  else {
    $default_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $default_role->rid);
  }

  $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
  $queue->createQueue();
  $queue->createItem($uid);

  $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user');
  $queue->createQueue();
  $queue->createItem(array('uid' => $uid, 'teamspeakdbid' => $account->teamspeakdbid));

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
  $uid = $account->uid;
  $api_list = array();

  $header = array(
    'action' => array(
      'data' => t('Action'),
    ),
    'keyid' => array(
      'data' => t('API Key ID'),
      'field' => 'a.keyid',
    ),
    'vcode' => array(
      'data' => t('API Verification Code'),
      'field' => 'a.vcode',
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
    'errorid' => array(
      'data' => t('Status'),
      'field' => 'a.errorid',
    ),
  );

  $result_a = db_select('oea_api_keys', 'a')
    ->fields('a', array(
      'apiid',
      'keyid',
      'vcode',
      'errorid',
      'updated',
      'created',
    ))
    ->extend('PagerDefault')
    ->limit(3)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->condition('drupalid', $uid, '=')
    ->execute();

  if ($result_a->rowCount()) {
    foreach ($result_a->fetchAll() as $row_a) {
      $api_list[$row_a->apiid] = array(
        'apiid' => $row_a->apiid,
        'keyid' => $row_a->keyid,
        'vcode' => $row_a->vcode,
        'errorid' => $row_a->errorid,
        'updated' => $row_a->updated,
        'created' => $row_a->created,
      );

      $result_b = db_select('oea_characters', 'c')->fields('c', array(
        'characterid',
        'charactername',
        'corporationid',
        'corporationname',
        'corporationticker',
        'allianceid',
        'alliancename',
        'allianceticker',
      ))->condition('drupalid', $uid, '=')->condition('deleted', 0, '=')->condition('apiid', $row_a->apiid, '=')->execute();

      if ($result_b->rowCount()) {
        foreach ($result_b->fetchAll() as $row_b) {
          $api_list[$row_a->apiid]['characters'][$row_b->characterid] = array(
            'characterid' => $row_b->characterid,
            'charactername' => $row_b->charactername,
            'corporationid' => $row_b->corporationid,
            'corporationname' => $row_b->corporationname,
            'corporationticker' => $row_b->corporationticker,
            'allianceid' => $row_b->allianceid,
            'alliancename' => $row_b->alliancename,
            'allianceticker' => $row_b->allianceticker,
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
        $corp_alliance = ($characters['allianceid'] == 0) ? $characters['corporationname'] : $characters['corporationname'] . '/' . $characters['alliancename'];
        $list_characters .= '[' . $characters['corporationticker'] . '] ' . $characters['charactername'] . ' (' . $corp_alliance . ')';
        $list_characters .= '<br />';
      }
    }

    $options[$api_single['apiid']] = array(
      'action' => array(
        'data' => array(
          '#type' => 'link',
          '#title' => t('Edit'),
          '#href' => 'user/' . $uid . '/omni_eve_api/update_api_key/' . $api_single['apiid'],
        ),
      ),
      'keyid' => $api_single['keyid'],
      'vcode' => $string = (strlen($api_single['vcode']) > 13) ? substr($api_single['vcode'], 0, 10) . '...' : $api_single['vcode'],
      'characters' => $list_characters,
      'updated' => format_date(strtotime($api_single['updated']), 'short'),
      'created' => format_date(strtotime($api_single['created']), 'short'),
      'errorid' => omni_eve_api_api_error_msg($api_single['errorid']),
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

  $form['list_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  foreach ($form_state['values']['api_keys'] as $api_key_id) {
    if ($api_key_id == 0) {
      continue;
    }

    db_delete('oea_api_keys')->condition('apiid', $api_key_id)->execute();
    db_update('oea_characters')->fields(array('deleted' => 1))->condition('apiid', $api_key_id)->execute();
  }

  $query = db_select('oea_characters', 'c');
  $query->condition('c.deleted', 0);
  $query->join('users', 'u', 'c.charactername = u.name');
  $query->fields('c', array(
    'charactername',
    'characterid',
  ));
  $query->condition('u.uid', $uid);
  $result = $query->execute();

  if (!$result->rowCount()) {
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
    user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);

    $unverified_role = user_role_load(variable_get('omni_eve_api_unverified_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $unverified_role->rid);

    try {
      omni_eve_api_jabber_connect();
      $result = db_select('ofUser', 'u')->fields('u', array(
        'username',
      ))->condition('username', omni_eve_api_jabber_format_name($account->name), '=')->execute();

      if ($result->rowCount()) {
        omni_eve_api_jabber_url_query('delete', $account->name);
      }

      omni_eve_api_jabber_disconnect();
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    db_update('users')->fields(array(
      'characterid' => 0,
    ))->condition('uid', $uid, '=')->execute();
    $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
    drupal_set_message(t('Please select a new Main Character'));
  }

  $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
  $queue->createQueue();
  $queue->createItem($uid);

  $row = db_select('users', 'u')->fields('u', array('teamspeakdbid'))->condition('uid', $uid, '=')->execute()->fetchAssoc();
  $queue = DrupalQueue::get('omni_eve_api_cron_teamspeak_user');
  $queue->createQueue();
  $queue->createItem(array('uid' => $uid, 'teamspeakdbid' => $row['teamspeakdbid']));

  drupal_set_message(t('API keys successfully deleted!'));
}

/**
 * Form constructor for the user update API form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 * @param string $api_key_id
 *   The unique api_key_id that is being updated.
 *
 * @see omni_eve_api_user_update_api_form_validate()
 * @see omni_eve_api_user_update_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_update_api_form($form, &$form_state, $account, $api_key_id) {
  $uid = $account->uid;

  $result = db_select('oea_api_keys', 'a')->fields('a', array(
    'keyid',
    'vcode',
  ))->condition('apiid', $api_key_id, '=')->execute();

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

  $form['update_api']['api_key_id'] = array(
    '#value' => $api_key_id,
    '#type' => 'hidden',
  );

  $form['update_api']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#default_value' => isset($form_state['values']['keyid']) ? $form_state['values']['keyid'] : $row['keyid'],
    '#weight' => 0,
  );

  $form['update_api']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#default_value' => isset($form_state['values']['vcode']) ? $form_state['values']['vcode'] : $row['vcode'],
    '#weight' => 10,
  );

  $form['update_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);
  $api_key_id = $form_state['values']['api_key_id'];

  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyid', t('There was an error with the API.'));
    form_set_error('vcode');
  }
  else {
    $form_state['values']['characters'] = $characters;

    $whitelist_characters = array();

    if (!empty($characters)) {
      foreach ($characters['characters'] as $character) {
        $whitelist_characters[] = $character['characterID'];
      }
    }

    $result = db_select('oea_whitelist', 'w')->fields('w', array('characterid'))->condition('characterid', $whitelist_characters, 'IN')->condition('deleted', 0, '=')->execute();
    if ($result->rowCount()) {
      if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
        form_set_error('keyid', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
        form_set_error('vcode');
      }
    }
    else {
      if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('omni_eve_api_access_mask', 268435455)) != variable_get('omni_eve_api_access_mask', 268435455)) {
        form_set_error('keyid', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
        form_set_error('vcode');
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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];
  $api_key_id = $form_state['values']['api_key_id'];
  $characters = $form_state['values']['characters'];

  $updated = date('Y-m-d H:i:s', time());

  db_update('oea_characters')->fields(array('deleted' => 1))->condition('apiid', $api_key_id)->execute();

  db_merge('oea_api_keys')->key(array('apiid' => $api_key_id))->fields(array(
    'apiid' => $api_key_id,
    'drupalid' => $uid,
    'keyid' => $keyid,
    'vcode' => $vcode,
    'updated' => $updated,
    'errorid' => 0,
  ))->execute();

  foreach ($characters['characters'] as $character) {
    db_merge('oea_characters')->key(array('characterid' => $character['characterID']))->fields(array(
      'characterid' => $character['characterID'],
      'drupalid' => $uid,
      'apiid' => $api_key_id,
      'charactername' => $character['characterName'],
      'corporationid' => $character['corporationID'],
      'corporationname' => $character['corporationName'],
      'corporationticker' => $character['corporationTicker'],
      'allianceid' => $character['allianceID'],
      'alliancename' => $character['allianceName'],
      'allianceticker' => $character['allianceTicker'],
      'errorid' => 0,
      'deleted' => 0,
    ))->execute();
  }

  $query = db_select('oea_characters', 'c');
  $query->condition('c.deleted', 0);
  $query->join('users', 'u', 'c.charactername = u.name');
  $query->fields('c', array(
    'charactername',
    'characterid',
  ));
  $query->condition('u.uid', $uid);
  $result = $query->execute();

  if (!$result->rowCount()) {
    try {
      omni_eve_api_jabber_connect();
      $result = db_select('ofUser', 'u')->fields('u', array(
        'username',
      ))->condition('username', omni_eve_api_jabber_format_name($account->name), '=')->execute();

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
      omni_eve_api_teamspeak_user_delete($ts3, $account->teamspeakdbid);
    }
    catch (OeaException $e) {
      // Log Error ?
    }

    db_update('users')->fields(array(
      'teamspeakuid' => 0,
      'teamspeakdbid' => 0,
      'characterid' => 0,
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
    '#markup' => ($account->characterid != 0) ? t('Teamspeak Name: @teamspeak', array('@teamspeak' => omni_eve_api_teamspeak_format_name($account->name))) : t('Teamspeak Name: <a href="@url">Please select a main character.</a>', array('@url' => url('user/' . $uid . '/omni_eve_api/select_main_character'))),
    '#suffix' => '</p>',
    '#weight' => 10,
  );

  $form['teamspeak']['teamspeak_uid'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => !empty($account->teamspeakuid) ? t('Teamspeak Unique ID: @teamspeak', array('@teamspeak' => $account->teamspeakuid)) : t('Teamspeak Unique ID: You are not registered.'),
    '#suffix' => '</p>',
    '#weight' => 20,
  );

  $form['teamspeak']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Register'),
  );

  if ($account->characterid == 0) {
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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  if ($account->characterid == 0) {
    drupal_goto('user/' . $uid . '/omni_eve_api/select_main_character');
  }

  try {
    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_name = omni_eve_api_teamspeak_format_name($account->name);
    $teamspeak_data = omni_eve_api_teamspeak_user_by_name($ts3, $teamspeak_name);
  }
  catch (OeaException $e) {
    form_set_error('teamspeak', $e->getMessage());
  }
}

/**
 * Form submission handler for omni_eve_api_user_teamspeak_form().
 *
 * @see omni_eve_api_user_teamspeak_form_validate()
 */
function omni_eve_api_user_teamspeak_form_submit($form, &$form_state) {
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  try {
    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_name = omni_eve_api_teamspeak_format_name($account->name);
    $teamspeak_data = omni_eve_api_teamspeak_user_by_name($ts3, $teamspeak_name);
    $teamspeakuid = $teamspeak_data->client_unique_identifier;
    $teamspeakdbid = $teamspeak_data->client_database_id;

    db_update('users')->fields(array(
        'teamspeakuid' => $teamspeakuid,
        'teamspeakdbid' => $teamspeakdbid,
      ))->condition('uid', $uid, '=')->execute();

    omni_eve_api_cron_teamspeak_user(array('uid' => $uid, 'teamspeakdbid' => $teamspeakdbid));

    // Need to assign roles.
    drupal_set_message(t('Teamspeak access granted.'));
  }
  catch (OeaException $e) {
    drupal_set_message($e->getMessage(), 'error');
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
    '#markup' => ($account->characterid != 0) ? t('Jabber Username: @jabber', array('@jabber' => omni_eve_api_jabber_format_name($account->name) . '@' . variable_get('omni_eve_api_jabber_domain', 'yourdomain.com'))) : t('Jabber Username: <a href="@url">Please select a main character.</a>', array('@url' => url('user/' . $uid . '/omni_eve_api/select_main_character'))),
    '#suffix' => '</p>',
    '#weight' => 10,
  );

  if ($account->characterid != 0) {
    $form['jabber']['jabber_password'] = array(
      '#type' => 'textfield',
      '#title' => t('Set Password'),
      '#description' => t('You can set or change your Jabber password.'),
      '#maxlength' => 50,
      '#weight' => 20,
      '#default_value' => '',
    );
  }

  $form['jabber']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Register'),
  );

  if ($account->characterid == 0) {
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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  if ($account->characterid == 0) {
    drupal_goto('user/' . $uid . '/omni_eve_api/select_main_character');
  }

  try {
    omni_eve_api_jabber_connect();

    $result = db_select('ofUser', 'u')->fields('u', array(
      'username',
    ))->condition('username', omni_eve_api_jabber_format_name($account->name), '=')->execute();

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
  $uid = $form_state['values']['uid'];
  $account = user_load($uid);

  $roles = user_roles();
  $user_roles = $all_roles = array();

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

    $result = db_select('ofUser', 'u')->fields('u', array(
      'username',
    ))->condition('username', omni_eve_api_jabber_format_name($account->name), '=')->execute();

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
