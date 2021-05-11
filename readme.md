<h3>ESP8266_AscomDome.ino</h3>
<p> ASCOM ALPACA dome controller over WiFi connection </p
<p>This application runs on the ESP8266-02 wifi-enabled SoC device to implement the ASCOM ALPACA Dome device interface</p>

<p>The unit is setup for i2c io operation ( motor, LCD ) and is expecting to see SCL on GPIO0, SDA on GPIO2 and Tx on GPIO3. <br/>
The device communicates using rest c\lls to the remote shutter device for shutter status and to the remote encoder device for doma position information. <br/>
The dome position interface supports use of locally encoder, remote encoder or remote  magnetometer to obtain the dome rotation positon readings. <br/>
The device supports use  of ESP8266-12 or ESP8266-01 devices. <br/>
In my arrangement, Node-red flows are used to listen for and graph the updated readings in the dashboard UI. SO this device listens for heartbeat topics every 60 seconds and responds to the device-specific heartbeat topic.</p>
  
<h3>Dependencies</h3>
<ul>
  <li>Arduino 1.8+ </li>
  <li>ESP8266 V2.4+ </li>
<li>Arduino MQTT client (https://pubsubclient.knolleary.net/api.html) - used for report state to the Node-red device health monitor</li>
<li>Arduino JSON library (pre v6)  - used to parse and respond to REST queries. </li>
<li>ASCOM_COMMON library - see other repo here. USed to create standard response templates and handle ALPACA UDP queries. </li>
<li>Remote Debug library  - Used to access via telnet for remote debugging</li>
</ul>


<h3>Testing</h3>
<p>Access by serial port  - Tx only is available from device at 115,600 baud at 3.3v. THis provides debug output .<br/>
Wifi is used for REST-ful web access <br/>
ESP8266HttpServer is used to service web requests on port 80 <br/>
UDP discovery of ALPACA is implemented but not fully working. <br/>
Use http://ESPdom01/setup to enter setup page for collection of devices. <br/>
Use http://api/v1/dome/0/setup to enter setup page for this device specifically. <br/>
</p>

<h3>Use</h3>
<p>use 'curl -X PUT -d"ClientID=99&ClientTransactionID=99&connected=true" http://espdom01/api/v1/dome/0/connected' to set the connected status for your connection to the dome. <br/>
</P>


<h3>ToD </h3>
<p>There is a bug whereby the device will occassionally drop the connected status, probably caused by a reboot, which means the clients will need to re-connect to re-establish use of the dome. This happens up to several times a night and so far is proving hard to track down. 
</p>
