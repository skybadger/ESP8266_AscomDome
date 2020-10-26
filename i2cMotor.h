#ifndef _i2cMotor_h_
#define _i2cMotor_h_

//*Include ASCOMDome enum for motor speed and direction settings.
enum motorSpeed: uint8_t     { MOTOR_SPEED_OFF=0, MOTOR_SPEED_SLOW_SLEW=130, MOTOR_SPEED_FAST_SLEW=240 };
enum motorDirection: uint8_t { MOTOR_DIRN_CW=0, MOTOR_DIRN_CCW=1 };


#pragma gcc warning "Wire i2c interface must be already initialised and globally defined as 'Wire' "   
#include <Wire.h>

int motorAddress = 0xB0 >> 1;
class i2cMotor 
{
   private:
   uint8_t _speed = 0;
   uint8_t _direction = (uint8_t)MOTOR_DIRN_CW;
   uint8_t _address = 0;

   TwoWire& tw = Wire;

   public:
   i2cMotor( uint8_t address ) : _address( address )
   {
      //
   }

   int getSpeed(void)
   {
    return _speed;
   }

   int getAddress(void)
   {
    return _address;
   }


   uint8_t getDirection(void)
   {
    return _direction;
   }

   void init( void )
   {
      //Write string to device
      uint8_t *outData = new byte[5];
      outData[0] = (uint8_t)0;   //register address to start writes to
      outData[1] = (uint8_t)0;   //motor mode - mode 0 is 0 (full reverse)  128 (stop)   255 (full forward).
      outData[2] = (uint8_t)128; //motor 1 is controlled by this speed register
      outData[3] = (uint8_t)128; //motor 2 is controlled by this speed register
      outData[4] = (uint8_t)255; //slowest acceleration
      
      tw.beginTransmission( _address ); // transmit to device
      tw.write( outData, 5 );  // sends array contents
      tw.endTransmission();    // stop transmitting
   }

   
   //Check motor speed control is present by reading register 7 for its version string. 
   //Motor speed control device is a Devasys i2c/serial/pwm 5A h-Bridge device from http://robot-electronics.co.uk
   bool check( void )
   {
      //Write string to device
      byte *outData = new byte[1];
      byte inData;
      bool readComplete = false;

      outData[0] = 7; //Version register address
      tw.beginTransmission( _address ); // transmit to device
      tw.write( outData, 1 );  // sends array contents
      tw.endTransmission(false);    // restart transmitting     
      
      tw.requestFrom( _address, (uint8_t) 1 );    // request 1 bytes from slave device
      while ( tw.available() && !readComplete ) 
      { // slave may send less than requested
         inData = tw.read(); // receive a byte as character
         readComplete = true;
      }
      return readComplete;
   }

   //Check motor speed control is present by reading register 7 for its version string. 
   //Motor speed control device is a Devasys i2c/serial/pwm 5A h-Bridge device from http://robot-electronics.co.uk
   bool getSpeedDirection( void )
   {
      //Write string to device
      byte *inData = new byte[3];
      byte* ptr = inData;
      bool readComplete = false;

      //outData[0] = 1; //Version register address
      tw.beginTransmission( _address ); // transmit to device
      tw.write( (uint8_t) 1 );  // sends address of register we want to read
      tw.endTransmission(false);    // restart transmitting           
      tw.requestFrom( _address, (uint8_t) 3 );    // request 1 bytes from slave device
      while ( tw.available() ) 
      { // slave may send less than requested
         *(ptr++) = tw.read(); // receive a byte as character
      }
      _speed = inData[1];
      if ( inData[0] < 128 ) 
        _direction = (uint8_t) MOTOR_DIRN_CW;
      else if ( inData[0] > 128 ) 
        _direction = (uint8_t) MOTOR_DIRN_CCW;
      return readComplete;
   }

   /*
   * Valid input values of speed are from 1 to 255;
   * Valid input values of direction are False - reverse, True - forwards 
   */
   bool setSpeedDirection( int newSpeed, uint8_t newDirn )
   {
      //Write string to device
      byte* outData = new byte[3];
      uint8_t speed = 128;
      
      //Convert speed and direction into simple velocity
      //motor mode register address - mode 2 is 0 (full reverse)  128 (stop)   255 (full forward).
      //set both speed registers - motor1 and motor2
      if (newSpeed == 0 )
      {
          speed = 128;
      }
      else 
      {
          speed = (uint8_t)( newSpeed/2 );
          if ( newDirn == MOTOR_DIRN_CW )
              speed = (uint8_t)( 128 - speed);
          else
              speed = (uint8_t)( 128 + speed);
      }
                      
      outData[0] = (uint8_t) 1;         // set start register;
      outData[1] = (uint8_t) speed;     // set speed value motor 1;
      outData[2] = (uint8_t) speed;     // set speed value motor 2;
      tw.beginTransmission( _address ); // transmit to device
      tw.write( outData, 3 );  // sends speed contents to register 1 
      int error = tw.endTransmission();    // stop transmitting
      _speed = speed;
      _direction = newDirn;
      return ( error == 0 );
   }



};
#endif
