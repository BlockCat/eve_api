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

  $role_list = omni_eve_api_role_list();

  $form['roles']['omni_eve_api_blue_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Blues'),
    '#description' => t('Please select the default role for blues.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_blue_role', '2'),
    '#weight' => 0,
  );

  $form['roles']['omni_eve_api_alliance_role'] = array(
    '#type' => 'select',
    '#title' => t('Select Role for Alliance Members'),
    '#description' => t('Please select an additional role assigned to all members of the Alliance.'),
    '#options' => $role_list,
    '#default_value' => variable_get('omni_eve_api_alliance_role', '2'),
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
  $uids = array();
  $result = db_select('users_roles', 'r')->fields('r', array('uid'))->condition('rid', (int) variable_get('omni_eve_api_blue_role', '2'), '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $uids[] = $row->uid;
    }
  }

  if (!empty($uids)) {
    user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_blue_role', '2'));
    // user_multiple_role_edit($uids, 'add_role',
    // (int) $form_state['values']['omni_eve_api_blue_role']);
  }

  $uids = array();
  $result = db_select('users_roles', 'r')->fields('r', array('uid'))->condition('rid', (int) variable_get('omni_eve_api_alliance_role', '2'), '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $uids[] = $row->uid;
    }
  }

  if (!empty($uids)) {
    user_multiple_role_edit($uids, 'remove_role', (int) variable_get('omni_eve_api_alliance_role', '2'));
    // user_multiple_role_edit($uids, 'add_role',
    // (int) $form_state['values']['omni_eve_api_alliance_role']);
  }

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
    '#description' => t('Enter the Alliance Executers or Corperations CEO API.'),
    '#weight' => 0,
  );

  $form['enter_api']['omni_eve_api_corp_keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@url">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => variable_get('omni_eve_api_corp_keyid', ''),
  );

  $form['enter_api']['omni_eve_api_corp_vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@url">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
    '#default_value' => variable_get('omni_eve_api_corp_vcode', ''),
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

  if (isset($characters['error'])) {
    form_set_error('omni_eve_api_corp_keyid', t('There was an error with the API.'));
    form_set_error('omni_eve_api_corp_vcode');
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Corporation' || $characters['accessMask'] != 67108863) {
      form_set_error('omni_eve_api_corp_keyid', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to your Alliance Executor or Corporation CEO, the "Type" drop down box is set to "Corporation", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('omni_eve_api_corp_vcode');
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
  variable_set('omni_eve_api_corp_vcode', $form_state['values']['omni_eve_api_corp_vcode']);

  // It's not pretty but it works.
  foreach ($characters['characters'] as $character) {
    variable_set('omni_eve_api_allianceID', (int) $character['allianceID']);
    variable_set('omni_eve_api_corporationID', (int) $character['corporationID']);
  }

  omni_eve_api_get_access_mask();
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

  $access_mask = variable_get('omni_eve_api_access_mask', '268435455');

  $result = db_select('oea_access_mask', 'a')->fields('a', array(
    'accessmask',
    'type',
    'name',
    'description',
  ))->condition('type', 'Character', '=')->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[$row->accessmask] = array(
        'mask' => $row->accessmask,
        'name' => $row->name,
        'description' => $row->description,
      );

      if (($access_mask & $row->accessmask) == TRUE) {
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
    '#markup' => t("Current Mask: @access_mask", array('@access_mask' => $access_mask)),
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
 * Implements hook_user_role_delete().
 */
function omni_eve_api_user_role_delete($role) {
  if ($role->rid == variable_get('omni_eve_api_blue_role', '2')) {
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
        'deleted' => ($row->deleted) ? t('Yes') : t('No'),
        'id' => $row->contactID,
        'name' => $row->contactName,
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

  drupal_set_message(t('A new Blue standing has been added!'));
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
    '#description' => t('Add characters to the whitelist, the key is not stored here, it is used to retrieve the characters on the API Key and add them to the whitelist.  There are times when you will have high profile members join your site, they may have sensitive e-mails or items in locations.  With this whitelist you are able to allow the user to register with a very limited and basic API Key, but still provide full functionality on the site.  Please provide the following <a href="@url">link</a> to have the new user create a new limited API Key, the user has the option to list all characters, or just a single character.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=8388680')),
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
