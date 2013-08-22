<?php
/**
 * @file
 * Functions related to allow admin's to alter the settings and options.
 */

/**
 * Form constructor for editing the settings.
 *
 * @see eve_api_admin_settings_form_validate()
 * @see eve_api_admin_settings_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_settings_form($form, &$form_state) {
  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Enter API Key'),
    '#description' => t('Enter the Alliance Executers or Corperations CEO API Key.'),
    '#weight' => 0,
    '#collapsed' => variable_get('eve_api_enable', FALSE),
    '#collapsible' => TRUE,
  );

  $form['enter_api']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#size' => 20,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => variable_get('eve_api_corp_keyid', ''),
  );

  $form['enter_api']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#size' => 80,
    '#maxlength' => 64,
    '#weight' => 10,
    '#default_value' => variable_get('eve_api_corp_vcode', ''),
  );

  $form['enter_api']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable EVE API'),
    '#default_value' => variable_get('eve_api_enable', FALSE),
    '#weight' => 20,
  );

  if (variable_get('eve_api_first_run', FALSE) && variable_get('eve_api_enable', FALSE)) {
    $form['enter_api']['force'] = array(
      '#type' => 'checkbox',
      '#title' => t('Force Update API Information via a cron task.'),
      '#default_value' => FALSE,
      '#weight' => 30,
    );

    $form['settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Settings'),
      '#description' => t('General configurable settings.'),
      '#weight' => 10,
      '#collapsed' => !variable_get('eve_api_enable', FALSE),
      '#collapsible' => TRUE,
    );

    $form['settings']['enable_cron'] = array(
      '#type' => 'radios',
      '#title' => t('Enable EVE API Cron'),
      '#description' => t('If you are experience troubles, or updateing the module, you can disable the cron related to EVE API.'),
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#weight' => 0,
      '#default_value' => (int) variable_get('eve_api_enable_cron', TRUE),
    );

    $form['settings']['nag_user'] = array(
      '#type' => 'radios',
      '#title' => t('Nag User to Select Character'),
      '#description' => t('Display a message on every page telling the user to select a main character if they have not selected a main character.'),
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#weight' => 0,
      '#default_value' => (int) variable_get('eve_api_nag_user', TRUE),
    );
  }
  else {
    $form['enter_api']['force'] = array(
      '#type' => 'hidden',
      '#value' => FALSE,
    );

    $form['enter_api']['enable_cron'] = array(
      '#type' => 'hidden',
      '#value' => TRUE,
    );

    $form['enter_api']['nag_user'] = array(
      '#type' => 'hidden',
      '#value' => TRUE,
    );
  }

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
    '#weight' => 20,
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['enter_api']['enable']['#default_value'] = TRUE;
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_admin_settings_form_validate');
  $form['#submit'] = array('eve_api_admin_settings_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_settings_form().
 *
 * @see eve_api_admin_settings_form_submit()
 */
function eve_api_admin_settings_form_validate($form, &$form_state) {
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

  $characters = eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyID', t('There was an error with the API.'));
    form_set_error('vCode');
    variable_set('eve_api_enable', FALSE);
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Corporation' || $characters['accessMask'] != 67108863) {
      form_set_error('keyID', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to your Alliance Executor or Corporation CEO, the "Type" drop down box is set to "Corporation", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('vCode');
      variable_set('eve_api_enable', FALSE);
    }

    $form_state['values']['characters'] = $characters;
  }
}

/**
 * Form submission handler for eve_api_admin_settings_form().
 *
 * @see eve_api_admin_settings_form_validate()
 */
