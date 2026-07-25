---
title: K8s Ingress Gateway Failure 诊断与修复
description: Kubernetes Ingress 配置错误、Gateway 不通的完整诊断-修复-验证 Skill
summary: Kubernetes Ingress 配置错误、Gateway 不通的完整诊断-修复-验证 Skill
category: Kubernetes-Incident-Response
tags:
- k8s
- skills
- sop
- runbook
- ingress
- gateway
- service-mesh
- nginx-ingress
- istio
- traffic-routing
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 网络工程师
estimated_read_time: 5min
intent_queries:
- K8s Ingress Gateway Failure 诊断与修复 是什么
- 如何修复 Ingress 不通
trigger_keywords:
- Ingress
- Gateway
- 502 Bad Gateway
- 503 Service Unavailable
- ingress controller
- upstream
- backend
- virtualservice
- 路由不通
prerequisites:
- kubectl-basics
- ingress-concepts
- networking-basics
skill_id: SKILL-ING-001
skill_name: K8s Ingress Gateway Failure 诊断与修复
version: 1.0.0
k8s_versions:
- 1.28.x
- 1.29.x
- 1.30.x
- 1.31.x
- 1.32.x
agent_execution_mode: L2-semi-auto
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# K8s [[Ingress|Ingress]] Gateway Failure 诊断与修复

Ingress 和 Gateway 是 [[Kubernetes|Kubernetes]] 集群对外暴露服务的核心机制。配置错误、Controller 异常、后端服务不可达都会导致外部流量无法进入集群。

本 [[SKILL|Skill]] 覆盖 Ingress 规则错误、Gateway API 配置问题、Service 后端不可达、SSL 证书问题等全部常见根因的诊断和修复。

## 何时使用此 Skill

| 症状 | 检测方法 | 置信度 |
|------|---------|--------|
| 外部访问返回 502/503 | 直接 HTTP 请求测试 | 0.95 |
| Ingress 地址为空 | `kubectl get ingress` ADDRESS 列 | 0.90 |
| Gateway 没有 listener | `kubectl get gateway` | 0.85 |
| SSL 证书错误 | 浏览器或 curl 报错 | 0.95 |
| 路由规则不生效 | `kubectl describe ingress` | 0.85 |

**排除条件**: Pod 未运行 → SKILL-POD-001; Service 不通 → SKILL-SVC-001

## 快速分级（2 分钟内完成）

```
影响范围
├── 生产入口全部不可用 ──────────→ P0（15min 内修复）
├── 部分路由/域名不可用 ─────────→ P1（30min 内修复）
├── 新 Ingress/Gateway 不生效 ───→ P2（2h 内修复）
└── SSL 证书过期 ────────────────→ P1（30min 内修复）
```

**立即升级条件**:
- 所有生产入口同时不可用
- Ingress Controller Pod 全部 CrashLoopBackOff
- 证书过期导致全部 HTTPS 中断

## 执行流程

```
# 🟢 低风险：只读/信息收集，通常无副作用
工单/告警触发
    │
    ▼
┌──────────────┐    脚本: scripts/diagnose-quick.sh
│ Phase 1      │    内容: kubectl + curl 快速检查（只读）
│ 快速检查      │    Step: D1.1-D1.6
└──────┬───────┘
       │ 确认根因
       ▼
┌──────────────┐    参考: reference/remediation-playbook.md
│ 修复操作      │    风险: LOW → MEDIUM → HIGH
│ REM-001~006  │
└──────┬───────┘
       │
       ▼
┌──────────────┐    脚本: scripts/verify-ingress.sh
│ 验证确认      │    检查: HTTP/HTTPS 可达性
└──────────────┘
```
## 可用脚本

| 脚本 | 用途 | 参数 | 风险 |
|------|------|------|------|
| `scripts/diagnose-quick.sh` | Ingress/Gateway 快速诊断 | `NAMESPACE` `INGRESS_NAME` | 只读 |
| `scripts/verify-ingress.sh` | 修复后验证 | `NAMESPACE` `INGRESS_NAME` | 只读 |

## 根因概览 (6 种)

