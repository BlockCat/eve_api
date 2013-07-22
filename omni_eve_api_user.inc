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
 * Add function to show all API's the user currently has and delete them via selection
 */

/**
 * Add function to add a new API
 */

/**
 * Add function to select a new main character
 */
