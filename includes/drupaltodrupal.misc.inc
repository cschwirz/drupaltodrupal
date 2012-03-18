<?php
/**
 * @file
 * Miscellaneous helper functions.
 */

/**
 * Return the version number of the module as a string
 */
function drupaltodrupal_get_version() {
  return DRUPALTODRUPAL_VERSION_STRING;
}

/**
 * Returns the options for an XML-RPC request, i.e. the third argument given
 * to xmlrpc(...)
 */
function drupaltodrupal_xmlrpc_options() {
  return array(
    'timeout' => variable_get('drupaltodrupal_timeout', DRUPALTODRUPAL_TIMEOUT_DEFAULT),
  );
}

function drupaltodrupal_check_url($url) {
  if (!is_string($url)) {
    return FALSE;
  }
  if (!preg_match('/^https?:\/\/.+\/$/', $url)) {
    return FALSE;
  }
  return strlen($url) <= DRUPALTODRUPAL_INSTANCE_URL_MAX_LENGTH;
}
/**
 * Include the instance with the provided ID in all groups that are marked as default.
 */
function drupaltodrupal_set_default_group_memberships($id) {
  $result = db_query('SELECT id FROM {drupaltodrupal_groups} WHERE auto_member=1');
  $records = $result->fetchAll();
  if (!empty($records)) {
    db_delete('drupaltodrupal_group_memberships')
      ->condition('instance_id', $id)->execute();
    $query = db_insert('drupaltodrupal_group_memberships')->fields(array('group_id', 'instance_id'));
    foreach ($records as $gid) {
      $query->values(array('group_id' => $gid->id, 'instance_id' => $id));
    }
    $query->execute();
  }
}

/**
 * Return the public key of $instance
 */
function drupaltodrupal_get_public_key_server($instance) {
  /*
  $result = db_select('drupaltodrupal_instances', 'i')
    ->fields('i', array('public_key'))
    ->condition('url', $instance, '=')
    ->execute();
  */
  $result = db_query('SELECT i.id as id, p.public_key as public_key FROM {drupaltodrupal_instances} i, {drupaltodrupal_public_keys} p WHERE i.url=:url AND i.public_key_id = p.id', array(':url' => $instance));

  if ($row = $result->fetchAssoc()) {
    $public_key = $row['public_key'];
    return $public_key;
  }
  return null;
}

/**
 * Return string stating friendship
 */
function drupaltodrupal_get_friendship_string($signer, $friend, $until) {
  return drupaltodrupal_implode(
    array(
      'type' => 'friendship',
      'signer' => $signer,
      'friend' => $friend,
      'until' => $until,
    )
  );
}

/**
 * Set valid flag of all incoming requests and all friendship requests signed by
 * the instance with id $id to null (= unchecked).
 */
function drupaltodrupal_invalidate($id) {
  $num_updated = db_update('drupaltodrupal_friendship_certificates')
    ->fields(array('valid' => null))
    ->condition('id_signer', $id)
    ->execute();
  $result = db_query('SELECT ir.inreq_id AS id FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_incoming_requests} ir WHERE ir.id_offer = f.fcert_id AND id_signer=:id', array(':id' => $id));
  $ids = array();
  foreach ($result as $record) {
    $ids[] = $record->id;
  }
  if (!empty($ids)) {
    $num_updated = db_update('drupaltodrupal_incoming_requests')
      ->fields(array('valid' => null))
      ->condition('inreq_id', $ids, 'IN')
      ->execute();
  }
}
/**
 * Revalidate flag of all incoming requests and all friendship requests signed by
 * the instance with id $id.
 */
