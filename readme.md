"ESP8266_SingleEncoder.ino
Incremental Encoder environment sensor over WiFi connection" 
This application runs on the ESP8266-02 wifi-enabled SoC device to capture encoder sensor readings and transmit them to the local MQTT service while also providing a restful interface for direct query using curl or http.
In my arrangement, Node-red flows are used to listen for and graph the updated readings in the dashboard UI. 
The unit is setup for 3 pin io operation ( A, B, Home) and is expecting to see A on GPIO0, B on GPIO2 and Home on GPIO3. 
. 

Dependencies:
Arduino 1.8+, 
ESP8266 V2.4+ 
Arduino MQTT client (https://pubsubclient.knolleary.net/api.html)
Arduino JSON library (pre v6) 
Arduino encoder library 

Testing
Access by serial port  - Tx only is available from device at 115,600 baud at 3.3v. THis provides debug output .
Wifi is used for MQTT reporting only 
ESP8266HttpServer is used to service  web requests
Use http://ESPENC01/encoder to receive json-formatted output of current encoder reading. 


Use:
I use mine to source a dashboard via hosting the mqtt server in node-red. It runs off a solar-panel supply in my observatory dome. 

ToDo:
