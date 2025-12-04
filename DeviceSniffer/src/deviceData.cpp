#include "deviceData.h"
#include <cmath>
#include <algorithm>

// Internal defaults for RSSI->distance conversion (implementation detail)
static constexpr float DEFAULT_REF_RSSI = -41.0f; // dBm at 1 meter
static constexpr float DEFAULT_PATHLOSS = 2.0f;   // path-loss exponent

deviceData::deviceData() = default;

deviceData::deviceData(const String &mac, time_t ts, int8_t s1, int8_t s2, int8_t s3)
{
    macAdress = hashMac(mac);
    timestamp = ts;
    sniffer1rssi = s1;
    sniffer2rssi = s2;
    sniffer3rssi = s3;
}

String deviceData::getMac() const { return macAdress; }
time_t deviceData::getTimestamp() const { return timestamp; }
int8_t deviceData::getRssi1() const { return sniffer1rssi; }
int8_t deviceData::getRssi2() const { return sniffer2rssi; }
int8_t deviceData::getRssi3() const { return sniffer3rssi; }

void deviceData::setSnifferRssi(const String &mac, time_t ts, int8_t s1, int8_t s2, int8_t s3)
{
    macAdress = hashMac(mac);

    timestamp = ts;
    sniffer1rssi = s1;
    sniffer2rssi = s2;
    sniffer3rssi = s3;
}

deviceData::Point deviceData::trilaterate(float x1, float y1,
                                          float x2, float y2,
                                          float x3, float y3) const
{
    auto rssiToDist = [&](int8_t rssi) -> float
    {
        // d = 10^((refRssi - rssi) / (10 * n)) using internal defaults
        float exponent = (DEFAULT_REF_RSSI - static_cast<float>(rssi)) / (10.0f * DEFAULT_PATHLOSS);
        return std::pow(10.0f, exponent);
    };

    float r1 = rssiToDist(sniffer1rssi);
    float r2 = rssiToDist(sniffer2rssi);
    float r3 = rssiToDist(sniffer3rssi);

    // Setup linear system from circle equations
    float A1 = 2.0f * (x2 - x1);
    float B1 = 2.0f * (y2 - y1);
    float C1 = r1 * r1 - r2 * r2 - x1 * x1 + x2 * x2 - y1 * y1 + y2 * y2;

    float A2 = 2.0f * (x3 - x1);
    float B2 = 2.0f * (y3 - y1);
    float C2 = r1 * r1 - r3 * r3 - x1 * x1 + x3 * x3 - y1 * y1 + y3 * y3;

    float det = A1 * B2 - A2 * B1;

    deviceData::Point p;
    if (std::fabs(det) < 1e-6f)
    {
        // Degenerate or nearly collinear sniffers; fallback to centroid
        p.x = (x1 + x2 + x3) / 3.0f;
        p.y = (y1 + y2 + y3) / 3.0f;
        return p;
    }

    p.x = (C1 * B2 - C2 * B1) / det;
    p.y = (A1 * C2 - A2 * C1) / det;
    return p;
}

deviceData::Area deviceData::trilaterateArea(float x1, float y1,
                                             float x2, float y2,
                                             float x3, float y3) const
{
    // Reuse trilaterate to compute a best-fit center
    Point center = trilaterate(x1, y1, x2, y2, x3, y3);

    auto rssiToDist = [&](int8_t rssi) -> float
    {
        float exponent = (DEFAULT_REF_RSSI - static_cast<float>(rssi)) / (10.0f * DEFAULT_PATHLOSS);
        return std::pow(10.0f, exponent);
    };

    float r1 = rssiToDist(sniffer1rssi);
    float r2 = rssiToDist(sniffer2rssi);
    float r3 = rssiToDist(sniffer3rssi);

    // Compute residuals between expected radii and distances from computed center
    auto dist = [](const Point &p, float x, float y) -> float
    {
        float dx = p.x - x;
        float dy = p.y - y;
        return std::sqrt(dx * dx + dy * dy);
    };

    float d1 = dist(center, x1, y1);
    float d2 = dist(center, x2, y2);
    float d3 = dist(center, x3, y3);

    float e1 = d1 - r1;
    float e2 = d2 - r2;
    float e3 = d3 - r3;

    // Root-mean-square residual as a simple uncertainty estimator
    float rms = std::sqrt((e1 * e1 + e2 * e2 + e3 * e3) / 3.0f);

    // Minimum uncertainty floor to avoid zero radius when perfect fit occurs
    const float minUncertainty = 0.05f; // meters
    deviceData::Area area;
    area.center = center;
    area.radius = std::max(rms, minUncertainty);
    return area;
}

String hashMac(const String mac) {
    unsigned long hash = 5381;
    for (size_t i = 0; i < mac.length(); ++i) {
        hash = ((hash << 5) + hash) + mac[i]; // hash * 33 + c
    }
    return String(hash);
}