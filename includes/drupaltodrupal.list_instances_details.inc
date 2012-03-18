<?php

/**
 * @file
 * Functions directly related to the list instances with details tab.
 */
function drupaltodrupal_list_instances_details() {
  $build = array();
  $build['details'] = array(
    '#title' => 'Details about the instances in the database',
    '#type' => 'fieldset',
  );
  $result = db_query('SELECT i.id as id, i.url as url, i.description as description, i.time_inserted as time_inserted, i.last_alive as last_alive, p.public_key as public_key FROM {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id');
  foreach ($result as $record) {
    $rows = array();
    $rows[] = array('id', l($record->id, 'admin/drupaltodrupal/instances/edit/' . $record->id));
    $rows[] = array('URL', $record->url);
    $rows[] = array('inserted', drupaltodrupal_get_absolute_time($record->time_inserted));
    $rows[] = array('description', $record->description);
    $rows[] = array('public key', is_null($record->public_key) ? 'N/A' : $record->public_key);
    $build['details']['details' . $record->id]['table_instance' . $record->id] = array(
      '#theme' => 'table',
      '#header' => array('Type', 'Value'),
      '#rows' => $rows,
      '#empty' => t('No information available.'),
    );
  }
  return $build;
}
