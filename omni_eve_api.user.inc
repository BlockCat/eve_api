<?php
/**
 * @file
 * Functions related to allowing users to alter the user settings and options.
 */

/**
 * Implements hook_user_delete().
 */
function omni_eve_api_user_delete($account) {
  db_delete('oea_characters')->condition('drupalid', $account->uid)->execute();
  db_delete('oea_api_keys')->condition('drupalid', $account->uid)->execute();
}

/**
 * Override template_preprocess_user_picture().
 *
 * Override template_preprocess_user_pictures() to display user pictures with
 * EVE Online integrationExample URL's.
 * http://image.eveonline.com/Character/1643072492_128.jpg
 * http://image.eveonline.com/Character/1_128.jpg
 *
 * @see template_preprocess_user_picture()
 * @see omni_eve_api_load_account()
 * @see omni_eve_api_get_account_user_picture()
 */
function omni_eve_api_preprocess_user_picture(&$variables) {
  $variables['user_picture'] = '';

  if (variable_get('user_pictures', 0)) {
    // Load the full user object since it is not provided with nodes, comments,
    // or views displays.
    $account = omni_eve_api_load_account($variables['account']);
    $filepath = omni_eve_api_get_account_user_picture($account);

    if (!empty($filepath)) {
      $alt = t("@user's picture", array('@user' => format_username($account)));
      if (module_exists('image') && file_valid_uri($filepath) && $style = variable_get('user_picture_style', '')) {
        $variables['user_picture'] = theme('image_style', array(
          'style_name' => $style,
          'path' => $filepath,
          'alt' => $alt,
          'title' => $alt,
        ));
      }
      else {
        $variables['user_picture'] = theme('image', array(
          'path' => $filepath,
          'alt' => $alt,
          'title' => $alt,
        ));
      }
      if ($account->uid && user_access('access user profiles')) {
        // Create link to the user's profile.
        $attributes = array('title' => t('View user profile.'));
        $variables['user_picture'] = l($variables['user_picture'], 'user/' . $account->uid, array(
          'attributes' => $attributes,
          'html' => TRUE,
        ));
      }
      elseif (!empty($account->homepage)) {
        // If user is anonymous, create link to the commenter's homepage.
        $attributes = array(
          'title' => t('View user website.'),
          'rel' => 'external nofollow',
        );
        $variables['user_picture'] = l($variables['user_picture'], $account->homepage, array(
          'attributes' => $attributes,
          'html' => TRUE,
        ));
      }
    }
  }
}

/**
 * Unmodified function from"Gravatar v1.1".
 * 
 * By Narno, Git Migration, Dave Reid, podarok, and macedigital.
 *
 * @param array $account
 *   A simple user object.
 * 
 * @return array
 *   A full user object.
 */
