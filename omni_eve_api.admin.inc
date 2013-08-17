<?php
/**
 * @file
 * Functions related to allow admin's to alter the settings and options.
 */

module_load_include('inc', 'omni_eve_api', 'omni_eve_api.api');
module_load_include('inc', 'omni_eve_api', 'omni_eve_api.cron');

/**
 * Form constructor for the admin edit roles form.
 *
 * @see omni_eve_api_admin_roles_form_validate()
 * @see omni_eve_api_admin_roles_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_roles_form($form, &$form_state) {
  $form['roles'] = array(
    '#type' => 'fieldset',
    '#title' => t('Roles'),
    '#weight' => 0,
  );

  $role_list = omni_eve_api_role_list();

  $form['roles']['blue_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Blues'),
    '#description' => t('Please select the default role for blues.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_blue_role', 2),
    '#weight' => 0,
  );

  $form['roles']['alliance_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Alliance Members'),
    '#description' => t('Please select an additional role assigned to all members of the Alliance.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_alliance_role', 2),
    '#weight' => 10,
  );

  $form['roles']['unverified_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Unverified'),
    '#description' => t('Please select the default role for Unverified members.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_unverified_role', 2),
    '#weight' => 20,
  );

  $form['roles']['ceo_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for CEOs'),
    '#description' => t('Please select the default role for CEOs.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_ceo_role', 2),
    '#weight' => 30,
  );

  $form['roles']['director_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Directors'),
    '#description' => t('Please select the default role for directors.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_director_role', 2),
    '#weight' => 40,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('omni_eve_api_admin_roles_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_roles_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_roles_form().
 *
 * @see omni_eve_api_admin_roles_form_submit()
 */
function omni_eve_api_admin_roles_form_validate($form, &$form_state) {
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
 * Form submission handler for omni_eve_api_admin_roles_form().
 *
 * @see omni_eve_api_admin_roles_form_validate()
 */
function omni_eve_api_admin_roles_form_submit($form, &$form_state) {
  if ((int) variable_get('omni_eve_api_blue_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('omni_eve_api_blue_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_blue_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['blue_role']);
    }
  }

  if ((int) variable_get('omni_eve_api_alliance_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('omni_eve_api_alliance_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_alliance_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['alliance_role']);
    }
  }

  if ((int) variable_get('omni_eve_api_unverified_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('omni_eve_api_unverified_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_unverified_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['unverified_role']);
    }
  }

  if ((int) variable_get('omni_eve_api_ceo_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('omni_eve_api_ceo_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_ceo_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['ceo_role']);
    }
  }

  if ((int) variable_get('omni_eve_api_director_role', 2) != 2) {
    $uids = array();
    $result = db_query('SELECT uid FROM {users_roles} WHERE rid = :rid', array(
      ':rid' => (int) variable_get('omni_eve_api_director_role', 2),
    ));

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        $uids[] = $row->uid;
      }
    }

    if (!empty($uids)) {
      user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_director_role', 2));
      user_multiple_role_edit($uids, 'add_role', (int) $form_state['values']['director_role']);
    }
  }

  variable_set('omni_eve_api_blue_role', (int) $form_state['values']['blue_role']);
  variable_set('omni_eve_api_alliance_role', (int) $form_state['values']['alliance_role']);
  variable_set('omni_eve_api_unverified_role', (int) $form_state['values']['unverified_role']);
  variable_set('omni_eve_api_ceo_role', (int) $form_state['values']['ceo_role']);
  variable_set('omni_eve_api_director_role', (int) $form_state['values']['director_role']);

  drupal_set_message(t('The roles have been updated and will be updated with the next scheduled cron tasks.'));
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
    '#description' => t('Enter the Alliance Executers or Corperations CEO API.'),
    '#weight' => 0,
  );

  $form['enter_api']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_corp_keyid', ''),
  );

  $form['enter_api']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#weight' => 10,
    '#default_value' => variable_get('omni_eve_api_corp_vcode', ''),
  );

  $form['enter_api']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Omni EVE API'),
    '#default_value' => variable_get('omni_eve_api_enable', FALSE),
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update'),
  );

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['enter_api']['enable']['#default_value'] = TRUE;
    $form['submit_button']['#disabled'] = TRUE;
  }

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
    variable_set('omni_eve_api_enable', FALSE);
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Corporation' || $characters['accessMask'] != 67108863) {
      form_set_error('keyID', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to your Alliance Executor or Corporation CEO, the "Type" drop down box is set to "Corporation", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('vCode');
      variable_set('omni_eve_api_enable', FALSE);
    }

    $form_state['values']['characters'] = $characters;
  }
}

