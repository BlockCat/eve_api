<?php

/**
 * Implements hook_admin().
 */
function omni_eve_api_admin()
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

  return system_settings_form($form);
}

/**
 * Implements hook_admin_validate().
 */
function omni_eve_api_admin_validate($form, &$form_state)
{
  if (user_role_load((int)$form_state['values']['omni_eve_api_blue_role']) == TRUE)
  {
    variable_set('omni_eve_api_blue_role', (int)$form_state['values']['omni_eve_api_blue_role']);
  }
  else
  {
    form_set_error('omni_eve_api_blue_role', 'The role does not exist.');
  }

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
 * Custom function to get list of all Roles
 */
function _omni_eve_api_role_list()
{
  $roles = array();

  $result = db_select('role', 'r') -> fields('r', array(
    'rid',
    'name',
  )) -> execute();

  if ($result -> fetchField())
  {
    foreach ($result->fetchAll() as $row)
    {
      $roles[$row -> rid] = check_plain($row -> name);
    }
  }

  return $roles;
}

/**
 * Implements hook_user_role_delete().
 */
function omni_eve_api_user_role_delete($role)
{
  if ($role->rid == variable_get('omni_eve_api_blue_role'))
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
 * Add function to show all API's the user currently has and delete them via selection
 */

/**
 * Add function to add a new API
 */

/**
 * Add function to select a new main character
 */
