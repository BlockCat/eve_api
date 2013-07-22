<?php

/**
 * Implements hook_user_delete().
 */
function omni_eve_api_user_delete($account)
{
  db_delete('oea_characters') -> condition('drupalid', $account -> uid) -> execute();
  db_delete('oea_api_keys') -> condition('drupalid', $account -> uid) -> execute();
}

/**
 * Find a way to modify avatars ?
 *
 * http://image.eveonline.com/Character/1643072492_256.jpg
 */


/**
 * Function that retrieves all API's associated with a $uid
 */
function omni_eve_api_list_api($uid)
{
  $api_info = array();

  $result_a = db_select('oea_api_keys', 'a') -> fields('a', array(
    'accountid',
    'keyid',
    'vcode',
    'errorid',
    'updated',
  )) -> condition('drupalid', $uid, '=') -> execute();

  if ($result_a -> fetchField())
  {
    foreach ($result_a->fetchAll() as $row_a)
    {
			$api_info[$row_a -> accountid] = array(
		    'accountid' => $row_a -> accountid,
		    'keyid' => $row_a -> keyid,
		    'vcode' => $row_a -> vcode,
		    'errorid' => $row_a -> errorid,
		    'updated' => $row_a -> updated,
			);

      $result_b = db_select('oea_characters', 'c') -> fields('c', array(
        'characterid',
        'charactername',
        'corporationid',
        'corporationname',
        'corporationticker',
        'allianceid',
        'alliancename',
        'allianceticker',
      )) -> condition('drupalid', $uid, '=') -> condition('accountid', $row_a -> accountid, '=') -> execute();

      if ($result_b -> fetchField())
      {
        foreach ($result_b->fetchAll() as $row_b)
        {
			    $api_info[$row_a -> accountid]['characters'][$row_b -> characterid] = array(
			      'characterid' => $row_b -> characterid,
			      'charactername' => $row_b -> charactername,
			      'corporationid' => $row_b -> corporationid,
			      'corporationname' => $row_b -> corporationname,
		        'corporationticker' => $row_b -> corporationticker,
		        'allianceid' => $row_b -> allianceid,
		        'alliancename' => $row_b -> alliancename,
		        'allianceticker' => $row_b -> allianceticker,
			    );
        }
      }
    }
  }

  return $api_info;
}

/**
 * Add function to show all API's the user currently has and delete them via selection
 */
function omni_eve_api_list_api_form()
{
  global $user;
  $uid = $user -> uid;
  $form = array();

  $uid_api_info = omni_eve_api_list_api($uid);

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
 * Add function to add a new API
 */

/**
 * Add function to select a new main character
 */
