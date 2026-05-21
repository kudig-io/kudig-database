---
title: kubectl 插件生态知识手册
description: '## 1. 插件概述'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- coredns
- statefulset
- job
- cronjob
- rbac
- agent
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- kubectl 插件生态知识手册 是什么
- 如何 kubectl 插件生态知识手册
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- kubectl
- 插件生态知识手册
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: domain
  path: ../domain-15-specialized-tech/
  label: '相关知识域: domain-15-specialized-tech'
- type: domain
  path: ../domain-10-troubleshooting-diagnostics/
  label: '相关知识域: domain-10-troubleshooting-diagnostics'
---

# kubectl 插件生态知识手册

> **文档类型**: 工具参考手册 | **适用版本**: K8s 1.28-1.33 | **最后更新**: 2026-05
> **使用场景**: Agent 了解常用 kubectl 插件的能力，以便在工单中正确处理"某个 kubectl 插件命令失败"的问题

---

<!-- chunk: 1. 插件概述 -->
## 1. 插件概述

kubectl 从 v1.14 开始支持插件机制，插件本质是名为 `kubectl-<name>` 的可执行文件，放在 `PATH` 中的任意位置（如 `/usr/local/bin/kubectl-foo`），即可通过 `kubectl foo` 调用。

### 1.1 插件管理工具

| 工具 | 说明 | 安装方式 |
|------|------|---------|
| **krew** | kubectl 官方推荐的插件管理器 | `kubectl krew install/upgrade/remove` |
| 手动安装 | 直接下载 binary 并 chmod +x | wget/curl |

### 1.2 krew 基本使用

```bash
# 安装 krew（只执行一次）
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/aarch64/arm64/')" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/krew-${OS}_${ARCH}.tar.gz" &&
  tar zxf krew-${OS}_${ARCH}.tar.gz &&
  ./krew-"${OS}_${ARCH}" install krew
)

# 常用 krew 命令
kubectl krew update                    # 更新插件索引
kubectl krew install <plugin>          # 安装插件
kubectl krew upgrade <plugin>         # 升级插件
kubectl krew uninstall <plugin>       # 卸载插件
kubectl krew list                      # 列出已安装插件
kubectl krew search <keyword>          # 搜索插件
```

---

<!-- chunk: 2. 高频生产插件详解 -->
## 2. 高频生产插件详解

### 2.1 kubectl-whoami（身份验证）

**用途**: 查看当前 kubectl 上下文的认证信息（User/Group/SA）

**安装**: `kubectl krew install whoami`

**使用场景**:
- "kubectl 操作报 Forbidden 时确认身份"
- "RBAC 调试时验证当前用户"
- "确认使用了正确的 ServiceAccount token"

**输出示例**:
```
SYSTEM:masters   (system:master-group)
  └─ discovery: /api/v1  /apis/rbac.authorization.k8s.io/v1
```

**典型故障排查**:
```bash
# 问题：kubectl 执行报 not authorized
kubectl whoami
# 检查当前身份是否正确

# 问题：使用了错误的 kubeconfig context
kubectl whoami --context prod-cluster
```

### 2.2 kubectl-neat（输出清理）

**用途**: 清理 kubectl 输出中的冗余字段（managedFields、creationTimestamp 等）

**安装**: `kubectl krew install neat`

**使用场景**:
- "将 kubectl get 的输出重定向到 YAML 文件时清理噪音"
- "生成干净的 YAML 配置用于 Git"

**输出示例**:
```bash
# 原始输出（kubectl get pod -o yaml）
# 包含大量 metadata.managedFields, metadata.resourceVersion 等

# 使用 neat 后
# 仅保留关键字段：apiVersion, kind, metadata, spec, status
```

**典型用法**:
```bash
kubectl get pod nginx -o yaml | kubectl neat
kubectl get deployment -o yaml | kubectl neat > clean-deployment.yaml
```

### 2.3 kubectl-tree（资源层级视图）

**用途**: 显示资源之间的层级关系（如 ReplicaSet 包含哪些 Pod，Service 引用哪些 Endpoints）

**安装**: `kubectl krew install tree`

**使用场景**:
- "快速了解一个 Deployment 下有多少 Pod（及其状态）"
- "追溯某个 Pod 属于哪个 ReplicaSet/Deployment"
- "排查 ServiceSelector 不匹配问题时查看实际 selector 覆盖了哪些 Pod"

**输出示例**:
```
deployment/nginx
└── replicaset/nginx-7d9f6b8c5
    └── pod/nginx-7d9f6b8c5-xk2p4
    └── pod/nginx-7d9f6b8c5-xk2q1
    └── pod/nginx-7d9f6b8c5-xk2r2
```

