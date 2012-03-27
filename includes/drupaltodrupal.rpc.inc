<?php
/**
 * @file
 * XML RPC functions to answer remote queries.
 */

/**
 * Implements hook_xmlrpc().
 */
function drupaltodrupal_xmlrpc() {
  $methods = array();
  $methods[] = array(
    'drupalToDrupal.getVersion',
    'drupaltodrupal_get_version',
    array('string'),
    t('Get version number.')
  );
  $methods[] = array(
    'drupalToDrupal.getPublicKey',
    'drupaltodrupal_get_public_key',
    array('string'),
    t('Get public key.')
  );
  $methods[] = array(
    'drupalToDrupal.requestFriendship',
    'drupaltodrupal_request_friendship',
    array('int', 'string', 'string'),
    t('Send friendship request.')
  );
  $methods[] = array(
    'drupalToDrupal.getFriends',
    'drupaltodrupal_share_friends',
    array('string', 'string', 'string'),
    t('Get list of friends.')
  );
  $methods[] = array(
    'drupalToDrupal.secureRPC',
    'drupaltodrupal_secure_rpc',
    array('string', 'string', 'string'),
    t('General secure RPC')
  );
  return $methods;
}


function drupaltodrupal_call_secure_rpc($friend, $method_name, $arguments, &$error_string) {
  drupaltodrupal_get_self_instance($id_b, $entity_b, $public_key_b);
  $private_key_b = variable_get('drupaltodrupal_private_key');
  $entity_a = $friend['url'];
  $public_key_a = $friend['public_key'];
  $id_a = $friend['id'];
  $request = array(
    'type' => 'secure_rpc',
    'version' => drupaltodrupal_get_version(),
    'method_name' => $method_name,
    'arguments' => drupaltodrupal_implode($arguments),
    'a' => $entity_a,
    'b' => $entity_b,
    'time' => drupaltodrupal_get_time(),
    'tok' => 42,
  );
  $imploded_request = drupaltodrupal_implode($request);
  if ($imploded_request === FALSE) {
    $error_string = t('error imploding request');
    return FALSE;
  }
  // encrypt request
  if (!drupaltodrupal_encrypt($imploded_request, $encrypted_request, $public_key_a)) {
    $error_string = t('failed encryption');
    return FALSE;
  }
  // sign encrypted request
  $signed_request = drupaltodrupal_sign($encrypted_request, $private_key_b, $entity_b);
  if ($signed_request === FALSE) {
    $error_string = t('failed signing');
    return FALSE;
  }
  $server_a = url($entity_a . "/xmlrpc.php");
  $result64 = xmlrpc($server_a, array('drupalToDrupal.secureRPC' => array(base64_encode($signed_request), drupaltodrupal_get_version())), drupaltodrupal_xmlrpc_options());
  if ($result64 === FALSE) {
    $error_string = t('remote error:') . ' ' . xmlrpc_error_msg();
    return FALSE;
  }
  // convert $result64 in MIME base64
  $result = base64_decode($result64, true);
  if ($result === FALSE) {
    $error_string = t('failed base64 decoding');
    return FALSE;
  }
  // explode $result
  $exploded_result = drupaltodrupal_explode($result);
  if ($exploded_result === FALSE) {
    $error_string = t('could not explode result');
    return FALSE;
  }
  // check if exploded $request is of desired structure
  if (!drupaltodrupal_check_array($exploded_result, array('data' => 'is_string', 'signature' => 'is_string', 'signer' => 'is_string'))) {
    $error_string = t('exploded result is not of desired strucutre');
    return FALSE;
  }
  $signer = $exploded_result['signer'];
  $encrypted_data = $exploded_result['data'];
  $signature = $exploded_result['signature'];
  if (openssl_verify($encrypted_data, $signature, $public_key_a) !== 1) {
    $error_string = t('wrong signature');
    return FALSE;
  }
  // explode $encrypted_data
  $cipher_envkey = drupaltodrupal_explode($encrypted_data);
  if ($cipher_envkey === FALSE) {
    $error_string = t('failed exploding encrypted data');
    return FALSE;
  }
  // check if exploded $encrypted_data is of desired structure
  if (!drupaltodrupal_check_array($cipher_envkey, array('cipher' => 'is_string', 'env_key' => 'is_string'))) {
    $error_string = t('encrypted data not of desired structure');
    return FALSE;
  }
  // finally decrypt
  $private_key_b = variable_get('drupaltodrupal_private_key');
  if (!openssl_open($cipher_envkey['cipher'], $data, $cipher_envkey['env_key'], $private_key_b)) {
    $error_string = t('failed decrypytion');
    return FALSE;
  }
  // explode $data
  $exploded_data = drupaltodrupal_explode($data);
  if ($exploded_data === FALSE) {
    $error_string = t('failed explosion');
    return FALSE;
  }
  // check if $exploded_data is of desired structure
  $check = array(
      'type' => 'is_string',
      'version' => 'is_string',
      'method_name' => 'is_string',
      'return' => 'is_string',
      'a' => 'is_string',
      'b' => 'is_string',
      'time' => 'drupaltodrupal_check_convert_int',
      'tok' => 'drupaltodrupal_check_convert_int',
  );
  if (!drupaltodrupal_check_array($exploded_data, $check)) {
    $error_string = t('exploded data not of desired structure');
    return FALSE;
  }
  if ($exploded_data['type'] !== 'secure_rpc_return') {
    $error_string = t('type != secure_rpc_return');
    return FALSE;
  }
  if ($exploded_data['version'] !== drupaltodrupal_get_version()) {
    $error_string = t('version in exploded data mismatch');
    return FALSE;
  }
  if ($method_name !== $exploded_data['method_name']) {
    $error_string = t('method_name mismatch');
  }
  $entity_a2 = $exploded_data['a'];
  $entity_b2 = $exploded_data['b'];
  if ($entity_a2 !== $entity_a) {
    $error_string = t('entity_a in exploded data mismatch');
    return FALSE;
  }
  if ($entity_b2 !== $entity_b) {
    $error_string = t('entity_b in exploded data mismatch');
    return FALSE;
  }
  // TODO: time, tok
  return $exploded_data['return'];
}

