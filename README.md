# STM32 快跑游戏 LoRa OTA 远程升级

STM32F103C8T6 在 OLED 上运行小鹿/恐龙快跑，LabVIEW 通过无线串口发送 `S/J/P/R` 控制；按升级键并重启后，Python 工具经 ATK-MWCC68D 分包传输新固件。新固件先完整写入片内暂存区，CRC32 校验通过后再安装到应用区。

## 先使用预编译交付包

[`releases/2026-10-15/`](releases/2026-10-15/) 已提供 `factory_deer.hex`、`factory_dinosaur.hex`、`deer_v1.bin`、`dinosaur_v2.bin`、`manifest.json` 和 `SHA256SUMS.txt`。该版针对用户确认的 SSD1306 屏幕取消错误的两列偏移，版本号移到右上角；血量移到右下角以避开版本号。首次测试不需要安装 Keil 或 ARM 编译器：用 ST-Link 烧录合并 HEX，再用 Python 工具发送应用 BIN。`manifest.json` 记录版本、长度和 CRC32，`SHA256SUMS.txt` 可核对交付文件是否改变。旧的 `2026-10-13` 包保留作历史记录。

已烧录旧包的开发板，这次建议用 ST-Link 全片擦除后烧录新合并 HEX，再复位检查白线是否消失及右上角版本号，因为 Bootloader 和游戏共用的 OLED 驱动都已修正。新版实屏显示仍待复烧确认，电脑画面预览见 [OLED 修正记录](docs/implementation/2026-10-15-oled-fix.md)。

安装带 Tk 的 Python，在本工程根目录依次运行：

```powershell
python -m pip install -r requirements.txt
python -m host.gui
```

GUI 选择 `releases/2026-10-15/dinosaur_v2.bin` 时会从文件名填入版本 2；`deer_v1.bin` 对应版本 1。仍需核对界面版本、所选文件和目标设备。

## 打开和构建（重新编译时）

打开根目录 `GameOTA.uvprojx`，三个 Keil 目标是 `boot`、`deer`、`dinosaur`，使用 ARM Compiler 5。默认外部晶振8MHz，主频72MHz。也可在仓库根目录运行：

```powershell
python tools/build.py
# 不同工具链位置：
python tools/build.py --toolchain D:/keil5/ARM/ARMCC/bin
```

`dist/` 生成 `factory_deer.hex`、`factory_dinosaur.hex`、各目标 BIN/HEX/AXF/MAP 及 `manifest.json`，同时生成 `deer_v1.bin`、`dinosaur_v2.bin`；保留的 `deer.bin`、`dinosaur.bin` 与对应版本别名内容相同。首次烧录使用合并的 `factory_deer.hex`；升级推荐使用带版本的 BIN 文件名。独立 `boot.hex`、游戏 HEX 不包含完整首次初始化状态，不能当作首次合并包。AXF/MAP 仅为本地调试产物，不包含在预编译交付包中。

VS Code可打开`GameOTA.code-workspace`，通过任务运行完整构建、软件验证和升级GUI；构建直接使用上述Python脚本，不继承原工程中已经失效的DMP/算法文件引用。

分区已固定，ROM超限构建失败。不要修改一个目标的地址后继续使用旧升级包。只打开Keil编译后，可运行Python构建命令生成合并包和经过检查的发布文件。

## 接线及模块参数

| 设备 | C8T6引脚 |
| --- | --- |
| OLED SCL / SDA | PB6 / PB7 |
| LoRa RXD / TXD | PA9 / PA10，交叉连接 |
| 升级按键 | PA0与GND；内部上拉，按下低电平 |
| SWDIO / SWCLK | PA13 / PA14 |

STM32、OLED、LoRa共地。模块供电按 ATK-MWCC68D 手册提供，UART信号为3.3V电平；需要足够的发射供电电流并安装天线。不要让另一只USB-TTL的TX和LoRa的TX同时连接PA10。

