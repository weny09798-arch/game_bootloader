# STM32 快跑游戏 LoRa OTA 远程升级

STM32F103C8T6 在 OLED 上运行小鹿/恐龙快跑，LabVIEW 通过无线串口发送 `S/J/P/R` 控制；按升级键并重启后，Python 工具经 ATK-MWCC68D 分包传输新固件。新固件先完整写入片内暂存区，CRC32 校验通过后再安装到应用区。

## 打开和构建

打开根目录 `GameOTA.uvprojx`，三个 Keil 目标是 `boot`、`deer`、`dinosaur`，使用 ARM Compiler 5。默认外部晶振8MHz，主频72MHz。也可在仓库根目录运行：

```powershell
python tools/build.py
# 不同工具链位置：
python tools/build.py --toolchain D:/keil5/ARM/ARMCC/bin
```

`dist/` 生成 `factory_deer.hex`、`factory_dinosaur.hex`、各目标 BIN/HEX/AXF/MAP 及 `manifest.json`。首次烧录使用合并的 `factory_deer.hex`；升级发送 `dinosaur.bin` 或 `deer.bin`。独立 `boot.hex`、游戏 HEX 不包含完整首次初始化状态，不能当作首次合并包。

分区已固定，ROM超限构建失败。不要修改一个目标的地址后继续使用旧升级包。只打开Keil编译后，可运行Python构建命令生成合并包和经过检查的发布文件。

## 接线及模块参数

| 设备 | C8T6引脚 |
| --- | --- |
| OLED SCL / SDA | PB6 / PB7 |
| LoRa RXD / TXD | PA9 / PA10，交叉连接 |
| 升级按键 | PA0与GND；内部上拉，按下低电平 |
| SWDIO / SWCLK | PA13 / PA14 |

STM32、OLED、LoRa共地。模块供电按 ATK-MWCC68D 手册提供，UART信号为3.3V电平；需要足够的发射供电电流并安装天线。不要让另一只USB-TTL的TX和LoRa的TX同时连接PA10。

电脑端为 USB-TTL→第二只LoRa模块，两端先用官方工具设置普通透传、UART **9600、8N1**，并保证地址/网络/信道/空中速率等通信参数一致。建议初次近距离测试使用19.2kbps空中速率；确切可选值以该型号配套配置工具为准。模式脚依照配套手册固定到正常透传，本工程不自动发送模块AT命令，也不使用模块自身的固件下载模式。先用串口工具完成两端双向文本透传，再进行MCU升级。

LabVIEW原按键VI继续发送大写ASCII命令：`S`开始/继续、`J`跳跃、`P`暂停/继续、`R`复位游戏。电脑的串口工具、LabVIEW和Python升级器一次只打开一个。原VI在`C:/毕业设计_许剑峰/labview/labview/按键.vi`，未修改它的内部逻辑。

## 首次烧录与升级

1. 使用ST-Link/SWD连接C8T6；BOOT0置低。**首次使用合并包前，完整擦除这块目标MCU的用户Flash**，清掉旧状态记录，再烧录 `dist/factory_deer.hex`。这是一次性初始化；日常LoRa升级不擦Bootloader。
2. 复位后Bootloader显示3秒按键窗口，未按键自动运行小鹿版，屏幕显示 `V1`。
3. 游戏运行时按PA0，屏幕显示升级请求已保存及重启提示；按复位键或断电重启进入 `WAITING FOR BIN`。也可在开机3秒窗口按PA0。
4. 关闭LabVIEW占用的串口，运行 `python -m host.gui`，选择电脑端LoRa的COM口，查询设备，选择 `dist/dinosaur.bin`，固件版本填2，开始升级。
5. 工具先显示传输进度，再等待校验和安装结果；设备安装成功后保留约10秒用于结果应答重试，再重启运行恐龙版 `V2`。如果回到小鹿，选择 `deer.bin`、版本1。

命令行的具体选项可通过 `python -m host.cli --help` 查看。Python依赖安装：`python -m pip install -r requirements.txt`。

## 分区与恢复能力

| 区域 | 起始地址 | 大小 |
| --- | --- | --- |
| B：Bootloader | 0x08000000 |16KB|
| A：游戏 | 0x08004000 |23KB|
| 新固件暂存 | 0x08009C00 |23KB|
| 双页状态日志 | 0x0800F800 / 0x0800FC00 |各1KB|

两版游戏都链接到A区，向量表偏移0x4000。每个数据包最多128字节，带会话/序号/偏移和CRC，收到正确应答才发送下一包。3秒仅用于正常启动选择；进入升级模式后持续等待上位机。

- 接收阶段断电：旧应用保留，下次重启从头接收。
- 安装阶段断电：状态已经持久化，下次上电验证暂存固件，从第一页重新安装。
- 状态写入断电：双页日志校验CRC和最后写入的提交标记，选择最新完整记录。
- A区无效：留在Bootloader等待修复，不跳入不完整程序。

这份暂存固件用于完成当前安装，不是出厂备份；没有第三个可运行的出厂程序，也不实现新版本逻辑故障的自动回滚。CRC32负责传输完整性，尚未实现固件签名认证。

## 验证与代码来源

```powershell
python tools/verify.py
python tools/build.py
```

电脑测试直接运行用于MCU的便携C升级核心，模拟64KB片内Flash、半字写入、页擦除与断电，并将Python上位机连接到同一C模拟器。ST-Link烧录、LoRa实际射频收发、真实硬件掉电属于板上验收，见 `docs/validation.md`；电脑测试不替代这些项目。

`firmware/legacy`保留用户提供的原游戏及ST标准库来源和原版权声明；在该副本中只对启动栈和VTOR配置做迁移所需调整。`firmware/app/game.c`是适配后的游戏；`firmware/common`是新增升级核心，`firmware/port`是硬件适配，`firmware/boot`是B区启动程序。CAN/HAL参考工程只作方案参考，没有直接复制其擦写与校验实现。原电脑目录中的游戏工程保持原样。
