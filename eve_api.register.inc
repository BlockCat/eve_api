<?php
/**
 * @file
 * Functions related to altering the registration method on the site.
 */

/**
 * Create callback for standard ctools registration wizard.
 *
 * @param string $step
 *   Indicates what step of the registration process is being called
 */
function eve_api_register_wizard($step = 'enter_api') {
  // Include required ctools files.
  ctools_include('wizard');
  ctools_include('object-cache');

  $form_info = array(
    // Specify unique form id for this form.
    'id' => 'multistep_registration',
    // Specify the path for this form. It is important to include space for the
    // $step argument to be passed.
    'path' => "user/register/%step",
    // Show bread crumb trail.
    'show trail' => TRUE,
    'show back' => FALSE,
    'show return' => FALSE,
    // Callback to use when the 'next' button is clicked.
    'next callback' => 'eve_api_subtask_next',
    // Callback to use when entire form is completed.
    'finish callback' => 'eve_api_subtask_finish',
    // Callback to use when user clicks final submit button.
    'return callback' => 'eve_api_subtask_finish',
    // Callback to use when user cancels wizard.
    'cancel callback' => 'eve_api_subtask_cancel',
    // Specify the order that the child forms will appear in, as well as their
    // page titles.
    'order' => array(
      'enter_api' => t('Enter EVE API'),
      'register' => t('Register'),
    ),
    // Define the child forms. Be sure to use the same keys here that were user
    // in the 'order' section of this array.
    'forms' => array(
      'enter_api' => array('form id' => 'eve_api_enter_api_form'),
      'register' => array('form id' => 'user_register_form'),
    ),
  );

  // Make cached data available within each step's $form_state array.
  $form_state['signup_object'] = eve_api_get_page_cache('signup');

  // Return the form as a Ctools multi-step form.
  $output = ctools_wizard_multistep_form($form_info, $step, $form_state);

  return $output;
}

/**
 * Retrieves an object from the cache.
 *
 * @param string $name
 *   The name of the cached object to retrieve.
 */
function eve_api_get_page_cache($name) {
  ctools_include('object-cache');
  $cache = ctools_object_cache_get('eve_api', $name);

  // If the cached object doesn't exist yet, create an empty object.
  if (!$cache) {
    $cache = new stdClass();
    $cache->locked = ctools_object_cache_test('eve_api', $name);
  }

  return $cache;
}

/**
 * Creates or updates an object in the cache.
 *
 * @param string $name
 *   The name of the object to cache.
 *
 * @param object $data
 *   The object to be cached.
 */
function eve_api_set_page_cache($name, $data) {
  ctools_include('object-cache');
  ctools_object_cache_set('eve_api', $name, $data);
}

/**
 * Removes an item from the object cache.
 *
 * @param string $name
 *   The name of the object to destroy.
 */
function eve_api_clear_page_cache($name) {
  ctools_include('object-cache');
  ctools_object_cache_clear('eve_api', $name);
}

/**
 * Callback executed when the 'next' button is clicked.
 */
function eve_api_subtask_next(&$form_state) {
  // Store submitted data in a ctools cache object, namespaced 'signup'.
  eve_api_set_page_cache('signup', $form_state['values']);
}

/**
 * Callback executed when the 'cancel' button is clicked.
 */
function eve_api_subtask_cancel(&$form_state) {
  // Clear our ctools cache object. It's good housekeeping.
  eve_api_clear_page_cache('signup');
}

/**
 * Callback executed when the entire form submission is finished.
 */
function eve_api_subtask_finish(&$form_state) {
  // Clear our Ctool cache object.
  eve_api_clear_page_cache('signup');

  // Redirect the user to the front page.
  drupal_goto('<front>');
}

/**
 * Implements hook_form_FORM_ID_alter() for user_register_form().
 */
