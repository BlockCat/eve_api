<?php
/**
 * @file
 * Functions related to allowing users to alter the user settings and options.
 */

/**
 * Form constructor for the user jabber registration form.
 *
 * @param string $account
 *   The account information for the user page being viewed.
 *
 * @see eve_api_jabber_user_form_validate()
 * @see eve_api_jabber_user_form_submit()
 *
 * @ingroup forms
 */
function eve_api_jabber_user_form($form, &$form_state, $account) {
  if (!isset($form_state['user'])) {
    $form_state['user'] = $account;
  }
  else {
    $account = $form_state['user'];
  }

  $uid = $account->uid;

  $form['jabber']['info'] = array(
    '#type' => 'item',
    '#title' => t('Register for a Jabber Account.'),
    '#description' => t('If you do not have a Jabber Client already installed, you can download and install @client from <a href="@client_url" target="_blank">here</a>.', array(
      '@client' => 'Trillian',
      '@client_url' => 'https://www.trillian.im/download/',
    )),
    '#weight' => 0,
  );

  $form['jabber']['username'] = array(
    '#type' => 'item',
    '#title' => t('Username:'),
    '#description' => ($account->characterID != 0) ? eve_api_jabber_format_name($account->name) . '@' . variable_get('eve_api_jabber_domain', 'yourdomain.com') : t('<a href="@url">Please select a main character.</a>', array('@url' => url('user/' . $uid . '/edit'))),
    '#weight' => 10,
  );

  if ($account->characterID != 0) {
    $form['jabber']['password'] = array(
      '#type' => 'textfield',
      '#title' => t('Set Password'),
      '#description' => t('You can set or change your Jabber password.'),
      '#size' => 60,
      '#maxlength' => 50,
      '#weight' => 20,
      '#default_value' => '',
    );
  }

  $form['submit_button'] = array(
    '#type' => 'submit',
    '#value' => t('Register'),
  );

  if ($account->characterID == 0) {
    $form['submit_button']['#disabled'] = TRUE;
  }

  $form['#validate'] = array('eve_api_jabber_user_form_validate');
  $form['#submit'] = array('eve_api_jabber_user_form_submit');

  return $form;
}

/**
 * Form validation handler for eve_api_jabber_user_form().
 *
 * @see eve_api_jabber_user_form_submit()
 */
function eve_api_jabber_user_form_validate($form, &$form_state) {
  $account = $form_state['user'];
  $uid = $account->uid;

  if ($account->characterID == 0) {
    drupal_goto('user/' . $uid . '/edit');
  }

  try {
    eve_api_jabber_connect();

    $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
      ':username' => (string) eve_api_jabber_format_name($account->name),
    ));

    if (!$result->rowCount() && empty($form_state['values']['password'])) {
      form_set_error('password', t('You are registering for the first time, you must enter a password.'));
    }

    eve_api_jabber_disconnect();
  }
  catch (EveApiException $e) {
    form_set_error('password', t('There was an error connecting with the Jabber Server, please try again later.'));
  }
}

/**
 * Form submission handler for eve_api_jabber_user_form().
 *
 * @see eve_api_jabber_user_form_validate()
 */
function eve_api_jabber_user_form_submit($form, &$form_state) {
  $account = $form_state['user'];

  $roles = user_roles();
  $user_roles = $all_roles = $corp_info = array();

  $result = db_query('SELECT corporationName, corporationTicker FROM {eve_api_alliance_list_corporations}');

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $corp_info[$row->corporationName] = $row->corporationTicker;
    }
  }

  foreach ($roles as $rid => $rname) {
    if ($rid == 1 || $rid == 2) {
      continue;
    }

    $description = $rname;

    foreach ($corp_info as $key => $value) {
      if ($key == $rname) {
        $rname = $value;
      }
    }

    $all_roles[$rname] = $description;

    $user_roles[] = eve_api_jabber_format_name($rname);
  }

  try {
    eve_api_jabber_connect();

    $query = array(
      'name' => eve_api_jabber_format_display_name($account->name),
      'email' => $account->mail,
      'groups' => implode(",", $user_roles),
    );

    $result = db_query('SELECT username FROM {ofUser} WHERE username = :username', array(
      ':username' => (string) eve_api_jabber_format_name($account->name),
    ));

    if ($result->rowCount()) {
      $type = 'update';
    }
    else {
      $type = 'add';
    }

    foreach ($all_roles as $name => $description) {
      eve_api_jabber_group_add($name, $description);
    }

    eve_api_jabber_disconnect();

    if (!empty($form_state['values']['password'])) {
      $password = array(
        'password' => (string) $form_state['values']['password'],
      );

      $query = array_merge($query, $password);
    }

    $result = eve_api_jabber_url_query($type, $account->name, $query);

    if ($result === TRUE) {
      if ($type = 'update') {
        drupal_set_message(t('Your Jabber account was updated.'));
      }
      else {
        drupal_set_message(t('Your Jabber account was created.'));
      }

      foreach ($all_roles as $name => $description) {
        $query = array(
          'name' => eve_api_jabber_format_name($name),
          'item_jid' => eve_api_jabber_format_name($name) . '@broadcast.' . variable_get('eve_api_jabber_domain', 'yourdomain.com'),
          'subscription' => 3,
        );

        eve_api_jabber_url_query('add_roster', $account->name, $query);

        $query_add = array(
          'groups' => eve_api_jabber_format_name($name),
        );

        $query = array_merge($query, $query_add);

        eve_api_jabber_url_query('update_roster', $account->name, $query);
      }
    }
    else {
      drupal_set_message(t('There was an error updateing your Jabber account, please try again later.'), 'error');
    }

    $queue = DrupalQueue::get('eve_api_jabber_cron_user_sync');

    $queue->createItem(array(
      'uid' => $account->uid,
      'name' => $account->name,
      'runs' => 1,
    ));
  }
  catch (EveApiException $e) {
    drupal_set_message(t('There was an error updateing your Jabber account, please try again later.'), 'error');
  }
}
