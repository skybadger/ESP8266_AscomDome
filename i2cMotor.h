#ifndef _i2cMotor_h
#define _i2cMotor_h
/*Include ASCOMDome enum for motor speed and direction settings.
enum motorSpeed: uint8_t     { MOTOR_SPEED_OFF=0, MOTOR_SPEED_SLOW_SLEW=80, MOTOR_SPEED_FAST_SLEW=120 };
enum motorDirection: uint8_t { MOTOR_DIRN_CW=0, MOTOR_DIRN_CCW=1 };
*/
#pragma gcc warning "Wire i2c interface must be already initialised and globally defined as 'Wire' "   
#include <Wire.h>
class i2cMotor 
{
   private:
   enum _motorSpeed _speed = MOTOR_SPEED_OFF;
   enum _motorDirection _direction = MOTOR_DIRN_CW;
   uint8_t _address = 0;

   TwoWire& tw = Wire;

   public:

   i2cMotor( uint8_t address )
   {
      _address = address;
   }

   /*
   * Valid input values of speed are from 1 to 255;
   * Valid input values of direction are False - reverse, True - forwards 
   */
   bool SetSpeedDirection( enum motorSpeed speed, enum motorDirection dirn )
   {
      bool validCmd = false;
      int writeState = 0;
      
      //Write string to device
      byte* outData = new byte[2];
      
      //Convert speed and direction into simple velocity
      //motor mode register address - mode 1 is 0 (full reverse)  128 (stop)   255 (full forward).
      //set both speed registers - motor1 and motor2

      if (speed == MOTOR_SPEED_OFF )
      {
          speed = 128;
      }
      else 
      {
          speed = (byte)(speed/2 );
          if ( dirn = MOTOR_DIRN_CW )
              speed = (byte)( 128 - speed);
          else
              speed = (byte)( 128 + speed);
      }
                      
      outData[0] = (byte)speed; //speed value motor 1;
      outData[1] = (byte)speed; //speed value motor 2;
      tw.beginTransmission( _address | I2C_WRITE); // transmit to device
      tw.write( outData, 2 );  // sends array contents
      tw.endTransmission();    // stop transmitting
      _speed = speed;
      _dirn = dirn;
   }

   //Check motor speed control is present by reading register 7 for its version string. 
   //Motor soeed control device is a Devasys i2c/serial/pwm 5A h-Bridge device from http://robot-electronics.co.uk
   bool CheckMotorDevice( void )
   {
      //Write string to device
      byte *outData = new byte[1];
      bool outStatus = false; 
      bool readComplete = false;

      outData[0] = 7; //Version register address
      
      tw.requestFrom( (uint8_t) ( _address | I2C_READ), (uint8_t) 1 );    // request 1 bytes from slave device

      while ( tw.available() && !readComplete ) 
      { // slave may send less than requested
         inData[0] = tw.read(); // receive a byte as character
         readComplete = true;
      }
      return readComplete;
   }

   void InitMotorDevice( void )
   {
      //Write string to device
      int responseCount = 0;
      byte *outData = new byte[4];
      outData[0] = 0; //motor mode register address - mode 1 is 0 (full reverse)  128 (stop)   255 (full forward).
      outData[1] = 128; //both motors are controlled by their own speed registers, speed register motor 1
      outData[2] = 128; //speed register motor 2
      outData[3] = 255; //slowest acceleration
      
      tw.beginTransmission( _address | I2C_WRITE); // transmit to device
      tw.write( outData, 4 );  // sends array contents
      tw.endTransmission();    // stop transmitting
   }
};
#endif
