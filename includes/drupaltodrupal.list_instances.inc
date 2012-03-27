<?php

/**
 * @file
 * Functions directly related to the list instances tab.
 */


function drupaltodrupal_form_add_instance() {
  $form = array();

  // receiver of the request
  $form['address'] = array(
    '#type' => 'textfield',
    '#title' => t('Address'),
    '#description' => t('Address of the instance'),
    '#default_value' => variable_get('drupaltodrupal_default_request'),
    '#size' => 40,
    '#maxlength' => DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH,
    '#required' => TRUE,
  );
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description of the instance'),
    '#description' => t('An optional desciption of the newly added instance, no longer than @length characters.', array('@length' => DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH)),
    '#rows' => 4,
    '#cols' => 60,
    '#default_value' => t('N/A'),
    '#required' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => t('Add instance'),
  );
  return $form;
}
function drupaltodrupal_form_add_instance_validate($form, &$form_state) {
  if (!drupaltodrupal_check_url($form_state['values']['address'])) {
    form_set_error('address', t('Address must start with \'http://\' or \'https://\' and end with \'/\'.'));
  }
  else {
    if ($record = db_query('SELECT id FROM {drupaltodrupal_instances} WHERE url=:url', array(':url' => $form_state['values']['address']))->fetchAssoc()) {
      form_set_error(
        'address',
        t(
          'Instance with provided address already exists in database, click !link to edit it.',
          array('!link' => l('here', 'admin/drupaltodrupal/instances/' . $record['id'] . '/details'))
        )
      );
    }
  }
  if (strlen($form_state['values']['description']) > DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH) {
    form_set_error('description', t('Provided description must be no longer than @max_length characters'), array('@max_length' => DRUPALTODRUPAL_INSTANCE_DESCRIPTION_MAX_LENGTH));
  }
}
function drupaltodrupal_form_add_instance_submit($form, &$form_state) {
  $id = db_insert('drupaltodrupal_instances')->fields(array(
      'url' => $form_state['values']['address'],
      'description' => $form_state['values']['description'],
      'time_inserted' => drupaltodrupal_get_time(),
      'public_key_id' => null,
    ))->execute();
  drupaltodrupal_set_default_group_memberships($id);
  drupal_set_message(
    t(
      'Instance inserted in database. Click !link to edit it, associate a public key with it and send a friendship request.',
      array('!link' => l('here', 'admin/drupaltodrupal/instances/' . $id . '/details'))
    )
  );
}


function drupaltodrupal_list_instances() {
  drupaltodrupal_get_self_instance($my_id, $my_url, $my_public_key);
  $friends = drupaltodrupal_get_friends();
  $friends_ids = array();
  foreach ($friends as $friend) {
    $friends_ids[$friend['id']] = $friend['url'];
  }
  $result = db_query('SELECT i.id as id, i.url as url, i.description as description, i.time_inserted as time_inserted, i.last_alive as last_alive, p.public_key as public_key FROM {drupaltodrupal_instances} i LEFT OUTER JOIN {drupaltodrupal_public_keys} p ON i.public_key_id = p.id');
  $page = array();
  $rows = array();
  foreach ($result as $record) {
    $rows[] = array(
      l($record->id, 'admin/drupaltodrupal/instances/' . $record->id . '/details'),
      l($record->url, $record->url),
      drupaltodrupal_get_absolute_time($record->time_inserted),
      /*is_null($record->last_alive) ? 'never' : date('F j, Y, g:i a', $record->last_alive),*/
      $record->description,
      is_null($record->public_key) ? 'N/A' : 'available',
      $record->id == $my_id ? '' : (array_key_exists($record->id, $friends_ids) ? t('yes') : t('no')),
    );
  }
  $build = array();
  $build['instances'] = array(
    '#title' => 'Instances in the database',
    '#type' => 'fieldset',
    '#description' => 'For showing and modifying details about an instance click on its ID.',
  );
  $build['instances'][] = array(
    '#theme' => 'table',
    '#header' => array('ID', 'URL', 'Inserted', /*'Last Seen',*/ 'Description', 'Public Key', 'Friend'),
    '#rows' => $rows,
    '#empty' => t('No instance in database.'),
  );
  $build['add_instance'] = array(
    '#title' => 'Add instance',
    '#type' => 'fieldset',
  );
  $build['add_instance'][] = drupal_get_form('drupaltodrupal_form_add_instance');
  return $build;
}
