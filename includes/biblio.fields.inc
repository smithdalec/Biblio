<?php
function _biblio_field_extra_fields() {

  foreach (biblio_types('biblio_contributor') as $type => $info) {
    $extras['biblio_contributor'][$type] = array(
      'display' => array(
        'view_all_by_contributor' => array(
          'label'       => t('"View all biblios by contributor" link'),
          'description' => t('Link on contributor entity view that takes the user to a page of all biblios from which a contributor is referenced.'),
          'weight'      => 10
        ),
      ),
    );
  }

  return $extras;
}

/**
 * Adds Biblio's fields (not field instances) to Drupal's Field API
 * This function runs upon enabling Biblio
 * @todo Add biblio.field.type.data.csv data (field instance labels, etc)
 */
function biblio_add_fields() {
  $file = drupal_get_path('module', 'biblio') . '/misc/default_fields.json';
  $json = json_decode(file_get_contents($file), TRUE);

  foreach ($json as $field) {
    $field_exists = (field_info_field($field['field_name']))? TRUE : FALSE;

    if (!$field_exists) {
      field_create_field($field);
    }
  }
}

/**
 * Add all field instances for a given entity type and bundle.
 * Adding field instances takes a long amount of time for the number of fields
 * and bundles we have. Fields for a biblio publication type are added the first
 * time a user visits the Biblio Add page for that publication type.
 * Contributor instances are added upon install, becasue there
 * are so few fields for those entity types.
 *
 * @param string $entity biblio or biblio_contributor
 * @param string $bundle The bundle for which field instances should be created.
 * for example, a biblio publication type
 */
function biblio_add_field_instances($entity, $bundle) {
  // Get default field instance data
  $file = drupal_get_path(
    'module', 'biblio') . '/misc/default_field_instances.json';
  $default_instance_info = json_decode(file_get_contents($file), TRUE);

  // Get default field instance name data
  $file = drupal_get_path(
    'module', 'biblio') . '/misc/default_field_pubtype_map.json';
  $default_map = json_decode(file_get_contents($file), TRUE);


  // @todo: remove all timers after development
  timer_start('field_instances_creation');

  $default_instances = array();
  foreach ($default_instance_info as $instance_info) {
    // key another array of instance data by field name
    $default_instances[$instance_info['field_name']] = $instance_info;
  }

  foreach ($default_map[$entity][$bundle] as $field => $label) {
    $instance = $default_instances[$field];
    $instance['bundle'] = $bundle;
    // If field label was specified in the default pubtype mapping, override
    // the default label
    if (!empty($label)){
      $instance['label'] = $label;
    }

    // Whether or not a field already exists for this instance. (it should)
    $field_exists = (field_info_field($field))? TRUE : FALSE;
    // Whether or not the field exists already (it shouldn't)
    $instance_exists = (field_info_instance($entity, $field, $bundle)) ?
      TRUE : FALSE;

    if ($field_exists && !$instance_exists) {
      field_create_instance($instance);
    }
  }

  timer_stop('field_instances_creation');
  drupal_set_message('Biblio field instances have been added for the '
    . $bundle . ' bundle.' . ' Time elapsed: ' .
    timer_read('field_instances_creation') . 'ms');
}

/*
  Helper code:
  module_load_include('inc', 'biblio', 'includes/biblio.fields');
  dpm(json_encode(biblio_parse_field_type_data_csv()));
 */
/**
 * Gets data from biblio.field.type.data.csv and returns it as an array
 *
 *  @todo remove this after development. I'm only keeping it for the time being,
 *  so that I can retrieve data easily during development.
 */
function biblio_parse_field_type_data_csv() {
  $csv_file = drupal_get_path('module', 'biblio') . '/misc/biblio.field.type.data.csv';
  $file_handle = fopen($csv_file, 'r');
  // array containing an array for each row in the csv
  $file_data = array();
  // the first line containing column names
  $header_line = fgetcsv($file_handle, 10000, ",");
  $info = entity_get_info('biblio');
  $fields = field_info_fields();

  while (!feof($file_handle)) {
    // the rest of the lines in the csv containing actual field data
    $row = fgetcsv($file_handle, 10000, ',');
    // Convert the human-readable name of the pubtype to machine name
    $bundle = str_replace(' ', '_', trim(strtolower($row[0])));
    // only for bundles that exist...
    if (isset($info['bundles'][$bundle])) {
      foreach ($header_line as $key => $field_name) {
        // only for fields that exist...
        if (isset($fields[$field_name])
          && $row[$key] != ''
          && $row[$key] != '~') {
          $file_data[$bundle][$field_name] = $row[$key];
        }
      }
    }
  }
  fclose($file_handle);
  ksort($file_data);

  return $file_data;
}

function biblio_check_instances($publication_type) {
  // Add field instances only if they dont already exist.
  if (biblio_field_instances_missing($publication_type)) {
    biblio_add_field_instances('biblio', $publication_type);
  }
}

/**
 * Determines if the field instances for a publication type have been created.
 * Used to decide whether or not to run biblio_add_field_instances when
 * admin/content/biblio/add is accessed for the first time for a specific publication type.
 *
 * @param string $bundle
 * @param string $entity_type biblio, biblio_contributor, etc...
 * @return boolean Whether or not biblio's required field instances are indeed missing.
 */
