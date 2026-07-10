# PX4 二次开发流程记录（STM32H743IIT6 最小系统板）

## 0. 阅读约定
每次继续 PX4 二次开发前，先读本文档，再继续操作。不要脱离已有上下文重新分析。

本文档是主索引，只放长期有效的上下文、规则、流程和索引；阶段性细节放在 `px4_flow_logs/` 子 md 中。整个 H743mini 自定义板级开发文档位于 PX4 仓库的 `board_docs/h743mini-development/`，与 PX4 上游官方 `docs/` 分开管理。

当前阶段最重要的约束先记住：

- TF 卡、`/fs/microsd`、`params`、`dataman`、`logger` 相关验证，当前都应以“完全断电后重新上电”的冷启动结果为准。
- `reboot` 或板载复位键后的 TF 卡状态当前仍不可靠，不要直接拿这类结果判断 TF 卡链路是否已经损坏。

阅读顺序建议：

1. 先看“当前阶段启动约束”。
2. 再看“当前状态”。
3. 再看“下一阶段任务和目标”。
4. 如果要烧录，看“烧录与启动原则”。
5. 如果要更新进展，看“进度更新规则”。
6. 如果要继续适配外设，看“逐步适配外设”。
7. 如果遇到问题，看“调试方法”和“常见坑”。

## 1. 当前状态
当前阶段：H743 最小系统板已经完成 bootloader 与主程序烧录，PX4 主程序可启动，USB/QGC/NSH 调试链路已打通。TF 卡、`/fs/microsd`、参数导入、`dataman`、`logger` 已在“完全断电后重新上电”的冷启动场景验证可用，但 `reboot` 或板载复位键后的 TF 卡状态仍不可靠。BL24C16F EEPROM 已完成 I2C2、AT24C16 bank 地址协议和 PX4 MTD 分区验证。GD25Q128 QSPI Flash 已完成 JEDEC 识别、QSPI 驱动修复、PX4 MTD 接入、`/fs/mtd_params` 参数持久化和 `/fs/mtd_waypoints` 备用分区验证。当前已完成第一版飞控传感器通信与引脚规划，并已在杜邦线临时外接条件下调通 `ICM42688P` 主 IMU：`SPI2 PC2/PC3/PD3(P2-36) + PE4 CS + PE6 DRDY`，当前稳定工作点为 `2 MHz SPI + 4 kHz ODR + 800 Hz FIFO 读取/发布 + SPI2 DMA`。`PB14 / TIM12_CH1` 蜂鸣器链路已完成 PX4 `tone_alarm` 适配、H7 TIM12 RCC 兼容补丁、编译验证和上板有声验证。4G MAVLink 远程数传已完成第一版引脚规划：保留 JY901B 的 `UART4 PH13/PH14`，新增 UART5 备用复用 `PB13/PB12` 分配给 4G DTU，当前仍处于文档规划阶段，尚未改源码和上板验证。由于现阶段仍是杜邦线连接，SPI 高频余量需要等 PCB 画完、走线和供电条件稳定后再重新测试。当前主线进入“保持 ICM 稳定参数、继续验证 EKF2，并接入 GPS/IST8310/JY901B 等真实外设”的阶段。

当前优先烧录方式：

```text
ST-Link / SWD + STM32CubeProgrammer CLI
```

当前优先调试方式：

```text
QGroundControl USB MAVLink + CH340 USART1 NSH
```

已完成：

- 已确认 PX4-Autopilot 源码路径：`/home/gjl/PX4-Autopilot`。
- 当前 Git 分支：`h743mini-v1.13.3`。
- 已成功编译官方目标：`make px4_fmu-v6c_default`。
- 已复制并识别自定义 board target：`gjl_h743mini`。
- 已成功编译自定义目标：`make gjl_h743mini_default`。
- 已完成对 `boards/gjl/h743mini` 的第一次只读关键词检查。
- 已确认 H743 最小系统板资料路径：`/media/gjl/新加卷/BaiduNetdiskDownload/慧勤智远 STM32H743IIT6 CB V1.5小系统板`。
- 已完成最小系统板资料目录只读检查，已发现用户手册和原理图 PDF。
- 已从原理图片段确认关键硬件信息：HSE 为 25 MHz，LSE 为 32.768 kHz，CH340 COM 口接 USART1，USB FS 接 PA11/PA12，SWD 和 BOOT0/NRST 已引出，板载 RGB 的 G/B 由 PB0/PB1 控制。
- 已将 `CONFIG_ARCH_BOARD_CUSTOM_DIR` 修正为自定义 board 路径，并重新编译通过。
- 已将 HSE/PLL 时钟配置按 25 MHz 晶振方向调整，并编译通过。
- 已将 console 调试串口从 V6C 默认 USART3 方向调整到板载 CH340 对应的 USART1 PA9/PA10，并编译通过。
- 已修正 USB VBUS 方向：当前板子 PA9 是 USART1_TX，不是 USB VBUS sense。
- 已编译 `gjl_h743mini_bootloader`。
- 已编译 `gjl_h743mini_default`。
- 已通过 STM32 ROM DFU 将 PX4 bootloader 烧录到 `0x08000000`。
- 已通过 STM32 ROM DFU 将 PX4 主程序烧录到 `0x08020000`。
- 已通过 ST-Link / SWD 将 PX4 bootloader 重新烧录到 `0x08000000`，并校验成功。
- 已通过 ST-Link / SWD 将 PX4 主程序重新烧录到 `0x08020000`，并校验成功。
- 已安装 Python `pyserial`，修复 `px_uploader.py` 一直停在 `waiting for the bootloader...` 的电脑端依赖问题。
- 已通过 `make gjl_h743mini_default upload` 跑通 PX4 bootloader upload，完成擦除、写入、校验和重启。
- 已确认 USB 枚举为 `Auterion PX4 FMU v6C.x`，设备为 `/dev/ttyACM0`。
- 已确认 QGroundControl 能连接飞控。
- 已确认 NSH 可进入，并能执行 `dmesg`、`ver all`、`mavlink status`、`sensors status`、`commander status`。
- 已验证：完全断电冷启动后，`/fs/microsd` 可挂载为 `vfat`，`/fs/microsd/params` 可导入，`dataman` 与 `logger` 可正常使用 TF 卡。
- 已确认：NSH `reboot` 或板载复位键后，TF 卡当前仍可能不 ready；当前阶段所有 TF 卡 / 参数保存 / `dataman` / `logger` 相关验证，都应以完全断电冷启动结果为准。
- 已生成官方 V6C 编译产物：
  - `build/px4_fmu-v6c_default/px4_fmu-v6c_default.elf`
  - `build/px4_fmu-v6c_default/px4_fmu-v6c_default.bin`
  - `build/px4_fmu-v6c_default/px4_fmu-v6c_default.px4`
- 已生成自定义 target 编译产物：
  - `build/gjl_h743mini_default/gjl_h743mini_default.elf`
  - `build/gjl_h743mini_default/gjl_h743mini_default.bin`
  - `build/gjl_h743mini_default/gjl_h743mini_default.px4`
- 最新已验证编译结果：
  - `gjl_h743mini_bootloader`：flash 使用约 `34.76%`。
  - `gjl_h743mini_default`：FLASH 使用约 `88.42%`。
- 已完成 BL24C16F EEPROM 适配：
  - I2C2 映射到 `PH4/PH5`，`i2cdetect -b 2` 可扫到 `0x50~0x57`。
  - `px4_24xxxx_mtd.c` 已适配 AT24C16/BL24C16F 的 banked I2C address + 1 字节 word address。
  - `CONFIG_AT24XX_SIZE=16` 已通过 `default.px4board` 的 `CONFIG_BOARD_COMPILE_DEFINITIONS` 进入编译。
  - MTD 几何已验证为 `16 bytes * 128 blocks = 2048 bytes`。
  - EEPROM 当前分区为 `/fs/mtd_caldata` 112 blocks / 1792 bytes，`/fs/mtd_id` 16 blocks / 256 bytes。
  - `mtd readtest` 与 `mtd rwtest` 已通过。
- 已完成 GD25Q128 QSPI Flash 基础适配：
  - QSPI 引脚已按原理图映射到 `PF8/PF9/PF7/PF6/PB6/PB2`。
  - NuttX 已启用 `CONFIG_STM32H7_QUADSPI` 与 `CONFIG_MTD_W25QXXXJV`。
  - 已修复 STM32H7 QSPI polling read command 和等待状态位卡死问题。
  - 已修复 `w25qxxxjv.c` 写擦 busy wait 饿 USB/MAVLink 的问题，`mtd -i 1 rwtest` 不再导致 QGC 断开。
  - GD25Q128 JEDEC ID 已确认为 `c8 40 18`，对应 128 Mbit / 16 MiB。
  - GD25Q128 当前分区为 `/fs/mtd_params` 128 KiB，`/fs/mtd_waypoints` 128 KiB。
	- PX4 参数默认文件已切到 `/fs/mtd_params`，`param save` 和重启持久化已验证。
	- `dataman` 当前保持官方默认 `/fs/microsd/dataman`，暂不迁移到 `/fs/mtd_waypoints`。
- 已完成第一版飞控传感器通信与引脚规划：
  - `ICM42688P`：主 IMU，走 `SPI2 PC2/PC3/PD3`，规划 `PE4` 为 CS，`PE6` 为 INT/DRDY；其中 `PD3` 在排针上对应 `P2-36`。
  - `GPS + IST8310`：GPS 走 `USART2 PA2/PA3`，IST8310 主磁力计走 `I2C1 PB7/PB8`。
  - `JY901B`：作为副 IMU / 气压计 / 姿态参考，规划走 `UART4 PH13/PH14`，不用 I2C。
