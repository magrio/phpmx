<?php // $Id: dblog.inc,v 1.1.2.1 2010/07/16 13:15:22 jchatard Exp $

function drupad_dblog() {
  if (!user_access('access site reports')) {
    return array();
  }

  $count_query = "SELECT count(wid) FROM {watchdog}";
  $result = pager_query("SELECT w.*
                         FROM {watchdog} w
                         ORDER BY w.timestamp DESC", 50, 0, $count_query);

  while ($message = db_fetch_object($result)) {
    if ($message->variables !== 'N;') {
      $message->message = t($message->message, unserialize($message->variables));
    }
    unset($message->variables);
    $message->wid = (int) $message->wid;
    $message->uid = (int) $message->uid;
    $message->timestamp = (int) $message->timestamp;
    $message->severity = (int) $message->severity;
    $account = db_fetch_object(db_query("SELECT name, mail, picture FROM {users} WHERE uid = %d", $message->uid));
    $message->username =  $account->name ? $account->name : t(variable_get('anonymous', 'Anonymous'));
    $message->mail = $account->mail ? $account->mail : NULL;
    $message->picture = $account->picture ? url($account->picture, array('absolute' => TRUE)) : NULL;
    $messages[] = $message;
  }

  $datas['content'] = $messages;

  global $pager_total_items;
  $datas['meta']['total_count'] = $pager_total_items[0];

  drupad_serve_response($datas);
}
