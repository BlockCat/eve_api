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
function omni_eve_api_preprocess_user_picture(&$variables)
{
  $variables['user_picture'] = '';

  if (variable_get('user_pictures', 0))
  {
    // Load the full user object since it is not provided with nodes, comments,
    // or views displays.
    $account = _omni_eve_api_load_account($variables['account']);
    $filepath = _omni_eve_api_get_account_user_picture($account);

    if (!empty($filepath))
    {
      $alt = t("@user's picture", array('@user' => format_username($account)));
      if (module_exists('image') && file_valid_uri($filepath) && $style = variable_get('user_picture_style', ''))
      {
        $variables['user_picture'] = theme('image_style', array(
          'style_name' => $style,
          'path' => $filepath,
          'alt' => $alt,
          'title' => $alt
        ));
      }
      else
      {
        $variables['user_picture'] = theme('image', array(
          'path' => $filepath,
          'alt' => $alt,
          'title' => $alt
        ));
      }
      if ($account -> uid && user_access('access user profiles'))
      {
        // Create link to the user's profile.
        $attributes = array('title' => t('View user profile.'));
        $variables['user_picture'] = l($variables['user_picture'], 'user/' . $account -> uid, array(
          'attributes' => $attributes,
          'html' => TRUE
        ));
      }
      elseif (!empty($account -> homepage))
      {
        // If user is anonymous, create link to the commenter's homepage.
        $attributes = array(
          'title' => t('View user website.'),
          'rel' => 'external nofollow',
        );
        $variables['user_picture'] = l($variables['user_picture'], $account -> homepage, array(
          'attributes' => $attributes,
          'html' => TRUE
        ));
      }
    }
  }
}

/**
 * Original function from"Gravatar v1.1" by Narno, Git Migration, Dave Reid, podarok, and macedigital
 * Unmodified function.
 */