- 已完成 4G MAVLink 远程数传第一版引脚规划：
  - `JY901B` 继续独占 `UART4 PH13/PH14`，不改变既有传感器规划。
  - 4G DTU 使用 `UART5_TX=PB13/P1-33`、`UART5_RX=PB12/P1-32`，避开 TF 卡 `PC12/PD2`。
  - `PB12` 当前仅残留 FMUv6C `Brick2 valid` 定义，`PB13` 当前仅残留已关闭的 `CAN2_TX` 定义；后续实现 UART5 时再按真实硬件释放。
- 已在杜邦线临时外接条件下调通 `ICM42688P` 主 IMU：
  - 板级设备表改为 `SPI2 + PE4 CS + PE6 DRDY`。
  - 启动命令为 `icm42688p -R 0 -s -f 2000 start`。
  - SPI2 DMA 已启用，`wq:SPI2` CPU 已降到低占用。
  - 当前稳定参数为 `2 MHz SPI + 4 kHz ODR + IMU_GYRO_RATEMAX=800`。
  - 当前杜邦线连接条件下不再继续追高 SPI 频率；等 PCB 完成后再复测高速 SPI 余量。
- 已完成蜂鸣器 / `tone_alarm` 基础适配与上板有声验证：
  - `CONFIG_DRIVERS_TONE_ALARM=y` 与 `CONFIG_SYSTEMCMDS_TUNE_CONTROL=y` 已启用。
  - `boards/gjl/h743mini/src/board_config.h` 已将蜂鸣器从 V6C 遗留 `PB0 / TIM3_CH3` 改为 H743mini 规划的 `PB14 / TIM12_CH1`。
  - 已修复 STM32H7 `tone_alarm` PWM 接口中 APB1L TIMxEN 位宏兼容问题，使 `TIM12` 能通过 `RCC_APB1LENR_TIM12EN` 兼容旧式 `RCC_APB1ENR_TIM12EN` 命名。
  - `make gjl_h743mini_default` 已验证通过，FLASH 使用约 `88.42%`。
  - 上板后蜂鸣器已有声音，说明 `tone_alarm -> TIM12_CH1 -> PB14 -> 蜂鸣器` 链路已跑通。
- 已屏蔽 V6C 遗留电源 GPIO 与当前 H743mini 引脚冲突：
  - `PB2` 不再初始化/驱动为 `GPIO_VDD_3V3_SENSORS_EN`，保留给 GD25Q128 `QSPI_CLK`。
  - `PC10/PC11` 不再初始化/驱动为 `VDD_5V_HIPOWER` 使能/过流检测，保留给 TF 卡 `SDMMC1 D2/D3`。
  - `PA15` 不再作为 `Brick1 valid` GPIO 读取，后续可释放给 `TIM2_CH1 / M1 PWM`。
  - `make gjl_h743mini_default` 已验证通过，FLASH 使用约 `94.48%`。

当前主要问题：

- 当前仍沿用较多 V6C 默认启动项，日志中会出现未接外设错误。
- TF 卡链路当前只在“完全断电冷启动”场景下可靠；`reboot` 或板载复位键后的 TF 卡状态仍不可靠。
- `/fs/mtd_caldata` 当前没有工厂校准数据；未启用 `SYS_FAC_CAL_MODE` 前，`param dump /fs/mtd_caldata` 显示 BSON no data 属于正常状态。
- GD25Q128 当前采用 QSPI polling 模式，DMA 未启用；当前测试已稳定，DMA 不是下一步阻塞项。
- `ICM42688P` 已在杜邦线临时外接条件下完成 SPI2 DMA 稳定读取，但实际安装方向 `-R 0` 仍需结合载板方向复核；高速 SPI 频率余量需要等 PCB 画完后再测试。
- `GPS/IST8310`、`JY901B` 尚未完成源码适配、接线和上板验证。
- `GPS_1_CONFIG=0` 已用于解除 GPS1 对 `/dev/ttyS0` 的默认占用；后续仍需为真实 GPS 接口重新规划板级串口角色。
- `No autostart ID found` 场景下仍会直接尝试启动 `ekf2`；当前已有 ICM42688P IMU，但 Baro、Compass、GPS 等外设尚未完整接入，EKF2 状态仍需继续验证。
- PX4IO、UAVCAN、部分串口和传感器启动项还需要按当前最小系统板裁剪。
- 4G UART5/TEL2 当前只有引脚与链路规划，尚未修改 `board.h`、NuttX UART5/DMA、串口角色和 MAVLink 参数，也未完成 USB-TTL/DTU/公网链路验证。

### 1.1 当前阶段启动约束

- 当前凡是涉及 TF 卡、`param save`、`/fs/microsd`、`dataman`、`logger`、用户配置脚本加载的验证，都要先完全断电，再重新上电。
- 不要把 NSH `reboot` 或板载复位键后的 TF 卡状态，当作当前阶段判断存储链路是否正常的最终依据。
- 判断冷启动成功的最直接标志：
  - `mount` 中出现 `/fs/microsd type vfat`
  - `dmesg` 中出现 `importing from '/fs/microsd/params'`
  - `dmesg` 中出现 `data manager file '/fs/microsd/dataman'`
  - `dmesg` 中出现 `logger started`
- 当前主线不是继续纠缠 TF 卡软复位问题，而是在“冷启动可用”的前提下，保持 ICM42688P 稳定参数、继续验证 EKF2，并接入 GPS/IST8310/JY901B 等真实外设。

### 1.2 下一阶段任务和目标
下一阶段目标：在 EEPROM 与 GD25Q128 存储链路均已验证、`ICM42688P` 主 IMU 已在杜邦线条件下通过 SPI2 DMA 稳定读取的基础上，先保持当前稳定 IMU 参数并验证 EKF2，再按“主磁力计/GPS -> JY901B 副传感器 -> PWM/RCIN”的顺序逐个适配真实外设，形成一个“存储可靠、启动日志更干净、USB/QGC/NSH 保留、传感器与执行器逐步接入”的基础固件。

优先任务：

1. 完整复测当前存储链路：`mtd status`、`mtd -i 1 readtest`、`mtd -i 1 rwtest`、`param save`、断电重启后 `dmesg`。
2. 临时关闭当前未接入的 V6C 默认传感器启动项，让 `dmesg` 更接近当前真实硬件。
3. 保持 `ICM42688P` 当前稳定工作点：`2 MHz SPI + 4 kHz ODR + 800 Hz FIFO 读取/发布 + SPI2 DMA`，不要在杜邦线接法下继续追高 SPI 频率。
4. 等 PCB 画完、IMU 走短 PCB 线并完成供电去耦后，再从 `2 MHz` 稳定点开始重新测试 `4/8/12/16/24 MHz` SPI 高频余量。
5. 继续验证 `ICM42688P` 进入 EKF2 后的姿态链路，并按实际安装方向复核 `-R 0`。
6. 再适配 `GPS + IST8310`：把 `USART2_TX` 从 `PD5` 调整到 `PA2`，用 `I2C1 PB7/PB8` 启动 `ist8310` 主磁力计。
7. 再接入 `JY901B`：启用 `UART4 PH13/PH14`，先做串口帧读取和日志对比，再考虑发布副 IMU、baro、mag。
8. 视需要处理 `No autostart ID found` 场景下的 `ekf2 start failed (-1)`。
9. 按当前引脚规划推进四路电机 PWM：`TIM2_CH1~CH4 -> PA15/PB3/PB10/PB11`。
10. 按当前引脚规划推进 RCIN / CRSF：`USART6 PC6/PC7`。
11. 临时关闭或延后 PX4IO、UAVCAN、无用 MAVLink 串口等启动项。
12. 后续若需要更高 QSPI 吞吐，再评估 DMA；当前不是优先项。
13. 编译、烧录、完全断电冷启动后重新检查 `dmesg`，目标是只保留当前阶段必须面对的问题。
14. 进入 4G 阶段时，在不改变现有功能引脚的前提下启用 `UART5 PB13/PB12`，同步核对 UART4/UART5/USART6 的最终 `/dev/ttySx` 映射，再逐级验证 USB-TTL、MAVLink、DTU 和公网 relay。

## 2. 项目上下文
这是一个 PX4 二次开发项目，当前聚焦在 STM32H7 最小系统板的板级适配。

已知条件：

- 芯片型号：`STM32H743IIT6`。
- 当前硬件：H7 最小系统板，不是完整 Pixhawk 飞控板。
- 最小系统板资料路径：`/media/gjl/新加卷/BaiduNetdiskDownload/慧勤智远 STM32H743IIT6 CB V1.5小系统板`。
- HSE 外部高速时钟：`25 MHz`，接 `PH0/PH1`。
- LSE 低速时钟：`32.768 kHz`，接 `PC14/PC15`。
- COM 调试串口：CH340，接 `USART1`，`PA9 = USART1_TX`，`PA10 = USART1_RX`。
- USB 口：独立 USB，`PA12 = USB_D+`，`PA11 = USB_D-`，与 CH340 COM 口分开。
- SWD：`PA13 = SWDIO`，`PA14 = SWCLK`，已引出。
- 复位：`NRST/RST` 已引出，当前语境下 RST 与 MCU 的 `NRST` 是同一个复位信号。
- BOOT0：已引出，按键按下时为高电平。
- 板载 RGB LED：R 已接地，上电为红色；G 由 `PB0` 控制；B 由 `PB1` 控制。
- 外部存储：SPI/QSPI Flash `16 MB`，SDRAM `32 MB`，EEPROM `2 KB`。
- PX4 版本：计划使用 `v1.13.3`。
- 初始参考方向：用户原本想参考 Pixhawk V6X。
- 当前参考结论：软件上优先参考 `px4_fmu-v6c`，不要直接套 `px4_fmu-v6x`。

