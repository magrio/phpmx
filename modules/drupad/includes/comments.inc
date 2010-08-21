<?php // $Id: comments.inc,v 1.1.2.1 2010/07/16 13:15:22 jchatard Exp $

function drupad_comments() {
  // Switch on the verb/action to take
  switch (arg(3)) {
    case 'publish':
      $data = drupad_comments_publish_cid(arg(2));
      break;
    case 'unpublish':
      $data = drupad_comments_unpublish_cid(arg(2));
      break;
    case 'report-mollom-delete':
      $data = drupad_comments_report_and_delete_cid(arg(2));
      break;
    case 'report-mollom-unpublish':
      $data = drupad_comments_report_and_unpublish_cid(arg(2));
      break;
    case 'delete':
      $data = drupad_comments_delete_cid(arg(2));
      break;
    case 'reply':
      $data = drupad_comments_reply_cid(arg(2));
      break;
    case 'node':
      $data = drupad_comments_post_to_nid(arg(2));
      break;
    default:
      $data = drupad_comments_list();
      break;
  }
  drupad_serve_response($data);
}

function drupad_comments_list() {
  if (!user_access('access comments')) {
    return array();
  }

  global $pager_total_items;

  if (isset($_GET['status']) && $_GET['status'] != '' && is_numeric($_GET['status'])) {
    $status = (int) $_GET['status'];
  }
  else {
    $status = COMMENT_PUBLISHED;
  }

  $comments = array();

  $count_query = "SELECT count(cid) FROM {comments} WHERE status = %d AND nid <> 0";

  $result = pager_query('SELECT c.subject, c.nid, c.cid, c.comment, c.timestamp,
                                c.status, c.name, c.homepage, u.name AS registered_name,
                                u.uid, u.mail, u.picture, n.title as node_title, n.comment as node_comment_mode
                                FROM {comments} c
                                INNER JOIN {users} u
                                  ON u.uid = c.uid
                                INNER JOIN {node} n
                                  ON n.nid = c.nid
                                WHERE c.status = %d
                                AND c.nid <> 0
                                ORDER BY c.timestamp DESC', 50, 0, $count_query, $status);

  while ($comment = db_fetch_object($result)) {

    $comment->picture = $comment->picture ? url($comment->picture, array('absolute' => TRUE)) : NULL;

    $comment->registered_name = $comment->registered_name != '' ? $comment->registered_name : t(variable_get('anonymous', 'Anonymous'));

    $comment->node_url = url('node/'. $comment->nid, array('absolute' => TRUE));

    $comment->comment_url = url('node/'. $comment->nid, array('absolute' => TRUE, 'fragment' => "comment-{$comment->cid}"));

    $node = node_load($comment->nid);

    $comment->comment = theme('comment_view', $comment, $node);

    $comments[] = $comment;
  }

  $data['content'] = $comments;

  $data['meta']['total_count'] = $pager_total_items[0];
  $data['meta']['type'] = 'comments';
  return $data;
}

function drupad_comments_publish_cid($cid) {
  if (!user_access('administer comments')) {
    return array();
  }

  $comment = _comment_load($cid);
  $comment->status = COMMENT_PUBLISHED;
  $comment = (array) $comment;
  $cid = comment_save($comment);
  $comment = _comment_load($cid);

  $data = array(
    'meta' => array(
      'cid' => $cid,
      'change' => 'status',
      'value' => COMMENT_PUBLISHED,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_comments_unpublish_cid($cid) {
  if (!user_access('administer comments')) {
    return array();
  }

  $comment = _comment_load($cid);
  $comment->status = COMMENT_NOT_PUBLISHED;
  $comment = (array) $comment;
  $cid = comment_save($comment);
  $comment = _comment_load($cid);

  $data = array(
    'meta' => array(
      'cid' => $cid,
      'change' => 'status',
      'value' => COMMENT_NOT_PUBLISHED,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_comments_delete_cid($cid) {
  if (!user_access('administer comments')) {
    return array();
  }

  $comment = _comment_load($cid);
  module_load_include('inc', 'comment', 'comment.admin');
  // Delete comment and its replies.
  _comment_delete_thread($comment);
  _comment_update_node_statistics($comment->nid);
  // Clear the cache so an anonymous user sees that his comment was deleted.
  cache_clear_all();
  $data = array(
    'meta' => array(
      'cid' => $nid,
      'change' => 'deleted',
      'value' => 1,
    ),
    'content' => array(),
  );
  return $data;
}

function drupad_comments_report_and_delete_cid($cid) {
  if (!user_access('administer comments')) {
    return array();
  }

  module_load_include('inc', 'comment', 'comment.admin');
  $comment = _comment_load($cid);
  // First, report the comments as spam to Mollom.com.
  if (module_exists('mollom') && $data = mollom_data_load('comment', $cid)) {
    _mollom_send_feedback($data->session);
  }

  _comment_delete_thread($comment);
  _comment_update_node_statistics($comment->nid);

  cache_clear_all();

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

function drupad_comments_report_and_unpublish_cid($cid) {
  if (!user_access('administer comments')) {
    return array();
  }

  $comment = _comment_load($cid);
  // First, report the comments as spam to Mollom.com.
  if (module_exists('mollom') && $data = mollom_data_load('comment', $cid)) {
    _mollom_send_feedback($data->session);
  }

  db_query("UPDATE {comments} SET status = %d WHERE cid = %d", COMMENT_NOT_PUBLISHED, $cid);

  _comment_update_node_statistics($comment->nid);

  cache_clear_all();

  $data = array(
    'meta' => array(
      'cid' => $cid,
      'change' => 'status',
      'value' => COMMENT_NOT_PUBLISHED,
    ),
    'content' => array(),
  );

  return $data;
}

function drupad_comments_reply_cid($cid) {
  if (!user_access('post comments') || !user_access('post comments without approval')) {
    return array();
  }

  global $user;

  // Validate the comment's subject. If not specified, extract
  // one from the comment's body.
  if (trim($_POST['subject']) == '') {
    // The body may be in any format, so we:
    // 1) Filter it into HTML
    // 2) Strip out all HTML tags
    // 3) Convert entities back to plain-text.
    // Note: format is checked by check_markup().
    $_POST['subject'] = truncate_utf8(trim(decode_entities(strip_tags(check_markup($_POST['comment'], 0)))), 29, TRUE);
    // Edge cases where the comment body is populated only by HTML tags will
    // require a default subject.
    if ($_POST['subject'] == '') {
      $_POST['subject'] = t('(No subject)');
    }
  }

  // Find the node for which the comment is made
  $nid = db_result(db_query("SELECT nid FROM {comments} WHERE cid = %d", $cid));

  // Build the comment array
  $edit = array(
    'nid' => $nid,
    'pid' => $cid,
    'uid' => $user->uid,
    'subject' => $_POST['subject'],
    'comment' => $_POST['comment'],
    'format' => 0,
  );

  if ($cid = comment_save($edit)) {
    $data = array(
      'meta' => array(
        'cid' => $cid,
        'change' => 'status',
        'value' => TRUE,
      ),
      'content' => array(),
    );
  }
  else {
    $data = array(
      'meta' => array(
        'cid' => $cid,
        'change' => 'status',
        'value' => FALSE,
      ),
      'content' => array(),
    );
  }

  return $data;
}

function drupad_comments_post_to_nid($nid) {
  if (!user_access('post comments') || !user_access('post comments without approval')) {
    return array();
  }

  global $user;

  // Validate the comment's subject. If not specified, extract
  // one from the comment's body.
  if (trim($_POST['subject']) == '') {
    // The body may be in any format, so we:
    // 1) Filter it into HTML
    // 2) Strip out all HTML tags
    // 3) Convert entities back to plain-text.
    // Note: format is checked by check_markup().
    $_POST['subject'] = truncate_utf8(trim(decode_entities(strip_tags(check_markup($_POST['comment'], 0)))), 29, TRUE);
    // Edge cases where the comment body is populated only by HTML tags will
    // require a default subject.
    if ($_POST['subject'] == '') {
      $_POST['subject'] = t('(No subject)');
    }
  }

  // Build the comment array
  $edit = array(
    'nid' => $nid,
    'pid' => NULL,
    'uid' => $user->uid,
    'subject' => $_POST['subject'],
    'comment' => $_POST['comment'],
    'format' => 0,
  );

  if ($cid = comment_save($edit)) {
    $data = array(
      'meta' => array(
        'cid' => $cid,
        'change' => 'status',
        'value' => TRUE,
      ),
      'content' => array(),
    );
  }
  else {
    $data = array(
      'meta' => array(
        'cid' => $cid,
        'change' => 'status',
        'value' => FALSE,
      ),
      'content' => array(),
    );
  }

  return $data;
}