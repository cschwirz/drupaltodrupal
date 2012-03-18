<?php
function drupaltodrupal_srpc_list_friends($arguments, $rpc_info) {
  $friends = drupaltodrupal_get_friends();
  $return_array = array();
  foreach ($friends as $friend) {
    $return_array[] = $friend['url'];
  }
  $imploded_return_array = drupaltodrupal_implode($return_array);
  if ($imploded_return_array === FALSE) {
    return '';
  }
  return $imploded_return_array;
}
function drupaltodrupal_srpc_get_public_key($arguments, $rpc_info) {
  $url = $arguments['url'];
  $result = db_query('SELECT p.public_key AS public_key FROM {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id WHERE i.url=:url', array(':url' => $url));
  if ($record = $result->fetchAssoc()) {
    return $record['public_key'] ? $record['public_key'] : '';
  }
  else {
    return '';
  }
}
function drupaltodrupal_srpc_list_permissions($arguments, $rpc_info) {
  $id = $rpc_info['id'];
  $result = db_query('SELECT DISTINCT p.method as method FROM {drupaltodrupal_instances} i, {drupaltodrupal_group_memberships} g, {drupaltodrupal_permissions} p WHERE i.id=:id AND i.id=g.instance_id AND g.group_id=p.group_id', array(':id' => $id));
  $records = $result->fetchAll();
  $allowed_methods = array();
  foreach ($records as $record) {
    $allowed_methods[$record->method] = 1;
  }
  $return_array = array();
  $hks = module_invoke_all('drupaltodrupal_secure_rpc');
  foreach ($hks as $key => $value) {
    if (array_key_exists($key, $allowed_methods)) {
      $args = '?';
      if (isset($value['arguments'])) {
        $arguments = $value['arguments'];
        if (is_array($arguments)) {
          $args = var_export(array_keys($arguments), TRUE);
        }
      }
      $imploded_array = drupaltodrupal_implode(
        array(
          'method' => $key,
          'description' => isset($value['description']) ? $value['description'] : '',
          'arguments' => $args,
        )
      );
      if ($imploded_array) {
        $return_array[] = $imploded_array;
      }
    }
  }
  $imploded_return_array = drupaltodrupal_implode($return_array);
  if ($imploded_return_array === FALSE) {
    return '';
  }
  return $imploded_return_array;
}

/**
 * Implements hook_drupaltodrupal_secure_rpc().
 *
 * Some methods are associated with a default group during the installation.
 */
function drupaltodrupal_drupaltodrupal_secure_rpc() {
  $methods = array();
  $methods['drupaltodrupal_get_friends'] = array(
    'arguments' => array(),
    'callback' => 'drupaltodrupal_srpc_list_friends',
    'description' => 'returns a list of friends',
  );
  $methods['drupaltodrupal_get_public_key'] = array(
    'arguments' => array('url' => 'drupaltodrupal_check_url'),
    'callback' => 'drupaltodrupal_srpc_get_public_key',
    'description' => 'returns the public key of the provided instance',
  );
  $methods['drupaltodrupal_list_permissions'] = array(
    'arguments' => array(),
    'callback' => 'drupaltodrupal_srpc_list_permissions',
    'description' => 'returns the methods the invocing friend may call',
  );
  // TODO: fictious functions
  /*
  $methods['drupaltodrupal_get_local_time'] = array(
    'arguments' => array(),
    'callback' => 'drupaltodrupal_srpc_get_local_time',
    'description' => '(*) returns the local system time',
  );
  $methods['drupaltodrupal_get_statistics'] = array(
    'arguments' => array(),
    'callback' => 'drupaltodrupal_srpc_get_statistics',
    'description' => '(*) provides statistics about this instance',
  );
  $methods['drupaltodrupal_get_news'] = array(
    'arguments' => array(),
    'callback' => 'drupaltodrupal_srpc_get_news',
    'description' => '(*) returns latest news published by this instance',
  );
  */
  return $methods;
}
