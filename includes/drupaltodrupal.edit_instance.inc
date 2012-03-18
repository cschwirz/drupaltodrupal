<?php

/**
 * @file
 * Functions directly related to the edit instance tab.
 */
function drupaltodrupal_form_delete_public_key($form, &$form_state, $id) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Remove public key'),
  );
  return $form;
}
function drupaltodrupal_form_delete_public_key_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $num_updated = db_update('drupaltodrupal_instances')
    ->fields(array(
      'public_key_id' => null,
    ))
    ->condition('id', $id)
    ->execute();
  if ($num_updated > 0) {
    drupal_set_message(t('Public key has been removed.'));
  }
  else {
    drupal_set_message(t('Error removing public key'), 'error');
  }
  drupaltodrupal_invalidate($id);
}
function drupaltodrupal_form_receive_public_key($form, &$form_state, $id) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Receive public key'),
  );
  return $form;
}
function drupaltodrupal_form_receive_public_key_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $result = db_query('SELECT i.id as id, i.url as url FROM {drupaltodrupal_instances} i WHERE i.id=:id', array(':id' => $id));
  if ($record = $result->fetchObject()) {
    $url = $record->url;
    $res = xmlrpc($url. '/xmlrpc.php', array('drupalToDrupal.getPublicKey' => array()));
    if ($res === FALSE) {
      drupal_set_message(t('Error retrieving public key'), 'error');
    }
    else {
      drupal_set_message(t('Public key has been received.'));
      $result = db_query('SELECT id as id FROM {drupaltodrupal_public_keys} WHERE id_instance=:id AND public_key=:public_key', array(':id' => $id, ':public_key' => $res));
      if ($record = $result->fetchObject()) {
        drupal_set_message(t('Public key #@id is already available in database.', array('@id' => $record->id)), 'error');
      }
      else {
        $id_pk = db_insert('drupaltodrupal_public_keys')->fields(array(
          'id_instance' => $id,
          'public_key' => $res,
        ))->execute();
        drupal_set_message(t('Public key has been inserted into database. Click on \'Choose this public key\' to associate the newly inserted public key with this instance.'));
      }
    }
  }
  else {
    drupal_set_message(t('Instance not found in database'), 'error');
  }
}
function drupaltodrupal_form_edit_description($form, &$form_state, $id, $description) {
  $form = array();
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description of the instance'),
    '#description' => t('An optional desciption of this instance, no longer than @max_length characters.', array('@max_length' => DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH)),
    '#rows' => 4,
    '#cols' => 60,
    '#default_value' => $description,
    '#required' => FALSE,
  );
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => 'Save description',
  );
  return $form;
}
function drupaltodrupal_form_edit_description_validate($form, &$form_state) {
  if (strlen($form_state['values']['description']) > DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH) {
    form_set_error('description', t('Provided description must be no longer than @max_length characters', array('@max_length' => DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH)));
  }
}
function drupaltodrupal_form_edit_description_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $description = $form_state['values']['description'];
  $num_updated = db_update('drupaltodrupal_instances')
    ->fields(array(
      'description' => $description,
    ))
    ->condition('id', $id)
    ->execute();
}
function drupaltodrupal_form_delete_or_choose_candidate_public_key($form, &$form_state, $id, $public_key_id) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['public_key_id'] = array(
    '#type' => 'hidden',
    '#value' => $public_key_id,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => 'Remove public key from database',
    '#submit' => array('drupaltodrupal_form_delete_candidate_public_key_submit'),
  );
  $form['submit2'] = array(
    '#type' => 'submit', 
    '#value' => 'Choose this public key',
    '#submit' => array('drupaltodrupal_form_choose_candidate_public_key_submit'),
  );
  return $form;
}
function drupaltodrupal_form_delete_candidate_public_key_submit($form, &$form_state) {
  $public_key_id = $form_state['values']['public_key_id'];
  $num_updated = db_delete('drupaltodrupal_public_keys')
    ->condition('id', $public_key_id)
    ->execute();
  if ($num_updated > 0) {
    drupal_set_message(t('Removed public key from the database'));
  }
  else {
    drupal_set_message(t('Error removing public key from the database'), 'error');
  }
}
function drupaltodrupal_form_choose_candidate_public_key_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $public_key_id = $form_state['values']['public_key_id'];
  drupaltodrupal_invalidate($id);
  $num_updated = db_update('drupaltodrupal_instances')
    ->fields(array(
      'public_key_id' => $public_key_id,
    ))
    ->condition('id', $id)
    ->execute();
  if ($num_updated > 0) {
    drupal_set_message(t('Chosen new publiy key.'));
  }
  else {
    drupal_set_message(t('Error choosing public key'), 'error');
  }
  drupaltodrupal_validate($id);
}
function drupaltodrupal_form_groups_by_instance($form, &$form_state, $id) {
  $rows = array();
  $default_value = array();
  $result = db_query('SELECT g.id as id, g.name as name, g.description as description, gm.group_id as flag FROM {drupaltodrupal_groups} g LEFT OUTER JOIN {drupaltodrupal_group_memberships} gm ON g.id=gm.group_id AND gm.instance_id=:id', array(':id' => $id));
  foreach ($result as $record) {
    $group = array(
      '#type' => 'item',
      '#markup' => $record->name,
      '#description' => $record->description,
    );

    $row = array($record->id, drupal_render($group)); //, $record->description, $record->flag ? 'yes' : 'no');
    $rows['id' . $record->id] = $row;
    $default_value['id' . $record->id] = $record->flag ? 'id' . $record->id : 0;
  }
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['groups_table'] = array
  (
    '#type' => 'tableselect',
    '#header' => array('ID', 'Name'),
    '#options' => $rows,
    '#empty' => t('No groups in database.'),
    '#default_value' => $default_value,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => 'Save changes',
  );
  return $form;
}
function drupaltodrupal_form_groups_by_instance_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $ids = _drupaltodrupal_get_ids($form_state, 'groups_table', 2);
  db_delete('drupaltodrupal_group_memberships')
    ->condition('instance_id', $id)->execute();
  if (!empty($ids)) {
    $query = db_insert('drupaltodrupal_group_memberships')->fields(array('group_id', 'instance_id'));
    foreach ($ids as $gid) {
      $query->values(array('group_id' => $gid, 'instance_id' => $id));
    }
    $query->execute();
  }
  drupal_set_message(t('The changes have been saved.'));
}

