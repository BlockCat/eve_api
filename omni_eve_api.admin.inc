<?php
/**
 * @file
 * Functions related to allow admin's to alter the settings and options.
 */

/**
 * Form constructor for the admin edit roles form.
 *
 * @see omni_eve_api_admin_settings_form_validate()
 * @see omni_eve_api_admin_settings_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_settings_form($form, &$form_state) {
  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Roles'),
    '#weight' => 0,
  );

  $form['roles']['omni_eve_api_blue_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Blues'),
    '#description' => t('Please select the default role for blues.'),
    '#options' => omni_eve_api_role_list(),
    '#default_value' => variable_get('omni_eve_api_blue_role'),
    '#weight' => 0,
  );

  $form['roles']['omni_eve_api_alliance_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Alliance Members'),
    '#description' => t('Please select an additional role assigned to all members of the Alliance.'),
    '#options' => omni_eve_api_role_list(),
    '#default_value' => variable_get('omni_eve_api_alliance_role'),
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['#validate'] = array('omni_eve_api_admin_settings_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_settings_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_settings_form().
 *
 * @see omni_eve_api_admin_settings_form_submit()
 */
function omni_eve_api_admin_settings_form_validate($form, &$form_state) {
  if (user_role_load((int) $form_state['values']['omni_eve_api_blue_role']) == FALSE) {
    form_set_error('omni_eve_api_blue_role', t('The role does not exist.'));
  }

  if (user_role_load((int) $form_state['values']['omni_eve_api_alliance_role']) == FALSE) {
    form_set_error('omni_eve_api_alliance_role', t('The role does not exist.'));
  }
}

/**
 * Form submission handler for omni_eve_api_admin_settings_form().
 *
 * @see omni_eve_api_admin_settings_form_validate()
 */
function omni_eve_api_admin_settings_form_submit($form, &$form_state) {
  // $uids = array();
  //  $result = db_select('users_roles', 'r') -> fields('r', array('uid', )) ->
  //  condition('rid', (int) variable_get('omni_eve_api_blue_role'), '=') ->
  //  execute();
  //  if ($result -> rowCount()) {
  //  foreach ($result->fetchAll() as $row) {
  //  $uids[] = $row -> uid;
  //  }
  //  }
  //  if (!empty($uids)) {
  //  user_multiple_role_edit($uids, 'remove_role',
  //  (int) variable_get('omni_eve_api_blue_role'));
  //  user_multiple_role_edit($uids, 'add_role',
  //  (int) $form_state['values']['omni_eve_api_blue_role']);
  //  }
  //  $uids = array();
  //  $result = db_select('users_roles', 'r') -> fields('r', array('uid', )) ->
  //  condition('rid', (int) variable_get('omni_eve_api_alliance_role'), '=') ->
  //  execute();
  //  if ($result -> rowCount()) {
  //  foreach ($result->fetchAll() as $row) {
  //  $uids[] = $row -> uid;
  //  }
  //  }
  //  if (!empty($uids)) {
  //  user_multiple_role_edit($uids, 'remove_role',
  //  (int) variable_get('omni_eve_api_alliance_role'));
  //  user_multiple_role_edit($uids, 'add_role',
  //  (int) $form_state['values']['omni_eve_api_alliance_role']);
  //  }
  variable_set('omni_eve_api_blue_role', (int) $form_state['values']['omni_eve_api_blue_role']);
  variable_set('omni_eve_api_alliance_role', (int) $form_state['values']['omni_eve_api_alliance_role']);
}

/**
 * Form constructor for editing the alliance API Key.
 *
 * @see omni_eve_api_admin_alliance_api_key_form_validate()
 * @see omni_eve_api_admin_alliance_api_key_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_alliance_api_key_form($form, &$form_state) {
  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Enter API Info'),
    '#description' => t('Enter the Alliance Executers or Corperations CEO\'s API.'),
    '#weight' => 0,
  );

  $form['enter_api']['omni_eve_api_corp_keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_corp_keyid'),
  );

  $form['enter_api']['omni_eve_api_corp_vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
    '#default_value' => variable_get('omni_eve_api_corp_vcode'),
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  $form['#validate'] = array('omni_eve_api_admin_alliance_api_key_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_alliance_api_key_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_alliance_api_key_form().
 *
 * @see omni_eve_api_admin_alliance_api_key_form_submit()
 */
