#include "testutils.h"
#include "control.h"
#include "gsocket.h"
#include "iv.h"
#include "iv_event.h"
#include "stdio.h"
#include "stats.h"
#include "control_client.h"
#include "control_client_posix.c"

#define CONTROL_NAME "test.ctl"
GStaticMutex server_starting = G_STATIC_MUTEX_INIT;
extern int verbose_flag;
extern int debug_flag;
extern int trace_flag;

GThread *server_thread;
struct iv_task check_stop_server_thread;
gboolean stop_server_thread = FALSE;

void
check_stop_signal(void *cookie)
{
  if (stop_server_thread)
    iv_quit();
  else
    iv_task_register(&check_stop_server_thread);
}

void *
control_socket_server_thread(void *user_data)
{
  iv_init();
  IV_TASK_INIT(&check_stop_server_thread);
  check_stop_server_thread.handler = check_stop_signal;
  check_stop_server_thread.cookie = NULL;
  iv_task_register(&check_stop_server_thread);
  control_init(CONTROL_NAME);
  g_static_mutex_unlock(&server_starting);
  iv_main();
  control_destroy();
  iv_deinit();
  return NULL;
}

void test_verbose()
{
  ControlClient *control_client = control_client_new(CONTROL_NAME);
  GString *reply = NULL;
  gchar *cmd;

  assert_true(control_client_connect(control_client),"Can't connect to the control server");

  cmd = "LOG VERBOSE ON\n";
  verbose_flag = 0;
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);

  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"OK","Bad reply");
  g_string_free(reply, TRUE);
  assert_gint(verbose_flag,1,"Wrong behaviour");

  cmd = "LOG VERBOSE\n";
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);
  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"VERBOSE=1","Bad Reply");
  g_string_free(reply, TRUE);

  cmd = "LOG VERBOSE OFF\n";
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);

  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"OK","Bad reply");
  g_string_free(reply, TRUE);
  assert_gint(verbose_flag,0,"Wrong behaviour");

  control_client_free(control_client);
}

void
test_debug()
{
  ControlClient *control_client = control_client_new(CONTROL_NAME);
  GString *reply = NULL;
  gchar *cmd;

  assert_true(control_client_connect(control_client),"Can't connect to the control server");

  cmd = "LOG DEBUG ON\n";
  debug_flag = 0;
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);

  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"OK","Bad reply");
  g_string_free(reply, TRUE);
  assert_gint(debug_flag,1,"Wrong behaviour");

  cmd = "LOG DEBUG\n";
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);
  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"DEBUG=1","Bad Reply");
  g_string_free(reply, TRUE);

  cmd = "LOG DEBUG OFF\n";
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);

  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"OK","Bad reply");
  g_string_free(reply, TRUE);
  assert_gint(debug_flag,0,"Wrong behaviour");

  control_client_free(control_client);
}

void
test_trace()
{
  ControlClient *control_client = control_client_new(CONTROL_NAME);
  GString *reply = NULL;
  gchar *cmd;

  assert_true(control_client_connect(control_client),"Can't connect to the control server");

  cmd = "LOG TRACE ON\n";
  trace_flag = 0;
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);

  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"OK","Bad reply");
  g_string_free(reply, TRUE);
  assert_gint(trace_flag,1,"Wrong behaviour");

  cmd = "LOG TRACE\n";
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);
  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"TRACE=1","Bad Reply");
  g_string_free(reply, TRUE);

  cmd = "LOG TRACE OFF\n";
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);

  reply = control_client_read_reply(control_client);
  assert_string(reply->str,"OK","Bad reply");
  g_string_free(reply, TRUE);
  assert_gint(trace_flag,0,"Wrong behaviour");

  control_client_free(control_client);
}

void
test_stats()
{
  ControlClient *control_client = control_client_new(CONTROL_NAME);
  GString *reply = NULL;
  gchar *cmd;
  StatsCounterItem *counter = NULL;

  stats_init();
  stats_lock();
  stats_register_counter(0, SCS_CENTER, "id", "received", SC_TYPE_PROCESSED, &counter);
  stats_unlock();

  assert_true(control_client_connect(control_client),"Can't connect to the control server");

  cmd = "STATS\n";
  assert_gint(control_client_send_command(control_client,cmd),strlen(cmd),"Can't send the whole command %s",cmd);

  reply = control_client_read_reply(control_client);
  assert_string(reply->str, "SourceName;SourceId;SourceInstance;State;Type;Number\ncenter;id;received;a;processed;0", "Bad reply");
  g_string_free(reply, TRUE);

  stats_destroy();
  control_client_free(control_client);
}


int
main(int argc G_GNUC_UNUSED, char *argv[] G_GNUC_UNUSED)
{
  g_thread_init(NULL);

  g_static_mutex_lock(&server_starting);
  server_thread = g_thread_create(control_socket_server_thread, NULL, TRUE, NULL);
  g_static_mutex_lock(&server_starting);
  g_static_mutex_unlock(&server_starting);

  test_verbose();
  test_debug();
  test_trace();

  test_stats();

  stop_server_thread = TRUE;
  g_thread_join(server_thread);
  unlink(CONTROL_NAME);
  return 0;
}
