REM batchfile for testing ASCOM ALPACA driver using CURL
echo "connect"
curl -X PUT -d"connected=true&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/connected 

echo "slewing"
curl -X PUT -d"azimuth=60&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/slewtoazimuth

echo "testing shutter"
curl -X PUT -d"ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/openshutter
curl -X PUT -d"altitude=45ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/slewtoaltitude
curl -X PUT -d"ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/closeshutter

echo "slew to home"
curl -X PUT -d"ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/park

echo "disconnect"
curl -X PUT -d"connected=false&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/0/connected

