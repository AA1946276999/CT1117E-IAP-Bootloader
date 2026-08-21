# Bootloader 固件详解与移植指南

STM32G431RBT6 的 Bootloader（蓝桥杯嵌入式 CT117E 开发板），占用 Flash 前 16KB。负责引导 App，并通过 **USART2 (PA2/PA3)** 实现串口在线升级（可接蓝牙/2.4G 无线模块做无线 OTA）。

## 工程结构

```
bootloader/
├── Core/
│   ├── Inc/                      CubeMX 生成的头文件
│   └── Src/
│       ├── main.c                主流程(固定模板, 只调 IAP/移植层函数)
│       ├── usart.c               USART1/2 初始化(115200, USART2 PA2/PA3 用于 OTA)
│       ├── gpio.c                按键 PB0、LED 引脚配置
│       └── stm32g4xx_it.c        中断服务(USART1/USART2_IRQHandler)
├── IAP/                          核心 + 移植层(可整体复制到其他工程)
│   ├── iap.h                     配置宏 + 协议常量 + 核心 API
│   ├── iap.c                     状态机 / CRC16 / Flash擦写 / 命令处理 / 跳转
│   ├── iap_port.h                移植层接口声明
│   └── iap_port.c                串口/按键/LED 适配(移植时只改这里)
├── bsp/
│   ├── led.c / led.h             板级 LED 驱动(PC8~PC15 + PD2 锁存)
└── MDK-ARM/                      Keil 工程 + scatter 文件
```

## 引脚配置

| 功能 | 引脚 | 说明 |
|------|------|------|
| USART2_TX | PA2 | OTA 升级串口发送(接无线模块 RX) |
| USART2_RX | PA3 | OTA 升级串口接收(接无线模块 TX) |
| USART1_TX/RX | PA9/PA10 | 板载 USB(GD32→Type-C)，接 DAPLink 烧录/调试 |
| 按键 | PB0 | 低有效, 上电按住进入升级模式 |
| LED | PC8~PC15 + PD2 | 指示灯(PD2 为锁存信号) |

> IAP 升级走 **USART2**。PA2/PA3 引出排针，可接 **蓝牙 / 2.4G 无线串口模块**实现无线 OTA。

## 系统配置

- 主时钟: **HSI 16MHz**(不使用 HSE/PLL)
- 串口: **USART2, 115200, 8N1**, 中断优先级 6（USART1 优先级 5 用于板载 USB）
- Flash: 页大小 **2KB**, 编程粒度 **64bit 双字**

## 内存分区

| 区域 | 地址范围 | 大小 |
|------|----------|------|
| Bootloader | 0x08000000 ~ 0x08003FFF | 16KB (scatter 里 0x4000) |
| App | 0x08004000 ~ 0x0801FFFF | 112KB |

分区依据 `MDK-ARM/bootloader/bootloader.sct`:

```
LR_IROM1 0x08000000 0x00004000   ← Bootloader 只占 16KB
```

App 区从 `0x08004000` 开始，对应 `iap.h` 里的 `IAP_APP_BASE`。

## 启动流程

```
main():
  HAL_Init() → SystemClock_Config() → MX_GPIO_Init()
  → MX_USART1_UART_Init() → MX_USART2_UART_Init()
    │
    ├─ IAP_Port_IsButtonPressed() 为假(没按键) → IAP_JumpToApplication() 跳 App
    │
    └─ 按键按下 → IAP_Init() → IAP_Port_RxStart() → while(IAP_Process())
                  ↑ 进入升级模式: LED 闪烁, 等待上位机
```

`main.c` 里的固定模板（USER CODE 2，换芯片不用改）：

```c
led_all_off();
if (!IAP_Port_IsButtonPressed())
    IAP_JumpToApplication();
IAP_Init();
IAP_Port_RxStart();
while (1) { IAP_Process(); /* LED 闪烁 */ }
```

跳转流程（`iap.c` 的 `IAP_JumpToApplication`）:
1. 检查 App 起始地址第一个字(SP)是否在 RAM 范围
2. 关中断 → 设 `SCB->VTOR = 0x08004000` → 切换 MSP → 恢复中断
3. 跳转到 App 复位入口

> 注意: 跳转前必须 `__enable_irq()`，否则 App 中断全关(会卡死在 HAL_Delay)。

## 通信协议

### 帧格式

```
字节:   0     1     2    3    4    5...5+LEN-1   5+LEN    6+LEN
      ┌─────┬─────┬────┬─────┬─────┬─────────────┬────────┬────────┐
      │ 0xAA│ 0x55│ CMD│LEN_LO│LEN_HI│   DATA      │CRC_LO  │ CRC_HI │
      └─────┴─────┴────┴─────┴─────┴─────────────┴────────┴────────┘
        帧头    命令  数据长度(小端,≤256)  数据      CRC16(小端)
```

