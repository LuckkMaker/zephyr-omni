# 在 Zephyr 中增加 APM32F407xx 芯片支持 — 除 DTSI 外还需的工作

APM32F4 与 STM32F4 寄存器兼容，可按两种方式集成：**最小复用（仅 DTS + 复用 ST 驱动）** 或 **完整 SoC 移植（树外模块）**。

---

## 一、最小复用方案（推荐先做）

不新增 SoC 系列，只把 APM32 当作「兼容 STM32F407」的芯片，用现有 STM32F4 的 Kconfig/驱动，仅通过 DTS 区分。

### 1. Devicetree (DTS)

- **`omni/dts/arm/geehy/apm32f4xx/apm32f4xx.dtsi`**  
  - 内容：`#include <st/f4/stm32f407Xg.dtsi>`（或 stm32f407.dtsi），必要时加 `compatible = "geehy,apm32f407xx", "st,stm32f407"` 等，让现有 `st,stm32f4-*` 驱动能匹配。
- **板级 DTS**（如 `omni_stm32f407xg.dts`）里用：
  - `#include <arm/geehy/apm32f4xx/apm32f4xx.dtsi>`，并设置好 HSE、PLL、外设等。

### 2. 厂商前缀（必须）

- 在 **DTS 能生效的 dts 根** 下的 `dts/bindings/vendor-prefixes.txt` 中增加一行：
  - `geehy	Geehy (or 极海半导体)`
- 若 DTS 在 **omni 模块** 里，需要在 **omni** 下建：`dts/bindings/vendor-prefixes.txt`（或合并到 Zephyr 的 vendor-prefixes，取决于你是否提交上游）。

### 3. Kconfig / SoC 选择

- **不新增 SoC 选项**：板子 defconfig 或 `board.cmake` 里继续选 `CONFIG_SOC_STM32F407XG=y`（或对应 ST 型号），这样现有 STM32F4 的 soc 层、时钟、pinctrl、外设驱动都会用上。
- 若希望菜单里显示「APM32」：可只在 **board** 的 Kconfig 里加一个「APM32 板型」选项，仍 `select SOC_STM32F407XG`。

### 4. 驱动与 HAL

- 现有 Zephyr 的 **st,stm32f4-*** 驱动（clock、pinctrl、uart、gpio 等）都通过 compatible 匹配；只要 DTS 里 compatible 包含 `st,stm32f407` 或具体外设的 compatible，就会用同一套驱动。
- **HAL**：若 APM32 提供与 STM32Cube 兼容的 HAL 头/库，可在模块里用 `HAS_APM32_HAL` 或继续用 `HAS_STM32CUBE`，并在 CMake 里把 include/lib 指到 Geehy SDK；若寄存器一致，甚至可继续用 ST 的 `stm32f4xx.h` 等头文件。

**小结（最小方案）**：DTSI + vendor-prefixes + 板子 DTS 选 STM32F407 SoC，无需新 SoC 目录、soc.yml 或新 Kconfig.soc。

---

## 二、完整 SoC 移植（树外模块，omni 里做）

若希望有独立的「Geehy APM32F4」SoC 选项、或要挂 Geehy 专用 HAL/启动代码，需要在模块里提供完整 soc 层，并让 Zephyr 通过 **soc_root** 发现。

### 1. 目录结构（在 omni 内）

建议布局（与 Zephyr 的 `soc/<vendor>/<series>/` 一致）：

```text
omni/
├── dts/
│   ├── arm/geehy/apm32f4xx/apm32f4xx.dtsi
│   └── bindings/vendor-prefixes.txt   # 添加 geehy
├── soc/
│   └── geehy/
│       ├── soc.yml                    # 声明 family/series/socs
│       ├── Kconfig.soc
│       ├── Kconfig.defconfig
│       ├── Kconfig
│       └── apm32f4x/
│           ├── Kconfig.soc
│           ├── Kconfig.defconfig
│           ├── Kconfig.defconfig.apm32f407xx
│           ├── Kconfig
│           ├── CMakeLists.txt
│           ├── soc.c
│           ├── soc.h
│           └── (可选) power.c, linker.ld 等
└── zephyr/
    └── module.yml                     # 增加 soc_root: soc
```

