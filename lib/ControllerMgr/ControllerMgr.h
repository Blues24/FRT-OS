#ifndef CONTROLLERMGR_H
#define CONTROLLERMGR_H

#include <Arduino.h>
#include <cmath>

struct BezierSample
{
    float x;
    float y;
    float dx; // derivative dx/dt
};

class ControllerMgr {
    private:
        static const int16_t BASE_MOTOR_SPD = 100;
        static const int16_t MAX_MOTOR_SPD = 255;
        static const uint8_t MIN_DZ = 15;
        static const uint8_t BOOST_DZ = 25;
        static const uint8_t MAX_DZ = 40;

        float bezierControlX1, bezierControlX2, bezierControlY1, bezierControlY2; 
        
        float applyBezier(float target_x);
        inline BezierSample getBezierSample(float t);
    public:
        
};


#endif // CONTROLLERMGR_H