function eve_api_admin_settings_form_submit($form, &$form_state) {
  $characters = (array) $form_state['values']['characters'];

  variable_set('eve_api_corp_keyid', (int) $form_state['values']['keyID']);
  variable_set('eve_api_corp_vcode', (string) $form_state['values']['vCode']);
  variable_set('eve_api_enable_cron', (bool) $form_state['values']['enable_cron']);
  variable_set('eve_api_nag_user', (bool) $form_state['values']['nag_user']);

  // It's not pretty but it works.
  foreach ($characters['characters'] as $character) {
    variable_set('eve_api_allianceID', (int) $character['allianceID']);
    variable_set('eve_api_corporationID', (int) $character['corporationID']);
  }

  if (((bool) $form_state['values']['enable'] && (bool) $form_state['values']['force']) || ((bool) $form_state['values']['enable'] && !variable_get('eve_api_enable', FALSE))) {
    variable_set('eve_api_first_run', TRUE);

    $queue = DrupalQueue::get('eve_api_cron_api_alliance_fetch');
    $queue->createItem(1);

    $queue = DrupalQueue::get('eve_api_cron_api_alliance_sync');
    $queue->createItem(1);

    $queue = DrupalQueue::get('eve_api_cron_api_mask_sync');
    $queue->createItem(1);

    drupal_set_message(t('The Alliance API Info is set to be retrieved on the next cron job, it can take up to a minute for the cron task to be triggered.'));
  }
  elseif ((bool) $form_state['values']['enable']) {
    drupal_set_message(t('Settings Updated.'));
  }
  else {
    variable_set('eve_api_first_run', FALSE);
    variable_set('eve_api_enable', FALSE);
    drupal_set_message(t('Registrations have been disabled, EVE API cron tasks have been disabled, and all user menus have been disabled.'));
  }

  menu_rebuild();
}

/**
 * Form constructor for the admin edit roles form.
 *
 * @see eve_api_admin_roles_form_validate()
 * @see eve_api_admin_roles_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_roles_form($form, &$form_state) {
  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Roles'),
    '#weight' => 0,
  );

  $role_list = eve_api_role_list();

  $form['roles']['blue_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Blues'),
    '#description' => t('Please select the default role for blues.'),
    '#options' => $role_list,
    '#default_value' => variable_get('eve_api_blue_role', 2),
    '#weight' => 0,
  );

  $form['roles']['alliance_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Alliance Members'),
    '#description' => t('Please select an additional role assigned to all members of the Alliance.'),
    '#options' => $role_list,
    '#default_value' => variable_get('eve_api_alliance_role', 2),
    '#weight' => 10,
  );

  $form['roles']['unverified_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Unverified'),
    '#description' => t('Please select the default role for Unverified members.'),
    '#options' => $role_list,
    '#default_value' => variable_get('eve_api_unverified_role', 2),
    '#weight' => 20,
  );

  $form['roles']['ceo_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for CEOs'),
    '#description' => t('Please select the default role for CEOs.'),
    '#options' => $role_list,
    '#default_value' => variable_get('eve_api_ceo_role', 2),
    '#weight' => 30,
  );

  $form['roles']['director_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Directors'),
    '#description' => t('Please select the default role for directors.'),
    '#options' => $role_list,
    '#default_value' => variable_get('eve_api_director_role', 2),
    '#weight' => 40,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_admin_roles_form_validate');
  $form['#submit'] = array('eve_api_admin_roles_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_roles_form().
 *
 * @see eve_api_admin_roles_form_submit()
 */
function eve_api_admin_roles_form_validate($form, &$form_state) {
  if (user_role_load((int) $form_state['values']['blue_role']) == FALSE) {
    form_set_error('blue_role', t('The role does not exist.'));
  }

  if (user_role_load((int) $form_state['values']['alliance_role']) == FALSE) {
    form_set_error('alliance_role', t('The role does not exist.'));
  }

  if (user_role_load((int) $form_state['values']['unverified_role']) == FALSE) {
    form_set_error('unverified_role', t('The role does not exist.'));
  }

  if (user_role_load((int) $form_state['values']['ceo_role']) == FALSE) {
    form_set_error('ceo_role', t('The role does not exist.'));
  }

  if (user_role_load((int) $form_state['values']['director_role']) == FALSE) {
    form_set_error('director_role', t('The role does not exist.'));
  }
}

/**
 * Form submission handler for eve_api_admin_roles_form().
 *
 * @see eve_api_admin_roles_form_validate()
 */
