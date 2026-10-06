#pragma once

inline constexpr double kEarthR = 6371000.0;      // 球半径 m
inline constexpr double kKnotToMs = 1852.0 / 3600.0;

struct XY { double x = 0, y = 0; }; // 东、北 米

// 以原点纬度做等距圆柱投影；经差取最短方向
XY project(double lat, double lon, double origin_lat, double origin_lon);

// 速度(节, 度自北顺时针) -> (东,北) m/s
XY velocity(double sog_kn, double cog_deg);

struct CpaResult { double dcpa = 0, tcpa = 0; };

// 相对运动最小距离；相对速度为零时 TCPA=0；时间夹至 [0, horizon]
CpaResult cpa(XY p0, XY v0, XY p1, XY v1, double horizon);