/**
 *
 */
function drupaltodrupal_secure_rpc($request64, $version) {
  $t = drupaltodrupal_get_time();
  // check version
  if ($version !== drupaltodrupal_get_version()) {
    return xmlrpc_error(0, 'wrong version');
  }
  // convert $request64 if in MIME base64
  // note that if $request64 is binary, an xml-rpc-error is returned
  $request = base64_decode($request64, true);
  if ($request === FALSE) {
    return xmlrpc_error(0, 'failed base64 decoding');
  }
  // explode $request
  $message = drupaltodrupal_explode($request);
  if ($message === FALSE) {
    return xmlrpc_error(0, 'could not explode request');
  }
  // check if exploded $request is of desired structure
  if (!drupaltodrupal_check_array($message, array('data' => 'is_string', 'signature' => 'is_string', 'signer' => 'is_string'))) {
    return xmlrpc_error(0, 'request is not of desired strucutre');
  }
  $signer = $message['signer'];
  $encrypted_data = $message['data'];
  $signature = $message['signature'];
  drupaltodrupal_get_self_instance($id_a, $entity_a, $public_key_a);
  // check friendship
  $result = db_query('SELECT f1.id_signer as id1, f1.id_friend as id2 , f1.verified_until as f1_valid_until, f2.verified_until as f2_valid_until, f2.url as url, f2.public_key as public_key FROM'
      . ' (SELECT id_signer, id_friend, MAX(valid_until) as verified_until FROM {drupaltodrupal_friendship_certificates} WHERE valid = 1 GROUP BY id_signer, id_friend) f1,'
      . ' (SELECT id_signer, id_friend, MAX(valid_until) as verified_until, i.url as url, p.public_key as public_key FROM {drupaltodrupal_friendship_certificates} f, {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id WHERE valid = 1 AND f.id_signer = i.id GROUP BY id_signer, id_friend) f2'
      . ' WHERE'
      . ' f1.id_signer = f2.id_friend'
      . ' AND f1.id_friend = f2.id_signer'
      . ' AND f1.verified_until >= :t'
      . ' AND f2.verified_until >= :t'
      . ' AND f1.id_signer = :id'
      . ' AND f2.url = :url',
      array(':t' => $t, ':id' => $id_a, ':url' => $signer)
      );
  if ($result->rowCount() == 0) {
    return xmlrpc_error(0, 'no friendship found');
  }
  $record = $result->fetchObject();
  $id_b = $record->id2;
  $public_key_b = $record->public_key;
  if (openssl_verify($encrypted_data, $signature, $public_key_b) !== 1) {
    return xmlrpc_error(0, 'wrong signature');
  }
  $entity_b = $record->url;
  // explode $encrypted_data
  $cipher_envkey = drupaltodrupal_explode($encrypted_data);
  if ($cipher_envkey === FALSE) {
    return xmlrpc_error(0, 'failed exploding encrypted data');
  }
  // check if exploded $encrypted_data is of desired structure
  if (!drupaltodrupal_check_array($cipher_envkey, array('cipher' => 'is_string', 'env_key' => 'is_string'))) {
    return xmlrpc_error(0, 'encrypted data not of desired structure');
  }
  // finally decrypt
  $private_key_a = variable_get('drupaltodrupal_private_key');
  if (!openssl_open($cipher_envkey['cipher'], $data, $cipher_envkey['env_key'], $private_key_a)) {
    return xmlrpc_error(0, 'failed decrypytion');
  }
  // explode $data
  $exploded_data = drupaltodrupal_explode($data);
  if ($exploded_data === FALSE) {
    return xmlrpc_error(0, 'failed explosion');
  }
  // check if $exploded_data is of desired structure
  $check = array(
    'type' => 'is_string',
    'version' => 'is_string',
    'method_name' => 'is_string',
    'arguments' => 'is_string',
    'a' => 'is_string',
    'b' => 'is_string',
    'time' => 'drupaltodrupal_check_convert_int',
    'tok' => 'drupaltodrupal_check_convert_int',
  );
  if (!drupaltodrupal_check_array($exploded_data, $check)) {
    return xmlrpc_error(0, 'exploded data not of desired structure');
  }
  if ($exploded_data['type'] !== 'secure_rpc') {
    return xmlrpc_error(0, 'type != secure_rpc');
  }
  if ($exploded_data['version'] !== $version) {
    return xmlrpc_error(0, 'version in exploded data mismatch');
  }
  $method_name = $exploded_data['method_name'];
  $arguments = $exploded_data['arguments'];
  $entity_a2 = $exploded_data['a'];
  $entity_b2 = $exploded_data['b'];
  if ($entity_a2 !== $entity_a) {
    return xmlrpc_error(0, 'entity_a in exploded data mismatch');
  }
  if ($entity_b2 !== $entity_b) {
    return xmlrpc_error(0, 'entity_b in exploded data mismatch');
  }
  $time_sent = $exploded_data['time'];
  if ($time_sent < $t - variable_get('drupaltodrupal_delta_0', DRUPALTODRUPAL_DELTA_0_DEFAULT) || $time_sent > $t + variable_get('drupaltodrupal_delta_1', DRUPALTODRUPAL_DELTA_1_DEFAULT)) {
    return xmlrpc_error(0, 'timestamp failed check');
  }
  // check if method name is properly mapped
  $hks = module_invoke_all('drupaltodrupal_secure_rpc');
  if (!array_key_exists($method_name, $hks)) {
    return xmlrpc_error(0, 'method does not exist');
  }
  $hk = $hks[$method_name];
  if (!is_array($hk)) {
    return xmlrpc_error(0, 'method exists but is not properly implemented');
  }
  $result = db_query('SELECT * FROM {drupaltodrupal_group_memberships} g, {drupaltodrupal_permissions} p WHERE g.instance_id=:id AND g.group_id=p.group_id AND p.method=:method', array(':id' => $id_b, ':method' => $method_name));
  if (!$result->rowCount()) {
    return xmlrpc_error(0, 'permission denied');
  }
  $argument_checks = (array_key_exists('arguments', $hk) ? $hk['arguments'] : array());
  $callback = $hk['callback'];
  $exploded_arguments = drupaltodrupal_explode($arguments);
  if ($exploded_arguments === FALSE) {
    return xmlrpc_error(0, 'failed explosion of arguments');
  }
  if (!drupaltodrupal_check_array($exploded_arguments, $argument_checks)) {
    return xmlrpc_error(0, 'arguments failed check');
  }
  $rpc_info = array(
    'url' => $entity_b,
    'id' => $id_b,
    'public_key' => $public_key_b,
    'ip' => $_SERVER['REMOTE_ADDR'],
  );
  try {
    $ret = call_user_func($callback, $exploded_arguments, $rpc_info);
  }
  catch (DrupalToDrupalRemoteException $e) {
    return xmlrpc_error(0, $e->getMessage());
  }
  if (!is_string($ret)) {
    return xmlrpc_error(0, 'called function did not return string');
  }
  $return = array(
    'type' => 'secure_rpc_return',
    'version' => drupaltodrupal_get_version(),
    'method_name' => $method_name,
    'return' => $ret,
    'a' => $entity_a,
    'b' => $entity_b,
    'time' => drupaltodrupal_get_time(),
    'tok' => 42,
  );
  $imploded_return = drupaltodrupal_implode($return);
  if ($imploded_return === FALSE) {
    return xmlrpc_error(0, 'error imploding return');
  }
  // encrypt return
  if (!drupaltodrupal_encrypt($imploded_return, $encrypted_return, $public_key_b)) {
    return xmlrpc_error(0, 'failed encryption!');
  }
  // sign encrypted return 
  $signed_return = drupaltodrupal_sign($encrypted_return, $private_key_a, $entity_a);
  if ($signed_return === FALSE) {
    return xmlrpc_error(0, 'failed signing');
  }
  return base64_encode($signed_return);
}

