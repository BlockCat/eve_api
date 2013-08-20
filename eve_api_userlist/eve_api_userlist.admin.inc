<?php
/**
 * @file
 * Functions related to allow admin's to alter the settings and options.
 */

/**
 * Form constructor for the character list form.
 *
 * @ingroup forms
 */
function eve_api_userlist_admin_characterlist_form($form, &$form_state) {
  $header = array(
    'characterName' => array(
      'data' => t('Name'),
      'field' => 'c.characterName',
    ),
    'corporationName' => array(
      'data' => t('Corporation'),
      'field' => 'c.corporationName',
    ),
    'corporationTicker' => array(
      'data' => t('Corporation'),
      'field' => 'c.corporationTicker',
    ),
    'allianceName' => array(
      'data' => t('Alliance'),
      'field' => 'c.allianceName',
    ),
    'allianceTicker' => array(
      'data' => t('Alliance'),
      'field' => 'c.allianceTicker',
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
        'characterName' => l($row->characterName, 'user/' . $row->uid . '/eve_api'),
        'corporationName' => l($row->corporationName, 'eve_api/userlist/corp/' . $row->corporationID),
        'corporationTicker' => check_plain($row->corporationTicker),
        'allianceName' => empty($row->allianceName) ? l(t('None'), 'eve_api/userlist/alliance/0') : l($row->allianceName, 'eve_api/userlist/alliance/' . $row->allianceID),
        'allianceTicker' => check_plain($row->allianceTicker),
        'joined' => check_plain(date('Y-m-d H:i:s', $row->created)),
        'access' => ($row->access == 0) ? '' : check_plain(date('Y-m-d H:i:s', $row->access)),
      );
    }
  }

  $form['characterlist'] = array(
    '#type' => 'fieldset',
    '#title' => t('User List'),
    '#description' => t('List of Active Users and some basic information.'),
    '#weight' => 0,
  );

  $form['characterlist']['characterlist'] = array(
    '#type' => 'markup',
    '#markup' => theme('table', array(
      'header' => $header,
      'rows' => $options
    )),
    '#weight' => 0,
  );

  $form['characterlist']['page'] = array(
    '#theme' => 'pager',
    '#weight' => 10,
  );

  return $form;
}
