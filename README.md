# m2468 UEFI（Mu）设备包

魅族 21 Note 上跑 Linux 用的 UEFI 引导，基于 [Project Mu](https://github.com/microsoft/mu) 的
[Project-Silicium/Mu-Silicium](https://github.com/Project-Silicium/Mu-Silicium) 移植。

这个仓库只放**我们写的那部分**：`m2468Pkg` 设备包和一份配置文件。
Mu-Silicium 本体请去上游拿，没必要 fork 一份重复的。

成果是一个 1 MB 的 `Mu-m2468.img`，刷进 `boot_a` / `boot_b`，它就是 ABL 之后、
Linux 之前那一层，负责初始化硬件、拉起 ESP 里的 `BOOTAA64.EFI`。

## 里面有什么

```
Platforms/Meizu/m2468Pkg/         设备包（edk2 平台描述）
  m2468.dsc / m2468.fdf / m2468.dec
  DeviceBuild.py                  编译入口（edk2toolext）
  Drivers/                        我们加的驱动（SpmiResetDxe、PmicDxe 等）
  Library/                        库
  Include/                        头文件
  FdtBlob/                        设备树 blob（UEFI 要给内核传 DT）
Resources/Configs/m2468.toml      Boot Shim + UEFI FD 配置
Mu-m2468.img                      编译产物，可直接刷
```

## 怎么编译

先按 Mu-Silicium 的 `Building.md` 把环境装好（Python 3.11+、GCC aarch64 交叉工具链）：

```bash
git clone https://github.com/Project-Silicium/Mu-Silicium.git
cd Mu-Silicium
git submodule update --init --recursive
python3 -m pip install -r pip-requirements.txt
```

然后把本仓库的文件按原路径放进去：

```bash
cp -r <本仓库>/Platforms/Meizu/m2468Pkg  Platforms/Meizu/
cp <本仓库>/Resources/Configs/m2468.toml Resources/Configs/
```

编译：

```bash
python3 build_uefi.py -d m2468
```

产物在 `Build/m2468Pkg/DEBUG_GCC5/FV/MU-M2468.fd`，把它改名成 `Mu-m2468.img` 就行。

## 关键配置

`Resources/Configs/m2468.toml` 里这几项不能乱改：

```toml
[uefi_fd]
base = 0xA7100000        # 这块 UEFI 加载到物理地址，写错了直接起不来
size = 0x300000

[boot_shim]
requires_kernel_header  = true
disable_gunyah_watchdog = true    # 不关这个，重载内核会被 watchdog 打断

[boot_image]
header_version = 4       # Android boot image v4，和 ESP 的 BOOTAA64.EFI 对应
```

## 刷

```bash
fastboot -s <序列号> flash boot_a Mu-m2468.img
fastboot -s <序列号> flash boot_b Mu-m2468.img
fastboot -s <序列号> --set-active=a
fastboot -s <序列号> reboot
```

**slot b 不可靠，`--set-active=a` 别漏。**

## 几个坑

`FdtBlob/` 里的 DTB 会被 UEFI 传给内核。如果你在 ESP 上也放了一份 dtb，
**UEFI 会优先读 ESP 上那份**，覆盖掉运行时的 DT，表现是换内核后屏幕完全不亮、串口也没输出。
所以 ESP 里只放 `BOOTAA64.EFI` 和 initramfs，别放 dtb。

`disable_gunyah_watchdog` 关掉之前，我们每次 `kexec` 重载内核都会在几秒后被重置。

UEFI FD 的 `base` 是硬编码的物理地址，换机型必须重新算。

## 许可

设备包沿用的是 Mu-Silicium 的 BSD-2-Clause-Patent，`m2468.dsc` 里的原始版权声明保留着。

## 致谢

整个东西能跑，主要靠
[Project-Silicium/Mu-Silicium](https://github.com/Project-Silicium/Mu-Silicium) 把 Project Mu
搬到了 ARM64 手机上，以及
[RiverKawa271828/uefi-m2381](https://github.com/RiverKawa271828/uefi-m2381)
在同芯片机型（魅族 20，m2381）上的实现 —— 我们的 m2468Pkg 大量参考了它。
