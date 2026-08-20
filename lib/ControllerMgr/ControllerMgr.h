#ifndef CONTROLLERMGR_H
#define CONTROLLERMGR_H

/**
 * Normalisasi nilai analog dengan Scaled Deadzone
 * @param rawValue   : Nilai ADC mentah (misal: 0 - 4095)
 * @param center     : Nilai titik tengah (misal: 2048)
 * @param minVal     : Nilai ADC minimum (0)
 * @param maxVal     : Nilai ADC maksimum (4095)
 * @param deadzone   : Nilai offset deadzone (misal: 150)
 * @return float     : Output ter-skala dari -1.0f (mundur/kiri) sampai 1.0f (maju/kanan)
 */
float getNormalizedAxis(
    int rawValADC, int center,
    int minVal, int maxVal, int dz);

#endif // CONTROLLERMGR_H