/**
 * Provide public key.
 */
function drupaltodrupal_get_public_key() {
  drupaltodrupal_get_self_instance($id, $url, $public_key);
  return $public_key;
}

// TODO: used for showcase, basically copy & paste from
//  drupaltodrupal.show_incoming_requests.inc
function drupaltodrupal_accept_request_by_id($id) {
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
    return;
  }
  $record = $result->fetchObject();
  $acceptable = $record->signature_valid && $record->certificate_ab_valid && $record->valid_ab_until > drupaltodrupal_get_time();
  if (!$acceptable) {
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
  $num_updated = db_update('drupaltodrupal_incoming_requests')
    ->fields(array(
      'state' => 1,
    ))
    ->condition('inreq_id', $id)
    ->execute();
}
/**
 * Process friendship request.
 *
 * Note that in the following, the substrings at the right of the '='-character
 * are just examples / descriptive, the only substring to strictly match this
 * description is the one following "type=".
 * Further note that the order of the "key=value"-pairs must be exactly as
 * described below.
 * "data=encrypted_data,signature=signer's_signature,signer=Bob".
 *
 * The given $request must be of the form
 * "cipher=encrypted data,env_key=envelope key"
 *
 * encrypted_data must be of the form
 * After decrypting, in a scenario where Bob requests Alice's friendship, and
 * Charlie is a common friend of the two, data should be of the form
 * "type=friendship_request," .
 * "version=0.01," .
 * "a=Alice," .
 * "b=Bob," .
 * "c=Charlie," .
 * "signature_cb=signature by Charlie proving friendship between Charlie and Bob," .
 * "until_cb=timestamp stating when friendship between Charlie and Bob expires," .
 * "signature_ba=signature by Bob stating friendship between Bob and Alice," .
 * "until_ba=timestamp stating when friendship between Bob and Alice expires," .
 * "mail=bob@example.com," .
 * "message=please accept my frienship request," .
 * "time=12345," .
 * "tok=42"
 * Note that both time and tok should have integer values. Further note that
 * the values of 'b' (in the decrypted data) and 'signer' (in $request) have to
 * match, and that the value of "version" and $version must match.
 *
 * Return values:
 *
 * versions do not match
 * failed to decode from MIME base 64
 * failed to explode request
 * exploded request not of desired structure
 * failed to explode $encrypted_data
 * exploded $encrypted_data is not of desired structure
 * decryption failed
 * failed to exploded $data
 * exploded $data is not of desired structure
 * sending entity and signing entity mismatch
 * this entity and entity specified as receiver mismatch
 * proposed frienship does not last long enough
 * TBC :)
 */
