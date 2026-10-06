#!/usr/bin/env python3
"""生成合法的 !AIVDM 语句（含XOR校验和）。类型1动态、类型5船名(两片)。"""
import sys

def bits_to_payload(bits):
    while len(bits) % 6:
        bits.append(0)
    fill = 0
    out = []
    for i in range(0, len(bits), 6):
        v = 0
        for b in bits[i:i+6]:
            v = (v << 1) | b
        c = v + 48
        if c > 87:
            c += 8
        out.append(chr(c))
    return ''.join(out), fill

def put(bits, value, length, signed=False):
    if signed and value < 0:
        value += 1 << length
    for i in range(length - 1, -1, -1):
        bits.append((value >> i) & 1)

def checksum(body):
    x = 0
    for ch in body:
        x ^= ord(ch)
    return f"{x:02X}"

def sentence(payload, fill, total=1, num=1, seq="", channel="A"):
    body = f"AIVDM,{total},{num},{seq},{channel},{payload},{fill}"
    return f"!{body}*{checksum(body)}"

def type1(mmsi, lat, lon, sog, cog):
    b = []
    put(b, 1, 6); put(b, 0, 2); put(b, mmsi, 30)
    put(b, 8, 4)            # nav status
    put(b, 0, 8, signed=True)  # ROT
    put(b, int(round(sog * 10)), 10)
    put(b, 0, 1)            # accuracy
    put(b, int(round(lon * 600000)), 28, signed=True)
    put(b, int(round(lat * 600000)), 27, signed=True)
    put(b, int(round(cog * 10)), 12)
    put(b, 511, 9)          # heading n/a
    put(b, 0, 6)            # second
    put(b, 0, 2); put(b, 0, 3); put(b, 0, 1); put(b, 0, 19)  # manoeuvre+spare+raim+radio
    payload, fill = bits_to_payload(b)
    return [sentence(payload, fill)]

def name_to_sixbit(name):
    vals = []
    for ch in name.ljust(20, '@')[:20]:
        o = ord(ch)
        vals.append(o if o < 0x40 else o - 64)
    return vals

def type5(mmsi, name):
    b = []
    put(b, 5, 6); put(b, 0, 2); put(b, mmsi, 30)
    put(b, 0, 2)            # ais version
    put(b, 0, 30)           # imo
    for v in [0]*7: put(b, v, 6)   # callsign
    for v in name_to_sixbit(name): put(b, v, 6)
    put(b, 70, 8)           # ship type cargo
    put(b, 0, 9); put(b, 0, 9); put(b, 0, 6); put(b, 0, 6)  # dim
    put(b, 0, 4)            # fix
    put(b, 0, 4); put(b, 0, 5); put(b, 0, 5); put(b, 0, 6)  # eta
    put(b, 0, 8)            # draught
    for v in [0]*20: put(b, v, 6)  # destination
    put(b, 0, 1); put(b, 0, 1)
    payload, fill = bits_to_payload(b)
    # 424 bits -> 71 chars, 两片: 60 + 11
    p1, p2 = payload[:60], payload[60:]
    return [sentence(p1, 0, total=2, num=1, seq="7"),
            sentence(p2, fill, total=2, num=2, seq="7")]

if __name__ == "__main__":
    for line in type1(412000001, 31.2304, 121.4737, 10.0, 90.0):
        print(line)
    for line in type1(477000002, 31.2350, 121.4837, 12.0, 270.0):
        print(line)
    for line in type5(477000002, "HAI XUN 01"):
        print(line)
