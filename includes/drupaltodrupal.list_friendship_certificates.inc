<?php

/**
 * @file
 * Functions directly related to the list friendship certificates tab.
 */
function drupaltodrupal_list_friendship_certificates() {
  $result = db_query('SELECT f.fcert_id as f_id, i1.id as i1_id, i2.id as i2_id, i1.url as i1_url, i2.url as i2_url, f.time_inserted as time_inserted, f.certificate as certificate, f.valid as valid, f.valid_until as valid_until FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i1, {drupaltodrupal_instances} i2 WHERE f.id_signer = i1.id AND f.id_friend = i2.id');
  $page = array();
  $rows = array();
  foreach ($result as $record) {
    $rows[] = array(
      $record->f_id,
      $record->i1_id,
      l($record->i1_url, $record->i1_url),
      $record->i2_id,
      l($record->i2_url, $record->i2_url),
      drupaltodrupal_get_relative_time($record->time_inserted),
      is_null($record->certificate) ? 'no' : 'yes',
      drupaltodrupal_bool_to_str($record->valid),
      //date('F j, Y, g:i a', $record->valid_until),
      drupaltodrupal_get_relative_time($record->valid_until),
      /*
      $record->url,
      date('F j, Y, g:i a', $record->time_inserted),
      is_null($record->last_alive) ? 'never' : date('F j, Y, g:i a', $record->last_alive),
      $record->description,
      is_null($record->public_key) ? 'N/A' : 'available',
      */
    );
  }
  $build = array();
  $build['friendship_certificates'] = array(
    '#title' => t('Friendship certificates in the database'),
    '#type' => 'fieldset',
  );
  $build['friendship_certificates'][] = array(
    '#theme' => 'table',
    '#header' => array(
      t('ID'),
      t('ID req'),
      t('requester'),
      t('ID fr'),
      t('friend'),
      t('time inserted'),
      t('exists'),
      t('valid'),
      t('valid until'),
    ),
    '#rows' => $rows,
    '#empty' => t('No friendship certificates in database.'),
  );
  return $build;
}