function drupaltodrupal_request_friendship($request64, $version) {
  $time_received = drupaltodrupal_get_time();

  // check version
  if ($version !== drupaltodrupal_get_version()) {
    return -1;
  }

  // convert $request64 if in MIME base64
  // note that if $request64 is binary, an xml-rpc-error is returned
  $request = base64_decode($request64, true);
  if ($request === FALSE) {
    return 1;
  }
  
  // explode $request
  $message = drupaltodrupal_explode($request);
  if ($message === FALSE) {
    return 2;
  }

  // check if exploded $request is of desired structure
  if (!drupaltodrupal_check_array($message, array('data' => 'is_string', 'signature' => 'is_string', 'signer' => 'is_string'))) {
    return 3;
  }

  // get signer's public key
  $signer = $message['signer'];
  if (!drupaltodrupal_check_url($signer)) {
    return 18;
  }
  $encrypted_data = $message['data'];
  $public_key_b = drupaltodrupal_get_public_key_server($signer);
  $signature = $message['signature'];
  $signature_verified = null;
  if ($public_key_b) {
    // check signature
    if (openssl_verify($encrypted_data, $signature, $public_key_b) !== 1) {
      $signature_verified = FALSE;
    }
    else {
      $signature_verified = TRUE;
    }
  }
  else {
    $public_key_b = null;
  }

  // explode $encrypted_data
  $cipher_envkey = drupaltodrupal_explode($encrypted_data);
  if ($cipher_envkey === FALSE) {
    return 6;
  }

  // check if exploded $encrypted_data is of desired structure
  if (!drupaltodrupal_check_array($cipher_envkey, array('cipher' => 'is_string', 'env_key' => 'is_string'))) {
    return 7;
  }

  // finally decrypt
  $private_key_a = variable_get('drupaltodrupal_private_key');
  if (!openssl_open($cipher_envkey['cipher'], $data, $cipher_envkey['env_key'], $private_key_a)) {
    return 8;
  }

  // explode $data
  $exploded_data = drupaltodrupal_explode($data);
  if ($exploded_data === FALSE) {
    return 9;
  }
  
  // check if $exploded_data is of desired structure
  $check = array(
    'type' => 'is_string',
    'version' => 'is_string',
    'a' => 'drupaltodrupal_check_url',
    'b' => 'drupaltodrupal_check_url',
    'c' => 'is_string',
    'certificate_cb' => 'is_string',
    'until_cb' => 'drupaltodrupal_check_convert_int',
    'certificate_ba' => 'is_string',
    'until_ba' => 'is_string',
    'mail' => 'is_string',
    'message' => 'is_string',
    'time' => 'drupaltodrupal_check_convert_int',
    'tok' => 'drupaltodrupal_check_convert_int',
  );
  if (!drupaltodrupal_check_array($exploded_data, $check)) {
    return 10;
  }
  if ($exploded_data['version'] !== $version) {
    return 15;
  }
  $type = $exploded_data['type'];
  $entity_a = $exploded_data['a'];
  $entity_b = $exploded_data['b'];
  $entity_c = $exploded_data['c'];
  $until_cb = $exploded_data['until_cb'];
  $until_ba = $exploded_data['until_ba'];
  $time_sent = $exploded_data['time'];
  $message = $exploded_data['message'];
  $email = $exploded_data['mail'];
  $certificate_ba = $exploded_data['certificate_ba'];
  $certificate_cb = $exploded_data['certificate_cb'];
  $until_cb = $exploded_data['until_cb'];
  drupaltodrupal_get_self_instance($id_a, $entity_a2, $public_key_a);
  // check type
  if ($type == 'request') {
    $type_id = 0;
  }
  elseif ($type == 'accept') {
    $type_id = 1;
  }
  elseif ($type == 'renew') {
    $type_id = 2;
  }
  else {
    // unknown type
    return 14;
  }

  // check if entity signing the message and b (i.e. requester) match
  if ($signer !== $entity_b) {
    return 11;
  }
  if ($entity_a2 !== $entity_a) {
    return 12;
  }
  if ($time_sent < $time_received - variable_get('drupaltodrupal_delta_0', DRUPALTODRUPAL_DELTA_0_DEFAULT) || $time_sent > $time_received + variable_get('drupaltodrupal_delta_1', DRUPALTODRUPAL_DELTA_1_DEFAULT)) {
    return 21;
  }

  // TODO: might not be an error...

  // is $until_ba in the (not so near) future?
  // TODO: get minimal requirement for validity from configuration
  /*
  if ($until_ba < drupaltodrupal_get_time() + 3600) {
    return 13;
  }
  */

  // check $exploded_data['certificate_ba']
  $certificate_ba_verified = null;
  if (!is_null($public_key_b)) {
    if (openssl_verify(drupaltodrupal_get_friendship_string($entity_b, $entity_a, $until_ba), $certificate_ba, $public_key_b) !== 1) {
      $certificate_ba_verified = false;
    }
    else {
      $certificate_ba_verified = true;
    }
  }
  if ($certificate_ba_verified === FALSE) {
    // if auto accept requests, an incoming request which can not be verified means
    // that is was signed with a public key that is not in the database, hence
    // set public key to NULL.
    if (variable_get('drupaltodrupal_auto_accept', DRUPALTODRUPAL_AUTO_ACCEPT)) {
      $public_key_b = NULL;
    }
  }

  // TODO
  // check certificate for friendship between A and C (if provided)
  //  -> this should be done without further communication with C

  // determine which answer to give
  // possible cases:
  // autorenew friendship (if was friend before)
  //  for (automatically) renewing friendship the latest certificate_ab should
  //  be provided. depending on the settings friendship could be renewed even
  //  when a friend has been forgotten

/*
  $result = db_select('drupaltodrupal_instances', 'i')
    ->fields('i', array('id', 'public_key'))
    ->condition('url', $entity_b, '=')
    ->execute();
*/
  $result = db_query('SELECT id, url FROM {drupaltodrupal_instances} WHERE url=:url', array(':url' => $entity_b));
  if ($row = $result->fetchAssoc()) {
    $id_b = $row['id'];
    if ($row['url'] !== $entity_b) {
      return 19;
    }
  }
  else {
    $id_b = db_insert('drupaltodrupal_instances')->fields(array(
        'url' => $entity_b,
        'description' => 'N/A',
        'time_inserted' => drupaltodrupal_get_time(),
        'public_key_id' => null,
      ))->execute();
    drupaltodrupal_set_default_group_memberships($id_b);
    // TODO: should not be needed.
    if ($public_key_b !== null) {
      $id_pk = db_insert('drupaltodrupal_public_keys')->fields(array(
        'id_instance' => $id_b,
        'public_key' => $public_key_b,
      ))->execute();
      $num_updated = db_update('drupaltodrupal_instances')
        ->fields(array(
        'public_key_id' => $id_pk,
      ))
      ->condition('id', $id_b)
      ->execute();
    }
  }
  $id_cb = null;
  $id_c = null;
  if ($until_cb >= $time_received && $certificate_cb && drupaltodrupal_check_url($entity_c)) {
    $public_key_c = null;
    $friends = drupaltodrupal_get_friends();
    foreach ($friends as $friend) {
      if ($friend['url'] === $entity_c) {
        $entity_c = $friend['url'];
        $id_c = $friend['id'];
        $public_key_c = $friend['public_key'];
        break;
      }
    }
    if ($public_key_c) {
      if (openssl_verify(drupaltodrupal_get_friendship_string($entity_c, $entity_b, $until_cb), $certificate_cb, $public_key_c) === 1) {
        $result = db_query('SELECT fcert_id FROM {drupaltodrupal_friendship_certificates} WHERE id_signer=:id_signer AND id_friend=:id_friend AND valid=:valid AND valid_until=:valid_until AND certificate=:certificate', array(':id_signer' => $id_c, ':id_friend' => $id_b, ':valid' => 1, ':valid_until' => $until_cb, ':certificate' => $certificate_cb));
        if ($record = $result->fetchAssoc()) {
          $id_cb = $record['fcert_id'];
        }
        else {
          $id_cb = db_insert('drupaltodrupal_friendship_certificates')->fields(array(
            'id_signer' => $id_c,
            'id_friend' => $id_b,
            'time_inserted' => drupaltodrupal_get_time(),
            'valid' => 1,
            'valid_until' => $until_cb,
            'certificate' => $certificate_cb,
          ))->execute();
        }
      }
      else {
        $id_c = null;
      }
    }
    else {
      $id_c = null;
    }
  }
  $result = db_query('SELECT fcert_id FROM {drupaltodrupal_friendship_certificates} WHERE id_signer=:id_signer AND id_friend=:id_friend AND valid=:valid AND valid_until=:valid_until AND certificate=:certificate', array(':id_signer' => $id_b, ':id_friend' => $id_a, ':valid' => $certificate_ba_verified, ':valid_until' => $until_ba, ':certificate' => $certificate_ba));
  if ($record = $result->fetchAssoc()) {
    $id_ba = $record['fcert_id'];
  }
  else {
    $id_ba = db_insert('drupaltodrupal_friendship_certificates')->fields(array(
      'id_signer' => $id_b,
      'id_friend' => $id_a,
      'time_inserted' => drupaltodrupal_get_time(),
      'valid' => drupaltodrupal_bool_to_int($certificate_ba_verified),
      'valid_until' => $until_ba,
      'certificate' => $certificate_ba,
    ))->execute();
  }
  $incoming_id = db_insert('drupaltodrupal_incoming_requests')->fields(array(
    'id_offer' => $id_ba,
    'id_common_friend' => $id_c,
    'email' => $email,
    'message' => $message,
    'time_sent' => $time_sent,
    'time_received' => $time_received,
    'ip' => $_SERVER['REMOTE_ADDR'],
    'request' => $encrypted_data,
    'signature' => $signature,
    'valid' => drupaltodrupal_bool_to_int($signature_verified),
    'state' => ($type_id != 0 && $signature_verified && $certificate_ba_verified ? 1 : 0),
    'type' => $type_id,
  ))->execute();
  if ($type_id != 0) { // accept or renew
    if ($signature_verified && $certificate_ba_verified) {
      return 0;
    }
    else {
      return 16;
    }
  }
  if (variable_get('drupaltodrupal_auto_accept', DRUPALTODRUPAL_AUTO_ACCEPT)) {
    $url = $entity_b;
    if (!$public_key_b) {
      $public_key_b = xmlrpc($url. '/xmlrpc.php', array('drupalToDrupal.getPublicKey' => array()));
      if ($public_key_b === FALSE) {
        return 0;
      }
    }
    $result = db_query('SELECT id as id FROM {drupaltodrupal_public_keys} WHERE id_instance=:id AND public_key=:public_key', array(':id' => $id_b, ':public_key' => $public_key_b));
    if ($record = $result->fetchObject()) {
      $id_pk = $record->id;
    }
    else {
      $id_pk = db_insert('drupaltodrupal_public_keys')->fields(array(
        'id_instance' => $id_b,
        'public_key' => $public_key_b,
      ))->execute();
    }
    drupaltodrupal_invalidate($id_b);
    $num_updated = db_update('drupaltodrupal_instances')
      ->fields(array('public_key_id' => $id_pk,))
      ->condition('id', $id_b)
      ->execute();
    drupaltodrupal_validate($id_b);
    drupaltodrupal_accept_request_by_id($incoming_id);
  }
  return 0;
}

