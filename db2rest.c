#include <ctype.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <db2ApiDf.h>
#include <sqlenv.h>
#include <ulfius.h>

#define PORT 8080

pthread_mutex_t lock;
pthread_cond_t  wait;

void exit_server(struct _u_instance* instance, int exit_value) {
  printf("End framework\n");
  
  ulfius_stop_framework(instance);
  ulfius_clean_instance(instance);

  exit(exit_value);
}

void* signal_thread(void* arg) {
  sigset_t *sigs = arg;
  int res, signum;

  res = sigwait(sigs, &signum);
  if (res) {
    fprintf(stderr, "Waiting for signals failed\n");
    exit(1);
  }
  if (signum == SIGQUIT || signum == SIGINT || signum == SIGTERM || signum == SIGHUP) {
    printf(" Received close signal: %s\n", strsignal(signum));
    pthread_mutex_lock(&lock);
    pthread_cond_signal(&wait);
    pthread_mutex_unlock(&lock);
    return NULL;
  }
  else {
    printf(" Recieved unexpected signal: %s\n", strsignal(signum));
  }
}

void db2_start_instance(struct sqlca *sqlca) {
  struct db2InstanceStartStruct instanceStartStruct;

  instanceStartStruct.iIsRemote = false;
  instanceStartStruct.piRemoteInstName = NULL;
  instanceStartStruct.piCommData = NULL;
  instanceStartStruct.piStartOpts = NULL;

  db2InstanceStart(db2Version11580, &instanceStartStruct, sqlca);
}

void db2_stop_instance(struct sqlca *sqlca) {
  struct db2InstanceStopStruct instanceStopStruct;

  sqlefrce(SQL_ALL_USERS, NULL, SQL_ASYNCH, sqlca);
  
  instanceStopStruct.iIsRemote = false;
  instanceStopStruct.piRemoteInstName = NULL;
  instanceStopStruct.piCommData = NULL;
  instanceStopStruct.piStopOpts = NULL;

  db2InstanceStop(db2Version11580, &instanceStopStruct, sqlca);
}

json_t* create_json(struct sqlca *sqlca) {
  char errorMsg[1024];
  int errorLen;
  json_t *json, *code, *emsg;
  
  json = json_object();
  code = json_integer(sqlca->sqlcode); 

  json_object_set(json, "sqlcode", code);

  if (sqlca->sqlcode != 0) {
    sqlaintp(errorMsg, 1024, 80, sqlca);
    
    errorLen = strlen(errorMsg);
    if (errorMsg[errorLen-1] == '\n')
      errorMsg[errorLen-1] = '\0';
    
    emsg = json_string(errorMsg);
    json_object_set(json, "error", emsg);
  }

  return json;
}

int callback_hello_world (const struct _u_request * request, struct _u_response * response, void * user_data) {
  // basically a healthcheck
  ulfius_set_string_body_response(response, 200, "Hello World!");
  return U_CALLBACK_CONTINUE;
}

int callback_databases (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct sqlca sqlca;
  struct sqledinfo *info;
  struct db2DbDirOpenScanStruct open;
  struct db2DbDirNextEntryStructV9 entry;
  struct db2DbDirCloseScanStruct close;
  unsigned short entries, handle, pHandle;
  json_t *json, *array, *sqlobj, *name, *path, *type;
  char dbname[8], drive[20], dbtype[20];

  open.piPath = NULL;
  open.oHandle = 0;

  db2DbDirOpenScan(db2Version11580, &open, &sqlca);
  entry.iHandle = open.oHandle;

  array = json_array();

  for (int i =0; i < open.oNumEntries; i++) {
        json = json_object();

        db2DbDirGetNextEntry(db2Version11580, &entry, &sqlca);
        sqlobj = create_json(&sqlca);
        json_object_set(json, "getentry", sqlobj);

        strncpy(dbname, entry.poDbDirEntry->dbname, sizeof(dbname));
        char *copy = dbname;
        char *trimmed = strtok_r(copy, "\r\t\n ", &copy);
        name = json_string(trimmed);
        json_object_set(json, "database", name);

        strncpy(drive, entry.poDbDirEntry->drive, sizeof(drive));
        copy = drive;
        trimmed = strtok_r(copy, "\r\t\n ", &copy);
        path = json_string(trimmed);
        json_object_set(json, "path", path);

        strncpy(dbtype, entry.poDbDirEntry->dbtype, sizeof(dbtype));

        int i = strlen(dbtype) - 1;
        while (i > 0) {
            if (isprint(dbtype[i]) == 0 || isspace(dbtype[i]) != 0 || iscntrl(dbtype[i]) != 0) {
                i--;
            }
            else break;
        }

        dbtype[i + 1] = '\0';

        if (dbtype[0] == '\2') {
          memmove(&dbtype[0], &dbtype[1], strlen(dbtype) - 1);
        }

        type = json_string(dbtype);
        json_object_set(json, "type", type);

        json_array_append(array, json);
  }

  close.iHandle = open.oHandle;
  db2DbDirCloseScan(db2Version11580, &close, &sqlca);

  ulfius_set_json_body_response(response, 200, array);
  json_decref(json);
  return U_CALLBACK_CONTINUE;
}