function eve_api_admin_roles_form_submit($form, &$form_state) {
  if ((int) variable_get('eve_api_blue_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('eve_api_blue_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('eve_api_blue_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['blue_role']);
    }
  }

  if ((int) variable_get('eve_api_alliance_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('eve_api_alliance_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('eve_api_alliance_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['alliance_role']);
    }
  }

  if ((int) variable_get('eve_api_unverified_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('eve_api_unverified_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('eve_api_unverified_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['unverified_role']);
    }
  }

  if ((int) variable_get('eve_api_ceo_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('eve_api_ceo_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('eve_api_ceo_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['ceo_role']);
    }
  }

  if ((int) variable_get('eve_api_director_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('eve_api_director_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('eve_api_director_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['director_role']);
    }
  }

  variable_set('eve_api_blue_role', (int) $form_state['values']['blue_role']);
  variable_set('eve_api_alliance_role', (int) $form_state['values']['alliance_role']);
  variable_set('eve_api_unverified_role', (int) $form_state['values']['unverified_role']);
  variable_set('eve_api_ceo_role', (int) $form_state['values']['ceo_role']);
  variable_set('eve_api_director_role', (int) $form_state['values']['director_role']);

  drupal_set_message(t('The roles have been updated and will be updated with the next scheduled cron tasks.'));
}

/**
 * Form constructor for the altering the access mask form.
 *
 * @see eve_api_admin_registration_form_validate()
 * @see eve_api_admin_registration_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_registration_form($form, &$form_state) {
  $header = array(
    'accessmask' => array(
      'data' => t('Mask'),
      'field' => 'a.accessmask',
    ),
    'name' => array(
      'data' => t('Name'),
      'field' => 'a.name',
    ),
    'description' => array(
      'data' => t('Description'),
      'field' => 'a.description',
    ),
  );

  $options = $selected = array();

  $access_mask = variable_get('eve_api_access_mask', 268435455);

  $result = db_select('eve_api_access_mask', 'a')
    ->fields('a', array(
      'accessmask',
      'type',
      'name',
      'description',
    ))
    ->condition('type', 'Character', '=')
    ->extend('PagerDefault')
    ->limit(30)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->accessmask] = array(
        'accessmask' => check_plain((int) $row->accessmask),
        'name' => check_plain($row->name),
        'description' => check_plain($row->description),
      );

      if (($access_mask & (int) $row->accessmask) == TRUE) {
        $selected[$row->accessmask] = TRUE;
      }
    }
  }

  $form['registration'] = array(
    '#type' => 'fieldset',
    '#title' => t('Modify Registration Settings'),
    '#description' => t('Edit your required user and active registration settings.'),
    '#weight' => 0,
    '#collapsed' => FALSE,
    '#collapsible' => TRUE,
  );

  $form['registration']['standing'] = array(
    '#type' => 'textfield',
    '#title' => t('Standings'),
    '#description' => t('Set the required standing to register on the site. Standing required will be equal to or greater than the value supplied.  Accepted values are 10.0 to -10.0'),
    '#maxlength' => 5,
    '#size' => 10,
    '#weight' => 10,
    '#required' => TRUE,
    '#default_value' => round((float) variable_get('eve_api_required_standing', 0.1), 1),
  );

  $form['registration']['expires'] = array(
    '#type' => 'radios',
    '#title' => t('Allow Expired Keys'),
    '#description' => t('API keys can be set to expire at a certain date and time. You can allow users to register with API Keys that will expire, but this may cause issues if the user forgets that the API Key has/will expire.'),
    '#options' => array(0 => t('No'), 1 => t('Yes')),
    '#weight' => 20,
    '#default_value' => (int) variable_get('eve_api_require_expires', FALSE),
  );

  $form['registration']['type'] = array(
    '#type' => 'radios',
    '#title' => t('Require All Characters'),
    '#description' => t('The API Key can list all characters on the account, or only show a single character.'),
    '#options' => array(0 => t('No'), 1 => t('Yes')),
    '#weight' => 30,
    '#default_value' => (int) variable_get('eve_api_require_type', TRUE),
  );

  $form['mask'] = array(
    '#type' => 'fieldset',
    '#title' => t('Required Access Mask'),
    '#description' => t('Edit your Access Mask that is required to register on the site.'),
    '#weight' => 10,
    '#collapsed' => FALSE,
    '#collapsible' => TRUE,
  );

  $form['mask']['current_mask'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => t('Current Mask: @access_mask', array('@access_mask' => $access_mask)),
    '#suffix' => '</p>',
    '#weight' => 0,
  );

  $form['mask']['access_mask'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#empty' => t('Accessmask not retrieved.'),
    '#weight' => 10,
  );

  $form['mask']['access_mask'][8]['#disabled'] = TRUE;
  $form['mask']['access_mask'][8388608]['#disabled'] = TRUE;

  $form['mask']['pager'] = array(
    '#theme' => 'pager',
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Mask'),
    '#weight' => 20,
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_admin_registration_form_validate');
  $form['#submit'] = array('eve_api_admin_registration_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_registration_form().
 *
 * @see eve_api_admin_registration_form_submit()
 */
function eve_api_admin_registration_form_validate($form, &$form_state) {
  if ((float) $form_state['values']['standing'] != round($form_state['values']['standing'], 1) || $form_state['values']['standing'] != (float) $form_state['values']['standing'] || (float) $form_state['values']['standing'] < -10.0 || (float) $form_state['values']['standing'] > 10.0) {
    form_set_error('standing', t('Standings are required to be between -10.0 and 10.0'));
  }
}

/**
 * Form submission handler for eve_api_admin_registration_form().
 *
 * @see eve_api_admin_registration_form_validate()
 */
function eve_api_admin_registration_form_submit($form, &$form_state) {
  variable_set('eve_api_required_standing', round((float) $form_state['values']['standing'], 1));
  variable_set('eve_api_require_expires', (bool) $form_state['values']['expires']);
  variable_set('eve_api_require_type', (bool) $form_state['values']['type']);

  $new_access_mask = 8388616;

  foreach ((array) $form_state['values']['access_mask'] as $access_bit) {
    $new_access_mask = $new_access_mask + (int) $access_bit;
  }

  $access_mask = variable_get('eve_api_access_mask', 268435455);

  variable_set('eve_api_access_mask', $new_access_mask);

  if ($access_mask != $new_access_mask) {
    $queue = DrupalQueue::get('eve_api_cron_api_user_sync');

    $result = db_query('SELECT DISTINCT uid FROM {eve_api_characters}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem(array(
          'uid' => $item->uid,
          'runs' => 1,
        ));
      }
    }
  }

  drupal_set_message(t('The registration settings have been updated, please be advised any user with incorrect API Masks or standing will have the roles removed.'));
}

/**
 * Form constructor for the admin list blues form.
 *
 * @see eve_api_admin_list_blues_form_validate()
 * @see eve_api_admin_list_blues_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_list_blues_form($form, &$form_state) {
  $header = array(
    'deleted' => array(
      'data' => t('Deleted'),
      'field' => 'b.deleted',
    ),
    'contactID' => array(
      'data' => t('ID'),
      'field' => 'b.contactID',
    ),
    'contactName' => array(
      'data' => t('Name'),
      'field' => 'b.contactName',
    ),
    'standing' => array(
      'data' => t('Standing'),
      'field' => 'b.standing',
    ),
    'manual' => array(
      'data' => t('Manually Entered'),
      'field' => 'b.manual',
    ),
  );

  $options = $selected = array();

  $result = db_select('eve_api_blue_standings', 'b')
    ->fields('b', array(
      'contactID',
      'contactName',
      'standing',
      'deleted',
      'manual',
    ))
    ->extend('PagerDefault')
    ->limit(20)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->contactID] = array(
        'deleted' => ($row->deleted) ? t('Yes') : t('No'),
        'contactID' => check_plain($row->contactID),
        'contactName' => check_plain($row->contactName),
        'standing' => check_plain($row->standing),
        'manual' => ($row->manual) ? t('Yes') : t('No'),
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
    '#empty' => t('There are no standings.'),
    '#weight' => 10,
  );

  $form['list_blues']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Blues'),
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_admin_list_blues_form_validate');
  $form['#submit'] = array('eve_api_admin_list_blues_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_list_blues_form().
 *
 * @see eve_api_admin_list_blues_form_submit()
 */
function eve_api_admin_list_blues_form_validate($form, &$form_state) {
  if (!is_array($form_state['values']['blues']) || !count(array_filter($form_state['values']['blues']))) {
    form_set_error('', t('No items selected.'));
  }
}

/**
 * Form submission handler for eve_api_admin_list_blues_form().
 *
 * @see eve_api_admin_list_blues_form_validate()
 */
function eve_api_admin_list_blues_form_submit($form, &$form_state) {
  db_update('eve_api_blue_standings')->fields(array('deleted' => 0))->execute();

  $deleted_blues = array();
  $is_change = FALSE;

  foreach ((array) $form_state['values']['blues'] as $blue_id) {
    $deleted_blues[] = (int) $blue_id;
    $is_change = TRUE;
  }

  db_update('eve_api_blue_standings')->fields(array('deleted' => 1))->condition('contactID', $deleted_blues, 'IN')->execute();

  if ($is_change) {
    $queue = DrupalQueue::get('eve_api_cron_api_user_sync');

    $result = db_query('SELECT DISTINCT uid FROM {eve_api_characters}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem(array(
          'uid' => $item->uid,
          'runs' => 1,
        ));
      }
    }
  }

  drupal_set_message(t('The Alliance Blues List have been updated, the users have been manually added to the cron queue to check standings.  This can take a while to process.'));
}