电脑端为 USB-TTL→第二只LoRa模块，两端先用官方工具设置普通透传、UART **9600、8N1**，并保证地址/网络/信道/空中速率等通信参数匹配；具体可选值和配置方法按 ATK-MWCC68D 配套手册与配置工具确认。模式脚依照配套手册固定到正常透传，本工程不自动发送模块AT命令，也不使用模块自身的固件下载模式。先用串口工具完成两端双向文本透传，再进行MCU升级。

LabVIEW原按键VI继续发送大写ASCII命令：`S`开始/继续、`J`跳跃、`P`暂停/继续、`R`复位游戏。电脑的串口工具、LabVIEW和Python升级器一次只打开一个。原VI在`C:/毕业设计_许剑峰/labview/labview/按键.vi`，未修改它的内部逻辑。

## 首次烧录与升级

1. 使用ST-Link/SWD连接C8T6；BOOT0置低。**使用合并包重新初始化前，完整擦除这块目标MCU的用户Flash**，清掉旧状态记录，再烧录 `releases/2026-10-15/factory_deer.hex`（重新构建时可用 `dist/factory_deer.hex`）。首次初始化或重刷合并包时执行；日常LoRa升级不擦Bootloader。
2. 复位后Bootloader显示3秒按键窗口，未按键自动运行小鹿版，屏幕显示 `V1`。
3. 游戏运行时按PA0，屏幕显示升级请求已保存及重启提示；按复位键或断电重启进入 `WAITING FOR BIN`。也可在开机3秒窗口按PA0。
4. 关闭 LabVIEW 和其他占用该串口的工具，安装上述 Python 依赖后运行 `python -m host.gui`，选择电脑端LoRa的COM口，查询设备，选择 `releases/2026-10-15/dinosaur_v2.bin`（重新构建时用 `dist/dinosaur_v2.bin`），核对固件版本2，开始升级。游戏中的按键仅保存请求并显示重启提示，必须手动复位后才能发送固件。
5. 工具先显示传输进度，再等待校验和安装结果；设备安装成功后保留约10秒用于结果应答重试，再重启运行恐龙版 `V2`。如果回到小鹿，选择 `deer_v1.bin`、版本1。`WAITING FOR BIN` 模式会持续等待，不受正常开机3秒窗口限制。

命令行的具体选项可通过 `python -m host.cli --help` 查看：

```powershell
python -m host.cli --port COM3 query
python -m host.cli --port COM3 upload releases/2026-10-15/dinosaur_v2.bin --version 2
```

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
- 安装状态下暂存固件损坏：先验证暂存源，再检查旧 A 区。源确实无效且 A 区检查完成后，持久化升级请求，保留经验证的旧应用或记录为空，等待新传输；任何读取/安装写入失败都保留待安装候选，供下次恢复重试。
- 状态写入断电：双页日志校验CRC和最后写入的提交标记，选择最新完整记录。
- A区无效：留在Bootloader等待修复，不跳入不完整程序。

应用 BIN 最大 23KB（23552 字节），两版当前各 19120 字节。这份暂存固件用于完成当前安装，不是出厂备份；没有第三个可运行的出厂程序，也不实现新版本逻辑故障的自动回滚。CRC32负责传输完整性，尚未实现固件签名认证。

## 验证与代码来源

```powershell
python tools/verify.py
python tools/build.py
```

电脑测试直接运行用于MCU的便携C升级核心，模拟64KB片内Flash、半字写入、页擦除与断电，并将Python上位机连接到同一C模拟器。ST-Link烧录、LoRa实际射频收发、真实硬件掉电属于板上验收，见 `docs/validation.md`；电脑测试不替代这些项目。

`firmware/legacy`保留用户提供的原游戏及ST标准库来源和原版权声明；在该副本中调整了启动栈、VTOR配置，以及 SSD1306 列地址和初始化点亮顺序。`firmware/app/game.c`是适配后的游戏；`firmware/common`是新增升级核心，`firmware/port`是硬件适配，`firmware/boot`是B区启动程序。CAN/HAL参考工程只作方案参考，没有直接复制其擦写与校验实现。原电脑目录中的游戏工程保持原样。
