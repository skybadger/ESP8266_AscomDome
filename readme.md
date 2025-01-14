<h3>ESP8266_AscomDome.ino</h3>
<p> ASCOM ALPACA dome controller over WiFi connection </p
<p>This application runs on the ESP8266-02 wifi-enabled SoC device to implement the ASCOM ALPACA Dome device interface</p>

<p>The unit is setup for i2c io operation ( motor, LCD ) and is expecting to see SCL on GPIO0, SDA on GPIO2 and Tx on GPIO3. <br/>
The device communicates using rest calls to the remote shutter device for shutter status and to the remote encoder device for dome position information. <br/>
The dome position interface supports use of locally connected encoder, remote encoder (REST) or remote  magnetometer (REST) to obtain the dome rotation position readings. <br/>
The device supports use  of ESP8266-12 or ESP8266-01 devices. <br/>
In my arrangement, Node-red flows are used to listen for and graph the updated readings in the dashboard UI. These are prompted for by heartbeat messages in MQTT so this device listens for those heartbeat topics and responds by posting current data to the device-specific output topic.</p>
  
<h3>Dependencies</h3>
<ul>
  <li>Arduino 1.8+ </li>
  <li>ESP8266 V2.4+ </li>
<li>Arduino MQTT client (https://pubsubclient.knolleary.net/api.html) - used for report state to the Node-red device health monitor</li>
<li>Arduino JSON library (pre v6 - e.g. 5.13)  - used to parse and respond to REST queries. </li>
<li>ASCOM_COMMON library - see other repo <a href="http://www.github.com/skybadger/ASCOM_COMMON">here</a>. Used to create standard response templates and handle the ALPACA UDP management queries in a consistent way. </li>
<li>Dome shutter device https://github.com/skybadger/ESP8266_Shutter</li>
<li>Optional - Remote encoder device https://github.com/skybadger/ESP8266_SingleEncoder </li>
<li>Optional - remote compass device as found in https://github.com/skybadger/ESP8266_DomeSensors </li>
<li>Remote Debug library  - Used to access via telnet for remote debugging https://github.com/JoaoLopesF/RemoteDebug</li>
<li>EasyEEPROM arduino library modified to add a string read and write function because the templated types was not handling this well. Need to add this to the repo. </li> 
<li> linked list library : https://github.com/ivanseidel/LinkedList/archive/master.zip </li>
</ul>

<h3>Testing</h3>
<p>Access by serial port  - Tx only is available from device at 115,600 baud at 3.3v. This provides debug output .<br/>
Wifi is used for REST-ful web access <br/>
ESP8266HttpServer is used to service web requests on port 80 <br/>
UDP discovery of ALPACA is implemented but not fully working. <br/>
Use http://ESPdom01/setup to enter setup page for collection of devices. <br/>
Use http://api/v1/dome/0/setup to enter setup page for this device specifically. <br/>
</p>

<h3>Use</h3>
<p>use 'curl -X PUT -d"ClientID=99&ClientTransactionID=99&connected=true" http://espdom01/api/v1/dome/0/connected' to set the connected status for your connection to the dome. <br/>
</P>

<h3>ToDo </h3>
<p>There is a bug whereby the device will occassionally drop the connected status, probably caused by a reboot, which means the clients will need to re-connect to re-establish use of the dome. This happens up to several times a night and so far is proving hard to track down. 
  
Update: This has been tracked down to the wifi stack exceeding its buffer - this happens when it receives data faster than it processes it. Since the program design already returns asynchronously for everything, the only thing that can be improved is to make the TCP stack async and move the http request to full async processing - removing a potential delay of up to 200ms. I'll try.. 

Another potential source is when the dome gets locked stuck as occasionally happens, eventually the motor controller draws too much power and causes a brown-out on the 5v power line which is sourced from the 12v line. If your power setup is different, you won't have this problem. 
</p>