function drupaltodrupal_form_remove_friendship($form, &$form_state, $id) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Remove all (even outdated) friendship certificates for this instance, outgoing requests'), 
  );
  return $form;
}
function drupaltodrupal_form_remove_friendship_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  drupaltodrupal_get_self_instance($my_id, $my_url, $my_public_key);
  $result = db_query('SELECT fcert_id FROM {drupaltodrupal_friendship_certificates} WHERE id_signer=:id_signer AND id_friend=:id_friend', array(':id_signer' => $my_id, ':id_friend' => $id));
  $records = $result->fetchAll();
  $fcert_ids = array();
  foreach ($records as $record) {
    $fcert_ids[] = $record->fcert_id;
  }
  if (empty($fcert_ids)) {
    drupal_set_message(t('No associated certificates / outgoing requests in the database.'));
  }
  else {
    $num_updated_outgoing = db_delete('drupaltodrupal_outgoing_requests')
      ->condition('id_offer', $fcert_ids)
      ->execute();
    $num_updated_certificates = db_delete('drupaltodrupal_friendship_certificates')
      ->condition('fcert_id', $fcert_ids)
      ->execute();
    drupal_set_message(t('Removed @a certificates and @b outgoing requests.', array('@a' => $num_updated_certificates, '@b' => $num_updated_outgoing)));
  } 
}


function drupaltodrupal_form_add_friendship_request($form, &$form_state, $id, $id_common_friend) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  if ($id_common_friend !== NULL) {
    $result = db_query('SELECT url FROM {drupaltodrupal_instances} WHERE id=:id', array(':id' => $id_common_friend));
    if ($record = $result->fetchAssoc()) {
      $form['referral'] = array(
        '#type' => 'checkbox',
        '#default_value' => TRUE,
        '#title' => t('Use instance with id @id (@url) as referral', array('@id' => $id_common_friend, '@url' => $record['url'])),
        '#description' => t('Proof of common friend will be sent with this request.'),
      );
      $form['id_common_friend'] = array(
        '#type' => 'hidden',
        '#value' => $id_common_friend,
      );
    }
  }
  $form['message'] = array(
    '#type' => 'textarea',
    '#title' => t('Message'),
    '#rows' => 4,
    '#cols' => 60,
    '#default_value' => DRUPALTODRUPAL_REQUEST_MESSAGE_DEFAULT,
    '#required' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Request friendship'),
  );
  return $form;
}

function drupaltodrupal_form_add_friendship_request_validate($form, &$form_state) {
  if (strlen($form_state['values']['message']) > DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH) {
    form_set_error('message', t('Provided message must be no longer than @max_length characters', array('@max_length' => DRUPALTODRUPAL_REQUEST_MESSAGE_MAX_LENGTH)));
  }
}

