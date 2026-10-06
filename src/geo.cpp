#include "geo.hpp"
#include <cmath>

static constexpr double kDeg = M_PI / 180.0;

static double wrap180(double d) {
    while (d > 180.0) d -= 360.0;
    while (d < -180.0) d += 360.0;
    return d;
}

XY project(double lat, double lon, double origin_lat, double origin_lon) {
    XY p;
    p.x = kEarthR * std::cos(origin_lat * kDeg) * wrap180(lon - origin_lon) * kDeg;
    p.y = kEarthR * (lat - origin_lat) * kDeg;
    return p;
}

XY velocity(double sog_kn, double cog_deg) {
    double v = sog_kn * kKnotToMs;
    double rad = cog_deg * kDeg;
    return {v * std::sin(rad), v * std::cos(rad)}; // 航向自北顺时针
}

CpaResult cpa(XY p0, XY v0, XY p1, XY v1, double horizon) {
    double rx = p1.x - p0.x, ry = p1.y - p0.y;
    double vx = v1.x - v0.x, vy = v1.y - v0.y;
    double vv = vx * vx + vy * vy;
    double t = 0.0;
    if (vv > 0.0) {
        t = -(rx * vx + ry * vy) / vv;
        if (t < 0.0) t = 0.0;
        if (t > horizon) t = horizon;
    }
    double dx = rx + vx * t, dy = ry + vy * t;
    return {std::hypot(dx, dy), t};
}
