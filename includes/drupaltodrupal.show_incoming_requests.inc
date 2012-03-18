<?php

/**
 * @file
 * Functions directly related to the incoming requests tab.
 */
function drupaltodrupal_form_accept_incoming_request($form, &$form_state, $id) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['submit_accept'] = array(
    '#type' => 'submit', 
    '#value' => t('Accept'),
  );
  return $form;
}
function drupaltodrupal_form_ignore_incoming_request($form, &$form_state, $id) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['submit_accept'] = array(
    '#type' => 'submit', 
    '#value' => t('Ignore'),
  );
  return $form;
}
function drupaltodrupal_form_ignore_all_incoming_requests($form, &$form_state, $id) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['submit_accept'] = array(
    '#type' => 'submit', 
    '#value' => t('Ignore all requests from this instance'),
  );
  return $form;
}
function drupaltodrupal_form_ignore_really_all_incoming_requests($form, &$form_state) {
  $form = array();
  $form['submit_accept'] = array(
    '#type' => 'submit', 
    '#value' => t('Ignore all incoming requests'),
  );
  return $form;
}
function drupaltodrupal_form_accept_incoming_request_submit($form, &$form_state) {
  // TODO: "improve" query
  $id = $form_state['values']['id'];
  $result = db_query(
      'SELECT'
    . ' r.inreq_id as inreq_id,'
    . ' r.id_offer as id_offer,'
    . ' r.email as email,'
    . ' r.message as message,'
    . ' r.time_sent as time_sent,'
    . ' r.time_received as time_received,'
    . ' r.ip as ip,'
    . ' r.request as request,'
    . ' r.signature as signature,'
    . ' r.valid as signature_valid,'
    . ' r.state as state,'
    . ' r.type as type,'
    . ' f1.valid as certificate_ab_valid,'
    . ' f1.valid_until as valid_ab_until,'
    . ' i.url as url,'
    . ' i.description as description,'
    . ' p.public_key as public_key,'
    . ' i.id as requester_id'
    . ' FROM'
    . ' {drupaltodrupal_incoming_requests} r,'
    . ' {drupaltodrupal_friendship_certificates} f1,'
    . ' {drupaltodrupal_instances} i'
    . ' LEFT OUTER JOIN'
    . ' {drupaltodrupal_public_keys} p'
    . ' ON'
    . ' i.public_key_id = p.id'
    . ' WHERE'
    . ' r.id_offer = f1.fcert_id'
    . ' AND f1.id_signer = i.id'
    . ' AND r.state = 0'
    . ' AND r.inreq_id = :i',
    array(':i' => $id)
  );
  if ($result->rowCount() == 0) {
    drupal_set_message(t('Request invalid'), 'error');
    return;
  }
  $record = $result->fetchObject();
  $acceptable = $record->signature_valid && $record->certificate_ab_valid && $record->valid_ab_until > drupaltodrupal_get_time();
  if (!$acceptable) {
    drupal_set_message(t('Request invalid'), 'error');
    return;
  }
  drupaltodrupal_get_self_instance($id_b, $entity_b, $public_key_b);
  $entity_a = $record->url;
  $id_a = $record->requester_id;
  $public_key_a = $record->public_key;
  // {{{ Insert certificate for friendship into database.
  // Time until when the new friendship should be valid.
  // TODO: make this time variable
  // TODO: different duration depending on time in request?
  $until_ba = drupaltodrupal_get_time() + variable_get('drupaltodrupal_validity', '40') * 60;
  if (is_null($public_key_a)) {
    $friendship_ba = null;
  }
  else {
    $friendship_ba = drupaltodrupal_get_friendship_string($entity_b, $entity_a, $until_ba);
  }
  $private_key_b = variable_get('drupaltodrupal_private_key');
  if (is_null($friendship_ba)) {
    $certificate_ba = null;
  }
  else {
    if (!openssl_sign($friendship_ba, $certificate_ba, $private_key_b)) {
      $certificate_ba = null;
    }
    else {
    }
  }
  $id_ba = db_insert('drupaltodrupal_friendship_certificates')->fields(array(
    'id_signer' => $id_b,
    'id_friend' => $id_a,
    'time_inserted' => drupaltodrupal_get_time(),
    'valid' => is_null($certificate_ba) ? null : TRUE,
    'valid_until' => $until_ba,
    'certificate' => $certificate_ba,
  ))->execute();
  // }}}
  $insert_time = drupaltodrupal_get_time();
  $outreq_id = db_insert('drupaltodrupal_outgoing_requests')->fields(array(
    'id_offer' => $id_ba,
    'id_common_friend' => null,
    'email' => variable_get('site_mail', 'N/A'),
    'message' => 'request accepted',
    'time_inserted' => $insert_time,
    'state' => 0,
    'reply' => null,
    'next_send' => $insert_time,
    'type' => 1, // reply
  ))->execute();
  drupal_set_message(t('Inserted accepted request into the database.'));
  $num_updated = db_update('drupaltodrupal_incoming_requests')
    ->fields(array(
      'state' => 1,
    ))
    ->condition('inreq_id', $id)
    ->execute();
  if ($num_updated) {
    drupal_set_message(t('Removed incoming request.'));
  }
  else {
    drupal_set_message(t('Error removing incoming request'), 'error');
  }
}
function drupaltodrupal_form_ignore_incoming_request_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $num_updated = db_update('drupaltodrupal_incoming_requests')
    ->fields(array(
      'state' => 2,
    ))
    ->condition('inreq_id', $id)
    ->execute();
  if ($num_updated) {
    drupal_set_message(t('Successfully ignored request.')); // TODO
  }
  else {
    drupal_set_message(t('Failed ignoring request.'), 'error');
  }
}
function drupaltodrupal_form_ignore_all_incoming_requests_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $result = db_query(
      'SELECT'
    . ' r.inreq_id as inreq_id'
    . ' FROM'
    . ' {drupaltodrupal_incoming_requests} r,'
    . ' {drupaltodrupal_friendship_certificates} f'
    . ' WHERE'
    . ' r.id_offer = f.fcert_id AND'
    . ' f.id_signer=:id_signer',
    array(':id_signer' => $id)
  );
  $inreq_ids = array();
  foreach ($result as $record) {
    $inreq_ids[] = $record->inreq_id;
  }
  if (empty($inreq_ids)) {
    $num_updated = 0;
  }
  else {
    $num_updated = db_update('drupaltodrupal_incoming_requests')
      ->fields(array(
        'state' => 2,
      ))
      ->condition('state', 0)
      ->condition('inreq_id', $inreq_ids)
      ->execute();
  }
  if ($num_updated) {
    drupal_set_message(t('Successfully ignoring @count request(s) from instance #@id.', array('@count' => $num_updated, '@id' => $id)));
  }
  else {
    drupal_set_message(t('No request ignored.'), 'error');
  }
}
function drupaltodrupal_form_ignore_really_all_incoming_requests_submit($form, &$form_state) {
  $num_updated = db_update('drupaltodrupal_incoming_requests')
    ->fields(array(
      'state' => 2,
    ))
    ->condition('state', 0)
    ->execute();
  if ($num_updated) {
    drupal_set_message(t('Successfully ignoring @count request(s).', array('@count' => $num_updated)));
  }
  else {
    drupal_set_message(t('No request ignored.'), 'error');
  }
}
function drupaltodrupal_show_incoming_requests() {
  $friends = drupaltodrupal_get_friends();
  $friend_ids = array();
  foreach ($friends as $friend) {
    $friend_ids[$friend['id']] = $friend['url'];
  }
  $build = array();
  $build['main'] = array(
    '#title' => t('Incoming requests'),
    '#type' => 'fieldset',
  );
  // XXX
  $result = db_query(
      'SELECT'
    . ' r.inreq_id as inreq_id,'
    . ' r.id_offer as id_offer,'
    . ' r.email as email,'
    . ' r.message as message,'
    . ' r.time_sent as time_sent,'
    . ' r.time_received as time_received,'
    . ' r.ip as ip,'
    . ' r.request as request,'
    . ' r.signature as signature,'
    . ' r.valid as signature_valid,'
    . ' r.state as state,'
    . ' r.type as type,'
    . ' r.id_common_friend as id_common_friend,'
    . ' f1.valid as certificate_ab_valid,'
    . ' f1.valid_until as valid_ab_until,'
    . ' i.id as id,'
    . ' i.url as url,'
    . ' i.description as description,'
    . ' p.public_key as public_key'
    . ' FROM'
    . ' {drupaltodrupal_incoming_requests} r,'
    . ' {drupaltodrupal_friendship_certificates} f1,'
    . ' {drupaltodrupal_instances} i'
    . ' LEFT OUTER JOIN'
    . ' {drupaltodrupal_public_keys} p'
    . ' ON'
    . ' i.public_key_id = p.id'
    . ' WHERE'
    . ' r.id_offer = f1.fcert_id'
    . ' AND f1.id_signer = i.id'
    . ' AND r.state = 0'
  );
  $n_incoming_requests = 0;
  foreach ($result as $record) {
    $n_incoming_requests++;
    $certificate_ab_valid = array(
      '#type' => 'item',
      '#markup' => drupaltodrupal_bool_to_str($record->certificate_ab_valid),
    );
    if (is_null($record->certificate_ab_valid)) {
      $certificate_ab_valid['#description'] = t('Associate a public key with the requesting instance !link.', array('!link' => l(t('here'), 'admin/drupaltodrupal/instances/' . $record->id . '/public_key')));
    }
    $certificate_ab_valid = drupal_render($certificate_ab_valid);
    $signature_valid = array(
      '#type' => 'item',
      '#markup' => drupaltodrupal_bool_to_str($record->signature_valid),
//      '#description' => isset($value['description']) ? $value['description'] : '',
    );
    if (is_null($record->signature_valid)) {
      $signature_valid['#description'] = t('Associate a public key with the requesting instance !link.', array('!link' => l(t('here'), 'admin/drupaltodrupal/instances/' . $record->id . '/public_key')));
    }
    $signature_valid = drupal_render($signature_valid);


    $valid_ab_until = array(
      '#type' => 'item',
      '#markup' => drupaltodrupal_get_relative_time($record->valid_ab_until),
    );
    if ($record->valid_ab_until < drupaltodrupal_get_time()) {
      $valid_ab_until['#description'] = t('The request is not valid anymore and cannot be accepted.');
    }
    $valid_ab_until = drupal_render($valid_ab_until);


    $rows = array(
      array(t('URL'), l(check_plain($record->url), $record->url)),
      array(t('Instance ID'), l($record->id, 'admin/drupaltodrupal/instances/' . $record->id . '/details')),
      array(t('sent from'), check_plain($record->ip)),
      array(t('received'), drupaltodrupal_get_relative_time($record->time_received)),
      array(t('contact'), check_plain($record->email)),
      array(t('message'), check_plain($record->message)),
      array(t('certificate for friendship valid'), $certificate_ab_valid),
      array(t('signature valid'), $signature_valid),
      array(t('friendship valid until'), $valid_ab_until),
      array(t('type'), drupaltodrupal_get_request_type($record->type)),
    );
    if ($record->id_common_friend !== null) {
      // check validity
      if (array_key_exists($record->id_common_friend, $friend_ids)) {
        // friend exists, check if there is valid friendship between the common
        $res = db_query('SELECT fcert_id, valid_until FROM {drupaltodrupal_friendship_certificates} WHERE valid=1 AND id_signer=:id_signer AND id_friend=:id_friend AND valid_until>=:t ORDER BY valid_until DESC', array(':id_signer' => $record->id_common_friend, ':id_friend' => $record->id, ':t' => drupaltodrupal_get_time()));
        if ($rec = $res->fetchAssoc()) {
          $common_friend = array(
            '#type' => 'item',
            '#markup' => $friend_ids[$record->id_common_friend],
            '#description' => t('the instance with id !link is a common friend of your instance and the requesting instance', array('!link' => l($record->id_common_friend, 'admin/drupaltodrupal/instances/' . $record->id_common_friend . '/details'))),
          );
          $rows[] = array(t('Common friend'), drupal_render($common_friend));
        }
      }
    }
    $acceptable = $record->signature_valid && $record->certificate_ab_valid && $record->valid_ab_until >= drupaltodrupal_get_time();
    $build['main']['instance' . $record->inreq_id] = array(
      '#title' => t('Incoming request #@id', array('@id' => $record->inreq_id)),
      '#type' => 'fieldset',
    );
    $build['main']['instance' . $record->inreq_id][] = array(
      '#theme' => 'table',
      '#header' => array(t('Type'), t('Value')),
      '#rows' => $rows,
      '#empty' => t('No information available.'),
      /*
      '#prefix' => "<div><h2></h2>",
      '#suffix' =>
        drupal_render(drupal_get_form("drupaltodrupal_form_accept_incoming_request_{$record->inreq_id}")) .
        drupal_render(drupal_get_form("drupaltodrupal_form_ignore_incoming_request_{$record->inreq_id}")),
      */
    );
    $form_state = array();
    $form_state['build_info']['args'] = array($record->inreq_id);
    $build['main']['instance' . $record->inreq_id][] = drupal_get_form("drupaltodrupal_form_ignore_incoming_request_{$record->inreq_id}");
    $build['main']['instance' . $record->inreq_id][] = drupal_get_form("drupaltodrupal_form_ignore_all_incoming_requests_{$record->id}");
    if ($acceptable) {
      $build['main']['instance' . $record->inreq_id][] = drupal_get_form("drupaltodrupal_form_accept_incoming_request_{$record->inreq_id}");
    }
  }
  if (!$n_incoming_requests) {
    $build['main']['#description'] = t('No incoming requests found.');
  }
  else {
    $build['main'][] = drupal_get_form('drupaltodrupal_form_ignore_really_all_incoming_requests');
  }
  return $build;
}
