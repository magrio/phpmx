<?php // $Id: content.inc,v 1.1.2.1 2010/07/16 13:15:22 jchatard Exp $

function drupad_content() {

  // Switch on the verb/action to take
  switch (arg(3)) {
    case 'publish':
      $data = drupad_content_publish_nid(arg(2));
      break;
    case 'unpublish':
      $data = drupad_content_unpublish_nid(arg(2));
      break;
    case 'report-mollom-delete':
      $data = drupad_content_report_and_delete_nid(arg(2));
      break;
    case 'report-mollom-unpublish':
      $data = drupad_content_report_and_unpublish_nid(arg(2));
      break;
    case 'delete':
      $data = drupad_content_delete_nid(arg(2));
      break;
    case 'promote':
      $data = drupad_content_promote_nid(arg(2));
      break;
    case 'unpromote':
      $data = drupad_content_unpromote_nid(arg(2));
      break;
    case 'sticky':
      $data = drupad_content_sticky_nid(arg(2));
      break;
    case 'unsticky':
      $data = drupad_content_unsticky_nid(arg(2));
      break;
    default:
      $data = drupad_content_list();
      break;
  }
  drupad_serve_response($data);
}

function drupad_content_list () {

  if (isset($_GET['status']) && $_GET['status'] != '' && is_numeric($_GET['status'])) {
    $status = (int) $_GET['status'];
  }
  else {
    $status = 1;
  }

  $nodes = array();

  $content_types = variable_get('drupad_content_types_excluded', array());

  $content_types = array_keys($content_types);

  if (count($content_types) > 0) {
    $not_in = " AND n.type NOT IN (". db_placeholders($content_types, 'varchar') .") ";
  }
  else {
    $not_in = " ";
  }

  $count_query = "SELECT count(nid) FROM {node} n INNER JOIN {users} u ON n.uid = u.uid
                  INNER JOIN {node_type} nt
                    ON n.type = nt.type
                  WHERE n.status = %d"
                  . $not_in .
                  "ORDER BY n.changed DESC, n.title ASC";

  $query = "SELECT n.*, u.name AS username, u.picture, u.mail, nt.name AS content_type, r.teaser, r.format
            FROM {node} n
            INNER JOIN {users} u
             ON n.uid = u.uid
            INNER JOIN {node_type} nt
             ON n.type = nt.type
            INNER JOIN {node_revisions} r
             ON n.vid = r.vid
            WHERE n.status = %d "
            . $not_in .
            "ORDER BY n.changed DESC, n.title ASC";

  $args = array($status);

  if (count($content_types) > 0) {
    foreach ($content_types as $value) {
      $args[] = $value;
    }
  }

  $result = pager_query($query, 50, 0, $count_query, $args);

  while ($node = db_fetch_object($result)) {
    unset($node->moderate); // We don't care, it's not used anymore
    $node->picture = $node->picture ? url($node->picture, array('absolute' => TRUE)) : NULL;
    $node->username = $node->username != '' ? $node->username : t('Anonymous');

    $node_object = node_load($node->nid);
    $node->teaser = node_view($node_object, $teaser = FALSE, $page = TRUE, $links = FALSE);
    // Locale module support
    if (module_exists('locale')) {
      $language_full = db_result(db_query("SELECT name FROM {languages} WHERE language = '%s'", $node->language));
      $node->language_full =  $language_full != FALSE ? $language_full : "";
    }

    $nodes[] = $node;
  }
  $data['content'] = $nodes;

  global $pager_total_items;
  $data['meta']['total_count'] = $pager_total_items[0];

  return $data;
}

function drupad_content_publish_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  $node = node_load($nid);
  $node->status = 1;
  node_save($node);

  $node_link = l(t('view'), 'node/'. $node->nid);
  $watchdog_args = array('@type' => $node->type, '%title' => $node->title);
  watchdog('content', '@type: updated %title.', $watchdog_args, WATCHDOG_NOTICE, $node_link);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'status',
      'value' => 1,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_unpublish_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  $node = node_load($nid);
  $node->status = 0;
  node_save($node);

  $node_link = l(t('view'), 'node/'. $node->nid);
  $watchdog_args = array('@type' => $node->type, '%title' => $node->title);
  watchdog('content', '@type: updated %title.', $watchdog_args, WATCHDOG_NOTICE, $node_link);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'status',
      'value' => 0,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_report_and_delete_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  if (module_exists('mollom') && $data = mollom_data_load('node', $nid)) {
    _mollom_send_feedback($data->session);
  }

  node_delete($nid);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'deleted',
      'value' => 1,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_report_and_unpublish_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  if (module_exists('mollom') && $data = mollom_data_load('node', $nid)) {
    _mollom_send_feedback($data->session);
  }

  $node = node_load($nid);

  $node->status = 0;
  node_save($node);

  $node_link = l(t('view'), 'node/'. $node->nid);
  $watchdog_args = array('@type' => $node->type, '%title' => $node->title);
  watchdog('content', '@type: updated %title.', $watchdog_args, WATCHDOG_NOTICE, $node_link);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'status',
      'value' => 0,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_delete_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  node_delete($nid);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'deleted',
      'value' => 1,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_promote_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  $node = node_load($nid);
  $node->promote = 1;
  node_save($node);

  $node_link = l(t('view'), 'node/'. $node->nid);
  $watchdog_args = array('@type' => $node->type, '%title' => $node->title);
  watchdog('content', '@type: updated %title.', $watchdog_args, WATCHDOG_NOTICE, $node_link);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'promote',
      'value' => 1,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_unpromote_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  $node = node_load($nid);
  $node->promote = 0;
  node_save($node);

  $node_link = l(t('view'), 'node/'. $node->nid);
  $watchdog_args = array('@type' => $node->type, '%title' => $node->title);
  watchdog('content', '@type: updated %title.', $watchdog_args, WATCHDOG_NOTICE, $node_link);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'promote',
      'value' => 0,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_sticky_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  $node = node_load($nid);
  $node->sticky = 1;
  node_save($node);

  $node_link = l(t('view'), 'node/'. $node->nid);
  $watchdog_args = array('@type' => $node->type, '%title' => $node->title);
  watchdog('content', '@type: updated %title.', $watchdog_args, WATCHDOG_NOTICE, $node_link);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'sticky',
      'value' => 1,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_content_unsticky_nid($nid) {
  if (!user_access('administer nodes')) {
    return array();
  }

  $node = node_load($nid);
  $node->sticky = 0;
  node_save($node);

  $node_link = l(t('view'), 'node/'. $node->nid);
  $watchdog_args = array('@type' => $node->type, '%title' => $node->title);
  watchdog('content', '@type: updated %title.', $watchdog_args, WATCHDOG_NOTICE, $node_link);

  $data = array(
    'meta' => array(
      'nid' => $nid,
      'change' => 'sticky',
      'value' => 0,
    ),
    'content' => array(),
  );

  return $data;
}