| RC ID | 根因 | 概率 | 首选修复 | 风险 |
|-------|------|------|---------|------|
| RC-001 | Ingress/Gateway 规则配置错误 | 高 | REM-001 修正规则 | LOW |
| RC-002 | Ingress Controller 异常 | 高 | REM-002 重启/修复 Controller | MEDIUM |
| RC-003 | 后端 Service 无可用 Endpoints | 高 | REM-003 修复后端服务 | MEDIUM |
| RC-004 | SSL/TLS 证书过期或配置错误 | 中 | REM-004 更新证书 | LOW |
| RC-005 | Gateway API controller 未安装 | 中 | REM-005 安装 controller | MEDIUM |
| RC-006 | DNS 解析不正确 | 中 | REM-006 修正 DNS | LOW |

## 关联资源

| 资源 | 路径 |
|------|------|
| 修复操作手册 | [reference/remediation-playbook.md](./reference/remediation-playbook.md) |
| 单文件完整版 | [../13-ingress-gateway-failure.md](../13-ingress-gateway-failure.md) |

## Related

- Ingress Gateway 知识图谱索引


## 远程顾问信息收集

> 作为远程顾问，我**无法直接连接你的集群**。请帮我收集以下信息，我会根据你提供的内容给出准确的诊断建议。

### 第一步：快速确认（30 秒内回答）

1. **影响范围**：这个问题影响多少个节点 / Pod / 命名空间？
2. **紧急程度**：业务是否已中断？是否有用户投诉？
3. **发生时间**：问题是突然发生还是逐渐恶化？最近是否有变更？

### 第二步：关键信息（请提供你能获取的）

4. **kubectl 版本**：`kubectl version --short` 的输出
5. **K8s 集群版本**：`kubectl get nodes -o wide` 中的 VERSION 列
6. **节点状态**：控制平面节点是否正常？工作节点是否正常？

### 第三步：诊断信息（按需补充）

> 如果以下命令你无法执行，请直接告诉我「无法执行」，我会提供替代方案。

7. **相关组件日志**：`kubectl logs -n <namespace> <pod>` 的最后 30 行
8. **节点资源**：`kubectl top nodes` 或 `kubectl describe node <node>` 的 Capacity/Allocated resources
9. **近期变更**：最近 24 小时是否有部署、扩缩容、配置变更？

### 如果信息不足

如果你目前只能提供部分信息，**请从第一步开始**。我会根据已有信息先给出初步判断，并告诉你还需要收集什么。

> **替代沟通方式**：如果你不方便执行命令，也可以直接描述你看到的页面/告警内容，我会帮你解读。


## 命令替代方案

> 如果你无法执行以下命令，请参考对应的替代方案。

### 通用替代方案

| 原命令 | 无法执行的原因 | 替代方案 A | 替代方案 B |
|:---|:---|:---|:---|
| `kubectl get pods` | 无 kubectl 权限 | 通过集群管理控制台查看 Pod 列表 | 请有权限的同事执行并截图 |
| `kubectl logs <pod>` | 无日志权限 | 查看应用自身的日志文件（/var/log/） | 使用日志聚合系统（如 ELK/Loki）查询 |
| `kubectl describe node <node>` | 无节点查看权限 | 查看监控系统的节点仪表盘 | 使用 `kubectl get node -o yaml`（如权限允许） |
| `ssh <node>` | 无法 SSH 到节点 | 使用 `kubectl debug node/<node> -it --image=busybox` | 通过跳板机访问：`ssh -J bastion <node>` |
| `systemctl status kubelet` | 无法进入节点 | 查看节点上的 kubelet 日志：`kubectl logs -n kube-system <kubelet-pod>` | 查看容器运行时日志 |
| `docker/crictl` | 无容器运行时权限 | 使用 `kubectl exec` 进入容器检查 | 查看容器运行时的事件 |

### 如果以上都无法执行

如果你因为安全策略、网络隔离或权限限制无法执行任何诊断命令：

1. **请收集你能访问的任何信息**：
   - 监控系统的截图
   - 告警通知的内容
   - 应用自身的错误页面/日志
   - 最近是否有变更（部署、扩缩容、配置更新）

2. **如果信息严重不足**：
   - 我会根据你描述的症状给出最可能的根因和修复建议
   - 但请注意：**信息不足时建议的置信度会降低**
   - 如果问题影响严重，建议立即升级给有权限的高级 SRE

