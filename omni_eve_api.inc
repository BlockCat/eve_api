<?php

/**
 * Function to verify API is valid
 */
function omni_eve_api_verify_api_status(&$xml, $keyid, $vcode)
{
	$xml = $keyid . $vcode;
	return NULL;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_verify_blue_status(&$xml)
{
	return NULL;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_valid_characters(&$xml)
{
	return array(
      'Character 1' => t('Plugin 1'),
      'Character 2' => t('Plugin 2'),
      $xml => t($xml),
    );
}
