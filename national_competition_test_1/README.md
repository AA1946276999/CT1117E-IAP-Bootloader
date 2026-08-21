# App 工程说明（national_competition_test_1）

应用固件示例工程，通过 Bootloader 引导运行，并支持串口在线升级。

## 功能定位

- 本工程是 **App（应用固件）**，从 `0x08004000` 开始运行
- 上电后由 Bootloader 判定：未按按键 → 直接跳转到 App；按住按键 → 进入升级模式
- App 本身不包含引导逻辑，启动完全依赖 Bootloader

## 功能简介

蓝桥杯嵌入式练习程序，主要功能：

- **LCD 显示**：IDLE / PARA / OUTP / RECD 等多个界面
- **按键交互**：模式切换、参数选择、密码校验
- **ADC**：读取电位器等模拟量
- **RTC**：日期时间显示与记录
- **PWM**：PA6 / PA7 输出
- **串口**：USART1 收发命令

## 链接地址要求

App 必须链接在 Bootloader 分区之后。当前分区：

| 区域 | 地址范围 | 大小 |
|------|----------|------|
| Bootloader | 0x08000000 ~ 0x08003FFF | 16KB |
| **App** | **0x08004000** ~ 0x0801FFFF | 112KB |

Keil scatter 文件关键行：

```
LR_IROM1 0x08004000 0x0001C000  {
  ER_IROM1 0x08004000 0x0001C000  {
   *.o (RESET, +First)
   ...
  }
  ...
}
```

**如果 Bootloader 分区大小变了，App 链接地址必须跟着改。**

## 与 Bootloader 的配合

### 中断向量表 (VTOR)

- Bootloader 跳转前会设置 `SCB->VTOR = 0x08004000`
- 因此 App 的 `system_stm32g4xx.c` 里 `USER_VECT_TAB_ADDRESS` 处于注释状态时也能正常工作（依赖 Bootloader 已设好 VTOR）
- 若希望 App 自包含（不依赖 Bootloader），可在 `system_stm32g4xx.c` 里定义:
  ```c
  #define USER_VECT_TAB_ADDRESS
  #define VECT_TAB_OFFSET 0x4000
  ```

### 中断状态

- Bootloader 跳转前会恢复全局中断（`__enable_irq()`），App 启动时中断状态与上电复位一致，无需额外处理

## 串口配置

| 串口 | 引脚 | 波特率 | 用途 |
|------|------|--------|------|
| USART1 | App 工程自定义 | 9600 | App 自身业务通信 |
| USART2 | PA2/PA3 | 115200 | **Bootloader OTA 升级串口**（App 不参与） |

- 两者是**不同的 USART**，引脚、波特率、用途互不冲突
- 升级时由 Bootloader 使用 USART2，App 不参与

## 使用 Bootloader 升级

### 1. 用 Keil5 生成 .bin

配置一条"编译后自动生成 bin"的命令：

1. Keil 菜单 **Options for Target → User** 标签页
2. 找到 **After Build/Rebuild**，勾选某一条 **User Command**
3. 填入：

```
fromelf --bin -o "$L@L.bin" "#L"
```

4. 编译完成后，输出目录里自动生成 `<工程名>.bin`

**命令解析**：

| 部分 | 含义 |
|------|------|
| `fromelf` | ARM 编译器自带的镜像转换工具 |
| `--bin` | 输出原始二进制格式 |
| `-o "文件"` | 指定输出文件名 |
| `$L` | Keil 变量：输出目录 |
| `@L` | Keil 变量：目标文件名（不含扩展名） |
| `#L` | Keil 变量：链接产物 `.axf` 的完整路径 |

> 若报错 `CreateProcess failed` 或找不到 `fromelf`，说明它不在 PATH 里，改用**完整路径**：
> ```
> "D:\004\51\ARM\ARMCC\Bin\fromelf.exe" --bin -o "$L@L.bin" "#L"
> ```
> （路径以你实际安装位置为准）

### 2. 串口升级

```
python ..\iap_tool\iap_upload.py ..\national_competition_test_1\MDK-ARM\national_competition_test_1\national_competition_test_1.bin COM18
```

1. 先把生成的 `.bin` 路径传给脚本（可换你自己的串口号）
2. **按住按键(PB0)上电**进入 Bootloader 模式，LED 闪烁
3. 脚本提示后按回车，等待 `握手成功 → 擦除完成 → 进度 → 整包校验通过 → 已跳转 App`

详细说明见 `../iap_tool/README.md`。

## 注意事项

- App `.bin` 大小不能超过 App 区(112KB)
- App 链接地址固定为 `0x08004000`，**不会覆盖** Bootloader 区(0x08000000~0x08003FFF)
- 升级中途掉电/失败会导致 App 损坏（当前无 A/B 备份），需要重新进 BL 完整烧录
- 重新编译后若地址配置变化，需重新烧录 Bootloader 并同步 App 地址