function omni_eve_api_admin_alliance_api_key_form_validate($form, &$form_state) {
  $query = array(
    'keyid' => $form_state['values']['omni_eve_api_corp_keyid'],
    'vcode' => $form_state['values']['omni_eve_api_corp_vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if ($characters['expires'] || $characters['type'] != 'Corporation' || $characters['accessMask'] != 67108863) {
    form_set_error('omni_eve_api_corp_keyid', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to your Alliance Executor or Corporation CEO, the "Type" drop down box is set to "Corporation", and that the "No Expiry" checkbox is ticked.'));
    form_set_error('omni_eve_api_corp_vcode');
  }

  $form_state['values']['characters'] = $characters;
}

/**
 * Form submission handler for omni_eve_api_admin_alliance_api_key_form().
 *
 * @see omni_eve_api_admin_alliance_api_key_form_validate()
 */
function omni_eve_api_admin_alliance_api_key_form_submit($form, &$form_state) {
  $characters = $form_state['values']['characters'];

  variable_set('omni_eve_api_corp_keyid', (int) $form_state['values']['omni_eve_api_corp_keyid']);
  variable_set('omni_eve_api_corp_vcode', $form_state['values']['omni_eve_api_corp_vcode']);

  // It's not pretty but it works.
  foreach ($characters['characters'] as $character) {
    variable_set('omni_eve_api_allianceID', (int) $character['allianceID']);
    variable_set('omni_eve_api_corporationID', (int) $character['corporationID']);
  }

  omni_eve_api_get_standings();
  omni_eve_api_get_alliance_corporations();
}

/**
 * Form constructor for the altering the access mask form.
 *
 * @see omni_eve_api_admin_access_mask_form_validate()
 * @see omni_eve_api_admin_access_mask_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_access_mask_form($form, &$form_state) {
  $header = array(
    'mask' => array(
      'data' => t('Mask'),
      'field' => 'n.mask',
    ),
    'name' => array(
      'data' => t('Name'),
      'field' => 'n.name',
    ),
    'description' => array(
      'data' => t('Description'),
      'field' => 'n.description',
    ),
  );

  $options = $selected = array();

  $access_mask_list = omni_eve_api_access_mask_list();

  $access_mask = variable_get('omni_eve_api_access_mask');

  foreach ($access_mask_list as $access_bit) {
    $options[$access_bit['mask']] = array(
      'mask' => $access_bit['mask'],
      'name' => $access_bit['name'],
      'description' => $access_bit['description'],
    );

    if (($access_mask & $access_bit['mask']) == TRUE) {
      $selected[$access_bit['mask']] = TRUE;
    }
  }

  $form['access_mask'] = array(
    '#type' => 'fieldset',
    '#title' => t('Edit Access Mask'),
    '#description' => t('Edit your Access Mask that is required to register on the site.'),
    '#weight' => 0,
  );

  $form['access_mask']['current_mask'] = array(
    '#type' => 'markup',
    '#markup' => t("Current Mask: @access_mask", array('@access_mask' => $access_mask)),
    '#weight' => 0,
  );

  $form['access_mask']['access_mask'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Mask'),
  );

  $form['#validate'] = array('omni_eve_api_admin_access_mask_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_access_mask_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_access_mask_form().
 *
 * @see omni_eve_api_admin_access_mask_form_submit()
 */
function omni_eve_api_admin_access_mask_form_validate($form, &$form_state) {
}

/**
 * Form submission handler for omni_eve_api_admin_access_mask_form().
 *
 * @see omni_eve_api_admin_access_mask_form_validate()
 */
function omni_eve_api_admin_access_mask_form_submit($form, &$form_state) {
  $new_access_mask = 0;

  foreach ($form_state['values']['access_mask'] as $access_bit) {
    $new_access_mask = $new_access_mask + $access_bit;
  }

  variable_set('omni_eve_api_access_mask', $new_access_mask);
}

/**
 * Custom function to get list of all Roles.
 *
 * @return array
 *   A simple array of All the Roles.
 */
function omni_eve_api_role_list() {
  $roles = array();

  $result = db_select('role', 'r')->fields('r', array(
    'rid',
    'name',
  ))->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      if ($row->rid != 1) {
        $roles[$row->rid] = check_plain($row->name);
      }
    }
  }

  return $roles;
}

