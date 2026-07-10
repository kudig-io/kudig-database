本文示例如何修改和新增一个 kconfig 的配置。

# 一、 总体方法
总的来说，您需要以下几步：

1. 进入到 `anolis/` 目录：

`cd anolis/`

2. 修改/新增 kconfig

假设要在所有架构中将 CONFIG_foo 都只为y，使用该命令：

`make dist-configs-modify C=CONFIG_foo all=y L=L1`

如果只在 x86 架构中将其置为 y，而在其他架构中保持关闭，使用该命令：

`make dist-configs-modify C=CONFIG_foo x86=y others=n L=L1`

这个命令执行以下动作：

a. 查找是否已存在现有的 CONFIG_CAN 的配置项，若有，则删除。

b. 根据传入的参数，重新生成 CONFIG_CAN 的配置项。

c. 根据新的配置关系，重新计算和刷新 kconfig 的依赖关系

在使用时，请注意以下几点：

a. 使用 `C=CONFIG_foo` 的方式来传递 kconfig 名称，而非使用这样的方式：

`make dist-configs-modify CONFIG_CAN x86=y others=n L=L1`

这是由 make 命令的语法所限制的。

b. 在传递 kconfig 的配置信息时，请注意必须传递层级信息，即 `L=xx`

3. 确认结果

kconfig 的依赖关系是相当复杂的，因此在对单个 kconfig 调整后，可能会出现依赖条件不满足而导致该 kconfig 实际并未开启的情况。
因此，`make dist-configs-modify`命令会重新计算依赖关系，这可能导致：

a. 生成一系列新的 kconfig。

这些都是由 CONFIG_foo 通过 `select` 或者 `depends on` 关系自动使能的。
这类新生成的 kconfig，需要人工调整它们到对应的层级（使用`make dist-configs-move`命令）。

b. 对特定 kconfig 的修改并未生效。

假设 CONFIG_foo 依赖于 CONFIG_bar，而 CONFIG_bar 之前并未打开，那么在重新计算依赖关系后，CONFIG_foo 依然会处于 `not set` 的状态，甚至是 `invisible` 状态（即在最终结果中看不到关于 CONFIG_foo 的任何配置项）。

这种情况，需要先定位依赖的 CONFIG_bar，并递归使用 `make dist-configs-modify` 修改 CONFIG_bar 及其依赖的 kconfig，最后再对 CONFIG_foo 进行修改。

具体定位的方法，推荐如下：

1. 生成最终的 .config 文件。
`cd /path/to/cloud-kernel/; make anolis_defconfig`

2. 执行 `make menuconfig` 命令

3. 在具体的 tui 界面中，搜索 CONFIG_foo，并通过搜索结果查看对应的依赖关系。

# 二、 示例
我们以使能 `CONFIG_CAN` 为例。
## 1. 修改 kconfig
```
cd anolis/;
make dist-configs-modify C=CONFIG_CAN all=y L=L1
```
这里，我们将 `CONFIG_CAN` 在所有架构中都打开，且将其层级置为 L1。

## 2. 检查结果