**典型故障排查**:
```bash
# 问题：某个 Pod 不属于 Service，找到该 Pod 所属的 ReplicaSet
kubectl tree deployment/my-app
# 在输出中找到异常的 Pod

# 问题：ReplicaSet 数量与预期不符
kubectl tree replicaset -n <namespace>
```

### 2.4 kubectl-debug（安全调试，已 GA in K8s 1.28+）

**用途**: 替代已废弃的 `kubectl exec` / debug 方式，安全地在 Pod/容器中添加 debug 工具或sidecar

**安装**: `kubectl krew install debug`

**使用场景**:
- "在运行中的 Pod 里启动一个 debug 容器"
- "在容器内执行诊断命令不影响主容器"
- "使用 nsenter 进入 Pod 的网络命名空间"
- "复制 Pod 到新容器并加 debug 工具"

**主要命令**:
```bash
# 在 Pod 的主容器旁边启动一个 debug 容器（ephemeral debug container）
kubectl debug <pod> -it --image=busybox --share-processes --copy-to=debug-pod

# 在节点上启动一个 debug 容器（节点级调试）
kubectl debug node/<node-name> -it --image=busybox

# 复制 Pod 并添加 debug 容器
kubectl debug <pod> --image=busybox --copy-to=debug-pod --share-processes

# 注入 sidecar 到现有 Pod
kubectl debug <pod> --inject=debug-tools --image=debug:latest

# 查看已创建的 debug Pod
kubectl get pods | grep debug

# 清理 debug Pod
kubectl debug --clean
```

**K8s 1.28+ 内置（无需 krew）**:
```bash
# K8s 1.28+ 内置 kubectl debug（无需插件）
# 临时容器（Ephemeral Container）
kubectl debug <pod> -it --image=busybox --target=<container-name>
# --target 指定要附加到的目标容器
```

**典型故障排查**:
```bash
# 问题：无法 exec 进入容器（容器运行时问题）
kubectl debug <pod> -it --image=busybox --share-processes
# 启动一个临时 debug 容器

# 问题：Pod 无法被调度（节点问题）
kubectl debug node/<node-name> -it --image=busybox
# 在节点级别启动 debug 容器进行排查
```

### 2.5 kubectl-exec-all（批量 exec）

**用途**: 在同一 Deployment/ReplicaSet/StatefulSet 的所有 Pod 中同时执行命令

**安装**: `kubectl krew install exec-all`

**使用场景**:
- "查看所有 Pod 的日志（tail）"
- "在所有 Pod 中执行相同的诊断命令（如查看进程列表）"
- "批量重启所有 Pod（发送 SIG HUP）"

**输出示例**:
```bash
# 在所有 nginx Pod 中执行 ls /app
kubectl exec-all -l app=nginx -- ls /app

# 输出：
# [nginx-7d9f6b8c5-xk2p4] ls /app
# application
# [nginx-7d9f6b8c5-xk2q1] ls /app
# application
# [nginx-7d9f6b8c5-xk2r2] ls /app
# application
```

**典型故障排查**:
```bash
# 问题：需要同时查看多个 Pod 的日志
kubectl exec-all -l app=nginx -- tail -f /var/log/nginx/access.log

# 问题：所有 Pod 配置不一致，需要逐个检查
kubectl exec-all -l app=nginx -- cat /etc/nginx/nginx.conf
```

### 2.6 kubectl-cost（成本估算）

**用途**: 按命名空间/Deployment/StatefulSet 估算 Kubernetes 资源成本

**安装**: `kubectl krew install cost`

**使用场景**:
- "了解哪些 Deployment 消耗最多资源（最贵）"
- "月度 Kubernetes 成本分析"
- "优化资源分配时确定优先级"

**输出示例**:
```
NAMESPACE    WORKLOAD              CPU REQUESTED   MEM REQUESTED   MONTHLY COST
default      nginx-deployment      500m            128Mi           $12.34
kube-system  coredns               200m            100Mi           $8.90
```

**典型故障排查**:
```bash
# 问题：资源使用率低但成本高
kubectl cost --show-cost-details
# 找到浪费资源的 Deployment

# 问题：优化后想看节省了多少
kubectl cost --historical
```

### 2.7 kubectl-ns（命名空间快速切换）

**用途**: 快速切换/查看当前命名空间，无需每次输入 `-n <namespace>`

**安装**: `kubectl krew install ns`

