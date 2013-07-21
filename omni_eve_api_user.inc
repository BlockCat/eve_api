<?php

/**
 * Implements hook_user_delete().
 */
function omni_eve_api_user_delete($account) {
  db_delete('oea_characters')->condition('drupalid', $account->uid)->execute();
  db_delete('oea_api_keys')->condition('drupalid', $account->uid)->execute();
}
