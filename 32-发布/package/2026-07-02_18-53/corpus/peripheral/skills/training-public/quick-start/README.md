---
title: 新人上手快速路径（Quick Start） [quick-start]
description: '## 概述'
summary: '本目录提供新人入职第一周的系统性上手路径，旨在帮助刚加入团队的 SRE 和运维工程师在最短时间内建立 [[Kubernetes|Kubernetes]] 生产运维的基本能力。新人培训是团队战斗力建设的关键环节，一个结构化的上手路径能够显著缩短新人从"第一天入职"到"能独立处理工单"的时间周期。'
category: learning
tags:
- k8s
- training
- hands-on
- kubelet
- prometheus
- grafana
- coredns
- docker
- rbac
- daemonset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 新人上手快速路径（Quick Start） 是什么
- 如何 新人上手快速路径（Quick Start）
trigger_keywords:
- 新人上手快速路径
- Quick
- Start
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- monitoring-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 新人上手快速路径（Quick Start）

> **适用对象**: 入职第一周的 SRE/Ops 工程师 | **版本**: K8s 1.28-1.33 | **最后更新**: 2026-05

---

## 概述

本目录提供新人入职第一周的系统性上手路径，旨在帮助刚加入团队的 SRE 和运维工程师在最短时间内建立 [[Kubernetes|Kubernetes]] 生产运维的基本能力。新人培训是团队战斗力建设的关键环节，一个结构化的上手路径能够显著缩短新人从"第一天入职"到"能独立处理工单"的时间周期。本指南基于阿里云 ACK（Container [[Service|Service]] for Kubernetes）生产环境实际运维经验编写，涵盖了从环境搭建到独立值班的完整流程。

四天的内容按照由浅入深的顺序排列：第一天完成所有工具安装和集群访问验证，确保基本工作环境就绪；第二天学习工单处理流程，掌握最常见的问题类型和处理方法；第三天理解值班交接的标准化流程，确保业务连续性；第四天安装和配置效率工具，提升日常运维效率。每天的学习目标清晰明确，产出可量化验证。

建议新人严格按照 Day 1 到 Day 4 的顺序完成，每天预留 4-5 小时的学习和实践时间。如果在某个环节遇到困难，不要跳过，应该及时向导师或团队成员求助。完成本快速路径后，新人应能独立完成日常的 oncall 值班工作，并进入为期四周的深度培训阶段。

**学习目标**：
- 在四个工作日内建立 K8s 生产运维基本能力
- 能够独立连接集群、处理简单工单、完成值班交接
- 安装并熟练使用核心运维工具链

**前置条件**：
- 拥有公司内网访问权限和阿里云 RAM 账号
- 基本了解 Linux 命令行操作
- 对容器和 Kubernetes 有初步概念

---

## 核心概念

### Day 1: 环境准备

第一天是所有后续工作的基础。新人需要完成开发/运维工具的安装配置，验证对 Kubernetes 集群的访问权限，并确认监控告警系统的可达性。这一天的核心产出是能够成功执行 `kubectl get [[Pods|pods]] -A` 并看到集群中所有命名空间的 Pod 列表。

关键活动包括：
- 安装 kubectl 命令行工具并配置 kubeconfig 文件
- 验证对 ACK 集群的访问权限
- 登录 Prometheus/Grafana 监控系统
- 确认告警通知渠道（钉钉/企微/Slack）可正常接收

### Day 2: 工单处理

第二天聚焦于实际的运维工作——工单处理。新人将学习公司内部的工单分类体系、SLA 标准，以及最常见的 Pod 故障处理方法。这是日常 oncall 工作的核心内容，掌握这些技能意味着你已经能够为团队分担基础的运维工作。

常见工单类型及处理方法：

| 工单类型 | 典型现象 | 处理方法 | SLA |
|----------|---------|---------|-----|
| CrashLoopBackOff | Pod 反复重启 | 查看日志定位启动失败原因 | P2: 15min |
| ImagePullBackOff | 镜像拉取失败 | 检查镜像地址和拉取凭证 | P2: 15min |
| Pod Pending | Pod 无法调度 | 检查资源余量和调度约束 | P2: 15min |
| Service 无 Endpoints | 服务不可达 | 检查 selector 匹配和 Pod 就绪 | P2: 15min |
| 节点 NotReady | 节点离线 | 检查 kubelet 和运行时状态 | P1: 5min |