function eve_api_form_user_register_form_alter(&$form, &$form_state) {
  unset($form['actions']);

  $account = $form['#user'];
  $register = ((int) $form['#user']->uid > 0 ? FALSE : TRUE);

  $admin = user_access('administer users');

  $form['account']['#type'] = 'fieldset';
  $form['account']['#title'] = t('Blue Status Verified');
  $form['account']['#description'] = t('Please select your main character from the drop down box.  This character will be used to identify yourself throughout the site and various tools.');

  $form['#validate'] = array(
    'eve_api_user_register_form_validate',
    'user_account_form_validate',
    'user_validate_picture',
    'user_register_validate',
  );

  $form['#submit'] = array(
    'user_register_submit',
    'eve_api_user_register_form_submit',
    'ctools_wizard_submit',
  );

  if (!is_array($form_state['signup_object'])) {
    form_set_error('keyID', t('Unknown Error, please try again.'));
    form_set_error('vCode');
    drupal_goto('user/register');
  }

  $key_id = (int) $form_state['signup_object']['keyID'];
  $v_code = (string) $form_state['signup_object']['vCode'];

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyID', t('Unknown Error, please try again.'));
    form_set_error('vCode');
    drupal_goto('user/register');
  }

  $whitelist = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $whitelist[] = (int) $character['characterID'];
    }
  }

  $result = db_query('SELECT characterID FROM {eve_api_whitelist} WHERE deleted = 0 AND characterID IN (:characterIDs)', array(
    ':characterIDs' => $whitelist,
  ));

  if ($result->rowCount()) {
    if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
      form_set_error('keyID', t('Unknown Error, please try again.'));
      form_set_error('vCode');
      drupal_goto('user/register');
    }
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
      form_set_error('keyID', t('Unknown Error, please try again.'));
      form_set_error('vCode');
      drupal_goto('user/register');
    }
  }

  if (!eve_api_verify_blue($characters)) {
    form_set_error('keyID', t('Unknown Error, please try again.'));
    form_set_error('vCode');
    drupal_goto('user/register');
  }

  if (eve_api_characters_exist($characters)) {
    form_set_error('keyID', t('Unknown Error, please try again.'));
    form_set_error('vCode');
    drupal_goto('user/register');
  }

  $form['account']['name'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => (!$register ? $account->name : ''),
    '#options' => eve_api_list_valid_characters($characters, ''),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
    '#attributes' => array('class' => array('username')),
    '#access' => ($register || $admin),
    '#weight' => -10,
  );
}

/**
 * Form validation handler for user_register_form().
 *
 * @see eve_api_user_register_form_submit()
 */
function eve_api_user_register_form_validate(&$form, &$form_state) {
  $key_id = (int) $form_state['signup_object']['keyID'];
  $v_code = (string) $form_state['signup_object']['vCode'];

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyID', t('Unknown Error, please try again.'));
    form_set_error('vCode');
    drupal_goto('user/register');
  }

  $whitelist = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $whitelist[] = (int) $character['characterID'];
    }
  }

  $result = db_query('SELECT characterID FROM {eve_api_whitelist} WHERE deleted = 0 AND characterID IN (:characterIDs)', array(
    ':characterIDs' => $whitelist,
  ));

  if ($result->rowCount()) {
    if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
      form_set_error('keyID', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
      form_set_error('vCode');
    }
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
      form_set_error('keyID', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('vCode');
    }
  }

  if (!eve_api_verify_blue($characters)) {
    form_set_error('keyID', t('No characters associated with your key are currently blue.'));
    form_set_error('vCode');
  }

  if ($chars = eve_api_characters_exist($characters)) {
    form_set_error('keyID', t('Characters on this key have already been registered. Characters registered: @chars', array('@chars' => implode(", ", $chars))));
    form_set_error('vCode');
  }

  $selected = array();

  foreach ($characters['characters'] as $character) {
    $selected[] = $character['characterName'];
  }

  if (!in_array((string) $form_state['values']['name'], $selected)) {
    form_set_error('name', t('You must select a main character!'));
  }
}

