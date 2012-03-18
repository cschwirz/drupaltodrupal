<?php

/**
 * @file
 * Functions directly related to the list friends of friends tab.
 */
function drupaltodrupal_form_add_friends_of_friends($form, &$form_state, $friends_of_friends, $db_instances) {
  $rows = array();
  $disabled = array();
  foreach ($friends_of_friends as $instance) {
    $link = '';
    if (array_key_exists($instance['url'], $db_instances)) {
      $id = $db_instances[$instance['url']];
      $link = l(t('edit'), 'admin/drupaltodrupal/instances/' . $id . '/details');
      $disabled[] = $instance['url'];
    }
    $rows[$instance['url']] = array(check_plain($instance['url']), $instance['count'], $link);
  }
  $form['instances'] = array
  (
    '#type' => 'tableselect',
    '#header' => array(t('Recommended instance'), t('No. of common friends'), ''),
    '#options' => $rows,
    '#empty' => t('All friends of your friends are already your friends.'),
  );
  foreach ($disabled as $instance) {
    $form['instances'][$instance]['#disabled'] = TRUE;
  }
  if (sizeof($disabled) < sizeof($rows)) {
    $form['submit'] = array(
      '#type' => 'submit', 
      '#value' => 'Insert into database',
    );
  }
  return $form;
}
function drupaltodrupal_form_add_friends_of_friends_submit($form, &$form_state) {
  $urls = _drupaltodrupal_get_ids($form_state, 'instances', 0);
  $n_inserted = 0;
  foreach ($urls as $url) {
    if (drupaltodrupal_check_url($url)) {
      try {
        $id = db_insert('drupaltodrupal_instances')->fields(array(
            'url' => $url,
            'description' => 'N/A',
            'time_inserted' => drupaltodrupal_get_time(),
            'public_key_id' => null,
          ))->execute();
        $n_inserted++;
        drupaltodrupal_set_default_group_memberships($id);
      }
      catch (Exception $e) {
      }
    }
  }
  drupal_set_message(t('Inserted @count instance(s) into database.', array('@count' => $n_inserted)));
}
// sort descending by second (numeric) entry, lexicographically in case of ties
function _drupaltodrupal_compare_second_desc($a, $b) {
  return $b['count'] == $a['count'] ? strcmp($a['url'], $b['url']) : $b['count'] - $a['count'];
}
function drupaltodrupal_list_friends_of_friends() {
  drupaltodrupal_get_self_instance($id, $entity, $public_key);
  $db_instances = array();
  $result = db_select('drupaltodrupal_instances', 'i')
    ->fields('i', array('id', 'url'))
    ->execute();
  foreach ($result as $record) {
    if ($id != $record->id) {
      $db_instances[$record->url] = $record->id;
    }
  }
  $friends = drupaltodrupal_get_friends();
  $build = array();
  $friends_of_friends = array();
  $rows_fof = array();
  foreach ($friends as $friend) {
    $id_a = $friend['id'];
    $entity_a = $friend['url'];
    $rows = array();
    $error_string = '';
    $res = drupaltodrupal_call_secure_rpc($friend, 'drupaltodrupal_get_friends', array(), $error_string);
    $description = t('no friends');
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
          $link = '';
          if (array_key_exists($record, $db_instances)) {
            $link = l(t('edit / request friendship with referral'), 'admin/drupaltodrupal/instances/' . $db_instances[$record] . '_' . $id_a . '/friendship');
          }
          $rows[] = array(check_plain($record), array('data' => $link, 'align' => 'right'));
          if (array_key_exists($record, $friends_of_friends)) {
            ++$friends_of_friends[$record];
          }
          else {
            $friends_of_friends[$record] = 1;
          }
        }
      }
    }
    $rows_fof[] = array(
      array(
        'data' => check_plain($entity_a),
        'style' => 'font-weight: bold',
      ),
      '',
    );
    if (empty($rows)) {
      $rows_fof[] = array('&nbsp;&nbsp;&nbsp;' . check_plain($description), '');
    }
    else {
      foreach ($rows as $row) {
        $row[0] = '&nbsp;&nbsp;&nbsp;' . $row[0];
        $rows_fof[] = $row;
      }
    }
  }
  $build['friends_of_friends'] = array(
    '#title' => t('Friends of friends'),
    '#type' => 'fieldset',
  );
  $build['friends_of_friends'][] = array(
    '#theme' => 'table',
    '#header' => array(t('URL'), ''),
    '#rows' => $rows_fof,
    '#empty' => t('No friends found in the database'),
  );
  foreach ($friends as $friend) {
    $friends_of_friends[$friend['url']] = 0;
  }
  drupaltodrupal_get_self_instance($id, $entity, $public_key);
  $friends_of_friends[$entity] = 0;
  $filtered_friends_of_friends = array();
  foreach ($friends_of_friends as $key => $value) {
    if ($value > 0) {
      $filtered_friends_of_friends[$key] = array('url' => $key, 'count' => $value);
    }
  }
  usort($filtered_friends_of_friends, '_drupaltodrupal_compare_second_desc');
    $build['recommondations'] = array(
      '#title' => t('Recommondations'),
      '#type' => 'fieldset',
    );
  $form_state = array();
  $form_state['build_info']['args'] = array($filtered_friends_of_friends, $db_instances);
  $build['recommondations'][] = drupal_build_form('drupaltodrupal_form_add_friends_of_friends', $form_state);
  /*
  $build['recommondations'][] = array(
    '#theme' => 'table',
    '#header' => array(t('Recommended instance'), t('No. of common friends')),
    '#rows' => $filtered_friends_of_friends,
    '#empty' => t('All friends of your friends are already your friend.'),
  );
  */
  return $build;
} 
