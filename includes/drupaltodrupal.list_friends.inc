<?php

/**
 * @file
 * Functions directly related to the list friends tab.
 */
function drupaltodrupal_list_friends() {
  $t = drupaltodrupal_get_time();
  $result = db_query('SELECT f1.id_signer as id1, f1.id_friend as id2 , f1.verified_until as f1_valid_until, f2.verified_until as f2_valid_until, f1.url as url1, f2.url as url2 FROM'
    . ' (SELECT f.id_signer, f.id_friend, MAX(f.valid_until) as verified_until, i.url as url FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i WHERE f.valid = 1 AND f.id_signer = i.id GROUP BY f.id_signer, f.id_friend) f1,'
    . ' (SELECT f.id_signer, f.id_friend, MAX(f.valid_until) as verified_until, i.url as url FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i WHERE f.valid = 1 AND f.id_signer = i.id GROUP BY f.id_signer, f.id_friend) f2'
    . ' WHERE f1.id_signer < f1.id_friend'
    . ' AND f1.id_signer = f2.id_friend'
    . ' AND f1.id_friend = f2.id_signer'
    . ' AND f1.verified_until >= :t'
    . ' AND f2.verified_until >= :t',
    array(':t' => $t)
  );
  $page = array();
  $rows = array();
  foreach ($result as $record) {
    $rows[] = array(
      $record->id1,
      $record->url1,
      $record->id2,
      $record->url2,
      drupaltodrupal_get_relative_time($record->f1_valid_until),
      drupaltodrupal_get_relative_time($record->f2_valid_until),
      drupaltodrupal_get_relative_time(min($record->f1_valid_until, $record->f2_valid_until)),
    );
  }
  $build = array();
  $build['friends'] = array(
    '#title' => t('List of valid friendships in the database'),
    '#type' => 'fieldset',
  );
  $build['friends'][] = array(
    '#theme' => 'table',
    '#header' => array(
      t('ID1'),
      t('URL1'),
      t('ID2'),
      t('URL2'),
      t('valid until (1)'),
      t('valid until (2)'),
      t('valid until'),
    ),
    '#rows' => $rows,
    '#empty' => t('No friendships found in database.'),
  );
  return $build;
}