/**
 * Function to get a list of all Access Masks and descriptions.
 *
 * @return array
 *   A simple array of Access Mask ID, Name, and Description.
 */
function omni_eve_api_access_mask_list() {
  $access_mask = array();

  $access_mask[] = array(
    'mask' => 1,
    'name' => 'AccountBalance',
    'description' => 'Current balance of characters wallet.',
  );
  $access_mask[] = array(
    'mask' => 2,
    'name' => 'AssetList',
    'description' => 'Entire asset list of character.',
  );
  $access_mask[] = array(
    'mask' => 4,
    'name' => 'CalendarEventAttendees',
    'description' => 'Event attendee responses. Requires UpcomingCalendarEvents to function.',
  );
  $access_mask[] = array(
    'mask' => 8,
    'name' => 'CharacterSheet',
    'description' => 'Character Sheet information. Contains basic "Show Info" information along with clones, account balance, implants, attributes, skills, certificates and corporation roles.',
  );
  $access_mask[] = array(
    'mask' => 16,
    'name' => 'ContactList',
    'description' => 'List of character contacts and relationship levels.',
  );
  $access_mask[] = array(
    'mask' => 32,
    'name' => 'ContactNotifications',
    'description' => 'Most recent contact notifications for the character.',
  );
  $access_mask[] = array(
    'mask' => 64,
    'name' => 'FacWarStats',
    'description' => 'Characters Factional Warfare Statistics.',
  );
  $access_mask[] = array(
    'mask' => 128,
    'name' => 'IndustryJobs',
    'description' => 'Character jobs, completed and active.',
  );
  $access_mask[] = array(
    'mask' => 256,
    'name' => 'KillMails',
    'description' => 'Character\'s killmails.',
  );
  $access_mask[] = array(
    'mask' => 512,
    'name' => 'MailBodies',
    'description' => 'EVE Mail bodies. Requires MailMessages as well to function.',
  );
  $access_mask[] = array(
    'mask' => 1024,
    'name' => 'MailingLists',
    'description' => 'List of all Mailing Lists the character subscribes to.',
  );
  $access_mask[] = array(
    'mask' => 2048,
    'name' => 'MailMessages',
    'description' => 'List of all messages in the characters EVE Mail Inbox.',
  );
  $access_mask[] = array(
    'mask' => 4096,
    'name' => 'MarketOrders',
    'description' => 'List of all Market Orders the character has made.',
  );
  $access_mask[] = array(
    'mask' => 8192,
    'name' => 'Medals',
    'description' => 'Medals awarded to the character.',
  );
  $access_mask[] = array(
    'mask' => 16384,
    'name' => 'Notifications',
    'description' => 'List of recent notifications sent to the character.',
  );
  $access_mask[] = array(
    'mask' => 32768,
    'name' => 'NotificationTexts',
    'description' => 'Actual body of notifications sent to the character. Requires Notification access to function.',
  );
  $access_mask[] = array(
    'mask' => 65536,
    'name' => 'Research',
    'description' => 'List of all Research agents working for the character and the progress of the research.',
  );
  $access_mask[] = array(
    'mask' => 131072,
    'name' => 'SkillInTraining',
    'description' => 'Skill currently in training on the character. Subset of entire Skill Queue.',
  );
  $access_mask[] = array(
    'mask' => 262144,
    'name' => 'SkillQueue',
    'description' => 'Entire skill queue of character.',
  );
  $access_mask[] = array(
    'mask' => 524288,
    'name' => 'Standings',
    'description' => 'NPC Standings towards the character.',
  );
  $access_mask[] = array(
    'mask' => 1048576,
    'name' => 'UpcomingCalendarEvents',
    'description' => 'Upcoming events on characters calendar.',
  );
  $access_mask[] = array(
    'mask' => 2097152,
    'name' => 'WalletJournal',
    'description' => 'Wallet journal of character.',
  );
  $access_mask[] = array(
    'mask' => 4194304,
    'name' => 'WalletTransactions',
    'description' => 'Market transaction journal of character.',
  );
  $access_mask[] = array(
    'mask' => 8388608,
    'name' => 'CharacterInfoPublic',
    'description' => 'Character information, exposes skill points and current ship information on top of "Show Info" information.',
  );
  $access_mask[] = array(
    'mask' => 16777216,
    'name' => 'CharacterInfoPrivate',
    'description' => 'Sensitive Character Information, exposes account balance and last known location on top of the other Character Information call.',
  );
  $access_mask[] = array(
    'mask' => 33554432,
    'name' => 'AccountStatus',
    'description' => 'EVE player account status.',
  );
  $access_mask[] = array(
    'mask' => 67108864,
    'name' => 'Contracts',
    'description' => 'List of all Contracts the character is involved in.',
  );
  $access_mask[] = array(
    'mask' => 134217728,
    'name' => 'Locations',
    'description' => 'Allows the fetching of coordinate and name data for items owned by the character.',
  );

  return $access_mask;
}

