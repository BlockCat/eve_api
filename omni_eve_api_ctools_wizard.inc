<?php

/**
 * Create callback for standard ctools registration wizard.
 */
function omni_eve_api_ctools_wizard($step = 'enter_api')
{
  // Include required ctools files.
  ctools_include('wizard');
  ctools_include('object-cache');

  $form_info = array(
    // Specify unique form id for this form.
    'id' => 'multistep_registration',
    //Specify the path for this form. It is important to include space for the $step argument to be passed.
    'path' => "user/register/%step",
    // Show breadcrumb trail.
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
    // Specify the order that the child forms will appear in, as well as their page titles.
    'order' => array(
      'enter_api' => t('Enter EVE API'),
      'register' => t('Register'),
    ),
    // Define the child forms. Be sure to use the same keys here that were user in the 'order' section of this array.
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
 * Retreives an object from the cache.
 *
 * @param string $name
 *  The name of the cached object to retreive.
 */
function omni_eve_api_get_page_cache($name)
{
  ctools_include('object-cache');
  $cache = ctools_object_cache_get('omni_eve_api', $name);

  // If the cached object doesn't exist yet, create an empty object.
  if (!$cache)
  {
    $cache = new stdClass();
    $cache -> locked = ctools_object_cache_test('omni_eve_api', $name);
  }

  return $cache;
}

/**
 * Creates or updates an object in the cache.
 *
 * @param string $name
 *  The name of the object to cache.
 *
 * @param object $data
 *  The object to be cached.
 */
function omni_eve_api_set_page_cache($name, $data)
{
  ctools_include('object-cache');
  $cache = ctools_object_cache_set('omni_eve_api', $name, $data);
}

/**
 * Removes an item from the object cache.
 *
 * @param string $name
 *  The name of the object to destroy.
 */
function omni_eve_api_clear_page_cache($name)
{
  ctools_include('object-cache');
  ctools_object_cache_clear('omni_eve_api', $name);
}

/**
 * Callback executed when the 'next' button is clicked.
 */
function omni_eve_api_subtask_next(&$form_state)
{
  // Store submitted data in a ctools cache object, namespaced 'signup'.
  omni_eve_api_set_page_cache('signup', $form_state['values']);
}

/**
 * Callback executed when the 'cancel' button is clicked.
 */
function omni_eve_api_subtask_cancel(&$form_state)
{
  // Clear our ctools cache object. It's good housekeeping.
  omni_eve_api_clear_page_cache('signup');
}

/**
 * Callback executed when the entire form submission is finished.
 */
function omni_eve_api_subtask_finish(&$form_state)
{
  // Clear our Ctool cache object.
  omni_eve_api_clear_page_cache('signup');

  // Redirect the user to the front page.
  drupal_goto('<front>');
}

/**
 * Implimentation of hook_form_FORM_ID_alter
 */
function omni_eve_api_form_user_profile_form_alter(&$form, $form_state)
{
  $form['charactername'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => '',
    '#options' => array('1' => '1', '2' => '2'),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );

  return $form;
}

/**
 * Implements hook_form_FORM_ID_alter().
 */
function omni_eve_api_form_user_register_form_alter(&$form, &$form_state)
{
  unset($form['actions']);

  $form['account']['#type'] = 'fieldset';
  $form['account']['#title'] = t('Blue Status Verified');
  $form['account']['#description'] = t('Please select your main character from the drop down box.  This character will be used to identify yourself throughout the site and various tools.');

  $form['#validate'] = array(
    'user_register_validate',
    'omni_eve_api_user_register_validate',
  );

  $form['#submit'] = array(
    'user_register_submit',
    'omni_eve_api_user_register_submit',
    'ctools_wizard_submit',
  );

  $query = array(
    'keyid' => $form_state['signup_object']['keyid'],
    'vcode' => $form_state['signup_object']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (is_null($characters))
  {
/**
 * Redirect to original registration page, give error message via form_set_error is possible ?
 */
  }

  $form['account']['charactername'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => '',
    '#options' => omni_eve_api_valid_characters($characters),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );
}

/**
 * First page of the multipage registration process
 */
function omni_eve_api_enter_api_form($form, &$form_state)
{
  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Verify Blue Status'),
    '#description' => t('Please enter your EVE API in order to verify you qualify for an account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['enter_api']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=268435455">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['enter_api']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=268435455">here</a>.'),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
  );

  $form['#validate'][] = 'omni_eve_api_enter_api_form_validate';
  $form['#submit'][] = 'omni_eve_api_enter_api_form_submit';

  return $form;
}

/**
 * Verify the API one more time before creating an account
 */
function omni_eve_api_user_register_validate($form, &$form_state)
{
  //Possible fix for signature field on user creation being created as "Array"
  $form_state['input']['signature'] = $form_state['value']['signature'] = '';

  $query = array(
    'keyid' => $form_state['signup_object']['keyid'],
    'vcode' => $form_state['signup_object']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (!$characters['expires'] && $characters['type'] == 'Account' && $characters['accessMask'] == 268435455)
  {
    if (!omni_eve_api_verify_blue($characters))
    {
      form_set_error('keyid', 'No characters associated with your key are currently blue.');
      form_set_error('vcode');
    }
  }
  else
  {
    form_set_error('keyid', 'Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.');
    form_set_error('vcode');
  }
}

/**
 * Custom submit function for the user registration to add the character name to a custom field
 */
function omni_eve_api_user_register_submit($form, &$form_state)
{
  $uid = $form_state['user'] -> uid;
  $keyid = $form_state['signup_object']['keyid'];
  $vcode = $form_state['signup_object']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  $nid = db_insert('oea_api_keys')
    ->fields(array(
      'drupalid' => $uid,
      'keyid' => $keyid,
      'vcode' => $vcode,
    ))
    ->execute();
  /**
   * Add function to insert characters and link to api table
   */
}

/**
 * Custom validation function for the API verification
 */
function omni_eve_api_enter_api_form_validate($form, &$form_state)
{
  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (!$characters['expires'] && $characters['type'] == 'Account' && $characters['accessMask'] == 268435455)
  {
    if (!omni_eve_api_verify_blue($characters))
    {
      form_set_error('keyid', 'No characters associated with your key are currently blue.');
      form_set_error('vcode');
    }
  }
  else
  {
    form_set_error('keyid', 'Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.');
    form_set_error('vcode');
  }
}

/**
 * Custom submit function for the API verification
 */
function omni_eve_api_enter_api_form_submit($form, &$form_state)
{

}
