#include <stdio.h>
#include <string.h>
#include <db2ApiDf.h>
#include <sqlenv.h>
#include <ulfius.h>

#define PORT 8080

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

void db2_start_instance(struct sqlca *sqlca){
  struct db2InstanceStartStruct instanceStartStruct;

  instanceStartStruct.iIsRemote = FALSE;
  instanceStartStruct.piRemoteInstName = NULL;
  instanceStartStruct.piCommData = NULL;
  instanceStartStruct.piStartOpts = NULL;

  db2InstanceStart(db2Version11580, &instanceStartStruct, sqlca);
}

void db2_stop_instance(struct sqlca *sqlca){
  struct db2InstanceStopStruct instanceStopStruct;

  sqlefrce(SQL_ALL_USERS, NULL, SQL_ASYNCH, sqlca);
  
  instanceStopStruct.iIsRemote = FALSE;
  instanceStopStruct.piRemoteInstName = NULL;
  instanceStopStruct.piCommData = NULL;
  instanceStopStruct.piStopOpts = NULL;

  db2InstanceStop(db2Version11580, &instanceStopStruct, sqlca);
}

json_t* create_json(struct sqlca *sqlca){
  char errorMsg[1024];
  int errorLen;
  json_t *json, *code, *emsg;
  
  json = json_object();
  code = json_integer(sqlca->sqlcode); 

  json_object_set(json, "sqlcode", code);

  if(sqlca->sqlcode != 0){
    sqlaintp(errorMsg, 1024, 80, sqlca);
    
    errorLen = strlen(errorMsg);
    if(errorMsg[errorLen-1] == '\n')
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

int callback_start_instance (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct sqlca sqlca;
  json_t *json;

  db2_start_instance(&sqlca);
  json = create_json(&sqlca);

  ulfius_set_json_body_response(response, 200, json);
  return U_CALLBACK_CONTINUE;
}

int callback_stop_instance (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct sqlca sqlca;
  json_t *json;

  db2_stop_instance(&sqlca);
  json = create_json(&sqlca);

  ulfius_set_json_body_response(response, 200, json);
  return U_CALLBACK_CONTINUE;
}

int callback_restart_instance (const struct _u_request * request, struct _u_response * response, void * user_data) {
  struct sqlca sqlca;
  json_t *json;

  db2_stop_instance(&sqlca);

  if(sqlca.sqlcode != 0){
    json = create_json(&sqlca);

    ulfius_set_json_body_response(response, 200, json);
    return U_CALLBACK_CONTINUE;
  }
  
  db2_start_instance(&sqlca);
  json = create_json(&sqlca);
  
  ulfius_set_json_body_response(response, 200, json);
  return U_CALLBACK_CONTINUE;
}

int main(void) {
  struct _u_instance instance;

  // Initialize instance with the port number
  if (ulfius_init_instance(&instance, PORT, NULL, NULL) != U_OK) {
    fprintf(stderr, "Error ulfius_init_instance, abort\n");
    return(1);
  }

  // Endpoint list declarations
  ulfius_add_endpoint_by_val(&instance, "GET", "/helloworld", NULL, 0, &callback_hello_world, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/startinstance", NULL, 0, &callback_start_instance, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/stopinstance", NULL, 0, &callback_stop_instance, NULL);
  ulfius_add_endpoint_by_val(&instance, "GET", "/restartinstance", NULL, 0, &callback_restart_instance, NULL);

  // Start the framework
  if (ulfius_start_framework(&instance) == U_OK) {
    printf("Start framework on port %d\n", instance.port);

    // Wait for the user to press <enter> on the console to quit the application
    getchar();
  } else {
    fprintf(stderr, "Error starting framework\n");
  }
  printf("End framework\n");

  ulfius_stop_framework(&instance);
  ulfius_clean_instance(&instance);

  return 0;
}
