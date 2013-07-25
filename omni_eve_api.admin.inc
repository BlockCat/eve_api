<?php

/**
 * Implements hook_admin().
 */
function omni_eve_api_admin_roles()
{
  $form = array();

  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Roles'),
    '#weight' => 10,
  );

  $form['roles']['omni_eve_api_blue_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Blues'),
    '#description' => t('Please select the default role for blues.'),
    '#options' => _omni_eve_api_role_list(),
    '#default_value' => variable_get('omni_eve_api_blue_role'),
  );

  $form['roles']['omni_eve_api_alliance_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Alliance Members'),
    '#description' => t('Please select an additional role assigned to all members of the Alliance.'),
    '#options' => _omni_eve_api_role_list(),
    '#default_value' => variable_get('omni_eve_api_alliance_role'),
  );

  return system_settings_form($form);
}

/**
 * Implements hook_admin_validate().
 */
function omni_eve_api_admin_roles_validate($form, &$form_state)
{
  if (user_role_load((int)$form_state['values']['omni_eve_api_blue_role']) == TRUE)
  {
    $uids = array();
    $result = db_select('users_roles', 'r') -> fields('r', array('uid', )) -> condition('rid', (int)$form_state['values']['omni_eve_api_blue_role'], '=') -> execute();

    if ($result -> rowCount())
    {
      foreach ($result->fetchAll() as $row)
      {
        $uids[$row -> uid] = $row -> uid;
      }
    }

    if (!empty($uids))
    {
      user_multiple_role_edit($uids, 'remove_role', variable_get('omni_eve_api_blue_role'));
      user_multiple_role_edit($uids, 'add_role', (int)$form_state['values']['omni_eve_api_blue_role']);
    }

    variable_set('omni_eve_api_blue_role', (int)$form_state['values']['omni_eve_api_blue_role']);
  }
  else
  {
    form_set_error('omni_eve_api_blue_role', 'The role does not exist.');
  }

  if (user_role_load((int)$form_state['values']['omni_eve_api_alliance_role']) == TRUE)
  {
    $uids = array();
    $result = db_select('users_roles', 'r') -> fields('r', array('uid', )) -> condition('rid', (int)$form_state['values']['omni_eve_api_alliance_role'], '=') -> execute();

    if ($result -> rowCount())
    {
      foreach ($result->fetchAll() as $row)
      {
        $uids[$row -> uid] = $row -> uid;
      }
    }

    if (!empty($uids))
    {
      user_multiple_role_edit($uids, 'remove_role', variable_get('omni_eve_api_alliance_role'));
      user_multiple_role_edit($uids, 'add_role', (int)$form_state['values']['omni_eve_api_alliance_role']);
    }

    variable_set('omni_eve_api_alliance_role', (int)$form_state['values']['omni_eve_api_alliance_role']);
  }
  else
  {
    form_set_error('omni_eve_api_alliance_role', 'The role does not exist.');
  }
}

/**
 * Implements hook_admin_validate().
 */
function omni_eve_api_admin_roles_submit($form, &$form_state)
{
  if (user_role_load((int)$form_state['values']['omni_eve_api_blue_role']) == TRUE)
  {
    $uids = array();
    $result = db_select('users_roles', 'r') -> fields('r', array('uid', )) -> condition('rid', (int)$form_state['values']['omni_eve_api_blue_role'], '=') -> execute();

    if ($result -> rowCount())
    {
      foreach ($result->fetchAll() as $row)
      {
        $uids[$row -> uid] = $row -> uid;
      }
    }

    if (!empty($uids))
    {
      user_multiple_role_edit($uids, 'remove_role', variable_get('omni_eve_api_blue_role'));
      user_multiple_role_edit($uids, 'add_role', (int)$form_state['values']['omni_eve_api_blue_role']);
    }

    variable_set('omni_eve_api_blue_role', (int)$form_state['values']['omni_eve_api_blue_role']);
  }
  else
  {
    form_set_error('omni_eve_api_blue_role', 'The role does not exist.');
  }

  if (user_role_load((int)$form_state['values']['omni_eve_api_alliance_role']) == TRUE)
  {
    $uids = array();
    $result = db_select('users_roles', 'r') -> fields('r', array('uid', )) -> condition('rid', (int)$form_state['values']['omni_eve_api_alliance_role'], '=') -> execute();

    if ($result -> rowCount())
    {
      foreach ($result->fetchAll() as $row)
      {
        $uids[$row -> uid] = $row -> uid;
      }
    }

    if (!empty($uids))
    {
      user_multiple_role_edit($uids, 'remove_role', variable_get('omni_eve_api_alliance_role'));
      user_multiple_role_edit($uids, 'add_role', (int)$form_state['values']['omni_eve_api_alliance_role']);
    }

    variable_set('omni_eve_api_alliance_role', (int)$form_state['values']['omni_eve_api_alliance_role']);
  }
  else
  {
    form_set_error('omni_eve_api_alliance_role', 'The role does not exist.');
  }
}

