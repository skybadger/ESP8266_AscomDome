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
  uint8_t _address = 0xC6;
  uint8_t _proxyAddress = 0x00;
  uint8_t _rowSize = 4;
  uint8_t _colSize = 16;
  bool _backlight = false;
  enum cursorMode _cursorState = CURSOR_NONE;  
  TwoWire& _tw = Wire;
   
  public:
  I2CLCD( uint8_t address )
  {
     _address = address;
  }
  
  I2CLCD( uint8_t address, uint8_t rowSize, uint8_t colSize )
  {
     _address = address;
     _rowSize = rowSize;
     _colSize = colSize;
  }

  bool checkLCD()
  {
      byte inData[1];;
      bool readComplete = false;

      // request 1 bytes from slave device - returns the number of free bytes in the fifo buffer
      _tw.requestFrom( (uint8_t) ((_address & 0xFE) | I2C_READ), (uint8_t) 0 );   
      delay(25); 
      while ( _tw.available() && !readComplete ) 
      { // slave may send less than requested
         inData[0] = _tw.read(); // receive a byte as character
         readComplete = true;
      }
      return readComplete;
  }
        
  void writeLCD( int row, int col, String letters )
  {
      byte *outData;
      int length = 0;

      length = letters.length() + 1; 
      char *buf = new char[ length ];
      letters.toCharArray( buf, length, 0);
      //Serial.printf( "writeLCD %i letters:%s\n", length, buf );

      //Move to row/col
      outData = new byte[3];
      outData[0] = (byte) 3;    //CMD
      outData[1] = (byte)(row); //data
      outData[2] = (byte)(col); //data
      _tw.beginTransmission( ( _address & 0xFE )| I2C_WRITE); // transmit to device
      _tw.write( outData, 3 );     // sends array contents
      _tw.endTransmission(false);    // stop transmitting

      //Write string to device display
      _tw.beginTransmission( _address | I2C_WRITE ); // transmit to device
      _tw.write( (byte*) buf, length );     // sends array contents
      _tw.endTransmission();    // stop transmitting
  }

  void clearScreen()
  {
      _tw.beginTransmission( _address | I2C_WRITE ); // transmit to device
      _tw.write( (byte) 12 );   // CLS cmd
      _tw.endTransmission();    // stop transmitting
  }
  
  void setBacklight( bool state )
  {
      byte* outData;

      outData = new byte[ 1 ];
      //outData[1] = (byte) state;
      
      if( state) 
        outData[0] = (byte) 19; //on
      else
        outData[0] = (byte) 20; //off

      _tw.beginTransmission( ( _address & 0xFE) | I2C_WRITE ); // transmit to device
      _tw.write( outData , 1);     // backlight on/off command
      _tw.endTransmission();       // stop transmitting
  }

  /*
   * Function to set row and column and cursor type of LCD cursor. 
   * Row is 1-based
   * col is 1-based 
   * cursor: 4 - hidden
   * cursor: 5 - underlined
   * cursor: 6 - block
   */
  void setCursor(int row, int col, enum cursorMode mode )
  {
      byte* outData;

      //Move to row/col
      outData = new byte[3];
      outData[0] = (byte) 3;
      outData[1] = (byte) row;
      outData[2] = (byte) col; //data
      _tw.beginTransmission( _address | I2C_WRITE ); // transmit to device
      _tw.write( outData, 3 );   // cmd
      _tw.endTransmission();  // stop transmitting

      //Set cursor mode to solid, blink or underline
      outData = new byte[1];
      _cursorState = mode;
      switch ( mode )
      {
          case CURSOR_BLINK: 
              outData[0] = 0x04;
              break;
          case CURSOR_UNDERLINE: 
              outData[0] = 0x05;
              break;
          case CURSOR_SOLID: 
              outData[0] = 0x06;
              break;
          default: 
              outData[0] = 0x04;
              break;
      }
      _tw.beginTransmission( _address | I2C_WRITE ); // transmit to device
      _tw.write( outData, 1 );   // cmd
      _tw.endTransmission();  // stop transmitting
   }
};     
#endif //_header file