/**
 * Form constructor for the admin add blues form.
 *
 * @see eve_api_admin_add_blues_form_validate()
 * @see eve_api_admin_add_blues_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_add_blues_form($form, &$form_state) {
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
    '#size' => 30,
    '#weight' => 0,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add Blue'),
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form_state['redirect'] = 'admin/settings/eve_api/list_blues';
  $form['#validate'] = array('eve_api_admin_add_blues_form_validate');
  $form['#submit'] = array('eve_api_admin_add_blues_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_add_blues_form().
 *
 * @see eve_api_admin_add_blues_form_submit()
 */
function eve_api_admin_add_blues_form_validate($form, &$form_state) {
  $result = db_query('SELECT contactID FROM {eve_api_blue_standings} WHERE contactID = :contactID', array(
    ':contactID' => (int) $form_state['values']['id'],
  ));

  if ($result->rowCount()) {
    form_set_error('id', t('Character/Corporation/Alliance already added to the standings.'));
  }

  $result = db_query('SELECT corporationID FROM {eve_api_alliance_corporations} WHERE deleted = 0 AND corporationID = :corporationID', array(
    ':corporationID' => (int) $form_state['values']['id'],
  ));

  if ($result->rowCount()) {
    form_set_error('id', t('Corporation already listed as an Alliance Corporation.'));
  }

  $query = array('corporationID' => (int) $form_state['values']['id']);
  $corp_info = eve_api_get_corporation_info($query);

  $query = array('ids' => (int) $form_state['values']['id']);
  $char_info = eve_api_get_character_name($query);

  if (isset($char_info['error'])) {
    form_set_error('id', t('There was an error with the API.'));
  }
  else {
    $result = db_query('SELECT allianceName FROM {eve_api_alliance_info} WHERE allianceID = :allianceID', array(
      ':allianceID' => (int) $form_state['values']['id'],
    ));

    if (isset($corp_info['corporationName'])) {
      $form_state['values']['name'] = (string) $corp_info['corporationName'];
    }
    elseif (isset($char_info[(int) $form_state['values']['id']])) {
      $form_state['values']['name'] = $char_info[(int) $form_state['values']['id']];
    }
    elseif ($result->rowCount()) {
      $row = $result->fetchAssoc();
      $form_state['values']['name'] = (string) $row['allianceName'];
    }
    else {
      form_set_error('id', t('No match found.'));
    }
  }
}