原因：

- V6X 更偏 `STM32H753` 和高冗余飞控设计。
- V6C 更接近 `STM32H743`，更适合作为当前 H743 最小系统板的软件参考。
- 当前板子不是完整飞控板，不能假设 IMU、气压计、GPS、CAN、SD 卡、IO MCU 都已经存在。

当前目标：

- 不是立刻飞行。
- 不是直接复制完整 Pixhawk。
- 先让最小系统板稳定启动。
- 再一个外设一个外设验证。

### 2.1 硬件资料索引

后续做外设引脚分配时，先把“原 FMU-v6C 参考设计”和“当前 STM32H743IIT6 最小系统板原理图”分开看，再逐项对照。

- [DS-018 Pixhawk Autopilot v6C Standard.pdf](<hardware_refs/DS-018 Pixhawk Autopilot v6C Standard.pdf>)：原始 FMU-v6C / Pixhawk Autopilot v6C 标准资料，用于理解 `px4_fmu-v6c` 模板的硬件假设和飞控架构来源。该资料对应原 FMU-v6C 参考方向，不等同于当前 `STM32H743IIT6` 最小系统板引脚。
- [STM32H743IIT6 CB V1.5_SCH.pdf](<hardware_refs/STM32H743IIT6 CB V1.5_SCH.pdf>)：当前最小系统板原理图，是后续分配和校验外设引脚的主要依据。

## 3. 关键结论
- `STM32H743IIT6` 可以作为 PX4 H7 移植对象，但不能简单等同于官方 V6C 或 V6X。
- V6C 可作为软件参考模板，V6X 可作为硬件架构参考。
- V6X 的 bootloader 和 board 配置不要直接烧到当前 H743 板子上。
- PX4 bootloader 不是所有 STM32H7 通用的固定文件，而是按具体 board target 编译出来的。
- 第一次上板应优先保留 ST-Link / SWD 调试通道。
- 在 bootloader 没有稳定前，不应依赖 `make xxx upload`。
- 先不要乱改 option bytes，不要开启读保护、写保护或加密保护。
- 第一个可验收目标是进入 `nsh>`，能执行 `ver` 和 `dmesg`。
- 当前 TF 卡、`params`、`dataman`、`logger` 链路以“完全断电后重新上电”的冷启动结果为准；`reboot` 或板载复位键后的结果不能直接作为当前阶段判断依据。
- BL24C16F EEPROM 已按当前硬件接到 I2C2 `PH4/PH5`，并按 PX4 MTD 标准用途分成 `/fs/mtd_caldata` 与 `/fs/mtd_id`。
- BL24C16F 只有 2 KB，适合存放校准数据和 ID 小分区，不适合承接参数、任务和 dataman 等大容量存储；后续大容量存储应优先接入 GD25Q128。
- GD25Q128 已按 QSPI Flash 接入 PX4 MTD，当前用于 `/fs/mtd_params` 参数存储，并预留 `/fs/mtd_waypoints` 备用分区。
- 当前 `dataman` 保持 PX4 官方默认 `/fs/microsd/dataman`，没有迁移到 `/fs/mtd_waypoints`。
- QSPI `rwtest` 早期导致 QGC 断开，本质是 `w25qxxxjv.c` Flash busy wait 紧循环饿住 USB/MAVLink；加入 `nxsig_usleep(1000)` 后压力测试期间通信保持正常。
- QSPI DMA 当前不是下一阶段优先项；在参数存储和小分区 MTD 场景中，当前 polling 模式已经通过验证。
- 4G MAVLink 数传保留 `UART4 PH13/PH14` 给 JY901B，使用 UART5 的备用复用 `PB13/PB12`；UART5 原 `PC12/PD2` 复用与 SDMMC1 冲突，禁止恢复。
- `CURRENT=PC4/P1-19/ADC1_INP4`，`VOLTAGE=PC5/P1-20/ADC1_INP8`，与 4G UART5 `PB13/PB12` 无冲突。

## 4. AI 续接规则
如果 AI 读到这份文档，需要遵守：

- 先围绕 H743 最小系统板讲，不要默认用户已经有完整 Pixhawk 硬件。
- 每次继续前，先查看本地 Git 当前分支、工作区状态和最近一次提交，例如：`git -C /home/gjl/PX4-Autopilot status --short --branch`、`git -C /home/gjl/PX4-Autopilot log --oneline --decorate -5`。
- 不要直接随意修改用户代码和文件；只有用户明确说明“修改、改代码、提交、推送、写入文件”等操作时，才能执行写入类操作。
- 用户说“我想……”“下一步想……”“看看……”“分析一下……”“修复思路”等表达时，只能先解释现状、列出方案和风险，不得自动修改代码；必须等用户明确说“帮我改代码”“直接修改”“开始实现”“按这个方案改”等确认后，才能写入源码、配置或构建脚本。
- 解释命令时要说清楚：这条命令做什么、为什么做、什么时候不能做。
- 涉及烧录时，优先提醒 ST-Link / SWD、NRST、BOOT0、SWDIO、SWCLK 的保留价值。
- 不要建议用户直接烧 `px4_fmu-v6x_bootloader`。
- 如果要改 PX4 源码，先读当前 checkout 的 `boards/px4/fmu-v6c` 和用户自定义 board 目录，再动手。
- 每次只推进一个阶段：源码准备、board 复制、时钟/console、bootloader、单个外设、整体验证。
- 每次对话结束后，如用户要求更新文档，应新建一份按“编号_任务名_时间”命名的进展子 md，并在本文档最后的“进展索引”中追加链接。
- 每次生成进展子 md 时，子 md 最后必须基于当前进展写出“下一阶段任务和目标”，不能只记录已经做过的事。

## 5. 进度更新规则
本文档作为 Obsidian 主索引和上下文入口使用，详细进展不再直接写入本文档。

更新规则：

- 主 md 路径：`/home/gjl/PX4-Autopilot/board_docs/h743mini-development/px4_flow.md`。
- 进展子 md 统一放在：`/home/gjl/PX4-Autopilot/board_docs/h743mini-development/px4_flow_logs/`。
- 子 md 命名格式：`NNN_任务名_YYYY-MM-DD_HH-MM-SS.md`。
- `NNN` 是三位递增编号，例如 `001`、`002`、`003`，用于确认进展顺序。
- 任务名用中文短语概括本轮做了什么，例如：`004_适配USART1控制台_2026-06-26_10-30-00.md`。
- 主 md 只在“进展索引”里追加子 md 链接，不直接写详细进展。
- 子 md 需要记录：任务目标、当前 Git 状态、最近一次提交、本轮修改内容、修改流程、关键输出、当前结论、下一阶段任务和目标。
- 子 md 的最后一个一级或二级小节必须是“下一阶段任务和目标”。
- “下一阶段任务和目标”必须基于本轮真实结果写，至少说明：下一阶段目标、优先任务、验收标准。
- 更新进展时重点记录关键输出，例如：FLASH 使用率、生成文件、关键报错、关键 warning、烧录结果、串口输出、`dmesg` 片段。
- 用户已经贴出输出时，AI 应优先从输出中提取事实；信息不够时，再向用户追问。

日志示例：

```text
已执行：make px4_fmu-v6c_default
结果：编译成功
关键输出：FLASH 使用约 95.62%，已生成 .elf / .bin / .px4
判断：官方 V6C 固件体积接近 flash 上限，但本次构建成功
```

子 md 末尾示例：

```markdown
## 下一阶段任务和目标

目标：清理 V6C 默认启动项，让最小系统启动日志变干净。

优先任务：
- 处理 USART1 控制台和 GPS1 `/dev/ttyS0` 冲突。
- 关闭当前未接入的 V6C 默认传感器启动项。
- 保留 USB MAVLink 和 NSH 调试链路。

验收标准：
- 编译通过。
- 烧录后 QGC 能连接。
- `dmesg` 中不再重复启动不存在的默认传感器。
```

## 6. PX4 整体流程
PX4 不是单一程序，而是一套飞控软件栈。它通常分成：

- 板级支持
- 底层驱动
- 系统模块
- 估计器
- 控制器
- 任务调度

对当前 H743 最小系统板来说，主线流程是：

```text
源码 -> 板级配置 -> 编译 -> 烧录 -> 启动 -> 控制台 -> 单个外设 -> 逐步扩展
```

## 7. 开发步骤
### 7.1 源码准备
```bash
cd /home/gjl/PX4-Autopilot
git checkout v1.13.3
git submodule update --init --recursive
```

说明：

- `git checkout v1.13.3`：切到计划使用的 PX4 版本。
- `git submodule update --init --recursive`：拉取 PX4 依赖的子仓库。

### 7.2 官方 V6C 编译验证
```bash
make px4_fmu-v6c_default
```

目的：

- 验证源码完整。
- 验证工具链可用。
- 验证官方 V6C 目标能正常出固件。

当前结果：已成功。

### 7.3 自定义 board 复制
```bash
cd /home/gjl/PX4-Autopilot
mkdir -p boards/gjl
cp -a boards/px4/fmu-v6c boards/gjl/h743mini
make list_config_targets | grep h743mini
```

目的：

- 不直接修改官方 `boards/px4/fmu-v6c`。
- 形成自己的 board target。
- 后续所有 H743II 适配都在自定义 board 中进行。

