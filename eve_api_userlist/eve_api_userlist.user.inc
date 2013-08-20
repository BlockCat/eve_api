<?php
/**
 * @file
 * Functions related to allowing users to alter the user settings and options.
 */

/**
 * Form constructor for the user list form.
 *
 * @ingroup forms
 */
function eve_api_userlist_user_userlist_form($form, &$form_state, $id = NULL) {
  timer_start('userlist');
  $account = $GLOBALS['user'];
  $character_data = eve_api_get_character_info($account->name);
  $is_director = eve_api_character_is_director($account->characterID, $character_data['corporationID']);
  $is_good = user_access('moderate eve_api users');

  $header = array(
    'characterName' => array(
      'data' => t('Name'),
      'field' => 'c.characterName',
    ),
    'corporationName' => array(
      'data' => t('Corporation'),
      'field' => 'c.corporationName',
    ),
    'allianceName' => array(
      'data' => t('Alliance'),
      'field' => 'c.allianceName',
    ),
    'joined' => array(
      'data' => t('Joined'),
    ),
    'access' => array(
      'data' => t('Last Login'),
    ),
  );

  $options = array();

  $query = db_select('users', 'u');
  $query_join = $query->join('eve_api_characters', 'c', 'u.uid = c.uid');
  $query->addField($query_join, 'characterID', 'cID');
  $query->fields('u', array(
      'created',
      'access',
      'uid',
      'characterID',
    ))
    ->fields('c', array(
      'characterName',
      'corporationID',
      'corporationName',
      'corporationTicker',
      'allianceID',
      'allianceName',
      'allianceTicker',
    ));

  if (isset($id)) {
    $or = db_or();
    $or->condition('c.allianceID', $id, '=');
    $or->condition('c.corporationID', $id, '=');
    $query->condition($or);
  }

  $query->condition('c.deleted', 0, '=')
    ->condition('u.status', 1, '=')
    ->condition('u.characterID', 0, '!=')
    ->extend('PagerDefault')
    ->limit(20)
    ->extend('TableSort')
    ->orderByHeader($header);
  $result = $query->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[] = array(
        'characterName' => ($row->characterID == $row->cID) ? l($row->characterName, 'user/' . $row->uid) : check_plain($row->characterName),
        'corporationName' => ((($row->corporationID == $character_data['corporationID'] && $is_director) || $is_good) ? l($row->corporationName, 'eve_api/userlist/' . $row->corporationID . '/director') : l($row->corporationName, 'eve_api/userlist/' . $row->corporationID)) . ' (' . check_plain($row->corporationTicker) . ')',
        'allianceName' => (empty($row->allianceName) ? l(t('None'), 'eve_api/userlist/0') : l($row->allianceName, 'eve_api/userlist/' . $row->allianceID) . ' (' . check_plain($row->allianceTicker) . ')'),
        'joined' => ($row->characterID == $row->cID) ? check_plain(date('Y-m-d H:i:s', $row->created)) : '',
        'access' => ($row->characterID == $row->cID) ? ($row->access == 0) ? '' : check_plain(date('Y-m-d H:i:s', $row->access)) : '',
      );
    }
  }

  $form['corporation_userlist'] = array(
    '#type' => 'fieldset',
    '#title' => t('User List'),
    '#description' => t('List of Active Users and some basic information.'),
    '#weight' => 0,
  );

  $form['corporation_userlist']['corporation_userlist'] = array(
    '#type' => 'markup',
    '#markup' => theme('table', array(
      'header' => $header,
      'rows' => $options,
    )),
    '#weight' => 0,
  );

  $form['corporation_userlist']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 10,
  );

  print_r(timer_stop('userlist'));

  return $form;
}

/**
 * Form constructor for the corporation user list form.
 *
 * @ingroup forms
 */
function eve_api_userlist_user_userlist_director_form($form, &$form_state, $corporation_id) {
  timer_start('userlist_corp');
  $header = array(
    'characterName' => array(
      'data' => t('Name'),
      'field' => 'c.characterName',
    ),
    'main' => array(
      'data' => t('Main Character'),
      'field' => 'u.name',
    ),
    'api' => array(
      'data' => t('API Data'),
    ),
    'joined' => array(
      'data' => t('Joined'),
      'field' => 'u.created',
    ),
    'access' => array(
      'data' => t('Last Login'),
      'field' => 'u.access',
    ),
  );

  $options = array();

  $query = db_select('users', 'u');
  $query_join = $query->join('eve_api_characters', 'c', 'u.uid = c.uid');
  $query->join('eve_api_api_keys', 'ak', 'c.apiID = ak.apiID');
  $query->addField($query_join, 'characterID', 'cID');
  $query->fields('u', array(
      'name',
      'created',
      'access',
      'uid',
      'characterID',
    ))
    ->fields('c', array(
      'characterName',
      'corporationID',
      'corporationName',
      'corporationTicker',
      'allianceID',
      'allianceName',
      'allianceTicker',
    ))
    ->fields('ak', array(
      'keyID',
      'vCode',
    ))
    ->condition('c.corporationID', $corporation_id, '=')
    ->condition('c.deleted', 0, '=')
    ->condition('u.status', 1, '=')
    ->condition('u.characterID', 0, '!=')
    ->extend('PagerDefault')
    ->limit(20)
    ->extend('TableSort')
    ->orderByHeader($header);
  $result = $query->execute();

  if ($result->rowCount()) {
    foreach ($result->fetchAll() as $row) {
      $options[] = array(
        'characterName' => l($row->characterName, 'user/' . $row->uid),
        'main' => l($row->name, 'user/' . $row->uid),
        'api' => t('Key ID: @keyID<br />Verification Code: @vCode', array('@keyID' => $row->keyID, '@vCode' => $row->vCode)),
        'joined' => ($row->characterID == $row->cID) ? check_plain(date('Y-m-d H:i:s', $row->created)) : '',
        'access' => ($row->characterID == $row->cID) ? ($row->access == 0) ? '' : check_plain(date('Y-m-d H:i:s', $row->access)) : '',
      );
    }
  }

  $form['corporation_userlist'] = array(
    '#type' => 'fieldset',
    '#title' => t('Corporation User List'),
    '#description' => t('List of Active Corporation Users and some basic information.'),
    '#weight' => 0,
  );

  $form['corporation_userlist']['corporation_userlist'] = array(
    '#type' => 'markup',
    '#markup' => theme('table', array(
      'header' => $header,
      'rows' => $options,
    )),
    '#weight' => 0,
  );

  $form['corporation_userlist']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 10,
  );

  print_r(timer_stop('userlist_corp'));

  return $form;
}
