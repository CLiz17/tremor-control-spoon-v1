#include <Wire.h>
#include <Servo.h>
#include <MPU6050.h>

MPU6050 mpu;
Servo servoMotor;

const float beta = 0.1;
const float dt = 0.01;
float q0 = 1.0, q1 = 0.0, q2 = 0.0, q3 = 0.0;
float targetAngle = 0.0;
float integralFBx = 0.0, integralFBy = 0.0, integralFBz = 0.0;

void setup()
{
    Wire.begin();
    mpu.initialize();
    servoMotor.attach(9);
    servoMotor.write(90); // Center the servo initially
}

void loop()
{
    int16_t ax, ay, az, gx, gy, gz;
    mpu.getMotion6(&ax, &ay, &az, &gx, &gy, &gz);

    // Convert raw data to SI units
    float ax_scaled = ax / 16384.0;
    float ay_scaled = ay / 16384.0;
    float az_scaled = az / 16384.0;
    float gx_scaled = gx / 131.0 * 0.0174533;
    float gy_scaled = gy / 131.0 * 0.0174533;
    float gz_scaled = gz / 131.0 * 0.0174533;

    // Madgwick filter algorithm
    float recipNorm;
    float s0, s1, s2, s3;
    float qDot1, qDot2, qDot3, qDot4;
    float halfvx, halfvy, halfvz, halfwx, halfwy, halfwz;
    float halfex, halfey, halfez;
    float qa, qb, qc;

    float q0q0 = q0 * q0;
    float q0q1 = q0 * q1;
    float q0q2 = q0 * q2;
    float q0q3 = q0 * q3;
    float q1q1 = q1 * q1;
    float q1q2 = q1 * q2;
    float q1q3 = q1 * q3;
    float q2q2 = q2 * q2;
    float q2q3 = q2 * q3;
    float q3q3 = q3 * q3;

    recipNorm = sqrt(ax_scaled * ax_scaled + ay_scaled * ay_scaled + az_scaled * az_scaled);
    ax_scaled /= recipNorm;
    ay_scaled /= recipNorm;
    az_scaled /= recipNorm;

    // Estimated direction of gravity and magnetic field
    halfvx = q1q3 - q0q2;
    halfvy = q0q1 + q2q3;
    halfvz = q0q0 - 0.5 + q3q3;

    halfex = ay_scaled * halfvz - az_scaled * halfvy;
    halfey = az_scaled * halfvx - ax_scaled * halfvz;
    halfez = ax_scaled * halfvy - ay_scaled * halfvx;

    if (beta > 0.0)
    {
        integralFBx += beta * halfex * dt;
        integralFBy += beta * halfey * dt;
        integralFBz += beta * halfez * dt;
        gx_scaled += integralFBx;
        gy_scaled += integralFBy;
        gz_scaled += integralFBz;
    }

    // Apply gyroscope measurements to quaternion differential equation
    qDot1 = 0.5 * (-q1 * gx_scaled - q2 * gy_scaled - q3 * gz_scaled);
    qDot2 = 0.5 * (q0 * gx_scaled + q2 * gz_scaled - q3 * gy_scaled);
    qDot3 = 0.5 * (q0 * gy_scaled - q1 * gz_scaled + q3 * gx_scaled);
    qDot4 = 0.5 * (q0 * gz_scaled + q1 * gy_scaled - q2 * gx_scaled);

    // Integrate quaternion rate and normalize
    q0 += qDot1 * dt;
    q1 += qDot2 * dt;
    q2 += qDot3 * dt;
    q3 += qDot4 * dt;
    recipNorm = 1.0 / sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    q0 *= recipNorm;
    q1 *= recipNorm;
    q2 *= recipNorm;
    q3 *= recipNorm;

    // Convert quaternion to Euler angles
    float rollAngle = atan2(2.0 * (q0 * q1 + q2 * q3), 1.0 - 2.0 * (q1 * q1 + q2 * q2)) * 180.0 / PI;

    float alpha = 0.99;
    float filteredAngle = (1.0 - alpha) * (filteredAngle + gx_scaled * dt) + alpha * rollAngle;

    int servoPosition = map(filteredAngle, -45, 45, 0, 180);
    servoPosition = constrain(servoPosition, 0, 180);
    servoMotor.write(servoPosition);
    delay(10);
}