function omni_eve_api_load_account($account) {
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
      $values = array_fill_keys(array(
        'name',
        'mail',
        'homepage',
        'hostname',
      ), '');
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
 * Original function from"Gravatar v1.1".
 * 
 * by Narno, Git Migration, Dave Reid, podarok, and macedigital.
 * Modified for use with "Omni EVE API".
 *
 * Decide which user picture should be displayed for a user account.
 *
 * @param array $account
 *   A user object.
 * 
 * @return string
 *   A string with the path to the user's picture.
 */
function omni_eve_api_get_account_user_picture($account) {
  $user = user_load($account->uid);

  if (!empty($account->picture->uri)) {
    // If the user has an uploaded picture, use it first.
    return $account->picture->uri;
  }
  elseif (empty($user->characterid)) {
    return omni_eve_api_get_eve_avatar(1);
  }
  else {
    return omni_eve_api_get_eve_avatar($user->characterid);
  }
}

/**
 * Generate a EVE Avatar URL.
 *
 * @param int $characterid
 *   A string with an EVE character ID.
 * 
 * @return string
 *   An URL string with the EVE avatar image.
 */
function omni_eve_api_get_eve_avatar($characterid) {
  global $is_https;

  $eve_avatar = $is_https ? variable_get('eve_avatar_https', 'https://image.eveonline.com/Character/') : variable_get('eve_avatar_http', 'http://image.eveonline.com/Character/');
  $eve_avatar .= $characterid . '_' . variable_get('eve_user_picture_dimensions', '128x128') . '.jpg';

  return url($eve_avatar);
}

/**
 * Form constructor for the user add api form.
 *
 * @see omni_eve_api_user_add_api_form_validate()
 * @see omni_eve_api_user_add_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_add_api_form($form, &$form_state, $account) {
  global $user;

  if ($account->uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $uid = $account->uid;

  $form['enter_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Verify Blue Status'),
    '#description' => t('Please enter your EVE API in order to add it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['enter_api']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@mask">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#weight' => 0,
  );

  $form['enter_api']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@mask">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#weight' => 10,
  );

  $form['enter_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Add API Key'),
  );

  $form['#validate'] = array('omni_eve_api_user_add_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_add_api_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_add_api_form().
 *
 * @see omni_eve_api_user_add_api_form_submit()
 */
function omni_eve_api_user_add_api_form_validate($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

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

    if ($chars = omni_eve_api_characters_exist($characters)) {
      form_set_error('keyid', t('Characters on this key have already been registered. Characters registered: @chars', array('@chars' => implode(", ", $chars))));
      form_set_error('vcode');
    }
  }
}

/**
 * Form submission handler for omni_eve_api_user_add_api_form().
 *
 * @see omni_eve_api_user_add_api_form_validate()
 */
function omni_eve_api_user_add_api_form_submit($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];

  $query = array(
    'keyid' => $keyid,
    'vcode' => $vcode,
  );

  $characters = omni_eve_api_get_character_api($query);

  if (isset($characters['error'])) {
    drupal_set_message(t('There was an error with the API.'), 'error');
  }
  else {
    $created = date('Y-m-d H:i:s', time());

    $new_api_key_id = db_insert('oea_api_keys')->fields(array(
      'drupalid' => $uid,
      'keyid' => $keyid,
      'vcode' => $vcode,
      'updated' => $created,
      'created' => $created,
    ))->execute();

    foreach ($characters['characters'] as $character) {
      db_merge('oea_characters')->key(array('characterid' => $character['characterID']))->fields(array(
        'characterid' => $character['characterID'],
        'drupalid' => $uid,
        'apiid' => $new_api_key_id,
        'charactername' => $character['characterName'],
        'corporationid' => $character['corporationID'],
        'corporationname' => $character['corporationName'],
        'corporationticker' => $character['corporationTicker'],
        'allianceid' => $character['allianceID'],
        'alliancename' => $character['allianceName'],
        'allianceticker' => $character['allianceTicker'],
        'deleted' => 0,
      ))->execute();
    }

    drupal_set_message(t('API Key successfully added!'));
  }
}

/**
 * Form constructor for the user select main character form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see omni_eve_api_user_select_main_character_form_validate()
 * @see omni_eve_api_user_select_main_character_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_select_main_character_form($form, &$form_state, $account) {
  global $user;

  if ($account->uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $uid = $account->uid;
  $uid_api_list = omni_eve_api_list_api($uid);

  $form['main_character'] = array(
    '#type' => 'fieldset',
    '#title' => t('Valid Characters'),
    '#weight' => 0,
  );

  $form['main_character']['select_charactername'] = array(
    '#type' => 'select',
    '#title' => t('Select your Main Character'),
    '#default_value' => $account->charactername,
    '#options' => omni_eve_api_list_valid_characters($uid_api_list['simple']),
    '#description' => t('Detected valid Main Characters.'),
    '#required' => TRUE,
  );

  $form['main_character']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Submit'),
  );

  $form['#validate'] = array('omni_eve_api_user_select_main_character_form_validate');
  $form['#submit'] = array('omni_eve_api_user_select_main_character_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_select_main_character_form().
 *
 * @see omni_eve_api_user_select_main_character_form_submit()
 */