### Day 3: 值班交接

第三天学习值班交接的标准操作流程（SOP）。良好的交接是保障 7x24 小时业务连续性的关键。新人需要掌握交班前的检查清单、接班后的验证步骤，以及紧急情况下的交接流程。

### Day 4: 工具安装

第四天专注于安装和配置提升日常运维效率的工具集，包括 k9s（终端 UI）、stern（多 Pod 日志聚合）、kubectx/kubens（上下文切换）等。这些工具虽然不是必须的，但能大幅提升日常操作效率。

---

## 目录结构

```
quick-start/
├── README.md                    # 本文件: 新人上手路径总览
├── 01-day-one-checklist.md      # Day 1: 新人首日检查清单
├── 02-first-ticket-guide.md     # Day 2: 第一个工单处理指南
├── 03-oncall-handoff.md         # Day 3: 值班交接 SOP
└── 04-debug-tools-setup.md      # Day 4: 调试工具全家桶安装
```

---

## 学习路径

### Day 1: 环境准备（第一天）

**目标**: 完成所有开发/运维工具安装和集群访问验证

**内容**:
- kubectl 安装和配置
- kubeconfig 配置
- oncall 工具访问验证（监控/告警/工单系统）
- 权限验证
- 知识储备自测

**产出**: 能够连接集群，查看所有命名空间的 Pod

**参考文件**: [01-day-one-checklist.md](./01-day-one-checklist.md)

**详细步骤**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Step 1: 安装 kubectl (macOS)
brew install kubectl
kubectl version --client

# 预期输出:
# Client Version: v1.33.0
# Kustomize Version: v5.6.0

# Step 2: 配置 kubeconfig
# 从 ACK 控制台下载 kubeconfig 文件
# 将下载的 config 文件保存到 ~/.kube/config
mkdir -p ~/.kube
cp ~/Downloads/kubeconfig ~/.kube/config
chmod 600 ~/.kube/config

# Step 3: 验证集群连接
kubectl cluster-info

# 预期输出:
# Kubernetes control plane is running at https://xxx.cn-hangzhou.aliyuncs.com:6443
# CoreDNS is running at https://xxx.cn-hangzhou.aliyuncs.com:6443/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy

# Step 4: 验证权限
kubectl auth whoami

# 预期输出:
# ATTRIBUTE   VALUE
# Username    sso-user@company.com
# Groups      [system:authenticated ...]

# Step 5: 查看所有命名空间的 Pod
kubectl get pods -A

# 预期输出:
# NAMESPACE     NAME                                      READY   STATUS    RESTARTS   AGE
# kube-system   coredns-66f5b8f7f5-abcde                  1/1     Running   0          30d
# kube-system   csi-plugin-xxxxx                          1/1     Running   0          30d
# kube-system   kube-proxy-worker-xxxxx                   1/1     Running   0          30d
# monitoring    prometheus-k8s-0                          2/2     Running   0          15d
```
---

### Day 2: 工单处理（第二天）

**目标**: 理解工单处理流程，能处理简单问题

**内容**:
- 工单分类和 SLA
- Pod 故障处理（CrashLoopBackOff/Pending）
- Service 无 Endpoints 处理
- 节点 NotReady 处理
- 工单记录模板

**产出**: 能处理常见的 oncall 工单

**参考文件**: [02-first-ticket-guide.md](./02-first-ticket-guide.md)

**工单处理标准流程**:

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# === 工单类型: Pod CrashLoopBackOff ===

# Step 1: 查看Pod状态
kubectl get pods -n <namespace>
# 预期输出:
# NAME                     READY   STATUS             RESTARTS   AGE
# my-app-7d9f8b6c4-xyz12   0/1     CrashLoopBackOff   5          10m

# Step 2: 查看Pod详情和Events
kubectl describe pod <pod-name> -n <namespace>
# 关注Events部分:
# Events:
#   Type     Reason     Age                Message
#   Warning  BackOff    60s (x5 over 3m)   Back-off restarting failed container

# Step 3: 查看上一次崩溃的日志
kubectl logs <pod-name> -n <namespace> --previous
# 常见原因和日志特征:
# - 应用启动失败: "Failed to start server"
# - 配置缺失: "Error: environment variable DB_HOST is required"
# - OOMKilled: 查看Last State中的Reason

# Step 4: 根据日志定位问题并修复
# 示例: 配置缺失 -> 补充ConfigMap/Secret
# 示例: 镜像错误 -> 修正镜像版本

# === 工单类型: Service 无 Endpoints ===

# Step 1: 查看Service
kubectl get svc -n <namespace>
# 预期输出:
# NAME         TYPE        CLUSTER-IP      EXTERNAL-IP   PORT(S)   AGE
# my-service   ClusterIP   10.96.123.45    <none>        80/TCP    5d

# Step 2: 查看Endpoints
kubectl get endpoints <service-name> -n <namespace>
# 预期输出 (问题状态):
# NAME         ENDPOINTS   AGE
# my-service   <none>      5d

# Step 3: 检查selector匹配
kubectl describe svc <service-name> -n <namespace> | grep Selector
# 对比Pod标签:
kubectl get pods -n <namespace> --show-labels

# Step 4: 修正selector或Pod标签使其匹配
```
---

