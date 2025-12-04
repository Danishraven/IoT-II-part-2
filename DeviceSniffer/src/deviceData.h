#ifndef DEVICEDATA_H
#define DEVICEDATA_H

#include <WString.h>
#include <cstdint>
#include <ctime>

class deviceData
{
private:
    String macAdress;
    int8_t sniffer1rssi = 0;
    int8_t sniffer2rssi = 0;
    int8_t sniffer3rssi = 0;

public:
    struct Point { float x; float y; };
    struct Area  { Point center; float radius; };

    deviceData();
    // Construct with provided MAC and three RSSI values
    deviceData(const String &mac, int8_t s1, int8_t s2, int8_t s3);

    // Setters for RSSI values (store MAC & timestamp alongside RSSI measurements)
    void setSnifferRssi(const String &mac, int8_t s1, int8_t s2, int8_t s3);

    // Trilateration APIs
    Point trilaterate(float x1, float y1,
                      float x2, float y2,
                      float x3, float y3) const;

    Area trilaterateArea(float x1, float y1,
                         float x2, float y2,
                         float x3, float y3) const;

    // Accessors
    String getMac() const;
    int8_t getRssi1() const;
    int8_t getRssi2() const;
    int8_t getRssi3() const;
};

#endif // DEVICEDATA_H