/**
 * Form submission handler for omni_eve_api_admin_alliance_api_key_form().
 *
 * @see omni_eve_api_admin_alliance_api_key_form_validate()
 */
function omni_eve_api_admin_alliance_api_key_form_submit($form, &$form_state) {
  $characters = (array) $form_state['values']['characters'];

  variable_set('omni_eve_api_corp_keyid', (int) $form_state['values']['keyID']);
  variable_set('omni_eve_api_corp_vcode', (string) $form_state['values']['vCode']);

  // It's not pretty but it works.
  foreach ($characters['characters'] as $character) {
    variable_set('omni_eve_api_allianceID', (int) $character['allianceID']);
    variable_set('omni_eve_api_corporationID', (int) $character['corporationID']);
  }

  if ((bool) $form_state['values']['enable']) {
    variable_set('omni_eve_api_first_run', TRUE);

    omni_eve_api_api_alliance_fetch_cron(TRUE);
    omni_eve_api_api_alliance_sync_cron(TRUE);
    omni_eve_api_api_mask_cron(TRUE);

    drupal_set_message(t('The Alliance API Info is set to be retrieved on the next cron job, it can take up to a minute for the cron task to be triggered.'));
  }
  else {
    variable_set('omni_eve_api_first_run', FALSE);
    variable_set('omni_eve_api_enable', FALSE);
    drupal_set_message(t('Registrations have been disabled, Omni EVE API cron tasks have been disabled, and all user menus have been disabled.'));
  }

  menu_rebuild();
}

/**
 * Form constructor for the altering the access mask form.
 *
 * @see omni_eve_api_admin_registration_form_validate()
 * @see omni_eve_api_admin_registration_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_registration_form($form, &$form_state) {
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

  $access_mask = variable_get('omni_eve_api_access_mask', 268435455);

  $result = db_select('oea_access_mask', 'a')
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
  );

  $form['registration']['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Settings'),
    '#description' => t('Various settings you can modify.'),
    '#weight' => 0,
  );

  $form['registration']['settings']['standing'] = array(
    '#type' => 'textfield',
    '#title' => t('Standings'),
    '#description' => t('Set the required standing to register on the site. Standing required will be equal to or greater than the value supplied.  Accepted values are 10.0 to -10.0'),
    '#maxlength' => 5,
    '#weight' => 0,
    '#required' => TRUE,
    '#default_value' => round((float) variable_get('omni_eve_api_required_standing', 0.1), 1),
  );

  $form['registration']['mask'] = array(
    '#type' => 'fieldset',
    '#title' => t('Edit Access Mask'),
    '#description' => t('Edit your Access Mask that is required to register on the site.'),
    '#weight' => 0,
  );

  $form['registration']['mask']['current_mask'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => t('Current Mask: @access_mask', array('@access_mask' => $access_mask)),
    '#suffix' => '</p>',
    '#weight' => 0,
  );

  $form['registration']['mask']['access_mask'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#empty' => t('Accessmask not retrieved.'),
    '#weight' => 10,
  );

  $form['registration']['mask']['access_mask'][8]['#disabled'] = TRUE;
  $form['registration']['mask']['access_mask'][8388608]['#disabled'] = TRUE;

  $form['registration']['pager'] = array(
    '#theme' => 'pager',
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update Mask'),
  );

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('omni_eve_api_admin_registration_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_registration_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_registration_form().
 *
 * @see omni_eve_api_admin_registration_form_submit()
 */
function omni_eve_api_admin_registration_form_validate($form, &$form_state) {
  if ((float) $form_state['values']['standing'] != round($form_state['values']['standing'], 1) || $form_state['values']['standing'] != (float) $form_state['values']['standing'] || (float) $form_state['values']['standing'] < -10.0 || (float) $form_state['values']['standing'] > 10.0) {
    form_set_error('standing', t('Standings are required to be between -10.0 and 10.0'));
  }
}

/**
 * Form submission handler for omni_eve_api_admin_registration_form().
 *
 * @see omni_eve_api_admin_registration_form_validate()
 */