function _omni_eve_api_load_account($account)
{
  // If this is a node or comment object, load the user object.
  if (!empty($account -> nid) || !empty($account -> cid) || empty($account -> roles))
  {
    $original_values = $account;

    // If a comment is being edited and previewed, the $account->uid is NULL.
    // @todo Remove when http://drupal.org/node/334826 is fixed in 6.x.
    if (!isset($account -> uid))
    {
      $account -> uid = 0;
    }

    $account = $account -> uid ? user_load($account -> uid) : drupal_anonymous_user();

    // Load mail/homepage variable from an anonymous comment.
    if (!$account -> uid)
    {
      $values = array_fill_keys(array(
        'name',
        'mail',
        'homepage',
        'hostname'
      ), '');
      foreach ($values as $value => $default_value)
      {
        if (empty($account -> $value))
        {
          $account -> $value = !empty($original_values -> $value) ? $original_values -> $value : $default_value;
        }
      }
    }
  }

  if (isset($account -> picture) && is_numeric($account -> picture))
  {
    $account -> picture = file_load($account -> picture);
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
function _omni_eve_api_get_account_user_picture($account)
{
  if (!empty($account -> picture -> uri))
  {
    // If the user has an uploaded picture, use it first.
    return $account -> picture -> uri;
  }
  elseif (empty($user -> characterid))
  {
    return omni_eve_api_get_eve_avatar('1');
  }
  else
  {
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
function omni_eve_api_get_eve_avatar($characterid)
{
  global $is_https;

  $eve_avatar = $is_https ? variable_get('eve_avatar_https') : variable_get('eve_avatar_http');
  $eve_avatar .= $characterid . '_' . variable_get('eve_user_picture_dimensions') . '.jpg';

  return url($eve_avatar);
}

/**
 * Function that retrieves all API's associated with a $uid
 */
function omni_eve_api_list_api($uid)
{
  $api_list = array();

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
      $api_list[$row_a -> accountid] = array(
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
          $api_list[$row_a -> accountid]['characters'][$row_b -> characterid] = array(
            'characterid' => $row_b -> characterid,
            'charactername' => $row_b -> charactername,
            'corporationid' => $row_b -> corporationid,
            'corporationname' => $row_b -> corporationname,
            'corporationticker' => $row_b -> corporationticker,
            'allianceid' => $row_b -> allianceid,
            'alliancename' => $row_b -> alliancename,
            'allianceticker' => $row_b -> allianceticker,
          );
          $api_list['characters'][$row_b -> characterid] = array(
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

  return $api_list;
}

/**
 * Add function to add a new API
 */
function omni_eve_api_user_add_api_form($form, &$form_state)
{
  $form = array();
  $form = omni_eve_api_enter_api_form($form, $form_state);

  $form['enter_api']['#title'] = t('Verify Blue Status');
  $form['enter_api']['#description'] = t('Please enter your EVE API in order to add it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.');

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add API Key'),
  );

  $form['#validate'] = array('omni_eve_api_user_add_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_add_api_form_submit');

  return $form;
}

/**
 * Custom validation function for adding additional API's for verification
 */
function omni_eve_api_user_add_api_form_validate($form, &$form_state)
{
  $query = array(
    'keyid' => $form_state['values']['keyid'],
    'vcode' => $form_state['values']['vcode'],
  );

  $characters = omni_eve_api_get_character_api($query);

  if (!$characters['expires'] && $characters['type'] == 'Account' && $characters['accessMask'] == 268435455)
  {
    if ($chars = omni_eve_api_characters_exist($characters))
    {
      form_set_error('keyid', 'Characters on this key have already been registered. Characters registered: ' . implode(", ", $chars));
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
 * Custom submit function for adding additional API's for submission
 */
function omni_eve_api_user_add_api_form_submit($form, &$form_state)
{
  global $user;
  $uid = $user -> uid;
  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  $new_api_key_id = db_insert('oea_api_keys') -> fields(array(
    'drupalid' => $uid,
    'keyid' => $keyid,
    'vcode' => $vcode,
  )) -> execute();

  foreach ($characters['characters'] as $character)
  {
    db_merge('oea_characters') -> key(array('characterid' => $character['corporationID'])) -> fields(array(
      'characterid' => $character['characterID'],
      'drupalid' => $uid,
      'accountid' => $new_api_key_id,
      'charactername' => $character['characterName'],
      'corporationid' => $character['corporationID'],
      'corporationname' => $character['corporationName'],
      'corporationticker' => $character['corporationTicker'],
      'allianceid' => $character['allianceID'],
      'alliancename' => $character['allianceName'],
      'allianceticker' => $character['allianceTicker'],
    )) -> execute();
  }

  ddrupal_set_message(t('API Key successfully added!'));
}

/**
 * Add function to show all API's the user currently has and delete them via selection
 */
function omni_eve_api_user_list_api_form($form, &$form_state)
{
  global $user;
  $uid = $user -> uid;
  $form = array();
/**
  $uid_api_list = omni_eve_api_list_api($uid);

  $form['list_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Your API\'s'),
    '#description' => t('Current list of API\'s associated with your account.'),
    '#weight' => 0,
  );

  $i = 0;

  foreach ($uid_api_list as $uid_api_single)
  {
    $form['list_api']['api_' . $i] = array(
      '#type' => 'fieldset',
      '#title' => t('API ' . $i),
      '#weight' => $i,
    );

    $form['list_api']['api_' . $i]['api_info'] = array(
      '#type' => 'markup',
      '#value' => 'Current list of API\'s associated with your account.',
      '#weight' => 0,
    );

    $form['list_api']['api_' . $i]['api_delete'] = array(
      '#type' => 'markup',
      '#value' => 'Current list of API\'s associated with your account.',
      '#weight' => 0,
    );

    $i++;
  }

 */
  $form['settings'] = array(
    '#type' => 'fieldset',
    '#title' => t('Settings'),
    '#weight' => 0,
  );

  $form['settings']['charactername'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => $user -> charactername,
    '#options' => omni_eve_api_valid_characters($uid_api_list),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['#validate'] = array('omni_eve_api_user_list_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_list_api_form_submit');

  return $form;
}

/**
 * Custom validation function for changing settings for verification
 */
function omni_eve_api_user_list_api_form_validate($form, &$form_state)
{

}

/**
 * Custom submit function for changing settings for submission
 */
function omni_eve_api_user_list_api_form_submit($form, &$form_state)
{
  global $user;
  $uid = $user -> uid;
  $account = user_load($uid);

  if (2 == 1)
  {
    ddrupal_set_message(t('API Key successfully deleted!'));
  }

  if ($user -> charactername != $form_state['values']['charactername'])
  {
    $old_charactername = $user -> charactername;
    $old_character_data = omni_eve_api_get_character_info($old_charactername);
    $charactername = $form_state['values']['charactername'];
    $character_data = omni_eve_api_get_character_info($charactername);
  
    if ($corporation_role = user_role_load_by_name($old_character_data['corporationName']))
    {
      $old_rid = $corporation_role -> rid;
    }
    elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role')))
    {
      $old_rid = $blue_role -> rid;
    }
  
    user_multiple_role_edit(array($uid), 'remove_role', $old_rid);
  
    db_update('users') -> fields(array('characterid' => $character_data['characterID'], )) -> condition('uid ', $uid, '=') -> execute();
    db_update('users') -> fields(array('charactername' => $character_data['characterName'], )) -> condition('uid ', $uid, '=') -> execute();
  
    if ($corporation_role = user_role_load_by_name($character_data['corporationName']))
    {
      $new_rid = $corporation_role -> rid;
    }
    else if ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role')))
    {
      $new_rid = $blue_role -> rid;
    }
    else
    {
      $default_role = user_role_load_by_name('authenticated user');
      $new_rid = $default_role -> rid;
    }
  
    user_multiple_role_edit(array($uid), 'add_role', $new_rid);
  
    ddrupal_set_message(t('New main character selected!'));
  }
}
