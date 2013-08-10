<?php
/**
 * @file
 * Functions related to allow admin's to alter the settings and options.
 */

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

  $form['roles']['omni_eve_api_blue_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Blues'),
    '#description' => t('Please select the default role for blues.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_blue_role', 2),
    '#weight' => 0,
  );

  $form['roles']['omni_eve_api_alliance_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Alliance Members'),
    '#description' => t('Please select an additional role assigned to all members of the Alliance.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_alliance_role', 2),
    '#weight' => 10,
  );

  $form['roles']['omni_eve_api_unverified_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Unverified'),
    '#description' => t('Please select the default role for Unverified members.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_unverified_role', 2),
    '#weight' => 10,
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
  if (user_role_load((int) $form_state['values']['omni_eve_api_blue_role']) == FALSE) {
    form_set_error('omni_eve_api_blue_role', t('The role does not exist.'));
  }

  if (user_role_load((int) $form_state['values']['omni_eve_api_alliance_role']) == FALSE) {
    form_set_error('omni_eve_api_alliance_role', t('The role does not exist.'));
  }

  if (user_role_load((int) $form_state['values']['omni_eve_api_unverified_role']) == FALSE) {
    form_set_error('omni_eve_api_unverified_role', t('The role does not exist.'));
  }
}

/**
 * Form submission handler for omni_eve_api_admin_roles_form().
 *
 * @see omni_eve_api_admin_roles_form_validate()
 */
