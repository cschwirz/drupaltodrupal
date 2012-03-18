<?php

/**
 * @file
 * Functions directly related to the manage groups tab.
 */
function drupaltodrupal_form_list_groups() {
  $rows = array();
  $result = db_query('SELECT id, name, description, auto_member FROM {drupaltodrupal_groups}');
  foreach ($result as $record) {
    $row = array(
      l($record->id, 'admin/drupaltodrupal/g_and_p/groups/edit/' . $record->id),
      $record->name,
      $record->description,
      drupaltodrupal_bool_to_str($record->auto_member),
    );
    $rows['id' . $record->id] = $row;
  }
  $form['groups_table'] = array
  (
    '#type' => 'tableselect',
    '#header' => array('ID', 'Name', 'Description', 'Default'),
    '#options' => $rows,
    '#empty' => t('No groups in database.'),
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => 'Delete',
  );
  return $form;
}
function _drupaltodrupal_get_ids(&$form_state, $table, $prefix_length) {
  $ids = array();
  foreach ($form_state['values'][$table] as $key => $value) {
    if ($value && strlen($value) > $prefix_length) {
      $ids[] = substr($key, $prefix_length);
    }
  }
  return $ids;
}
function drupaltodrupal_form_list_groups_validate($form, &$form_state) {
  $ids = _drupaltodrupal_get_ids($form_state, 'groups_table', 2);
  if (empty($ids)) {
    form_set_error('groups_table', t('No group selected for deletion.'));
  }
}
function drupaltodrupal_form_list_groups_submit($form, &$form_state) {
  $ids = _drupaltodrupal_get_ids($form_state, 'groups_table', 2);
  db_delete('drupaltodrupal_groups')
    ->condition('id', $ids)
    ->execute();
  db_delete('drupaltodrupal_permissions')
    ->condition('group_id', $ids)
    ->execute();
  db_delete('drupaltodrupal_group_memberships')
    ->condition('group_id', $ids)
    ->execute();
}


function theme_drupaltodrupal_list_permissions($variables) {
  $form = $variables['form'];
  foreach (element_children($form['permission']) as $key) {
    $row = array();
    $row[] = array(
      'data' => drupal_render($form['permission'][$key]),
      'class' => array('permission'),
    );
    if (array_key_exists('checkboxes', $form)) {
      foreach (element_children($form['checkboxes']) as $gid) {
        $row[] = array('data' => drupal_render($form['checkboxes'][$gid][$key]), 'class' => array('checkbox'));
      }
    }
    $rows[] = $row;
  }
  $header[] = (t('Method'));
  if (array_key_exists('group', $form)) {
    foreach (element_children($form['group']) as $gid) {
      $header[] = array('data' => drupal_render($form['group'][$gid]), 'class' => array('checkbox'));
    }
  }
  $output = theme('table', array('header' => $header, 'rows' => $rows));
  $output .= drupal_render_children($form);
  return $output;
}

