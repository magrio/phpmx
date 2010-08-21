<?php // $Id: offline.inc,v 1.1.2.1 2010/07/16 13:15:22 jchatard Exp $

function drupad_offline() {
  if (!user_access('administer site configuration')) {
    return array();
  }

  if (arg(2)) {
    $switch_to = arg(2);
    if ($switch_to !== 'offline' && $switch_to !== 'online') {
      drupal_not_found();
    }
  }

  // Keep track of old state
  $was = variable_get('site_offline', 0);

  if ($switch_to === 'offline') {
    variable_set('site_offline', 1);
  }
  else if ($switch_to === 'online') {
    variable_set('site_offline', 0);
  }

  // Get new state
  $is = variable_get('site_offline', 0);

  $data['meta'] = array();

  $data['content'] = array(
    'offline_was' => $was,
    'offline_is' => $is,
  );

  drupad_serve_response($data);
}