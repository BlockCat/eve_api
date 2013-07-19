<?php

/**
 * Function to verify API is valid
 */
function omni_eve_api_verify_api_status(&$xml, $keyid, $vcode)
{
	return TRUE;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_verify_blue_status(&$xml)
{
	return TRUE;
}

/**
 * Function to compare the API against the Alliance Blue List
 */
function omni_eve_api_valid_characters(&$xml)
{
	return array(
      'plugin1' => t('Plugin 1'),
      'plugin2' => t('Plugin 2'),
      'plugin3' => t('Plugin 3'),
    );
}
