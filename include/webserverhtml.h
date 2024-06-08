// webserverhtml.h
/*
Credit: Much of this code is from an anonymous author. References to that source:
https://myhomethings.eu/en/esp32-web-updater-and-spiffs-manager/
https://www.hackster.io/myhomethings/esp32-web-updater-and-spiffs-file-manager-cf8dc5
https://hackaday.com/2023/02/17/esp32-web-updater-allows-file-system-management-and-ota-updates/

The original code has been modified.
*/

inline const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
  <head>
    <title>%TITLE_TEXT% - Home</title>
    <meta charset="UTF-8">
    <style>
      .content {
        max-width: 550px;
        margin: auto;
        padding: 30px;
      }
      h2 {text-align: center;}
      h3 {text-align: center;}
    </style>
  </head>
  <body>
  <div class="content">
    <h3><a href="/manager">( Settings/File Manager )</a></h3>
    <p></p>
    <h2>%TITLE_TEXT% - Home</h2>
    <hr>
    <pre>%PROGRAM_INFO%</pre>
    <hr>
    <h3><a href="/manager">( Settings/File Manager )</a></h3>
  </div>
  </body>
</html>)rawliteral";

inline const char manager_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>%TITLE_TEXT% - Manager</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #submit {
        width:120px;
      }
      #edit_path {
        width:250px;
      }
      #delete_path {
        width:250px;
      }
      #spacer_50 {
        height: 50px;
      }
      #spacer_20 {
        height: 20px;
      }
      table {
        background-color: #e0e0e0;
        border-collapse: collapse;
        width:550px;
      }
      td, th {
        border: 1px solid #e0e0e0;
        text-align: left;
        font-weight: normal;
        padding: 8px;
      }
      #first_td_th {
        width:400px;
      }
      tr:nth-child(even) {
        background-color: #ffffff;
      }
      fieldset {
        width:570px;
        background-color: #f7f7f7;
      }
      #format_notice {
        color: #ff0000;
        font-weight: bold;
      }
    </style>
    <script>
      function validateFormUpdate()
      {
        var inputElement = document.getElementById('update');
        var files = inputElement.files;
        if(files.length==0)
        {
          alert("You have not chosen a file!");
          return false;
        }
        var value = inputElement.value;
        var dotIndex = value.lastIndexOf(".")+1;
        var valueExtension = value.substring(dotIndex);
        if(valueExtension != "bin")
        {
          alert("Incorrect file type!");
          return false;
        }
      }
      function validateFormUpload()
      {
        var inputElement = document.getElementById('upload_data');
        var files = inputElement.files;
        if(files.length==0)
        {
          alert("You have not chosen a file!");
          return false;
        }
      }
      function validateFormEdit()
      {
        var allowedExtensions = "%ALLOWED_EXTENSIONS_EDIT%";
        var editSelectValue = document.getElementById('edit_path').value;
        var dotIndex = editSelectValue.lastIndexOf(".")+1;
        var editSelectValueExtension = editSelectValue.substring(dotIndex);
        var extIndex = allowedExtensions.indexOf(editSelectValueExtension);

        if(editSelectValue == "new") { return true; }
        if(editSelectValue == "choose") { alert("You have not chosen a file!"); return false; }
        if(extIndex == -1) { alert("Editing of this file type is not supported!"); return false; }
      }
      function validateFormDelete()
      {
        var deleteSelectValue = document.getElementById('delete_path').value;
        if(deleteSelectValue == "choose" ) { alert("You have not chosen a file!"); return false; }
      }
      function confirmFormat()
      {
        var text = "Pressing the \"OK\" button immediately deletes all data from the filesystem and restarts the device!";
        if (confirm(text) == true) { return true; } else { return false; }
      }
      function confirmReboot()
      {
        var text = "Pressing the \"OK\" button will immediately restart the device!";
        if (confirm(text) == true) { return true; } else { return false; }
      }
    </script>
  </head>
  <body>
    <center>
      <h3><a href="/">( Home Page )</a></h3>
      <p></p>
      <h2>System</h2>

      <fieldset>
        <legend>Firmware Update (takes 20-50 seconds)</legend>
          <div id="spacer_20"></div>
          <form method="POST" action="/update" enctype="multipart/form-data">
            <table><tr><td id="first_td_th">
            <input type="file" id="update" name="update">
            </td><td>
            <input type="submit" id="submit" value="Update" onclick="return validateFormUpdate()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>

      <fieldset>
        <legend>Reboot the System</legend>
          <div id="spacer_20"></div>
          <form method="POST" action="/reboot" target="self_page">
            <table><tr><td id="first_td_th">
            <div>WARNING: This will abruptly restart the device!</div>
            </td><td>
            <input type="submit" id="submit" value="Reboot" onclick="return confirmReboot()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>

      <h2>Filesystem Manager</h2>

      <fieldset>
        <legend>File list</legend>
          <p>Filesystem Storage: %SYSTEM_TOTAL_BYTES% total, %SYSTEM_USED_BYTES% used, %SYSTEM_FREE_BYTES% available</p>
          %LISTEN_FILES%
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>

      <fieldset>
        <legend>Edit file</legend>
          <div id="spacer_20"></div>
          <form method="GET" action="/edit">
            <table><tr><td id="first_td_th">
            %EDIT_FILES%
            </td><td>
            <input type="submit" id="submit" value="Edit" onclick="return validateFormEdit()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>

      <fieldset>
        <legend>Upload file</legend>
          <div id="spacer_20"></div>
          <form method="POST" action="/upload" enctype="multipart/form-data">
            <table><tr><td id="first_td_th">
            <input type="file" id="upload_data" name="upload_data">
            </td><td>
            <input type="submit" id="submit" value="Upload" onclick="return validateFormUpload()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>

      <fieldset>
        <legend>Delete file</legend>
          <div id="spacer_20"></div>
          <form method="GET" action="/delete">
            <table><tr><td id="first_td_th">
            %DELETE_FILES%
            </td><td>
            <input type="submit" id="submit" value="Delete" onclick="return validateFormDelete()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>

      <fieldset>
        <legend>Format the Filesystem</legend>
          <div id="spacer_20"></div>
          <form method="POST" action="/format" target="self_page">
            <table><tr><td id="first_td_th">
            <p id="format_notice">WARNING: This will delete all data from the filesystem!</p>
            </td><td>
            <input type="submit" id="submit" value="Format" onclick="return confirmFormat()">
            </td></tr></table>
          </form>
          <div id="spacer_20"></div>
      </fieldset>

      <div id="spacer_20"></div>

      <iframe style="display:none" name="self_page"></iframe>
      <h3><a href="/">( Home Page )</a></h3>
    </center>
  </body>