/**
 * Form submission handler for eve_api_admin_add_blues_form().
 *
 * @see eve_api_admin_add_blues_form_validate()
 */
function eve_api_admin_add_blues_form_submit($form, &$form_state) {
  db_insert('eve_api_blue_standings')->fields(array(
    'contactID' => (int) $form_state['values']['id'],
    'contactName' => (string) $form_state['values']['name'],
    'standing' => 1,
    'manual' => 1,
  ))->execute();

  $queue = DrupalQueue::get('eve_api_cron_api_user_sync');

  $result = db_query('SELECT DISTINCT uid FROM {eve_api_characters}');

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $item) {
      $queue->createItem(array(
        'uid' => $item->uid,
        'runs' => 1,
      ));
    }
  }

  drupal_set_message(t('A new Blue standing has been added!'));
}

/**
 * Form constructor for the admin list whitelist form.
 *
 * @see eve_api_admin_list_whitelist_form_validate()
 * @see eve_api_admin_list_whitelist_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_list_whitelist_form($form, &$form_state) {
  $header = array(
    'deleted' => array(
      'data' => t('Deleted'),
      'field' => 'w.deleted',
    ),
    'characterID' => array(
      'data' => t('Character ID'),
      'field' => 'w.characterID',
    ),
    'characterName' => array(
      'data' => t('Character Name'),
      'field' => 'w.characterName',
    ),
  );

  $options = $selected = array();

  $result = db_select('eve_api_whitelist', 'w')
    ->fields('w', array(
      'characterID',
      'characterName',
      'deleted',
    ))
    ->extend('PagerDefault')
    ->limit(20)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->characterID] = array(
        'deleted' => ($row->deleted) ? t('Yes') : t('No'),
        'characterID' => check_plain($row->characterID),
        'characterName' => check_plain($row->characterName),
      );

      if ($row->deleted) {
        $selected[$row->characterID] = TRUE;
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
    '#empty' => t('No Characters Whitelisted.'),
    '#weight' => 10,
  );

  $form['whitelist']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Whitelist'),
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_admin_list_whitelist_form_validate');
  $form['#submit'] = array('eve_api_admin_list_whitelist_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_list_whitelist_form().
 *
 * @see eve_api_admin_list_whitelist_form_submit()
 */