### 7.4 最小启动目标
第一阶段不追求飞行，只追求：

```text
供电正常
SWD 可连接
程序可启动
串口或 USB 控制台可用
能进入 nsh>
能执行 ver
能执行 dmesg
```

## 8. 烧录与启动原则
当前板子有三种主要烧录方式：

### 8.1 ST-Link / SWD 烧录，当前优先使用
这是当前阶段最推荐的日常烧录方式。

原因：

- 已验证可用：bootloader 和 default 主程序都已通过 ST-Link 写入并校验成功。
- 不依赖 BOOT0 进入 STM32 ROM DFU 的时序。
- 不依赖 PX4 bootloader 上传握手。
- 主程序不完整、启动异常、USB 不枚举时，仍可用 connect under reset 救回。
- 可查看 option bytes，适合当前 H743 最小系统板 bring-up 阶段。

连接前提：

```text
ST-Link USB 接电脑
SWDIO 接 PA13
SWCLK 接 PA14
GND 共地
目标板正常供电
NRST/RST 建议接到 ST-Link reset 或保持可手动复位
```

确认设备：

```bash
STM32_Programmer_CLI -l
```

能看到 ST-Link SN 和 FW，即表示 ST-Link 探针被电脑识别。

烧录 bootloader：

```bash
STM32_Programmer_CLI -c port=SWD mode=UR reset=HWrst freq=1000 \
  -w build/gjl_h743mini_bootloader/gjl_h743mini_bootloader.bin 0x08000000 \
  -v
```

烧录主程序：

```bash
STM32_Programmer_CLI -c port=SWD mode=UR reset=HWrst freq=1000 \
  -w build/gjl_h743mini_default/gjl_h743mini_default.bin 0x08020000 \
  -v -rst
```

成功判据：

```text
Download verified successfully
MCU Reset
Software reset is performed
```

注意：

- `.bin` 是裸二进制，给 STM32CubeProgrammer 直接烧。
- bootloader 放在 `0x08000000`。
- 主程序放在 `0x08020000`。
- 不要把主程序烧到 `0x08000000`，否则会覆盖 PX4 bootloader。
- 如果出现 `Unable to get core ID`，优先重试 `mode=UR reset=HWrst freq=1000`，必要时降低频率。
- 如果出现 `DEV_USB_COMM_ERR`，通常是 ST-Link 自身 USB 通讯卡住，优先拔插 ST-Link，或在 Linux 下软复位 ST-Link USB 设备。

### 8.2 STM32 ROM DFU 烧录，备选方式
这是 STM32 芯片内部 ROM bootloader 的烧录方式。

适合：

- 没有 ST-Link 时临时烧录。
- BOOT0 进入 ROM DFU 已确认稳定时烧录。
- 需要验证芯片 ROM bootloader 和原生 USB PA11/PA12 链路时使用。

当前情况：

- 曾通过 STM32 ROM DFU 成功烧录 bootloader 和主程序。
- 但后续 BOOT0 + RST 进入 ROM DFU 不稳定，`STM32_Programmer_CLI -l` 不一定能看到 DFU。
- 因此当前不作为日常优先烧录方式。

进入方式：

```text
BOOT0 拉高
按 RST
电脑识别到 STM32 DFU
```

确认设备：

```bash
STM32_Programmer_CLI -l
```

正常应看到类似：

```text
DFU Interface
STM32 BOOTLOADER
0483:df11
```

烧录 bootloader：

```bash
STM32_Programmer_CLI -c port=USB1 \
  -w build/gjl_h743mini_bootloader/gjl_h743mini_bootloader.bin 0x08000000 \
  -v
```

烧录主程序：

```bash
STM32_Programmer_CLI -c port=USB1 \
  -w build/gjl_h743mini_default/gjl_h743mini_default.bin 0x08020000 \
  -v
```

烧录完成后：

```text
BOOT0 拉低
按 RST
从 PX4 bootloader 启动并跳转主程序
```

### 8.3 PX4 bootloader 上传，已验证可用的主程序更新方式
这种方式使用：

```bash
make gjl_h743mini_default upload
```

或 QGroundControl 固件上传。

这里的上传链路是：

```text
电脑 px_uploader.py / QGC
  -> USB CDC 串口
  -> PX4 bootloader
  -> PX4 bootloader 擦写主程序区域
  -> 主程序位于 0x08020000
```

`px_uploader.py` 握手指的是：电脑端上传工具需要在板子刚复位、PX4 bootloader 短暂停留的时间窗口内，和 PX4 bootloader 交换固定协议字节，确认“对面是 PX4 bootloader、板号匹配、可以开始传固件”。握手成功后，才会把 `.px4` 包里的固件镜像发送给 bootloader。

它和 STM32 ROM DFU 烧录不同：

```text
STM32 ROM DFU：
  使用 ST 芯片内部 ROM bootloader
  需要 BOOT0 + RST
  使用 STM32CubeProgrammer
  直接写 .bin 到指定 Flash 地址

PX4 bootloader upload：
  使用用户 Flash 里的 PX4 bootloader
  正常 BOOT0 = 0
  使用 px_uploader.py / make upload / QGC
  上传 .px4 固件包，由 PX4 bootloader 写入主程序区
```

它依赖：

- 板子已经烧入正确 PX4 bootloader。
- USB CDC 能枚举。
- PX4 uploader 能和 bootloader 正常握手。
- `.px4` 固件包里的 board id、校验、镜像信息正确。

当前情况：

- PX4 bootloader 已能启动。
- 主程序已能启动，QGC 已能连接。
- `.px4` 固件包 `board_id = 56`，bootloader `BOARD_TYPE = 56`，二者匹配。
- 已成功通过 `make gjl_h743mini_default upload` 完成上传。
- 成功识别输出：`Found board id: 56,0 bootloader version: 5`。
- 成功流程包含：`Erase`、`Program`、`Verify`、`Rebooting`。

注意：

- 它适合更新主程序，不适合救 bootloader。
- bootloader 自身损坏、USB 不枚举、主程序严重异常时，仍优先使用 ST-Link / SWD。
- 如果一直停在 `waiting for the bootloader...`，先检查 QGC/串口助手是否占用 `/dev/ttyACM0`。
- 如果串口未被占用但仍然一直等待，检查 Python 是否安装 `pyserial`。本次曾遇到 `Tools/serial/` 目录干扰 `import serial` 的现象，安装 `pyserial` 后解决：

```bash
python3 -m pip install --user pyserial
```

### 8.4 文件选择
常用输出文件含义：

```text
.elf  调试文件，给 GDB / 调试器使用
.bin  裸二进制，给 STM32CubeProgrammer 直接写 Flash
.px4  PX4 固件包，给 PX4 bootloader / QGC / make upload 使用
```

当前日常修改 PX4 主程序时，优先使用 PX4 bootloader upload：

```bash
make gjl_h743mini_default upload
```

主程序更新目标区域：

```text
build/gjl_h743mini_default/gjl_h743mini_default.px4 -> PX4 bootloader -> 0x08020000
```

当前需要直接烧 `.bin` 的主要场景是：

```text
ST-Link / SWD 烧 bootloader：
build/gjl_h743mini_bootloader/gjl_h743mini_bootloader.bin -> 0x08000000

ST-Link / SWD 救援烧主程序：
build/gjl_h743mini_default/gjl_h743mini_default.bin -> 0x08020000
```

### 8.5 什么时候烧 bootloader，什么时候烧 default
简单判断：

```text
bootloader = 启动加载器，负责上电后等待上传、校验并跳转主程序
default    = PX4 主程序，负责真正运行飞控功能、驱动、模块、启动脚本
```

优先原则：

```text
改“怎么进入/加载主程序”的东西：烧 bootloader
改“PX4 启动之后运行的东西”：烧 default
```

需要烧 bootloader 的情况：

| 情况 | 操作 |
|---|---|
| 新 STM32 空片第一次使用 | 编译并烧 `gjl_h743mini_bootloader` |
| bootloader 被覆盖或损坏 | 用 ST-Link 重新烧 bootloader |
| 改了 bootloader 相关 USB / 串口上传逻辑 | 重新烧 bootloader |
| 改了 `BOARD_TYPE` / board id | 重新烧 bootloader，并确认 `.px4` 的 `board_id` 匹配 |
| 改了 `APP_LOAD_ADDRESS` | 重新烧 bootloader |
| 改了 Flash 分区、启动地址、bootloader 链接脚本 | 重新烧 bootloader |
| 改了 bootloader 的 LED 指示逻辑 | 重新烧 bootloader |
| `make upload` 完全无法进入 PX4 bootloader，且确认不是电脑端依赖或串口占用问题 | 可能需要重烧 bootloader |

bootloader 常见相关文件：

```text
boards/gjl/h743mini/src/hw_config.h
boards/gjl/h743mini/src/bootloader_main.c
boards/gjl/h743mini/nuttx-config/bootloader/defconfig
boards/gjl/h743mini/nuttx-config/scripts/bootloader_script.ld
```

bootloader 编译和烧录：

```bash
make gjl_h743mini_bootloader

STM32_Programmer_CLI -c port=SWD mode=UR reset=HWrst freq=1000 \
  -w build/gjl_h743mini_bootloader/gjl_h743mini_bootloader.bin 0x08000000 \
  -v
```

需要烧 default 主程序的情况：