/**
 * Implements hook_admin().
 */
function omni_eve_api_admin_alliance_api_key()
{
  $form = array();

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

  return system_settings_form($form);
}

/**
 * Implements hook_admin_validate().
 */
function omni_eve_api_admin_alliance_api_key_validate($form, &$form_state)
{
  $query = array(
    'keyid' => $form_state['values']['omni_eve_api_corp_keyid'],
    'vcode' => $form_state['values']['omni_eve_api_corp_vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if ($characters['expires'] || $characters['type'] != 'Corporation' || $characters['accessMask'] != 67108863)
  {
    form_set_error('omni_eve_api_corp_keyid', 'Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to your Alliance Executor or Corporation CEO, the "Type" drop down box is set to "Corporation", and that the "No Expiry" checkbox is ticked.');
    form_set_error('omni_eve_api_corp_vcode');
  }
  else
  {
    foreach ($characters['characters'] as $character)
    {
      variable_set('omni_eve_api_allianceID', (int)$character['allianceID']);
      variable_set('omni_eve_api_corporationID', (int)$character['corporationID']);
    }

    omni_eve_api_get_standings();
    omni_eve_api_get_alliance_corporations();
  }
}

/**
 * Implements hook_admin().
 */
function omni_eve_api_admin_access_mask()
{
  $form = array();

  $header = array(
    'mask' => array(
      'data' => t('Mask'),
      'field' => 'n.mask'
    ),
    'name' => array(
      'data' => t('Name'),
      'field' => 'n.name'
    ),
    'description' => array(
      'data' => t('Description'),
      'field' => 'n.description'
    ),
  );

  $options = $selected = array();

  $access_mask_list = _omni_eve_api_access_mask_list();

  $access_mask = variable_get('omni_eve_api_access_mask');

  foreach ($access_mask_list as $access_bit)
  {
    $options[$access_bit['mask']] = array(
      'mask' => $access_bit['mask'],
      'name' => $access_bit['name'],
      'description' => $access_bit['description'],
    );

    if (($access_mask & $access_bit['mask']) == true)
    {
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
    '#weight' => 1,
  );

  return system_settings_form($form);
}

/**
 * Implements hook_admin_validate().
 */
function omni_eve_api_admin_access_mask_validate($form, &$form_state)
{
  $new_access_mask = 0;

  foreach ($form_state['values']['access_mask'] as $access_bit)
  {
    $new_access_mask = $new_access_mask + $access_bit;
  }

  variable_set('omni_eve_api_access_mask', $new_access_mask);
}

/**
 * Custom function to get list of all Roles
 */
function _omni_eve_api_role_list()
{
  $roles = array();

  $result = db_select('role', 'r') -> fields('r', array(
    'rid',
    'name',
  )) -> execute();

  if ($result -> rowCount())
  {
    foreach ($result->fetchAll() as $row)
    {
      if ($row -> rid != 1)
      {
        $roles[$row -> rid] = check_plain($row -> name);
      }
    }
  }

  return $roles;
}

/**
 * Custom function to get list of all Access Masks
 */
function _omni_eve_api_access_mask_list()
{
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
function omni_eve_api_user_role_delete($role)
{
  if ($role -> rid == variable_get('omni_eve_api_blue_role'))
  {
    variable_set('omni_eve_api_blue_role', '2');
  }
}

/*
 * Need to implement function to display form for adding a custom standing
 */

/*
 * Need to implement function to display all custom standings and select them for deletion
 */

/*
 * Need to implement function to display form for adding a custom corporation to the alliance
 */

/*
 * Need to implement function to display all custom corporations and select them for deletion
 */

/**
 * Add function to add a character name to a bypass list to whitelist an API
 */