在调整后，自动使能了一大堆kconfig，我们需要对这些新的 kconfig 调整层级。
```
$make dist-configs-modify C=CONFIG_CAN all=y L=L1
make -C configs/ dist-configs-modify
make[1]: Entering directory '/cloud-kernel/anolis/configs'
remove old file: /cloud-kernel/anolis/configs/L2-OPTIONAL/default/CONFIG_CAN
created new file: /cloud-kernel/anolis/configs/L1-RECOMMEND/default/CONFIG_CAN
refresh configs
collect all old configs...
* generated file: /cloud-kernel/anolis/output/kernel-ANCK-generic-x86.config
* processed file: /cloud-kernel/anolis/output/kernel-ANCK-generic-x86.config
* generated file: /cloud-kernel/anolis/output/kernel-ANCK-debug-x86.config
* processed file: /cloud-kernel/anolis/output/kernel-ANCK-debug-x86.config
* generated file: /cloud-kernel/anolis/output/kernel-ANCK-generic-arm64.config
* processed file: /cloud-kernel/anolis/output/kernel-ANCK-generic-arm64.config
* generated file: /cloud-kernel/anolis/output/kernel-ANCK-debug-arm64.config
* processed file: /cloud-kernel/anolis/output/kernel-ANCK-debug-arm64.config
split new configs...
replace old configs with new configs....

******************************************************************************
There are some UNKNOWN level's new configs.

CONFIG_CAN_8DEV_USB        CONFIG_CAN_DEBUG_DEVICES  CONFIG_CAN_GRCAN      CONFIG_CAN_ISOTP          CONFIG_CAN_MCBA_USB     CONFIG_CAN_PHYTIUM  CONFIG_CAN_UCAN
CONFIG_CAN_BCM             CONFIG_CAN_DEV            CONFIG_CAN_GS_USB     CONFIG_CAN_J1939          CONFIG_CAN_MCP251X      CONFIG_CAN_RAW      CONFIG_CAN_VCAN
CONFIG_CAN_CALC_BITTIMING  CONFIG_CAN_EMS_USB        CONFIG_CAN_GW         CONFIG_CAN_KVASER_PCIEFD  CONFIG_CAN_MCP251XFD    CONFIG_CAN_SJA1000  CONFIG_CAN_VXCAN
CONFIG_CAN_CC770           CONFIG_CAN_ESD_USB2       CONFIG_CAN_HI311X     CONFIG_CAN_KVASER_USB     CONFIG_CAN_PEAK_PCIEFD  CONFIG_CAN_SLCAN    CONFIG_CAN_XILINXCAN
CONFIG_CAN_C_CAN           CONFIG_CAN_FLEXCAN        CONFIG_CAN_IFI_CANFD  CONFIG_CAN_M_CAN          CONFIG_CAN_PEAK_USB     CONFIG_CAN_SOFTING  CONFIG_NET_EMATCH_CANID

Need to classify above configs manually !!!
See: /cloud-kernel/anolis/configs/UNKNOWN
HINT: `make dist-configs-move` can help you.
eg: make dist-configs-move C=CONFIG_CAN* L=L2

******************************************************************************

The Final Configs After Refresh
default: CONFIG_CAN=y

******************************************************************************
make[1]: Leaving directory '/cloud-kernel/anolis/configs'
```
可以看到，大量与 CONFIG_CAN 有关的 kconfig 被刷新出来了，但是 `CONFIG_CAN` 的结果是符合预期的。

这里，我们将这些自动生效的 kconfig，都放入 L2 层级中。
```
make dist-configs-move C=CONFIG_CAN* L=L2
```

## 3. 结束
到这里为止，所以的步骤已完成，可以使用 `git add` 和 `git commit` 命令记录这些变更，并发起 PR 了。

# 三、注意事项

1. 针对衍生版本 Kconfig 的修改的一些注意事项：

在 OVERRIDE 目录下修改 config 时，如果希望修改的 Kconfig 类型为 choice 时，需要将该 choice 的所有 Kconfig 都放入 OVERRIDE 文件中，并且显示置位。

举例而言：
    在 ANCK 6.6 中，Preemption Model 相关的 Kconfig 为 choice 类型(参考: kernel/Kconfig.preempt 文件)，包含 CONFIG_PREEMPT_NONE、CONFIG_PREEMPT_VOLUNTARY、CONFIG_PREEMPT、CONFIG_PREEMPT_RT 四个配置选项。
    在 configs/L0-MANDATORY/default/CONFIG_PREEMPT_NONE 中 CONFIG_PREEMPT_NONE 被配置成 y。
    此时如果需要在衍生版中将 Preemption Model 配置成 CONFIG_PREEMPT=Y，需要在 OVERRIDE 文件中新增如下配置选项才能生效：
"""
# CONFIG_PREEMPT_NONE is not set
# CONFIG_PREEMPT_VOLUNTARY is not set
CONFIG_PREEMPT=y
# CONFIG_PREEMPT_RT is not set
"""
    如果只配置了一部分 Kconfig，可能会出现非预期的现象，如: Kconfig 修改失败。

# 附：`make dist-configs-move`参数说明
`make dist-configs-move` 用于在不同的层级之间移动 kconfig。
参数如下：
- OLD 可选，表示 kconfig 原来所在的层级。默认为 UNKNOWN
- C 必选，表示需要移动的 kconfig，可使用通配符，如 `C=CONFIG_CAN*`
- L 必选，表示新的层级。