/**
 * Implements hook_user_role_delete().
 */
function omni_eve_api_user_role_delete($role) {
  if ($role->rid == variable_get('omni_eve_api_blue_role')) {
    variable_set('omni_eve_api_blue_role', '2');
  }
}

/**
 * Form constructor for the admin list blues form.
 *
 * @see omni_eve_api_admin_list_blues_form_validate()
 * @see omni_eve_api_admin_list_blues_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_list_blues_form($form, &$form_state) {
  $header = array(
    'deleted' => array(
      'data' => t('Deleted'),
      'field' => 'n.deleted',
    ),
    'id' => array(
      'data' => t('ID'),
      'field' => 'n.id',
    ),
    'name' => array(
      'data' => t('Name'),
      'field' => 'n.name',
    ),
    'standing' => array(
      'data' => t('Standing'),
      'field' => 'n.standing',
    ),
    'manual' => array(
      'data' => t('Manually Entered'),
      'field' => 'n.manual',
    ),
  );

  $options = $selected = array();

  $result = db_select('oea_blue_standings', 'b')->fields('b', array(
    'contactID',
    'contactName',
    'standing',
    'deleted',
    'manual',
  ))->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->contactID] = array(
        'deleted' => ($row->deleted) ? 'Yes' : 'No',
        'id' => $row->contactID,
        'name' => $row->contactName,
        'standing' => $row->standing,
        'manual' => ($row->manual) ? 'Yes' : 'No',
      );

      if ($row->deleted) {
        $selected[$row->contactID] = TRUE;
      }
    }
  }

  $form['list_blues'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update Blues'),
    '#description' => t('Toggle the checkbox to remove/add an Existing Blue.'),
    '#weight' => 0,
  );

  $form['list_blues']['blues'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Blues'),
  );

  $form['#validate'] = array('omni_eve_api_admin_list_blues_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_list_blues_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_list_blues_form().
 *
 * @see omni_eve_api_admin_list_blues_form_submit()
 */
function omni_eve_api_admin_list_blues_form_validate($form, &$form_state) {
}

/**
 * Form submission handler for omni_eve_api_admin_list_blues_form().
 *
 * @see omni_eve_api_admin_list_blues_form_validate()
 */
