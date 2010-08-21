<?php // $Id: cron_tasks.inc,v 1.1.2.1 2010/07/16 13:15:22 jchatard Exp $

function drupad_cron_tasks() {
  if (!user_access('administer site configuration')) {
    return array();
  }

  if (arg(2) == 'run') {
    $run = TRUE;
    drupal_cron_run();
  }
  else {
    $run = FALSE;
  }

  $data['meta'] = array();

  $data['content'] = array(
    'last_run' => variable_get('cron_last', NULL),
  );

  if ($run) {
    $data['content']['was_just_run'] = TRUE;
  }
  else {
    $data['content']['was_just_run'] = FALSE;
  }

  drupad_serve_response($data);
}