**使用场景**:
- "频繁操作某命名空间时减少输入"
- "查看当前所在命名空间"

**输出示例**:
```
Current namespace: default
```

**典型用法**:
```bash
kubectl ns my-namespace  # 切换到 my-namespace
kubectl ns               # 查看当前命名空间
kubectl ns -             # 返回上一个命名空间
```

### 2.8 kubectl-ctx（Context 切换）

**用途**: 快速切换 kubectl context（集群）

**安装**: `kubectl krew install ctx`

**使用场景**:
- "在多集群环境中快速切换"
- "确认当前操作的集群（避免误操作生产）"

**输出示例**:
```
CURRENT   NAME             CLUSTER          NAMESPACE
*         dev-cluster      dev.example.com  default
          prod-cluster     prod.example.com  default
          staging-cluster  staging.example.com  default
```

**典型用法**:
```bash
kubectl ctx prod-cluster  # 切换到生产集群
kubectl ctx               # 查看所有 context
```

---

<!-- chunk: 3. 其他常用插件 -->
## 3. 其他常用插件

### 3.1 kubectl-sniff（网络抓包）

**用途**: 在 Pod 内启动 tcpdump 抓包（需要 privileges）

**安装**: `kubectl krew install sniff`

**使用场景**:
- "排查 Service 访问不通时抓包分析"
- "调试微服务间网络问题"

**典型用法**:
```bash
kubectl sniff <pod-name> -n <namespace>
# 生成 wireshark-compatible pcap 文件
```

### 3.2 kubectl-view-secret（查看 Secret 内容）

**用途**: 解码并查看 Secret 内容（替代 base64 -d）

**安装**: `kubectl krew install view-secret`

**使用场景**:
- "快速查看 Secret 的 value"
- "调试 Secret 挂载问题"

**典型用法**:
```bash
kubectl view-secret <secret-name> <key-name> -n <namespace>
kubectl view-secret <secret-name> -n <namespace>  # 列出所有 key
```

### 3.3 kubectl-purge（清理已终止资源）

**用途**: 批量删除已终止的 Pod、Job、CronJob

**安装**: `kubectl krew install purge`

**使用场景**:
- "清理已完成但未删除的 Job"
- "清理 Evicted 的 Pod"

**典型用法**:
```bash
kubectl purge jobs,deployments -n <namespace> --older-than=24h
# 删除 24 小时前已完成的资源
```

### 3.4 kubectl-image-pull-secret（管理镜像拉取凭证）

**用途**: 快速创建 imagePullSecrets 或查看现有的凭证

**安装**: `kubectl krew install image-pull-secret`

**使用场景**:
- "配置私有镜像仓库凭证"
- "查看已配置的 imagePullSecrets"

---

<!-- chunk: 4. 插件安装失败排查 -->
## 4. 插件安装失败排查

### 4.1 常见错误

| 错误 | 原因 | 解决方案 |
|------|------|---------|
| `plugin not recognized` | 文件名不是 `kubectl-<name>` 或不在 PATH | 检查文件路径 |
| `exec format error` | 二进制非 Linux/amd64 架构 | 确认下载了正确架构的版本 |
| `permission denied` | 文件没有执行权限 | `chmod +x kubectl-<name>` |
| `krew install 失败` | 网络问题或下载超时 | 手动下载并直接放到 PATH |

### 4.2 krew 自身故障

```bash
# 重置 krew（如果 krew 命令本身出错）
kubectl krew version
# 如版本显示正常但安装插件失败：

# 清理并重新安装 krew
rm -rf ~/.krew
# 重新执行 krew 安装脚本
```

### 4.3 验证插件可用

```bash
# 列出所有已安装插件
kubectl plugin list

# 或直接运行（会显示错误）
kubectl-whoami
```

---

<!-- chunk: 5. 插件与 RBAC -->
## 5. 插件与 RBAC

### 5.1 kubectl whoami 的权限要求

`kubectl-whoami` 需要访问 `selfsubjectaccessreviews` API，不需要特殊的 RBAC 权限。

### 5.2 其他插件的 RBAC 要求

| 插件 | 所需最小权限 | 说明 |
|------|------------|------|
| whoami | `create` on `selfsubjectaccessreviews` | 通常所有用户都有 |
| neat | read 权限（与 kubectl get 相同） | - |
| tree | read 权限（watch 某些资源） | - |
| debug | `create` on `pods/ephemeral-containers` | K8s 1.28+ 稳定 |
| cost | read 权限（metrics） | 需要 metrics-server |
| ns | 无特殊权限 | - |
| ctx | 无特殊权限 | - |