function omni_eve_api_admin_list_blues_form_submit($form, &$form_state) {
  db_update('oea_blue_standings')->fields(array('deleted' => 0))->execute();
  $deleted_blues = array();
  foreach ($form_state['values']['blues'] as $blue_id) {
    $deleted_blues[] = $blue_id;
  }
  db_update('oea_blue_standings')->fields(array('deleted' => 1))->condition('contactID', $deleted_blues, 'IN')->execute();
}

/**
 * Form constructor for the admin add blues form.
 *
 * @see omni_eve_api_admin_add_blues_form_validate()
 * @see omni_eve_api_admin_add_blues_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_add_blues_form($form, &$form_state) {
  $form['add_blues'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add Blue'),
    '#description' => t('Add a custom blue immediately, this blue will remain permanately and updated with the next cron if found.'),
    '#weight' => 0,
  );

  $form['add_blues']['id'] = array(
    '#type' => 'textfield',
    '#title' => t('Character/Corporation/Alliance ID'),
    '#description' => t('Please enter the ID of the Character/Corporation/Alliance, if found, details will be retrieved from the API.'),
    '#required' => TRUE,
    '#maxlength' => 20,
    '#weight' => 0,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add Blue'),
  );

  $form_state['redirect'] = 'admin/settings/omni_eve_api/list_blues';
  $form['#validate'] = array('omni_eve_api_admin_add_blues_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_add_blues_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_add_blues_form().
 *
 * @see omni_eve_api_admin_add_blues_form_submit()
 */
function omni_eve_api_admin_add_blues_form_validate($form, &$form_state) {
  $result = db_select('oea_blue_standings', 'b')->fields('b', array('contactID'))->condition('contactID', (int) $form_state['values']['id'], '=')->execute();

  if ($result->rowCount()) {
    form_set_error('id', t('Character/Corporation/Alliance already added to the standings.'));
  }

  $result = db_select('oea_alliance_corporations', 'a')->fields('a', array('corporationid'))->condition('deleted', 0, '=')->condition('corporationid', (int) $form_state['values']['id'], '=')->execute();

  if ($result->rowCount()) {
    form_set_error('id', t('Corporation already listed as an Alliance Corporation.'));
  }

  $query = array('corporationID' => (int) $form_state['values']['id']);
  $corp_info = omni_eve_api_get_corporation_info($query);

  $query = array('ids' => (int) $form_state['values']['id']);
  $char_info = omni_eve_api_get_character_name($query);

  $alliance_info = db_select('oea_alliance_info', 'a')->fields('a', array('alliancename'))->condition('allianceid', (int) $form_state['values']['id'], '=')->execute()->fetchAssoc();

  if (isset($corp_info)) {
    $form_state['values']['name'] = $corp_info['corporationName'];
  }
  elseif (isset($char_info)) {
    $form_state['values']['name'] = $char_info[(int) $form_state['values']['id']];
  }
  elseif ($alliance_info->rowCount()) {
    $form_state['values']['name'] = $row['alliancename'];
  }
  else {
    form_set_error('id', t('No match found.'));
  }
}

/**
 * Form submission handler for omni_eve_api_admin_add_blues_form().
 *
 * @see omni_eve_api_admin_add_blues_form_validate()
 */
function omni_eve_api_admin_add_blues_form_submit($form, &$form_state) {
  db_insert('oea_blue_standings')->fields(array(
    'contactID' => $form_state['values']['id'],
    'contactName' => $form_state['values']['name'],
    'standing' => 1,
    'manual' => 1,
  ))->execute();
}