function omni_eve_api_user_select_main_character_form_validate($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }
}

/**
 * Form submission handler for omni_eve_api_user_select_main_character_form().
 *
 * @see omni_eve_api_user_select_main_character_form_validate()
 */
function omni_eve_api_user_select_main_character_form_submit($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $account = user_load($uid);

  if ($account->charactername != $form_state['values']['select_charactername'] && !empty($form_state['values']['select_charactername'])) {
    $old_charactername = $account->charactername;
    $old_character_data = omni_eve_api_get_character_info($old_charactername);
    $charactername = $form_state['values']['select_charactername'];
    $character_data = omni_eve_api_get_character_info($charactername);

    $old_rid_2 = NULL;

    if ($corporation_role = user_role_load_by_name($old_character_data['corporationName'])) {
      $old_rid = $corporation_role->rid;

      if ($alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2))) {
        $old_rid_2 = $alliance_role->rid;
      }
    }
    elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2))) {
      $old_rid = $blue_role->rid;
    }

    user_multiple_role_edit(array($uid), 'remove_role', $old_rid);

    if (isset($old_rid_2)) {
      user_multiple_role_edit(array($uid), 'remove_role', $old_rid_2);
    }

    db_update('users')->fields(array('characterid' => $character_data['characterID'], 'charactername' => $character_data['characterName']))->condition('uid', $uid, '=')->execute();

    $new_rid_2 = NULL;

    if ($corporation_role = user_role_load_by_name($character_data['corporationName'])) {
      $new_rid = $corporation_role->rid;

      if ($alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2))) {
        $new_rid_2 = $alliance_role->rid;
      }
    }
    elseif ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2))) {
      $new_rid = $blue_role->rid;
    }
    else {
      $default_role = user_role_load_by_name('authenticated user');
      $new_rid = $default_role->rid;
    }

    user_multiple_role_edit(array($uid), 'add_role', $new_rid);

    if (isset($new_rid_2)) {
      user_multiple_role_edit(array($uid), 'add_role', $new_rid_2);
    }

    $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
    $queue->createQueue();
    $queue->createItem($uid);

    drupal_set_message(t('New main character selected!'));
  }
}

/**
 * Form constructor for the user list api form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see omni_eve_api_user_list_api_form_validate()
 * @see omni_eve_api_user_list_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_list_api_form($form, &$form_state, $account) {
  global $user;

  if ($account->uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $uid = $account->uid;
  $uid_api_list = omni_eve_api_list_api($uid);

  $header = array(
    'edit' => array(
      'data' => t('Edit'),
      'field' => 'n.edit',
    ),
    'keyid' => array(
      'data' => t('API Key ID'),
      'field' => 'n.keyid',
    ),
    'vcode' => array(
      'data' => t('API Verification Code'),
      'field' => 'n.vcode',
    ),
    'characters' => array(
      'data' => t('Characters'),
      'field' => 'n.characters',
    ),
    'updated' => array(
      'data' => t('Last Updated'),
      'field' => 'n.updated',
    ),
    'created' => array(
      'data' => t('Created'),
      'field' => 'n.created',
    ),
    'status' => array(
      'data' => t('Status'),
      'field' => 'n.status',
    ),
  );

  $options = array();

  foreach ($uid_api_list['full'] as $uid_api_single) {
    $list_characters = '';

    if (isset($uid_api_single['characters'])) {
      foreach ($uid_api_single['characters'] as $characters) {
        $corp_alliance = ($characters['allianceid'] == 0) ? $characters['corporationname'] : $characters['corporationname'] . '/' . $characters['alliancename'];
        $list_characters .= '[' . $characters['corporationticker'] . '] ' . $characters['charactername'] . ' (' . $corp_alliance . ')';
        $list_characters .= '<br />';
      }
    }

    $options[$uid_api_single['apiid']] = array(
      'edit' => array(
        'data' => array(
          '#type' => 'link',
          '#title' => t('Edit API # @api_key_id', array('@api_key_id' => $uid_api_single['apiid'])),
          '#href' => 'user/' . $uid . '/omni_eve_api/update_api_key/' . $uid_api_single['apiid'],
        ),
      ),
      'keyid' => $uid_api_single['keyid'],
      'vcode' => $string = (strlen($uid_api_single['vcode']) > 13) ? substr($uid_api_single['vcode'], 0, 10) . '...' : $uid_api_single['vcode'],
      'characters' => $list_characters,
      'updated' => format_date(strtotime($uid_api_single['updated']), 'short'),
      'created' => format_date(strtotime($uid_api_single['created']), 'short'),
      'status' => $uid_api_single['errorid'],
    );
  }

  $form['list_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['list_api']['api_keys'] = array(
    '#type' => 'tableselect',
    '#header' => $header,
    '#options' => $options,
    '#empty' => t('No API Keys associated with your account.'),
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Delete'),
  );

  $form['#validate'] = array('omni_eve_api_user_list_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_list_api_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_list_api_form().
 *
 * @see omni_eve_api_user_list_api_form_submit()
 */
