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
function omni_eve_api_register_wizard($step = 'enter_api') {
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
    'next callback' => 'omni_eve_api_subtask_next',
    // Callback to use when entire form is completed.
    'finish callback' => 'omni_eve_api_subtask_finish',
    // Callback to use when user clicks final submit button.
    'return callback' => 'omni_eve_api_subtask_finish',
    // Callback to use when user cancels wizard.
    'cancel callback' => 'omni_eve_api_subtask_cancel',
    // Specify the order that the child forms will appear in, as well as their
    // page titles.
    'order' => array(
      'enter_api' => t('Enter EVE API'),
      'register' => t('Register'),
    ),
    // Define the child forms. Be sure to use the same keys here that were user
    // in the 'order' section of this array.
    'forms' => array(
      'enter_api' => array('form id' => 'omni_eve_api_enter_api_form'),
      'register' => array('form id' => 'user_register_form'),
    ),
  );

  // Make cached data available within each step's $form_state array.
  $form_state['signup_object'] = omni_eve_api_get_page_cache('signup');

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
function omni_eve_api_get_page_cache($name) {
  ctools_include('object-cache');
  $cache = ctools_object_cache_get('omni_eve_api', $name);

  // If the cached object doesn't exist yet, create an empty object.
  if (!$cache) {
    $cache = new stdClass();
    $cache->locked = ctools_object_cache_test('omni_eve_api', $name);
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
function omni_eve_api_set_page_cache($name, $data) {
  ctools_include('object-cache');
  $cache = ctools_object_cache_set('omni_eve_api', $name, $data);
}

/**
 * Removes an item from the object cache.
 *
 * @param string $name
 *   The name of the object to destroy.
 */
function omni_eve_api_clear_page_cache($name) {
  ctools_include('object-cache');
  ctools_object_cache_clear('omni_eve_api', $name);
}

/**
 * Callback executed when the 'next' button is clicked.
 */
function omni_eve_api_subtask_next(&$form_state) {
  // Store submitted data in a ctools cache object, namespaced 'signup'.
  omni_eve_api_set_page_cache('signup', $form_state['values']);
}

/**
 * Callback executed when the 'cancel' button is clicked.
 */
function omni_eve_api_subtask_cancel(&$form_state) {
  // Clear our ctools cache object. It's good housekeeping.
  omni_eve_api_clear_page_cache('signup');
}

/**
 * Callback executed when the entire form submission is finished.
 */
function omni_eve_api_subtask_finish(&$form_state) {
  // Clear our Ctool cache object.
  omni_eve_api_clear_page_cache('signup');

  // Redirect the user to the front page.
  drupal_goto('<front>');
}

/**
 * Implements hook_form_FORM_ID_alter() for user_register_form().
 */
function omni_eve_api_form_user_register_form_alter(&$form, &$form_state) {
  unset($form['actions']);

  $form['account']['#type'] = 'fieldset';
  $form['account']['#title'] = t('Blue Status Verified');
  $form['account']['#description'] = t('Please select your main character from the drop down box.  This character will be used to identify yourself throughout the site and various tools.');

  $form['#validate'] = array(
    'omni_eve_api_user_register_form_validate',
    'user_account_form_validate',
    'user_validate_picture',
    'user_register_validate',
  );

  $form['#submit'] = array(
    'user_register_submit',
    'omni_eve_api_user_register_form_submit',
    'ctools_wizard_submit',
  );

  if (!is_array($form_state['signup_object'])) {
    form_set_error('keyid', t('Unknown Error, please try again.'));
    form_set_error('vcode');
    drupal_goto('user/register');
  }

  $keyid = $form_state['signup_object']['keyid'];
  $vcode = $form_state['signup_object']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyid', t('Unknown Error, please try again.'));
    form_set_error('vcode');
    drupal_goto('user/register');
  }

  $whitelist_characters = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $whitelist_characters[] = $character['characterID'];
    }
  }

  $result = db_select('oea_whitelist', 'w')->fields('w', array('characterid'))->condition('characterid', $whitelist_characters, 'IN')->condition('deleted', 0, '=')->execute();
  if ($result->rowCount()) {
    if ($characters['expires'] || $characters['accessMask'] != 8388680) {
      form_set_error('keyid', t('Unknown Error, please try again.'));
      form_set_error('vcode');
      drupal_goto('user/register');
    }
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Account' || $characters['accessMask'] != variable_get('omni_eve_api_access_mask', 268435455)) {
      form_set_error('keyid', t('Unknown Error, please try again.'));
      form_set_error('vcode');
      drupal_goto('user/register');
    }
  }

  if (!omni_eve_api_verify_blue($characters)) {
    form_set_error('keyid', t('Unknown Error, please try again.'));
    form_set_error('vcode');
    drupal_goto('user/register');
  }

  if ($chars = omni_eve_api_characters_exist($characters)) {
    form_set_error('keyid', t('Unknown Error, please try again.'));
    form_set_error('vcode');
    drupal_goto('user/register');
  }

  $form['account']['charactername'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => '',
    '#options' => omni_eve_api_list_valid_characters($characters),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );
}

/**
 * Form validation handler for user_register_form().
 *
 * @see omni_eve_api_user_register_form_submit()
 */