</html>)rawliteral";

inline const char edit_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>%TITLE_TEXT% - Edit file</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #submit {
        width:120px;
      }
      #spacer_50 {
        height: 50px;
      }
      #spacer_20 {
        height: 20px;
      }
      fieldset {
        width:800px;
        background-color: #f7f7f7;
      }
      table {
        background-color: #dddddd;
      }
      td, th {
        text-align: center;
        padding: 15px;
      }
      textarea {
        width: 700px;
        height: 500px;
        padding: 12px 20px;
        box-sizing: border-box;
        border: 2px solid #ccc;
        border-radius: 4px;
        resize: none;
      }
    </style>
    <script>
      function validateForm()
      {
        var allowedExtensions = "%ALLOWED_EXTENSIONS_EDIT%";
        var inputMessage = document.getElementById('save_path').value;
        var dotIndex = inputMessage.lastIndexOf(".")+1;
        var inputMessageExtension = inputMessage.substring(dotIndex);
        var extIndex = allowedExtensions.indexOf(inputMessageExtension);
        var isSlash = inputMessage.substring(0,1);

        if(inputMessage == "")
          { alert("Enter the file name! \ne.g.: /new.txt"); return false; }
        if(isSlash != "/")
          { alert("The slash at the beginning of the file is missing!"); return false; }
        if(dotIndex == 0)
          { alert("The extension is missing at the end of the file!"); return false; }
        if(inputMessageExtension == "")
          { alert("The extension is missing at the end of the file!"); return false; }
        if(extIndex == -1)
          { alert("Extension not supported!"); return false; }
      }
    </script>
  </head>
  <body>
    <center>
      <h2>Edit file</h2>
      <div id="spacer_20"></div>

      <fieldset>
        <legend>Edit text file</legend>
        <div id="spacer_20"></div>
        <table><tr><td colspan="2">
        <form name="edit_file" action="/save" onsubmit="return validateForm()">
          <textarea name="edit_textarea">%TEXTAREA_CONTENT%</textarea>
          <div id="spacer_20"></div>
        </td></tr><tr><td>
          %SAVE_PATH_INPUT%
          <button type "submit" id="submit" >Save</button>
        </form>
        </td><td>
        <button id="submit" onclick="window.location.href='/manager';">Cancel</button>
        </td></tr></table>
        <div id="spacer_50"></div>
      </fieldset>

      <iframe style="display:none" name="self_page"></iframe>
    </center>
  </body>
</html>)rawliteral";

inline const char update_ok_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Update successful</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_50 {
        height: 50px;
      }
    </style>
  </head>
  <body>
    <center>
      <h2>The update was successful.</h2>
      <div id="spacer_50"></div>
      <button onclick="window.location.href='/manager';">Return to Page</button>
    </center>
  </body>
</html>)rawliteral";

inline const char update_failed_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML>
<html>
  <head>
    <title>Update unsuccessful</title>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
      body {
        background-color: #f7f7f7;
      }
      #spacer_50 {
        height: 50px;
      }
    </style>
  </head>
  <body>
    <center>
      <h2>The update has failed.</h2>
      <div id="spacer_50"></div>
      <button onclick="window.location.href='/manager';">Return to Page</button>
    </center>
  </body>
</html>)rawliteral";