| 情况 | 操作 |
|---|---|
| 改 PX4 驱动 | 烧 default |
| 改板级引脚配置 | 烧 default |
| 改 `board_config.h` | 烧 default |
| 改 `board.h` 时钟配置 | 烧 default |
| 改 `default.px4board` 模块开关 | 烧 default |
| 改启动脚本 `rc.board_*` | 烧 default |
| 改 MAVLink、传感器、LED、串口、I2C、SPI、CAN 配置 | 烧 default |
| 改 PX4 应用模块、参数、uORB、控制逻辑 | 烧 default |
| 只是重新测试同一个固件 | 不一定需要重烧 |

default 常见相关文件：

```text
boards/gjl/h743mini/default.px4board
boards/gjl/h743mini/src/board_config.h
boards/gjl/h743mini/nuttx-config/include/board.h
boards/gjl/h743mini/init/rc.board_defaults
boards/gjl/h743mini/init/rc.board_sensors
src/drivers/*
src/modules/*
```

default 编译和上传：

```bash
make gjl_h743mini_default upload
```

如果 PX4 bootloader upload 临时不可用，也可以用 ST-Link 救援烧主程序：

```bash
STM32_Programmer_CLI -c port=SWD mode=UR reset=HWrst freq=1000 \
  -w build/gjl_h743mini_default/gjl_h743mini_default.bin 0x08020000 \
  -v -rst
```

## 9. 逐步适配外设
建议顺序：

1. 供电和时钟
2. SWD 调试
3. 串口控制台
4. I2C
5. SPI
6. IMU
7. 气压计
8. PWM 输出
9. SD 卡
10. GPS / CAN

每加一个外设，只做三件事：

- 改引脚。
- 开驱动。
- 验证日志。

不要一次打开多个外设，否则错误来源会混在一起。

## 10. 调试方法
- 先看 `dmesg`，再看 `ver`。
- 串口不通时，先查引脚复用、波特率、TX/RX 是否交叉。
- 外设不通时，先做最小闭环：单总线、单设备、单日志。
- I2C/SPI 设备不响应时，先确认供电、片选、上拉、电平、时钟频率。
- 救板时，优先使用 ST-Link 的 connect under reset。
- 无论什么时候，都尽量保留 SWD、NRST、BOOT0。

## 11. 常见坑
- V6X 和 V6C 不是一回事，别混烧 bootloader。
- H743II 和 H753II 很接近，但不是同一个板级配置。
- 晶振频率配错，会导致系统、USB、串口都异常。
- 没有接的传感器如果还在启动脚本里开着，会一直报错。
- 没有 IO MCU 时，不要照搬完整 Pixhawk 方案。
- 直接烧错 bootloader 通常不等于永久变砖，但别乱改 option bytes。
- `.px4` 文件主要面向 PX4 bootloader / QGC 上传，不要把它当普通裸机 bin 随便烧。

## 12. 复盘要点
真正的目标不是“复制官方飞控”，而是：

- 让你的板子有自己的 board target。
- 让最小系统稳定启动。
- 让每个外设都能被独立验证。
- 让固件升级路径清晰可恢复。

## 13. 进展索引
详细进展统一记录在子 md 中。每次继续前先看最新一条，再结合 Git 当前状态判断下一步。

- [[px4_flow_logs/001_历史进展整理_2026-06-26|001 历史进展整理 2026-06-26]]
- [[px4_flow_logs/002_文档管理规则更新_2026-06-26_10-08-27|002 文档管理规则更新 2026-06-26 10:08:27]]
- [[px4_flow_logs/003_迁移到Obsidian并调整命名规则_2026-06-26_10-13-47|003 迁移到 Obsidian 并调整命名规则 2026-06-26 10:13:47]]
- [[px4_flow_logs/004_子md编号规则更新_2026-06-26_10-16-40|004 子 md 编号规则更新 2026-06-26 10:16:40]]
- [[px4_flow_logs/005_安装STM32CubeProgrammer_2026-06-27_12-08-43|005 安装 STM32CubeProgrammer 2026-06-27 12:08:43]]
- [[px4_flow_logs/006_ST-Link探针识别成功_2026-06-27_12-11-00|006 ST-Link 探针识别成功 2026-06-27 12:11:00]]
- [[px4_flow_logs/007_ST-Link连接H743成功_2026-06-27_12-12-17|007 ST-Link 连接 H743 成功 2026-06-27 12:12:17]]
- [[px4_flow_logs/008_bootloader烧录成功但校验参数换行错误_2026-06-27_12-15-34|008 bootloader 烧录成功但校验参数换行错误 2026-06-27 12:15:34]]
- [[px4_flow_logs/009_HOTPLUG连接成功但verify用法错误_2026-06-27_12-17-50|009 HOTPLUG 连接成功但 verify 用法错误 2026-06-27 12:17:50]]
- [[px4_flow_logs/010_bootloader烧录并校验成功_2026-06-27_12-19-12|010 bootloader 烧录并校验成功 2026-06-27 12:19:12]]
- [[px4_flow_logs/011_主程序烧录前SWD重新连接失败_2026-06-27_12-23-00|011 主程序烧录前 SWD 重新连接失败 2026-06-27 12:23:00]]
- [[px4_flow_logs/012_原生USB无枚举并发现VBUS配置嫌疑_2026-06-27_12-36-22|012 原生 USB 无枚举并发现 VBUS 配置嫌疑 2026-06-27 12:36:22]]
- [[px4_flow_logs/013_完成bootloader与主程序DFU烧录_2026-06-27_13-50-08|013 完成 bootloader 与主程序 DFU 烧录 2026-06-27 13:50:08]]
- [[px4_flow_logs/014_QGC连接与NSH日志确认_2026-06-27_14-09-24|014 QGC 连接与 NSH 日志确认 2026-06-27 14:09:24]]
- [[px4_flow_logs/015_优化主md结构与烧录说明_2026-06-27_14-35-59|015 优化主 md 结构与烧录说明 2026-06-27 14:35:59]]
- [[px4_flow_logs/016_修复h743mini头文件索引_2026-06-27_14-41-45|016 修复 h743mini 头文件索引 2026-06-27 14:41:45]]
- [[px4_flow_logs/017_ST-Link烧录bootloader和主程序成功_2026-06-27_18-27-55|017 ST-Link 烧录 bootloader 和主程序成功 2026-06-27 18:27:55]]
- [[px4_flow_logs/018_跑通PX4_bootloader_upload_2026-06-27_18-40-11|018 跑通 PX4 bootloader upload 2026-06-27 18:40:11]]
- [[px4_flow_logs/019_切换SDMMC1并验证TF卡挂载_2026-06-28_14-09-03|019 切换 SDMMC1 并验证 TF 卡挂载 2026-06-28 14:09:03]]
- [[px4_flow_logs/020_TF卡读取问题调试复盘_2026-06-29|020 TF 卡读取问题调试复盘 2026-06-29]]
- [[px4_flow_logs/021_外设引脚规划与电机PWM记录_2026-07-03_01-07-28|021 外设引脚规划与电机 PWM 记录 2026-07-03 01:07:28]]
- [[px4_flow_logs/022_LED重映射与面包板验证_2026-07-03_14-13-47|022 外设引脚规划补充与 LED 重映射验证 2026-07-03 14:13:47]]
- [[px4_flow_logs/023_主md补充TF卡冷启动约束_2026-07-03_17-50-48|023 主 md 补充 TF 卡冷启动约束 2026-07-03 17:50:48]]
- [[px4_flow_logs/024_EEPROM适配与MTD分区验证_2026-07-04_13-07-56|024 EEPROM 适配与 MTD 分区验证 2026-07-04 13:07:56]]
- [[px4_flow_logs/025_GD25Q128_QSPI适配与MTD参数存储验证_2026-07-04_20-40-04|025 GD25Q128 QSPI 适配与 MTD 参数存储验证 2026-07-04 20:40:04]]
- [[px4_flow_logs/026_飞控传感器通信与引脚规划_2026-07-06_18-22-08|026 飞控传感器通信与引脚规划 2026-07-06 18:22:08]]
- [[px4_flow_logs/027_屏蔽V6C遗留电源GPIO冲突_2026-07-06_18-38-02|027 屏蔽 V6C 遗留电源 GPIO 冲突 2026-07-06 18:38:02]]
- [[px4_flow_logs/028_板级遗留外设裁剪与串口收口_2026-07-07_09-25-33|028 板级遗留外设裁剪与串口收口 2026-07-07 09:25:33]]
- [[px4_flow_logs/029_ICM42688P_SPI2_DMA读取调通与速率优化_2026-07-08_13-26-14|029 ICM42688P SPI2 DMA 读取调通与速率优化（杜邦线测试条件）2026-07-08 13:26:14]]
- [[px4_flow_logs/030_H743mini蜂鸣器tone_alarm适配与上板发声验证_2026-07-08_23-16-35|030 H743mini 蜂鸣器 tone_alarm 适配与上板发声验证 2026-07-08 23:16:35]]
- [[px4_flow_logs/031_4G_MAVLink数传与UART5备用引脚规划_2026-07-10_15-26-11|031 4G MAVLink 数传与 UART5 备用引脚规划 2026-07-10 15:26:11]]

## 14. 当前已占用引脚表

本节用于后续外设引脚分配时快速避让。当前阶段先记录已经确认正在使用、已经验证可用、或必须长期保留的引脚。

