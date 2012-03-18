<?php

/**
 * @file
 * Functions directly related to the settings tab.
 */
function drupaltodrupal_show_settings() {
  $build['general_settings'] = array(
    '#title' => t('General Settings'),
    '#type' => 'fieldset',
  );
  $build['general_settings'][] = drupal_get_form('drupaltodrupal_form_settings');
  $build['advanced'] = array(
    '#title' => t('Advanced'),
    '#type' => 'fieldset',
  );
  $build['advanced'][] = drupal_get_form('drupaltodrupal_form_send_requests');
  return $build;
}
