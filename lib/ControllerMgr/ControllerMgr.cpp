#include "ControllerMgr.h"

// Buat satu instance menggunakan meyers singleton
ControllerMgr& ControllerMgr::getInstance(){
    static ControllerMgr instance;
    return instance;
}

ControllerMgr::ControllerMgr() : bezierControlX1(0.0f), bezierControlX2(1.0f), bezierControlY1(0.0f), bezierControlY2(1.0f) {}

void ControllerMgr::notifyUser(){
    // Sambut pengguna menggunakan buzzer
}

void ControllerMgr::onConnect(){
    Serial.println("[INFO] PS3 Controller has been Connected!");
    
}

void ControllerMgr::onDisconnect(){
    Serial.println("[INFO] PS3 Controller has been Disconnected!");
    
}

void ControllerMgr::initPs3(const char* mac){
    Ps3.attach(notifyUser);
    Ps3.attachOnConnect(onConnect);
    Ps3.attachOnDisconnect(onDisconnect);
    Ps3.begin(mac);
}

bool ControllerMgr::isConnected(){
    return Ps3.isConnected();
}

int ControllerMgr::getBatteryLevel(){
    if (!isConnected()) return 0;
    return Ps3.data.status.battery;
}

void ControllerMgr::setCurve(float x1, float x2, float y1, float y2){
    bezierControlX1 = constrain(x1, 0.0f, 1.0f);
    bezierControlX2 = constrain(x2, 0.0f, 1.0f);
    bezierControlY1 = constrain(y1, 0.0f, 1.0f);
    bezierControlX2 = constrain(y2, 0.0f, 1.0f);
}

void ControllerMgr::getDriveInput(float& leftStickX, float& leftStickY, float& rightStickX, int16_t& outSpeed){
    if(!isConnected()){
        leftStickX, leftStickY, rightStickX, outSpeed = 0;
        return;
    }

    int stickLeftX  = Ps3.data.analog.stick.lx;
    int stickLeftY  = Ps3.data.analog.stick.ly;
    int stickRightX = Ps3.data.analog.stick.rx;
    int stickRightY = Ps3.data.analog.stick.ry;

    // terapkan deadzone
    int absLeftX  = abs(stickLeftX);
    int absLeftY  = abs(stickLeftY);
    int absRightX = abs(stickRightX);
    int absRightY = abs(stickRightY);

    if(absLeftX < MIN_DZ)    leftStickX = 0;
    if(absLeftY < MIN_DZ)    leftStickY = 0;
    if(absRightX < BOOST_DZ) rightStickX = 0;
    if(absRightY < MIN_DZ)   rightStickX = 0;

    // normalisasikan input dari -1.0 sampai dengan 1.0
    float normalizedLeftX, normalizedLeftY, normalizedRightX = 0;

    if(absLeftX > BOOST_DZ)  normalizedLeftX = (leftStickX - (leftStickX > 0 ? BOOST_DZ : -BOOST_DZ)) / (127.0f - BOOST_DZ);
    if(absLeftY > BOOST_DZ)  normalizedLeftY = (leftStickY - (leftStickY > 0 ? BOOST_DZ : -BOOST_DZ)) / (127.0f - BOOST_DZ);
    if(absRightX > BOOST_DZ) normalizedRightX = rightStickX / 127.0f;

    normalizedLeftX  = constrain(normalizedLeftX, -1.0f, 1.0f);
    normalizedLeftY  = constrain(normalizedLeftY, -1.0f, 1.0f);
    normalizedRightX = constrain(normalizedRightX, -1.0f, 1.0f);

    // Terapkan kurva bezier
    normalizedLeftX  = applyBezierCurve(normalizedLeftX);
    normalizedLeftY  = applyBezierCurve(normalizedLeftY);
    normalizedRightX = applyBezierCurve(normalizedRightX);

    // Hitung kecepatan menggunakan logika boost
    int16_t mSpeed = BASE_MOTOR_SPD;
    float speedBoostMultiplier = 0;

    if(normalizedLeftY < -0.05f) mSpeed = MAX_MOTOR_SPD;
    else if(absRightY > BOOST_DZ){
        float rotationStrength = absRightY / 127.0f;
        float boostStrength    = (absRightY - BOOST_DZ) / (127.0f - BOOST_DZ);
        
        if(absRightY < ROTATION_PRIORITY_THRESHOLD){
            float rotationPenalty = rotationStrength * 0.3f;
            speedBoostMultiplier = boostStrength * (1.0f - rotationPenalty);

            int boostAmount = (MAX_MOTOR_SPD - BASE_MOTOR_SPD) * speedBoostMultiplier;
            mSpeed += boostAmount;

            // Periksa kecepatan motor terhadap MAX_MOTOR_SPD
            if(mSpeed > MAX_MOTOR_SPD) mSpeed = MAX_MOTOR_SPD;
        }
    }

    leftStickX  = normalizedLeftX; leftStickY = normalizedLeftY;
    rightStickX = normalizedRightX; outSpeed = mSpeed;
}

