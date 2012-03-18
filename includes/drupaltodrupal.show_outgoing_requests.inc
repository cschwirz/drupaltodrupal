<?php

/**
 * @file
 * Functions directly related to the outgoing requests tab.
 */
function drupaltodrupal_show_outgoing_requests() {
  $result = db_query('SELECT o.outreq_id as outreq_id, o.email as email, o.message as message, o.time_inserted as time_inserted, o.next_send as next_send, i.url as receiver_url, o.state as state, o.reply as reply, o.type as otype FROM {drupaltodrupal_outgoing_requests} o, {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i WHERE o.id_offer = f.fcert_id AND f.id_friend = i.id');
  $page = array();
  $rows = array();
  foreach ($result as $record) {
    $rows[] = array(
      $record->outreq_id,
      l($record->receiver_url, $record->receiver_url),
      $record->email,
      $record->message,
      date('F j, Y, g:i a', $record->time_inserted),
      drupaltodrupal_get_request_type($record->otype),
      $record->state,
      $record->reply,
      is_null($record->next_send) ? '-' : drupaltodrupal_get_relative_time($record->next_send),
    );
  }
  $build = array();
  $build['outgoing_requests'] = array(
    '#title' => t('Outgoing requests in the database'),
    '#type' => 'fieldset',
  );
  $build['outgoing_requests'][] = array(
    '#theme' => 'table',
    '#header' => array(
      t('ID'),
      t('Reciever'),
      t('E-Mail'),
      t('Message'),
      t('Time sent'),
      t('Type'),
      t('State'),
      t('Reply'),
      t('Next Send'),
    ),
    '#rows' => $rows,
    '#empty' => t('No outgoing requests in database.'),
  );
  return $build;
}
