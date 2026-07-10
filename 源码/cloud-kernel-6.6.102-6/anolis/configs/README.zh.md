# ANCK config说明
# 为什么会有 ANCK kconfig 基线
## 管理较乱
原来的 ANCK kconfig 管理较乱，出现了许多问题。比如：

1. 以为打开了，实际没打开的情况
比如 [CONFIG_NUMA_AWARE_SPINLOCKS](https://gitee.com/anolis/cloud-kernel/pulls/535) 和 [CONFIG_CK_KABI_SIZE_ALIGN_CHECKS](https://gitee.com/anolis/cloud-kernel/pulls/1627)，虽然修改了 anolis_defconfig 文件，但是由于依赖不满足，实际上未成功开启。

2. 在 Kconfig 中新增了 kconfig ，但是没有更新 anolis_defconfig 文件
比如 [CONFIG_VTOA](https://gitee.com/anolis/cloud-kernel/pulls/1749) 和 [CONFIG_SCHED_ACPU](https://gitee.com/anolis/cloud-kernel/pulls/2260)

3. kconfig依赖错误
比如 [CONFIG_YITIAN_CPER_RAWDATA](https://gitee.com/anolis/cloud-kernel/pulls/2046)，仅与 arm64 arch 相关，但出现在了 x86 的 anolis_defconfig 中。

4. 重要config被错误修改，导致严重的性能问题
比如 [CONFIG_ARM64_TLB_RANGE 和 CONFIG_ARM64_PTR_AUTH](https://gitee.com/anolis/cloud-kernel/pulls/1960)

## 变更难以追溯
之前许多 kconfig 的变更没有及时记录下来，导致在决定 kconfig 是否可以修改时需要仔细斟酌，没有可以参考的历史信息。
通过 git 回溯信息也有困难，因为不断地刷新 anolis_defconfig 文件，导致很多 kconfig 的位置不断变化，导致需要 git blame 多次才能找到原始的 commit 。

## 兼容性
ANCK 需要将重要 kconfig 高亮出来，作为给 ANCK 下游衍生版本的参考，以保证下游衍生版本与 ANCK 的兼容性。

## 逐渐复杂的 kconfig 文件
随着龙蜥社区的发展，ANCK 的 kconfig 配置文件，在原来仅支持 x86 和 arm64 的 defconfig 和 debug-defconfig 共计 4 个 kconfig 文件的基础上，增加了对龙芯、申威架构的支持，对 核代码覆盖率 gcov 的支持，以及对 arm64 64k 的支持。
当 kconfig 配置文件增多以后，很容易出现调整某个文件的配置项后，忘记调整其他文件的情况。
比如该问题：更新config配置时，未同时更新 anolis_defconfig 和 anolis_debug-defconfig
比如：[CONFIG_KVM_INTEL_TDX](https://gitee.com/anolis/cloud-kernel/pulls/818) 和 [CONFIG_AMD_PTDMA](https://gitee.com/anolis/cloud-kernel/pulls/288)

# kconfig 组织结构说明
## 背景
一个具体的 kconfig 配置项，由以下要素决定：
1. dist
产品。表示该 kconfig 是关于哪个产品的配置。比如说 CONFIG_ABC，可能关于 ANCK 的配置是 y，而关于 ANCK 的下游某个衍生版的配置为 m。
2. level
层级。表示该 kconfig 对当前产品的重要程度，ANCK 划分了 3 个层级(L0/L1/L2)，具体内容见后文。
3. variant
场景。表示该 kconfig 是关于哪个场景的配置，比如是生产环境（generic）、测试环境(debug)、还是覆盖率测试(gcov)等。
4. arch
架构。表示该 kconfig 是关于当前产品某个场景下，某个具体架构的配置。比如 x86、 arm64、loongarch 等。
5. name
名称。该 kconfig 的名字，比如 CONFIG_EXT4_FS。
6. value
值。该 kconfig 的值，比如 `CONFIG_EXT4_FS=m`。

举例：
假设当前有内核版本 ANCK，以及它的下游衍生版 FOO，以及配置项 CONFIG_EXT4_FS。
在不同的产品、场景、架构下对该值的配置可能完全不同，重要程度也不同。
比如 ANCK 需要在 x86 上要求 CONFIG_EXT4_FS 为y，而在 arm64 是需要它 m 即可，且该选项非常重要，不应该被随意变更。
以及在衍生版 FOO 上，该文件系统并不重要，因此应该置为 `not set`。
那么我们可以这么表示：
> Conf[(name="CONFIG_EXT4_FS", dist="ANCK", level="L0", variant="generic", arch="x86")] = "y"
> Conf[(name="CONFIG_EXT4_FS", dist="ANCK", level="L0", variant="generic", arch="arm64")] = "m"
> Conf[(name="CONFIG_EXT4_FS", dist="FOO", level="L2", variant="generic", arch="default")] = "n"

## 产品说明
1. ANCK （Anolis Cloud Kernel）
这是 Anolis 的内核，Anolis7、Anolis8、Anolis23 会搭载不同版本的 ANCK 内核。
2. FOO
您可以在 ANCK 现有的代码和 kconfig 基础上进行修改和构建，从而形成一个 ANCK 的下游衍生版本，比如说新的版本名为 FOO。

## 分层说明
ANCK 按照重要程度，将所有的 kconfig 划分为 3 个层级，以便标记重要的 config，为开发者修改 kconfig 时提供参考。
### L0-MANDATORY
最核心的 kconfig，这类 kconfig 赋予内核最基础的产品化能力，保证内核能作为一个基本的服务器操作系统进行使用。
这类 kconfig 的变更需要十分谨慎，建议 ANCK 下游衍生版不要去 override 此类配置。

入选条件：
1. 有国家标准/行业标准背书。
2. 对兼容性有着重要影响的 kconfig。具体而言，可分为以下几类：
- 具有不证自明的基础能力支持，比如CONFIG_NET、CONFIG_PCI。
- 具有广泛的通用使用场景的 kconfig。如CONFIG_NFS_FS，绝大多数服务器操作系统都支持了 nfs。
- 被主流开源软件所广泛使用/依赖的kconfig。如CONFIG_USERFAULTFD，qemu 热迁移需要该特性。
3. 有现实案例背书，或者有用户反馈错误配置会导致严重的功能/性能问题的 kconfig。

### L1-RECOMMEND
针对特定场景有着重要意义的 kconfig，此类 kconfig 的错误配置将导致该场景出现严重的产品化问题。
Anolis 会站在云场景和服务器场景的角度配置 L1 层的 kconfig，下游衍生版按照可根据实际业务需求对其酌情 override。

入选条件：
1. 有重要的特定业务场景背书。注意是特定场景，如果是通用场景，请放L0。
2. 在特定场景中因为错误配置，引发过一些事故的。

### L2-OPTIONAL
不被关注的 kconfig。

此层级 kconfig 其中分为两类：
1. 可以被手动修改的 kconfig。
此类 kconfig 当前已配置，但是无法确定其对现有场景是否有重要意义，出于兼容性考虑，保持其不变，但将其放置到 L2。
ANCK 认为它们的变更不会对现有使用场景造成严重影响，可以被任意打开或者关闭，比如 CONFIG_CAN、CONFIG_WIRELESS。
下游衍生版如有需要，可随意覆盖。
若后续发现该层级中某些 kconfig 对于某些场景非常重要，可提 PR 申请将其调整至 L1 或 L0。

2. 无法被手动修改的 kconfig。
某些 kconfig 无法被手动调整，只能通过调整其他 kconfig 时通过依赖关系自动 select。关注此类 kconfig 的意义不大，因此将其放置到 L2 中。
典型的 kconfig，如：CONFIG_ARCH_WANT_XXX，CONFIG_HAVE_XXX

入选条件：
1. 当前已经配置了，但是说不清具体使用场景和使用价值的 kconfig
2. 不能被手动配置，只能被自动 select 的 kconfig

### UNKNOWN
尚不明确具体层级的 kconfig。
不建议将 kconfig 长期归类在此层级中。

## 场景说明
ANCK 典型的场景包括：
1. generic。
对应生产环境，属于正式上线使用的场景。
2. debug。
对应测试环境，在版本发布阶段的测试中使用，通常来说会打开 KASAN、KMEMLEAK、LOCKDEP 之类的检测项，以及时发现内核的相关问题。
3. gcov。
对应覆盖率测试的环境，在版本发布阶段的测试中使用。
4. 64k。
使用 arm64 64k 页表的内核。

## 架构说明
ANCK 典型的架构包括：
1. x86
2. arm64
3. loongarch (龙芯)
4. sw_64 (申威)

# kconfig 目录结构说明
kconfig 的目录组织结构，是按照上文提到的几要素来的。具体来说，
kconfig 目录位于 $(srctree)/anolis/configs 中，共分为以下几类：
- scripts/ ，用于存放于 kconfig 有关的脚本文件，开发者通常无需关注
- metadata/ ，用于存放 kconfig 的元数据信息。
    - metadata/description，关于 kconfig 的描述信息
    - metadata/changelog， 关于 kconfig 的变更记录
- L*/ ，以分层方式存放的 kconfig 配置，用于生产环境
    - L*/{x86,arm64,...}， 按架构存放 kconfig 的配置
- custom-overrides/，用于存放除生产环境以外的其他场景的差异化 kconfig
  - custom-overrides/{debug, gcov}，与 debug/gcov 有关的，与生产环境有差异的 kconfig
    - custom-overrides/{debug, gcov}/{default, x86, arm64}.config， 与 debug/gcov 有关的，与生产环境有差异的，通用/x86 特有/arm64 特有的 kconfig
- OVERRIDE/ ，为 ANCK 衍生版提供的，用于存放覆盖 ANCK 的基础配置的目录
  - OVERRIDE/FOO，衍生版 FOO 相对于 ANCK 的差异化配置
    - OVERRIDE/FOO/L*/，衍生版 FOO 以分层方式存放 kconfig 的配置
    ...

## 如何更新 kconfig
请参考 How-To-Modify-Kconfig.zh.md
