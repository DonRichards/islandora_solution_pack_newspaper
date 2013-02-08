<?php

/**
 * @file
 * Handles the upload of a single page.
 */

/**
 * The form for uploading a single page object.
 *
 * At the moment we will always
 * append the page to the end of the newspaper, it can be re-sequenced later.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_newspaper_upload_page_form(array $form, array &$form_state) {
  $get_default_value = function($name, $default) use(&$form_state) {
    return isset($form_state['values'][$name]) ? $form_state['values'][$name] : $default;
  };
  $upload_size = min((int) ini_get('post_max_size'), (int) ini_get('upload_max_filesize'));
  $extensions = array('tiff tif');

  $out = array(
    'file' => array(
      '#title' => t('Page'),
      '#type' => 'managed_file',
      '#description' => t('Select page images to upload.<br/>Files must be less than <b>@size MB.</b><br/>Allowed file types: <b>@ext.</b>', array('@size' => $upload_size, '@ext' => $extensions[0])),
      '#default_value' => $get_default_value('files', NULL),
      '#upload_location' => 'temporary://',
      '#required' => TRUE,
      '#upload_validators' => array(
        'file_validate_extensions' => $extensions,
        'file_validate_size' => array($upload_size * 1024 * 1024),
      ),
    ),
  );

  // Don't bring up languages if the ocr module is not enabled.
  if (module_exists('islandora_ocr')) {
    module_load_include('inc', 'islandora_ocr', 'includes/utilities');
    $languages = islandora_ocr_get_enabled_tesseract_languages();
    $out['language'] = array(
      '#title' => t('Language'),
      '#type' => 'select',
      '#description' => t('Please select the language that the page is written in.'),
      '#options' => $languages,
      '#default_value' => $get_default_value('language', NULL),
    );
  }

  return $out;
}

/**
 * Sets RELS-EXT/RELS-INT properties and creates the 'OBJ' datastream.
 *
 * @param array $form
 *   The Drupal form definition.
 * @param array $form_state
 *   The Drupal form state.
 */
function islandora_newspaper_upload_page_form_submit(array $form, array &$form_state) {
  module_load_include('inc', 'islandora_paged_content', 'includes/utilities');
  $object = islandora_ingest_form_get_object($form_state);
  $file = file_load($form_state['values']['file']);
  $object->label = $file->filename;
  islandora_paged_content_update_datastream($object, drupal_realpath($file->uri), 'OBJ', NULL, NULL, 'M', FALSE);
  // Update RELS-EXT properties, page/sequence/etc, and append the page at the
  // end of the book.
  $rels_ext = $object->relationships;
  $language = $form_state['values']['language'];
  $configuration = islandora_ingest_form_get_configuration($form_state);
  $newspaper = islandora_object_load($configuration['newspaper']);
  $pages = islandora_paged_content_get_pages($newspaper);
  $num_pages = count($pages) + 1;
  if ($language) {
    islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'hasLanguage', $language, TRUE);
  }
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isPageOf', $newspaper->id);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isSequenceNumber', (string) $num_pages, TRUE);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isPageNumber', (string) $num_pages, TRUE);
  islandora_paged_content_set_relationship($rels_ext, ISLANDORA_RELS_EXT_URI, 'isSection', '1', TRUE);
  islandora_paged_content_set_relationship($rels_ext, FEDORA_RELS_EXT_URI, 'isMemberOf', $newspaper->id);
  islandora_paged_content_set_relationship($rels_ext, FEDORA_MODEL_URI, 'hasModel', 'islandora:newspaperPageCModel');
}