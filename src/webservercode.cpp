// webserver.cpp
/*
Credit: Much of this code is from an anonymous author. References to that source:
https://myhomethings.eu/en/esp32-web-updater-and-spiffs-manager/
https://www.hackster.io/myhomethings/esp32-web-updater-and-spiffs-file-manager-cf8dc5
https://hackaday.com/2023/02/17/esp32-web-updater-allows-file-system-management-and-ota-updates/

The original code has been modified.
*/

#include "main.h"
#include <ESPAsyncWebServer.h>
#include <AsyncTCP.h>
#include <LittleFS.h>
#include <Update.h>
#include "webserverhtml.h"

static AsyncWebServer server(80);
static String allowedExtensionsForEdit = "txt, log, ini, htm, html, css, js";

static String filesDropdownOptions = "";
static String textareaContent = "";
static String savePath = "";
static String savePathInput = "";

static const char param_delete_path[] = "delete_path";
static const char param_edit_path[] = "edit_path";
static const char param_edit_textarea[] = "edit_textarea";
static const char param_save_path[] = "save_path";

static const char textPlain[] = "text/plain";
static const char pageNotFound[] = "404 Page not found";

static bool authNeeded(AsyncWebServerRequest *request) 
{
  return request->authenticate(stg.webserverUsername, stg.webserverPassword) ?
    false : true;
}

static bool serverDisabled(void) 
{
  return webserverTimedout.isActive() ? false : true;
}

static void notFound(AsyncWebServerRequest *request) 
{
  request->send(404, textPlain, pageNotFound);
}

static String convertFileSize(const size_t bytes)
{
  if(bytes < 10240)
  {
    return String(bytes) + " B";
  }
  else if (bytes < 2 * 1048576)
  {
    return String(bytes / 1024.0) + " kB";
  }
  else
  {
    return String(bytes / 1048576.0) + " MB";
  }
}

static String listDir(fs::FS &fs, const char * dirname, uint8_t levels)
{
  filesDropdownOptions = "";
  String listenFiles = "<table><tr><th id=\"first_td_th\">Filename</th><th>";
  listenFiles += dirname;
  listenFiles += "</th></tr>";

  File root = fs.open(dirname);
  String fail = "";
  if(!root)
  {
    fail = " the directory cannot be opened";
    return fail;
  }
  if(!root.isDirectory())
  {
    fail = " this is not a directory";
    return fail;
  }

  File file = root.openNextFile();
  while(file)
  {
    if(file.isDirectory())
    {
      listenFiles += "<tr><td id=\"first_td_th\">Dir: ";
      listenFiles += file.name();

      filesDropdownOptions += "<option value=\"";
      filesDropdownOptions += "/";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "\">";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "</option>";

      listenFiles += "</td><td> - </td></tr>";

      if(levels)
      {
        #define BUFFER_SIZE 50
        char buffer[BUFFER_SIZE];
        strlcpy(buffer, "/", BUFFER_SIZE);
        strlcat(buffer, file.name(), BUFFER_SIZE);
        listDir(fs, buffer, levels -1);
        #undef BUFFER_SIZE
      }
    }
    else // it's a File
    {
      listenFiles += "<tr><td id=\"first_td_th\">";
      listenFiles += file.name();

      filesDropdownOptions += "<option value=\"";
      filesDropdownOptions += "/";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "\">";
      filesDropdownOptions += file.name();
      filesDropdownOptions += "</option>";

      listenFiles += "</td><td>Size: ";
      listenFiles += convertFileSize(file.size());
      listenFiles += "</td></tr>";
    }
    file = root.openNextFile();
  }
  listenFiles += "</table>";
  return listenFiles;  
}

static String readFile(fs::FS &fs, const char * path)
{
  String fileContent = "";
  File file = fs.open(path, "r");
  if(!file || file.isDirectory())
  {
    return fileContent;
  }
  while(file.available())
  {
    fileContent+=String((char)file.read());
  }
  file.close();
  return fileContent;
}

static void writeFile(fs::FS &fs, const char * path, const char * message)
{
  File file = fs.open(path, "w");
  if(!file)
  {
    return;
  }
  file.print(message);
  file.close();
}

static void uploadFile(AsyncWebServerRequest *request, String filename, size_t index,
  uint8_t *data, size_t len, bool final) 
{
  if(!index)
  {
    request->_tempFile = LittleFS.open(filename, "w");
  }
  if(len)
  {
    request->_tempFile.write(data, len);
  }
  if(final)
  {
    request->_tempFile.close();
    request->redirect("/manager");
  }
}

static String kilobyteString(const size_t bytes)
{
  return String(bytes / 1024) + " kB";
}