function drupaltodrupal_validate($id) {
  // friendship certificates
  $result = db_query('SELECT p.public_key AS public_key, i.url AS url FROM {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id WHERE i.id=:id', array(':id' => $id));
  if (!($record = $result->fetchAssoc())) {
    return;
  }
  $public_key_b = $record['public_key'];
  $entity_b = $record['url'];
  $result = db_query('SELECT f.fcert_id AS fid, i.url as url, f.certificate AS certificate, f.valid_until AS valid_until FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i WHERE f.id_signer=:id AND f.id_friend=i.id', array(':id' => $id));
  $records = $result->fetchAll();
  $updates = array();
  foreach ($records as $record) {
    $certificate_ba_verified = null;
    $until_ba = $record->valid_until;
    $entity_a = $record->url;
    $certificate_ba = $record->certificate;
    $fid = $record->fid;
    if (!is_null($public_key_b)) {
      if (openssl_verify(drupaltodrupal_get_friendship_string($entity_b, $entity_a, $until_ba), $certificate_ba, $public_key_b) !== 1) {
        $certificate_ba_verified = 0;
      }
      else {
        $certificate_ba_verified = 1;
      }
    }
    $updates[$fid] = $certificate_ba_verified;
  }
  // TODO: group updates by 0, 1, null and make at most three updates
  foreach ($updates as $key => $value) {
    $num_updated = db_update('drupaltodrupal_friendship_certificates')
      ->fields(array('valid' => $value))
      ->condition('fcert_id', $key)
      ->execute();
  }
  // incoming requests
  $result = db_query(
    'SELECT' .
    ' ir.inreq_id as inreq_id,' .
    ' ir.request as request,' .
    ' ir.signature as signature,' .
    ' p.public_key as public_key' .
    ' FROM' .
    ' {drupaltodrupal_friendship_certificates} f,' .
    ' {drupaltodrupal_incoming_requests} ir,' . 
    ' {drupaltodrupal_instances} i' .
    ' LEFT OUTER JOIN' .
    ' {drupaltodrupal_public_keys} p' .
    ' ON' .
    ' p.id=i.public_key_id' .
    ' WHERE' .
    ' i.id=:id AND' .
    ' f.id_signer=i.id AND' .
    ' ir.id_offer=f.fcert_id',
    array(':id' => $id)
  );
  $records = $result->fetchAll();
  $updates = array();
  $updates['false'] = array();
  $updates['null'] = array();
  $updates['true'] = array();
  foreach ($records as $record) {
    $inreq_id = $record->inreq_id;
    $encrypted_data = $record->request;
    $signature = $record->signature;
    $public_key = $record->public_key;
    if ($public_key === null) {
      $updates['null'][] = $inreq_id;
    }
    else {
      if (openssl_verify($encrypted_data, $signature, $public_key) !== 1) {
        $updates['false'][] = $inreq_id;
      }
      else {
        $updates['true'][] = $inreq_id;
      }
    }
  }
  foreach (array('true' => 1, 'false' => 0, 'null' => null) as $key => $value) {
    if (!empty($updates[$key])) {
      $num_updated = db_update('drupaltodrupal_incoming_requests')
        ->fields(array('valid' => $value))
        ->condition('inreq_id', $updates[$key])
        ->execute();
    }
  }
  return;
}

function drupaltodrupal_get_time() {
  return time() + variable_get('drupaltodrupal_utc_offset', DRUPALTODRUPAL_UTC_OFFSET_DEFAULT);
}
// TODO: use t(.) for units
function drupaltodrupal_format_unit($v, $unit, $skip=FALSE) {
  if (!$v && $skip) {
    return '';
  }
  else {
    return strval($v) . ' ' . $unit . ($v != 1 ? 's' : '');
  }
}
function drupaltodrupal_get_absolute_time($t) {
  return date('F j, Y, g:i a', $t);
}
function drupaltodrupal_get_relative_time($t) {
  $diff = $t - drupaltodrupal_get_time();
  $prefix = '';
  $suffix = '';
  if ($diff > 0) {
    $absdiff = $diff;
    $prefix = 'in ';
  }
  else {
    $absdiff = -$diff;
    $suffix = ' ago';
  }
  $diffs = array(
    gmdate('Y', $absdiff) - 1970,
    gmdate('n', $absdiff) - 1,
    gmdate('j', $absdiff) - 1,
    gmdate('G', $absdiff),
    intval(gmdate('i', $absdiff)),
    intval(gmdate('s', $absdiff)),
    0,
  );
  $relative = '';
  $units = array('year', 'month', 'day', 'hour', 'minute', 'second', '');
  for ($i = 0; $i < 6; $i++) {
    if ($i == 5 || $diffs[$i] > 0) {
      $relative = drupaltodrupal_format_unit($diffs[$i], $units[$i]);
      $tmp = drupaltodrupal_format_unit($diffs[$i + 1], $units[$i + 1], TRUE);
      if ($tmp) {
        $relative = "$relative, $tmp";
      }
      break;
    }
  }
  return $prefix . $relative . $suffix;
}

