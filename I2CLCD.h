#ifndef _I2CLCD_h_
#define _I2CLCD_h_
#pragma gcc warning "Wire interface must be already initialised and globally defined as 'Wire' "   
#include <Wire.h> //Uses I2C interface
/*
 * 
 interface LCDDevice
 {
     bool checkLCD();
     bool clearScreen();
     bool setCursor(int row, int col, enum cursorMode mode);
     void writeLCD(int row, int col, String words);
     public void setBacklight( bool state );
 }
*/
/*
  Tech Specs: http://www.robot-electronics.co.uk/htm/Lcd05tech.htm
  OR http://www.robot-electronics.co.uk/htm/Lcd05tech.htm
*/

class I2CLCD {
  public:
  enum cursorMode : uint8_t { CURSOR_SOLID, CURSOR_BLINK, CURSOR_UNDERLINE, CURSOR_NONE };

  private:  
  uint8_t _address = (0xC6 >> 1); //7-bit addresses used in Wire. 
  uint8_t _proxyAddress = 0x00;
  uint8_t _rowSize = 4;
  uint8_t _colSize = 16;
  bool _backlight = false;
  enum cursorMode _cursorState = CURSOR_NONE;  
  TwoWire& _tw = Wire;
   
  public:
  I2CLCD( uint8_t address ) : _address(address)
  {
     
  }
  
  I2CLCD( uint8_t address, uint8_t rowSize=4, uint8_t colSize=16, TwoWire& tw = Wire ):
    _address( address),_rowSize( rowSize),_colSize( colSize ), _tw( tw )
  {
     
  }

  uint8_t checkLCD()
  {
      byte inData;
      bool readComplete = false;

      DEBUGS1(F("CheckLCD:: Using address ") );DEBUGSL1( _address );
      
      // request 1 bytes from slave device - returns the version id held in register 3
      _tw.beginTransmission( _address );   
      _tw.write(3);
      //_tw.write(15); //Only for serial port use
      _tw.endTransmission();
      
      delay(5);
      
      _tw.requestFrom( (int) _address, (int) 1 ); 
      while ( _tw.available() && !readComplete ) 
      { // slave may send less than requested
         inData = _tw.read(); // receive a byte as character
         readComplete = true;
      }
      
      DEBUGS1( F("CheckLCD:: written data - response code: "));DEBUGSL2( (int) inData, DEC );
      return (uint8_t) inData; //should be non-zero version number, otherwise not present.
  }
        
  int writeLCD( int row, int col, String letters )
  {
      byte *outData = nullptr;
      int length = 0;
      int errorCode = 0;
      byte* buf = nullptr;

      length = letters.length() + 1; 
      if ( ( buf = (byte*) calloc( sizeof(char), length ) ) == nullptr )
      {
        DEBUGSL1(F("WriteLCD:: calloc failed - quitting call."));
        return 4;
      }
      letters.toCharArray( (char*) buf, length );
      //strcpy( (char*) buf, letters.c_str() );
      //Serial.printf( "writeLCD %i letters: %s\n", length, buf );

      //Move to row/col
      outData = new byte[4];
      outData[0] = (byte) 0;    //register
      outData[1] = (byte) 3;    //CMD
      outData[2] = (byte)(row); //data
      outData[3] = (byte)(col); //data
      _tw.beginTransmission( _address ); // transmit to device
      _tw.write( outData, 4 );     // sends array content
      errorCode = _tw.endTransmission(false);    // stop transmitting
      //DEBUGS1("WriteLCD:: move cursor - response code: ");DEBUGSL1( errorCode );
      
      delay(2);
      
      //Write string to device display
      _tw.beginTransmission( _address );    // transmit to device
      _tw.write( (byte) 32 );               //Doing this cures an apparent bug where the first character isnlt otherwise printed. 
      for( int j = 0; j< length; j++ )
        _tw.write( (byte) buf[j] );         // sends array contents
      errorCode = _tw.endTransmission();    // stop transmitting
      //DEBUGS1("WriteLCD:: write string - response code: ");DEBUGSL1( errorCode );
      
      free(buf);
      return errorCode;
  }

  int clearScreen()
  {
      int errorCode = 0;
      //DEBUGS1("ClearScreen:: Using address ");DEBUGSL1( _address );
      _tw.beginTransmission( _address ); // transmit to device
      _tw.write( (byte) 0 );    // 0 register
      _tw.write( (byte) 12 );   // CLS cmd
      errorCode = _tw.endTransmission();    // Transmit queued content
      //DEBUGS1(F("ClearScreen:: written data - response code: "));DEBUGSL1( errorCode );
      return errorCode;
  }
  
  int setBacklight( bool state )
  {
      byte* outData;
      int errorCode = 0;
      outData = new byte[ 2 ];
      outData[0] = (byte) 0;
      
      if( state) 
        outData[1] = (byte) 19; //on
      else
        outData[1] = (byte) 20; //off

      _tw.beginTransmission( _address ); // transmit to device
      _tw.write( outData , 2);     // backlight on/off command
      errorCode = _tw.endTransmission();       // stop transmitting
      //DEBUGS1(F("ClearScreen:: written data - response code: "));DEBUGSL1( errorCode );
      return errorCode;
  }

  /*
   * Function to set row and column and cursor type of LCD cursor. 
   * Row is 1-based
   * col is 1-based 
   * cursor: 4 - hidden
   * cursor: 5 - underlined
   * cursor: 6 - block
   */
  int setCursor(int row, int col, enum cursorMode mode )
  {
      byte* outData;
      int errorCode = 0;

      //Move to row/col
      outData = new byte[4];
      outData[0] = (byte) 0;
      outData[1] = (byte) 3;
      outData[2] = (byte) row;
      outData[3] = (byte) col; //data
      _tw.beginTransmission( _address ); // transmit to device
      _tw.write( outData, 4 );   // cmd
      _tw.endTransmission();  // stop transmitting
      //DEBUGS1(F("SetCursor row/col:: written data - response code: "));DEBUGSL1( errorCode );

      //Set cursor mode to solid, blink or underline
      outData = new byte[2];
      outData[1] = 0;
      _cursorState = mode;
      switch ( mode )
      {
          case CURSOR_BLINK: 
              outData[1] = 0x04;
              break;
          case CURSOR_UNDERLINE: 
              outData[1] = 0x05;
              break;
          case CURSOR_SOLID: 
              outData[1] = 0x06;
              break;
          default: 
              outData[1] = 0x04;
              break;
      }
      _tw.beginTransmission( _address ); // transmit to device
      _tw.write( outData, 2 );   // cmd
      errorCode = _tw.endTransmission();  // stop transmitting
      //DEBUGS1(F("SetCursor type:: written data - response code: "));DEBUGSL1( errorCode );
      return errorCode;
   }
};     
#endif //_header file