static String processor(const String& var)
{

  if(var == "PROGRAM_INFO")
    return programInfo();

  if(var == "TITLE_TEXT") {
    #define BUFFER_SIZE 64
    static char buffer[BUFFER_SIZE];

    strlcpy(buffer, stg.hostName, BUFFER_SIZE - 3);
    strcat(buffer, " - ");
    strlcpy(buffer + strlen(buffer), stg.deviceName, BUFFER_SIZE - strlen(buffer));
    return buffer;
    #undef BUFFER_SIZE
  }

  if(var == "ALLOWED_EXTENSIONS_EDIT")
    return allowedExtensionsForEdit;

  if(var == "SYSTEM_FREE_BYTES")
    return kilobyteString((LittleFS.totalBytes() - LittleFS.usedBytes()));

  if(var == "SYSTEM_USED_BYTES")
    return kilobyteString(LittleFS.usedBytes());

  if(var == "SYSTEM_TOTAL_BYTES")
    return kilobyteString(LittleFS.totalBytes());
  
  if(var == "LISTEN_FILES")
    return listDir(LittleFS, "/", 0);

  if(var == "EDIT_FILES")
  {
    String editDropdown = "<select name=\"edit_path\" id=\"edit_path\">";
    editDropdown += "<option value=\"choose\">Select file to edit</option>";
    editDropdown += "<option value=\"new\">New text file</option>";
    editDropdown += filesDropdownOptions;      
    editDropdown += "</select>";
    return editDropdown;
  }
  
  if(var == "DELETE_FILES")
  {
    String deleteDropdown = "<select name=\"delete_path\" id=\"delete_path\">";
    deleteDropdown += "<option value=\"choose\">Select file to delete</option>";
    deleteDropdown += filesDropdownOptions;      
    deleteDropdown += "</select>";
    return deleteDropdown;
  }

  if(var == "TEXTAREA_CONTENT")
    return textareaContent;
  
  if(var == "SAVE_PATH_INPUT")
  {
    if(savePath == "/new.txt")
    {
      savePathInput = "<input type=\"text\" id=\"save_path\" name=\"save_path\" value=\"" +
        savePath + "\" >";
    }
    else
    {
      savePathInput = "";
    }
    return savePathInput;
  }
  return String();
}

void setupAsyncWebserver()
{
  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (serverDisabled()) {
      logi("Attempted access to disabled web server");
      return request->send(404, textPlain, pageNotFound);
    }
    request->send_P(200, "text/html", index_html, processor);
  });

  server.on("/manager", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    static minTimedOut logWaitTimedout;

    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    if (logWaitTimedout) {
      logWaitTimedout.reset(20); // don't want to log events every minute or two
      logi("Web interface manager accessed");
    }
    request->send_P(200, "text/html", manager_html, processor);
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    rebootRequest = !Update.hasError();
    logw(rebootRequest ?
      "Web interface program update and reboot" :
      "Web interface program update failed"
    );
    AsyncWebServerResponse *response = request->beginResponse(200, "text/html",
      rebootRequest ? update_ok_html : update_failed_html);

    response->addHeader("Connection", "close");
    request->send(response);
  },
  [](AsyncWebServerRequest *request, String filename, size_t index,
    uint8_t *data, size_t len, bool final)
  {
    if(!index)
	  {
      logi("Updating: %s", filename.c_str());

      if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000))
	    {
        Update.printError(Serial);
      }
    }
    if(!Update.hasError())
	  {
      if(Update.write(data, len) != len)
	    {
        Update.printError(Serial);
      }
    }
    if(final)
	  {
      if(Update.end(true))
	    {
        logi("The update is finished: %s", convertFileSize(index + len).c_str());
      }
	    else
	    {
        logi("The update failed");
        Update.printError(Serial);
      }
    }
  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) 
  {
    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    request->send(200);
  }, uploadFile);

  server.on("/edit", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    String inputMessage = request->getParam(param_edit_path)->value();
    if(inputMessage =="new")
    {
      textareaContent = "";
      savePath = "/new.txt";
    }
    else
    {
      savePath = inputMessage;
      textareaContent = readFile(LittleFS, inputMessage.c_str());
    }
    request->send_P(200, "text/html", edit_html, processor);
  });

  server.on("/save", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    String inputMessage = "";
    if (request->hasParam(param_edit_textarea)) 
    {
      inputMessage = request->getParam(param_edit_textarea)->value();
    }
    if (request->hasParam(param_save_path)) 
    {
      savePath = request->getParam(param_save_path)->value();
    }
    writeFile(LittleFS, savePath.c_str(), inputMessage.c_str());

    request->redirect("/manager");
  });

  server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request)
  {
    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    String inputMessage = request->getParam(param_delete_path)->value();
    if(inputMessage != "choose")
    {
      LittleFS.remove(inputMessage.c_str());
    }
    request->redirect("/manager");
  });

  server.on("/format", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    logw("Web interface reformat filesystem and reboot");
    LittleFS.format();
    rebootRequest = true;
    request->send(200);
  });

  server.on("/reboot", HTTP_POST, [](AsyncWebServerRequest *request)
  {
    if (authNeeded(request)) return request->requestAuthentication();
    if (serverDisabled()) return request->send(404, textPlain, pageNotFound);
    logw("Web interface user reboot");
    rebootRequest = true;
    request->send(200);
  });

  server.onNotFound(notFound);
  server.rewrite("/", "/index.html");
  server.begin();
}