/**
 * Form constructor for the admin list blues form.
 *
 * @see omni_eve_api_admin_list_whitelist_form_validate()
 * @see omni_eve_api_admin_list_whitelist_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_list_whitelist_form($form, &$form_state) {
  $header = array(
    'deleted' => array(
      'data' => t('Deleted'),
      'field' => 'n.deleted',
    ),
    'characterid' => array(
      'data' => t('Character ID'),
      'field' => 'n.characterid',
    ),
    'charactername' => array(
      'data' => t('Character Name'),
      'field' => 'n.charactername',
    ),
  );

  $options = $selected = array();

  $result = db_select('oea_whitelist', 'w')->fields('w', array(
    'characterid',
    'charactername',
    'deleted',
  ))->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->characterid] = array(
        'deleted' => ($row->deleted) ? 'Yes' : 'No',
        'characterid' => $row->characterid,
        'charactername' => $row->charactername,
      );

      if ($row->deleted) {
        $selected[$row->characterid] = TRUE;
      }
    }
  }

  $form['whitelist'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update Whitelist'),
    '#description' => t('Toggle the checkbox to remove/add an Whitelisted Character.'),
    '#weight' => 0,
  );

  $form['whitelist']['whitelist'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Whitelist'),
  );

  $form['#validate'] = array('omni_eve_api_admin_list_whitelist_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_list_whitelist_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_list_whitelist_form().
 *
 * @see omni_eve_api_admin_list_whitelist_form_submit()
 */
function omni_eve_api_admin_list_whitelist_form_validate($form, &$form_state) {
}

/**
 * Form submission handler for omni_eve_api_admin_list_whitelist_form().
 *
 * @see omni_eve_api_admin_list_whitelist_form_validate()
 */
function omni_eve_api_admin_list_whitelist_form_submit($form, &$form_state) {
  db_update('oea_whitelist')->fields(array('deleted' => 0))->execute();
  $deleted_whitelist = array();
  foreach ($form_state['values']['whitelist'] as $whitelist) {
    $deleted_whitelist[] = $whitelist;
  }
  db_update('oea_whitelist')->fields(array('deleted' => 1))->condition('characterid', $deleted_whitelist, 'IN')->execute();
}

/**
 * Form constructor for the admin add blues form.
 *
 * @see omni_eve_api_admin_add_whitelist_form_validate()
 * @see omni_eve_api_admin_add_whitelist_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_add_whitelist_form($form, &$form_state) {
  $form['add_whitelist'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add Whitelist'),
    '#description' => t('Add characters to the whitelist, the key is not stored here, it is used to retrieve the characters on the API Key and add them to the whitelist.  There are times when you will have high profile members join your site, they may have sensitive e-mails or items in locations.  With this whitelist you are able to allow the user to register with a very limited and basic API Key, but still provide full functionality on the site.  Please provide the following <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=8388680">link</a> to have the new user create a new limited API Key, the user has the option to list all characters, or just a single character.'),
    '#weight' => 0,
  );

  $form['add_whitelist']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter the Key ID.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['add_whitelist']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter the Verification Code.'),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add Whitelist'),
  );

  $form_state['redirect'] = 'admin/settings/omni_eve_api/list_whitelist';
  $form['#validate'] = array('omni_eve_api_admin_add_whitelist_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_add_whitelist_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_add_whitelist_form().
 *
 * @see omni_eve_api_admin_add_whitelist_form_submit()
 */
function omni_eve_api_admin_add_whitelist_form_validate($form, &$form_state) {
  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if ($characters['expires'] || $characters['accessMask'] != 8388680) {
    form_set_error('keyid', t('Please ensure that only the following boxes are highlighted and selected for the API: (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet))
<br />
In addition the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
    form_set_error('vcode');
  }

  if (!omni_eve_api_verify_blue($characters)) {
    form_set_error('keyid', t('No characters associated with the key are currently blue, please add the characters to the blue standings.'));
    form_set_error('vcode');
  }
}

/**
 * Form submission handler for omni_eve_api_admin_add_whitelist_form().
 *
 * @see omni_eve_api_admin_add_whitelist_form_validate()
 */
function omni_eve_api_admin_add_whitelist_form_submit($form, &$form_state) {
  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  foreach ($characters['characters'] as $character) {
    db_merge('oea_whitelist')->key(array('characterid' => $character['characterID']))->fields(array(
      'characterid' => $character['characterID'],
      'charactername' => $character['characterName'],
      'deleted' => 0,
    ))->execute();
  }

  drupal_set_message(t('Whitelisted Characters successfully added!'));
}
