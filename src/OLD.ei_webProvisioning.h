static const char PROGMEM webPgWiFiSetup[] = R"rawliteral(
<!-- wifiSetup.h version 1.0 6/30/2024 by Stephen McKeon -->

<!DOCTYPE HTML>
<html>
  <head>
    <style>
      /* Global Reset & Modern Dark Theme */
      body {
        font-family: -apple-system, BlinkMacSystemFont, "Segoe UI", Roboto, Helvetica, Arial, sans-serif;
        background-color: #121212;
        color: #e0e0e0;
        margin: 0;
        padding: 20px;
      }

      /* Clean Modern Tables (No Ugly Grid Lines) */
      table {
        width: 100%;
        border-collapse: collapse;
        margin-top: 15px;
        background-color: #242424;
        border-radius: 6px;
        overflow: hidden;
      }
      th, td {
        border: none;
        border-bottom: 1px solid #333;
        padding: 12px 14px;
        text-align: left;
      }
      th {
        background-color: #2a2a2a;
        color: #337ab7;
        font-weight: 600;
        text-transform: uppercase;
        font-size: 12px;
        letter-spacing: 0.5px;
      }

     /* Native Forms: Input Fields Styling */
      input[type="text"], input[type="password"], select {
        width: 100%;
        box-sizing: border-box;
        background-color: transparent; /* 💡 Melts the box into the row color */
        border: none;                  /* 💡 Deletes the default harsh input border box */
        border-bottom: 1px dashed #555; /* 💡 Adds a clean dashed line under the text to type on */
        color: #ffffff;
        padding: 6px 4px;
        font-size: 14px;
        font-family: monospace;
        outline: none;
      }
      input:focus, select:focus {
        border-bottom-color: #337ab7; /* 💡 Turns the underline blue when you click into it to type! */
      }

      /* Premium Solid Button Actions */
      .btn, .saveButton {
        display: block;
        width: 100%;
        max-width: 250px;
        height: 44px;
        background-color: #5cb85c;
        color: #ffffff;
        border: none;
        border-radius: 4px;
        font-size: 15px;
        font-weight: bold;
        text-transform: uppercase;
        letter-spacing: 0.5px;
        cursor: pointer;
        margin: 20px auto;
        transition: background-color 0.2s;
      }
      .btn:hover, .saveButton:hover {
        background-color: #4cae4c;
      }

      /* Headers & Typography */
      .header {
        text-align: center;
        color: #5cb85c;
        font-size: 24px;
        font-weight: bold;
        margin-bottom: 20px;
        text-transform: uppercase;
        letter-spacing: 0.5px;
      }
      .instructions {
        width: 100%;
        max-width: 750px;
        margin: 0 auto 25px auto;
        font-size: 14px;
        color: #aaaaaa;
        line-height: 1.6;
        text-align: left;
      }

      /* Container Panels (Replaces the blue borders with sleek charcoal cards) */
      .main, .main2, #main {
        display: block;
        width: 100%;
        max-width: 750px;
        margin: 25px auto;
        padding: 25px;
        box-sizing: border-box;
        background-color: #1c1c1c;
        border: 1px solid #333;
        border-radius: 8px;
        box-shadow: 0 4px 15px rgba(0,0,0,0.5);
        text-align: left;
      }
      .main2 { max-width: 500px; }
      #main { max-width: 500px; }

      /* Privacy Utility */
      .hidetext { -webkit-text-security: disc; }
      .hidden { display: none; }
       /* Clean Modern Dropdown and Custom Buttons in Upload Section */
       select, button, input[type="button"] {
         background-color: #2a2a2a;
         border: 1px solid #444;
         color: #ffffff;
         padding: 8px 12px;
         border-radius: 4px;
         font-size: 13px;
         cursor: pointer;
         transition: background-color 0.2s, border-color 0.2s;
       }
       select:hover, button:hover, input[type="button"]:hover {
         background-color: #333333;
         border-color: #5cb85c; /* Highlights with a subtle green border on hover! */
       }

       /* Style the specific Question Mark Help Button */
       /* Note: If your HTML uses a specific ID or class for the help button, adjust this selector */
       input[value="?"], button:contains("?") {
         background-color: #337ab7;
         color: white;
         border: none;
         font-weight: bold;
         border-radius: 4px;
         padding: 8px 14px;
         margin-right: 5px;
       }
       input[value="?"]:hover {
         background-color: #286090;
       }

       /* Modernize the native 'Choose File' upload button row */
       input[type="file"] {
         color: #aaaaaa;
         font-size: 13px;
         margin-top: 10px;
       }
       /* Styles the actual clickable button inside the browser file picker */
       input[type="file"]::file-selector-button {
         background-color: #2a2a2a;
         border: 1px solid #444;
         color: white;
         padding: 6px 12px;
         border-radius: 4px;
         cursor: pointer;
         margin-right: 10px;
         transition: background-color 0.2s;
       }
       input[type="file"]::file-selector-button:hover {
         background-color: #333;
         border-color: #337ab7;
       }
    </style>
  </head>
  <body>
    <div class="header" id="header1">
      <h1><span id='pgHeader'></span> initial setup</h1>
    </div>
    <div class="main2" id='getHTMLfile'>
      <table id='cfgTbl'>
        <tr>
          <td>WiFi SSID name-1</td>
          <td contenteditable id="ssidRow" onclick='selCellTxt(this)' onblur='captureCfgData(this)'>ENTER SSID NAME</td>
        </tr>
        <tr>
          <td>Wifi SSID password</td>
          <td class='hidetext' contenteditable id="ssidPwdRow"  onclick='selCellTxt(this)' onblur='captureCfgData(this)'>ENTER SSID password</td>
        </tr>

        <tr>
          <td>MQTT host IP address</td>
          <td contenteditable id="mqttHostRow" onclick='selCellTxt(this)' onblur='captureCfgData(this)'>ENTER HOST IP ADDRESS</td>
        </tr>
        <tr>
          <td>MQTT Server port #</td>
          <td contenteditable id="mqttPortNum" onclick='selCellTxt(this)' onblur='captureCfgData(this)'>ENTER MQTT SERVER PORT NUMBER</td>
        </tr>
        <tr>
          <td>MQTT broker user name</td>
          <td contenteditable id="mqttBrokerName" onclick='selCellTxt(this)' onblur='captureCfgData(this)'>ENTER MQTT BROKER</td>
        </tr>
        <tr>
          <td>MQTT broker password</td>
          <td class='hidetext' contenteditable id="mqttBrokerPwd" onclick='selCellTxt(this)' onblur='captureCfgData(this)'>ENTER MQTT BROKER PWD</td>
        </tr>

      </table>
      <br>
      <button class='btn saveButton' onclick='saveCfgData()' >Save</button>&emsp;&emsp;
    </div>
    <br><br>
    <div class="main" id='getHTMLfile' style=padding:left:50px;>
      Use this section to upload new/updated files to your controller.  The'Choose destination' menu must be filled
      in first.  Click on 'Choose destination and select the location you want the file to be placed in on the controller.
      After the destination  is set click on 'Choose file', select the file in the open file dialog that appears and select
      'Open'. This file will then be uploaded to the controller.<br><br>
      <b>&nbsp;&nbsp;&nbsp;Select .html,.csv,.css,or .js file to download</b>&emsp;&emsp;&emsp;
      <input type="button" id="dwnLdFileHelp" name="dwnLdFileHelp" onclick="dwnLdFileHelp(this)" value="?">&emsp;&emsp;&emsp;
      <input type="button" id="chooseDestination" name="chDest" onclick="doChooseDest(this)" value="Choose destination">&emsp;<scan id="dnLdLoc"></scan><br>
      <input type="file" name="inputfile" id="inputfile" style='margin-top:5px; onchange='readSingleFile()' accept=".html,.csv,.css,.js">
      <scan id='dwnLoadResults' style='color:blue;'></scan>
      <br>
    </div>
    <div id='dirTblBox' class='hidden' style = 'background-color: #15F3FC;'>
      <table id='dirTbl'>
        <tr>
          <th><b></b></th>
        </tr>
      </table>
    </div>


    <script>
      var gateway = `ws://${window.location.hostname}/ws`;
      let websocket;
      let srvIpAddr = "";                                       // what is the servers ip address
      let myIpAddr = "";                                        // what is this web pages ip address
      let myUUID = create_UUID();                               // client UUID, stable until the page is refreshed
      let ssid = "";
      let ssidPwd = "";
      let MQTT_serverIP = "";
      let MQTT_port = "";
      let MQTT_brokerUsr = "";
      let MQTT_BrokerPwd = "";
      let dirTblSelRow = 0;                                                     // currently selected directory table row
      let dirTblBxBkGndColor = '#15F3FC';                                       // background color for the 'dirTblBox'
      let dirList = "DIR : /,DIR : /var,DIR : /var/log,DIR : /html,DIR : /temp,DIR : /appData,DIR : /dirLists,"

      window.addEventListener('load', onLoad);
      document.getElementById('inputfile').addEventListener('change', readSingleFile);

      function create_UUID(){
        var dt = new Date().getTime();
        var uuid = 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, function(c) {
            var r = (dt + Math.random()*16)%16 | 0;
            dt = Math.floor(dt/16);
            return (c=='x' ? r :(r&0x3|0x8)).toString(16);
        });
        return uuid;
      }

      // 2. Unpack the JSON string straight into your HTML text input boxes
      function initDataFieldsJSON(jsonStr) {
        try {
          let config = JSON.parse(jsonStr);
          let tbl = document.getElementById('cfgTbl'); 
          
          tbl.rows[0].cells[1].innerText = config.ssid       || "";
          tbl.rows[1].cells[1].innerText = config.pass       || "";
          tbl.rows[2].cells[1].innerText = config.mqttServer || "";
          tbl.rows[3].cells[1].innerText = config.mqttPort   || 1883;
          tbl.rows[4].cells[1].innerText = config.mqttUser   || "";
          tbl.rows[5].cells[1].innerText = config.mqttPass   || "";
          
          ssid           = tbl.rows[0].cells[1].innerText;
          ssidPwd        = tbl.rows[1].cells[1].innerText;
          MQTT_serverIP  = tbl.rows[2].cells[1].innerText;
          MQTT_port      = tbl.rows[3].cells[1].innerText;
          MQTT_brokerUsr = tbl.rows[4].cells[1].innerText;
          MQTT_BrokerPwd = tbl.rows[5].cells[1].innerText;
          
          console.log("Webpage fields successfully populated via clean JSON payload!");
        } catch(e) {
          console.error("Failed to parse incoming initialization JSON:", e);
        }
      }

      function captureCfgData(cell) {
        console.log("User exited field ID: '" + cell.id + "'");                                 
        
        // 🧼 Isolates pure clean characters, stripping out hidden spans/divs instantly
        let cellText = cell.textContent.trim();                                          
        
        // 🎯 DYNAMIC OVERRIDE: Switch directly on your element's explicit HTML ID name!
        // No loops, no content-guessing, completely immune to HTML tag contamination!
        switch(cell.id) {                                                       
          case "ssidRow":
            ssid = cellText;
            break;
          case "ssidPwdRow":
            ssidPwd = cellText;
            break;
          case "mqttHostRow":
            MQTT_serverIP = cellText;
            break;
          case "mqttPortNum":
            MQTT_port = cellText;
            break;
          case "mqttBrokerName":
            MQTT_brokerUsr = cellText;
            break;
          case "mqttBrokerPwd":
            MQTT_BrokerPwd = cellText;
            break;
          default:
            console.log("An unrecognized cell ID blurred out: " + cell.id);
        }
      }
      function selCellTxt(theCell) {
        document.execCommand("selectall", null, false);
      }

     function saveCfgData() {
        let configData = {
          networkSsid: ssid.trim(),
          networkPass: ssidPwd.trim(),
          mqttServer: MQTT_serverIP.trim(),
          mqttPort: parseInt(MQTT_port),
          mqttUser: MQTT_brokerUsr.trim(),
          mqttPass: MQTT_BrokerPwd.trim()
        };

        // Send a clean, labeled JSON payload over the socket line
        sendText("cfgDataJSON:" + JSON.stringify(configData));
        console.log("Cfg data saved via modern JSON... In progress");
      }

      function doHdlDirList(s) {
        count = s.match(/,/g);
        let arr = s.split(',');
        let theResults = new Array();
        for(let i = 0; i < arr.length; i++) {
          if(arr[i].indexOf('DIR :') != -1) {
            theResults.push(arr[i]);
          }
        }
        return theResults
      }

      function doChooseDest(elem) {
        document.getElementById('inputfile').disabled = true;                   // disable the choose file button
        let btn = document.getElementById('chooseDestination');                 // get the choose distination button element
        let tblBx = document.getElementById('dirTblBox');                       // get the dir table box element
        tblBx.style.position = 'absolute';                                      // set the 'dirTblBox' to have an absolute placement
        tblBx.style.top = "-500px";                                             // set it off screen
        tblBx.style.left = -"500px";                                            // set it off screen
        tblBx.classList.remove('hidden');                                       // remove the 'hidden' class from it
        document.getElementById('dirTblBox').hidden = false;                                              // show the element
        let bxTop = (btn.getBoundingClientRect().bottom - tblBx.getBoundingClientRect().height) + 'px';   // get the coordinates for it's top
        let bxLeft = btn.getBoundingClientRect().left + 'px';                                             // and it's left
        tblBx.style.top = bxTop;                                                // move it to it's top
        tblBx.style.left = bxLeft;                                              // and its left
      }