function drupaltodrupal_list_permissions($form, &$form_state) {
  $result = db_query('SELECT id, name, description FROM {drupaltodrupal_groups}');
  $groups = array();
  foreach ($result as $record) {
    $groups[$record->id] = $record->name;
  }
  $form['groups'] = array(
    '#type' => 'value',
    '#value' => $groups,
  );
  $result = db_query('SELECT group_id, method FROM {drupaltodrupal_permissions}');
  $permissions = array();
  foreach ($result as $record) {
    $permissions[$record->group_id][$record->method] = 1;
  }

  $hks = module_invoke_all('drupaltodrupal_secure_rpc');
  $options = array();
  $status = array();
  foreach ($hks as $key => $value) {
    if (strlen($key) > DRUPALTODRUPAL_GROUP_NAME_MAX_LENGTH) {
      drupal_set_message(t('Ignoring hook for method @method (correspongind key may be no longer than @length characters).', array('@method' => $key, '@length' => DRUPALTODRUPAL_GROUP_NAME_MAX_LENGTH)), 'warning');
      continue;
    }
    $options[$key] = '';
    $form['permission'][$key] = array(
      '#type' => 'item',
      '#markup' => $key,
      '#description' => isset($value['description']) ? $value['description'] : '',
    );
    foreach ($groups as $group_id => $group_name) {
      if (isset($permissions[$group_id][$key])) {
        $status[$group_id][] = $key;
      }
    }
  }
  foreach ($groups as $group_id => $group_name) {
    $form['checkboxes'][$group_id] = array(
      '#type' => 'checkboxes',
      '#options' => $options,
      '#default_value' => isset($status[$group_id]) ? $status[$group_id] : array(),
    );
    $form['group'][$group_id] = array('#markup' => $group_name, '#tree' => TRUE);
  }
  $form['actions'] = array('#type' => 'actions');
  $form['actions']['submit'] = array('#type' => 'submit', '#value' => t('Save permissions'));
  return $form;
} 
function drupaltodrupal_list_permissions_submit($form, &$form_state) {
  $methods = array();
  $num_updated = db_delete('drupaltodrupal_permissions')->execute();
  foreach ($form_state['values']['groups'] as $gid => $name) {
    $has_permission = array();
    foreach ($form_state['values'][$gid] as $key => $value) {
      if ($value !== 0) {
        $has_permission[] = $key;
        $methods[$key] = 1;
      }
    }
    /*
    if (!empty($has_permission))
    {
      $num_updated = db_delete('drupaltodrupal_permissions')
        ->condition('group_id', $gid)
        ->condition('method', $has_permission, 'NOT IN')
        ->execute();
    }
    else {
      $num_updated = db_delete('drupaltodrupal_permissions')
        ->condition('group_id', $gid)
        ->execute();
    }
    */
    if (!empty($has_permission)) {
      /*
      db_delete('drupaltodrupal_permissions')
        ->condition('method', $has_permission)
        ->execute();
      */
      $query = db_insert('drupaltodrupal_permissions')->fields(array('group_id', 'method'));
      foreach ($has_permission as $method) {
        $query->values(array('group_id' => $gid, 'method' => $method));
      }
      $query->execute();
    }
  }
  // remove all groups that have don't appear at all
  if (empty($methods)) {
    $num_updated = db_delete('drupaltodrupal_permissions')
      ->execute();
  }
  else {
    $num_updated = db_delete('drupaltodrupal_permissions')
      ->condition('method', array_keys($methods), 'NOT IN')
      ->execute();
  }
  drupal_set_message(t('The changes have been saved.'));
}
function drupaltodrupal_form_add_group() {
  $form = array();
  $form['name'] = array(
    '#type' => 'textfield',
    '#title' => t('Name'),
    '#description' => t('Name of the group'),
    '#default_value' => '',
    '#size' => 40,
    '#maxlength' => DRUPALTODRUPAL_GROUP_NAME_MAX_LENGTH,
    '#required' => TRUE,
  );
  $form['default'] = array(
    '#type' => 'checkbox',
    '#default_value' => FALSE,
    '#title' => t('Automatically add new instances to this group.'),
    '#description' => t('This has no effect on existing instances.'),
  );
  $form['description'] = array(
    '#type' => 'textarea',
    '#title' => t('Description of the group'),
    '#description' => t('An optional desciption of the newly added group, no longer than 1024 characters.'),
    '#rows' => 4,
    '#cols' => 60,
    '#default_value' => 'N/A',
    '#required' => FALSE,
  );
  $form['submit'] = array(
    '#type' => 'submit', 
    '#value' => 'Add group',
  );
  return $form;
}
function drupaltodrupal_form_add_group_validate($form, &$form_state) {
  if (db_query('SELECT * FROM {drupaltodrupal_groups} WHERE name=:name', array(':name' => $form_state['values']['name']))->fetchAssoc()) {
    form_set_error('name', t('Provided name already exists in database'));
  }
  if (strlen($form_state['values']['description']) > DRUPALTODRUPAL_GROUP_DESCRIPTION_MAX_LENGTH) {
    form_set_error('description', t('Provided description must be no longer than @max_length characters'), array('@max_length' => DRUPALTODRUPAL_GROUP_DESCRIPTION_MAX_LENGTH));
  }
}
function drupaltodrupal_form_add_group_submit($form, &$form_state) {
  $name = $form_state['values']['name'];
  $description = $form_state['values']['description'];
  try {
    $id = db_insert('drupaltodrupal_groups')->fields(array(
        'name' => $name,
        'description' => $description,
        'auto_member' => $form_state['values']['default'] ? 1 : 0,
      ))->execute();
  }
  catch (Exception $e) {
    drupal_set_message(t('A group with that name already exists in the databaset.'), 'error');
  }
}


function drupaltodrupal_manage_groups() {
  $build = array();
  $build['groups'] = array(
    '#title' => t('Groups in the database'),
    '#type' => 'fieldset',
  );
  $build['groups'][] = drupal_get_form('drupaltodrupal_form_list_groups');

  $build['add_group'] = array(
    '#title' => t('Add group'),
    '#type' => 'fieldset',
  );
  $form_state = array();
  $build['add_group'][] = drupal_get_form('drupaltodrupal_form_add_group');

  return $build;
}
function drupaltodrupal_manage_permissions() {
  $build = array();
  $build['group_rights'] = array(
    '#title' => t('Permissions'),
    '#type' => 'fieldset',
  );
  $build['group_rights'][] = drupal_get_form('drupaltodrupal_list_permissions');
  return $build;
}