function omni_eve_api_admin_roles_form_submit($form, &$form_state) {
  $uids = array();
  $result = db_select('users_roles', 'r')->fields('r', array('uid'))->condition('rid', (int) variable_get('omni_eve_api_blue_role', 2), '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $uids[] = $row->uid;
    }
  }

  if (!empty($uids)) {
    user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_blue_role', 2));
    // user_multiple_role_edit($uids, 'add_role',
    // (int) $form_state['values']['omni_eve_api_blue_role']);
  }

  $uids = array();
  $result = db_select('users_roles', 'r')->fields('r', array('uid'))->condition('rid', (int) variable_get('omni_eve_api_alliance_role', 2), '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $uids[] = $row->uid;
    }
  }

  if (!empty($uids)) {
    user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_alliance_role', 2));
    // user_multiple_role_edit($uids, 'add_role',
    // (int) $form_state['values']['omni_eve_api_alliance_role']);
  }

  $uids = array();
  $result = db_select('users_roles', 'r')->fields('r', array('uid'))->condition('rid', (int) variable_get('omni_eve_api_unverified_role', 2), '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $uids[] = $row->uid;
    }
  }

  if (!empty($uids)) {
    user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_unverified_role', 2));
    // user_multiple_role_edit($uids, 'add_role',
    // (int) $form_state['values']['omni_eve_api_unverified_role']);
  }

  variable_set('omni_eve_api_blue_role', (int) $form_state['values']['omni_eve_api_blue_role']);
  variable_set('omni_eve_api_alliance_role', (int) $form_state['values']['omni_eve_api_alliance_role']);
  variable_set('omni_eve_api_unverified_role', (int) $form_state['values']['omni_eve_api_unverified_role']);

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

  $form['enter_api']['omni_eve_api_corp_keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_corp_keyid', ''),
  );

  $form['enter_api']['omni_eve_api_corp_vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
    '#default_value' => variable_get('omni_eve_api_corp_vcode', ''),
  );

  $form['enter_api']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Omni EVE API.'),
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
  $query = array(
    'keyid' => $form_state['values']['omni_eve_api_corp_keyid'],
    'vcode' => $form_state['values']['omni_eve_api_corp_vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('omni_eve_api_corp_keyid', t('There was an error with the API.'));
    form_set_error('omni_eve_api_corp_vcode');
    variable_set('omni_eve_api_enable', FALSE);
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Corporation' || $characters['accessMask'] != 67108863) {
      form_set_error('omni_eve_api_corp_keyid', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to your Alliance Executor or Corporation CEO, the "Type" drop down box is set to "Corporation", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('omni_eve_api_corp_vcode');
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
  $characters = $form_state['values']['characters'];

  variable_set('omni_eve_api_corp_keyid', (int) $form_state['values']['omni_eve_api_corp_keyid']);
  variable_set('omni_eve_api_corp_vcode', (string) $form_state['values']['omni_eve_api_corp_vcode']);

  // It's not pretty but it works.
  foreach ($characters['characters'] as $character) {
    variable_set('omni_eve_api_allianceID', (int) $character['allianceID']);
    variable_set('omni_eve_api_corporationID', (int) $character['corporationID']);
  }

  if ((bool) $form_state['values']['enable']) {
    variable_set('omni_eve_api_first_run', TRUE);
    omni_eve_api_general_cron(TRUE);
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
 * @see omni_eve_api_admin_access_mask_form_validate()
 * @see omni_eve_api_admin_access_mask_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_admin_access_mask_form($form, &$form_state) {
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
        'accessmask' => (int) $row->accessmask,
        'name' => $row->name,
        'description' => $row->description,
      );

      if (($access_mask & (int) $row->accessmask) == TRUE) {
        $selected[$row->accessmask] = TRUE;
      }
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
    '#prefix' => '<p>',
    '#markup' => t('Current Mask: @access_mask', array('@access_mask' => $access_mask)),
    '#suffix' => '</p>',
    '#weight' => 0,
  );

  $form['access_mask']['access_mask'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#default_value' => $selected,
    '#empty' => t('Accessmask not retrieved.'),
    '#weight' => 10,
  );

  $form['access_mask']['access_mask'][8]['#disabled'] = TRUE;
  $form['access_mask']['access_mask'][8388608]['#disabled'] = TRUE;

  $form['access_mask']['pager'] = array(
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
  $new_access_mask = 8388616;

  foreach ($form_state['values']['access_mask'] as $access_bit) {
    $new_access_mask = $new_access_mask + $access_bit;
  }

  $access_mask = variable_get('omni_eve_api_access_mask', 268435455);

  variable_set('omni_eve_api_access_mask', $new_access_mask);

  if ($access_mask != $new_access_mask) {
    $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
    $queue->createQueue();
    $query = db_select('oea_api_keys', 'ak');
    $query->join('users', 'u', 'ak.uid = u.uid');
    $query->fields('ak', array(
      'uid',
    ));
    $query->distinct();
    $query->condition('u.status', 1, '=');
    $result = $query->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $item) {
        $queue->createItem($item->uid);
      }
    }
  }

  drupal_set_message(t('The required API Access Mask has been updated, please be advised any user with incorrect API Masks will have the roles removed.'));
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
        'contactID' => $row->contactID,
        'contactName' => $row->contactName,
        'standing' => $row->standing,
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

  foreach ($form_state['values']['blues'] as $blue_id) {
    $deleted_blues[] = $blue_id;
    $is_change = TRUE;
  }

  db_update('oea_blue_standings')->fields(array('deleted' => 1))->condition('contactID', $deleted_blues, 'IN')->execute();

  if ($is_change) {
    $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
    $queue->createQueue();
    $query = db_select('oea_api_keys', 'ak');
    $query->join('users', 'u', 'ak.uid = u.uid');
    $query->fields('ak', array(
      'uid',
    ));
    $query->distinct();
    $query->condition('u.status', 1, '=');
    $result = $query->execute();

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

  if (isset($char_info['error'])) {
    form_set_error('id', t('There was an error with the API.'));
  }
  else {
    $alliance_info = db_select('oea_alliance_info', 'a')->fields('a', array('alliancename'))->condition('allianceid', (int) $form_state['values']['id'], '=')->execute()->fetchAssoc();

    if (isset($corp_info['corporationName'])) {
      $form_state['values']['name'] = $corp_info['corporationName'];
    }
    elseif (isset($char_info[(int) $form_state['values']['id']])) {
      $form_state['values']['name'] = $char_info[(int) $form_state['values']['id']];
    }
    elseif ($alliance_info->rowCount()) {
      $form_state['values']['name'] = $row['alliancename'];
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
    'contactID' => $form_state['values']['id'],
    'contactName' => $form_state['values']['name'],
    'standing' => 1,
    'manual' => 1,
  ))->execute();

  $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
  $queue->createQueue();
  $query = db_select('oea_api_keys', 'ak');
  $query->join('users', 'u', 'ak.uid = u.uid');
  $query->fields('ak', array(
    'uid',
  ));
  $query->distinct();
  $query->condition('u.status', 1, '=');
  $result = $query->execute();

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
    'characterid' => array(
      'data' => t('Character ID'),
      'field' => 'w.characterid',
    ),
    'charactername' => array(
      'data' => t('Character Name'),
      'field' => 'w.charactername',
    ),
  );

  $options = $selected = array();

  $result = db_select('oea_whitelist', 'w')
    ->fields('w', array(
      'characterid',
      'charactername',
      'deleted',
    ))
    ->extend('PagerDefault')
    ->limit(20)
    ->extend('TableSort')
    ->orderByHeader($header)
    ->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->characterid] = array(
        'deleted' => ($row->deleted) ? t('Yes') : t('No'),
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
  foreach ($form_state['values']['whitelist'] as $whitelist) {
    $deleted_whitelist[] = $whitelist;
  }
  db_update('oea_whitelist')->fields(array('deleted' => 1))->condition('characterid', $deleted_whitelist, 'IN')->execute();

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
  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('omni_eve_api_corp_keyid', t('There was an error with the API.'));
    form_set_error('omni_eve_api_corp_vcode');
  }
  else {
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

    $form_state['values']['characters'] = $characters;
  }
}