function eve_api_admin_list_whitelist_form_validate($form, &$form_state) {
  if (!is_array($form_state['values']['whitelist']) || !count(array_filter($form_state['values']['whitelist']))) {
    form_set_error('', t('No items selected.'));
  }
}

/**
 * Form submission handler for eve_api_admin_list_whitelist_form().
 *
 * @see eve_api_admin_list_whitelist_form_validate()
 */
function eve_api_admin_list_whitelist_form_submit($form, &$form_state) {
  db_update('eve_api_whitelist')->fields(array('deleted' => 0))->execute();
  $deleted_whitelist = array();

  foreach ((array) $form_state['values']['whitelist'] as $whitelist) {
    $deleted_whitelist[] = (int) $whitelist;
  }

  $queue = DrupalQueue::get('eve_api_cron_api_user_sync');
  $result = db_query('SELECT DISTINCT uid FROM {eve_api_characters} WHERE characterID IN (:characterIDs)', array(':characterIDs' => $deleted_whitelist));

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $item) {
      $queue->createItem(array(
        'uid' => $item->uid,
        'runs' => 1,
      ));
    }
  }

  db_update('eve_api_whitelist')->fields(array('deleted' => 1))->condition('characterID', $deleted_whitelist, 'IN')->execute();

  drupal_set_message(t('The Alliance White List have been updated.'));
}

/**
 * Form constructor for the admin add whitelist form.
 *
 * @see eve_api_admin_add_whitelist_form_validate()
 * @see eve_api_admin_add_whitelist_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_add_whitelist_form($form, &$form_state) {
  $form['add_whitelist'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add Whitelist'),
    '#description' => t('Add characters to the whitelist, the key is not stored here, it is used to retrieve the characters on the API Key and add them to the whitelist.  There are times when you will have high profile members join your site, they may have sensitive e-mails or items in locations.  With this whitelist you are able to allow the user to register with a very limited and basic API Key, but still provide full functionality on the site.  Please provide the following <a href="@url" target="_blank">link</a> to have the new user create a new limited API Key, the user has the option to list all characters, or just a single character.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=8388680')),
    '#weight' => 0,
  );

  $form['add_whitelist']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter the Key ID.'),
    '#required' => TRUE,
    '#size' => 20,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['add_whitelist']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter the Verification Code.'),
    '#required' => TRUE,
    '#size' => 80,
    '#maxlength' => 64,
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add Whitelist'),
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form_state['redirect'] = 'admin/settings/eve_api/list_whitelist';
  $form['#validate'] = array('eve_api_admin_add_whitelist_form_validate');
  $form['#submit'] = array('eve_api_admin_add_whitelist_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_add_whitelist_form().
 *
 * @see eve_api_admin_add_whitelist_form_submit()
 */