function drupaltodrupal_form_add_friendship_request_submit($form, &$form_state) {
  drupaltodrupal_get_self_instance($id_b, $entity_b, $public_key_b);
  $id_common_friend = null;
  if (isset($form_state['values']['referral']) && $form_state['values']['referral']) {
    $id_common_friend = $form_state['values']['id_common_friend'];
    $result = db_query('SELECT fcert_id, valid_until FROM {drupaltodrupal_friendship_certificates} WHERE valid=1 AND id_signer=:id_signer AND id_friend=:id_friend AND valid_until>=:t ORDER BY valid_until DESC', array(':id_signer' => $id_common_friend, ':id_friend' => $id_b, ':t' => drupaltodrupal_get_time()));
    if ($record = $result->fetchAssoc()) {
    }
    else {
      $id_common_friend = null;
      drupal_set_message('No valid friendship found for referral.', 'error');
      return;
    }
  }
  $id_a = $form_state['values']['id'];
//  $entity_a = $form_state['values']['receiver']; // receiver of the friendship request

  // {{{ Insert instance into database.
  $result = db_query('SELECT i.url as url, p.public_key as public_key FROM {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id WHERE i.id=:id', array(':id' => $id_a));
  $public_key_a = null;
  if (!$row = $result->fetchAssoc()) {
    drupal_set_message(t('Instance not found.'), 'error');
    return;
  }
  $entity_a = $row['url'];
  $public_key_a = $row['public_key'];
  // }}}
  
  // {{{ Insert certificate for friendship into database.
  // Time until when the new friendship should be valid.
  $until_ba = drupaltodrupal_get_time() + variable_get('drupaltodrupal_validity', DRUPALTODRUPAL_VALIDITY_DEFAULT) * 60;
  $friendship_ba = drupaltodrupal_get_friendship_string($entity_b, $entity_a, $until_ba);
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
    'id_common_friend' => $id_common_friend,
    'email' => variable_get('site_mail', 'N/A'),
    'message' => $form_state['values']['message'],
    'time_inserted' => $insert_time,
    'state' => 0,
    'reply' => null,
    'next_send' => $insert_time,
    'type' => 0, // request
  ))->execute();
  $msg = t('Inserted outgoing request into database.');
  if (!$public_key_a) {
    $msg = $msg . ' ' . t('A public key has to be associated with this instance for the request to be sent.');
  }
  drupal_set_message($msg);
  // process outgoing requests...
  return;
}
function drupaltodrupal_form_check_public_keys($form, &$form_state, $id, $url) {
  $form = array();
  $form['id'] = array(
    '#type' => 'hidden',
    '#value' => $id,
  );
  $form['url'] = array(
    '#type' => 'hidden',
    '#value' => $url,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Check friends\' public keys'),
  );
  return $form;
}
function drupaltodrupal_form_check_public_keys_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $url = $form_state['values']['url'];
  $public_keys = array();
  $result = db_query('SELECT id as public_key_id, public_key FROM {drupaltodrupal_public_keys} WHERE id_instance=:id_instance', array(':id_instance' => $id));
  foreach ($result as $record) {
    $public_keys[$record->public_key] = $record->public_key_id;
  }
  $friends = drupaltodrupal_get_friends();
  $n_checked = 0;
  foreach ($friends as $friend) {
    if ($id == $friend['id']) {
      continue;
    }
    $error_string = '';
    $res = drupaltodrupal_call_secure_rpc($friend, 'drupaltodrupal_get_public_key', array('url' => $url), $error_string);
    if ($res === FALSE) {
      drupal_set_message(t('@url returned @error', array('@url' => $friend['url'], '@error' => $error_string)), 'error');
    }
    else {
      if (!$res) {
        drupal_set_message(t('@url has no public key for this instance in the database.', array('@url' => $friend['url'])));
      }
      elseif (array_key_exists($res, $public_keys)) {
        drupal_set_message(t('@url uses public key #@id for this instance.', array('@url' => $friend['url'], '@id' => $public_keys[$res])));
      }
      else {
        drupal_set_message(t('@url has a public key in the database that is not listed here.', array('@url' => $friend['url'])));
      }
    }
    $n_checked++;
  }
  if ($n_checked == 0) {
    drupal_set_message(t('No friends to compare public key with'));
  }
}
function _drupaltodrupal_edit_instance_build_header($instance) {
  $build = array();
  $build['main'] = array(
    '#title' => t('Instance #') . $instance['id'],
    '#type' => 'fieldset',
  );
  $rows = array();
  $rows[] = array(t('URL'), check_plain($instance['url']));
  $rows[] = array(t('inserted'), drupaltodrupal_get_absolute_time($instance['time_inserted']));
  $rows[] = array(t('public key'), is_null($instance['public_key']) ? 'N/A' : $instance['public_key']);
  $build['main']['overview'] = array(
    '#theme' => 'table',
    '#header' => array(t('Type'), t('Value')),
    '#rows' => $rows,
  );
  return $build;
}
function drupaltodrupal_edit_instance_details($instance) {
//  drupal_set_message(var_export($instance, TRUE));
  $build = _drupaltodrupal_edit_instance_build_header($instance);
  $form_state = array();
  $form_state['build_info']['args'] = array($instance['id'], $instance['description']);
  $build['main']['details']['description'] = drupal_build_form('drupaltodrupal_form_edit_description', $form_state);
  return $build;
}
function drupaltodrupal_edit_instance_public_key($instance) {
//  drupal_set_message(var_export($instance, TRUE));
  $build = _drupaltodrupal_edit_instance_build_header($instance);

  $result = db_query('SELECT id as public_key_id, public_key FROM {drupaltodrupal_public_keys} WHERE id_instance=:id_instance', array(':id_instance' => $instance['id']));
  $build['pk_manage'] = array(
    '#title' => t('Manage public keys'),
    '#type' => 'fieldset',
    /*
    '#collapsible' => TRUE,
    '#attached' => array(
      'js' => array(
        'misc/form.js',
        'misc/collapse.js',
      ),
    ),
    '#attributes' => array(
      'class' => array('collapsible', 'collapsed'),
    ),
    */
  );
  if (!$instance['is_me'] && !is_null($instance['public_key'])) {
    $form_state = array();
    $form_state['build_info']['args'] = array($instance['id']);
    $build['pk_manage'][] = drupal_build_form("drupaltodrupal_form_delete_public_key", $form_state);
  }
  if (!$instance['is_me']) {
    $form_state = array();
    $form_state['build_info']['args'] = array($instance['id']);
    $build['pk_manage'][] = drupal_build_form("drupaltodrupal_form_receive_public_key", $form_state);
  }
  $n_public_keys = 0;
  foreach ($result as $record) {
    $build['pk_manage']['pk' . $record->public_key_id] = array(
      '#title' => 'Public key #' . $record->public_key_id,
      '#type' => 'fieldset',
    );
    $pk_rows = array(array($record->public_key));
    $build['pk_manage']['pk' . $record->public_key_id]['overview'] = array(
      '#theme' => 'table',
      '#header' => array(t('Public key')),
      '#rows' => $pk_rows,
    );
    if ($record->public_key_id != $instance['public_key_id']) {
      $form_state['build_info']['args'] = array($instance['id'], $record->public_key_id);
      $build['pk_manage']['pk' . $record->public_key_id][] = drupal_build_form('drupaltodrupal_form_delete_or_choose_candidate_public_key', $form_state);
    }
    $n_public_keys++;
  }
  if ($n_public_keys > 0) {
    $form_state = array();
    $form_state['build_info']['args'] = array($instance['id'], $instance['url']);
    $build['pk_manage']['pk_check'] = drupal_build_form('drupaltodrupal_form_check_public_keys', $form_state);
  }
  return $build;
}
function drupaltodrupal_edit_instance_friendship($instance) {
//  drupal_set_message(var_export($instance, TRUE));
  $build = _drupaltodrupal_edit_instance_build_header($instance);

  $build['friendship'] = array(
    '#title' => t('Friendship'),
    '#type' => 'fieldset',
  );
  if (!$instance['is_me']) {
    $t = drupaltodrupal_get_time();
    $rows = array(
      array(t('you certified frienship until'), $instance['valid_until_str_a']),
      array(t('@url certified friendship until', array('@url' => $instance['url'])), $instance['valid_until_str_b']),
    );
    if ($instance['is_friend']) {
      $rows[] = array(
        array(
          'data' => t('you are friends until'),
          'style' => 'font-weight: bold',
        ),
        array(
          'data' => drupaltodrupal_get_relative_time(min($instance['valid_until_a'], $instance['valid_until_b'])),
          'style' => 'font-weight: bold',
        ),
      );
    }
    $build['friendship'][] = array(
      '#theme' => 'table',
      '#header' => array(t('Type'), t('Value')),
      '#rows' => $rows,
    );
    $form_state = array();
    $form_state['build_info']['args'] = array($instance['id']);
    $build['friendship'][] = drupal_build_form('drupaltodrupal_form_remove_friendship', $form_state);
    $build['friendship_request'] = array(
      '#title' => t('Friendship Request'),
      '#type' => 'fieldset',
    );
    $form_state = array();
    $form_state['build_info']['args'] = array($instance['id'], $instance['id_common_friend']);
    $build['friendship_request'][] = drupal_build_form('drupaltodrupal_form_add_friendship_request', $form_state);
  }
  return $build;
}
function drupaltodrupal_edit_instance_groups($instance) {
//  drupal_set_message(var_export($instance, TRUE));
  $build = _drupaltodrupal_edit_instance_build_header($instance);
  $build['groups'] = array(
    '#title' => t('Groups'),
    '#type' => 'fieldset',
    '#description' => t('Group memberships for this instance.'),
  );
  $form_state = array();
  $form_state['build_info']['args'] = array($instance['id']);
  $build['groups'][] = drupal_build_form('drupaltodrupal_form_groups_by_instance', $form_state);
  return $build;
}
function drupaltodrupal_edit_instance_permissions($instance) {
//  drupal_set_message(var_export($instance, TRUE));
  $build = _drupaltodrupal_edit_instance_build_header($instance);
  $rows = array();
  if (!$instance['is_me']) {
    $result = db_query('SELECT DISTINCT p.method as method FROM {drupaltodrupal_instances} i, {drupaltodrupal_group_memberships} g, {drupaltodrupal_permissions} p WHERE i.id=:id AND i.id=g.instance_id AND g.group_id=p.group_id', array(':id' => $instance['id']));
    $records = $result->fetchAll();
    $allowed_methods = array();
    foreach ($records as $record) {
      $allowed_methods[$record->method] = 1;
    }
    $hks = module_invoke_all('drupaltodrupal_secure_rpc');
    foreach ($hks as $key => $value) {
      if (array_key_exists($key, $allowed_methods)) {
        $col1 = array(
          '#type' => 'item',  
          '#markup' => $key,
          '#description' => isset($value['description']) ? $value['description'] : '',
        );
        $col2 = '?';
        if (isset($value['arguments'])) {
          $arguments = $value['arguments'];
          if (is_array($arguments)) {
            $col2 = var_export(array_keys($arguments), TRUE);
          }
        }
        $rows[] = array(drupal_render($col1), $col2);
      }
    }
  }
  $build['permissions'] = array(
    '#title' => t('Permissions'),
    '#type' => 'fieldset',
    '#description' => t('These methods can be called by remote by this instance if friendship is valid.'),
  );
  $build['permissions'][] = array(
    '#theme' => 'table',
    '#header' => array(t('Method'), t('Arguments')),
    '#rows' => $rows,
    '#empty' => t('You are not allowed to call any methods on this instance.'),
  );
  return $build;
}
function drupaltodrupal_edit_instance_remote_permissions($instance) {
//  drupal_set_message(var_export($instance, TRUE));
  $build = _drupaltodrupal_edit_instance_build_header($instance);
  $build['remote_permissions'] = array(
    '#title' => t('Remote Permissions'),
    '#type' => 'fieldset',
    '#description' => t('These methods can be called by remote on this instance if friendship is valid.'),
  );
  $rows = array();
  $description = t('no methods may be called');
  if ($instance['is_friend'] && $instance['public_key']) {
    $friend = array('id' => $instance['id'], 'url' => $instance['url'], 'public_key' => $instance['public_key']);
    $error_string = '';
    $res = drupaltodrupal_call_secure_rpc($friend, 'drupaltodrupal_list_permissions', array(), $error_string);
    if ($res === FALSE) {
      $description = $error_string;
    }
    else {
      $exploded_res = drupaltodrupal_explode($res);
      if ($exploded_res === FALSE) {
        $description = t('erroneous return:') . ' ' . $res;
      }
      else {
        foreach ($exploded_res as $record) {
          $exploded_record = drupaltodrupal_explode($record);
          if ($exploded_record) {
            if (drupaltodrupal_check_array($exploded_record, array('method' => 'is_string', 'description' => 'is_string', 'arguments' => 'is_string'))) {
              $col1 = array(
                '#type' => 'item',  
                '#markup' => check_plain($exploded_record['method']),
                '#description' => check_plain($exploded_record['description']),
              );
              $rows[] = array(drupal_render($col1), check_plain($exploded_record['arguments']));
            }
            else {
              // TODO
            }
          }
          else {
            // TODO
          }
        }
      }
    }
  }
  $build['remote_permissions'][] = array(
    '#theme' => 'table',
    '#header' => array(t('Method'), t('Arguments')),
    '#rows' => $rows,
    '#empty' => $description,
  );
  return $build;
}
