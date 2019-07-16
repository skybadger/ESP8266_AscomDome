#include <ESP8266WebServer.h>

//Class that extends ESPWebServer to :
//add case insensitive header/argument compares
//add case-insensitive path and wildcard expanded paths
class ESP8266WebServerEx:ESP8266WebServer
{
   //Everything else remains the same - its the case insensitivity we want to add. 
   
   // check if argument exists with differnt character case combinations
   //E.g. MikesArg vs mikesArg vs mikesarg
   //What is the argument is present in multiple different cases ?
   //How do you find which version of the cased argument you are getting ?
   String hasArg(const String& name, bool ignoreCase : false) const
   {
      int argFoundCount = 0;;
      int i=0;
      String output;
      
      if ( !ignoreCase && hasArg( name )  )
      {
         output = name;         
         //There can only ever be one if case sensitive.
         return output;
      }

      for ( i=0; i< _postArgsLen ; i++ )
      {
         if (  _postArgs[i].key.equalsIgnoreCase( name ) )
         {
            argFoundCount++;
            output = _currentArgs[i].key;            
         }
      }
      if( argFoundCount >= 1) 
         return output;

      for ( i=0; i< _currentArgsCount ; i++ )
      {
         if (  _currentArgs[i].key.equalsIgnoreCase( name ) )
         {
            argFoundCount++;
            output = _currentArgs[i].key;            
         }
      }
      if( argFoundCount >= 1) 
         return output;
      
   }
   
   
}