int callback_start_instance (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct sqlca sqlca;
  json_t *json;

  db2_start_instance(&sqlca);
  json = create_json(&sqlca);

  ulfius_set_json_body_response(response, 200, json);
  json_decref(json);
  return U_CALLBACK_CONTINUE;
}

int callback_stop_instance (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct sqlca sqlca;
  json_t *json;

  db2_stop_instance(&sqlca);
  json = create_json(&sqlca);

  ulfius_set_json_body_response(response, 200, json);
  json_decref(json);
  return U_CALLBACK_CONTINUE;
}

int callback_restart_instance (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct sqlca sqlca;
  json_t *json;

  db2_stop_instance(&sqlca);

  if (sqlca.sqlcode != 0) {
    json = create_json(&sqlca);

    ulfius_set_json_body_response(response, 200, json);
    json_decref(json);
    return U_CALLBACK_CONTINUE;
  }
  
  db2_start_instance(&sqlca);
  json = create_json(&sqlca);
  
  ulfius_set_json_body_response(response, 200, json);
  json_decref(json);
  return U_CALLBACK_CONTINUE;
}

int main(void) {
  struct _u_instance instance;
  pthread_t signal_thread_id;
  static sigset_t close_signals;

  // Initialize instance with the port number
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) {
    fprintf(stderr, "Error ulfius_init_instance, abort\n");
    return 1;
  }

  if (pthread_mutex_init(&lock, NULL) || pthread_cond_init(&wait, NULL)) {
    fprintf(stderr, "Error initializing mutex lock and condition\n");
  }

  if (sigemptyset(&close_signals) == -1 || sigaddset(&close_signals, SIGQUIT) == -1 || sigaddset(&close_signals, SIGINT) == -1 ||
      sigaddset(&close_signals, SIGTERM) == -1 || sigaddset(&close_signals, SIGHUP) == -1 ) {
    fprintf(stderr, "Error creating signal mask\n");
    exit_server(&instance, 1);
  }
  
  if (pthread_sigmask(SIG_BLOCK, &close_signals, NULL)) {
    fprintf(stderr, "Error setting signal mask\n");
    exit_server(&instance, 1);
  }

  if (pthread_create(&signal_thread_id, NULL, &signal_thread, &close_signals)) {
    fprintf(stderr, "init - Error creating signal thread\n");
    exit_server(&instance, 1);
    return 1;
  }
  // Endpoint list declarations
  ulfius_add_endpoint_by_val(&instance, "GET", "/helloworld", NULL, 0, &callback_hello_world, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/databases", NULL, 0, &callback_databases, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/startinstance", NULL, 0, &callback_start_instance, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/stopinstance", NULL, 0, &callback_stop_instance, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/restartinstance", NULL, 0, &callback_restart_instance, NULL);

  // Start the framework
  if (ulfius_start_framework(&instance) == U_OK) {
    printf("Start framework on port %d\n", instance.port);

    // Wait for an interrupt to quit the application
    pthread_mutex_lock(&lock);
    pthread_cond_wait(&wait,&lock);
    pthread_mutex_unlock(&lock);
  } 
  else {
    fprintf(stderr, "Error starting framework\n");
  } 

  if (pthread_mutex_destroy(&lock) || pthread_cond_destroy(&wait)) {
    fprintf(stderr, "Error initializing mutex lock and condition\n");
  }

  exit_server(&instance, 0);
}
