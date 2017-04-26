<?php
	namespace Drupal\eve_api\Form;

	use Drupal\Core\Form\FormBase;
	use Drupal\Core\Form\FormStateInterface;
	
	/**
	 * Form constructor for editing the settings.
	 *
	 * @ingroup forms
	 */
	class AdminSettingsForm extends FormBase {
		/**
		 * Get the id of the form
		 */
		 public function getFormId() {
			return 'eve_api_admin_settings_form';
		 }
		 
	public function buildForm(array $form, FormStateInterface $form_state) {
			$form['enter_api'] = array(
				'#type' => 'fieldset',
				'#title' => t('Enter API Key'),
				'#description' => t("Enter the Alliance Executor's or Corporation CEO's API Key."),
				'#weight' => 0,
				'#collapsed' => \Drupal::config('eve_api.config')->get('eve_api_enable', FALSE),
				'#collapsible' => TRUE,
			);
			
			$form['enter_api']['keyID'] = array(
    '#type' => 'textfield',
    '#title' => t('Key ID'),
    '#description' => t('Please enter your Key ID from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#size' => 20,
    '#maxlength' => 15,
    '#weight' => 0,
    '#default_value' => \Drupal::config('eve_api.config')->get('eve_api_corp_keyid', ''),
  );

  $form['enter_api']['vCode'] = array(
    '#type' => 'textfield',
    '#title' => t('Verification Code'),
    '#description' => t('Please enter your Verification Code from the EVE API Page located <a href="@url" target="_blank">here</a>.', array('@url' => 'http://community.eveonline.com/support/api-key/CreatePredefined?accessMask=67108863')),
    '#required' => TRUE,
    '#size' => 80,
    '#maxlength' => 64,
    '#weight' => 10,
    '#default_value' => \Drupal::config('eve_api.config')->get('eve_api_corp_vcode', ''),
  );

  $form['enter_api']['enable'] = array(
    '#type' => 'checkbox',
    '#title' => t('Enable EVE API'),
    '#default_value' => \Drupal::config('eve_api.config')->get('eve_api_enable', FALSE),
    '#weight' => 20,
  );

  if (\Drupal::config('eve_api.config')->get('eve_api_first_run', FALSE) && \Drupal::config('eve_api.config')->get('eve_api_enable', FALSE)) {
    $form['enter_api']['force'] = array(
      '#type' => 'checkbox',
      '#title' => t('Force Update API Information via a cron task.'),
      '#default_value' => FALSE,
      '#weight' => 30,
    );

    $form['settings'] = array(
      '#type' => 'fieldset',
      '#title' => t('Settings'),
      '#description' => t('General configurable settings.'),
      '#weight' => 10,
      '#collapsed' => !\Drupal::config('eve_api.config')->get('eve_api_enable', FALSE),
      '#collapsible' => TRUE,
    );

    $form['settings']['enable_cron'] = array(
      '#type' => 'radios',
      '#title' => t('Enable EVE API Cron'),
      '#description' => t('If you experience problems, or are updating the module, you can disable the cron related to EVE API.'),
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#weight' => 0,
      '#default_value' => (int) \Drupal::config('eve_api.config')->get('eve_api_enable_cron', TRUE),
    );

    $form['settings']['debug'] = array(
      '#type' => 'radios',
      '#title' => t('Enable EVE API Debug'),
      '#description' => t('If you experience problems with EVE API, you can enable the verbose logging.'),
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#weight' => 10,
      '#default_value' => (int) \Drupal::config('eve_api.config')->get('eve_api_debug', FALSE),
    );

    $form['settings']['nag_user'] = array(
      '#type' => 'radios',
      '#title' => t('Nag User to Select Character'),
      '#description' => t('Display a message on every page telling the user to select a main character if they have not selected a main character.'),
      '#options' => array(0 => t('No'), 1 => t('Yes')),
      '#weight' => 20,
      '#default_value' => (int) \Drupal::config('eve_api.config')->get('eve_api_nag_user', TRUE),
    );
  }
  else {
    $form['enter_api']['force'] = array(
      '#type' => 'hidden',
      '#value' => FALSE,
    );

    $form['enter_api']['enable_cron'] = array(
      '#type' => 'hidden',
      '#value' => TRUE,
    );

    $form['enter_api']['debug'] = array(
      '#type' => 'hidden',
      '#value' => (int) \Drupal::config('eve_api.config')->get('eve_api_debug', FALSE),
    );

    $form['enter_api']['nag_user'] = array(
      '#type' => 'hidden',
      '#value' => (int) \Drupal::config('eve_api.config')->get('eve_api_nag_user', TRUE),
    );
			
				
		 }
	}

	public function validateForm(array &$form, FormStateInterface $form_state) {
		$key_id = (int) $form_state['values']['keyID'];
		$v_code = (string) $form_state['values']['vCode'];
		
		if (empty($key_id) || empty($v_code) || preg_match('/[^a-z0-9]/i', $v_code) || preg_match('/[^0-9]/', $key_id) || strlen($key_id) > 15 || strlen($v_code) > 64 || strlen($v_code) < 20) {
			form_set_error('keyID', t('Invalid input, please try again.'));
			form_set_error('vCode');
			return;
		}	

		$query = array(
			'keyID' => $key_id,
			'vCode' => $v_code,
		);

		$characters = eve_api_get_api_key_info_api($query);		
		
		if (isset($characters['error'])) {
			form_set_error('keyID', t('There was an error with the API.'));
			form_set_error('vCode');
			\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_enable', FALSE)->save();
		}
		else {
			if ($characters['expires'] || $characters['type'] != 'Corporation' || ($characters['accessMask'] & 270680) != 270680) {
				form_set_error('keyID', t('Please ensure that all boxes are highlighted and selected for the API, the "Character" drop down box is set to your Alliance Executor or Corporation CEO, the "Type" drop down box is set to "Corporation", and that the "No Expiry" checkbox is ticked.'));
				form_set_error('vCode');
				\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_enable', FALSE)->save();
			}
			$form_state['values']['characters'] = $characters;
		}		
	}
	
	public function submitForm(array &$form, FormStateInterface $form_state) {
		$characters = (array) $form_state['values']['characters'];

		\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_corp_keyid', (int) $form_state['values']['keyID'])->save();
		\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_corp_vcode', (string) $form_state['values']['vCode'])->save();
		\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_enable_cron', (bool) $form_state['values']['enable_cron'])->save();
		\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_debug', (bool) $form_state['values']['debug'])->save();
		\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_nag_user', (bool) $form_state['values']['nag_user'])->save();
		
		  // It's not pretty but it works.
		foreach ($characters['characters'] as $character) {
			\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_allianceID', (int) $character['allianceID'])->save();
			\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_corporationID', (int) $character['corporationID'])->save();
		}
		
		if (((bool) $form_state['values']['enable'] && (bool) $form_state['values']['force']) || ((bool) $form_state['values']['enable'] && !\Drupal::config('eve_api.config')->get('eve_api_enable', FALSE))) {
			\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_first_run', TRUE)->save();

			$queue = Drupal::queue('eve_api_cron_api_alliance_fetch');
			$queue->createItem(1);

			$queue = Drupal::queue('eve_api_cron_api_alliance_sync');
			$queue->createItem(1);

			$queue = Drupal::queue('eve_api_cron_api_mask_sync');
			$queue->createItem(1);

			$queue = Drupal::queue('eve_api_cron_api_skill_tree');
			$queue->createItem(1);

			$queue = Drupal::queue('eve_api_cron_api_error_list');
			$queue->createItem(1);

			drupal_set_message(t('The Alliance API Info is set to be retrieved on the next cron job, it can take up to a minute for the cron task to be triggered.'));
		}
		elseif ((bool) $form_state['values']['enable']) {
			drupal_set_message(t('Settings Updated.'));
		}
		else {
			\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_first_run', FALSE)->save();
			\Drupal::configFactory()->getEditable('eve_api.config')->set('eve_api_enable', FALSE)->save();
			drupal_set_message(t('Registrations have been disabled, EVE API cron tasks have been disabled, and all user menus have been disabled.'));
		}
		//TODO: Look into this
	  //menu_rebuild();
		
	}
?>