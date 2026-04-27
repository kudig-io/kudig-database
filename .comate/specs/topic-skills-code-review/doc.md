# topic-skills 代码健壮性审查规格

## 审查范围
对 `topic-skills` 目录下所有 Bash 脚本进行全面的逻辑漏洞、异常处理和边界条件审查，重点包括：
- `skill-set/k8s-node-notready/scripts/*.sh`（4 个诊断/修复脚本）
- `skills-run/*.sh`（3 个基础设施脚本）
- `skills-run/scenarios/*.sh`（10 个场景演示脚本）

## 发现的问题清单

### 严重 (Critical)

| # | 文件 | 行号 | 问题描述 | 影响 |
|---|------|------|----------|------|
| 1 | `04-dns-failure.sh` | 71 | `kubectl run` 的 `--overrides` 被放在 `--command` 之后，会被当作容器命令参数传给 `sleep`，导致 Pod 创建时未应用 overrides（terminationGracePeriodSeconds 未生效） | Pod 无法被强制删除，清理阶段可能卡住 |

### 高 (High)

| # | 文件 | 行号 | 问题描述 | 影响 |
|---|------|------|----------|------|
| 2 | `diagnose-deep.sh` | 493, 496 | `grep -oP`（Perl 正则）在 macOS BSD grep 中不支持 `-P` 标志 | 脚本在 macOS 上直接崩溃 |
| 3 | `diagnose-deep.sh` | 501 | `nc -zv` 中 `-z` 选项在 OpenBSD nc（macOS 默认）中不存在 | TCP 连通性测试失败 |
| 4 | `diagnose-deep.sh` | 618 | `grep -i` 使用 `\|` 作为 alternation，在 POSIX BRE 中支持不一致 | 某些系统上关键内核日志可能漏检 |
| 5 | `verify-node.sh` | 316-317, 327 | `cut -d= -f2` 在值包含 `=` 时会截断 | 版本比较逻辑错误 |
| 6 | `verify-node.sh` | 356-357 | `V1_RESULT` 和 `V2_RESULT` 定义后从未使用 | 死代码，增加维护负担 |

### 中 (Medium)

| # | 文件 | 行号 | 问题描述 | 影响 |
|---|------|------|----------|------|
| 7 | `diagnose-deep.sh` | 多处 | `run_ssh "..." || true` 吞掉 SSH 超时返回码 (124) | 调用方无法区分超时和命令成功 |
| 8 | `setup-kind-cluster.sh` | 62 | `read -rp` 在非交互式环境（如 CI）中会阻塞或失败 | 自动化执行失败 |
| 9 | `diagnose-deep.sh` | 550, 589 | 证书日期解析的 BSD `date -jf` 格式 `"%b %d %H:%M:%S %Y %Z"` 假设输出含时区，某些 OpenSSL 版本可能不含 | 日期解析回退到 GNU `date -d`，在 macOS 无 GNU date 时失败 |

### 低 (Low)

| # | 文件 | 行号 | 问题描述 | 影响 |
|---|------|------|----------|------|
| 10 | `02-pod-crashloop.sh` | 209 | `kubectl rollout status` 未包装 `|| true`，若 rollout 失败会触发 `set -e` 退出 | 脚本异常终止（虽然概率低） |
| 11 | `diagnose-quick.sh` | 83 | `kubectl version --client -o json` 的 `grep -o` 解析依赖 JSON 键名格式 | kubectl 输出格式变化时解析失败 |

## 修复方案

### 修复 1: kubectl run --overrides 位置修正
将 `--overrides` 移到 `--command` 之前：
```bash
# 错误
kubectl run dns-test --image=busybox:1.36 --restart=Never -n ${NS} --command -- sleep 300 --overrides='...'

# 正确
kubectl run dns-test --image=busybox:1.36 --restart=Never -n ${NS} --overrides='...' --command -- sleep 300
```

### 修复 2-3: grep -oP 和 nc -z 跨平台替换
- `grep -oP 'https?://[^\s"]+'` → `sed -n 's|.*\(https\?://[^ "]*\).*|\1|p'`
- `grep -oP ':\K[0-9]+'` → `sed 's|.*:\([0-9]*\).*|\1|'` 或 `awk -F: '{print $NF}'`
- `nc -zv HOST PORT -w 5` → `bash -c "</dev/tcp/HOST/PORT"` (Bash 内置 TCP 测试)

### 修复 4: grep alternation 安全化
所有 `grep -i "...\|...\|..."` 替换为 `grep -iE "...|...|..."` 或使用多行 `grep -i` 链式调用。

### 修复 5: cut 边界处理
`cut -d= -f2` → `cut -d= -f2-`（保留 `=` 后的所有字段）。

### 修复 6: 移除未使用变量
删除 `V1_RESULT` 和 `V2_RESULT` 的定义，或将其整合到 V1-V5 展示循环中。

### 修复 7: run_ssh 错误码透传
移除不必要的 `|| true`，让 `run_ssh` 的超时/错误码能被上层逻辑正确处理。

### 修复 8: read 交互式保护
添加 `if [[ -t 0 ]]` 检查 stdin 是否为终端，非终端时使用默认行为（不删除已有集群）。

### 修复 9: 证书日期解析增强
在 `date -jf` 尝试之前先剥离时区后缀，并增加纯数字格式的回退解析。

### 修复 10: rollout status 容错
`kubectl rollout status` 添加 `|| true` 防止 `set -e` 退出。

### 修复 11: kubectl version 解析增强
使用 `jq`（如果可用）或更健壮的 `sed` 解析替代 `grep -o`。

## 预期结果
- 所有脚本在 macOS (BSD) 和 Linux (GNU) 上均可正常运行
- `set -euo pipefail` 模式下无意外退出
- 资源清理在异常中断时可靠执行
- 无死代码和未使用变量