function drupaltodrupal_get_self_instance(&$id, &$url, &$public_key) {
  // TODO: use row with minimal id?
  $id = variable_get('drupaltodrupal_instance_id', $id);
  $result = db_query('SELECT p.public_key as public_key, i.url as url FROM {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id WHERE i.id=:id', array(':id' => $id));
  /*
  $result = db_select('drupaltodrupal_instances', 'i')
    ->fields('i', array('url', 'public_key'))
    ->condition('id', $id, '=')
    ->execute();
  */
  if ($row = $result->fetchAssoc()) {
    $url = $row['url'];
    $public_key = $row['public_key'];
  }
  else {
    // TODO
  }
}

function drupaltodrupal_bool_to_str($b) {
  if (is_null($b)) {
    return t('may be');
  }
  else {
    return $b ? t('true') : t('false');
  }
}

// gets ids of next friends, marks out-dated requests
function drupaltodrupal_get_next_ids() {
  $result = db_query('SELECT o.outreq_id as outreq_id, o.next_send AS next_send,' .
    ' f.id_friend as id_friend, f.valid_until as valid_until' .
    ' FROM {drupaltodrupal_outgoing_requests} o,' .
    ' {drupaltodrupal_friendship_certificates} f' .
    ' WHERE o.next_send IS NOT NULL' .
    ' AND o.id_offer = f.fcert_id' .
    ' ORDER BY o.outreq_id DESC'
  );
  $seen_friend_ids = array();
  $current_time = drupaltodrupal_get_time();
  $next_ids = array();
  $delete_ids = array();
  foreach ($result as $record) {
    $flag = FALSE;
    if (!array_key_exists($record->id_friend, $seen_friend_ids)) {
      $seen_friend_ids[$record->id_friend] = 1;
      if ($record->valid_until < $current_time) {
        $flag = TRUE;
      }
      elseif ($record->next_send <= $current_time) {
        $next_ids[] = $record->outreq_id;
      }
    }
    else {
      $flag = TRUE;
    }
    if ($flag) {
      $delete_ids[] = $record->outreq_id;
    }
  }
  if ($delete_ids) {
    $num_updated = db_update('drupaltodrupal_outgoing_requests')
      ->fields(array('next_send' => null))
      ->condition('outreq_id', $delete_ids)
      ->execute();
  }
  return $next_ids;
}

function drupaltodrupal_get_request_type($i) {
  switch ($i) {
    case 0:
      return 'request';
    case 1:
      return 'accept';
    case 2:
      return 'renew';
    default:
      return 'N/A';
  }
}
/**
 * Remove outdated entries from
 *  drupaltodrupal_friendship_certificates,
 *  drupaltodrupal_incoming_requests,
 *  drupaltodrupal_outgoing_requests
 */
