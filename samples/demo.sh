#!/usr/bin/env bash
# 构建 -> 启动 -> 发送样例 -> curl 展示更新与预警
set -eu
cd "$(dirname "$0")/.."
make
./ais_server samples/ais_server.conf &
SRV=$!
trap 'kill $SRV 2>/dev/null || true' EXIT
sleep 1

echo '== 发送本船 + 来船动态与船名 =='
python3 - <<'PY'
import socket, sys
sys.path.insert(0, 'samples')
from gen_ais import type1, type5
s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
def send(ls):
    for l in ls: s.sendto((l+'\r\n').encode(), ('127.0.0.1', 5010))
send(type1(412000001, 31.2304, 121.4737, 10.0, 90.0))   # 本船: 原点, 10kn 向东
send(type1(477000003, 31.2304, 121.4769, 0.0, 0.0))     # 来船: 正东约300m 静止
send(type5(477000003, "TEST TARGET"))
send(type1(477000004, 91.0, 181.0, 5.0, 180.0))         # 船位不可用 -> 缺测
PY
sleep 1

echo '== GET /ships =='
curl -s http://127.0.0.1:8080/ships | python3 -m json.tool
echo '== GET /encounters =='
curl -s http://127.0.0.1:8080/encounters | python3 -m json.tool