| 功能 | 引脚 | 当前用途 / 约束 |
|---|---|---|
| HSE 外部高速晶振 | `PH0` / `PH1` | 25 MHz，系统时钟来源，固定保留 |
| LSE 外部低速晶振 | `PC14` / `PC15` | 32.768 kHz，RTC/低速时钟相关，固定保留 |
| SWD 调试 | `PA13` / `PA14` | `SWDIO` / `SWCLK`，救板和调试必须保留 |
| 复位 | `NRST` / `RST` | 复位和 ST-Link connect under reset 使用，必须保留 |
| BOOT0 | `BOOT0` | 进入 STM32 ROM bootloader 使用，必须保留 |
| CH340 调试串口 | `PA9` / `PA10` | `USART1_TX` / `USART1_RX`，当前作为 NSH/调试控制台使用 |
| USB FS | `PA11` / `PA12` | `USB_DM` / `USB_DP`，当前用于 PX4 bootloader upload、QGC、MAVLink shell |
| TF 卡 SDMMC1 | `PC8` | `SDIO_D0`，已验证 TF 卡冷启动可挂载 |
| TF 卡 SDMMC1 | `PC9` | `SDIO_D1`，已验证 TF 卡冷启动可挂载 |
| TF 卡 SDMMC1 | `PC10` | `SDIO_D2`，注意不要再作为 V6C 遗留电源 GPIO 使用 |
| TF 卡 SDMMC1 | `PC11` | `SDIO_D3`，注意不要再作为 V6C 遗留电源 GPIO 使用 |
| TF 卡 SDMMC1 | `PC12` | `SDIO_CLK`，与 `UART5_TX` 冲突，后续不要分给 UART5 |
| TF 卡 SDMMC1 | `PD2` | `SDIO_CMD`，与 `UART5_RX` 冲突，后续不要分给 UART5 |
| 板载 RGB LED | `PB0` | 当前作为板载 RGB LED 的 G 通道控制 |
| 板载 RGB LED | `PB1` | 当前作为板载 RGB LED 的 B 通道控制 |
| 无源蜂鸣器 / `tone_alarm` | `PB14` | 已适配为 `TIM12_CH1` 输出蜂鸣器音调并完成上板有声验证；后续不要再规划给 `SDMMC2` 或 LCD 背光相关功能 |
| 主 IMU `ICM42688P` | `PD3` / `PC2` / `PC3` | 规划为 `SPI2_SCK` / `SPI2_MISO` / `SPI2_MOSI`；`PD3` 对应 `P2-36` |
| 主 IMU `ICM42688P` | `PE4` | 规划为 ICM42688P 片选 CS，替代 V6C 遗留的 `PC13` |
| 主 IMU `ICM42688P` | `PE6` | 规划为 ICM42688P INT/DRDY |
| GPS1 | `PA2` / `PA3` | 规划为 `USART2_TX` / `USART2_RX` |
| GPS1 外置罗盘 `IST8310` | `PB7` / `PB8` | 规划为 `I2C1_SDA` / `I2C1_SCL`，作为主磁力计 |
| 副 IMU / 气压计 / 姿态参考 `JY901B` | `PH13` / `PH14` | 规划为 `UART4_TX` / `UART4_RX` |
| 4G MAVLink 数传 | `PB13` / `PB12` | 规划为 `UART5_TX/P1-33` / `UART5_RX/P1-32`；保留 JY901B 的 UART4，不使用与 TF 卡冲突的 `PC12/PD2` |
| 电池电流检测 `CURRENT` | `PC4` | `P1-19 / ADC1_INP4`，接电源模块缩放后的电流模拟量 |
| 电池电压检测 `VOLTAGE` | `PC5` | `P1-20 / ADC1_INP8`，接电源模块缩放后的电压模拟量，禁止直连电池正极 |
| GD25Q128 QSPI | `PB2` | `QSPI_CLK`，已屏蔽 V6C 遗留 `GPIO_VDD_3V3_SENSORS_EN` |
| 电机 PWM 预留 | `PA15` | 规划为 `TIM2_CH1 / M1 PWM`，已屏蔽 V6C 遗留 `GPIO_nPOWER_IN_A` 读取 |

当前阶段先不规划：

- `安全开关 / Safety LED`：当前 GPS 模块不包含完整 FMU-v6C GPS 口的安全开关和安全灯，第一阶段先不接入。
- `4G 数传 / TELEM2`：已规划 UART5 `PB13/PB12`，当前仍依赖 USB/QGC 和 CH340/NSH 调试；源码适配与上板验证后再作为正式链路。
- `传感器恒温 heater`：当前最小系统板阶段先不做 IMU 恒温加热，后续不优先占用 `PB9` 做 heater。
- `CAN / UAVCAN`：当前先不接 CAN 收发器，也不优先规划 UAVCAN 外设。

### 14.1 飞控传感器总体规划

当前飞控传感器分工：

```text
ICM42688P：主 IMU，提供主 accel / gyro
IST8310：主磁力计，随 GPS 模块外置安装
JY901B：副 IMU / 气压计 / 姿态参考，先走 UART4
```

规划原则：

- `ICM42688P` 是主飞控 IMU，走 `SPI2`，优先保证时间戳、输出率和低延迟；原 `SPI1 PA5/PA6/PA7` 方案因 `PA6` 未从 P1/P2 排针引出，不适合当前 2.54 母座载板。
- `JY901B` 是带 MCU 的十轴姿态模块，串口输出加速度、角速度、磁场、气压/高度和姿态角；它不直接替代 EKF2 的姿态输出，第一阶段先作为副传感器和日志对照。
- `JY901B` 不使用 I2C2，因为 I2C2 已用于 BL24C16F EEPROM，且 JY901B 默认 I2C 地址容易与 `0x50~0x57` 地址段冲突。
- `IST8310` 比 JY901B 内置磁力计更适合作主磁力计，因为 GPS 模块可以安装在机身上方并远离电机、电调、电源线和大电流线。

#### ICM42688P 主 IMU 规划

| ICM42688P 信号 | H743mini 引脚 | STM32 外设 | 连接说明 |
|---|---|---|---|
| `VCC` | `3.3V` | 电源 | 使用 3.3V，不建议 5V |
| `GND` | `GND` | 地 | 必须与飞控共地 |
| `SCK` | `PD3` / `P2-36` | `SPI2_SCK` | 已启用 SPI2，当前杜邦线测试稳定 |
| `MISO` | `PC2` / `P1-6` | `SPI2_MISO` | 已启用 SPI2，当前杜邦线测试稳定 |
| `MOSI` | `PC3` / `P1-7` | `SPI2_MOSI` | 已启用 SPI2，当前杜邦线测试稳定 |
| `CS` | `PE4` | GPIO | 已作为 ICM42688P 片选，替代 V6C 遗留 `PC13` |
| `INT` / `DRDY` | `PE6` | GPIO interrupt | 已作为 ICM42688P 数据就绪中断 |

源码注意：

- 当前 `boards/gjl/h743mini/src/spi.cpp` 已将 `ICM42688P` 迁移到 `SPI2 PC2/PC3/PD3 + PE4 CS + PE6 DRDY`。
- 当前实物测试是杜邦线临时外接，稳定工作点先固定为 `2 MHz SPI + 4 kHz ODR + 800 Hz FIFO 读取/发布 + SPI2 DMA`；PCB 完成前不再追高 SPI 频率。
- 当前 `PC13` 已用于板级 LED，不再适合继续作为 ICM42688P 片选。
- 第一阶段只启用 `ICM42688P`，不要同时恢复 BMI055、MS5611、IST8310 等旧 V6C 启动项。

#### JY901B 副传感器规划

| JY901B 信号 | H743mini 引脚 | STM32 外设 | 连接说明 |
|---|---|---|---|
| `VCC` | `3.3V` | 电源 | 优先 3.3V，减少电平风险 |
| `GND` | `GND` | 地 | 必须与飞控共地 |
| `TX` | `PH14` | `UART4_RX` | JY901B 发，飞控收 |
| `RX` | `PH13` | `UART4_TX` | 飞控发，JY901B 收 |

用途边界：

- `JY901B` 的角度、四元数只作为参考和日志对照，不直接覆盖 PX4 `vehicle_attitude`。
- 若后续接入 PX4 传感器系统，应解析串口帧后发布 `sensor_accel`、`sensor_gyro`、`sensor_mag`、`sensor_baro`。
- `JY901B` 的 accel / gyro 不应一开始与 ICM42688P 平权；建议先低优先级、只记录和对比，再评估是否作为副 IMU 进入传感器选择链路。

源码注意：

- 当前 `board.h` 尚未启用 `UART4 PH13/PH14`。
- 当前 NuttX 配置尚未启用 `CONFIG_STM32H7_UART4`。
- 后续需要先做最小串口读帧验证，再考虑写完整 PX4 driver。

### 14.2 GPS1 规划

当前 GPS1 候选模块先按图片资料记录为：

```text
型号：SR25M10DI
GNSS 芯片/方案：M10050 / M10
罗盘：IST8310
接口座：GH1.25mm 6pin
接口信号：GND / SDA / SCL / RX / TX / VCC
协议：NMEA / UBX
芯片默认波特率：38400 bps
模块出厂波特率：115200 bps
供电：DC 3.3V - 5V，典型 5V
```

GPS1 当前规划优先使用 `USART2`，走第一张 GPIO 排针，方便用杜邦线引出调试：

| 模块信号 | H743mini 引脚 | STM32 外设 | 连接说明 |
|---|---|---|---|
| `VCC` | `5V` 或 `3.3V` | 电源 | 按模块供电要求接入，需确认信号电平为 3.3V |
| `GND` | `GND` | 地 | 必须与飞控共地 |
| `TX` | `PA3` | `USART2_RX` | GPS 发，飞控收 |
| `RX` | `PA2` | `USART2_TX` | 飞控发，GPS 收 |
| `SCL` | `PB8` | `I2C1_SCL` | 给模块上的 `IST8310` 罗盘使用 |
| `SDA` | `PB7` | `I2C1_SDA` | 给模块上的 `IST8310` 罗盘使用 |