function omni_eve_api_user_register_form_validate($form, &$form_state) {
  $keyid = (int) $form_state['signup_object']['keyid'];
  $vcode = (string) $form_state['signup_object']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    form_set_error('keyid', t('Unknown Error, please try again.'));
    form_set_error('vcode');
    drupal_goto('user/register');
  }

  $whitelist_characters = array();

  if (!empty($characters)) {
    foreach ($characters['characters'] as $character) {
      $whitelist_characters[] = (int) $character['characterID'];
    }
  }

  $result = db_select('oea_whitelist', 'w')->fields('w', array('characterid'))->condition('characterid', $whitelist_characters, 'IN')->condition('deleted', 0, '=')->execute();
  if ($result->rowCount()) {
    if ($characters['expires'] || $characters['accessMask'] != 8388680) {
      form_set_error('keyid', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
      form_set_error('vcode');
    }
  }
  else {
    if ($characters['expires'] || $characters['type'] != 'Account' || $characters['accessMask'] != variable_get('omni_eve_api_access_mask', 268435455)) {
      form_set_error('keyid', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
      form_set_error('vcode');
    }
  }

  if (!omni_eve_api_verify_blue($characters)) {
    form_set_error('keyid', t('No characters associated with your key are currently blue.'));
    form_set_error('vcode');
  }

  if ($chars = omni_eve_api_characters_exist($characters)) {
    form_set_error('keyid', t('Characters on this key have already been registered. Characters registered: @chars', array('@chars' => implode(", ", $chars))));
    form_set_error('vcode');
  }

  $selected_charactername = array();

  foreach ($characters['characters'] as $character) {
    $selected_charactername[] = $character['characterName'];
  }

  if (!in_array((string) $form_state['values']['charactername'], $selected_charactername)) {
    form_set_error('charactername', t('You must select a main character!'));
  }
}

/**
 * Form submission handler for user_login_form().
 *
 * @see omni_eve_api_user_register_form_validate()
 */
function omni_eve_api_user_register_form_submit($form, &$form_state) {
  $uid = $form_state['user']->uid;
  $keyid = $form_state['signup_object']['keyid'];
  $vcode = $form_state['signup_object']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  $created = date('Y-m-d H:i:s', time());

  $new_api_key_id = db_insert('oea_api_keys')->fields(array(
    'drupalid' => (int) $uid,
    'keyid' => (int) $keyid,
    'vcode' => (string) $vcode,
    'updated' => (string) $created,
    'created' => (string) $created,
  ))->execute();

  foreach ($characters['characters'] as $character) {
    db_merge('oea_characters')->key(array('characterid' => (int) $character['characterID']))->fields(array(
      'characterid' => (int) $character['characterID'],
      'drupalid' => (int) $uid,
      'apiid' => (int) $new_api_key_id,
      'charactername' => (string) $character['characterName'],
      'corporationid' => (int) $character['corporationID'],
      'corporationname' => (string) $character['corporationName'],
      'corporationticker' => (string) $character['corporationTicker'],
      'allianceid' => (int) $character['allianceID'],
      'alliancename' => (string) $character['allianceName'],
      'allianceticker' => (string) $character['allianceTicker'],
    ))->execute();

    if ((string) $form_state['values']['charactername'] == (string) $character['characterName']) {
      $charactername = $character['characterName'];
      $characterid = $character['characterID'];
      $corporationname = $character['corporationName'];
    }
  }

  if ($corporation_role = user_role_load_by_name($corporationname)) {
    user_multiple_role_edit(array($uid), 'add_role', $corporation_role->rid);

    $alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $alliance_role->rid);
  }
  else {
    $blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2));
    user_multiple_role_edit(array($uid), 'add_role', $blue_role->rid);
  }

  db_update('users')->fields(array(
      'characterid' => $characterid,
      'charactername' => $charactername,
      'signature' => '',
    ))->condition('uid', $uid, '=')->execute();
}

/**
 * Form constructor for the landing page for the user registration process.
 *
 * @see omni_eve_api_enter_api_form_validate()
 * @see omni_eve_api_enter_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_enter_api_form($form, &$form_state) {
  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Verify Blue Status'),
    '#description' => t('Please enter your EVE API in order to verify you qualify for an account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
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

  $form['#validate'][] = 'omni_eve_api_enter_api_form_validate';
  $form['#submit'][] = 'omni_eve_api_enter_api_form_submit';

  return $form;
}

/**
 * Form validation handler for omni_eve_api_enter_api_form().
 *
 * @see omni_eve_api_enter_api_form_submit()
 */
function omni_eve_api_enter_api_form_validate($form, &$form_state) {
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
      if ($characters['expires'] || $characters['accessMask'] != 8388680) {
        form_set_error('keyid', t('Your account has been whitelisted, please ensure that the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.  Only (Public Information -> (Characterinfo and FacWarStats), (Private Information) -> (CharacterSheet)) are required.'));
        form_set_error('vcode');
      }
    }
    else {
      if ($characters['expires'] || $characters['type'] != 'Account' || $characters['accessMask'] != variable_get('omni_eve_api_access_mask', 268435455)) {
        form_set_error('keyid', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'));
        form_set_error('vcode');
      }
    }

    if (!omni_eve_api_verify_blue($characters)) {
      form_set_error('keyid', t('No characters associated with your key are currently blue to this alliance.'));
      form_set_error('vcode');
    }

    if ($chars = omni_eve_api_characters_exist($characters)) {
      form_set_error('keyid', t('Characters on this key have already been registered. Characters registered: @chars', array('@chars' => implode(", ", $chars))));
      form_set_error('vcode');
    }
  }
}

/**
 * Form submission handler for omni_eve_api_enter_api_form().
 *
 * @see omni_eve_api_enter_api_form_validate()
 */
function omni_eve_api_enter_api_form_submit($form, &$form_state) {
}