### 命令

| CMD | 含义 | DATA 内容 | 回复 |
|-----|------|-----------|------|
| 0x01 | 握手 | 空 | ACK |
| 0x02 | 擦除 App 区 | 空 | ACK/NAK |
| 0x03 | 写数据块 | ≤256B 数据 | ACK/NAK |
| 0x04 | 结束 | 总长度(4B)+整包CRC(2B) | ACK/NAK |
| 0x05 | 跳转 App | 空 | ACK(校验通过才放行) |

回复: `0x06`=ACK, `0x15`=NAK。

### CRC16

- 算法: CRC16-CCITT, 多项式 `0x1021`, 初值 `0xFFFF`
- 每帧对"帧头~DATA"做 CRC；整包对所有数据字节累加做 CRC

### 升级时序

```
PC 上位机                     Bootloader
    │                             │
    ├── 握手(0x01) ──────────────►│ 回 ACK(进 BL 先发 "BL\r\n")
    ├── 擦除(0x02) ──────────────►│ 按页擦 App 区, 回 ACK
    ├── 数据帧(0x03) ◄─ACK────────┤ 每 8B 一个双字写 Flash, 回 ACK
    │     ... 循环 256B/帧 ...
    ├── 结束(0x04, 带整包CRC) ───►│ 补写余数+整包校验, 回 ACK
    ├── 跳转(0x05) ─────────────►│ 校验通过才 ACK, 然后跳转 App
    │                             │
```

## 编译与烧录

1. 用 Keil 打开 `MDK-ARM/bootloader.uvprojx`，编译（目标 `bootloader`）
2. 通过板载 USB（USART1/GD32→Type-C）连接 DAPLink，Keil 里 **Download** 烧录到芯片
3. 之后 App 的升级就走 USART2 无线 OTA，不再需要下载器

## 分层架构（核心思想）

```
iap.c(核心)     协议/状态机/CRC/Flash/跳转    —— 与硬件无关, 不碰 UART
    │ 依赖
iap_port.h     移植层接口
    │ 实现
iap_port.c     串口收发/按键/LED/延时          —— 唯一要改的硬件适配
    │
main.c         固定模板, 只调 iap / iap_port 函数
```

**好处**: 换 MCU 或换 UART 时，核心逻辑一行不用改。

## 移植指南（换 STM32 或换工程）

### 步骤

1. **复制 `IAP/` 文件夹**到新工程，加入 Keil 工程并添加 `../IAP` 到 include 路径
2. **改 `iap.h`** 三个宏:

```c
#define IAP_APP_BASE        0x08004000UL   /* App 起始地址(和 BL 分区对应) */
#define IAP_APP_END         0x08020000UL   /* App 结束地址 = 芯片 Flash 上限 */
#define IAP_FLASH_PAGE_SIZE 2048UL         /* 芯片 Flash 页大小 */
```

3. **改 `iap_port.c`** 顶部:

```c
#define IAP_UART  huart2   /* 改成你的 UART 句柄(OTA 用) */
```
   以及按键引脚、LED 等硬件适配。

4. **CubeMX 配置**: 重配 USART 引脚、波特率(和上位机一致)、按键引脚
5. **若 Flash 擦除/编程 API 不同**(如 F4 扇区、H7 块): 改 `iap.c` 里的 `iap_erase_app` 和双字编程部分
6. **上位机**: `iap_tool/iap_upload.py` 里把 `BAUD` 改成和固件一致

### 各芯片 Flash 差异速查

| 芯片 | Flash 起始 | 总大小 | 擦除单元 | 编程粒度 |
|------|-----------|--------|----------|----------|
| G431 | 0x08000000 | 128KB | 2KB 页 | 64bit 双字 |
| G0 | 0x08000000 | 64KB | 2KB 页 | 64bit 双字 |
| L4 | 0x08000000 | 128KB+ | 2KB 页 | 64bit 双字 |
| F407 | 0x08000000 | 1MB | 16KB 扇区 | 32bit 字 |
| F1 | 0x08000000 | 64KB+ | 1KB 页 | 16bit 半字 |

## 常见问题

- **握手失败**: 确认板子已进 BL(按住按键上电,LED 闪烁)、串口号/波特率正确、无线模块已配对
- **用串口助手先发过握手会残留 `BL\r\n`**: 上位机脚本在回车后会 `reset_input_buffer()` 清掉，无影响
- **App 起不来**: 检查跳转前是否 `__enable_irq()`; 检查 App 是否链接在 `0x08004000`
- **超 16KB**: Bootloader 代码 + 库过大，需扩大分区(改 scatter + `iap.h` + App 地址)
- **无线 OTA 不稳定**: 降低波特率或用有线 USB 转 TTL 排查
