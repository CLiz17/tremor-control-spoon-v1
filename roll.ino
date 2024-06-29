#include <Wire.h>
#include <MPU6050.h>
#include <Servo.h>

MPU6050 mpu;
Servo myServo;

// Calibration offsets
const float accX_offset = 236;
const float accY_offset = -368;
const float accZ_offset = 16384 - 15728;
const float gyroX_offset = 51;
const float gyroY_offset = -715;
const float gyroZ_offset = 89;

void setup()
{
    Serial.begin(9600);
    Wire.begin();
    mpu.initialize();

    if (mpu.testConnection())
    {
        Serial.println("MPU6050 connection successful");
    }
    else
    {
        Serial.println("MPU6050 connection failed");
    }

    myServo.attach(9);
    myServo.write(90); // Initialize servo to 90 degrees (horizontal)
}

void loop()
{
    int16_t ax, ay, az;
    int16_t gx, gy, gz;

    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    float acc_x = (ax - accX_offset) / 16384.0;
    float acc_y = (ay - accY_offset) / 16384.0;
    float acc_z = (az - accZ_offset) / 16384.0;

    // Calculate roll angle from accelerometer data
    float roll = atan2(acc_x, acc_z) * 180 / PI;

    // Map roll angle to servo position to keep the platform horizontal
    int servoAngle = 90 + roll;

    servoAngle = constrain(servoAngle, 0, 180);

    myServo.write(servoAngle);

    Serial.print("Roll Angle: ");
    Serial.print(roll);
    Serial.print("° -> Servo Angle: ");
    Serial.print(servoAngle);
    Serial.println("°");

    delay(100);
}