function eve_api_admin_add_whitelist_form_validate($form, &$form_state) {
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

  $characters = eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyID', t('There was an error with the API.'));
    form_set_error('vCode');
  }
  else {
    if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
      form_set_error('keyID', t('Please ensure that only the following boxes are highlighted and selected for the API: (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet))'));
      form_set_error('keyID', t('In addition the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('vCode');
    }

    if (!eve_api_verify_blue($characters)) {
      form_set_error('keyID', t('No characters associated with the key are currently blue, please add the characters to the blue standings.'));
      form_set_error('vCode');
    }

    $form_state['values']['characters'] = $characters;
  }
}

/**
 * Form submission handler for eve_api_admin_add_whitelist_form().
 *
 * @see eve_api_admin_add_whitelist_form_validate()
 */
function eve_api_admin_add_whitelist_form_submit($form, &$form_state) {
  $characters = (array) $form_state['values']['characters'];

  foreach ($characters['characters'] as $character) {
    db_merge('eve_api_whitelist')->key(array('characterID' => (int) $character['characterID']))->fields(array(
      'characterID' => (int) $character['characterID'],
      'characterName' => (string) $character['characterName'],
      'deleted' => 0,
    ))->execute();
  }

  drupal_set_message(t('Whitelisted Characters successfully added!'));
}

/**
 * Form constructor for the admin list roles whitelist form.
 *
 * @see eve_api_admin_role_whitelist_form_validate()
 * @see eve_api_admin_role_whitelist_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_role_whitelist_form($form, &$form_state) {
  $header = array(
    'characterID' => array(
      'data' => t('Character ID'),
      'field' => 'c.characterID',
    ),
    'characterName' => array(
      'data' => t('Character Name'),
      'field' => 'c.characterName',
    ),
    'corporationName' => array(
      'data' => t('Corporation'),
      'field' => 'c.corporationName',
    ),
    'isCEO' => array(
      'data' => t('CEO'),
      'field' => 'crw.isCEO',
    ),
    'isDirector' => array(
      'data' => t('Director'),
      'field' => 'crw.isDirector',
    ),
  );

  $options = array();

  $query = db_select('eve_api_characters_roles_whitelist', 'crw');
  $query->join('eve_api_characters', 'c', 'crw.characterID = c.characterID');
  $query->fields('crw', array(
        'isCEO',
        'isDirector',
      ))
    ->fields('c', array(
        'characterID',
        'characterName',
        'corporationName',
      ))
    ->extend('PagerDefault')
    ->limit(20)
    ->extend('TableSort')
    ->orderByHeader($header);
  $result = $query->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->characterID] = array(
        'characterID' => check_plain($row->characterID),
        'characterName' => check_plain($row->characterName),
        'corporationName' => check_plain($row->corporationName),
        'isCEO' => ($row->isCEO) ? t('Yes') : t('No'),
        'isDirector' => ($row->isDirector) ? t('Yes') : t('No'),
      );
    }
  }

  $form['role_whitelist'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update Roles Whitelist'),
    '#description' => t('Toggle the checkbox to remove the Whitelisted Roles for the Character.'),
    '#weight' => 0,
  );

  $form['role_whitelist']['role_whitelist'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No Character Roles Whitelisted.'),
    '#weight' => 10,
  );

  $form['role_whitelist']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Roles'),
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_admin_role_whitelist_form_validate');
  $form['#submit'] = array('eve_api_admin_role_whitelist_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_role_whitelist_form().
 *
 * @see eve_api_admin_role_whitelist_form_submit()
 */
function eve_api_admin_role_whitelist_form_validate($form, &$form_state) {
  if (!is_array($form_state['values']['role_whitelist']) || !count(array_filter($form_state['values']['role_whitelist']))) {
    form_set_error('', t('No items selected.'));
  }
}

/**
 * Form submission handler for eve_api_admin_role_whitelist_form().
 *
 * @see eve_api_admin_role_whitelist_form_validate()
 */