function omni_eve_api_user_list_api_form_validate($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }
}

/**
 * Form submission handler for omni_eve_api_user_list_api_form().
 *
 * @see omni_eve_api_user_list_api_form_validate()
 */
function omni_eve_api_user_list_api_form_submit($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  foreach ($form_state['values']['api_keys'] as $api_key_id) {
    if ($api_key_id == 0) {
      continue;
    }

    db_delete('oea_api_keys')->condition('apiid', $api_key_id)->execute();
    db_update('oea_characters')->fields(array('deleted' => 1))->condition('apiid', $api_key_id)->execute();

    drupal_set_message(t('API # @api_key_id  successfully deleted!', array('@api_key_id' => $api_key_id)));
  }

  $query = db_select('oea_characters', 'c');
  $query->condition('c.deleted', 0);
  $query->join('users', 'u', 'c.charactername = u.charactername');
  $query->fields('c', array(
    'charactername',
    'characterid',
  ));
  $query->condition('u.uid', $uid);
  $result = $query->execute();

  if (!$result->rowCount()) {
    $result = db_select('oea_alliance_corporations', 'ac')->fields('ac', array(
      'corporationid',
      'corporationname',
    ))->condition('deleted', 0, '=')->execute();

    if ($result->rowCount()) {
      foreach ($result->fetchAll() as $row) {
        if ($corporation_role = user_role_load_by_name($row->corporationname)) {
          user_multiple_role_edit(array($uid), 'remove_role', $corporation_role->rid);
        }
      }
    }

    if ($alliance_role = user_role_load(variable_get('omni_eve_api_alliance_role', 2))) {
      user_multiple_role_edit(array($uid), 'remove_role', $alliance_role->rid);
    }

    if ($blue_role = user_role_load(variable_get('omni_eve_api_blue_role', 2))) {
      user_multiple_role_edit(array($uid), 'remove_role', $blue_role->rid);
    }

    db_update('users')->fields(array(
      'charactername' => '',
      'characterid' => 0,
    ))->condition('uid', $uid, '=')->execute();
    $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
    drupal_set_message(t('Please select a new Main Character'));
  }

  $queue = DrupalQueue::get('omni_eve_api_cron_queue_users');
  $queue->createQueue();
  $queue->createItem($uid);
}