规划理由：

- 不继续占用当前 `USART1` / `/dev/ttyS0`，避免和 CH340/NSH 调试控制台冲突。
- `PA2` / `PA3` 在 GPIO 排针上，第一阶段手工接线和排障最方便。
- `PB7` / `PB8` 对应 FMU-v6C 风格的 GPS1 外置罗盘 I2C1，后续适配 PX4 外置罗盘更顺。
- 该模块不包含完整 FMU-v6C GPS1 10pin 口里的安全开关、安全 LED、蜂鸣器；当前阶段先只规划 GPS + 罗盘。

源码注意：

- 当前 `boards/gjl/h743mini/nuttx-config/include/board.h` 里 `USART2_TX` 仍配置为 `PD5`，如果实际按 `PA2` 接 GPS，需要后续把 `GPIO_USART2_TX` 改到 `PA2` 对应复用。

安装约束：

- 模块应安装在机身顶部，陶瓷天线朝上。
- 因为模块带 `IST8310` 罗盘，应尽量远离电机、电调、电源线、电池、大电流线和磁性材料。

### 14.3 POWER 电源与电压/电流检测规划

当前阶段先区分两个概念：

- `供电`：给 H743mini 最小系统板提供稳定 `5V`，让 PX4 能启动、调试、连接 QGC。
- `电压/电流检测`：把电源模块的缩放后模拟量接到 STM32 ADC，让 PX4 知道电池电压和电流。

第一阶段开发建议：

```text
当前先不强制接 POWER 电源模块。
使用稳定 5V 给最小系统板供电即可。
电机、电调、大电流供电链路先和飞控逻辑供电分开验证。
```

后续如果接类似 `PM02 V3` 的电源模块，优先按下面方式预留：

| 电源模块信号 | H743mini 引脚 | STM32 外设 | 连接说明 |
|---|---|---|---|
| `5V` | `5V` | 电源输入 | 给飞控板供电，必须确认模块 5V 输出稳定 |
| `GND` | `GND` | 地 | 必须与飞控、电调、电池负极共地 |
| `CURR` / `CURRENT` | `PC4` | `ADC1_INP4` | 电流检测模拟量，源码里对应 `ADC_BATTERY1_CURRENT_CHANNEL` |
| `VOLT` / `VBAT` | `PC5` | `ADC1_INP8` | 电压检测模拟量，源码里对应 `ADC_BATTERY1_VOLTAGE_CHANNEL` |

规划理由：

- `PC4` / `PC5` 是当前源码里沿用 FMU-v6C 风格预留的电池电流、电压 ADC 引脚。
- `PA2` 已规划给 GPS1 的 `USART2_TX`，不再优先作为第二路电流 ADC 使用。
- 开发早期没有接电源模块时，PX4 可以先不依赖电流计；真正飞行前再补电池电压/电流检测更稳。

接线风险：

- 不能把电池正极直接接到 `PC5`，必须经过电源模块或分压电路，保证 ADC 输入不超过 `3.3V`。
- `CURR` / `VOLT` 接入前要先用万用表量电压范围，确认空载、低油门、高油门都不会超过 STM32 ADC 允许范围。
- 当前 `gjl/h743mini` 配置里电池检测相关模块还没有完整启用，后续真正接入 PM02 时，需要同步检查 `ADC` 驱动、`battery_status` 模块和参数标定。

### 14.4 RCIN 遥控接收机规划

当前遥控链路先按 `Radiomaster Pocket ELRS 2.4G` 遥控器 + `ELRS 2.4G CRSF` 接收机规划。

接收机候选参数按商品图资料记录为：

```text
遥控器：Radiomaster Pocket ELRS 2.4G 版
接收机：ELRS 2.4G NANO
连接协议：CRSF
通道数：16
接收机焊盘：RX / TX / 5V / GND
输入电压：3.6V - 5.5V
典型芯片组合：SX1281 + ESP8285
```

概念区分：

- `ELRS`：遥控器和机载接收机之间的 2.4G 无线链路。
- `CRSF`：机载接收机和飞控之间的 UART 串口协议，不是 `MAVLink`。
- `RCIN`：飞控上的遥控输入功能/接口名，不等于某一种固定协议。

RCIN 当前规划优先使用 `USART6`，复用原 FMU-v6C 用于 `PX4IO` 的串口。当前最小系统板不接 `PX4IO`，所以这组脚可以释放给 ELRS/CRSF 接收机：

| 接收机信号 | H743mini 引脚 | STM32 外设 | 连接说明 |
|---|---|---|---|
| `5V` | `5V` | 电源 | 按接收机要求供电，当前候选接收机支持 `3.6V - 5.5V` |
| `GND` | `GND` | 地 | 必须与飞控共地 |
| `TX` | `PC7` | `USART6_RX` | 接收机发 CRSF 数据，飞控收 |
| `RX` | `PC6` | `USART6_TX` | 飞控向接收机回传 CRSF 遥测 |

规划理由：

- `USART1` 的 `PA9` / `PA10` 已作为 CH340/NSH 调试控制台，不再分给遥控接收机。
- `USART2` 已规划给 GPS1，避免 GPS 和 RC 抢同一组串口。
- `UART5` 的 `PC12` / `PD2` 复用组与 TF 卡 `SDMMC1` 冲突，不再使用；UART5 改用不冲突的 `PB13/PB12` 复用组承接 4G 数传。
- `USART6` 的 `PC6` / `PC7` 在 FMU-v6C 原设计中用于 FMU 与 `PX4IO` 通信；当前 `gjl/h743mini` 已关闭 `PX4IO`，适合复用为 RCIN。

源码注意：

- 当前 `boards/gjl/h743mini/nuttx-config/include/board.h` 中 `USART6_RX` 是 `PC7`，`USART6_TX` 是 `PC6`。
- 当前 `boards/gjl/h743mini/src/board_config.h` 中仍保留 `PX4IO_SERIAL_DEVICE "/dev/ttyS4"` 和 `GPIO_USART6_TX/RX`，后续可把这组 `/dev/ttyS4` 作为 `RC_SERIAL_PORT` 复用。
- 当前 `boards/gjl/h743mini/default.px4board` 中 `PX4IO` 已关闭，但还需要后续启用 `rc_input` 驱动，才能让 PX4 直接解析 CRSF。
- `src/drivers/rc_input/RCInput.cpp` 已支持 `TBS Crossfire (CRSF)`；ELRS 接收机输出的 CRSF 可走同一解析路径。
- `src/lib/rc/crsf.cpp` 中 CRSF 串口波特率为 `420000`，解析的核心帧类型是 `rc_channels_packed`。

接线风险：

- 接收机的 `TX/RX` 命名是站在接收机角度：接收机 `TX` 必须接飞控 `RX`，接收机 `RX` 必须接飞控 `TX`。
- 只接 `TX -> PC7`、`5V`、`GND` 通常可以先看到遥控通道；但建议同时接 `RX <- PC6`，给后续 CRSF 遥测回传留好链路。
- 不建议买 `PWM1~7` 输出型 ELRS 接收机；那类接收机适合直接接舵机/电调，不适合当前 PX4 最小系统板的 UART RCIN 规划。

### 14.5 PWM 电机输出规划

当前四旋翼电机 PWM 先暂定统一使用 `TIM2` 的四个通道，方便四路电机保持同一个定时器基准：

| 电机输出 | H743mini 引脚 | STM32 外设 | 排针位置 | 连接说明 |
|---|---|---|---|---|
| `M1` | `PA15` | `TIM2_CH1` | `P2-41` | 电调信号线，需与电调共地 |
| `M2` | `PB3` | `TIM2_CH2` | `P2-25` | 电调信号线，需与电调共地 |
| `M3` | `PB10` | `TIM2_CH3` | `P1-23` | 电调信号线，需与电调共地 |
| `M4` | `PB11` | `TIM2_CH4` | `P1-24` | 电调信号线，需与电调共地 |

规划理由：

- 四路电机统一在 `TIM2_CH1~CH4`，比跨多个定时器更规整，后续配置 PWM 输出更清晰。
- 这四个引脚都在当前 IIT6 GPIO 排针上引出，第一阶段方便用杜邦线接电调信号。
- 避开 `PA0` / `PA1` 两个按键相关引脚，降低按键电路对 PWM 信号的影响。
- 避开 GPS1 已规划的 `PA2` / `PA3`，不和 `USART2` 冲突。
- 避开 RCIN 已规划的 `PC6` / `PC7`，不和 `USART6` 冲突。

PA15 说明：

- `PA15` 在原 FMU-v6C 源码里被定义为 `GPIO_nPOWER_IN_A`，用于判断 `Brick1` 电源输入是否有效。
- 该信号是数字状态检测，不是电压/电流 ADC；真正的电压/电流检测当前规划仍是 `PC5` / `PC4`。
- `GPIO_nPOWER_IN_A` 是低电平有效，源码中 `BOARD_ADC_BRICK1_VALID` 通过读取该脚判断 `system_power.brick_valid`。
- 当前最小系统板没有照搬 FMU-v6C 的完整电源选择/有效检测电路，所以可先释放 `PA15` 给 `TIM2_CH1` 做电机 PWM。
- 当前 `BOARD_ADC_BRICK1_VALID` 已改为固定有效，`GPIO_nPOWER_IN_A` 也已从 `PX4_GPIO_INIT_LIST` 中屏蔽；后续做 PWM 时仍需在 `timer_config.cpp` 和 NuttX TIM2 配置中真正启用 `PA15 -> TIM2_CH1`。

