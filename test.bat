REM batchfile for testing ASCOM ALPACA driver using CURL
echo "connect"
curl -X PUT -d"connected=true&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/connected 

echo "slewing"
curl -X PUT -d"azimuth=60&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/slewtoazimuth
pause



echo "testing shutter"
REM curl -X PUT -d"ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/openshutter
REM curl -X PUT -d"altitude=45ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/slewtoaltitude
REM curl -X PUT -d"ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/closeshutter

echo "slew to home"
curl -X PUT -d"ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/park
timeout /T 240

echo "slew to real park"
curl -X PUT -d"ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/park



curl -X PUT -d"azimuth=355&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/slewtoazimuth
timeout /T 240 

echo "disconnect"
curl -X PUT -d"connected=false&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/connected

