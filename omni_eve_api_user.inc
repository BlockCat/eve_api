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
 * Override template_preprocess_user_picture() to display user pictures with
 * EVE Online integration.
 *
 * Example URL's
 * http://image.eveonline.com/Character/1643072492_128.jpg
 * http://image.eveonline.com/Character/1_128.jpg
 * 
 * @see template_preprocess_user_picture()
 * @see _gravatar_load_account()
 * @see _gravatar_get_account_user_picture()
 */
function omni_eve_api_preprocess_user_picture(&$variables) {
  $variables['user_picture'] = '';

  if (variable_get('user_pictures', 0)) {
    // Load the full user object since it is not provided with nodes, comments,
    // or views displays.
    $account = _omni_eve_api_load_account($variables['account']);
    $filepath = _omni_eve_api_get_account_user_picture($account);

    if (!empty($filepath)) {
      $alt = t("@user's picture", array('@user' => format_username($account)));
      if (module_exists('image') && file_valid_uri($filepath) && $style = variable_get('user_picture_style', '')) {
        $variables['user_picture'] = theme('image_style', array('style_name' => $style, 'path' => $filepath, 'alt' => $alt, 'title' => $alt));
      }
      else {
        $variables['user_picture'] = theme('image', array('path' => $filepath, 'alt' => $alt, 'title' => $alt));
      }
      if ($account->uid && user_access('access user profiles')) {
        // Create link to the user's profile.
        $attributes = array('title' => t('View user profile.'));
        $variables['user_picture'] = l($variables['user_picture'], 'user/' . $account->uid, array('attributes' => $attributes, 'html' => TRUE));
      }
      elseif (!empty($account->homepage)) {
        // If user is anonymous, create link to the commenter's homepage.
        $attributes = array(
          'title' => t('View user website.'),
          'rel' => 'external nofollow',
        );
        $variables['user_picture'] = l($variables['user_picture'], $account->homepage, array('attributes' => $attributes, 'html' => TRUE));
      }
    }
  }
}

/**
 * Original function from"Gravatar v1.1" by Narno, Git Migration, Dave Reid, podarok, and macedigital
 * Unmodified function.
 */
function _omni_eve_api_load_account($account) {
  // If this is a node or comment object, load the user object.
  if (!empty($account->nid) || !empty($account->cid) || empty($account->roles)) {
    $original_values = $account;

    // If a comment is being edited and previewed, the $account->uid is NULL.
    // @todo Remove when http://drupal.org/node/334826 is fixed in 6.x.
    if (!isset($account->uid)) {
      $account->uid = 0;
    }

    $account = $account->uid ? user_load($account->uid) : drupal_anonymous_user();

    // Load mail/homepage variable from an anonymous comment.
    if (!$account->uid) {
      $values = array_fill_keys(array('name', 'mail', 'homepage', 'hostname'), '');
      foreach ($values as $value => $default_value) {
        if (empty($account->$value)) {
          $account->$value = !empty($original_values->$value) ? $original_values->$value : $default_value;
        }
      }
    }
  }

  if (isset($account->picture) && is_numeric($account->picture)) {
    $account->picture = file_load($account->picture);
  }

  return $account;
}

/**
 * Original function from"Gravatar v1.1" by Narno, Git Migration, Dave Reid, podarok, and macedigital
 * 
 * Modified for use with "Omni EVE API"
 * 
 * Decide which user picture should be displayed for a user account.
 *
 * @param $account
 *   A user object.
 * @return
 *   A string with the path to the user's picture.
 */
function _omni_eve_api_get_account_user_picture($account) {
  if (!empty($account->picture->uri)) {
    // If the user has an uploaded picture, use it first.
    return $account->picture->uri;
  }
  elseif (empty($user -> characterid)) {
    return omni_eve_api_get_eve_avatar('1');
  }
  else {
    return omni_eve_api_get_eve_avatar($user -> characterid);
  }
}

/**
 * Generate a EVE Avatar URL.
 *
 * @param $characterid
 *   A string with an EVE character ID.
 * @return
 *   An URL string with the EVE avatar image.
 */
function omni_eve_api_get_eve_avatar($characterid) {
  global $is_https;

  $eve_avatar = $is_https ? variable_get('eve_avatar_https') : variable_get('eve_avatar_http');
  $eve_avatar .= $characterid . '_'  . variable_get('eve_user_picture_dimensions') . '.jpg';

  return url($eve_avatar);
}

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