/*
      async function getDirData(fName) {
        const response = await fetch(fName)                                     // initiate the data fetch
         .then(response => response.text())                                     // looking for a text file
         .then(result => {                                                      // when the result is complete
           processDirArray(result)                                              // process the incoming directory information
           sendText('Directory download complete');                             // tell the server the download is complete
         })
         .catch(error => {                                                      // if an error occured
           console.error('Error:', error);                                      // write it to the error log
         });
      }
*/
      function dwnLdFileHelp(elem) {
        alert('This section is to allow for the download of updated HTML, CSV, CSS, or JS (javascript)files' +
                    ' Choose the file system you want to upload to.  This will ebe either the SD or LittleFS systems.' +
                    ' The SD system are where all the data files are and the Littlefs system is where all the HTML' +
                    ' page code files are.  This, if you are uploading a new version of the code for one of the web pages' +
                    ' you would choose the LittleFS file system.  if, for example, you were uploading an update of the' +
                    ' timezones file (tzs.html) you would choose the SD file system.  Normally when update are necessary' +
                    ' you should receive instructions telling you the steps you need to take.');
      }

      function setBtnHilite(elem, selected) {
        if(selected) {
          elem.style.backgroundColor = 'blue';
          elem.style.color = 'white';
        } else {
          elem.style.backgroundColor = '';
          elem.style.color = '';
        }
      }

      function swapBtnHiLite(elemId1, elemId2) {                                // hilites btn elemId1 and unhilites btn elem2
        theBtn1 = document.getElementById(elemId1);
        theBtn2 = document.getElementById(elemId2);
        setBtnHilite(theBtn1, true);
        theBtn1.checked = true;
        setBtnHilite(theBtn2, false);
        theBtn2.checked = false;
      /*    theBtn1.style.backgroundColor = 'blue';
          theBtn1.style.color = 'white';
          theBtn1.checked = true;
          theBtn2.style.backgroundColor = '';
          theBtn2.style.color = '';
          theBtn2.checked = false;
      */
      }

      function readSingleFile() {
        document.getElementById('dwnLoadResults').innerHTML = '';               // empty out the last success message
        var file = document.getElementById('inputfile').files[0];               // get the file the user choose
        if (!file) {                                                            // if file is empty then
          return;                                                               // abort by returning
        }
        const reader = new FileReader();                                        // create a new file reader
        reader.onload = function(e) {                                           // set up a function for the file reader to use
          var contents = e.target.result;                                       // put the contents of the file into contents
          uploadFile(contents);                                                 // upload the contents to the server
        };
        reader.readAsText(file);                                                // read the file in as a text file
      }

      async function uploadFile(contents) {
        const fu1 = document.getElementById("inputfile").files[0];              //creating form data object and append file into that form data
        let formData = new FormData();                                          // create a new form data
        formData.append('username','/html');                                    //
        formData.append(inputfile,fu1);                                         // append the file to be transfered to the form
        const response = await fetch('/post' , {
          method: 'POST',
          body: formData
        })
        myConsoleLog("File size for '" + fu1.name + "' is '" + contents.length + "'.");
        let msg="&nbsp;&nbsp;Download of '" + fu1.name + "' is complete - return code:'" + response.statusText + "'"; // assemble the success message
        document.getElementById('dwnLoadResults').innerHTML = msg;              // write the success response to the web page
        document.getElementById("inputfile").value = "";                        // set the 'inputfile' element value to an empty strng so it will notice a chnge of the same file is selected for down load
        myConsoleLog(response);                                                 // when this is complete log the response to the console
        }

      function getTzOffset(minutes) {
        let h = Math.floor(minutes / 60);                                       // get the number of hours
        let m = minutes % 60;                                                   // get the bumber of minutes
        if(h > 0) {                                                             // if h is less than zero
          h = '-' + h;                                                          // put a '-' sign in from of the number
        } else h = '+' + h;                                                     // else put a '+' sing n front if it
        if(h.length < 3) {                                                      // if h is less than 3 chars
          h = h.substring(0, 1) + '0' + h.substring(1);                         // pad it with a zero
        }
        if(m < 10) m = '0' + m;                                                 // get the number of minutes
        h = h + m;                                                              // add a ':' and the minutes to the hour
        return h                                                                // and return it
      }

      function updateProgressBar(percentDone) {
        let x = document.getElementById('fDwnload');

      }

      function processDirArray(s) {
        let arr = s.split(",")
        arr.sort();                                                             // sort the array
        arr.reverse();                                                          // reverse sort the array
        let a = new Array();
        myConsoleLog("Processing directory listing");                           // log what is happening
//        console.log("Processing directory listing");                          // log what is happening
        for(let i = 0; i < arr.length; i++) {                                   // for each item in the array remove everything except the directory name
          if(arr[i].indexOf("DIR") != -1) {                                     // if 'DIR' is in the string then
            arr[i] = arr[i].substring(arr[i].indexOf('DIR : ')+6);              // get rid of everything from the ': ' to the beginning of the string
            arr[i] = arr[i].trim();                                             // trim any spaces off the beginning and end of the the string a[i]
            a.push(arr[i]);                                                     // add the item to the end of the array 'a'
          }
        }
        let tbl = document.getElementById('dirTbl');                            // get the directory table object
        if(tbl.rows.length > 0) {                                               // if there are any rows in the table
          for(let i = tbl.rows.length; i > 0; i--) {                            // starting at the last row
            tbl.deleteRow(i-1);                                                 // delete the table row
          }
        }
        /* there should only be an empty table now...*/
        let cell;
        for(let i = 0;i < a.length; i++) {                                      // for each item in the array 'a'
          row = tbl.insertRow(i);                                               // insert a row
          row.setAttribute('id', 'dirTblR'+i);                                  // and set it's id to 'dirTbl' plus the value of i
          row.style.cursor = 'cell';                                            // set the row cursor to 'cell'
          cell = tbl.rows[i].insertCell(0);                                     // create cell one, index 0
          cell.innerHTML = a[i];                                                // set the contents of cell one to arra a[i] contents
          row.setAttribute('onclick', 'hdlDirTblRowClick(this)');               // set the rows onclick attribute
        }
        document.getElementById('chooseDestination').disabled = false;           // disable the choose destination button
      }

      async function hdlDirTblRowClick(x) {                                     // user is selecting directory to send file to
        unHiLiteDirTblRow(dirTblSelRow);                                        // unhilite the existimg hilited row
        hiLiteDirTblRow(x.rowIndex);                                            // hilite the clicked on row
        selUploadLoc = x.cells[0].innerHTML                                     // get the text out of the row the user clicked on and save it to 'selUploadLoc'
        document.getElementById('dirTblBox').hidden = true;                     // hide the element
        sendText('downloadLocation:' + selUploadLoc);                           // send the download location to the server
      }

      function unHiLiteDirTblRow(rowNum) {
          dirTbl = document.getElementById('dirTbl');
          dirTbl.rows[rowNum].style.backgroundColor = dirTblBxBkGndColor;
          dirTbl.rows[rowNum].style.color = 'black';
      }

      function hiLiteDirTblRow(rowNum) {
        dirTbl = document.getElementById('dirTbl');
        dirTbl.rows[rowNum].style.backgroundColor = 'blue';
        dirTbl.rows[rowNum].style.color = 'white';
        dirTblSelRow = rowNum;
      }

      function onMessage(event) {
        let s = event.data;                                                     // convert event.data to a string
        console.log("received webSocket event: " + s);
        if(s.indexOf('whoAreYou') != -1)  {                                     // if this is asking who you are
          sendText('iAmAWiFiSetUpPg:' + myUUID);                                // reply 'iAmAWiFiPage' and send the pages UUID
          return;                                                               // just in case the string contains other match
        }
        if(s.indexOf('DownLoadLocRec:') != -1) {                                // if this is a download location receipt
          document.getElementById('inputfile').disabled = false;                // enable the choose file button
          document.getElementById('dnLdLoc').innerText = s.substring(15);       // write the download location to the page

        }
        if(s.indexOf('serverip:') != -1) {                                      // if this is the server ip coming in
          s = s.substring(9);                                                   // strip 'serverip:' off the string
          srvIpAddr = s;                                                        // put the incoming ip address into the srvIpAddr var
          return;                                                               // just in case the string contains other matches
        }
        if(s.indexOf('listDir:') != -1)  {                                      // if this is a list of directories and files
//          processDirArray(doHdlDirList(s.substring(8)));                        // call doHdlDirList() and pass 's' less the header to it and then put the array into a table
          processDirArray(s.substring(8));                                      // call doHdlDirList() and pass 's' less the header to it and then put the array into a table
          return;                                                               // just in case the string contains other match
        }
        if(s.indexOf('clientip:') != -1) {                                      // if this is this pages ip address coming in
          s = s.substring(9);                                                   // strip 'clientip' off the string
          myIpAddr = s;                                                         // put the incoming ip address into the myIpAddr var
          return;                                                               // just in case the string contains other matches
        }
        if(s.indexOf('pgHeader:') != -1) {                                      // if this is this pages header coming in
          s = s.substring(s.indexOf(':')+1);                                    // strip 'pgHeader:' off the string
          document.getElementById('pgHeader').innerHTML = s                     // and post to page
          return;                                                               // just in case the string contains other matches
        }
        if(s.indexOf('initWebDataJSON:') !== -1) {                                     
          let rawJsonText = s.substring(s.indexOf(':') + 1);
          initDataFieldsJSON(rawJsonText);
          return;                                                               
        }

      }

       function myConsoleLog(s) {
        let tzo = new Date().getTimezoneOffset();                                                         // get the timezone offset in minutes
        let tzoffset = (tzo * 60000);                                                                     // timezone offset in milliseconds
        let zone = new Date().toLocaleTimeString('en-us',{timeZoneName:'short'}).split(' ')[2];
        console.log((new Date(Date.now() - tzoffset)).toISOString().slice(0, -1) + '-' + zone +  " " + s);
      }

      function sendText(data) {                                 // send text to the server
        let status = websocket.readyState;                      // is the websocket sys up and runnimg
        if(websocket.readyState != 1)  {                        // if it is not ready
          setTimeout(() => {  websocket.send(data); }, 10000);  // set a 10 second timeout
        } else websocket.send("wifisetup:" + data);             // else send the text to the server
      }

      function initWebSocket() {
        myConsoleLog("Starting 'initWebSocket'.");
        document.getElementById('inputfile').disabled = true;                   // disable the choose file button
        document.getElementById('chooseDestination').disabled = true;           // disable the choose destination button
        console.log('Trying to open a WebSocket connection...'); // log initWebSocket has been called
        websocket = new WebSocket(gateway);
        websocket.onopen    = onOpen;
        websocket.onclose   = onClose;
        websocket.onmessage = onMessage; // <-- add this line
      }

      function onOpen(event) {
        console.log("Opening connection to server.");
//        console.log('Connection opened to :' + srvIpAddr);
      }

      function onClose(event) {
        console.log('Connection closed');
        setTimeout(initWebSocket, 2000);
      }

      function onLoad(event) {
        console.log("In onLoad(event)");
        initWebSocket();
      }

    </script>
  </body>
</html>
)rawliteral";