/**
 * Form submission handler for user_login_form().
 *
 * @see eve_api_user_register_form_validate()
 */
function eve_api_user_register_form_submit(&$form, &$form_state) {
  $account = $form_state['user'];
  $uid = (int) $account->uid;
  $key_id = (int) $form_state['signup_object']['keyID'];
  $v_code = (string) $form_state['signup_object']['vCode'];

  $query = array(
    'keyID' => $key_id,
    'vCode' => $v_code,
  );

  $characters = eve_api_get_character_api($query);

  $created = date('Y-m-d H:i:s', time());

  $api_id = db_insert('eve_api_api_keys')->fields(array(
    'uid' => $uid,
    'keyID' => $key_id,
    'vCode' => $v_code,
    'updated' => (string) $created,
    'created' => (string) $created,
  ))->execute();

  foreach ($characters['characters'] as $character) {
    $sheet_query = $query + array('characterID' => (int) $character['characterID']);

    $character_sheet = eve_api_get_character_sheet($sheet_query);

    if (!empty($character_sheet['corporationRoles'])) {
      foreach ($character_sheet['corporationRoles'] as $roles) {
        db_merge('eve_api_characters_roles')->key(array(
          'characterID' => (int) $character['characterID'],
          'type' => 'corporationRoles',
          'roleID' => (int) $roles['roleID'],
        ))->fields(array(
          'characterID' => (int) $character['characterID'],
          'uid' => (int) $uid,
          'apiID' => (int) $api_id,
          'type' => 'corporationRoles',
          'roleID' => (int) $roles['roleID'],
          'roleName' => (string) $roles['roleName'],
          'deleted' => 0,
          'errorID' => 0,
        ))->execute();
      }
    }

    if (!empty($character_sheet['corporationRolesAtHQ'])) {
      foreach ($character_sheet['corporationRolesAtHQ'] as $roles) {
        db_merge('eve_api_characters_roles')->key(array(
          'characterID' => (int) $character['characterID'],
          'type' => 'corporationRolesAtHQ',
          'roleID' => (int) $roles['roleID'],
        ))->fields(array(
          'characterID' => (int) $character['characterID'],
          'uid' => (int) $uid,
          'apiID' => (int) $api_id,
          'type' => 'corporationRolesAtHQ',
          'roleID' => (int) $roles['roleID'],
          'roleName' => (string) $roles['roleName'],
          'deleted' => 0,
          'errorID' => 0,
        ))->execute();
      }
    }

    if (!empty($character_sheet['corporationRolesAtBase'])) {
      foreach ($character_sheet['corporationRolesAtBase'] as $roles) {
        db_merge('eve_api_characters_roles')->key(array(
          'characterID' => (int) $character['characterID'],
          'type' => 'corporationRolesAtBase',
          'roleID' => (int) $roles['roleID'],
        ))->fields(array(
          'characterID' => (int) $character['characterID'],
          'uid' => (int) $uid,
          'apiID' => (int) $api_id,
          'type' => 'corporationRolesAtBase',
          'roleID' => (int) $roles['roleID'],
          'roleName' => (string) $roles['roleName'],
          'deleted' => 0,
          'errorID' => 0,
        ))->execute();
      }
    }

    if (!empty($character_sheet['corporationRolesAtOther'])) {
      foreach ($character_sheet['corporationRolesAtOther'] as $roles) {
        db_merge('eve_api_characters_roles')->key(array(
          'characterID' => (int) $character['characterID'],
          'type' => 'corporationRolesAtOther',
          'roleID' => (int) $roles['roleID'],
        ))->fields(array(
          'characterID' => (int) $character['characterID'],
          'uid' => (int) $uid,
          'apiID' => (int) $api_id,
          'type' => 'corporationRolesAtOther',
          'roleID' => (int) $roles['roleID'],
          'roleName' => (string) $roles['roleName'],
          'deleted' => 0,
          'errorID' => 0,
        ))->execute();
      }
    }

    db_merge('eve_api_characters')->key(array('characterID' => (int) $character['characterID']))->fields(array(
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
      'errorID' => 0,
    ))->execute();

    if ((string) $form_state['values']['name'] == (string) $character['characterName']) {
      $character_name = (string) $character['characterName'];
      $character_id = (int) $character['characterID'];
      $corporation_name = (string) $character['corporationName'];
      $is_director = eve_api_characters_is_director((int) $character['characterID']);
      $is_ceo = eve_api_characters_is_ceo((int) $character['characterID']);
    }
  }

  if ($corporation_role = user_role_load_by_name($corporation_name)) {
    user_multiple_role_edit(array($uid), 'add_role', (int) $corporation_role->rid);

    $alliance_role = user_role_load(variable_get('eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', (int) $alliance_role->rid);

    if ($is_director) {
      $director_role = user_role_load(variable_get('eve_api_director_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', (int) $director_role->rid);
    }

    if ($is_ceo) {
      $ceo_role = user_role_load(variable_get('eve_api_ceo_role', 2));
      user_multiple_role_edit(array($uid), 'add_role', (int) $ceo_role->rid);
    }
  }
  else {
    $blue_role = user_role_load(variable_get('eve_api_blue_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', (int) $blue_role->rid);
  }

  db_update('users')->fields(array(
      'characterID' => $character_id,
      'name' => $character_name,
      'signature' => '',
    ))->condition('uid', $uid, '=')->execute();
}

/**
 * Form constructor for the landing page for the user registration process.
 *
 * @see eve_api_enter_api_form_validate()
 * @see eve_api_enter_api_form_submit()
 *
 * @ingroup forms
 */
function eve_api_enter_api_form($form, &$form_state) {
  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Verify Blue Status'),
    '#description' => t('Please enter your EVE API in order to verify you qualify for an account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['enter_api']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['enter_api']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@mask" target="_blank">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 64,
    '#weight' => 10,
  );

  if (!variable_get('eve_api_enable', FALSE)) {
    drupal_set_message(t('Registrations temporarily disabled.'), 'error');
    $form['buttons']['next']['#disabled'] = TRUE;
  }

  $form['#validate'][] = 'eve_api_enter_api_form_validate';

  return $form;
}

/**
 * Form validation handler for eve_api_enter_api_form().
 *
 * @see eve_api_enter_api_form_submit()
 */
function eve_api_enter_api_form_validate($form, &$form_state) {
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
    $whitelist = array();

    if (!empty($characters)) {
      foreach ($characters['characters'] as $character) {
        $whitelist[] = (int) $character['characterID'];
      }
    }

    $result = db_query('SELECT characterID FROM {eve_api_whitelist} WHERE deleted = 0 AND characterID IN (:characterIDs)', array(
      ':characterIDs' => $whitelist,
    ));

    if ($result->rowCount()) {
      if ($characters['expires'] || ($characters['accessMask'] & 8388680) != 8388680) {
        form_set_error('keyID', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
        form_set_error('vCode');
      }
    }
    else {
      if ($characters['expires'] || $characters['type'] != 'Account' || ($characters['accessMask'] & variable_get('eve_api_access_mask', 268435455)) != variable_get('eve_api_access_mask', 268435455)) {
        form_set_error('keyID', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
        form_set_error('vCode');
      }
    }

    if (!eve_api_verify_blue($characters)) {
      form_set_error('keyID', t('No characters associated with your key are currently blue to this alliance.'));
      form_set_error('vCode');
    }

    if ($chars = eve_api_characters_exist($characters)) {
      form_set_error('keyID', t('Characters on this key have already been registered. Characters registered: @chars', array('@chars' => implode(", ", $chars))));
      form_set_error('vCode');
    }
  }
}
