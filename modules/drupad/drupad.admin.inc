<?php // $Id: drupad.admin.inc,v 1.1.2.1 2010/07/16 13:15:22 jchatard Exp $

function drupad_settings_form($form_state) {
  $form = array();

  // Services
  $form['drupad_services'] = array(
    '#type' => 'fieldset',
    '#title' => t('Enabled services'),
    '#description' => t("Select features you want your site to expose to the application. Users must have the permission <strong>use Drupad application</strong> to be able to login form their device. If a feature below is not enabled, it won't be shown on the application regardless of the permission, even for user 1."),
    '#collapsible' => TRUE,
  );

  foreach (drupad_available_callbacks() as $callback => $service) {

    $disabled = $service['disabled'] == TRUE ? TRUE : FALSE;

    if (!$disabled) {
      $default_value = variable_get('drupad_callback_'. $callback, 1);
    }
    else {
      variable_set('drupad_callback_'. $callback, 0);
      $default_value = 0;
    }

    $form['drupad_services']['drupad_callback_'. $callback] = array(
      '#type' => 'checkbox',
      '#title' => $service['title'],
      '#description' => $service['description'],
      '#default_value' => $default_value,
      '#disabled' => $disabled,
    );
  }

  // User permissions
  $form['options'] = array(
    '#type' => 'fieldset',
    '#title' => t('Other settings'),
    '#collapsible' => TRUE,
  );

  $content_types = node_get_types($op = 'types', $node = NULL, $reset = FALSE);

  $types = array();
  foreach ($content_types as $system_name => $content_type) {
    $types[$system_name] = $content_type->name;
  }

  asort($types, $sort_flags = SORT_STRING);

  $form['options']['drupad_content_types_excluded'] = array(
    '#type' => 'select',
    '#title' => t('Content types to exclude'),
    '#options' => $types,
    '#default_value' => variable_get('drupad_content_types_excluded', array()),
    '#description' => t('Select content types that you do not want Drupad to expose to the application, good for micro blogging streams.'),
    '#multiple' => TRUE,
  );

  $form = system_settings_form($form);

  $form['#submit'][] = 'drupad_settings_form_submit';

  return $form;
}

function drupad_settings_form_submit($form, &$form_state) {
  menu_rebuild();
}