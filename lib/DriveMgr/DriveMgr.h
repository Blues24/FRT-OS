#ifndef DRIVEMGR_H
#define DRIVEMGR_H

#include <Arduino.h>

class DriveMgr {
    private:
       // Pin Variable Area
        uint8_t _ain1;
        uint8_t _ain2;
        uint8_t _bin1;
        uint8_t _bin2;
       
        // Volatile Variable Area
       uint8_t minPWM;
       uint8_t maxPWM;
       
       // QoL Improvement for Motor Driver
       float trim_fb;
       float trim_lr;
       float rotateGainMultiplier;
       float strafeGainMultiplier;
       

    public:
        void MotorInit(
            uint8_t ain1, uint8_t ain2,
             uint8_t bin1, uint8_t bin2,
              uint32_t freq, uint8_t res
            );

        void stop();
        void setPowerTrim(float trim_frontback, float trim_leftright);
        void setGainMultipliers(float strafeGainMultiplier, float rotateGainMultiplier);
        void drive(float strafeX, float forwardY, float rotationX, int16_t baseSpeed);
        /**
        * Memetakan input perintah (0 - 255) ke output PWM fisik driver motor.
        * @param inputPWM : Nilai target kecepatan (0 - 255)
        * @return uint8_t : Nilai PWM yang disesuaikan dengan deadzone driver
        */
        uint8_t ApplyDeadzone(uint8_t inputPWM);

        uint8_t DriveMgr::SetMotorSpeed(int16_t speed);



};
#endif // DRIVEMGR_H