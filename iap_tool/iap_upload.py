#!/usr/bin/env python3
# -*- coding: utf-8 -*-
"""
串口 IAP 上位机脚本
用法:
    python iap_upload.py <bin文件绝对路径> [串口号]
示例:
    python iap_upload.py D:/project/app/app.bin COM5
不带串口号时自动列出可用串口供选择
"""

import serial
import serial.tools.list_ports
import sys
import time

BAUD = 115200
BLOCK = 256
MAX_RETRY = 3
ACK, NACK = 0x06, 0x15
CMD_HS, CMD_ERASE, CMD_DATA, CMD_END, CMD_JUMP = 1, 2, 3, 4, 5


def crc16(data: bytes, init: int = 0xFFFF) -> int:
    crc = init
    for b in data:
        crc ^= b << 8
        for _ in range(8):
            crc = ((crc << 1) ^ 0x1021) & 0xFFFF if (crc & 0x8000) else (crc << 1) & 0xFFFF
    return crc


def build_frame(cmd: int, data: bytes = b"") -> bytes:
    frame = bytes([0xAA, 0x55, cmd,
                   len(data) & 0xFF, (len(data) >> 8) & 0xFF]) + data
    c = crc16(frame)
    return frame + bytes([c & 0xFF, (c >> 8) & 0xFF])


def send_wait(ser, frame) -> bool:
    for _ in range(MAX_RETRY):
        ser.write(frame)
        resp = ser.read(1)
        if resp == bytes([ACK]):
            return True
        time.sleep(0.1)
    return False


def pick_port() -> str:
    ports = list(serial.tools.list_ports.comports())
    if not ports:
        print("未检测到任何串口，请检查 USB 转 TTL 是否连接")
        sys.exit(1)
    print("可用串口:")
    for i, p in enumerate(ports):
        print(f"  [{i}] {p.device}  {p.description}")
    while True:
        try:
            idx = int(input("请输入序号选择串口: ").strip())
            if 0 <= idx < len(ports):
                return ports[idx].device
        except (ValueError, IndexError):
            pass
        print("序号无效，请重新输入")


def main():
    if len(sys.argv) >= 2:
        path = sys.argv[1]
    else:
        path = input("请输入 bin 文件绝对路径: ").strip().strip('"')
        if not path:
            print("未输入路径，退出")
            sys.exit(1)
    try:
        with open(path, "rb") as f:
            bin_data = f.read()
    except OSError as e:
        print(f"无法打开文件: {e}")
        sys.exit(1)
    print(f"文件: {path}")
    print(f"文件大小: {len(bin_data)} 字节")

    port = sys.argv[2] if len(sys.argv) > 2 else pick_port()

    ser = serial.Serial(port, BAUD, timeout=2)
    ser.reset_input_buffer()

    try:
        print("请确保目标板已进入 Bootloader 模式（按住按键上电，LED 闪烁）...")
        input("按回车键开始烧录 (Ctrl+C 取消): ")
        ser.reset_input_buffer()
        if not send_wait(ser, build_frame(CMD_HS)):
            print("握手失败: 请确认串口号正确、板子已进入 Bootloader 模式")
            time.sleep(3)
            return
        print("握手成功")

        if not send_wait(ser, build_frame(CMD_ERASE)):
            print("擦除失败")
            time.sleep(3)
            return
        print("擦除完成")

        total_crc = 0xFFFF
        total = len(bin_data)
        for i in range(0, total, BLOCK):
            chunk = bin_data[i:i + BLOCK]
            total_crc = crc16(chunk, total_crc)
            if not send_wait(ser, build_frame(CMD_DATA, chunk)):
                print(f"\n数据帧失败 @ 0x{i:X}，请重新烧录")
                time.sleep(3)
                return
            done = min(i + BLOCK, total)
            print(f"\r进度: {done}/{total} ({done * 100 // total}%)", end="")
        print()

        end = len(bin_data).to_bytes(4, "little") + total_crc.to_bytes(2, "little")
        if not send_wait(ser, build_frame(CMD_END, end)):
            print("结束帧失败，整包校验未通过")
            time.sleep(3)
            return
        print("整包校验通过")

        send_wait(ser, build_frame(CMD_JUMP))
        print("已跳转 App")
        time.sleep(3)
    finally:
        ser.close()


if __name__ == "__main__":
    main()