function omni_eve_api_admin_registration_form_submit($form, &$form_state) {
  variable_set('omni_eve_api_required_standing', round((float) $form_state['values']['standing'], 1));

  $new_access_mask = 8388616;

  foreach ((array) $form_state['values']['access_mask'] as $access_bit) {
    $new_access_mask = $new_access_mask + (int) $access_bit;
  }

  $access_mask = variable_get('omni_eve_api_access_mask', 268435455);

  variable_set('omni_eve_api_access_mask', $new_access_mask);

  if ($access_mask != $new_access_mask) {
    $queue = DrupalQueue::get('omni_eve_api_cron_api_user_sync');

    $result = db_query('SELECT DISTINCT uid FROM {oea_characters}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem($item->uid);
      }
    }
  }

  drupal_set_message(t('The registration settings have been updated, please be advised any user with incorrect API Masks or standing will have the roles removed.'));
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

  $result = db_select('oea_blue_standings', 'b')
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

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

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
  $is_change = FALSE;

  foreach ((array) $form_state['values']['blues'] as $blue_id) {
    $deleted_blues[] = (int) $blue_id;
    $is_change = TRUE;
  }

  db_update('oea_blue_standings')->fields(array('deleted' => 1))->condition('contactID', $deleted_blues, 'IN')->execute();

  if ($is_change) {
    $queue = DrupalQueue::get('omni_eve_api_cron_api_user_sync');

    $result = db_query('SELECT DISTINCT uid FROM {oea_characters}');

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem($item->uid);
      }
    }
  }

  drupal_set_message(t('The Alliance Blues List have been updated, the users have been manually added to the cron queue to check standings.  This can take a while to process.'));
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

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

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
  $result = db_query('SELECT contactID FROM {oea_blue_standings} WHERE contactID = :contactID', array(
    ':contactID' => (int) $form_state['values']['id'],
  ));

  if ($result->rowCount()) {
    form_set_error('id', t('Character/Corporation/Alliance already added to the standings.'));
  }

  $result = db_query('SELECT corporationID FROM {oea_alliance_corporations} WHERE deleted = 0 AND corporationID = :corporationID', array(
    ':corporationID' => (int) $form_state['values']['id'],
  ));

  if ($result->rowCount()) {
    form_set_error('id', t('Corporation already listed as an Alliance Corporation.'));
  }

  $query = array('corporationID' => (int) $form_state['values']['id']);
  $corp_info = omni_eve_api_get_corporation_info($query);

  $query = array('ids' => (int) $form_state['values']['id']);
  $char_info = omni_eve_api_get_character_name($query);

  if (isset($char_info['error'])) {
    form_set_error('id', t('There was an error with the API.'));
  }
  else {
    $result = db_query('SELECT allianceName FROM {oea_alliance_info} WHERE allianceID = :allianceID', array(
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
 * Form submission handler for omni_eve_api_admin_add_blues_form().
 *
 * @see omni_eve_api_admin_add_blues_form_validate()
 */
function omni_eve_api_admin_add_blues_form_submit($form, &$form_state) {
  db_insert('oea_blue_standings')->fields(array(
    'contactID' => (int) $form_state['values']['id'],
    'contactName' => (string) $form_state['values']['name'],
    'standing' => 1,
    'manual' => 1,
  ))->execute();

  $queue = DrupalQueue::get('omni_eve_api_cron_api_user_sync');

  $result = db_query('SELECT DISTINCT uid FROM {oea_characters}');

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $item) {
      $queue->createItem($item->uid);
    }
  }

  drupal_set_message(t('A new Blue standing has been added!'));
}

/**
 * Form constructor for the admin list whitelist form.
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

  $result = db_select('oea_whitelist', 'w')
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

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

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

  foreach ((array) $form_state['values']['whitelist'] as $whitelist) {
    $deleted_whitelist[] = (int) $whitelist;
  }

  db_update('oea_whitelist')->fields(array('deleted' => 1))->condition('characterID', $deleted_whitelist, 'IN')->execute();

  drupal_set_message(t('The Alliance White List have been updated.'));
}

/**
 * Form constructor for the admin add whitelist form.
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
    '#description' => t('Add characters to the whitelist, the key is not stored here, it is used to retrieve the characters on the API Key and add them to the whitelist.  There are times when you will have high profile members join your site, they may have sensitive e-mails or items in locations.  With this whitelist you are able to allow the user to register with a very limited and basic API Key, but still provide full functionality on the site.  Please provide the following <a href="@url" target="_blank">link</a> to have the new user create a new limited API Key, the user has the option to list all characters, or just a single character.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=8388680')),
    '#weight' => 0,
  );

  $form['add_whitelist']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter the Key ID.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['add_whitelist']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter the Verification Code.'),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add Whitelist'),
  );

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

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
    if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
      form_set_error('keyID', t('Please ensure that only the following boxes are highlighted and selected for the API: (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet))'));
      form_set_error('keyID', t('In addition the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('vCode');
    }

    if (!omni_eve_api_verify_blue($characters)) {
      form_set_error('keyID', t('No characters associated with the key are currently blue, please add the characters to the blue standings.'));
      form_set_error('vCode');
    }

    $form_state['values']['characters'] = $characters;
  }
}

/**
 * Form submission handler for omni_eve_api_admin_add_whitelist_form().
 *
 * @see omni_eve_api_admin_add_whitelist_form_validate()
 */
function omni_eve_api_admin_add_whitelist_form_submit($form, &$form_state) {
  $characters = (array) $form_state['values']['characters'];

  foreach ($characters['characters'] as $character) {
    db_merge('oea_whitelist')->key(array('characterID' => (int) $character['characterID']))->fields(array(
      'characterID' => (int) $character['characterID'],
      'characterName' => (string) $character['characterName'],
      'deleted' => 0,
    ))->execute();
  }

  drupal_set_message(t('Whitelisted Characters successfully added!'));
}

/**
 * Form constructor for the teamspeak connection form.
 *
 * @see omni_eve_api_admin_teamspeak_form_validate()
 * @see omni_eve_api_admin_teamspeak_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_teamspeak_form($form, &$form_state) {
  $form['teamspeak'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configure Teamspeak 3 Connection'),
    '#description' => t('Configure Teamspeak 3 connection settings.'),
    '#weight' => 0,
  );

  $form['teamspeak']['hostname'] = array(
    '#type' => 'textfield',
    '#title' => t('IP Address'),
    '#description' => t('The Teamspeak 3 IP address used to connect to the Teamspeak 3 Server. This can be 127.0.0.1 if the Teamspeak server is hosted on the same server as the webhost.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_ts3_host', '127.0.0.1'),
  );

  $form['teamspeak']['server_address'] = array(
    '#type' => 'textfield',
    '#title' => t('Server Address'),
    '#description' => t('The Teamspeak 3 server address users will use to connect to your Teamspeak Server.'),
    '#required' => TRUE,
    '#maxlength' => 100,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_ts3_server_address', 'teamspeak.com'),
  );

  $form['teamspeak']['server_port'] = array(
    '#type' => 'textfield',
    '#title' => t('Server Port'),
    '#description' => t('The Teamspeak 3 server port users will use to connect to your Teamspeak Server. (Default: 9987)'),
    '#required' => TRUE,
    '#maxlength' => 6,
    '#weight' => 10,
    '#default_value' => variable_get('omni_eve_api_ts3_server_port', 9987),
  );

  $form['teamspeak']['server_password'] = array(
    '#type' => 'textfield',
    '#title' => t('Server Password'),
    '#description' => t('If you require a password to connect to your Teamspeak, please input the password, otherwise leave this empty.'),
    '#maxlength' => 50,
    '#weight' => 20,
    '#default_value' => variable_get('omni_eve_api_ts3_server_password', ''),
  );

  $form['teamspeak']['query_port'] = array(
    '#type' => 'textfield',
    '#title' => t('Query Port'),
    '#description' => t('The Teamspeak 3 query port. (Default: 10011)'),
    '#required' => TRUE,
    '#maxlength' => 6,
    '#weight' => 30,
    '#default_value' => variable_get('omni_eve_api_ts3_query_port', 10011),
  );

  $form['teamspeak']['query_username'] = array(
    '#type' => 'textfield',
    '#title' => t('Query Username'),
    '#description' => t('Query username choosen on your Teamspeak 3 server.'),
    '#required' => TRUE,
    '#maxlength' => 50,
    '#weight' => 40,
    '#default_value' => variable_get('omni_eve_api_ts3_query_username', 'Username'),
  );

  $form['teamspeak']['query_password'] = array(
    '#type' => 'textfield',
    '#title' => t('Query Password'),
    '#description' => t('Query password generated on your Teamspeak 3 server.'),
    '#required' => TRUE,
    '#maxlength' => 50,
    '#weight' => 50,
    '#default_value' => variable_get('omni_eve_api_ts3_query_password', 'Password'),
  );

  $form['teamspeak']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Teamspeak 3 Connection'),
    '#default_value' => variable_get('omni_eve_api_ts3_enable', FALSE),
    '#weight' => 70,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  if (!variable_get('omni_eve_api_ts3_framework_installed', FALSE)) {
    drupal_set_message(t('TeamSpeak 3 PHP Framework Library is not installed, please read the README.txt for instructions on how to install the library.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('omni_eve_api_admin_teamspeak_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_teamspeak_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_teamspeak_form().
 *
 * @see omni_eve_api_admin_teamspeak_form_submit()
 */
function omni_eve_api_admin_teamspeak_form_validate($form, &$form_state) {
  variable_set('omni_eve_api_ts3_host', (string) $form_state['values']['hostname']);
  variable_set('omni_eve_api_ts3_server_address', (string) $form_state['values']['server_address']);
  variable_set('omni_eve_api_ts3_server_port', (int) $form_state['values']['server_port']);
  variable_set('omni_eve_api_ts3_server_password', (string) $form_state['values']['server_password']);
  variable_set('omni_eve_api_ts3_query_port', (int) $form_state['values']['query_port']);
  variable_set('omni_eve_api_ts3_query_username', (string) $form_state['values']['query_username']);
  variable_set('omni_eve_api_ts3_query_password', (string) $form_state['values']['query_password']);

  if (variable_get('omni_eve_api_ts3_framework_installed', FALSE)) {
    if ((bool) $form_state['values']['enable']) {
      try {
        $ts3 = omni_eve_api_teamspeak_connection(TRUE);
        unset($ts3);
      }
      catch (OeaException $e) {
        form_set_error('enable', check_plain($e->getMessage()));
        variable_set('omni_eve_api_ts3_enable', FALSE);
      }
    }
  }
  else {
    form_set_error('enable', t('Teamspeak 3 PHP Framework not found, unable to enable Teamspeak 3 connection.'));
  }
}

/**
 * Form submission handler for omni_eve_api_admin_teamspeak_form().
 *
 * @see omni_eve_api_admin_teamspeak_form_validate()
 */
function omni_eve_api_admin_teamspeak_form_submit($form, &$form_state) {
  if ((bool) $form_state['values']['enable']) {
    variable_set('omni_eve_api_ts3_enable', TRUE);
    omni_eve_api_cron_teamspeak_role_sync(1);
  }
  else {
    variable_set('omni_eve_api_ts3_enable', FALSE);
  }

  menu_rebuild();

  drupal_set_message(t('Teamspeak Settings Updated.'));
}

/**
 * Form constructor for the teamspeak connection form.
 *
 * @see omni_eve_api_user_teamspeak_settings_form_validate()
 * @see omni_eve_api_user_teamspeak_settings_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_teamspeak_settings_form($form, &$form_state) {
  $form['teamspeak_settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Teamspeak 3 Settings'),
    '#description' => t('Teamspeak 3 settings and options.'),
    '#weight' => 0,
  );

  $form['teamspeak_settings']['nickname'] = array(
    '#type' => 'textfield',
    '#title' => t('Username Format'),
    '#description' => t('The format that each user must have when connecting to Teamspeak 3. You can use the following variables:<br />@corp - Corporation Ticker<br />@alliance - Alliance Ticker<br />@name - EVE Character Name'),
    '#required' => TRUE,
    '#maxlength' => 50,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_ts3_nickname', '[@corp] @name'),
  );

  try {
    $teamspeakgroups = array();

    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_groups = $ts3->serverGroupList();

    foreach ($teamspeak_groups as $teamspeak_group) {
      if ((int) $teamspeak_group->type == 1 && (string) $teamspeak_group->name != 'Guest') {
        $teamspeakgroups[(int) $teamspeak_group->sgid] = check_plain((string) $teamspeak_group->name);
      }
    }

    $form['teamspeak_settings']['bypass_group'] = array(
      '#type' => 'select',
      '#title' => t('Select Teamspeak Bypass Group'),
      '#description' => t('Please select a Teamspeak Group for users to bypass the name and registration check.'),
      '#options' => $teamspeakgroups,
      '#default_value' => variable_get('omni_eve_api_ts3_bypass_group', 0) == 0 ? '' : variable_get('omni_eve_api_ts3_bypass_group', 0),
      '#weight' => 10,
    );
  }
  catch (OeaException $e) {
    $form['teamspeak_settings']['bypass_group'] = array(
      '#type' => 'markup',
      '#prefix' => '<p>',
      '#markup' => t('Teamspeak 3 Connection failed, check settings and try again. Error: @error', array('@error' => check_plain($e->getMessage()))),
      '#suffix' => '</p>',
      '#weight' => 10,
    );
  }

  $selectable_options = array(
    1 => '1 Minute',
    5 => '5 Minutes',
    10 => '10 Minutes',
    15 => '15 Minutes',
    20 => '20 Minutes',
    25 => '25 Minutes',
    30 => '30 Minutes',
    35 => '35 Minutes',
    40 => '40 Minutes',
    45 => '45 Minutes',
    50 => '50 Minutes',
    55 => '55 Minutes',
    60 => '1 Hour',
  );

  $long_selectable_options = array(
    120 => '2 Hours',
    180 => '3 Hours',
    240 => '4 Hours',
    300 => '5 Hours',
    360 => '6 Hours',
    420 => '7 Hours',
    480 => '8 Hours',
    540 => '9 Hours',
    600 => '10 Hours',
    660 => '11 Hours',
    720 => '12 Hours',
    780 => '13 Hours',
    840 => '14 Hours',
    900 => '15 Hours',
    960 => '16 Hours',
    1020 => '17 Hours',
    1080 => '18 Hours',
    1140 => '19 Hours',
    1200 => '20 Hours',
    1260 => '21 Hours',
    1320 => '22 Hours',
    1380 => '23 Hours',
    1440 => '1 Day',
    2880 => '2 Days',
    4320 => '3 Days',
    5760 => '4 Days',
    7200 => '5 Days',
    8640 => '6 Days',
    10080 => '1 Week',
    20160 => '2 Weeks',
    30240 => '3 Weeks',
    43200 => '1 Month',
    525600 => '1 Year',
  );

  $form['teamspeak_settings']['reg'] = array(
    '#type' => 'fieldset',
    '#title' => t('Registration Notification'),
    '#description' => t('Options releated to non-registered Teamspeak users.'),
    '#weight' => 20,
  );

  $form['teamspeak_settings']['reg']['reg_time'] = array(
    '#type' => 'select',
    '#title' => t('User Register Time Limit'),
    '#description' => t('How long until the registration options are enforced.'),
    '#weight' => 0,
    '#options' => $selectable_options,
    '#default_value' => variable_get('omni_eve_api_ts3_cron_reg_time', 10),
  );

  $form['teamspeak_settings']['reg']['reg_ban_time'] = array(
    '#type' => 'select',
    '#title' => t('Ban Time'),
    '#description' => t('How long to ban the user.'),
    '#weight' => 10,
    '#options' => ($selectable_options + $long_selectable_options),
    '#default_value' => variable_get('omni_eve_api_ts3_cron_reg_ban_time', 10),
  );

  $header = array(
    'type' => array(
      'data' => t('Type'),
    ),
    'message' => array(
      'data' => t('Custom Message'),
    ),
  );

  $options = $selected = array();

  $options['reg_message_enable'] = array(
    'type' => t('Message'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_reg_message', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'reg_message',
        ),
      ),
    ),
  );

  $options['reg_poke_enable'] = array(
    'type' => t('Poke'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_reg_poke', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'reg_poke',
        ),
      ),
    ),
  );

  $options['reg_kick_enable'] = array(
    'type' => t('Kick'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_reg_kick', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'reg_kick',
        ),
      ),
    ),
  );

  $options['reg_ban_enable'] = array(
    'type' => t('Ban'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_reg_ban', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'reg_ban',
        ),
      ),
    ),
  );

  $selected['reg_message_enable'] = variable_get('omni_eve_api_ts3_cron_reg_message_enable', FALSE);
  $selected['reg_poke_enable'] = variable_get('omni_eve_api_ts3_cron_reg_poke_enable', FALSE);
  $selected['reg_kick_enable'] = variable_get('omni_eve_api_ts3_cron_reg_kick_enable', FALSE);
  $selected['reg_ban_enable'] = variable_get('omni_eve_api_ts3_cron_reg_ban_enable', FALSE);

  $form['teamspeak_settings']['reg']['reg_options'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#weight' => 20,
  );

  $form['teamspeak_settings']['user'] = array(
    '#type' => 'fieldset',
    '#title' => t('Nickname Notification'),
    '#description' => t('Options releated to registered Teamspeak users with incorrect nicknames.'),
    '#weight' => 30,
  );

  $form['teamspeak_settings']['user']['user_time'] = array(
    '#type' => 'select',
    '#title' => t('Correct Name Time Limit'),
    '#description' => t('How long until the incorrect nickname options are enforced.'),
    '#weight' => 0,
    '#options' => $selectable_options,
    '#default_value' => variable_get('omni_eve_api_ts3_cron_user_time', 10),
  );

  $form['teamspeak_settings']['user']['user_ban_time'] = array(
    '#type' => 'select',
    '#title' => t('Ban Time'),
    '#description' => t('How long to ban the user.'),
    '#weight' => 10,
    '#options' => ($selectable_options + $long_selectable_options),
    '#default_value' => variable_get('omni_eve_api_ts3_cron_user_ban_time', 10),
  );

  $options = $selected = array();

  $options['user_message_enable'] = array(
    'type' => t('Message'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_user_message', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'user_message',
        ),
      ),
    ),
  );

  $options['user_poke_enable'] = array(
    'type' => t('Poke'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_user_poke', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'user_poke',
        ),
      ),
    ),
  );

  $options['user_kick_enable'] = array(
    'type' => t('Kick'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_user_kick', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'user_kick',
        ),
      ),
    ),
  );

  $options['user_ban_enable'] = array(
    'type' => t('Ban'),
    'message' => array(
      'data' => array(
        '#type' => 'textfield',
        '#value' => variable_get('omni_eve_api_ts3_cron_user_ban', ''),
        '#maxlength' => 50,
        '#attributes' => array(
          'name' => 'user_ban',
        ),
      ),
    ),
  );

  $selected['user_message_enable'] = variable_get('omni_eve_api_ts3_cron_user_message_enable', FALSE);
  $selected['user_poke_enable'] = variable_get('omni_eve_api_ts3_cron_user_poke_enable', FALSE);
  $selected['user_kick_enable'] = variable_get('omni_eve_api_ts3_cron_user_kick_enable', FALSE);
  $selected['user_ban_enable'] = variable_get('omni_eve_api_ts3_cron_user_ban_enable', FALSE);

  $form['teamspeak_settings']['user']['user_options'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#weight' => 20,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  if (!variable_get('omni_eve_api_ts3_framework_installed', FALSE)) {
    drupal_set_message(t('TeamSpeak 3 PHP Framework Library is not installed, please read the README.txt for instructions on how to install the library.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('omni_eve_api_user_teamspeak_settings_form_validate');
  $form['#submit'] = array('omni_eve_api_user_teamspeak_settings_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_teamspeak_settings_form().
 *
 * @see omni_eve_api_user_teamspeak_settings_form_submit()
 */
function omni_eve_api_user_teamspeak_settings_form_validate($form, &$form_state) {
  if (!variable_get('omni_eve_api_ts3_framework_installed', FALSE)) {
    form_set_error('enable', t('Teamspeak 3 PHP Framework not found, unable to enable Teamspeak 3 connection.'));
  }
}

/**
 * Form submission handler for omni_eve_api_user_teamspeak_settings_form().
 *
 * @see omni_eve_api_user_teamspeak_settings_form_validate()
 */
function omni_eve_api_user_teamspeak_settings_form_submit($form, &$form_state) {
  variable_set('omni_eve_api_ts3_bypass_group', (int) $form_state['values']['bypass_group']);
  variable_set('omni_eve_api_ts3_nickname', (string) $form_state['values']['nickname']);
  variable_set('omni_eve_api_ts3_cron_reg_time', (int) $form_state['values']['reg_time']);
  variable_set('omni_eve_api_ts3_cron_user_time', (int) $form_state['values']['user_time']);
  variable_set('omni_eve_api_ts3_cron_reg_ban_time', (int) $form_state['values']['reg_ban_time']);
  variable_set('omni_eve_api_ts3_cron_user_ban_time', (int) $form_state['values']['user_ban_time']);

  variable_set('omni_eve_api_ts3_cron_reg_message', omni_eve_api_clean_input((string) $form_state['input']['reg_message']));
  variable_set('omni_eve_api_ts3_cron_reg_poke', omni_eve_api_clean_input((string) $form_state['input']['reg_poke']));
  variable_set('omni_eve_api_ts3_cron_reg_kick', omni_eve_api_clean_input((string) $form_state['input']['reg_kick']));
  variable_set('omni_eve_api_ts3_cron_reg_ban', omni_eve_api_clean_input((string) $form_state['input']['reg_ban']));
  variable_set('omni_eve_api_ts3_cron_user_message', omni_eve_api_clean_input((string) $form_state['input']['user_message']));
  variable_set('omni_eve_api_ts3_cron_user_poke', omni_eve_api_clean_input((string) $form_state['input']['user_poke']));
  variable_set('omni_eve_api_ts3_cron_user_kick', omni_eve_api_clean_input((string) $form_state['input']['user_kick']));
  variable_set('omni_eve_api_ts3_cron_user_ban', omni_eve_api_clean_input((string) $form_state['input']['user_ban']));

  $form_state['redirect'] = 'admin/settings/omni_eve_api/teamspeak';

  foreach ((array) $form_state['values']['reg_options'] as $field => $value) {
    $field = 'omni_eve_api_ts3_cron_' . (string) $field;
    variable_set($field, (bool) $value);
  }

  foreach ((array) $form_state['values']['user_options'] as $field => $value) {
    $field = 'omni_eve_api_ts3_cron_' . (string) $field;
    variable_set($field, (bool) $value);
  }

  drupal_set_message(t('Teamspeak Settings Updated.'));
}

/**
 * Form constructor for the jabber connection form.
 *
 * @see omni_eve_api_admin_jabber_form_validate()
 * @see omni_eve_api_admin_jabber_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_jabber_form($form, &$form_state) {
  $form['jabber'] = array(
    '#type' => 'fieldset',
    '#title' => t('Configure Jabber Connection'),
    '#description' => t('Configure the Jabber Database connection settings.'),
    '#weight' => 0,
  );

  $form['jabber']['host'] = array(
    '#type' => 'textfield',
    '#title' => t('Hostname'),
    '#description' => t('The Jabber Database IP address or hostname. This can be 127.0.0.1 if the Database is hosted on the same server as the webhost.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_jabber_host', '127.0.0.1'),
  );

  $form['jabber']['database'] = array(
    '#type' => 'textfield',
    '#title' => t('Database Name'),
    '#description' => t('The name of the Jabber database on the MySQL Server.'),
    '#required' => TRUE,
    '#maxlength' => 100,
    '#weight' => 10,
    '#default_value' => variable_get('omni_eve_api_jabber_database', 'Jabber'),
  );

  $form['jabber']['username'] = array(
    '#type' => 'textfield',
    '#title' => t('Username'),
    '#description' => t('The username required to connect to the Jabber Database.'),
    '#required' => TRUE,
    '#maxlength' => 100,
    '#weight' => 20,
    '#default_value' => variable_get('omni_eve_api_jabber_username', 'Username'),
  );

  $form['jabber']['password'] = array(
    '#type' => 'textfield',
    '#title' => t('Password'),
    '#description' => t('The password required to connect to the Jabber Database.'),
    '#maxlength' => 100,
    '#weight' => 30,
    '#default_value' => variable_get('omni_eve_api_jabber_password', 'Password'),
  );

  $form['jabber']['url'] = array(
    '#type' => 'textfield',
    '#title' => t('Openfire Admin URL'),
    '#description' => t('The web address to the Openfire admin page. Default http://localhost:9090'),
    '#maxlength' => 100,
    '#weight' => 40,
    '#default_value' => variable_get('omni_eve_api_jabber_url', 'http://localhost:9090'),
  );

  $form['jabber']['secret'] = array(
    '#type' => 'textfield',
    '#title' => t('Openfire Secret Password'),
    '#description' => t('The Openfire secret password to access the User Service.'),
    '#maxlength' => 100,
    '#weight' => 50,
    '#default_value' => variable_get('omni_eve_api_jabber_secret', 'Password'),
  );

  $form['jabber']['domain'] = array(
    '#type' => 'textfield',
    '#title' => t('Jabber Domain'),
    '#description' => t('The domain appended to the Jabber login username. (ex. user@yourdomain.com)'),
    '#maxlength' => 100,
    '#weight' => 60,
    '#default_value' => variable_get('omni_eve_api_jabber_domain', 'yourdomain.com'),
  );

  $form['jabber']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Jabber Connection'),
    '#default_value' => variable_get('omni_eve_api_jabber_enable', FALSE),
    '#weight' => 70,
  );

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['#validate'] = array('omni_eve_api_admin_jabber_form_validate');
  $form['#submit'] = array('omni_eve_api_admin_jabber_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_admin_jabber_form().
 *
 * @see omni_eve_api_admin_jabber_form_submit()
 */
function omni_eve_api_admin_jabber_form_validate($form, &$form_state) {
  variable_set('omni_eve_api_jabber_host', (string) $form_state['values']['host']);
  variable_set('omni_eve_api_jabber_database', (string) $form_state['values']['database']);
  variable_set('omni_eve_api_jabber_username', (string) $form_state['values']['username']);
  variable_set('omni_eve_api_jabber_password', (string) $form_state['values']['password']);
  variable_set('omni_eve_api_jabber_url', (string) $form_state['values']['url']);
  variable_set('omni_eve_api_jabber_secret', (string) $form_state['values']['secret']);
  variable_set('omni_eve_api_jabber_domain', (string) $form_state['values']['domain']);

  if ((bool) $form_state['values']['enable']) {
    try {
      omni_eve_api_jabber_connect(TRUE);
      omni_eve_api_jabber_test_connection();
      omni_eve_api_jabber_disconnect();
    }
    catch (OeaException $e) {
      omni_eve_api_jabber_disconnect();
      form_set_error('enable', t('Please check database settings and try again. Error: @error', array('@error' => check_plain($e->getMessage()))));
      variable_set('omni_eve_api_jabber_enable', FALSE);
    }

    try {
      $result = omni_eve_api_jabber_url_query('check');

      if ($result != 'IllegalArgumentException') {
        form_set_error('enable', t('Please check your Openfire URL settings and try again.'));
        variable_set('omni_eve_api_jabber_enable', FALSE);
      }
    }
    catch (OeaException $e) {
      form_set_error('enable', t('Please check your Openfire URL settings and try again.'));
      variable_set('omni_eve_api_jabber_enable', FALSE);
    }
  }
}

/**
 * Form submission handler for omni_eve_api_admin_jabber_form().
 *
 * @see omni_eve_api_admin_jabber_form_validate()
 */
function omni_eve_api_admin_jabber_form_submit($form, &$form_state) {
  if ((bool) $form_state['values']['enable']) {
    variable_set('omni_eve_api_jabber_enable', TRUE);
  }
  else {
    variable_set('omni_eve_api_jabber_enable', FALSE);
  }

  menu_rebuild();

  drupal_set_message(t('Jabber Settings Updated.'));
}
