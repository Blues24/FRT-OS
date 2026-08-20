#include "DriveMgr.h"
#include "RaggedyPins.h"
#include <Arduino.h>

void DriveMgr::MotorInit(uint8_t ain1, uint8_t ain2,
                         uint8_t bin1, uint8_t bin2,
                         uint32_t freq, uint8_t res)
{
    // Ubah variabel pin menjadi class variable
    _ain1 = ain1; _ain2 = ain2;
    _bin1 = bin1; _bin2 = bin2;
    
    trim_fb = 0.0f;
    trim_lr = 0.0f;

    strafeGainMultiplier = 1.0f;
    rotateGainMultiplier = 1.0f;

    uint8_t PinsList[4] = {_ain1, _ain2, _bin1, _bin2};
    
    for(size_t mPin = 0; mPin < 4; mPin++){
        ledcAttach(PinsList[mPin], freq, res);
        ledcWrite(PinsList[mPin], 0);
    }
}



uint8_t DriveMgr::ApplyDeadzone(uint8_t inputPWM){
    if(inputPWM == 0) return 0;

    uint32_t PWM_map = minPWM + ((uint32_t)inputPWM * (maxPWM - minPWM)) / 255;
    
    return static_cast<uint8_t>(std::min<uint32_t>(PWM_map, maxPWM));
}

uint8_t DriveMgr::SetMotorSpeed(int16_t speed){
    uint8_t realPWM = ApplyDeadzone(speed);
    if(speed > 0){
        ledcWrite(_ain1, realPWM);
        ledcWrite(_ain2, 0);
        ledcWrite(_bin1, realPWM);
        ledcWrite(_bin2, 0);
    }else if (speed < 0){
        ledcWrite(_ain1, 0);
        ledcWrite(_ain2, realPWM);
        ledcWrite(_bin1, 0);
        ledcWrite(_bin2, realPWM);
    }else {
        ledcWrite(_ain1, 0);
        ledcWrite(_ain2, 0);
        ledcWrite(_bin1, 0);
        ledcWrite(_bin2, 0);
    }
}

void DriveMgr::stop(){
    for (int mPinIndx = 0; mPinIndx < 4; mPinIndx++){
        SetMotorSpeed(0);
    }
}

void DriveMgr::setPowerTrim(float trim_frontback, float trim_leftright){
    trim_fb = trim_frontback;
    trim_lr = trim_leftright;
}
void DriveMgr::setGainMultipliers(float strafeGain, float rotateGain){
    strafeGainMultiplier = strafeGain;
    rotateGainMultiplier = rotateGain;
}

float getMaxAbsValue(float a, float b, float c, float d) {
    float max = fabs(a);
    if (fabs(b) > max) max = fabs(b);
    if (fabs(c) > max) max = fabs(c);
    if (fabs(d) > max) max = fabs(d);
    return max;
}

void DriveMgr::drive(float lx, float ly, float rx, int16_t baseSpeed){
    lx *= strafeGainMultiplier;
    ly *= rotateGainMultiplier;


    // Kinematik Mechanum (X - Config)
    float frontLeftSpeed    = ly + lx + rx;
    float frontRightSpeed   = ly - lx - rx;
    float backLeftSpeed     = ly - lx + rx;
    float backRightSpeed    = ly + lx - rx;

    float maxSpeed = getMaxAbsValue( frontLeftSpeed, frontRightSpeed,
                                     backLeftSpeed, backRightSpeed);
    if(maxSpeed > 1.0f){
        frontLeftSpeed  /= maxSpeed;
        frontRightSpeed /= maxSpeed;
        backLeftSpeed   /= maxSpeed;
        backRightSpeed  /= maxSpeed;
    }

    float targetSpeed[4] = {
        frontLeftSpeed * baseSpeed, frontRightSpeed * baseSpeed,
        backLeftSpeed  * baseSpeed, backRightSpeed  * baseSpeed
    };

    // handle Power Trim
    float frontTrim = (trim_fb < 0 ? -trim_fb : 0);
    float backTrim  =  (trim_fb > 0 ? trim_fb : 0);
    float leftTrim  =  (trim_lr < 0 ? -trim_lr : 0);
    float rightTrim = (trim_lr > 0 ? trim_lr : 0);

    // Apply Scale power trim
    float scales[4] = {
        1.0f + frontTrim + leftTrim,
        1.0f + frontTrim + rightTrim,
        1.0f + backTrim + leftTrim,
        1.0f + backTrim + rightTrim,
    };

    for(int i = 0; i < 4; i++){
        targetSpeed[i] *= scales[i];
    }
}