function eve_api_admin_role_whitelist_form_submit($form, &$form_state) {
  $deleted_whitelist = array();

  foreach ((array) $form_state['values']['role_whitelist'] as $whitelist) {
    $deleted_whitelist[] = (int) $whitelist;
  }

  $queue = DrupalQueue::get('eve_api_cron_api_user_sync');
  $result = db_query('SELECT DISTINCT uid FROM {eve_api_characters} WHERE characterID IN (:characterIDs)', array(':characterIDs' => $deleted_whitelist));

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $item) {
      $queue->createItem(array(
        'uid' => $item->uid,
        'runs' => 1,
      ));
    }
  }

  db_delete('eve_api_characters_roles_whitelist')->condition('characterID', $deleted_whitelist, 'IN')->execute();

  drupal_set_message(t('The Character Roles Whitelist has been updated.'));
}

/**
 * Form constructor for the admin add roles whitelist form.
 *
 * @see eve_api_admin_add_role_whitelist_form_validate()
 * @see eve_api_admin_add_role_whitelist_form_submit()
 *
 * @ingroup forms
 */
function eve_api_admin_add_role_whitelist_form($form, &$form_state) {
  $form['add_role_whitelist'] = array(
    '#type' => 'fieldset',
    '#title' => t('Add/Update Roles Whitelist'),
    '#description' => t('Add characters to the roles whitelist, specify if they are a CEO or a Director.'),
    '#weight' => 0,
  );

  $form['add_role_whitelist']['characterName'] = array(
    '#type' => 'textfield',
    '#title' => t('Character Name'),
    '#description' => t('Please enter the Character Name, this character must already exist in the database.'),
    '#required' => TRUE,
    '#size' => 60,
    '#maxlength' => 50,
    '#weight' => 0,
  );

  $form['add_role_whitelist']['isCEO'] = array(
    '#type' => 'checkbox',
    '#title' => t('CEO'),
    '#description' => t('Manually add the role CEO to this character.'),
    '#weight' => 10,
  );

  $form['add_role_whitelist']['isDirector'] = array(
    '#type' => 'checkbox',
    '#title' => t('Director'),
    '#description' => t('Manually add the role Director to this character.'),
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add Roles Whitelist'),
  );

  if (variable_get('eve_api_first_run', FALSE) && !variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_admin_add_role_whitelist_form_validate');
  $form['#submit'] = array('eve_api_admin_add_role_whitelist_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_admin_add_role_whitelist_form().
 *
 * @see eve_api_admin_add_role_whitelist_form_submit()
 */
function eve_api_admin_add_role_whitelist_form_validate($form, &$form_state) {
  $character_name = (string) $form_state['values']['characterName'];
  $is_ceo = ($form_state['values']['isCEO'] == TRUE) ? TRUE : FALSE;
  $is_director = ($form_state['values']['isDirector'] == TRUE) ? TRUE : FALSE;

  if (!$is_ceo && !$is_director) {
    form_set_error('isCEO', t('At least one option must be selected.'));
    form_set_error('isDirector');
  }

  $chars = eve_api_get_character_info($character_name);

  if (!$chars) {
    form_set_error('characterName', t('The Character does not exist, please try again.'));
  }
}

/**
 * Form submission handler for eve_api_admin_add_role_whitelist_form().
 *
 * @see eve_api_admin_add_role_whitelist_form_validate()
 */
function eve_api_admin_add_role_whitelist_form_submit($form, &$form_state) {
  $form_state['redirect'] = 'admin/settings/eve_api/role_whitelist';

  $character_name = (string) $form_state['values']['characterName'];
  $is_ceo = ($form_state['values']['isCEO'] == TRUE) ? 1 : 0;
  $is_director = ($form_state['values']['isDirector'] == TRUE) ? 1 : 0;

  $character = eve_api_get_character_info($character_name);

  $queue = DrupalQueue::get('eve_api_cron_api_user_sync');
  $queue->createItem(array(
    'uid' => $character['uid'],
    'runs' => 1,
  ));

  db_merge('eve_api_characters_roles_whitelist')->key(array('characterID' => (int) $character['characterID']))->fields(array(
    'characterID' => (int) $character['characterID'],
    'isCEO' => $is_ceo,
    'isDirector' => $is_director,
  ))->execute();

  drupal_set_message(t('Whitelisted Characters successfully added!'));
}