### Day 3: 值班交接（第三天）

**目标**: 掌握值班交接流程，能独立进行 oncall 交接

**内容**:
- 交班前准备工作
- 接班后检查流程
- 交接文档模板
- 紧急交接场景处理
- 值班纪律

**产出**: 能独立完成 oncall 值班交接

**参考文件**: [03-oncall-handoff.md](./03-oncall-handoff.md)

**值班交接检查清单**:

```markdown
## 交班前检查清单
- [ ] 当前所有告警状态: □ 无告警  □ 有告警 (列出)
- [ ] 进行中的工单: 共 ___ 个
  - 工单 #___ : 状态 ___
  - 工单 #___ : 状态 ___
- [ ] 今日变更记录: 共 ___ 次
- [ ] 待处理事项:
  - 事项1: ___
  - 事项2: ___
- [ ] 集群整体状态: □ 正常  □ 异常

## 接班后检查清单
- [ ] 确认告警通道可接收: 钉钉/企微/Slack
- [ ] 确认集群可访问: kubectl get nodes
- [ ] 确认监控系统可达: Prometheus/Grafana
- [ ] 查看待处理工单和事项
- [ ] 确认联系方式和升级路径
```

---

### Day 4: 工具安装（第四天）

**目标**: 安装并熟悉所有调试工具

**内容**:
- kubectl 基础配置和别名
- k9s 终端 UI
- stern 日志工具
- kubescape 安全扫描
- Popeye 集群健康检查
- krew 插件集合（kubectx/kubens/debug）

**产出**: 能在本地终端高效操作集群

**参考文件**: [04-debug-tools-setup.md](./04-debug-tools-setup.md)

**工具安装命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# === kubectl 别名和自动补全 ===
cat >> ~/.zshrc << 'EOF'
# kubectl aliases
alias k=kubectl
alias kg='kubectl get'
alias kd='kubectl describe'
alias kl='kubectl logs'
alias ke='kubectl exec -it'
alias kns='kubectl config set-context --current --namespace'
complete -o default -F __start_kubectl k
source <(kubectl completion zsh)
EOF
source ~/.zshrc

# === k9s: 终端UI ===
brew install k9s
# 启动: k9s
# 常用快捷键:
#   :pod      切换到Pod视图
#   :svc      切换到Service视图
#   :ns       切换命名空间
#   l         查看日志
#   s         进入Shell
#   e         查看YAML

# === stern: 多Pod日志聚合 ===
brew install stern
# 使用示例: 跟踪所有nginx Pod日志
stern nginx -n default
# 预期输出:
# + my-nginx-abc123 [nginx] 10.0.1.5 - - [18/May/2026:10:30:00 +0000] "GET / HTTP/1.1" 200 612
# + my-nginx-def456 [nginx] 10.0.1.5 - - [18/May/2026:10:30:01 +0000] "GET / HTTP/1.1" 200 612

# === krew: kubectl 插件管理器 ===
brew install krew
echo 'export PATH="${KREW_ROOT:-$HOME/.krew}/bin:$PATH"' >> ~/.zshrc
source ~/.zshrc

# 安装常用插件
kubectl krew install ctx          # 快速切换上下文
kubectl krew install ns           # 快速切换命名空间
kubectl krew install debug        # 调试插件
kubectl krew install whoami       # 查看当前身份
kubectl krew install neat         # 清理YAML输出
kubectl krew install df-pv        # 查看PV使用情况
kubectl krew install resource-cap # 查看资源容量

