<?php

/**
 * @file
 * Functions directly related to the edit group tab.
 */
function drupaltodrupal_form_edit_group($form, &$form_state, $id, $name, $description, $default) {
  $form = array();
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Name of the group, no longer than @max_length characters.', array('@max_length' => DRUPALTODRUPAL_GROUP_NAME_MAX_LENGTH)),
    '#default_value' => $name,
    '#size' => 40,
    '#maxlength' => DRUPALTODRUPAL_GROUP_NAME_MAX_LENGTH,
    '#required' => TRUE,
  );
  $form['default'] = array(
    '#type' => 'checkbox',
    '#default_value' => $default,
    '#title' => t('Automatically add new instances to this group.'),
    '#description' => t('This has no effect on existing instances.'),
  );
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description of the instance'),
    '#description' => t('An optional desciption of the group, no longer than @max_length characters.', array('@max_length' => DRUPALTODRUPAL_GROUP_DESCRIPTION_MAX_LENGTH)),
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
    '#value' => 'Save group',
  );
  return $form;
}
function drupaltodrupal_form_edit_group_validate($form, &$form_state) {
  $id = $form_state['values']['id'];
  $name = $form_state['values']['name'];
  $description = $form_state['values']['description'];
  $result = db_query('SELECT * FROM {drupaltodrupal_groups} WHERE name=:name AND id<>:id', array(':name' => $name, ':id' => $id));
  if ($result->fetchAssoc()) {
    form_set_error('name', t('Group name already exists in database.'));
  }
  if (strlen($form_state['values']['description']) > DRUPALTODRUPAL_GROUP_DESCRIPTION_MAX_LENGTH) {
    form_set_error('description', t('Provided description must be no longer than @max_length characters.', array('@max_length' => DRUPALTODRUPAL_GROUP_DESCRIPTION_MAX_LENGTH)));
  }
} 
function drupaltodrupal_form_edit_group_submit($form, &$form_state) {
  $id = $form_state['values']['id'];
  $name = $form_state['values']['name'];
  $description = $form_state['values']['description'];
  $num_updated = db_update('drupaltodrupal_groups')
    ->fields(array(
      'name' => $name,
      'description' => $description,
      'auto_member' => $form_state['values']['default'] ? 1 : 0,
    ))
    ->condition('id', $id)
    ->execute();
  if ($num_updated > 0) {
    drupal_set_message(t('The changes have been saved.'));
  }
} 
function drupaltodrupal_edit_group($id) {
  $build = array();
  $build['main'] = array(
    '#title' => 'Group #' . $id,
    '#type' => 'fieldset',
  );
  $form_state = array();
  $result = db_query('SELECT name, description, auto_member FROM {drupaltodrupal_groups} WHERE id=:id', array(':id' => $id));
  if (!($record = $result->fetchAssoc())) {
    drupal_set_message(t('Group not found in database.'), 'error');
    return $build;
  }
  $form_state['build_info']['args'] = array($id, $record['name'], $record['description'], $record['auto_member']);
  $build['main']['details'] = drupal_build_form('drupaltodrupal_form_edit_group', $form_state);
  return $build;
}