function drupaltodrupal_remove_old_database_entries($verbose = TRUE) {
  $t = drupaltodrupal_get_time() - variable_get('drupaltodrupal_garbage_time', DRUPALTODRUPAL_GARBAGE_TIME_DEFAULT) * 60;
  $result = db_query('SELECT fcert_id FROM {drupaltodrupal_friendship_certificates} WHERE valid_until<=:t', array(':t' => $t));
  $records = $result->fetchAll();
  $fcert_ids = array();
  foreach ($records as $record) {
    $fcert_ids[] = $record->fcert_id;
  }
  if (!empty($fcert_ids)) {
    $num_updated_outgoing = db_delete('drupaltodrupal_outgoing_requests')
      ->condition('id_offer', $fcert_ids)
      ->execute();
    $num_updated_incoming = db_delete('drupaltodrupal_incoming_requests')
      ->condition('id_offer', $fcert_ids)
      ->execute();
    $num_updated_certificates = db_delete('drupaltodrupal_friendship_certificates')
      ->condition('fcert_id', $fcert_ids)
      ->execute();
    if ($verbose) {
      drupal_set_message(t('Removed @a certificates, @b outgoing requests, and @c incoming request.', array('@a' => $num_updated_certificates, '@b' => $num_updated_outgoing, '@c' => $num_updated_incoming)));
    }
  }
}
function drupaltodrupal_send_requests($verbose = TRUE) {
  // get information about this instance, private key.
  drupaltodrupal_get_self_instance($id_b, $entity_b, $public_key_b);
  $private_key_b = variable_get('drupaltodrupal_private_key');
  $ids = drupaltodrupal_get_next_ids();
  if ($verbose) {
    drupal_set_message(t('Processing @count outgoing request(s).', array('@count' => sizeof($ids))));
  }

  foreach ($ids as $outreq_id) {
    if ($verbose) {
      drupal_set_message(t('Processing outgoing request with id @id.', array('@id' => $outreq_id)));
    }
    // get entry from the database:
    // request, associated certificate, friend instance
    $result = db_query(
        'SELECT'
      . ' o.email as email,'
      . ' o.message as message,'
      . ' o.state as state,'
      . ' o.reply as reply,'
      . ' o.type as type,'
      . ' o.id_common_friend as id_common_friend,'
      . ' i.url as i_url,'
      . ' p.public_key as i_public_key,'
      . ' f.valid as f_valid,'
      . ' f.valid_until as f_valid_until,'
      . ' f.certificate as f_certificate'
      . ' FROM'
      . ' {drupaltodrupal_outgoing_requests} o,'
      . ' {drupaltodrupal_friendship_certificates} f,'
      . ' {drupaltodrupal_instances} i'
      . ' LEFT OUTER JOIN'
      . ' {drupaltodrupal_public_keys} p'
      . ' ON'
      . ' p.id = i.public_key_id'
      . ' WHERE'
      . ' o.outreq_id = :outreq_id AND'
      . ' f.fcert_id = o.id_offer AND'
      . ' i.id = f.id_friend',
      array(':outreq_id' => $outreq_id)
    );
    $record = $result->fetchObject();
    // id not valid? -> continue
    if (!$record) {
      if ($verbose) {
        drupal_set_message(t('Id @id could not be found', array('@id' => $outreq_id)), 'error');
      }
      continue;
    }
    
    $public_key_a = $record->i_public_key;
    $entity_a = $record->i_url;
    $certificate_ba = $record->f_certificate;
    $until_ba = $record->f_valid_until;
    $email = $record->email;
    $message = $record->message;
    $type = $record->type;

    // check if friend's public key is availabe
    if (is_null($public_key_a)) {
      if ($verbose) {
        drupal_set_message(t('Public key for request with id @id not yet available.', array('@id' => $outreq_id)));
      }
      continue;
    }
    $c = '';
    $certificate_cb = '';
    $until_cb = 0;
    if ($record->id_common_friend) {
      $res = db_query('SELECT f.certificate as certificate, f.valid_until as valid_until, i.url as url FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i WHERE f.valid=1 AND f.id_signer=:id_signer AND f.id_friend=:id_friend AND f.valid_until>=:t AND i.id=f.id_signer ORDER BY valid_until DESC', array(':id_signer' => $record->id_common_friend, ':id_friend' => $id_b, ':t' => drupaltodrupal_get_time()));
      if ($rec = $res->fetchAssoc()) {
        $c = $rec['url'];
        $certificate_cb = $rec['certificate'];
        $until_cb = $rec['valid_until'];
      }
    }
    // update next_send *before* sending
    $num_updated = db_update('drupaltodrupal_outgoing_requests')
      ->fields(array('next_send' => drupaltodrupal_get_time() + variable_get('drupaltodrupal_resend_time', DRUPALTODRUPAL_RESEND_TIME_DEFAULT) * 60))
      ->condition('outreq_id', $outreq_id)
      ->execute();

    // send request
    $request = array(
      'type' => drupaltodrupal_get_request_type($type),
      'version' => drupaltodrupal_get_version(),
      'a' => $entity_a,
      'b' => $entity_b,
      'c' => $c,
      'certificate_cb' => $certificate_cb,
      'until_cb' => $until_cb,
      'certificate_ba' => $certificate_ba,
      'until_ba' => $until_ba,
      'mail' => $email,
      'message' => $message,
      'time' => drupaltodrupal_get_time(),
      'tok' => 42,
    );
    $imploded_request = drupaltodrupal_implode($request);
    if ($imploded_request === FALSE) {
      continue;
    }
    // encrypt request
    if (!drupaltodrupal_encrypt($imploded_request, $encrypted_request, $public_key_a)) {
      if ($verbose) {
        drupal_set_message(t('Failed encrypting request with id @id.', array('@id' => $outreq_id)), 'error');
      }
      continue;
    }
    // sign encrypted request
    $signed_request = drupaltodrupal_sign($encrypted_request, $private_key_b, $entity_b);
    if ($signed_request === FALSE) {
      if ($verbose) {
        drupal_set_message(t('Failed signing request with id @id.', array('@id' => $outreq_id)), 'error');
      }
      continue;
    }
    $server_a = url($entity_a . "/xmlrpc.php");
    if ($verbose) {
      drupal_set_message(t('Sending request with id @id.', array('@id' => $outreq_id)));
    }
    if ($verbose) {
      drupal_set_message(t('Sending request to @url', array('@url' => $entity_a)));
      //drupal_set_message('request64 = ' . base64_encode($signed_request));
    }
    $result = xmlrpc($server_a, array('drupalToDrupal.requestFriendship' => array(base64_encode($signed_request), drupaltodrupal_get_version())), drupaltodrupal_xmlrpc_options());
    if ($result === FALSE) {
      if ($verbose) {
        drupal_set_message(t("Error return from xmlrpc(): Error: @errno, Message: @message", array('@errno' => xmlrpc_errno(), '@message' => xmlrpc_error_msg())));
      }
      $num_updated = db_update('drupaltodrupal_outgoing_requests')
        ->fields(array('reply' => -2))
        ->condition('outreq_id', $outreq_id)
        ->execute();
    }
    else {
      if ($verbose) {
        drupal_set_message(t("The XMLRPC server returned this response: @response", array('@response' => print_r($result, TRUE))));
      }
      if ($result === 0) {
        $num_updated = db_update('drupaltodrupal_outgoing_requests')
          ->fields(array('next_send' => null, 'reply' => 0))
          ->condition('outreq_id', $outreq_id)
          ->execute();
      }
      elseif (is_int($result)) {
        $num_updated = db_update('drupaltodrupal_outgoing_requests')
          ->fields(array('reply' => $result))
          ->condition('outreq_id', $outreq_id)
          ->execute();
      }
    }
  }
}