源码注意：

- 当前 `boards/gjl/h743mini/src/timer_config.cpp` 仍沿用 FMU-v6C 的直接 PWM 映射，前四路是 `PA8` / `PE11` / `PE13` / `PE14`。
- 当前 IIT6 板上 `PE11` / `PE13` / `PE14` 已用于 SDRAM，不能继续作为电机 PWM。
- 后续需要把 `timer_config.cpp` 改成 `TIM2_CH1~CH4 -> PA15/PB3/PB10/PB11`。
- 后续需要在 NuttX 配置中启用 `CONFIG_STM32H7_TIM2=y`。
- `PB10` / `PB11` 早期曾受 V6C 默认 I2C2 假设影响；当前 EEPROM 的 I2C2 已改到 `PH4` / `PH5`，后续推进 PWM 前仍需复核 `timer_config.cpp` 和 NuttX pinmap，避免重新引入冲突。
- `PB3` 可作为 `TRACESWO/JTDO` 调试输出，但当前 4pin SWD 下载只使用 `PA13/SWDIO` 和 `PA14/SWCLK`，不占用 `PB3`。

接线风险：

- 电机 PWM 输出只接电调信号线和地线，不从 STM32 GPIO 给电调供电。
- 电调供电、大电流电池线和飞控逻辑供电要先分开验证，避免把大电流或高电压引入 MCU 引脚。
- 真正上电调试前，先不装桨，先用示波器或逻辑分析仪确认四路 PWM 输出顺序和频率。

### 14.6 LED / 普通 GPIO 重映射候选

当前计划不再继续使用板载 RGB LED 的 `PB0` / `PB1` 作为长期 LED 方案，后续准备重新映射 3 个普通 GPIO。

从当前 P1/P2 排针图、已有外设规划和 NuttX H743 pinmap 复用功能数量综合看，优先候选为：

| 用途           | H743mini 引脚 | 排针位置    | 复用情况 / 说明                                                                 |
| ------------ | ----------- | ------- | ------------------------------------------------------------------------- |
| `LED/GPIO 1` | `PI8`       | `P2-11` | 当前 NuttX H743 pinmap 中基本只有 `EVENTOUT`，复用价值低，适合作普通 GPIO                    |
| `LED/GPIO 2` | `PC13`      | `P2-10` | 复用功能很少，适合低速状态灯或简单使能 GPIO                                                  |
| `LED/GPIO 3` | `PI11`      | `P2-7`  | 主要复用为 `LCD_G6` / `OTG_HS_ULPI_DIR` / `EVENTOUT`，当前规划未使用 LCD 或 USB HS ULPI |

选择理由：

- 避开 `PA13` / `PA14`，保留 SWD 救板通道。
- 避开 `PC8` / `PC9` / `PC10` / `PC11` / `PC12` / `PD2`，保留 TF 卡 `SDMMC1`。
- 避开 `PA2` / `PA3` / `PB7` / `PB8`，保留 GPS1 和外置罗盘规划。
- 避开 `PC6` / `PC7`，保留 RCIN / CRSF 规划。
- 避开 `PA15` / `PB3` / `PB10` / `PB11`，保留四路电机 PWM 规划。
- 避开 `PH10` / `PH11` / `PH12` / `PI0`，避免与 FMC / SDRAM 冲突。
- `EVENTOUT` 只是把 MCU 内部事件信号输出到引脚的特殊复用功能，当前 PX4 板级规划基本不会使用；只要按普通 GPIO 配置，就不会进入 `EVENTOUT` 模式。

接线注意：

- 这 3 个脚当前只作为候选记录，尚未在源码里启用。
- 如果用于直接驱动 LED，必须串联限流电阻，不要让 GPIO 直接承受过大电流。
- `PC13` 更适合低速状态灯或使能类信号，不适合高速翻转或大电流负载。
- 真正改源码前，再统一检查一次是否和 `ICM42688P`、`JY901B`、GPS/IST8310 等已规划外设冲突。

### 14.7 蜂鸣器 / tone_alarm 规划

当前无源蜂鸣器信号脚已适配到 `PB14`：

| 功能 | H743mini 引脚 | STM32 外设 | 连接说明 |
|---|---|---|---|
| 无源蜂鸣器信号 | `PB14` | `TIM12_CH1` | 已通过 PX4 `tone_alarm` 输出可变频率音调并完成上板有声验证，需要与飞控共地 |

规划理由：

- PX4 原 FMU-v6C 的蜂鸣器逻辑是 `tone_alarm` 通过定时器输出不同频率的音调，更适合接无源蜂鸣器或压电片。
- `PB14` 在 NuttX H743 pinmap 中可复用为 `TIM12_CH1OUT_1`，当前已承接 `tone_alarm` 输出。
- 当前 `gjl/h743mini` 已启用 `CONFIG_DRIVERS_TONE_ALARM` 和 `CONFIG_SYSTEMCMDS_TUNE_CONTROL`，并将 `TONE_ALARM_TIMER / CHANNEL` 映射为 `TIM12 / CH1`。
- STM32H7 的 APB1L 命名与 PX4 旧式 `APB1ENR` 命名存在差异，本轮已在 `ToneAlarmInterfacePWM.cpp` 中补齐缺失的 TIMxEN 位宏别名，避免 `RCC_APB1ENR_TIM12EN` 找不到。

长期避让：

- `PB14` 已确定留给无源蜂鸣器，后续不要再规划给 `SDMMC2`。
- `PB14` 后续也不要再规划给 LCD 背光相关功能，避免蜂鸣器音调输出和背光控制互相冲突。
- 如果后续仍需要 `SDMMC2` 或 LCD 背光，应另选引脚，不再挪用 `PB14`。

### 14.8 4G MAVLink 远程数传规划

4G 数传使用 STM32H743 的 `UART5`，不是 `USART5`。为保留 TF 卡和现有全部外设规划，UART5 不使用当前板级遗留的 `PC12/PD2` 复用组，改用 `PB13/PB12`：

| 4G DTU 信号 | H743mini 引脚 | STM32 外设 | 排针位置 | 连接说明 |
|---|---|---|---|---|
| `RXD` | `PB13` | `UART5_TX` | `P1-33` | 飞控发送，接 DTU RXD |
| `TXD` | `PB12` | `UART5_RX` | `P1-32` | DTU 发送，接飞控 RX |
| `GND` | `GND` | 共地 | 就近 GND | DTU 与飞控必须共地 |
| `VIN` | 独立稳压电源 | 电源 | 按模块手册 | 不从弱电流 GPIO/小电流口直接供电 |

保留关系：

```text
UART4  PH13/PH14  -> JY901B，保持不变
UART5  PB13/PB12  -> 4G MAVLink / TEL2，新增规划
USART2 PA2/PA3    -> GPS1，保持不变
USART6 PC6/PC7    -> RCIN/CRSF，保持不变
PC8~PC12、PD2     -> TF 卡 SDMMC1，保持不变
PC4/PC5           -> CURRENT/VOLTAGE，保持不变
```

当前源码状态与后续修改边界：

- `board.h` 当前仍把 UART5 定义为 `GPIO_UART5_RX_3=PD2`、`GPIO_UART5_TX_3=PC12`，后续应切换为 `_1` 对应的 `PB12/PB13`。
- `PB12` 当前残留为 FMUv6C `GPIO_nPOWER_IN_B / Brick2 valid`；当前最小系统板没有规划完整的双路 Brick valid 功能，实现 UART5 时需要释放该遗留定义。
- `PB13` 当前残留为 `CAN2_TX`；当前 UAVCAN 已关闭，CAN2 收发器路径也未确认，实现 UART5 后不再把 PB13 用作 CAN2_TX。
- UART5、UART5 RX/TX DMA 和 `TEL2` 当前尚未启用，4G 数传仍是规划状态。
- 两线 DTU 不接 RTS/CTS，后续设置 `MAV_1_FLOW_CTRL=0`。

最终同时启用 UART4 和 UART5 后，预计串口设备顺序为：

```text
USART1 -> /dev/ttyS0  CH340/NSH
USART2 -> /dev/ttyS1  GPS1
USART3 -> /dev/ttyS2  当前未分配
UART4  -> /dev/ttyS3  JY901B
UART5  -> /dev/ttyS4  4G MAVLink / TEL2
USART6 -> /dev/ttyS5  RCIN/CRSF
UART7  -> /dev/ttyS6
UART8  -> /dev/ttyS7
```

设备号必须以修改后的构建产物 `rc.serial_port` 和板上 `/dev/ttyS*` 实测为准，不能在 UART4 尚未启用时提前把 UART5 固定写成 `/dev/ttyS3`。

第一版参数起点：

```text
MAV_1_CONFIG=102
MAV_1_MODE=0
MAV_1_FLOW_CTRL=0
MAV_1_RADIO_CTL=0
SER_TEL2_BAUD=115200
MAV_1_RATE=5000~8000
```

安全与供电约束：

- DTU UART IO 必须与 STM32H743 的 3.3V TTL 兼容，不能只根据 VIN 支持 5V/12V 就判断串口电平安全。
- 4G 发射有瞬时电流需求，应按模块手册单独设计稳压、去耦和峰值电流余量，并与飞控共地。
- 服务器 UDP relay 需要处理 DTU/QGC 两端 NAT、端点学习、超时与重连。
- 公网裸 UDP MAVLink 存在命令注入风险，正式使用时需要访问控制，并优先采用 VPN、加密隧道或应用层鉴权。
- 当前阶段只完成规划，尚未修改 PX4 源码、编译、烧录或上板验证。