inline BezierSample ControllerMgr::getBezierSample(float t){
    float u = 1.0f - t;
    float u2 = u * u;
    float t2 = t * t;
    // Hitung nilai x, y, dan dx
    float x = 3.0f * u2 * t * bezierControlX1 + 3.0f * u * t2 * bezierControlX2 + t * t2;
    float y = 3.0f * u2 * t * bezierControlY1 + 3.0f * u * t2 * bezierControlY2 + t * t2;
    float dx = 3.0f * (u2 * bezierControlX1 + 2.0f * u * t * (bezierControlX2 - bezierControlX1) + t2 * (1.0f - bezierControlX2));

    return {x,y,dx};
}

float ControllerMgr::solveBezier(float target_axis){
    float lowerVal = 0.0f;
    float upperVal = 1.0f;
    float t = target_axis;
    
    if(target_axis <= 0.0f) return lowerVal;
    if(target_axis >= 1.0f) return upperVal;

    // lakukan iterasi sampel bezier sebanyak 4-6 kali
    for(int i = 0; i < 6; i++){
        BezierSample sample = getBezierSample(t);
        float err = sample.x - target_axis;
        if(fabs(err) < 1e-5f) return t;

        // terapkan dx ke t
        float derivativeX = sample.dx;
        if(fabs(derivativeX) < 1e-6f) t = 0.5f * (lowerVal + upperVal);
        else {
            float tNext = t - err / derivativeX;
            if(tNext <= lowerVal || tNext >= upperVal) t = 0.5f * (lowerVal + upperVal);
            else t = tNext;
        }

        // Persempit bracket
        if (getBezierSample(t).x > target_axis) upperVal = t;
        else lowerVal = t;

    }

    return t;
}

inline float ControllerMgr::applyBezierCurve(float rawVal){
    if(rawVal == 0) return 0;

    float absInputVal = fabsf(rawVal);
    float bezierParam = solveBezier(absInputVal);
    float appliedAbsCurveVal = getBezierSample(bezierParam).y;

    return (rawVal > 0) ? appliedAbsCurveVal : -appliedAbsCurveVal;
}

bool ControllerMgr::isPressed(ButtonPress Btn){
    switch(Btn){
        case ButtonPress::Cross:     return Ps3.data.button.cross;
        case ButtonPress::Circle:    return Ps3.data.button.circle;
        case ButtonPress::Square:    return Ps3.data.button.square;
        case ButtonPress::Triangle:  return Ps3.data.button.triangle;
        case ButtonPress::Up:        return Ps3.data.button.up;
        case ButtonPress::Down:      return Ps3.data.button.down;
        case ButtonPress::Left:      return Ps3.data.button.left;
        case ButtonPress::Right:     return Ps3.data.button.right;
        case ButtonPress::L1:        return Ps3.data.button.l1;
        case ButtonPress::L2:        return Ps3.data.button.l2;
        case ButtonPress::Select:    return Ps3.data.button.select;
        case ButtonPress::Start:     return Ps3.data.button.start;
        case ButtonPress::PSButton:  return Ps3.data.button.ps;
        default: return false;
    }
}