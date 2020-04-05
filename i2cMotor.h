#ifndef _i2cMotor_h_
#define _i2cMotor_h_
/*Include ASCOMDome enum for motor speed and direction settings.
enum motorSpeed: uint8_t     { MOTOR_SPEED_OFF=0, MOTOR_SPEED_SLOW_SLEW=80, MOTOR_SPEED_FAST_SLEW=120 };
enum motorDirection: uint8_t { MOTOR_DIRN_CW=0, MOTOR_DIRN_CCW=1 };
*/
#pragma gcc warning "Wire i2c interface must be already initialised and globally defined as 'Wire' "   
#include <Wire.h>
class i2cMotor 
{
   private:
   enum motorSpeed _speed  = MOTOR_SPEED_OFF;
   enum motorDirection _direction = MOTOR_DIRN_CW;
   uint8_t _address   = 0;

   TwoWire& tw = Wire;

   public:

   i2cMotor( uint8_t address )
   {
      _address = address;
   }

   enum motorSpeed getSpeed(void)
   {
    return _speed;
   }

   enum motorDirection getDirection(void)
   {
    return _direction;
   }

   /*
   * Valid input values of speed are from 1 to 255;
   * Valid input values of direction are False - reverse, True - forwards 
   */
   bool setSpeedDirection( enum motorSpeed newSpeed, enum motorDirection newDirn )
   {
      //Write string to device
      byte* outData = new byte[2];
      uint8_t speed;
      
      //Convert speed and direction into simple velocity
      //motor mode register address - mode 1 is 0 (full reverse)  128 (stop)   255 (full forward).
      //set both speed registers - motor1 and motor2

      if (newSpeed == MOTOR_SPEED_OFF )
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
                      
      outData[0] = (byte)speed; //speed value motor 1;
      outData[1] = (byte)speed; //speed value motor 2;
      tw.beginTransmission( ( _address & 0xFE ) | I2C_WRITE); // transmit to device
      tw.write( outData, 2 );  // sends array contents
      tw.endTransmission();    // stop transmitting
      _speed = newSpeed;
      _direction = newDirn;
      return true;
   }

   //Check motor speed control is present by reading register 7 for its version string. 
   //Motor speed control device is a Devasys i2c/serial/pwm 5A h-Bridge device from http://robot-electronics.co.uk
   bool check( void )
   {
      //Write string to device
      byte *outData = new byte[1];
      byte inData[1];
      bool readComplete = false;

      outData[0] = 7; //Version register address
      tw.beginTransmission( (_address & 0xFE) | I2C_WRITE); // transmit to device
      tw.write( outData, 1 );  // sends array contents
      tw.endTransmission(false);    // restart transmitting
      
      tw.requestFrom( (uint8_t) ( ( _address & 0xFE ) | I2C_READ), (uint8_t) 1 );    // request 1 bytes from slave device
      while ( tw.available() && !readComplete ) 
      { // slave may send less than requested
         inData[0] = tw.read(); // receive a byte as character
         readComplete = true;
      }
      return readComplete;
   }

   void init( void )
   {
      //Write string to device
      byte *outData = new byte[4];
      outData[0] = 0; //motor mode register address - mode 1 is 0 (full reverse)  128 (stop)   255 (full forward).
      outData[1] = 128; //both motors are controlled by their own speed registers, speed register motor 1
      outData[2] = 128; //speed register motor 2
      outData[3] = 255; //slowest acceleration
      
      tw.beginTransmission( ( _address & 0xFE ) | I2C_WRITE); // transmit to device
      tw.write( outData, 4 );  // sends array contents
      tw.endTransmission();    // stop transmitting
   }
};
#endif
