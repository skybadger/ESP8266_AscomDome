REM batchfile for testing ASCOM ALPACA driver using CURL
echo "connect"
curl -X PUT -d"connected=true&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/1/connected 

curl -v http://espdom01/api/v1/dome/1/CanFindHome?ClientID=99&ClientTransactionID=99 


echo "disconnect"
curl -X PUT -d"connected=false&ClientID=99&ClientTransactionID=99" http://espdom01/api/v1/dome/1/connected