/**
 * Form constructor for the user update api form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 * @param string $api_key_id
 *   The unique api_key_id that is being updated.
 *
 * @see omni_eve_api_user_update_api_form_validate()
 * @see omni_eve_api_user_update_api_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_update_api_form($form, &$form_state, $account, $api_key_id) {
  global $user;

  if ($account->uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $uid = $account->uid;

  $result = db_select('oea_api_keys', 'a')->fields('a', array(
    'keyid',
    'vcode',
  ))->condition('apiid', $api_key_id, '=')->execute();

  if (!$result->rowCount()) {
    drupal_goto('user/' . $uid . '/omni_eve_api');
  }

  $row = $result->fetchAssoc();

  $form['update_api'] = array(
    '#type' => 'fieldset',
    '#title' => t('Update API'),
    '#description' => t('Please enter your EVE API in order to update it to your existing account.  A Full API is required for registration. Please ensure that the "Character" drop down box is set to "All", the "Type" drop down box is set to "Character", and that the "No Expiry" checkbox is ticked.'),
    '#weight' => 0,
  );

  $form['update_api']['api_key_id'] = array(
    '#value' => $api_key_id,
    '#type' => 'hidden',
  );

  $form['update_api']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['update_api']['keyid'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@mask">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 15,
    '#default_value' => isset($form_state['values']['keyid']) ? $form_state['values']['keyid'] : $row['keyid'],
    '#weight' => 0,
  );

  $form['update_api']['vcode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@mask">here</a>.', array('@mask' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=' . variable_get('omni_eve_api_access_mask', 268435455))),
    '#required' => TRUE,
    '#maxlength' => 80,
    '#default_value' => isset($form_state['values']['vcode']) ? $form_state['values']['vcode'] : $row['vcode'],
    '#weight' => 10,
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Update API Key'),
  );

  $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api';
  $form['#validate'] = array('omni_eve_api_user_update_api_form_validate');
  $form['#submit'] = array('omni_eve_api_user_update_api_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_update_api_form().
 *
 * @see omni_eve_api_user_update_api_form_submit()
 */
function omni_eve_api_user_update_api_form_validate($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];
  $api_key_id = $form_state['values']['api_key_id'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

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
    $form_state['values']['characters'] = $characters;

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
  }
}

/**
 * Form submission handler for omni_eve_api_user_update_api_form().
 *
 * @see omni_eve_api_user_update_api_form_validate()
 */
function omni_eve_api_user_update_api_form_submit($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $keyid = $form_state['values']['keyid'];
  $vcode = $form_state['values']['vcode'];
  $api_key_id = $form_state['values']['api_key_id'];
  $characters = $form_state['values']['characters'];

  $updated = date('Y-m-d H:i:s', time());

  db_update('oea_characters')->fields(array('deleted' => 1))->condition('apiid', $api_key_id)->execute();

  db_merge('oea_api_keys')->key(array('apiid' => $api_key_id))->fields(array(
    'apiid' => $api_key_id,
    'drupalid' => $uid,
    'keyid' => $keyid,
    'vcode' => $vcode,
    'updated' => $updated,
  ))->execute();

  foreach ($characters['characters'] as $character) {
    db_merge('oea_characters')->key(array('characterid' => $character['corporationID']))->fields(array(
      'characterid' => $character['characterID'],
      'drupalid' => $uid,
      'apiid' => $api_key_id,
      'charactername' => $character['characterName'],
      'corporationid' => $character['corporationID'],
      'corporationname' => $character['corporationName'],
      'corporationticker' => $character['corporationTicker'],
      'allianceid' => $character['allianceID'],
      'alliancename' => $character['allianceName'],
      'allianceticker' => $character['allianceTicker'],
      'deleted' => 0,
    ))->execute();
  }

  $query = db_select('oea_characters', 'c');
  $query->condition('c.deleted', 0);
  $query->join('users', 'u', 'c.charactername = u.charactername');
  $query->fields('c', array(
    'charactername',
    'characterid',
  ));
  $query->condition('u.uid', $uid);
  $result = $query->execute();

  if (!$result->rowCount()) {
    db_update('users')->fields(array(
      'charactername' => '',
      'characterid' => 0,
    ))->condition('uid', $uid, '=')->execute();
    $form_state['redirect'] = 'user/' . $uid . '/omni_eve_api/select_main_character';
    drupal_set_message(t('Please select a new Main Character'));
  }

  drupal_set_message(t('API Key successfully updated!'));
}


/**
 * Form constructor for the user select main character form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see omni_eve_api_user_teamspeak_form_validate()
 * @see omni_eve_api_user_teamspeak_form_submit()
 *
 * @ingroup forms
 */
