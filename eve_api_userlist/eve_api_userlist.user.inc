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
function eve_api_userlist_form($form, &$form_state) {
  $header = array(
    'name' => array(
      'data' => t('Name'),
      'field' => 'u.name',
    ),
    'corporationName' => array(
      'data' => t('Corporation'),
      'field' => 'c.corporationName',
    ),
    'allianceName' => array(
      'data' => t('Alliance'),
      'field' => 'c.allianceName',
    ),
    'created' => array(
      'data' => t('Created'),
      'field' => 'u.created',
    ),
    'access' => array(
      'data' => t('Last Login'),
      'field' => 'u.access',
    ),
  );

  $options = array();

  $query = db_select('users', 'u');
  $query->join('eve_api_characters', 'c', 'u.characterID = c.characterID');
  $query->fields('u', array(
        'name',
        'created',
        'access',
      ))
    ->fields('c', array(
        'corporationID',
        'corporationName',
        'corporationTicker',
        'allianceID',
        'allianceName',
        'allianceTicker',
      ))
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
        'name' => check_plain($row->name),
        'corporationName' => check_plain($row->corporationName),
        'allianceName' => check_plain($row->allianceName),
        'created' => check_plain(date('Y-m-d H:i:s', $row->created)),
        'access' => ($row->access == 0) ? '' : check_plain(date('Y-m-d H:i:s', $row->access)),
      );
    }
  }

  $form['userlist'] = array(
    '#type' => 'fieldset',
    '#title' => t('User List'),
    '#description' => t('List of Active Users and some basic information.'),
    '#weight' => 0,
  );

  $form['userlist']['userlist'] = array(
    '#type' => 'markup',
    '#markup' => theme('table', array(
      'header' => $header,
      'rows' => $options
    )),
    '#weight' => 0,
  );

  $form['userlist']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 10,
  );

  return $form;
}
 