function biblio_field_instances_missing($bundle, $entity_type = 'biblio') {
  $file = drupal_get_path(
    'module', 'biblio') . '/misc/default_field_pubtype_map.json';
  $default_field_pubtype_map = json_decode(file_get_contents($file), TRUE);
  $default_instances = $default_field_pubtype_map[$entity_type][$bundle];

  // Field instances that already exist. There could be user-created field
  // instances here, so we can't assume that existing fields means we shouldn't
  // add our default field instances.
  $existing_instances = field_info_instances($entity_type, $bundle);
  if (empty($existing_instances)) {
    // If no instances exist, we definitely need to add the defaults
    return TRUE;
  }
  foreach ($default_instances as $field => $label) {
    // If field name isn't already in the list of existing field instances
    if (!isset($existing_instances[$field])) {
      // As soon as we find one default instance that doesn't already exist, we
      // know that all defaults need to be added
      return TRUE;
    }
  }
  // Every default field instance already exists for the given publication type
  return FALSE;
}

/**
 * Get a list of all biblio fields that have been created, and are prefixed
 * with "biblio_"
 *
 * @return array
 */
function biblio_field_list() {
  // Get an array of all fields
  $fields = field_info_fields();
  $biblio_fields = array();
  foreach ($fields as $field => $info) {
    // Check if name of field has the 'biblio_' prefix
    if (strstr($field, 'biblio_') != FALSE
      || isset($info['bundles']['biblio'])
      || ($info['type'] == 'biblio_text')) {
      $biblio_fields[] = $field;
    }
  }

  return $biblio_fields;
}

/**
 * Deletes all Biblio fields
 *
 * @return array Array of all fields that were deleted.
 */
function biblio_delete_fields() {
  $db_fields = array('id', 'field_name', 'entity_type', 'bundle');
  $result = db_select('field_config_instance', 'fci')
    ->fields('fci', $db_fields)
    ->condition(db_or()
      ->condition('entity_type', 'biblio')
      ->condition('entity_type', 'biblio_contributor'))
    ->condition('deleted', '0')
    ->execute();


    // while ($row = $result->fetchAssoc()) {
    //   $instances_to_delete[] = $row;
    // }

    // $batch = array(
    //   'title' => t('Deleting Field Instances'),
    //   'operations' => array(
    //     array('biblio_batch_delete_field', array($instances_to_delete)),
    //   ),
    //   'finished' => 'biblio_batch_delete_field_finished',
    // );
    // batch_set($batch);
    // batch_process('admin/modules');

  while ($row = $result->fetchAssoc()) {
    field_delete_instance($row, TRUE);
  }

  $fields = biblio_field_list();
  foreach ($fields as $field) {
    // Each iteration takes ~90ms to run
    field_delete_field($field);
  }

  return $fields;
}

function biblio_batch_delete_field($instances, &$context) {
  $iteration_limit = 10;
  $i = 0;
  $context['sandbox']['max'] = count($instances);
  error_log($i);

  foreach ($instances as $instance) {
    error_log($i);
    if ($i >= $iteration_limit)
      break;
    if (in_array($instance['id'], $context['sandbox']['completed_fields']))
      continue;
    field_delete_field($instance);
    $context['sandbox']['completed_fields'][] = $instance['id'];
    $i++;
    $context['sandbox']['progress']++;
  }

  if ($context['sandbox']['progress'] != $context['sandbox']['max']) {
    $context['finished'] = $context['sandbox']['progress'] / $context['sandbox']['max'];
  }
}

function biblio_batch_delete_field_finished($success, $results, $operations, $timer) {
  drupal_set_message('Total completion time: ' . $timer);
}

/**
 * Checks all existing fields instances for a given instance, and returns wheter
 * or not it actually exists
 *
 * @param string $instance_name The machine name to check whether or not it is
 *  a valid field instance
 * @param string $entity_type biblio, contributor, etc.
 * @param string $bundle book, journal_article, etc.
 * @return boolean whether or not the given string exists as a field instance
 */
function biblio_is_field_instance($instance_name, $entity_type, $bundle) {
  $instances = field_info_instances($entity_type, $bundle);
  if (isset($instances[$instance_name])) {
    return TRUE;
  }
  return FALSE;
}

//  module_load_include('inc', 'biblio', 'includes/biblio.fields');
//  modify_biblio_field_link_data_csv();
/**
 * This function only exists because I'm lazy and don't want to manipulate
 * CSV data by hand. Copy the above lines to call this function quickly
 * @todo remove after development
 */
function modify_biblio_field_link_data_csv() {
  $csv_file = drupal_get_path('module', 'biblio') . '/misc/biblio.field.link.data.csv';
  $handle = fopen($csv_file, 'r+');
  $data = biblio_parse_field_link_data_csv();
  $data_to_write[] = array_keys($data[1]);
  foreach($data as $value) {
    $data_to_write[] = $value;
  }

  foreach($data_to_write as $key => $row) {
    if (isset($row[0])) {
      // Modify headers here...
    }
    if (isset($row['fid'])) {
      // Modify data here...

      // get rid of unsupported field types listed in the CSV
      // @todo clean up the CSV and replace with applicable field types
      if ($row['type'] == 'contrib_widget' ||
          $row['type'] == 'textfield' ||
          $row['type'] == 'select'
          ) {
        $data_to_write[$key]['type'] = 'text';
      }
      if ($row['type'] == 'text_format') {
        $data_to_write[$key]['type'] = 'text_long';
      }
      if($row['type'] == 'text') {
        $data_to_write[$key]['type'] = 'biblio_text';
      }
      // change all field ids to the order in which they are placed in the file.
      $data_to_write[$key]['fid'] = $key;

    }
  }
  foreach ($data_to_write as $key => $value) {
    if (isset($value['fid']) || isset($value[0])) {
      fputcsv($handle, $value);
    }
  }
}