# === kubescape: 安全扫描 ===
brew install kubescape
kubescape scan --exclude-namespaces kube-system
# 预期输出:
# +----------+-------------------+---------------------+--------+
# | Control  |    Control Name   | Docs                 | Status |
# +----------+-------------------+---------------------+--------+
# | C-0012   | Run as non-root   | https://...          | failed |
# | C-0013   | Read-only FS      | https://...          | passed |
# +----------+-------------------+---------------------+--------+
# Overall compliance: 78%

# === Popeye: 集群健康检查 ===
brew install derailed/popeye
popeye
# 预期输出:
#  ┌─────────────────────────────────────────────┐
#  │ Popeye · Cluster Health Scanner             │
#  ├─────────────────────────────────────────────┤
#  │ Pods        : 3 issues found                 │
#  │ Services    : 0 issues found                 │
#  │ Nodes       : 1 issue found                  │
#  │ Namespaces  : OK                             │
#  └─────────────────────────────────────────────┘
```
---

## 配置参考

### kubectl 配置文件示例 (~/.kube/config)

```yaml
apiVersion: v1
kind: Config
clusters:
- cluster:
    certificate-authority-data: LS0tLS1CRUdJTi...
    server: https://xxx.cn-hangzhou.aliyuncs.com:6443
  name: production-ack
- cluster:
    certificate-authority-data: LS0tLS1CRUdJTi...
    server: https://xxx.cn-hangzhou.aliyuncs.com:6443
  name: staging-ack
contexts:
- context:
    cluster: production-ack
    user: sso-user@company.com
    namespace: default
  name: prod
- context:
    cluster: staging-ack
    user: sso-user@company.com
    namespace: default
  name: staging
current-context: prod
users:
- name: sso-user@company.com
  user:
    exec:
      apiVersion: client.authentication.k8s.io/v1beta1
      command: kubectl-oidc_login
      args:
      - --oidc-issuer-url=https://auth.company.com
      - --oidc-client-id=kubectl
```

### kubeconfig 参数说明

| 参数 | 说明 | 示例 |
|------|------|------|
| `clusters[].cluster.server` | API Server 地址 | `https://xxx.aliyuncs.com:6443` |
| `clusters[].cluster.certificate-authority-data` | CA 证书 (Base64) | `LS0tLS1CRUdJTi...` |
| `contexts[].context.cluster` | 引用的集群名 | `production-ack` |
| `contexts[].context.user` | 引用的用户名 | `sso-user@company.com` |
| `contexts[].context.namespace` | 默认命名空间 | `default` |
| `current-context` | 当前使用的上下文 | `prod` |
| `users[].user.exec` | 外部凭证执行命令 | `kubectl-oidc_login` |

---

## 常见问题

### Q1: kubectl 连接集群报 "Unable to connect to the server" 怎么办？

**A**: 按以下顺序排查：
1. 检查网络连通性：`ping <api-server-address>` 或 `curl -k https://<api-server-address>:6443/healthz`
2. 检查 kubeconfig 文件路径：确认 `KUBECONFIG` 环境变量或 `~/.kube/config` 存在
3. 检查凭证是否过期：`kubectl auth whoami`
4. 检查 VPN/代理是否已连接（如需内网访问）
5. 检查证书是否有效：`openssl x509 -in <cert> -noout -dates`

### Q2: kubectl 报 "You must be logged in to the server (Unauthorized)" 怎么办？

**A**: 这是认证问题：
1. 确认 RAM 用户已授权 ACK 集群访问权限
2. 在 ACK 控制台重新下载 kubeconfig
3. 如果使用 SSO/OIDC，确认 SSO 服务可用并重新登录
4. 检查 kubeconfig 中的 exec 命令是否能正常执行

### Q3: 如何快速切换不同集群的访问配置？

**A**: 使用 kubectx 插件：
```bash
# 列出所有上下文
kubectx
# 预期输出:
# prod
# staging
# dev

# 切换到staging集群
kubectx staging
# Switched to context "staging".

# 切回上一个
kubectx -
# Switched to context "prod".

```

### Q4: 新人没有权限查看某些命名空间的 Pod 怎么办？

**A**: 这是正常的 RBAC 权限限制：
1. 联系集群管理员确认你的 RAM 账号已关联正确的 RBAC 角色
2. 在 ACK 控制台检查权限配置：集群 -> 授权管理
3. 临时查看自己有权限的命名空间：`kubectl auth can-i --list`

### Q5: Day 1 工具安装失败怎么办？

