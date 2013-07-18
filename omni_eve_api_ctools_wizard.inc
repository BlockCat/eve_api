<?php

/**
 * Create callback for standard ctools registration wizard.
 */
function omni_eve_api_ctools_wizard($step = 'enter_api') {
	// Include required ctools files.
	ctools_include('wizard');
	ctools_include('object-cache');

	$form_info = array(
    	// Specify unique form id for this form.
    	'id' => 'multistep_registration',
    	//Specify the path for this form. It is important to include space for the $step argument to be passed.
    	'path' => "user/register/%step",
    	// Show breadcrumb trail.
    	'show trail' => TRUE, 'show back' => TRUE, 'show return' => FALSE,
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
 *  The name of the object to cache.
 *
 * @param object $data
 *  The object to be cached.
 */
function omni_eve_api_set_page_cache($name, $data) {
  ctools_include('object-cache');
  $cache = ctools_object_cache_set('omni_eve_api', $name, $data);
}
 
/**
 * Removes an item from the object cache.
 *
 * @param string $name
 *  The name of the object to destroy.
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

function omni_eve_api_enter_api_form($form, &$form_state) {
  $form['enter_api']['registration_info'] = array(
    '#markup' => t('Enter your EVE API in order to verify you qualify for an account.'),
    '#weight' => 10,
  );

	// Please enter your EVE API Key ID
	$form['enter_api']['keyid'] = array(
		'#type' => 'textfield',
		'#title' => t('Key ID'),
		'#description' => t('Please enter your Key ID from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=268435455">here</a>.'),
		'#required' => TRUE,
		'#weight' => 20,
	);

	// Please enter your EVE API Key ID
	$form['enter_api']['vcode'] = array(
		'#type' => 'textfield',
		'#title' => t('Verification Code'),
		'#description' => t('Please enter your Verification Code from the EVE API Page located <a href="http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=268435455">here</a>.'),
		'#required' => TRUE,
		'#weight' => 30,
	);

  $form['enter_api']['api_info'] = array(
    '#markup' => t('A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 40,
  );

  return $form;
}
