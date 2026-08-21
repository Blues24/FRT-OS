#include <Arduino.h>
#include <cmath>

float getNormalizedAxis(
    int rawValADC, int center,
    int minVal, int maxVal, int dz)
{
    int delta = rawValADC - center;

    if (std::abs(delta) <= dz)
        return 0.0f;

    if (delta > 0)
    {
        float maxPosRange = (maxVal - center) - dz;
        return (float)(delta - dz) / maxPosRange;
    }
    else
    {
        float maxNegRange = (center - minVal) - dz;
        return (float)(delta + dz) / maxNegRange;
    }
}