function omni_eve_api_user_teamspeak_form($form, &$form_state, $account) {
  global $user;

  if ($account->uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $uid = $account->uid;
  $uid_api_list = omni_eve_api_list_api($uid);

  $form['teamspeak'] = array(
    '#type' => 'fieldset',
    '#title' => t('Register your Teamspeak Identity to your User Account'),
    '#weight' => 0,
  );

  $form['teamspeak']['howto'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => t('In order to register with Teamspeak, you must first connect to our Teamspeak.<br />If you do not have Teamspeak already installed, please download and install Teamspeak 3 from <a href="@teamspeak_url">here</a>.<br />Please use the following settings to connect:<br />Address - @teamspeak_address<br />Port - @teamspeak_port<br />Password - @teamspeak_password', array(
      '@teamspeak_url' => 'http://www.teamspeak.com/?page=downloads',
      '@teamspeak_address' => variable_get('omni_eve_api_ts3_server_address', 'teamspeak.com'),
      '@teamspeak_port' => variable_get('omni_eve_api_ts3_server_port', 9987),
      '@teamspeak_password' => variable_get('omni_eve_api_ts3_server_password', ''),
    )),
    '#suffix' => '</p>',
    '#weight' => 0,
  );

  $form['teamspeak']['teamspeak_name'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => !empty($account->charactername) ? t('Teamspeak Name: @teamspeak', array('@teamspeak' => omni_eve_api_teamspeak_format_name($account))) : t('Teamspeak Name: <a href="@url">Please select a main character.</a>', array('@url' => url('user/' . $uid . '/omni_eve_api/select_main_character'))),
    '#suffix' => '</p>',
    '#weight' => 10,
  );

  $form['teamspeak']['teamspeak_uid'] = array(
    '#type' => 'markup',
    '#prefix' => '<p>',
    '#markup' => !empty($account->teamspeakuid) ? t('Teamspeak Unique ID: @teamspeak', array('@teamspeak' => $account->teamspeakuid)) : t('Teamspeak Unique ID: You are not registered.'),
    '#suffix' => '</p>',
    '#weight' => 20,
  );

  $form['teamspeak']['uid'] = array(
    '#value' => $uid,
    '#type' => 'hidden',
  );

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Register'),
  );

  $form['#validate'] = array('omni_eve_api_user_teamspeak_form_validate');
  $form['#submit'] = array('omni_eve_api_user_teamspeak_form_submit');

  return $form;
}

/**
 * Form validation handler for omni_eve_api_user_teamspeak_form().
 *
 * @see omni_eve_api_user_teamspeak_form_submit()
 */
function omni_eve_api_user_teamspeak_form_validate($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $account = user_load($uid);

  try {
    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_name = omni_eve_api_teamspeak_format_name($account);
    $teamspeakuid = omni_eve_api_teamspeak_get_by_name($ts3, $teamspeak_name);
  }
  catch (Exception $e) {
    form_set_error('teamspeak', $e->getMessage());
  }
}

/**
 * Form submission handler for omni_eve_api_user_teamspeak_form().
 *
 * @see omni_eve_api_user_teamspeak_form_validate()
 */
function omni_eve_api_user_teamspeak_form_submit($form, &$form_state) {
  global $user;

  $uid = $form_state['values']['uid'];

  if ($uid != $user->uid && !user_access('moderate omni_eve_api users')) {
    drupal_goto('<front>');
  }

  $account = user_load($uid);

  try {
    $ts3 = omni_eve_api_teamspeak_connection();
    $teamspeak_name = omni_eve_api_teamspeak_format_name($account);
    $teamspeakuid = omni_eve_api_teamspeak_get_by_name($ts3, $teamspeak_name);

    db_update('users')->fields(array(
        'teamspeakuid' => $teamspeakuid,
      ))->condition('uid', $uid, '=')->execute();

    // Need to assign roles.
    drupal_set_message(t('Teamspeak access granted.'));
  }
  catch (Exception $e) {
    drupal_set_message($e->getMessage(), 'error');
  }
}
