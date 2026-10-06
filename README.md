# AIS 会遇预警后端

港区值班用 AIS 会遇预警服务。C++20 + Boost.Asio/Beast 1.74（头文件方式），
无前端。UDP 接收 `!AIVDM` 语句，HTTP 查询船舶与本船对各目标的会遇。

## 构建与启动

```bash
./prepare-deps.sh        # 首次：拉取 Boost1.74 与 nlohmann/json 到 .deps/
make                     # 生成 ais_server
./ais_server samples/ais_server.conf
```

演示（构建、启动、发样例、curl 展示更新与预警）：

```bash
samples/demo.sh
```

## 配置（key=value）

| 键 | 含义 | 默认 |
|---|---|---|
| udp_port | 环回/本机 UDP 接收端口 | 5010 |
| http_port | HTTP 查询端口 | 8080 |
| own_mmsi | 本船 MMSI | 0 |
| origin_lat / origin_lon | 港区原点（|纬度| < 70°） | 0 / 0 |
| max_age_s | 动态有效秒数（按单调收包时刻计龄） | 30 |
| predict_s | 预测区间秒数 | 180 |
| warn_m | 警戒距离米数 | 500 |
| max_groups / max_group_messages / max_vessels / max_datagram | 分片组数、组内片数、船数、报文字节上限 | 64 / 9 / 256 / 1024 |

## 处理规则

- **语句校验**：XOR 校验和、7 字段结构、载荷字符集与填充位（0–5）。
- **消息类型**：1/2/3 动态报告（经纬度、SOG、COG），5 船名；其余忽略。
- **分片重组**：按（发送端点, 序号, 频道）隔离；乱序可收；重复分片幂等；
  同序号载荷冲突整组丢弃；未齐 5 秒清理；多片序号不能为空。
- **状态合并**：按 MMSI 合并船名与动态；新动态整体替换旧的，不可用值
  （lon=181、lat=91、SOG=1023、COG=3600）标缺测，不沿用旧位置。
- **计龄**：用单调时钟收包时刻，不用 AIS 秒字段；过期不参与预警。

## 会遇模型（匀速局部近似）

- 球半径 6371000 m，以原点纬度做等距圆柱投影得东/北米坐标，经差取最短方向。
- 节 × 1852/3600 = m/s；航向自北顺时针分解速度。
- 各船按各自船龄匀速外推至查询同一时刻，再解相对运动：
  TCPA = −(r·v)/|v|²（相对速度为零取 0），夹至 [0, predict_s]；
  DCPA 为该时刻相对距离，DCPA ≤ warn_m 报警。
- 仅处理距原点 50 km 内的船位。小范围、短时间内的匀速平面近似，
  不适用于大范围或机动场景。

## HTTP 接口

- `GET /ships`：全部船舶（MMSI、船名、船龄、位置/速度或缺测、过期状态）。
- `GET /encounters`：本船有效性（own_missing / own_no_dynamic /
  own_pos_missing / own_expired / own_out_of_area / ok）及逐目标
  DCPA/TCPA/alarm；目标状态含 no_dynamic、expired、pos_missing、out_of_area。
  本船无效时返回明确告警文案，不提供会遇结论（不当安全）。
- `GET /health`：存活检查。

## 样例报文

`samples/gen_ais.py` 生成合法 `!AIVDM`（类型1与两片类型5，含校验和）：

```bash
python3 samples/gen_ais.py
# !AIVDM,1,1,,A,168rO0H01T8d3wpAod@3Q?v00000,0*70
# !AIVDM,2,1,7,A,576qi@P000000000000P4V1QDr3340000000001600000000000000000000,0*49
# ...
```

## 结构

- `src/nmea.*` 语句解析校验；`src/sixbit.*` 六位解码；`src/ais.*` 消息解码
- `src/reassembler.*` 分片重组；`src/store.*` 按 MMSI 航迹合并（互斥保护，快照读）
- `src/geo.*` 投影与 CPA；`src/query.*` 查询 JSON
- `src/udp_server.*` / `src/http_server.*` 异步接入；`src/main.cpp` 装配与信号退出
