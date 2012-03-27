<?php
/**
 * @file
 * Functions for creating and handling forms.
 */

function drupaltodrupal_form_settings() {
  $form = array();
  // receiver of the request
  $form['utc_offset'] = array(
    '#type' => 'textfield',
    '#title' => t('UTC offset'),
    '#description' => t('Number of seconds that are added to the system time (in case the system time is incorrect and can not be adjusted directly).'),
    '#default_value' => variable_get('drupaltodrupal_utc_offset', DRUPALTODRUPAL_UTC_OFFSET_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['validity'] = array(
    '#type' => 'textfield',
    '#title' => t('Period of validity'),
    '#description' => t('Time in minutes a friendship certificate stays valid.'),
    '#default_value' => variable_get('drupaltodrupal_validity', DRUPALTODRUPAL_VALIDITY_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['renew_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Time before friendship gets renewed'),
    '#description' => t('Time in minutes before an own outdating friendship certificate gets renewed. Note that this value should be larger than the period in which cron.php gets called.'),
    '#default_value' => variable_get('drupaltodrupal_renew_time', DRUPALTODRUPAL_RENEW_TIME_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['resend_time'] = array(
    '#type' => 'textfield',
    '#title' => t('Time until resend'),
    '#description' => t('Time in minutes to wait until an erroneous request is resent.'),
    '#default_value' => variable_get('drupaltodrupal_resend_time', DRUPALTODRUPAL_RESEND_TIME_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['timeout'] = array(
    '#type' => 'textfield',
    '#title' => t('Timeout'),
    '#description' => t('Maximum time in seconds a remote procedure call may take.'),
    '#default_value' => variable_get('drupaltodrupal_timeout', DRUPALTODRUPAL_TIMEOUT_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['garbage'] = array(
    '#type' => 'textfield',
    '#title' => t('Garbage collection time'),
    '#description' => t('Time in minutes until outdated certificates get removed from the database.'),
    '#default_value' => variable_get('drupaltodrupal_garbage_time', DRUPALTODRUPAL_GARBAGE_TIME_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['delta0'] = array(
    '#type' => 'textfield',
    '#title' => t('Incoming requests lower bound'),
    '#description' => t('Number of seconds the timestamp of an incoming request / function call may be older than the system time of receival.'),
    '#default_value' => variable_get('drupaltodrupal_delta_0', DRUPALTODRUPAL_DELTA_0_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['delta1'] = array(
    '#type' => 'textfield',
    '#title' => t('Incoming requests upper bound'),
    '#description' => t('Number of seconds the timestamp of an incoming request / function call may be newer than the system time of receival.'),
    '#default_value' => variable_get('drupaltodrupal_delta_1', DRUPALTODRUPAL_DELTA_1_DEFAULT),
    '#size' => 10,
    '#maxlength' => 10,
    '#required' => TRUE,
  );
  $form['auto_accept'] = array(
    '#type' => 'checkbox',
    '#default_value' => variable_get('drupaltodrupal_auto_accept', DRUPALTODRUPAL_AUTO_ACCEPT),
    '#title' => t('Automatically accept incoming friendship requests'),
    '#description' => t('On friendship request, automatically pull public key, add it to the database and accept friendship request.'),
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Save'),
  );
  return $form;
}
function drupaltodrupal_form_settings_validate($form, &$form_state) {
  $utc_offset = $form_state['values']['utc_offset'];
  if (!drupaltodrupal_check_convert_int($utc_offset)) {
    form_set_error('utc_offset', t('Offset must be an integer.'));
  }
  elseif ($utc_offset < DRUPALTODRUPAL_UTC_OFFSET_MIN || $utc_offset > DRUPALTODRUPAL_UTC_OFFSET_MAX) {
    form_set_error('utc_offset', t('Offset must be between @min and @max.', array('@min' => DRUPALTODRUPAL_UTC_OFFSET_MIN, '@max' => DRUPALTODRUPAL_UTC_OFFSET_MAX)));
  }
  $validity = $form_state['values']['validity'];
  if (!drupaltodrupal_check_convert_int($validity)) {
    form_set_error('validity', t('Period of validity must be an integer.'));
  }
  elseif ($validity < DRUPALTODRUPAL_VALIDITY_MIN || $validity > DRUPALTODRUPAL_VALIDITY_MAX) {
    form_set_error('validity', t('Period of validity must be between @min and @max.', array('@min' => DRUPALTODRUPAL_VALIDITY_MIN, '@max' => DRUPALTODRUPAL_VALIDITY_MAX)));
  }
  $renew_time = $form_state['values']['renew_time'];
  if (!drupaltodrupal_check_convert_int($renew_time)) {
    form_set_error('renew_time', t('Time before an outdating friendship certificate gets renewed must be an integer.'));
  }
  elseif ($renew_time < DRUPALTODRUPAL_RENEW_TIME_MIN || $renew_time > DRUPALTODRUPAL_RENEW_TIME_MAX) {
    form_set_error('renew_time', t('Time before an outdating friendship certificate gets renewed must be between @min and @max.', array('@min' => DRUPALTODRUPAL_RENEW_TIME_MIN, '@max' => DRUPALTODRUPAL_RENEW_TIME_MAX)));
  }
  $resend_time = $form_state['values']['resend_time'];
  if (!drupaltodrupal_check_convert_int($resend_time)) {
    form_set_error('resend_time', t('Resend time must be an integer.'));
  }
  elseif ($resend_time < DRUPALTODRUPAL_RESEND_TIME_MIN || $resend_time > DRUPALTODRUPAL_RESEND_TIME_MAX) {
    form_set_error('resend_time', t('Resended time must be between @min and @max.', array('@min' => DRUPALTODRUPAL_RESEND_TIME_MIN, '@max' => DRUPALTODRUPAL_RESEND_TIME_MAX)));
  }
  $timeout = $form_state['values']['timeout'];
  if (!drupaltodrupal_check_convert_int($timeout)) {
    form_set_error('timeout', t('Time until timeout must be an integer.'));
  }
  elseif ($timeout < DRUPALTODRUPAL_TIMEOUT_MIN || $timeout > DRUPALTODRUPAL_TIMEOUT_MAX) {
    form_set_error('timeout', t('Time until timeout must be between @min and @max.', array('@min' => DRUPALTODRUPAL_TIMEOUT_MIN, '@max' => DRUPALTODRUPAL_TIMEOUT_MAX)));
  }
  $garbage = $form_state['values']['garbage'];
  if (!drupaltodrupal_check_convert_int($garbage)) {
    form_set_error('garbage', t('Garbage collection time must be an integer.'));
  }
  elseif ($garbage < DRUPALTODRUPAL_GARBAGE_TIME_MIN || $garbage > DRUPALTODRUPAL_GARBAGE_TIME_MAX) {
    form_set_error('garbage', t('Garbage collection time must be between @min and @max.', array('@min' => DRUPALTODRUPAL_GARBAGE_TIME_MIN, '@max' => DRUPALTODRUPAL_GARBAGE_TIME_MAX)));
  }
  $delta0 = $form_state['values']['delta0'];
  if (!drupaltodrupal_check_convert_int($delta0)) {
    form_set_error('delta0', t('Incoming requests lower bound must be an integer.'));
  }
  elseif ($delta0 < DRUPALTODRUPAL_DELTA_0_MIN|| $delta0 > DRUPALTODRUPAL_DELTA_0_MAX) {
    form_set_error('delta0', t('Incoming requests lower bound must be between @min and @max.', array('@min' => DRUPALTODRUPAL_DELTA_0_MIN, '@max' => DRUPALTODRUPAL_DELTA_0_MAX)));
  }
  $delta1 = $form_state['values']['delta1'];
  if (!drupaltodrupal_check_convert_int($delta1)) {
    form_set_error('delta1', t('Incoming requests lower bound must be an integer.'));
  }
  elseif ($delta1 < DRUPALTODRUPAL_DELTA_1_MIN|| $delta1 > DRUPALTODRUPAL_DELTA_1_MAX) {
    form_set_error('delta1', t('Incoming requests upper bound must be between @min and @max.', array('@min' => DRUPALTODRUPAL_DELTA_1_MIN, '@max' => DRUPALTODRUPAL_DELTA_1_MAX)));
  }
}
function drupaltodrupal_form_settings_submit($form, &$form_state) {
  $utc_offset = $form_state['values']['utc_offset'];
  $validity = $form_state['values']['validity'];
  $renew_time = $form_state['values']['renew_time'];
  $resend_time = $form_state['values']['resend_time'];
  $timeout = $form_state['values']['timeout'];
  $garbage = $form_state['values']['garbage'];
  $delta0 = $form_state['values']['delta0'];
  $delta1 = $form_state['values']['delta1'];
  drupaltodrupal_check_convert_int($utc_offset);
  drupaltodrupal_check_convert_int($validity);
  drupaltodrupal_check_convert_int($renew_time);
  drupaltodrupal_check_convert_int($resend_time);
  drupaltodrupal_check_convert_int($timeout);
  drupaltodrupal_check_convert_int($delta0);
  drupaltodrupal_check_convert_int($delta1);
  variable_set('drupaltodrupal_utc_offset', $utc_offset);
  variable_set('drupaltodrupal_validity', $validity);
  variable_set('drupaltodrupal_renew_time', $renew_time);
  variable_set('drupaltodrupal_resend_time', $resend_time);
  variable_set('drupaltodrupal_timeout', $timeout);
  variable_set('drupaltodrupal_garbage_time', $garbage);
  variable_set('drupaltodrupal_delta_0', $delta0);
  variable_set('drupaltodrupal_delta_1', $delta1);
  variable_set('drupaltodrupal_auto_accept', $form_state['values']['auto_accept'] ? TRUE : FALSE);
  drupal_set_message(t('Settings have been saved.'));
}