**A**: 常见解决方案：
1. macOS Homebrew 网络超时：配置国内镜像源或使用代理
2. kubectl 版本与集群版本差异过大：安装与集群版本匹配的 kubectl（允许 +/- 1 个小版本）
3. 磁盘空间不足：清理 Docker 镜像 `docker system prune -a`

### Q6: 如何确认我的权限范围？

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看当前身份
kubectl auth whoami

# 查看在default命名空间的权限
kubectl auth can-i --list -n default

# 检查特定操作权限
kubectl auth can-i create pods -n default
kubectl auth can-i delete deployments -n production
kubectl auth can-i get secrets -n kube-system
```
---

## 推荐阅读顺序

```
Day 1 → Day 2 → Day 3 → Day 4 → Week 1 培训
         ↓
    四周培训课程
         ↓
    考核评估体系
```

---

## 配套资源

| 资源 | 说明 |
|------|------|
| `domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md` | kubectl 场景速查卡 |
| `P1-5-oncall-quick-reference-card.md` | oncall 速查卡 |
| `domain-10-troubleshooting-diagnostics/` | 故障排查手册 |
| `domain-10-troubleshooting-diagnostics/topic-skills/assessment/daily-check-quiz.md` | 每日一题 |

---

## 新人自检清单

在完成 Day 1-4 后，你应该能够：

- [ ] `kubectl get pods -A` 正常执行
- [ ] `kubectl auth whoami` 显示正确的用户身份
- [ ] 登录 Prometheus/Grafana 查看监控数据
- [ ] 收到测试告警并能在 5 分钟内响应
- [ ] 创建一个测试 Pod 并成功删除
- [ ] 解释 Pod / Deployment / Service 的关系
- [ ] 知道当 Pod 处于 CrashLoopBackOff 时应该查看 `kubectl logs --previous`
- [ ] 知道当 Service 无 Endpoints 时应该检查 selector 匹配

---

## 要点总结

- 新人上手遵循 **Day 1 环境准备 → Day 2 工单处理 → Day 3 值班交接 → Day 4 工具安装** 的线性路径
- 每天的学习目标必须 **量化验证**，不要仅停留在"了解"层面
- **CrashLoopBackOff** 是最常见的工单类型，排查三板斧：`describe` → `logs --previous` → `get events`
- **Service 无 Endpoints** 的根因通常是 selector 不匹配或 Pod 未就绪
- 值班交接的核心是 **信息完整传递**，使用标准化模板确保不遗漏关键信息
- 善用工具链（k9s、stern、kubectx）可以大幅提升日常运维效率

---

## 延伸阅读

- [Kubernetes 官方文档 - kubectl 概述](https://kubernetes.io/docs/reference/kubectl/)
- [ACK 产品文档](https://help.aliyun.com/product/85222.html)
- [k9s 官方文档](https://k9scli.io/)
- [stern GitHub 仓库](https://github.com/stern/stern)
- [krew 插件索引](https://krew.sigs.k8s.io/plugins/)

---

```yaml
---
title: 新人上手快速路径（Quick Start）
last_updated: 2026-05-18
difficulty: beginner
intent_queries:
  - "新人上手路径"
  - "第一天做什么"
  - "oncall工具"
  - "K8s快速入门"
  - "值班工具安装"
trigger_keywords:
  - "QuickStart"
  - "快速上手"
  - "Day1"
  - "Day2"
  - "Day3"
  - "Day4"
  - "工具安装"
  - "kubeconfig"
  - "oncall入门"
reading_level: beginner
audience:
  - sre工程师
  - ops工程师
  - 新入职员工
estimated_read_time: 20min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training
  - domain-10-troubleshooting-diagnostics/topic-skills/assessment/k8s-fundamentals-quiz
  - P1-5-oncall-quick-reference-card
id: QUICKSTART-INDEX-001
topic: onboarding
type: index
tags: [onboarding, quick-start, day-1-4, new-engineer, k8s-1.28-1.33]
---
```

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[entities/release-notes-networking.md|发布说明索引 — 网络]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/01-gpu-scheduling-management|GPU 调度与管理]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-14-ai-ml-infra/01-ai-infra/02-distributed-training-frameworks|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[32-发布/package/2026-07-02_18-53/corpus/core/domain-07-platform-engineering/operate/01-monitoring-alerting-system|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]

```

<!-- risk-assessed -->
