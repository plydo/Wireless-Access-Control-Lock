// api_bodgery_v1.cpp - support for Bodgery v1 backend
/*
Copyright 2024 Timm Murray
Copyright 2024 Mark Pickhard
Copyright rights associated with this file are nonexclusively transferred to The Bodgery Inc,
  a 501c(3) nonprofit entity.
This file is part of WACL. WACL is free software: you can redistribute it and/or modify it under
  the terms of the GNU General Public License as published by the Free Software Foundation, either
  version 3 of the License, or (at your option) any later version.
WACL is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even the
  implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
  Public License for more details.
You should have received a copy of the GNU General Public License along with WACL. If not, see
  <https://www.gnu.org/licenses/>.
*/
#include "main.h"

#if ENABLE_BACKEND_BODGERY_V1

#include <HTTPClient.h>
#include <ArduinoJson.h>

/*
This looks up the ID (1st arg) using the backend. It returns the enable in the
2nd arg and the name in the 3rd arg. If not found, it returns ID_NOT_FOUND
in the 2nd arg. Also, the fcn return value is nonzero if an error occurred.
*/
int api_bodgery_v1_lookup(uID_t idTag, unsigned long &idEnable, char idName[])
{
  char buffer[20];
  JsonDocument jsonDoc;

  idEnable = 0; // initialize return values in case of error
  strlcpy(idName, ".", ID_NAME_MAX);

  sprintf(buffer, "%010u", idTag);
  HTTPClient http;
  String request = "";
  request.concat( stg.backendURL );
  request.concat( "/v1/check_tag/" );
  request.concat( buffer );
  request.concat( "/" );
  request.concat( stg.deviceName );
  String authToken = "Bearer ";
  authToken.concat( stg.backendSecret );

  auto start = millis();
  if (!http.begin(request.c_str(), root_ca)) {
    loge("Error: Couldn't begin https");
    return 10;
  }
  http.setConnectTimeout(5000 /*ms*/);
  http.setTimeout(5000 /*ms*/); // not needed?
  http.addHeader("Authorization", authToken); // http.setAuthorization(stg.backendUsername, stg.backendSecret);
  int status = http.GET();
  String body = http.getString();
  http.end();
  logd("BodgeryV1 request='%s'", request.c_str());
  logd("response=%i, t=%lums", status, millis() - start);
  logd("body='%s'", body.c_str());
  if (status < 0) {
    loge("Http status error %i", status);
    return 20;
  }

  if (status == HTTP_CODE_OK || status == 403) {
    if (body.length() == 0) {
      loge("Error: No http body");
      return 30;
    }
    DeserializationError jsonError = deserializeJson(jsonDoc, body);
    if (jsonError) {
      loge("Json error: %s", jsonError.c_str());
      return 40;
    }
    const char* full_name = jsonDoc["full_name"];
    if (full_name == nullptr) {
      loge("Error: No json name in http body");
      return 50;
    }
    strlcpy(idName, full_name, ID_NAME_MAX);
  }

  switch (status) {
    case HTTP_CODE_OK: // found, enabled/active
      idEnable = 1;
      return 0;
    case 404: // not found
      idEnable = ID_NOT_FOUND;
      return 0;
    case 403: // found, disabled/inactive
      return 0;
    case 400: // bad request
      return 90;
    default:
      return 999;
  }
}

/*
This adds the idTag to the access list.
The fcn return value is nonzero if an error occurred.
*/
int api_bodgery_v1_add(uID_t idTag)
{
  char buffer[20];

  sprintf(buffer, "%010u", idTag);
  HTTPClient http;
  String request = "";
  request.concat( stg.backendURL );
  request.concat( "/v1/role/" );
  request.concat( stg.deviceGroup );
  request.concat( "/" );
  request.concat( buffer );
  String authToken = "Bearer ";
  authToken.concat( stg.backendSecret );

  auto start = millis();
  if (!http.begin(request.c_str(), root_ca)) {
    loge("Error: Couldn't begin https");
    return 10;
  }
  http.setConnectTimeout(5000 /*ms*/);
  http.setTimeout(5000 /*ms*/); // not needed?
  http.addHeader("Authorization", authToken); // http.setAuthorization(stg.backendUsername, stg.backendSecret);
  int status = http.PUT("x");
  String body = http.getString();
  http.end();
  logd("BodgeryV1 request='%s'", request.c_str());
  logd("response=%i, t=%lums", status, millis() - start);
  if (status != HTTP_CODE_CREATED) {
    loge("Error %i adding user to group '%s' device '%s'",
      status, stg.deviceGroup, stg.deviceName);
    return status;
  }
  return 0;
}

#else

/* This stub is used when not compiling support for this backend */
int api_bodgery_v1_lookup(uID_t idTag, unsigned long &idEnable, char idName[])
{
  idEnable = 0;
  strlcpy(idName, "Not Enabled", ID_NAME_MAX);
  return 1;
}

/* This stub is used when not compiling support for this backend */
int api_bodgery_v1_add(uID_t idTag)
  return 1;
{
#endif