/**
 * Form submission handler for omni_eve_api_admin_add_whitelist_form().
 *
 * @see omni_eve_api_admin_add_whitelist_form_validate()
 */
function omni_eve_api_admin_add_whitelist_form_submit($form, &$form_state) {
  $characters = $form_state['values']['characters'];

  foreach ($characters['characters'] as $character) {
    db_merge('oea_whitelist')->key(array('characterid' => $character['characterID']))->fields(array(
      'characterid' => $character['characterID'],
      'charactername' => $character['characterName'],
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
    '#title' => t('Configure Teamspeak Connection'),
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
    '#description' => t('The Teamspeak 3 server address used to connect to your Teamspeak Server.'),
    '#required' => TRUE,
    '#maxlength' => 100,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_ts3_server_address', 'teamspeak.com'),
  );

  $form['teamspeak']['server_port'] = array(
    '#type' => 'textfield',
    '#title' => t('Server Port'),
    '#description' => t('The port used to connect to Teamspeak 3. (Default: 9987)'),
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

  $form['teamspeak']['nickname'] = array(
    '#type' => 'textfield',
    '#title' => t('Username Format'),
    '#description' => t('The format that each user must have when connecting to Teamspeak 3. You can use the following variables:<br />@corp - Corporation Ticker<br />@alliance - Alliance Ticker<br />@name - EVE Character Name'),
    '#required' => TRUE,
    '#maxlength' => 50,
    '#weight' => 60,
    '#default_value' => variable_get('omni_eve_api_ts3_nickname', '[@corp] @name'),
  );

  $form['teamspeak']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable Teamspeak 3 Connection.'),
    '#default_value' => variable_get('omni_eve_api_ts3_enable', FALSE),
    '#weight' => 70,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  if (variable_get('omni_eve_api_ts3_enable', FALSE) && variable_get('omni_eve_api_ts3_framework_installed', FALSE) !== FALSE) {
    try {
      $teamspeakgroups = array();

      $ts3 = omni_eve_api_teamspeak_connection();
      $teamspeak_groups = $ts3->serverGroupList();

      foreach ($teamspeak_groups as $teamspeak_group) {
        if ((int) $teamspeak_group->type == 1 && (string) $teamspeak_group->name != 'Guest') {
          $teamspeakgroups[(int) $teamspeak_group->sgid] = check_plain((string) $teamspeak_group->name);
        }
      }

      $form['teamspeak']['omni_eve_api_ts3_bypass_group'] = array(
        '#type' => 'select',
        '#title' => t('Select Teamspeak Bypass Group'),
        '#description' => t('Please select a Teamspeak Group for users to bypass the name and registration check.'),
        '#options' => $teamspeakgroups,
        '#default_value' => variable_get('omni_eve_api_ts3_bypass_group', 0),
        '#weight' => 80,
      );
    }
    catch (OeaException $e) {
      $form['teamspeak']['omni_eve_api_ts3_bypass_group'] = array(
        '#type' => 'markup',
        '#prefix' => '<p>',
        '#markup' => t('Teamspeak 3 Connection failed, check settings and try again. Error: @error', array('@error' => $e->getMessage())),
        '#suffix' => '</p>',
        '#weight' => 80,
      );
    }
  }

  if (variable_get('omni_eve_api_first_run', FALSE) && !variable_get('omni_eve_api_enable', FALSE)) {
    drupal_set_message(t('Please wait until the cron tasks have pulled the Alliance API Info.'));
    $form['submit_button']['#disabled'] = TRUE;
  }

  if (variable_get('omni_eve_api_ts3_framework_installed', FALSE) === FALSE) {
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
  variable_set('omni_eve_api_ts3_nickname', (string) $form_state['values']['nickname']);
  variable_set('omni_eve_api_ts3_bypass_group', (int) $form_state['values']['omni_eve_api_ts3_bypass_group']);

  if (variable_get('omni_eve_api_ts3_framework_installed', FALSE) !== FALSE) {
    if ((bool) $form_state['values']['enable']) {
      try {
        $query = 'serverquery://' . (string) $form_state['values']['query_username'] . ':' . (string) $form_state['values']['query_password'] . '@' . (string) $form_state['values']['hostname'] . ':' . (int) $form_state['values']['query_port'] . '/?server_port=' . (int) $form_state['values']['server_port'] . '&blocking=0';
        $ts3 = TeamSpeak3::factory($query);
      }
      catch (TeamSpeak3_Exception $e) {
        form_set_error('enable', t('Teamspeak 3 Connection failed, check settings and try again. Error: @error', array('@error' => $e->getMessage())));
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
    omni_eve_api_cron_teamspeak_role(1);
  }
  else {
    variable_set('omni_eve_api_ts3_enable', FALSE);
  }

  menu_rebuild();

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
    '#title' => t('Enable Jabber Connection.'),
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
      if (variable_get('omni_eve_api_jabber_enable', FALSE) === FALSE) {
        $jabber_database = array(
          'host' => variable_get('omni_eve_api_jabber_host', '127.0.0.1'),
          'database' => variable_get('omni_eve_api_jabber_database', 'Jabber'),
          'username' => variable_get('omni_eve_api_jabber_username', 'Username'),
          'password' => variable_get('omni_eve_api_jabber_password', 'Password'),
          'driver' => 'mysql',
        );

        Database::addConnectionInfo('Jabber', 'default', $jabber_database);
      }

      omni_eve_api_jabber_connect(TRUE);
      omni_eve_api_jabber_test_connection();
      omni_eve_api_jabber_disconnect();
    }
    catch (OeaException $e) {
      omni_eve_api_jabber_disconnect();
      form_set_error('enable', t('Please check database settings and try again. Error: @error', array('@error' => $e->getMessage())));
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
