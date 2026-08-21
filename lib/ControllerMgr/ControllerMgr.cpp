#include "ControllerMgr.h"



inline BezierSample ControllerMgr::getBezierSample(float t){
    float u = 1.0f - t;
    float u2 = u * u;
    float t2 = t * t;
    // Calculate x,y,dx value
    float x = 3.0f * u2 * t * bezierControlX1 + 3.0f * u * t2 * bezierControlX2 + t * t2;
    float y = 3.0f * u2 * t * bezierControlY1 + 3.0f * u * t2 * bezierControlY2 + t * t2;
    float dx = 3.0f * (u2 * bezierControlX1 + 2.0f * u * t * (bezierControlX2 - bezierControlX1) + t2 * (1.0f - bezierControlX2));

    return {x,y,dx};
}

float ControllerMgr::applyBezier(float target_x){
    if(target_x <= 0.0f) return 0.0f;
    if(target_x >= 1.0f) return 1.0f;

    float lowerVal = 0.0f;
    float upperVal = 1.0f;

    float t = target_x;

    for(int i = 0; i < 6; i++){
        BezierSample sample = getBezierSample(t);
        float err = sample.x - target_x;
        if(fabs(err) < 1e-5f) return t;

        float derivativeX = sample.dx;
        if(fabs(derivativeX) < 1e-6f) t = 0.5f * (lowerVal + upperVal);
        else {
            float tNext = t - err / derivativeX;
            if(tNext <= lowerVal || tNext >= upperVal) t = 0.5f * (lowerVal + upperVal);
            else t = tNext;
        }

        // Shrink bracket
        if (getBezierSample(t).x > target_x) upperVal = t;
        else lowerVal = t;

    }

    return t;
}