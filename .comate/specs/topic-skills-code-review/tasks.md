# topic-skills 代码健壮性修复任务计划

## 任务分组原则
- 按文件分组，优先修复严重和高优先级问题
- 同类型跨平台兼容性问题集中处理
- 每完成一个文件立即验证

---

- [x] Task 1: 修复 `04-dns-failure.sh` kubectl run 语法错误（严重）
    - 1.1: 将 `--overrides` 从 `--command` 之后移到之前
    - 1.2: 验证 kubectl run 帮助文档确认正确语法

- [x] Task 2: 修复 `diagnose-deep.sh` 跨平台兼容性问题（高优先级）
    - 2.1: 替换 `grep -oP 'https?://[^\s"]+'` 为 `sed` 实现（macOS BSD grep 不支持 `-P`）
    - 2.2: 替换 `grep -oP ':\K[0-9]+'` 为 `sed`/`awk` 实现
    - 2.3: 替换 `nc -zv HOST PORT -w 5` 为 Bash 内置 `/dev/tcp/HOST/PORT` 测试
    - 2.4: 将所有 `grep -i "...\|..."` 替换为 `grep -iE "...|..."`

- [x] Task 3: 修复 `verify-node.sh` 逻辑与死代码问题（高优先级）
    - 3.1: `cut -d= -f2` → `cut -d= -f2-`（处理值包含 `=` 的情况）
    - 3.2: 移除未使用的 `V1_RESULT` 和 `V2_RESULT` 变量，或整合到展示循环

- [ ] Task 4: 修复 `diagnose-deep.sh` 错误码透传问题（中优先级）
    - 4.1: 移除 `run_ssh "..." || true` 中不必要的 `|| true`，让超时错误码 (124) 能被上层感知
    - 4.2: 在需要忽略错误的调用点明确注释原因

- [x] Task 5: 修复 `setup-kind-cluster.sh` 非交互式环境兼容（中优先级）
    - 5.1: `read -rp` 前添加 `[[ -t 0 ]]` 终端检测
    - 5.2: 非终端环境下默认使用已有集群或安全退出

- [x] Task 6: 修复 `diagnose-deep.sh` 证书日期解析增强（中优先级）
    - 6.1: `date -jf` 前增加时区后缀检测和剥离逻辑
    - 6.2: 增加纯数字格式（如 `260115083000Z`）的回退解析

- [x] Task 7: 修复 `02-pod-crashloop.sh` rollout 容错（低优先级）
    - 7.1: `kubectl rollout status` 添加 `|| true` 防止 `set -e` 退出

- [x] Task 8: 修复 `diagnose-quick.sh` kubectl version 解析增强（低优先级）
    - 8.1: `grep -o` 解析改为 `jq`（如果可用）或更健壮的 `sed` 方案
    - 8.2: 解析失败时优雅降级（不显示版本号但不退出）

- [x] Task 9: 最终验证
    - 9.1: 在 macOS 环境下用 `bash -n` 检查所有修改脚本的语法
    - 9.2: 全局搜索确认 `grep -oP` 已清除
    - 9.3: 全局搜索确认 `nc -z` 已清除
    - 9.4: 运行 `git diff --stat` 确认修改范围

