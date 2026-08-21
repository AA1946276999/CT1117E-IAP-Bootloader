# STM32G431 串口 IAP 在线升级系统

基于 **STM32G431RBT6** 的串口在线升级（IAP, In-Application Programming）系统。板子上电后由 Bootloader 决定进入 App 或进入升级模式，通过 **USART2**（可外接蓝牙/2.4G 无线模块）接收 PC 上位机发送的固件，写入 Flash 后跳转运行。

## 特性

- **无线 OTA**：USART2(PA2/PA3) 外接蓝牙 / 2.4G 模块，隔空升级不用接线
- **可靠传输**：逐帧 CRC16 + ACK/NAK 重传 + 整包校验，通过才允许跳转
- **分层可移植**：核心逻辑(`iap.c`)与硬件适配(`iap_port.c`)分离，换 MCU 只改移植层
- **指示灯提示**：BL 模式下 LED 闪烁，直观确认状态

## 开发平台

本工程面向 **蓝桥杯嵌入式竞赛开发板（CT117E）**：

| 资源 | 配置 |
|------|------|
| 主控 | STM32G431RBT6 (Cortex-M4, 128KB Flash, 32KB RAM) |
| 屏幕 | 2.4 寸 TFT LCD（并行接口） |
| LED | PC8~PC15 + PD2 锁存 |
| 按键 | PB0 等 |
| 串口 | USART1(PA9/10, 板载USB) + USART2(PA2/3, 引出) |
| 其他 | 电位器(ADC)、RTC、PWM 等 |

> 本项目用到的板载资源：**USART2(PA2/PA3, OTA 串口)**、PB0(进 BL 按键)、LED(BL 模式指示)。

### 双串口说明

| 串口 | 引脚 | 波特率 | 用途 |
|------|------|--------|------|
| USART1 | PA9(TX) / PA10(RX) | 115200 | 板载 USB：通过板载 GD32 芯片转 Type-C，接 DAPLink 烧录/调试 |
| **USART2** | **PA2(TX) / PA3(RX)** | 115200 | 引出排针，外接 **蓝牙 / 2.4G 无线串口模块**，用于 **OTA 在线升级** |

> IAP 升级走 **USART2**（`iap_port.c` 里 `IAP_UART = huart2`）。接无线模块后可实现**无线 OTA**，不用连 USB 线。

## 目录结构

```
IAP Bootloader/
├── README.md                          ← 本文档(总览)
├── bootloader/                        ← Bootloader 固件工程 (Keil)
│   ├── README.md                      ← Bootloader 详解 / 移植指南
│   ├── Core/                          ← CubeMX 生成代码(main/usart/gpio...)
│   ├── IAP/                           ← IAP 核心逻辑 + 移植层(可整体复制)
│   ├── bsp/                           ← 板级支持(led)
│   └── MDK-ARM/                       ← Keil 工程文件
├── iap_tool/                          ← PC 上位机工具 (Python)
│   ├── README.md                      ← 上位机使用说明
│   └── iap_upload.py                  ← 烧录脚本
└── national_competition_test_1/       ← App 固件工程(示例, 完整 Keil 工程)
    ├── README.md                      ← App 工程说明
    ├── Core/  MDK-ARM/  ...           ← CubeMX + Keil 工程文件
```

## 工作原理

```
上电复位
  │
  ├─ 未按按键 ───────────► 跳转到 App 运行
  │                          (若 App 无效则驻留 BL, 作为安全兜底)
  │
  └─ 按住按键(PB0) ──────► 进入 Bootloader 升级模式
                           等待上位机: 握手→擦除→数据→校验→跳转
```

- **Bootloader** 占用 Flash 前 16KB，负责引导 + 串口升级
- **App** 从 0x08004000 开始，正常运行业务逻辑
- 升级通过 **USART2 (PA2/PA3)** 进行（可外接蓝牙/2.4G 无线模块实现无线 OTA），协议带 CRC 校验和 ACK/NAK 重传

## 内存分布

| 区域 | 起始地址 | 结束地址 | 大小 | 用途 |
|------|----------|----------|------|------|
| Bootloader | 0x08000000 | 0x08003FFF | 16KB | 引导 + IAP 升级代码 |
| App | 0x08004000 | 0x0801FFFF | 112KB | 应用固件 |
| RAM | 0x20000000 | 0x20007FFF | 32KB | 数据区 |

> 注: 当前为 **BL 单区 + App 单区**，未实现 A/B 双备份(升级失败无回滚)。

## 快速上手

### 1. 烧录 Bootloader（一次性）
- 用 Keil 打开 `bootloader/MDK-ARM/bootloader.uvprojx`，编译
- 通过板载 USB（USART1/GD32→Type-C）连 DAPLink 下载到板子
- 之后 App 升级走无线 OTA，不再需要下载器

### 2. 编译 App
- 用 Keil 打开 `national_competition_test_1` 工程
- 确认链接地址为 `0x08004000`，编译生成 `.bin`
- 生成 bin 的 Keil5 配置：`Options for Target → User → After Build` 填 `fromelf --bin -o "$L@L.bin" "#L"`（详见 App 工程 README）

### 3. 串口升级

有线（USB 转 TTL 接 PA2/PA3，或接无线模块对端）:
```
cd iap_tool
D:\Python\python.exe iap_upload.py D:/你的/app.bin COM18
```
- 按提示进入 Bootloader 模式（按住 PB0 上电，LED 闪烁）
- 回车开始烧录，等待"已跳转 App"

详细步骤、无线模块配对见各子目录 README。

## 通信协议（简述）

```
帧格式:  AA 55  CMD  LEN_LO  LEN_HI  DATA[0~256]  CRC_LO  CRC_HI
```

| CMD | 含义 |
|-----|------|
| 0x01 | 握手 |
| 0x02 | 擦除 App 区 |
| 0x03 | 写数据块(≤256B) |
| 0x04 | 结束 + 整包 CRC 校验 |
| 0x05 | 跳转 App |

- 校验: CRC16-CCITT(0x1021, 初值 0xFFFF)
- 回复: `0x06`=ACK, `0x15`=NAK
- 波特率: 115200, 8N1

## 详细文档

- [Bootloader 固件详解与移植指南](./bootloader/README.md)
- [PC 上位机使用说明](./iap_tool/README.md)
- [App 工程说明](./national_competition_test_1/README.md)
