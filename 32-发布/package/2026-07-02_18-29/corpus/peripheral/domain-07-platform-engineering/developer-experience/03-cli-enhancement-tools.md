---
title: 110 - CLI 增强与效率工具 (CLI Enhancement)
description: '| **Stern** | 多 Pod 日志聚合 | 85% | brew/apt |'
summary: '| **Stern** | 多 Pod 日志聚合 | 85% | brew/apt |'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- mysql
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- CLI 增强与效率工具 (CLI Enhancement) 是什么
- 如何 CLI 增强与效率工具 (CLI Enhancement)
- Kubernetes 9 platform ops 最佳实践
trigger_keywords:
- CLI
- 增强与效率工具
- CLI
- Enhancement
- platform
- ops
prerequisites:
- kubectl-basics
- platform-engineering-basics
- mysql-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 110 - CLI 增强与效率工具 (CLI Enhancement)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

<!-- chunk: CLI 效率提升工具 -->
## CLI 效率提升工具

| 工具 (Tool) | 核心功能 (Function) | 效率提升 | 安装方式 |
|------------|-------------------|---------|---------|
| **kubectx / kubens** | 快速切换上下文/命名空间 | 90% | brew/apt |
| **kube-capacity** | 资源容量查看 | 80% | kubectl krew |
| **Stern** | 多 Pod 日志聚合 | 85% | brew/apt |
| **kubectl-tree** | 资源依赖树 | 70% | kubectl krew |
| **kubectl-neat** | 清理 YAML 输出 | 75% | kubectl krew |

<!-- chunk: kubectx / kubens 快速切换 -->
## kubectx / kubens 快速切换

### 基本用法
```bash
# 列出所有上下文
kubectx

# 切换上下文
kubectx production

# 切换回上一个上下文
kubectx -

# 列出所有命名空间
kubens

# 切换命名空间
kubens kube-system
```

### 别名配置
```bash
# ~/.bashrc 或 ~/.zshrc
alias kx='kubectx'
alias kn='kubens'
```

<!-- chunk: kube-capacity 资源余量 -->
## kube-capacity 资源余量

### 查看集群容量
```bash
# 查看所有节点
kube-capacity

# 按节点分组
kube-capacity --sort cpu.util

# 查看 Pod 级别
kube-capacity --pods

# 输出 JSON
kube-capacity -o json
```

### 输出示例
```
NODE              CPU REQUESTS   CPU LIMITS    MEMORY REQUESTS   MEMORY LIMITS
node-1            1950m (48%)    3900m (97%)   7Gi (43%)         14Gi (87%)
node-2            1200m (30%)    2400m (60%)   5Gi (31%)         10Gi (62%)
```

<!-- chunk: kubectl-tree 资源依赖 -->
## kubectl-tree 资源依赖

### 查看资源树
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 Deployment 依赖
kubectl tree deployment myapp

# 查看 StatefulSet 依赖
kubectl tree statefulset mysql

# 输出示例
NAMESPACE  NAME                           READY  REASON  AGE
default    Deployment/myapp               -              5d
default    ├─ReplicaSet/myapp-7d8f9c      -              5d
default    │ ├─Pod/myapp-7d8f9c-abc       True           5d
default    │ └─Pod/myapp-7d8f9c-def       True           5d
```
<!-- chunk: kubectl-neat 清理输出 -->
## kubectl-neat 清理输出

### 清理 YAML
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 清理 managedFields 等冗余字段
kubectl get pod myapp -o yaml | kubectl neat

# 清理并保存
kubectl get deployment myapp -o yaml | kubectl neat > myapp-clean.yaml
```
<!-- chunk: kubectl 别名与函数 -->
## kubectl 别名与函数

### 常用别名

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# ~/.bashrc 或 ~/.zshrc
alias k='kubectl'
alias kg='kubectl get'
alias kd='kubectl describe'
alias kdel='kubectl delete'
alias kl='kubectl logs'
alias kex='kubectl exec -it'
alias kaf='kubectl apply -f'

# 快速查看 Pod
alias kgp='kubectl get pods'
alias kgpa='kubectl get pods --all-namespaces'

# 快速查看 Service
alias kgs='kubectl get svc'

# 快速查看 Node
alias kgn='kubectl get nodes'
```
### 实用函数

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 快速进入 Pod Shell
ksh() {
  kubectl exec -it $1 -- /bin/bash
}

# 快速查看 Pod 日志
klog() {
  kubectl logs -f $1
}

# 快速删除 Evicted Pod
kdele() {
  kubectl get pods --all-namespaces | grep Evicted | awk '{print $2, "-n", $1}' | xargs kubectl delete pod
}
```
<!-- chunk: kubectl 插件管理 (Krew) -->
## kubectl 插件管理 (Krew)

### 安装 Krew
```bash
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower:]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/\(arm\)\(64\)\?.*/\1\2/' -e 's/aarch64$/arm64/')" &&
  KREW="krew-${OS}_${ARCH}" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz" &&
  tar zxvf "${KREW}.tar.gz" &&
  ./"${KREW}" install krew
)
```

### 推荐插件
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install ctx        # kubectx
kubectl krew install ns         # kubens
kubectl krew install tree       # 资源树
kubectl krew install neat       # YAML 清理
kubectl krew install capacity   # 容量查看
kubectl krew install debug      # 调试工具
kubectl krew install tail       # 日志追踪
```
<!-- chunk: 效率提升技巧 -->
## 效率提升技巧

| 技巧 (Tip) | 说明 (Description) |
|-----------|-------------------|
| **自动补全** | `source <(kubectl completion bash)` |
| **别名缩写** | 减少 80% 输入 |
| **插件生态** | Krew 插件市场 |
| **上下文管理** | kubectx 快速切换 |
| **资源模板** | 保存常用 YAML 模板 |


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-07-platform-engineering KUDIG Database — Global MOC
- [[domain-07-platform-engineering/README.md|[[Platform Ops Domain (平台运维领域)|Platform Ops Domain (平台运维领域)]]]]
- index.md|Domain-9 平台运维 — 开源项目索引]]
- 平台运维概述
- 集群生命周期管理
- [[32-发布/package/2026-07-02_18-29/corpus/core/domain-07-platform-engineering/governance/01-capacity-planning-resource-assessment|03 capacity planning resource assessment]]
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)

## See Also

- 21-api-aggregation
- 22-client-libraries
- 24-addons-extensions
- 25-virtual-clusters


<!-- risk-assessed -->