function drupaltodrupal_send_request($outreq_id, $entity_b, $private_key_b) {
  $result = db_query('SELECT o.email as email, o.message as message, o.state as state, o.reply as reply, i.url as i_url, b.public_key as i_public_key, f.valid as f_valid, f.valid_until as f_valid_until, f.certificate as f_certificate FROM {drupaltodrupal_outgoing_requests} o, {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON p.id = i.public_key_id WHERE o.outreq_id = :outreq_id AND f.fcert_id = o.id_offer AND i.id = f.id_friend', array(':outreq_id' => $outreq_id));
  $record = $result->fetchObject();
  drupal_set_message("outreq_id = {$outreq_id}");
  if (!$record) {
    drupal_set_message(t('ID @id could not be found.', array('@id' => $outreq_id)), 'error');
  }
  else {
    $public_key_a = $record->i_public_key;
    $entity_a = $record->i_url;
    if (is_null($public_key_a)) {
      drupal_set_message(t('Public key of entity A is null'), 'error');
      return;
    }
    if (is_null($record->f_certificate) || !$record->f_certificate || is_null($record->f_certificate)) {
      drupal_set_message(t('Certificate is not complete'), 'error');
      return;
    }
    /*
    if ($record->f_valid_until < time() + 3600) { // TODO
      drupal_set_message('certificate will be invalid soon', 'error');
      return;
    }
    */
    $request = array(
      'type' => 'request',
      'version' => drupaltodrupal_get_version(),
      'a' => $entity_a,
      'b' => $entity_b,
      'c' => '',
      'certificate_cb' => '',
      'until_cb' => '',
      'certificate_ba' => $record->f_certificate,
      'until_ba' => $record->f_valid_until,
      'mail' => $record->email,
      'message' => $record->message,
      'time' => time(),
      'tok' => 42,
    );
    $imploded_request = drupaltodrupal_implode($request);
    if ($imploded_request === FALSE) {
      return;
    }
    // encrypt request
    if (!drupaltodrupal_encrypt($imploded_request, $encrypted_request, $public_key_a)) {
      drupal_set_message(t('Failed encryption!'));
      return;
    }
    // sign encrypted request
    $signed_request = drupaltodrupal_sign($encrypted_request, $private_key_b, $entity_b);
    if ($signed_request === FALSE) {
      drupal_set_message(t('Failed signing!'));
      return;
    }
    $server_a = url($entity_a . "/xmlrpc.php");
    drupal_set_message('request will be sent');
    $result = xmlrpc($server_a, array('drupalToDrupal.requestFriendship' => array(base64_encode($signed_request), drupaltodrupal_get_version())), drupaltodrupal_xmlrpc_options());
    if ($result === FALSE) {
      drupal_set_message(t("Error return from xmlrpc(): Error: @errno, Message: @message", array('@errno' => xmlrpc_errno(), '@message' => xmlrpc_error_msg())));
    }
    else {
      drupal_set_message(t("The XMLRPC server returned this response: @response", array('@response' => print_r($result, TRUE))));
    }
  }
}
function drupaltodrupal_get_groups() {
  $result = db_query('SELECT id, name, description FROM {drupaltodrupal_groups}');
  $records = $result->fetchAll();
  return $records;
}
function drupaltodrupal_get_friends() {
  drupaltodrupal_get_self_instance($id, $entity, $public_key);
  $t = drupaltodrupal_get_time();
  // XXX XXX
  $result = db_query('SELECT f1.id_signer as id1, f1.id_friend as id2 , f1.verified_until as f1_valid_until, f2.verified_until as f2_valid_until, f2.url as url, f2.public_key as public_key FROM'
    . ' (SELECT id_signer, id_friend, MAX(valid_until) as verified_until FROM {drupaltodrupal_friendship_certificates} WHERE valid = 1 GROUP BY id_signer, id_friend) f1,'
    . ' (SELECT id_signer, id_friend, MAX(valid_until) as verified_until, i.url as url, p.public_key as public_key FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id WHERE valid = 1 AND f.id_signer = i.id GROUP BY id_signer, id_friend) f2'
    . ' WHERE'
    . ' f1.id_signer = f2.id_friend'
    . ' AND f1.id_friend = f2.id_signer'
    . ' AND f1.verified_until >= :t'
    . ' AND f2.verified_until >= :t'
    . ' AND f1.id_signer = :id',
    array(':t' => $t, ':id' => $id)
  );
  $friends = array();
  foreach ($result as $record) {
    $friends[] = array(
      'id' => $record->id2,
      'url' => $record->url,
      'public_key' => $record->public_key,
      'self_valid_until' => $record->f1_valid_until,
      'friend_valid_until' => $record->f2_valid_until,
    );
  }
  return $friends;
}
function drupaltodrupal_get_friends_in_group($groups) {
  if (empty($groups)) {
    return array();
  }
  drupaltodrupal_get_self_instance($id, $entity, $public_key);
  $t = drupaltodrupal_get_time();
  $result = db_query('SELECT DISTINCT f1.id_signer as id1, f1.id_friend as id2 , f1.verified_until as f1_valid_until, f2.verified_until as f2_valid_until, f2.url as url, f2.public_key as public_key FROM'
      . ' (SELECT id_signer, id_friend, MAX(valid_until) as verified_until FROM {drupaltodrupal_friendship_certificates} WHERE valid = 1 GROUP BY id_signer, id_friend) f1,'
      . ' (SELECT id_signer, id_friend, MAX(valid_until) as verified_until, i.url as url, p.public_key as public_key FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id WHERE valid = 1 AND f.id_signer = i.id GROUP BY id_signer, id_friend) f2,'
      . ' (SELECT instance_id FROM {drupaltodrupal_group_memberships} WHERE group_id IN (:groups)) gm'
      . ' WHERE'
      . ' f1.id_signer = f2.id_friend'
      . ' AND f1.id_friend = f2.id_signer'
      . ' AND f1.verified_until >= :t'
      . ' AND f2.verified_until >= :t'
      . ' AND f1.id_signer = :id'
      . ' AND f1.id_friend = gm.instance_id',
      array(':t' => $t, ':id' => $id, ':groups' => $groups)
      );
  $friends = array();
  foreach ($result as $record) {
    $friends[] = array(
        'id' => $record->id2,
        'url' => $record->url,
        'public_key' => $record->public_key,
        'self_valid_until' => $record->f1_valid_until,
        'friend_valid_until' => $record->f2_valid_until,
        );
  }
  return $friends;
}
function drupaltodrupal_add_renew_requests() {
  $friends = drupaltodrupal_get_friends();
  drupaltodrupal_get_self_instance($id_b, $entity_b, $public_key_b);
  foreach ($friends as $friend) {
    if (drupaltodrupal_get_time() + variable_get('drupaltodrupal_renew_time', DRUPALTODRUPAL_RESEND_TIME_DEFAULT) * 60 < $friend['self_valid_until']) {
      continue;
    }
    $entity_a = $friend['url'];
    $public_key_a = $friend['public_key'];//TODO
    $id_a = $friend['id'];
    // {{{ Insert certificate for friendship into database.
    // Time until when the new friendship should be valid.
    $until_ba = drupaltodrupal_get_time() + variable_get('drupaltodrupal_validity', '40') * 60;
    if (is_null($public_key_a)) {
      $friendship_ba = null;
    }
    else {
      // TODO: probably a frienship certificate should also somehow contain the
      // public key of the other entity, otherwise one could be tricked into a
      // more or less non-symmetric friendship, where the other entity could
      // use a different private/public-key pair to communicate with his real
      // friends still making them believe that he's my friend...
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
    // Insert outgoing request. 
    $insert_time = drupaltodrupal_get_time();
    $outreq_id = db_insert('drupaltodrupal_outgoing_requests')->fields(array(
      'id_offer' => $id_ba,
      'id_common_friend' => null,
      'email' => variable_get('site_mail', 'N/A'),
      'message' => 'automatic message',
      'time_inserted' => $insert_time,
      'state' => 0,
      'reply' => null,
      'next_send' => $insert_time,
      'type' => 2, // request
    ))->execute();
  }
}