### 2. 各环节必做项

| 项目 | 说明 |
|------|------|
| **module.yml** | 在 `build.settings` 下增加 `soc_root: soc`（以及已有的 `board_root`/`dts_root`），这样 west 会把 `omni/soc` 加入 SOC_ROOT。 |
| **soc.yml** | 在 `omni/soc/geehy/soc.yml` 声明 family（如 `geehy`）、series（如 `apm32f4x`）、socs（如 `apm32f407xx`）。格式参考 `zephyr/soc/st/stm32/soc.yml`。list_hardware 靠 `soc/**/soc.yml` 发现 SoC。 |
| **Kconfig** | 在 `soc/geehy/` 和 `soc/geehy/apm32f4x/` 提供 Kconfig.soc / Kconfig.defconfig / Kconfig，定义 `SOC_FAMILY_GEEHY`、`SOC_SERIES_APM32F4X`、`SOC_APM32F407XX` 等，并 `select ARM`、`CPU_CORTEX_M4`、`HAS_*` 等（可参考 `zephyr/soc/st/stm32/stm32f4x/Kconfig.soc`）。 |
| **soc.c / soc.h** | 上电初始化（如 Flash 预取、缓存、SystemCoreClock），可参考 `zephyr/soc/st/stm32/stm32f4x/soc.c`；soc.h 包含 Geehy 或 ST 兼容的 HAL 头。 |
| **CMakeLists.txt** | 编译 soc.c（及可选 power.c），并 `zephyr_include_directories`；若用树外 Cortex-M 公共代码，需按 Zephyr 文档 `add_subdirectory(${ZEPHYR_BASE}/soc/arm/common/cortex_m ...)`。 |
| **DTS** | 保持 `apm32f4xx.dtsi`，可 include `st/f4/stm32f407Xg.dtsi` 再覆盖/补充；或自写节点，compatible 用 `geehy,apm32f407xx` 并在驱动里做兼容。 |
| **DTS 绑定** | 若外设与 ST 完全一致，可继续用 `st,stm32f4-*`；若有差异，在 `dts/bindings/` 下为 APM32 增加或改写 yaml。 |
| **链接脚本** | 一般用 Zephyr 提供的 `arch/arm/cortex_m/scripts/linker.ld`；若有 CCM/特殊内存，在 DTS 里描述并在链接脚本里用 `zephyr,ccm` 等。 |
| **启动/复位** | Cortex-M 通常用 Zephyr 公共 startup；若 Geehy 有专用复位/启动流程，再在 soc 或 arch 里接进去。 |

### 3. 驱动与时钟

- **时钟**：若与 STM32F4 RCC 一致，可在 DTS 里用 `st,stm32f4-rcc` 等，继续用 Zephyr 的 clock_control；否则需在模块里写 clock 驱动并绑定到 `geehy,apm32f4-rcc` 之类。
- **Pinctrl**：同法，兼容则用现有 `st,stm32f4-pinctrl`；否则写 `geehy,apm32f4-pinctrl` 和对应 binding。
- **GPIO / UART / SPI / I2C 等**：优先用现有 ST 驱动 + DTS compatible；只有寄存器不同时才为新 compatible 写驱动。

### 4. vendor-prefixes

- 在 **omni** 的 `dts/bindings/vendor-prefixes.txt`（或你使用的 dts_root 下）添加：`geehy	Geehy`，否则 DTS 里 `geehy,apm32f407xx` 会报未注册前缀。

---

## 三、推荐顺序

1. **先做最小复用**：补全 `apm32f4xx.dtsi`、vendor-prefixes、板子 DTS 选 `SOC_STM32F407XG`，确认能建出镜像并跑起来。
2. **再视需要做完整 SoC**：在 omni 里加 `soc/geehy/`、soc.yml、Kconfig、soc.c/soc.h、module.yml 的 `soc_root`，并让板子选 `SOC_APM32F407XX` 而不是 ST 的 SoC。

这样除了 DTSI 之外，你需要动到的就是：**vendor-prefixes**、**（可选）soc 目录与 Kconfig/CMake/soc.c**、**module.yml 的 soc_root**，以及按需的**驱动/绑定**。