3. **紧急情况下**：
   - 如果业务已中断且你无法执行任何操作
   - 请立即联系有集群管理员权限的同事
   - 同时可以准备以下信息以便快速交接：
     - 问题发生时间
     - 影响范围
     - 已尝试的操作
     - 当前的任何异常观察

## 异常反馈处理

以下场景工程师可能给出异常反馈，需准备应对：

- **Ingress配置正确但返回404** → 检查Ingress Class和Controller配置

- **证书正确但浏览器仍报错** → 检查证书链是否完整

- **高并发下Ingress返回502** → 检查后端连接超时和keepalive设置

- **重定向循环** → 检查ssl-redirect和rewrite-target注解配置


## 相关Skill交叉引用

本Skill诊断过程中可能涉及的其他Skill：

- [[19-故障诊断/08-技能体系/05-service-connectivity.md|05 service connectivity]]

- k8s-dns-failure

- [[19-故障诊断/08-技能体系/06-certificate-expiry.md|06 certificate expiry]]


当本Skill的诊断步骤无法定位根因时，建议按上述顺序排查相关Skill。

## 预防性措施

### Ingress配置规范
1. **TLS强制**：所有Ingress配置tls段，强制HTTPS
2. **超时调优**：根据后端服务调整proxy-read-timeout和proxy-send-timeout
3. **限速配置**：对公开端点配置rate limiting
4. **健康检查**：配置自定义健康检查路径

### 证书管理
- 使用cert-manager自动申请和续期Let's Encrypt证书
- 配置证书过期前30天告警
- 备用证书方案应对CA问题

## 诊断决策流程

```mermaid
flowchart TD
    A[工程师报告问题] --> B{Round 1: 快速确认}
    B -->|症状明确| C[执行针对性命令]
    B -->|症状模糊| D[执行通用检查命令]
    C --> E{Round 2: 深度诊断}
    D --> E
    E -->|定位根因| F[执行修复命令]
    E -->|根因不明| G[检查相关Skill]
    F --> H{Round 3: 验证修复}
    G --> H
    H -->|修复成功| I[结束并记录]
    H -->|修复失败| J[升级给高级SRE]
    I --> K[更新监控告警]
    J --> L[准备问题报告]

```

## 工具速查表

| 工具 | 用途 | 典型命令 |
|:---|:---|:---|
| kubectl | Kubernetes CLI | `kubectl get/describe/logs/exec` |
| jq | JSON处理 | `kubectl get ... -o json | jq ...` |
| openssl | 证书检查 | `openssl x509 -in <cert> -noout -dates` |
| tcpdump | 网络抓包 | `tcpdump -i any port <port> -n` |
| strace | 系统调用追踪 | `strace -p <pid> -f` |
| iostat/vmstat | IO/内存监控 | `iostat -x 1` |
| journalctl | 系统日志 | `journalctl -u <service> -f` |
| crictl | 容器运行时 | `crictl ps/logs/inspect` |

## 远程顾问执行清单

- [ ] 确认工程师身份和环境访问权限
- [ ] 收集集群版本、发行版、网络拓扑
- [ ] 确认问题影响范围和紧急程度
- [ ] 指导执行Round 1命令并收集输出
- [ ] 分析输出，选择Round 2分支
- [ ] 指导执行Round 2命令并收集输出
- [ ] 定位根因，提供修复方案
- [ ] 指导执行修复命令并验证
- [ ] 确认修复成功，更新相关文档
- [ ] 评估是否需要升级或事后复盘

## 典型生产案例

### 案例：证书过期导致HTTPS业务全中断
**场景**：凌晨3点，所有HTTPS入口返回证书过期错误，用户无法访问。
**诊断**：
1. openssl x509 -in <cert> -noout -dates
2. kubectl get certificate -n ingress-nginx
3. kubectl get clusterissuer
**修复**：
1. 紧急创建临时证书: openssl req -x509 -nodes -days 1 -newkey rsa:2048 -keyout temp.key -out temp.crt
2. kubectl create secret tls temp-tls --cert=temp.crt --key=temp.key -n ingress-nginx
3. 更新cert-manager或手动renew正式证书
**教训**：证书过期告警需提前30天触发，并配置自动续期。


## 相关概念

- [[22-概念/03-网络/ingress-controller.md|Ingress Controller]] — Ingress 控制器原理与配置

```

<!-- risk-assessed -->
