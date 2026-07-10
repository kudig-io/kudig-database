# 背景
本文档用于存放 kconfig 的检查规则，以便检查 kconfig 的是否有违背规则。

# 目录组织
- L0-MANDATORY/，用于存放**必须**遵守的 kconfig 规则，如果违反则视为失败
- L1-RECOMMEND/，用于存放**推荐**遵守的 kconfig 规则，如果违反则会告警
- {L0-MANDATORY, L1-RECOMMEND}/{x86/arm64/loongarch/sw_64/riscv}.config，对应 x86、arm64、龙芯、申威、riscv平台的 kconfig 规则

# 规则文件说明
文件的每一行存放一个规则，具体如下：
1. `CONFIG_FOO=value`
CONFIG_FOO 必须出现在 config 文件中，且值必须为 value

2. `# CONFIG_FOO is not set`
CONFIG_FOO 必须出现在 config 文件中，其值必须为 not set

3. `# UNLIMITED CONFIG_FOO`
对 CONFIG_FOO 不做要求

4. `# CHOICE CONFIG_FOO a/b/c`
CONFIG_FOO 必须出现在 config 文件中，值必须在 a/b/c 中选择一个

5. `# RANGE CONFIG_FOO a,b`
CONFIG_FOO 必须出现在 config 文件中，值为整型，且必须在 [a, b] 这个范围内

6. `# EXCLUSIVE value CONFIG_FOO1 [CONFIG_FOO2 ...]`
CONFIG_FOO1, CONFIG_FOO2 等列表中，有且只有一个能出现在 config 文件中，且值必须为 value

7. `## xxxx`
此行为注释

# 使用方式
在 clone 该仓库后，执行 `cd anolis; make dist-configs-check` 命令即可。