---

<!-- chunk: 6. 插件速查表 -->
## 6. 插件速查表

| 插件 | 安装命令 | 主要用途 | 故障场景 |
|------|---------|---------|---------|
| `kubectl-whoami` | `kubectl krew install whoami` | 查看当前身份 | RBAC 调试 |
| `kubectl-neat` | `kubectl krew install neat` | 清理 YAML 噪音 | 导出配置 |
| `kubectl-tree` | `kubectl krew install tree` | 资源层级视图 | 排查 Pod/Deployment 关系 |
| `kubectl-debug` | `kubectl krew install debug` | 安全调试 | 无法 exec 时 |
| `kubectl-exec-all` | `kubectl krew install exec-all` | 批量 exec | 批量日志收集 |
| `kubectl-cost` | `kubectl krew install cost` | 成本估算 | 成本分析 |
| `kubectl-ns` | `kubectl krew install ns` | 命名空间切换 | 频繁切换 NS |
| `kubectl-ctx` | `kubectl krew install ctx` | 集群切换 | 多集群切换 |
| `kubectl-sniff` | `kubectl krew install sniff` | 网络抓包 | 网络调试 |
| `kubectl-purge` | `kubectl krew install purge` | 清理已终止资源 | 批量清理 |

---

<!-- chunk: 附录：手动安装（非 krew）示例 -->
## 附录：手动安装（非 krew）示例

```bash
# 下载二进制
wget https://github.com/hjacobs/kubectl-whoami/releases/download/v1.0.0/kubectl-whoami-linux-amd64
mv kubectl-whoami-linux-amd64 /usr/local/bin/kubectl-whoami
chmod +x /usr/local/bin/kubectl-whoami

# 验证
kubectl whoami

# 卸载
rm /usr/local/bin/kubectl-whoami
```

---

```yaml
---
id: KUBECTL-PLUGIN-001
domain: platform-ops
type: tool-reference
tags: [kubectl, plugins, tool-ecosystem, k8s-1.28-1.33, agent-corpus]
intent_queries:
  - "kubectl 插件有哪些"
  - "kubectl whoami 有什么用"
  - "kubectl debug 怎么用"
  - "kubectl tree 是干什么的"
  - "krew 怎么安装插件"
difficulty: intermediate
target_roles: [sre, ops-engineer]
k8s_versions: ["1.28", "1.29", "1.30", "1.31", "1.32", "1.33"]
related:
  - domain-07-platform-engineering/23-cli-enhancement-tools.md
  - domain-01-cluster-fundamentals/31-kubectl-complete-reference.md
---
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- [[domain-07-platform-engineering/MOC.md|domain-07-platform-engineering MOC]]
- [[domain-07-platform-engineering/README.md|Platform Ops Domain (平台运维领域)]]
- [[domain-07-platform-engineering/00-open-source-projects-index.md|Domain-9 平台运维 — 开源项目索引]]
- [[domain-07-platform-engineering/01-platform-ops-overview.md|平台运维概述]]
- [[domain-07-platform-engineering/02-cluster-lifecycle-management.md|集群生命周期管理]]
- [[domain-07-platform-engineering/03-capacity-planning-resource-assessment.md|容量规划与资源评估 (Capacity Planning & Resource Assessment)]]
- [[domain-07-platform-engineering/04-performance-benchmarking-tuning.md|性能基准测试与调优 (Performance Benchmarking & Tuning)]]
- [[domain-07-platform-engineering/05-operations-metrics-system.md|运维指标体系建设 (Operations Metrics System)]]
- [[domain-07-platform-engineering/06-monitoring-alerting-system.md|监控告警体系]]
- [[domain-07-platform-engineering/07-gitops-configuration-management.md|GitOps配置管理 (GitOps Configuration Management)]]
- [[domain-07-platform-engineering/08-automation-toolchain.md|运维自动化工具链 (Operations Automation Toolchain)]]
- [[domain-07-platform-engineering/09-cost-optimization-finops.md|成本优化与FinOps实践 (Cost Optimization & FinOps)]]

## See Also

- [[domain-07-platform-engineering/24-addons-extensions.md|24-addons-extensions]]
- [[domain-07-platform-engineering/25-virtual-clusters.md|25-virtual-clusters]]
- [[domain-07-platform-engineering/99-java-k8s-client-operator-guide.md|99-java-k8s-client-operator-guide]]
- [[domain-07-platform-engineering/99-kubernetes-v1.33-platform-ops-guide.md|99-kubernetes-v1.33-platform-ops-guide]]
