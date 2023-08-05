# README

REST API for controlling a DB2 instance. 

## Dependencies

1. Docker
2. Git (optional)
3. HTTPie (optional)

## Building

Run the below docker command replacing \<password> with a password of your choosing and \<path> to where you cloned or unzipped this repo.  If desired, change the DBNAME and/or the application port mapping for 8080.

```docker run -itd --name db2rest --privileged=true -p 50000:50000 -p 8080:8080 -e LICENSE=accept -e DB2INST1_PASSWORD=<password> -e DBNAME=testdb -v C:\<path>\db2rest:/database ibmcom/db2:11.5.8.0```

Once the image is pulled and container is running, run ```docker exec -ti db2rest```.  While in the container run the stage.sh script to build the development environment.  

## Running
After the script has completed run ```make``` then ```./db2rest``` to start the application.  You can test the API endpoints using Postman, Insomnia, HTTPie, or similar tools.  The API will return a JSON object with a SQL code of zero on success.   If you stop the instance and try to stop it again, it will return a JSON object with the SQL code and related error message from the sqlca struct.  Same pattern for trying to start the instance when it is already started.

### Examples using HTTPie:
```http localhost:8080/stopinstance``` \
```http localhost:8080/startinstance``` \
```http localhost:8080/restartinstance```

### TODO:
Add endpoints to dump database names, table names, etc. \
Add OAuth 2.0 support to secure the control endpoints.