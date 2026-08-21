---
title: 证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis
description: Kubernetes 证书过期与 TLS 故障的完整诊断-修复-验证工单处理 Skill
summary: Kubernetes 证书过期与 TLS 故障的完整诊断-修复-验证工单处理 Skill
category: security
tags:
- k8s
- skills
- sop
- runbook
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
tier: core
created: '2026-05-23'
last_updated: '2026-04-26'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 30min
intent_queries:
- 证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis 是什么
- 如何 证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis
trigger_keywords:
- certificate expired
- x509
- 证书过期
- TLS handshake
- 证书错误
- cert-manager
- certificate renewal
- kubelet certificate
- 证书轮换
- unable to connect to the server
- certificate signed by unknown authority
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- service-mesh-basics
- prometheus-basics
- etcd-basics
- tls-basics
skill_id: SKILL-SEC-001
skill_name: 证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis
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




<!-- condition: kubeadm certs check-expiration 2>/dev/null | grep -E 'EXPIRES|expired' 显示证书即将过期或已过期 -->

# 证书过期与 TLS 故障诊断与修复 / Certificate Expiry & TLS Failure Diagnosis

---

## 1. 概述

证书（Certificate）是 [[kubernetes|Kubernetes]] 安全模型的基石。集群内几乎所有组件间通信都依赖 TLS 双向认证（mTLS）：apiserver ↔ [[kubelet|kubelet]]、apiserver ↔ [[etcd|etcd]]、apiserver ↔ controller-manager / scheduler、以及面向用户的 [[ingress|Ingress]] TLS 终止。**证书过期是 P0 级别事件** —— 它可以在瞬间让整个集群完全不可用，且恢复过程复杂、风险极高。

### Kubernetes 中的证书体系

Kubernetes 集群涉及以下几类证书，每类的有效期、管理方式和影响范围各不相同：

1. **API Server Serving Certificate** — apiserver 对外提供 HTTPS 服务的证书（默认 1 年，kubeadm 管理）
2. **kubelet Client Certificate** — kubelet 用于与 apiserver 通信的客户端证书（支持自动轮换）
3. **etcd Peer / Client Certificates** — etcd 集群成员间通信和 apiserver → etcd 的客户端证书（默认 1 年）
4. **Front-Proxy Certificate** — apiserver 聚合层（aggregation layer）使用的前端代理证书
5. **ServiceAccount Signing Key** — 用于签发 ServiceAccount token 的密钥对
6. **Webhook TLS Certificates** — ValidatingWebhookConfiguration / MutatingWebhookConfiguration 使用的 TLS 证书
7. **Ingress TLS Certificates** — 面向用户的 Ingress 入口 TLS 证书（通常由 [[cert-manager|cert-manager]] 管理）
8. **CA Certificates** — 上述各类证书的签发 CA（默认 10 年有效期，但确实会过期）

### 本 Skill 覆盖范围

- **集群基础设施证书**（kubeadm 管理）：apiserver、kubelet、etcd、front-proxy 等
- **应用层 TLS 证书**（cert-manager 管理）：Ingress TLS、Webhook TLS、自定义应用证书
- **主动预防**：证书过期监控、自动轮换验证、提前告警
- **紧急恢复**：kubectl 不可用时的证书恢复流程

### 前置条件

- **RBAC 权限**:
  - 最小权限: 对 `nodes`, `secrets`, `certificatesigningrequests` (certificates.k8s.io), `events` 的 `get/list/watch`
  - 修复权限: `secrets`, `certificatesigningrequests` 的 `create/update/delete`
  - 验证命令: `kubectl auth can-i list secrets`
- **SSH 访问**: 控制平面节点的 SSH 权限（用于直接操作证书文件）
- **工具要求**:
  - `kubectl` >= v1.28（客户端版本建议与集群版本相差不超过 1 个 minor）
  - `openssl` >= 1.1.1
  - `ssh`
  - `kubeadm` >= v1.28（kubeadm 管理的集群）
  - `jq` >= 1.6（推荐）
- **cert-manager 访问**（如适用）: 对 cert-manager namespace 的 `get/list/watch` 权限
- **证书文件路径**: 默认路径为 `/etc/kubernetes/pki/`（kubeadm 集群），自定义安装可能不同

> **重要**: 当 apiserver 证书过期时，所有 kubectl 命令将失败。此时必须通过 SSH 直接登录控制平面节点，使用 `kubeadm` 或 `openssl` 工具进行证书操作。本 Skill 包含 kubectl 不可用场景下的完整恢复流程。

---

## 2. 症状识别

### 2.1 症状模式表

| # | 症状描述 | 检测方法 | 置信度 | 排除条件 |
|---|---------|---------|--------|---------|
| S1 | `x509: certificate has expired or is not yet valid` 错误出现在 kubectl 输出、组件日志或应用日志中 / x509 certificate expired error in kubectl output or component logs | `kubectl` 执行任何命令时返回该错误；或 `journalctl -u kubelet` / apiserver 日志中包含 `x509: certificate has expired` | 0.95 | 证书有效但系统时钟偏移导致误判（检查 NTP 同步） |
| S2 | `kubectl` 所有命令均失败，返回 "Unable to connect to the server" / All kubectl commands fail with connection error | `kubectl get nodes` 返回 `Unable to connect to the server: x509: certificate has expired` 或 `net/http: TLS handshake timeout` | 0.90 | apiserver Pod/进程未运行（非证书问题）；网络不通（防火墙/安全组变更）；kubeconfig 中的 server 地址配置错误 |
| S3 | kubelet 无法与 apiserver 通信，节点状态变为 NotReady / kubelet unable to communicate with apiserver, node becomes NotReady | `kubectl get nodes` 显示节点 NotReady（如能执行）；或 SSH 到节点后 `journalctl -u kubelet` 日志中出现 `x509` 错误 | 0.85 | 节点网络分区（无 TLS 错误）→ SKILL-NET-002；kubelet 进程崩溃（非证书原因）→ SKILL-NODE-001 |
| S4 | etcd 集群成员间通信失败，apiserver 无法连接 etcd / etcd cluster members cannot communicate, apiserver cannot reach etcd | apiserver 日志出现 `connection error: desc = "transport: authentication handshake failed"`；etcd 日志出现 `rejected connection from ... (error "tls: failed to verify certificate")` | 0.85 | etcd 进程未运行；etcd 磁盘 I/O 超时；etcd 数据损坏 |
| S5 | TLS handshake 失败出现在应用日志或 Ingress Controller 日志中 / TLS handshake failure in application or ingress controller logs | Ingress Controller（如 nginx-ingress）日志出现 `SSL_do_handshake() failed`；应用日志出现 `tls: handshake failure` | 0.80 | 客户端不支持服务端的 TLS 版本/密码套件（非过期问题）；SNI 配置错误 |
| S6 | Webhook 调用失败，返回证书错误 / Webhook calls failing with certificate errors | `kubectl` 创建/更新资源时返回 `Internal error occurred: failed calling webhook ... x509: certificate has expired`；或 apiserver 日志中出现 webhook TLS 错误 | 0.80 | Webhook Service 不可达（网络问题）；Webhook caBundle 配置为空 |
| S7 | Ingress TLS 证书过期，浏览器显示安全警告 / Ingress TLS certificate expired, browser shows security warning | 浏览器访问 HTTPS 站点显示 `NET::ERR_CERT_DATE_INVALID`；`echo | openssl s_client -connect <host>:443 -servername <host> 2>/dev/null | openssl x509 -noout -dates` 显示 notAfter 已过 | 0.90 | 证书有效但域名不匹配（SAN 问题）；证书链不完整（缺少中间 CA） |
| S8 | cert-manager Certificate 资源显示 Ready=False / cert-manager Certificate shows Ready=False status | `kubectl get certificates -A` 显示 READY 列为 `False`；`kubectl describe certificate <name> -n <ns>` 的 Events 中有错误信息 | 0.85 | cert-manager 控制器未运行（非证书过期问题）；ACME DNS challenge 的 DNS 传播延迟（需等待） |
| S9 | `certificate signed by unknown authority` 错误 / Certificate signed by unknown authority error | kubectl 或组件日志中出现 `x509: certificate signed by unknown authority` | 0.75 | CA 证书被轮换但组件未更新 CA bundle（是 CA 不匹配，不一定是过期）；自签名证书环境中缺少 CA 信任配置 |

### 2.2 工单关键词映射

以下为常见工单描述示例，Agent 应将其映射到本 Skill：

**中文工单描述**:
- "kubectl 连不上集群了，报证书过期"
- "集群证书到期了，所有命令都用不了"
- "节点通信出现 x509 错误"
- "网站 HTTPS 证书过期，浏览器报不安全"
- "cert-manager 证书签发失败"
- "etcd 报 TLS 握手错误"
- "Webhook 调用失败，证书问题"
- "kubelet 证书轮换失败"
- "集群一年没续签证书，现在挂了"
- "Ingress TLS 证书快过期了，需要更新"

**English ticket descriptions**:
- "kubectl failing with certificate expired error"
- "Cluster certificates expired, unable to manage cluster"
- "x509 certificate has expired error in kubelet logs"
- "TLS handshake failure between components"
- "Website HTTPS certificate expired"
- "cert-manager certificate not renewing"
- "Webhook admission failing with TLS error"
- "etcd peer certificate expired"
- "Need to renew kubeadm certificates"
- "Certificate signed by unknown authority after upgrade"

### 2.3 排除标准

以下场景**不适用**本 Skill，Agent 应路由到其他 Skill 或手动处理：

| 排除条件 | 正确路由 | 说明 |
|---------|---------|------|
| 仅 Service 连通性失败，无任何 TLS/x509 错误 | SKILL-NET-002 | 网络级问题，非证书问题 |
| 节点 NotReady 但日志中无证书错误 | SKILL-NODE-001 | 节点问题的其他根因（资源压力、运行时问题等） |
| Pod CrashLoopBackOff 但非 TLS 相关 | SKILL-POD-001 | 应用自身错误 |
| TLS 版本/密码套件不兼容（非过期） | 安全配置调优 | 需要调整 TLS 配置而非证书更新 |
| 证书有效但域名/IP 不匹配（SAN 问题） | 证书重新签发 | 证书未过期，需重新生成含正确 SAN 的证书 |
| OIDC / LDAP 外部认证问题 | 认证配置排查 | 不属于 X.509 证书过期范畴 |

---

## 3. 快速分级（2 分钟内完成）

### 3.1 影响评估

按顺序执行以下命令，判断问题爆炸半径：

**Step T1**: 验证 kubectl 是否可用（判断 apiserver 证书是否过期）
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 尝试执行最简单的 kubectl 命令
kubectl version --short 2>&1
# 或
kubectl get ns default 2>&1
```
> **判断规则**:
> - 命令成功执行 → apiserver serving cert 有效，问题可能在其他证书层面，继续 T2
> - 返回 `x509: certificate has expired` → **P0 立即**，apiserver 证书过期，集群管理不可用
> - 返回 `Unable to connect to the server` → 可能是证书过期或网络问题，需 SSH 确认
> - 返回 `certificate signed by unknown authority` → CA 可能被更换或 kubeconfig 中的 CA 数据不正确

**Step T2**: 判断受影响的证书类型（基础设施 vs 应用层）
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果 kubectl 可用：检查 cert-manager 证书状态
kubectl get certificates -A 2>/dev/null
# 检查节点状态（kubelet 证书是否影响节点通信）
kubectl get nodes 2>/dev/null
# 检查 webhook 证书
kubectl get validatingwebhookconfigurations -o name 2>/dev/null
kubectl get mutatingwebhookconfigurations -o name 2>/dev/null
```
> **判断规则**:
> - 所有节点 NotReady + kubectl 工作异常 → 基础设施证书问题（P0）
> - 部分节点 NotReady + kubelet 日志有 x509 错误 → kubelet 证书问题（P0-P1）
> - cert-manager Certificate Ready=False → 应用层证书问题（P1-P2）
> - Webhook 调用失败 → Webhook 证书问题（P1）

**Step T3**: 评估受影响组件数量
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果 kubectl 可用：
# 检查有多少 Certificate 资源处于非 Ready 状态
kubectl get certificates -A --no-headers 2>/dev/null | grep -c "False"

# 检查有多少节点因证书错误而 NotReady
kubectl get nodes --no-headers 2>/dev/null | grep -c "NotReady"

# 如果 kubectl 不可用，SSH 到控制平面节点：
# 检查 kubeadm 管理的证书到期状态
kubeadm certs check-expiration 2>/dev/null
```
> **判断规则**:
> - kubeadm 证书检查显示多个证书过期 → P0
> - 仅单个应用证书过期 → P1-P2
> - 仅 Ingress TLS 过期 → P2

**Step T4**: 确认过期时间（已过期 vs 即将过期）
```bash
# 如果可以 SSH 到控制平面节点：
# 快速检查 apiserver 证书到期时间
echo | openssl s_client -connect localhost:6443 2>/dev/null | openssl x509 -noout -dates 2>/dev/null
# 或直接检查文件
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -enddate 2>/dev/null
```
> **判断规则**:
> - 证书已过期 → 立即执行修复
> - 证书将在 24h 内过期 → 紧急预防性续签
> - 证书将在 7 天内过期 → 尽快安排维护窗口续签
> - 证书有效期 >7 天 → 不属于本 Skill 的紧急处理范畴

### 3.2 严重性分级

| 条件 | 级别 | 说明 | SLA 要求 |
|------|------|------|---------|
| apiserver / etcd 证书过期，kubectl 不可用 | **P0** | 集群完全不可用，所有管理操作瘫痪。etcd 证书过期可能导致数据访问完全中断 | 立即响应，15min 内开始恢复 |
| 多节点 kubelet 证书过期，节点批量 NotReady | **P0** | 大量工作负载受影响，集群可用性严重降级 | 立即响应，30min 内恢复 |
| 单节点 kubelet 证书过期 **或** Webhook 证书过期影响生产部署 **或** cert-manager 证书过期影响生产流量 | **P1** | 部分组件或工作负载受影响，但集群核心功能可用 | 15min 内响应，1h 内修复 |
| Ingress TLS 证书过期（用户可见） **或** 非关键 cert-manager 证书过期 | **P2** | 外部用户看到 HTTPS 安全警告，影响用户体验和信任，但后端服务正常 | 30min 内响应，2h 内修复 |
| 证书即将过期（7 天内）但当前未过期 | **P3** | 预防性处理，无当前影响 | 工作时间内处理 |

### 3.3 立即升级触发条件

以下任一条件满足时，**跳过诊断流程，立即升级至高级 SRE / 值班工程师**：

- **apiserver 证书过期且 kubectl 完全不可用**: 所有远程管理能力丧失，需要 SSH 到控制平面节点进行紧急恢复
- **etcd 证书过期**: etcd 集群可能丢失 quorum，存在数据丢失风险
- **CA 证书过期**: 影响所有由该 CA 签发的证书，需要集群范围内的证书重新签发
- **多个控制平面组件证书同时过期**: 复合问题，恢复过程复杂且有严格顺序要求
- **证书过期 + etcd 不健康**: 可能需要 etcd 数据恢复，超出常规证书恢复范畴

> **升级消息模板**: 参见 Section 8.2

---

## 4. 诊断工作流

### Phase 1: 快速检查（只读，零风险）

> **目标**: 快速定位是哪类证书过期，确定问题影响范围。优先使用 kubectl（如果可用），否则 SSH 到控制平面节点。
> **预计耗时**: 2-5 分钟

**Step D1.1**: 检查 kubeadm 管理的证书到期状态
- **命令**:
  ```bash
  # SSH 到控制平面节点执行（如果 kubectl 不可用，这是首选方法）
  kubeadm certs check-expiration
  ```
- **超时**: 10s
- **预期输出模式**: 表格输出包含各证书名称、到期时间、剩余有效期、CA 名称
  ```
  CERTIFICATE                EXPIRES                  RESIDUAL TIME   CERTIFICATE AUTHORITY   EXTERNALLY MANAGED
  admin.conf                 Mar 05, 2027 00:00 UTC   364d            ca                      no
  apiserver                  Mar 05, 2027 00:00 UTC   364d            ca                      no
  apiserver-etcd-client      Mar 05, 2027 00:00 UTC   364d            etcd-ca                 no
  apiserver-kubelet-client   Mar 05, 2027 00:00 UTC   364d            ca                      no
  controller-manager.conf    Mar 05, 2027 00:00 UTC   364d            ca                      no
  etcd-healthcheck-client    Mar 05, 2027 00:00 UTC   364d            etcd-ca                 no
  etcd-peer                  Mar 05, 2027 00:00 UTC   364d            etcd-ca                 no
  etcd-server                Mar 05, 2027 00:00 UTC   364d            etcd-ca                 no
  front-proxy-client         Mar 05, 2027 00:00 UTC   364d            front-proxy-ca          no
  scheduler.conf             Mar 05, 2027 00:00 UTC   364d            ca                      no
  
  CERTIFICATE AUTHORITY   EXPIRES                  RESIDUAL TIME   EXTERNALLY MANAGED
  ca                      Mar 03, 2036 00:00 UTC   9y              no
  etcd-ca                 Mar 03, 2036 00:00 UTC   9y              no
  front-proxy-ca          Mar 03, 2036 00:00 UTC   9y              no
  ```
- **判断规则**:
  - 任何证书的 RESIDUAL TIME 显示为负值或 `invalid` → 该证书已过期，记录证书名称
  - RESIDUAL TIME < 24h → 即将过期，需立即处理
  - EXTERNALLY MANAGED 为 `yes` → 该证书由外部 CA 管理（如 Vault），kubeadm 无法续签
  - 命令执行失败（kubeadm 不存在）→ 非 kubeadm 管理集群，需手动检查证书文件
- **版本差异**:
  - **[v1.28+]**: `kubeadm certs check-expiration` 输出格式稳定
  - **[v1.31+]**: 增强了对外部管理证书的检测提示

**Step D1.2**: 检查 apiserver serving certificate
- **命令**:
  ```bash
  # 从网络层检查（可在集群外部执行）
  echo | openssl s_client -connect <apiserver-host>:6443 2>/dev/null | openssl x509 -noout -dates -subject -issuer

  # 从文件系统检查（SSH 到控制平面节点）
  openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates -subject -issuer -ext subjectAltName
  ```
- **超时**: 10s
- **预期输出模式**: 证书日期、Subject、Issuer、SAN 信息
- **判断规则**:
  - `notAfter` 早于当前时间 → apiserver serving cert 已过期（RC-001），apiserver 无法提供 HTTPS 服务
  - `notAfter` 在 7 天内 → 需要紧急续签
  - Subject / SAN 不包含当前 apiserver 的 IP 或 hostname → SAN 不匹配（RC-012）
  - Issuer 与预期 CA 不一致 → CA 可能被替换
- **版本差异**: 无

**Step D1.3**: 检查 kubelet 客户端证书
- **命令**:
  ```bash
  # SSH 到问题节点
  openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject 2>/dev/null
  # 如果上述路径不存在，尝试：
  openssl x509 -in /var/lib/kubelet/pki/kubelet-client.pem -noout -dates -subject 2>/dev/null
  ```
- **超时**: 10s
- **预期输出模式**: 证书日期和 Subject 信息
- **判断规则**:
  - `notAfter` 早于当前时间 → kubelet 客户端证书过期（RC-002）
  - 证书文件不存在 → 证书自动轮换可能失败或证书被误删
  - Subject 的 O (Organization) 字段应为 `system:nodes`
- **版本差异**:
  - **[v1.28+]**: kubelet 客户端证书自动轮换（RotateKubeletClientCertificate）已 GA 并默认启用

**Step D1.4**: 检查 cert-manager 证书状态（如果集群使用 cert-manager）
- **命令**:
  ```bash
  # 列出所有 Certificate 资源及其状态
  kubectl get certificates -A -o wide

  # 列出所有 CertificateRequest 资源
  kubectl get certificaterequests -A

  # 检查 cert-manager 控制器是否运行
  kubectl get pods -n cert-manager
  ```
- **超时**: 15s
- **预期输出模式**: Certificate 列表显示 READY 状态
- **判断规则**:
  - READY 列为 `False` → 证书签发或续签失败（RC-004 或 RC-005）
  - cert-manager Pod 不 Running → cert-manager 控制器异常，无法签发证书
  - CertificateRequest 状态为 `Failed` → 查看 describe 获取失败原因
  - 所有 Certificate Ready=True → cert-manager 管理的证书正常
- **版本差异**: 无（取决于 cert-manager 版本而非 K8s 版本）

**Step D1.5**: 快速检查所有 Ingress TLS 证书有效期
- **命令**:
  ```bash
  # 列出所有 Ingress 及其 TLS secret 名称
  kubectl get ingress -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.metadata.name}: {range .spec.tls[*]}secret={.secretName} hosts={.hosts[*]}{"; "}{end}{"\n"}{end}'

  # 检查特定 TLS secret 中证书的有效期
  kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates -subject
  ```
- **超时**: 15s
- **预期输出模式**: Ingress 与 TLS secret 的映射关系、证书有效期
- **判断规则**:
  - `notAfter` 早于当前时间 → Ingress TLS 证书已过期（RC-009）
  - Secret 不存在 → TLS secret 可能被误删或 cert-manager 未创建
  - Secret 存在但无 `tls.crt` 字段 → Secret 格式错误
- **版本差异**: 无

---

### Certificate Type Decision Tree（证书类型决策树）

根据 Phase 1 检查结果，按以下决策树进入对应的深度诊断路径：

```
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl 不可用?
├── 是 → SSH 到控制平面 → kubeadm certs check-expiration
│   ├── apiserver 证书过期 → 进入 Phase 2: D2.1 (apiserver cert path)
│   ├── etcd 证书过期 → 进入 Phase 2: D2.3 (etcd cert path)
│   └── 多个证书过期 → 进入 Phase 2: D2.1 → D2.3 (按优先级逐个处理)
│
└── 否 → kubectl 可用
    ├── 节点 NotReady + kubelet x509 错误 → D1.3 → Phase 2: D2.2 (kubelet cert path)
    ├── cert-manager Certificate Not Ready → D1.4 → Phase 2: D2.5 (cert-manager path)
    ├── Ingress TLS 过期 → D1.5 → Phase 2: D2.9 (application cert path)
    ├── Webhook TLS 错误 → Phase 2: D2.8 (webhook cert path)
    └── etcd 日志有 TLS 错误 → Phase 2: D2.3 (etcd cert path)
```
---

### Phase 2: 深度检查（只读，零风险）

> **目标**: 深入检查特定证书的详细信息，确认根因。部分操作需 SSH 到控制平面或工作节点。
> **预计耗时**: 5-15 分钟

**Step D2.1**: 检查 apiserver 证书详细信息
- **命令**:
  ```bash
  # 完整证书信息
  openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text

  # 关键字段摘要
  openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout \
    -dates -subject -issuer -ext subjectAltName -serial

  # 验证证书链
  openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt
  ```
- **超时**: 10s
- **预期输出模式**: 完整证书详情，包含 Not Before、Not After、Subject、SAN、Issuer、验证结果
- **判断规则**:
  - 验证结果为 `OK` → 证书链完整
  - 验证失败 `certificate has expired` → 确认过期（RC-001）
  - 验证失败 `unable to get local issuer certificate` → CA 证书缺失或不匹配（RC-006）
  - SAN 中缺少当前节点 IP → 证书 SAN 不匹配（RC-012），可能节点 IP 变更后未重新签发
- **版本差异**: 无

**Step D2.2**: 检查 kubelet 证书自动轮换状态
- **命令**:
  ```bash
  # 检查 kubelet 配置中的轮换设置
  ssh <node-ip> "cat /var/lib/kubelet/config.yaml | grep -A 2 -i rotate"

  # 检查 kubelet 证书轮换相关日志
  ssh <node-ip> "journalctl -u kubelet --since '1 hour ago' --no-pager | grep -i 'certificate|rotate|csr|x509'"

  # 检查证书文件的符号链接（auto-rotation 使用 current 链接）
  ssh <node-ip> "ls -la /var/lib/kubelet/pki/"
  ```
- **超时**: 15s
- **预期输出模式**: kubelet 配置和证书文件列表
- **判断规则**:
  - `rotateCertificates: true` → 自动轮换已启用（默认行为）
  - `rotateCertificates: false` 或缺少该配置 → 自动轮换未启用（RC-002 的原因之一）
  - `kubelet-client-current.pem` 是符号链接指向时间戳文件 → 自动轮换正在工作
  - 日志中出现 `certificate rotation` + `error` → 轮换过程遇到错误
  - 日志中出现 `failed to request certificate` → CSR 请求失败或未被批准
- **版本差异**:
  - **[v1.28+]**: RotateKubeletClientCertificate 已 GA，默认启用
  - **[v1.29+]**: 改进的证书轮换日志，包含更详细的错误信息

**Step D2.3**: 检查 etcd 证书
- **命令**:
  ```bash
  # SSH 到控制平面节点，逐一检查 etcd 相关证书
  for cert in server peer ca healthcheck-client; do
    echo "=== etcd $cert ===" && openssl x509 -in /etc/kubernetes/pki/etcd/${cert}.crt -noout -dates -subject 2>/dev/null
  done
  # apiserver 到 etcd 的客户端证书
  openssl x509 -in /etc/kubernetes/pki/apiserver-etcd-client.crt -noout -dates -subject
  # 验证证书链
  openssl verify -CAfile /etc/kubernetes/pki/etcd/ca.crt /etc/kubernetes/pki/etcd/server.crt
  openssl verify -CAfile /etc/kubernetes/pki/etcd/ca.crt /etc/kubernetes/pki/etcd/peer.crt
  ```
- **超时**: 15s
- **预期输出模式**: 各 etcd 证书的有效期和验证结果
- **判断规则**:
  - etcd server 或 peer 证书过期 → RC-003（etcd 证书过期），etcd 集群通信将中断
  - apiserver-etcd-client 证书过期 → apiserver 无法连接 etcd
  - etcd CA 证书过期 → RC-006（CA 过期），最严重情况，需要完整 CA 轮换
  - healthcheck-client 过期 → etcd 健康检查失败，但不影响核心数据通信
- **版本差异**: 无

**Step D2.4**: 检查 front-proxy 证书
- **命令**:
  ```bash
  # SSH 到控制平面节点
  openssl x509 -in /etc/kubernetes/pki/front-proxy-client.crt -noout -dates -subject -issuer
  openssl x509 -in /etc/kubernetes/pki/front-proxy-ca.crt -noout -dates -subject

  # 验证证书链
  openssl verify -CAfile /etc/kubernetes/pki/front-proxy-ca.crt /etc/kubernetes/pki/front-proxy-client.crt
  ```
- **超时**: 10s
- **预期输出模式**: front-proxy 证书有效期和验证结果
- **判断规则**:
  - front-proxy-client 过期 → RC-010（front-proxy 证书过期），影响 API aggregation layer（如 metrics-server）
  - front-proxy-ca 过期 → 需要 CA 轮换
  - 验证通过 → front-proxy 证书正常
- **版本差异**: 无

**Step D2.5**: 检查 cert-manager Issuer 健康状态
- **命令**:
  ```bash
  kubectl get clusterissuer -o wide
  kubectl get issuer -A -o wide
  kubectl describe certificate <cert-name> -n <namespace>
  kubectl describe certificaterequest <cr-name> -n <namespace>
  kubectl logs -n cert-manager deployment/cert-manager --tail=100 | grep -i "error|fail|expire"
  # 检查 ACME Order 和 Challenge（如果使用 Let's Encrypt）
  kubectl get orders -A
  kubectl get challenges -A
  ```
- **超时**: 20s
- **预期输出模式**: Issuer 状态、Certificate 详情、CertificateRequest 状态
- **判断规则**:
  - ClusterIssuer Ready=False → Issuer 配置错误（RC-004），检查 describe 输出的 Events
  - CertificateRequest 状态 Failed → 签发失败，可能是 ACME challenge 失败（RC-005）或 Issuer 配置错误
  - Challenge 状态 pending 或 failed → ACME challenge 失败（DNS/HTTP 验证问题）
  - cert-manager 日志有 `rate limit` → Let's Encrypt 限流（RC-005）
  - cert-manager 日志有 `connection refused` → 无法连接 ACME server
- **版本差异**: 无（取决于 cert-manager 版本）

**Step D2.6**: 检查时间同步状态
- **命令**:
  ```bash
  ssh <node-ip> "timedatectl status"
  ssh <node-ip> "chronyc tracking 2>/dev/null || ntpstat 2>/dev/null || echo 'No NTP client found'"
  ssh <node-ip> "date -u"
  ```
- **超时**: 10s
- **预期输出模式**: 时间同步状态
- **判断规则**:
  - `System clock synchronized: no` → NTP 未同步（RC-007），可能导致有效证书被误判为过期
  - 时间偏差 > 5 秒 → 可能导致证书验证失败
  - 时间偏差 > 1 分钟 → 几乎确定导致 TLS 握手失败
  - 时间偏差 > 1 小时 → 严重时钟偏移，证书验证和 Lease 续租均受影响
  - 时间同步正常 → 排除时间因素，确认为证书本身过期
- **版本差异**: 无

**Step D2.7**: 检查 CA 证书链完整性
- **命令**:
  ```bash
  # CA 证书剩余有效期
  echo "=== Kubernetes CA ===" && openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -dates -subject
  echo "=== etcd CA ===" && openssl x509 -in /etc/kubernetes/pki/etcd/ca.crt -noout -dates -subject
  echo "=== front-proxy CA ===" && openssl x509 -in /etc/kubernetes/pki/front-proxy-ca.crt -noout -dates -subject

  # 验证证书链完整性
  openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt
  openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver-kubelet-client.crt
  openssl verify -CAfile /etc/kubernetes/pki/etcd/ca.crt /etc/kubernetes/pki/etcd/server.crt
  openssl verify -CAfile /etc/kubernetes/pki/front-proxy-ca.crt /etc/kubernetes/pki/front-proxy-client.crt
  ```
- **超时**: 15s
- **预期输出模式**: CA 有效期和证书链验证结果
- **判断规则**:
  - CA 证书过期 → RC-006（CA 过期），这是最严重情况，所有由该 CA 签发的证书都将不可信
  - 验证失败 `certificate signature failure` → CA 已被替换但下游证书未更新
  - CA 有效期 < 1 年 → 预警，需规划 CA 轮换
  - 所有验证通过 → CA 链完整
- **版本差异**: 无

**Step D2.8**: 检查 Webhook TLS 证书
- **命令**:
  ```bash
  # 检查 Webhook 的 caBundle 信息
  kubectl get validatingwebhookconfigurations -o json | \
    jq '.items[] | {name: .metadata.name, webhooks: [.webhooks[] | {name: .name, caBundle_length: (.clientConfig.caBundle // "" | length)}]}'

  # 解码并检查特定 webhook 的 caBundle 证书有效期
  kubectl get validatingwebhookconfiguration <webhook-name> \
    -o jsonpath='{.webhooks[0].clientConfig.caBundle}' | base64 -d | openssl x509 -noout -dates -subject

  # 检查 webhook service 的 TLS 证书
  kubectl get secret <webhook-tls-secret> -n <ns> \
    -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates -subject
  ```
- **超时**: 15s
- **预期输出模式**: Webhook caBundle 信息和证书有效期
- **判断规则**:
  - caBundle 长度为 0 → caBundle 未配置（可能使用了 cert-manager 的 ca-injector）
  - caBundle 中的证书过期 → Webhook CA 过期（RC-008）
  - Webhook TLS secret 中的证书过期 → Webhook 服务证书过期（RC-008）
  - caBundle 与 webhook service 实际使用的 CA 不一致 → CA 不匹配（RC-008）
- **版本差异**: 无

**Step D2.9**: 检查 Ingress TLS Secret 中的证书
- **命令**:
  ```bash
  # 检查特定 TLS secret 中的证书详情
  kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | \
    base64 -d | openssl x509 -noout -dates -subject -issuer -ext subjectAltName

  # 批量检查所有 Ingress TLS secret 的到期时间
  for ns_secret in $(kubectl get ingress -A -o jsonpath='{range .items[*]}{.metadata.namespace}/{.spec.tls[0].secretName}{"\n"}{end}' 2>/dev/null); do
    ns=${ns_secret%%/*}; secret=${ns_secret##*/}
    [ -n "$secret" ] && [ "$secret" != "null" ] && echo -n "$ns/$secret: " && \
      kubectl get secret $secret -n $ns -o jsonpath='{.data.tls\.crt}' 2>/dev/null | base64 -d | openssl x509 -noout -enddate 2>/dev/null || echo "ERROR"
  done
  ```
- **超时**: 30s
- **预期输出模式**: 各 TLS secret 的证书到期日期
- **判断规则**:
  - `notAfter` 早于当前时间 → Ingress TLS 证书过期（RC-009）
  - 无法读取证书 → Secret 不存在或格式错误
  - Issuer 为 Let's Encrypt 且已过期 → 检查 cert-manager 自动续签（D2.5）
  - Issuer 非 cert-manager → 手动管理的证书，需手动更新
- **版本差异**: 无

---

### Phase 3: 主动探测（低风险，可能需审批）

**Step D3.1**: 使用特定证书验证端到端连接
- **命令**:
  ```bash
  # 使用 apiserver 客户端证书测试连接
  curl -v --cacert /etc/kubernetes/pki/ca.crt \
    --cert /etc/kubernetes/pki/apiserver-kubelet-client.crt \
    --key /etc/kubernetes/pki/apiserver-kubelet-client.key \
    https://<apiserver-ip>:6443/healthz

  # 使用 etcd 客户端证书测试 etcd 连接
  curl -v --cacert /etc/kubernetes/pki/etcd/ca.crt \
    --cert /etc/kubernetes/pki/etcd/healthcheck-client.crt \
    --key /etc/kubernetes/pki/etcd/healthcheck-client.key \
    https://127.0.0.1:2379/health
  ```
- **超时**: 15s
- **风险级别**: 🟢 低（只读 HTTP GET 请求）
- **预期输出模式**: TLS 握手详情和 HTTP 响应
- **判断规则**:
  - TLS 握手成功 + HTTP 200 → 该证书对可用于正常通信
  - `SSL certificate problem: certificate has expired` → 确认证书过期
  - `SSL certificate problem: unable to get local issuer certificate` → CA 链断裂
  - `SSL peer certificate or SSH remote key was not OK` → 证书验证失败
- **版本差异**: 无

**Step D3.2**: 测试 cert-manager 证书签发能力
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 创建临时 Certificate 资源以验证 Issuer 工作正常
  # 注意：这将创建一个测试资源，测试后需要删除
  cat <<EOF | kubectl apply -f -
  apiVersion: cert-manager.io/v1
  kind: Certificate
  metadata:
    name: test-cert-issuance
    namespace: default
  spec:
    secretName: test-cert-issuance-tls
    duration: 1h
    renewBefore: 30m
    issuerRef:
      name: <issuer-name>
      kind: ClusterIssuer  # 或 Issuer
    commonName: test.example.com
    dnsNames:
      - test.example.com
  EOF

  # 等待签发结果
  kubectl get certificate test-cert-issuance -n default -w

  # 清理测试资源
  kubectl delete certificate test-cert-issuance -n default
  kubectl delete secret test-cert-issuance-tls -n default 2>/dev/null
  ```
- **超时**: 120s（ACME 签发可能较慢）
- **风险级别**: 🟡 中（创建临时资源，需审批）
- **预期输出模式**: Certificate 状态变化
- **判断规则**:
  - Certificate 变为 Ready=True → Issuer 工作正常，问题可能在特定 Certificate 的配置
  - Certificate 保持 Ready=False → Issuer 自身有问题（RC-004 / RC-005）
  - CertificateRequest 失败 → 查看详细错误信息
- **版本差异**: 无

**Step D3.3**: 检查 CSR（CertificateSigningRequest）审批状态
- **命令**:
  ```bash
  # 列出所有 CSR 及其状态
  kubectl get csr

  # 检查 pending 的 CSR 详情
  kubectl get csr --no-headers | grep -i pending | while read name _; do
    echo "=== $name ==="
    kubectl describe csr $name
  done
  ```
- **超时**: 10s
- **风险级别**: 🟢 低（只读操作）
- **预期输出模式**: CSR 列表及状态
- **判断规则**:
  - 存在 Pending 状态的 CSR → kubelet 证书轮换请求未被批准（RC-002 的子原因）
  - CSR 被 Denied → 检查 deny 原因
  - 无 Pending CSR 且 kubelet 证书已过期 → 自动轮换机制未触发或 kubelet 无法提交 CSR
- **版本差异**:
  - **[v1.28+]**: certificates.k8s.io/v1 API 稳定
  - **[v1.30+]**: Structured authorization configuration 可能影响 CSR 的自动批准策略

---

### Phase 4: cert-manager 自动轮转排查

> **目标**: 深入检查 cert-manager 证书管理流程，定位自动轮转失败的根因。
> **预计耗时**: 5-10 分钟
> **前置条件**: 集群已部署 cert-manager

**Step D4.1**: 检查 Certificate 资源状态
- **命令**:
  ```bash
  # 获取所有 Certificate 资源的详细状态
  kubectl get certificates -A -o wide
  
  # 检查特定 Certificate 的详细信息
  kubectl describe certificate <cert-name> -n <namespace>
  
  # 检查 Certificate 的 conditions
  kubectl get certificate <cert-name> -n <namespace> -o jsonpath='{range .status.conditions[*]}{.type}{"="}{.status}{" reason="}{.reason}{" message="}{.message}{"\n"}{end}'
  ```
- **超时**: 15s
- **预期输出模式**: Certificate 列表显示 READY 状态和到期时间
- **判断规则**:
  - Ready=True → 证书当前有效，检查 renewalTime 是否即将到期
  - Ready=False + reason=Issuing → 正在签发中，可能是正常轮转
  - Ready=False + reason=Failed → 签发失败，需检查 CertificateRequest
  - notAfter 早于当前时间 → 证书已过期（RC-004/RC-005）
- **版本差异**: 无（取决于 cert-manager 版本）

**Step D4.2**: 检查 CertificateRequest 状态
- **命令**:
  ```bash
  # 获取所有 CertificateRequest
  kubectl get certificaterequests -A -o wide
  
  # 检查与特定 Certificate 关联的 CertificateRequest
  kubectl get certificaterequests -n <namespace> -l cert-manager.io/certificate-name=<cert-name>
  
  # 检查 CertificateRequest 详情
  kubectl describe certificaterequest <cr-name> -n <namespace>
  ```
- **超时**: 10s
- **预期输出模式**: CertificateRequest 列表显示 Ready 状态
- **判断规则**:
  - Ready=True + Approved=True → 请求已批准并签发
  - Ready=False + Approved=False → 请求未被批准，检查审批策略
  - Ready=False + Denied=True → 请求被拒绝，查看 deny 原因
  - Failed=True → Issuer 签发失败，检查 Events
- **版本差异**: 无

**Step D4.3**: 检查 Issuer/ClusterIssuer 状态
- **命令**:
  ```bash
  # 获取所有 Issuer 和 ClusterIssuer
  kubectl get issuers -A -o wide
  kubectl get clusterissuers -o wide
  
  # 检查 Issuer 详细状态
  kubectl describe issuer <issuer-name> -n <namespace>
  kubectl describe clusterissuer <issuer-name>
  
  # 检查 Issuer 的 conditions
  kubectl get clusterissuer <issuer-name> -o jsonpath='{range .status.conditions[*]}{.type}{"="}{.status}{" reason="}{.reason}{"\n"}{end}'
  ```
- **超时**: 10s
- **预期输出模式**: Issuer Ready 状态
- **判断规则**:
  - Ready=True → Issuer 配置正确，可以签发证书
  - Ready=False + reason=ErrInitIssuer → Issuer 初始化失败（RC-004）
  - Ready=False + reason=NotFound → 引用的 Secret 不存在
  - ACME Issuer 的 account 未注册 → 检查 ACME server 连通性
- **版本差异**: 无

**Step D4.4**: 检查 cert-manager controller 日志
- **命令**:
  ```bash
  # 获取 cert-manager controller 日志
  kubectl logs -n cert-manager deployment/cert-manager --tail=200 | grep -iE "error|fail|expire|rate.?limit"
  
  # 检查 cert-manager webhook 日志
  kubectl logs -n cert-manager deployment/cert-manager-webhook --tail=50
  
  # 检查 cert-manager cainjector 日志
  kubectl logs -n cert-manager deployment/cert-manager-cainjector --tail=50 | grep -i error
  ```
- **超时**: 15s
- **预期输出模式**: 日志条目
- **判断规则**:
  - `rate limited` 或 `too many requests` → Let's Encrypt 限流（RC-005）
  - `connection refused` 或 `timeout` → 无法连接 ACME server 或 Issuer 后端
  - `secret not found` → 缺少必要的 Secret（CA 密钥、ACME 账号等）
  - `failed to verify` → 证书链验证失败
- **版本差异**: 无

**Step D4.5**: 检查 ACME challenge 失败原因（HTTP-01/DNS-01）
- **命令**:
  ```bash
  # 获取所有 Order
  kubectl get orders -A
  
  # 检查 Order 详情
  kubectl describe order <order-name> -n <namespace>
  
  # 获取所有 Challenge
  kubectl get challenges -A
  
  # 检查 Challenge 详情
  kubectl describe challenge <challenge-name> -n <namespace>
  
  # HTTP-01: 检查 challenge solver Pod
  kubectl get pods -n cert-manager -l acme.cert-manager.io/http01-solver=true
  
  # DNS-01: 检查 DNS 记录是否创建
  dig +short TXT _acme-challenge.<domain>
  ```
- **超时**: 30s
- **预期输出模式**: Order/Challenge 状态
- **判断规则**:
  - Order state=invalid → Challenge 失败，检查 Challenge 详情
  - Challenge state=pending 且持续时间过长 → 验证未完成
  - HTTP-01 solver Pod 不存在或异常 → solver 创建失败
  - DNS TXT 记录不存在 → DNS provider 配置错误或权限不足
  - `Waiting for DNS record` 持续 → DNS 传播延迟或 DNS provider API 失败
- **版本差异**: 无

**Step D4.6**: 检查 Private CA / Vault Issuer 连接
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # Private CA Issuer: 检查 CA Secret
  kubectl get secret <ca-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates -subject
  
  # Vault Issuer: 检查 Vault 连通性
  kubectl exec -n cert-manager deployment/cert-manager -- wget -qO- --timeout=5 <vault-addr>/v1/sys/health 2>/dev/null || echo "Vault unreachable"
  
  # 检查 Vault token Secret
  kubectl get secret <vault-token-secret> -n cert-manager -o jsonpath='{.data.token}' | base64 -d | head -c 20 && echo "..."
  
  # 检查 Vault Issuer 的 status
  kubectl get clusterissuer <vault-issuer-name> -o jsonpath='{.status}' | jq .
  ```
- **超时**: 15s
- **预期输出模式**: CA 证书有效期、Vault 连通性
- **判断规则**:
  - CA Secret 不存在 → CA Issuer 配置错误（RC-004）
  - CA 证书已过期 → Private CA 过期（RC-006）
  - Vault 不可达 → 网络问题或 Vault 宕机
  - Vault token 无效 → token 过期或被撤销（RC-004）
- **版本差异**: 无

---

### Phase 5: mTLS 故障诊断

> **目标**: 诊断双向 TLS 认证失败的问题，包括 Service Mesh mTLS 和应用层 mTLS。
> **预计耗时**: 5-10 分钟
> **前置条件**: 应用使用 mTLS 进行通信

**Step D5.1**: 双向 TLS 握手失败分析
- **命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

  ```bash
  # 测试双向 TLS 连接
  openssl s_client -connect <host>:<port> \
    -cert /path/to/client.crt \
    -key /path/to/client.key \
    -CAfile /path/to/ca.crt \
    -verify_return_error 2>&1 | head -50
  
  # 从 Pod 内部测试 mTLS
  kubectl exec <pod-name> -n <namespace> -- \
    curl -v --cacert /path/to/ca.crt \
    --cert /path/to/client.crt \
    --key /path/to/client.key \
    https://<service>:<port>/healthz 2>&1 | grep -iE "ssl|tls|error|handshake"
  ```
- **超时**: 15s
- **预期输出模式**: TLS 握手详情
- **判断规则**:
  - `certificate has expired` → 客户端或服务端证书过期
  - `certificate verify failed` → CA 不受信任（RC-008 mTLS 场景）
  - `tlsv1 alert unknown ca` → 服务端不信任客户端 CA
  - `sslv3 alert handshake failure` → TLS 版本或密码套件不匹配
  - 握手成功 → mTLS 配置正确
- **版本差异**: 无

**Step D5.2**: CA bundle 一致性检查
- **命令**:
  ```bash
  # 检查客户端信任的 CA
  kubectl get configmap <client-ca-bundle> -n <namespace> -o jsonpath='{.data.ca\.crt}' | \
    openssl x509 -noout -subject -issuer -fingerprint
  
  # 检查服务端信任的 CA
  kubectl get secret <server-tls-secret> -n <namespace> -o jsonpath='{.data.ca\.crt}' | base64 -d | \
    openssl x509 -noout -subject -issuer -fingerprint
  
  # 比较两个 CA 的 fingerprint 是否一致
  CLIENT_CA_FP=$(kubectl get configmap <client-ca-bundle> -n <namespace> -o jsonpath='{.data.ca\.crt}' | openssl x509 -noout -fingerprint -sha256 2>/dev/null)
  SERVER_CA_FP=$(kubectl get secret <server-tls-secret> -n <namespace> -o jsonpath='{.data.ca\.crt}' | base64 -d | openssl x509 -noout -fingerprint -sha256 2>/dev/null)
  echo "Client CA: $CLIENT_CA_FP"
  echo "Server CA: $SERVER_CA_FP"
  [ "$CLIENT_CA_FP" = "$SERVER_CA_FP" ] && echo "CA Match: YES" || echo "CA Match: NO"
  ```
- **超时**: 15s
- **预期输出模式**: CA 指纹对比
- **判断规则**:
  - CA fingerprint 一致 → CA bundle 正确
  - CA fingerprint 不一致 → CA 不匹配（RC-012 mTLS 场景）
  - CA 无法解析 → CA 数据损坏或格式错误
- **版本差异**: 无

**Step D5.3**: 证书 SAN (Subject Alternative Name) 匹配验证
- **命令**:
  ```bash
  # 检查服务端证书的 SAN
  kubectl get secret <server-tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | \
    openssl x509 -noout -ext subjectAltName
  
  # 检查客户端证书的 SAN
  kubectl get secret <client-tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | \
    openssl x509 -noout -ext subjectAltName
  
  # 检查连接时使用的主机名是否在 SAN 中
  echo "Connecting to: <service>.<namespace>.svc.cluster.local"
  kubectl get secret <server-tls-secret> -n <namespace> -o jsonpath='{.data.tls\.crt}' | base64 -d | \
    openssl x509 -noout -ext subjectAltName | grep -i "<service>"
  ```
- **超时**: 10s
- **预期输出模式**: SAN 列表
- **判断规则**:
  - 连接主机名在 SAN 中 → SAN 匹配正确
  - 连接主机名不在 SAN 中 → SAN 不匹配（RC-012）
  - 无 SAN 扩展 → 证书缺少 SAN，可能导致 TLS 验证失败
  - SAN 包含 IP 但连接使用域名（或反之）→ 类型不匹配
- **版本差异**: 无

**Step D5.4**: TLS 版本协商检查
- **命令**:
  ```bash
  # 检查服务端支持的 TLS 版本
  echo | openssl s_client -connect <host>:<port> -servername <host> 2>/dev/null | grep "Protocol"
  
  # 强制使用 TLS 1.2 测试
  echo | openssl s_client -connect <host>:<port> -tls1_2 2>&1 | grep -E "Cipher|Protocol|error"
  
  # 强制使用 TLS 1.3 测试
  echo | openssl s_client -connect <host>:<port> -tls1_3 2>&1 | grep -E "Cipher|Protocol|error"
  
  # 列出服务端支持的密码套件
  nmap --script ssl-enum-ciphers -p <port> <host> 2>/dev/null | grep -A 20 "TLSv1"
  ```
- **超时**: 30s
- **预期输出模式**: TLS 版本和密码套件
- **判断规则**:
  - TLS 1.2 和 1.3 均支持 → 兼容性良好
  - 仅支持 TLS 1.3 但客户端仅支持 TLS 1.2 → 版本不兼容
  - `wrong version number` 错误 → TLS 版本不匹配
  - `no cipher` 错误 → 无共同支持的密码套件
- **版本差异**: 无

---

## 5. 根因分类

| 根因 ID | 描述 | 概率 | 诊断证据 | FTA 映射 |
|--------|------|------|---------|---------|
| RC-001 | **kubeadm 管理的控制面证书过期**（默认 1 年有效期）— apiserver serving cert、apiserver-kubelet-client cert、controller-manager / scheduler kubeconfig 中的证书过期，导致集群管理面不可用 | 高 | D1.1 kubeadm certs check-expiration 显示过期；D1.2 apiserver 证书 notAfter 已过 | certificate-fta: BE-cp-cert-expired |
| RC-002 | **kubelet 客户端证书自动轮换失败** — kubelet 的 RotateKubeletClientCertificate 机制未正常工作，CSR 未提交或未被批准，导致 kubelet 无法与 apiserver 通信 | 中 | D1.3 kubelet 证书过期；D2.2 rotateCertificates 配置异常或轮换日志有错误；D3.3 存在 Pending CSR | certificate-fta: BE-kubelet-cert-rotation-fail |
| RC-003 | **etcd 证书过期** — etcd server、peer 或 healthcheck-client 证书过期，导致 etcd 集群内部通信中断或 apiserver 无法连接 etcd | 高 | D2.3 etcd 证书 notAfter 已过；etcd 日志出现 TLS 错误；D3.1 etcd 连接测试失败 | certificate-fta: BE-etcd-cert-expired |
| RC-004 | **cert-manager Issuer / ClusterIssuer 配置错误** — Issuer 的 CA secret 不存在、ACME 账号配置错误、Vault token 过期等，导致 cert-manager 无法签发或续签证书 | 中 | D2.5 Issuer Ready=False；cert-manager 日志有配置错误信息 | certificate-fta: BE-issuer-misconfigured |
| RC-005 | **cert-manager ACME challenge 失败（Let's Encrypt）** — DNS-01 或 HTTP-01 challenge 无法完成，可能是 DNS 记录未创建、HTTP 端口不可达、或 Let's Encrypt 限流 | 中 | D2.5 Challenge 状态 failed/pending；Order 状态 invalid；cert-manager 日志有 ACME 错误或 rate limit 信息 | certificate-fta: BE-acme-challenge-fail |
| RC-006 | **CA 证书过期**（默认 10 年有效期，但确实会过期）— 根 CA 或中间 CA 过期，所有由该 CA 签发的下游证书均不可信 | 低 | D2.7 CA 证书 notAfter 已过；所有验证链均失败 | certificate-fta: BE-ca-expired |
| RC-007 | **NTP 时间不同步导致证书验证失败** — 节点时钟偏差超出证书有效期窗口，导致有效证书被系统判定为"未生效"或"已过期" | 中 | D2.6 时钟未同步或偏差 >5s；证书实际有效期覆盖当前 UTC 时间但本地时间不在有效期内 | certificate-fta: BE-ntp-skew |
| RC-008 | **Webhook caBundle 与实际 CA 不一致** — Webhook 的 caBundle 字段中的 CA 证书与 webhook service 实际使用的 TLS 证书的签发 CA 不匹配 | 中 | D2.8 caBundle 解码后的 CA 与 webhook TLS secret 中的 issuer 不一致；webhook 调用返回 x509 错误 | certificate-fta: BE-webhook-ca-mismatch |
| RC-009 | **手动创建的 TLS Secret 过期** — 运维人员手动创建的 TLS Secret 中的证书到达有效期，且无自动轮换机制 | 中 | D2.9 TLS secret 中证书 notAfter 已过；Issuer 信息不含 cert-manager | certificate-fta: BE-manual-cert-expired |
| RC-010 | **前端代理（front-proxy）证书过期** — API aggregation layer 使用的 front-proxy-client 证书过期，导致 metrics-server、自定义 API server 等聚合 API 不可用 | 低 | D2.4 front-proxy-client 证书 notAfter 已过；metrics-server 返回错误 | certificate-fta: BE-front-proxy-expired |
| RC-011 | **ServiceAccount token signing key 过期或不匹配** — SA token 签名密钥被替换但 apiserver 未加载新密钥，导致已签发的 SA token 验证失败 | 低 | Pod 中的 ServiceAccount token 认证失败；apiserver 日志出现 `token verification failed` | certificate-fta: BE-sa-key-mismatch |
| RC-012 | **证书 SAN 不匹配（IP/域名变更后）** — 集群节点 IP 或域名变更后，原有证书的 SAN (Subject Alternative Name) 中不包含新 IP/域名，导致 TLS 连接验证失败 | 低 | D2.1 SAN 不包含当前 IP/域名；TLS 握手返回 `x509: certificate is valid for ..., not ...` | certificate-fta: BE-san-mismatch |
| RC-013 | **cert-manager 自动轮转失败** — cert-manager 的 Certificate 资源未能在 renewBefore 时间内完成轮转。常见原因：Issuer 不可用、DNS-01 challenge 失败、Let's Encrypt Rate Limit、webhook validation 失败、Secret 权限不足等 | 中 | D4.1 Certificate Ready=False；D4.2 CertificateRequest 失败；D4.4 cert-manager 日志有 error；D4.5 Challenge/Order 处于 pending/invalid 状态 | certificate-fta: BE-certmanager-renewal-fail |
| RC-014 | **mTLS 配置不匹配** — 双向 TLS 认证场景下，客户端证书未签发、CA 不受信任、证书 SAN 不匹配等。表现为 TLS 握手失败，`certificate verify failed` 或 `unknown ca` 错误 | 中 | D5.1 mTLS 连接测试失败；D5.2 CA bundle 指纹不一致；D5.3 证书 SAN 未包含连接主机名；服务端日志 `peer did not return a certificate` | certificate-fta: BE-mtls-mismatch |
| RC-015 | **OCSP Stapling / CRL 检查失败** — 证书吸收状态检查（OCSP）失败或证书撤销列表（CRL）下载失败，导致 TLS 握手延迟或失败。OCSP responder 不可达、CRL 文件过大或过期是常见原因 | 低 | `openssl s_client -status` 显示 OCSP 无响应或错误；TLS 握手延迟 >5s；应用日志 `OCSP response verify failed` 或 `unable to get certificate CRL` | certificate-fta: BE-ocsp-crl-fail |

---

## 6. 修复操作

### 6.1 🟢 低风险（Agent 可建议自动执行）

#### REM-001: 删除 cert-manager Certificate 触发重新签发
- **适用根因**: RC-004, RC-005, RC-009（cert-manager 管理的证书）
- **前置检查**:
  ```bash
  # 确认 Certificate 资源存在且由 cert-manager 管理
  kubectl get certificate <cert-name> -n <namespace> -o yaml

  # 确认 Issuer/ClusterIssuer 当前健康
  kubectl get clusterissuer <issuer-name> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
  # 预期: True

  # 记录当前证书信息以便回滚参考
  kubectl get secret <tls-secret-name> -n <namespace> -o yaml > /tmp/cert-backup-<cert-name>.yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

  ```bash
  # 方法1（推荐）：删除关联的 Secret，cert-manager 将检测到 Secret 缺失并重新签发
  kubectl delete secret <tls-secret-name> -n <namespace>

  # 方法2：通过 cmctl（cert-manager CLI）触发续签
  cmctl renew <cert-name> -n <namespace>

  # 等待 cert-manager 重新签发
  kubectl get certificate <cert-name> -n <namespace> -w
  ```
- **后置验证**:
  ```bash
  # 确认 Certificate 状态恢复为 Ready
  kubectl get certificate <cert-name> -n <namespace>
  # 预期: READY=True

  # 确认新 Secret 已创建且证书有效
  kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | \
    base64 -d | openssl x509 -noout -dates
  # 预期: notAfter 为新的到期时间
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 如果新证书签发失败，从备份恢复旧 Secret
  kubectl apply -f /tmp/cert-backup-<cert-name>.yaml
  ```

#### REM-002: 更新 TLS Secret 中的证书
- **适用根因**: RC-009（手动管理的 TLS 证书）
- **前置检查**:
  ```bash
  # 确认已获取新的证书文件（tls.crt 和 tls.key）
  openssl x509 -in /path/to/new/tls.crt -noout -dates -subject
  # 预期: notAfter 为新的有效期

  # 验证新证书与私钥匹配
  diff <(openssl x509 -in /path/to/new/tls.crt -noout -modulus) \
       <(openssl rsa -in /path/to/new/tls.key -noout -modulus)
  # 预期: 无输出（modulus 一致）

  # 备份旧 Secret
  kubectl get secret <tls-secret-name> -n <namespace> -o yaml > /tmp/secret-backup-<tls-secret-name>.yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 使用新证书更新 Secret
  kubectl create secret tls <tls-secret-name> \
    --cert=/path/to/new/tls.crt \
    --key=/path/to/new/tls.key \
    -n <namespace> \
    --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  # 确认 Secret 已更新
  kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | \
    base64 -d | openssl x509 -noout -dates
  # 预期: 显示新证书的有效期

  # 如果是 Ingress TLS，测试 HTTPS 连接
  echo | openssl s_client -connect <host>:443 -servername <host> 2>/dev/null | \
    openssl x509 -noout -dates
  # 预期: 显示新证书的有效期（可能需要 Ingress Controller 自动 reload）
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/secret-backup-<tls-secret-name>.yaml
  ```

#### REM-003: 批准 Pending CSR
- **适用根因**: RC-002（kubelet CSR 未被批准）
- **前置检查**:
  ```bash
  # 确认存在 Pending CSR
  kubectl get csr | grep -i pending

  # 检查 CSR 的请求者和内容，确保是合法的 kubelet 请求
  kubectl describe csr <csr-name>
  # 确认 Requesting User 为 system:node:<node-name> 或 system:bootstrap:<token-id>
  # 确认 Subject 为 O=system:nodes, CN=system:node:<node-name>
  ```
- **执行命令**:
  ```bash
  # 批准 CSR
  kubectl certificate approve <csr-name>

  # 如果有多个合法的 Pending CSR（批量批准）
  kubectl get csr --no-headers | grep -i pending | awk '{print $1}' | xargs -I {} kubectl certificate approve {}
  ```
- **后置验证**:
  ```bash
  # 确认 CSR 已被批准
  kubectl get csr <csr-name>
  # 预期: CONDITION 显示 Approved,Issued

  # 确认 kubelet 已使用新证书
  ssh <node-ip> "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates"
  # 预期: 新的有效期

  # 确认节点恢复 Ready
  kubectl get node <node-name>
  # 预期: STATUS=Ready
  ```
- **回滚命令**:
  ```bash
  # CSR 批准为不可逆操作，但如果误批准了恶意 CSR，可以：
  # 1. 删除签发的证书
  # 2. 在 kubelet 节点上删除新证书文件
  # 3. 重新生成正确的证书
  ```

---

### 6.2 🟡 中风险（Agent 建议，人工审批后执行）

#### REM-004: 修复 cert-manager Issuer 配置
- **适用根因**: RC-004
- **影响说明**: 修改 Issuer/ClusterIssuer 配置后，cert-manager 将使用新配置重新尝试签发证书。如果新配置仍然错误，可能产生额外的失败事件。
- **审批提示**: "建议修复 cert-manager Issuer `<issuer-name>` 的配置错误。修改后 cert-manager 将自动重试签发失败的证书。是否批准？"
- **前置检查**:
  ```bash
  # 详细检查 Issuer 错误信息
  kubectl describe clusterissuer <issuer-name>
  # 或
  kubectl describe issuer <issuer-name> -n <namespace>

  # 备份当前 Issuer 配置
  kubectl get clusterissuer <issuer-name> -o yaml > /tmp/issuer-backup.yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

  ```bash
  # 根据具体错误类型修复。常见场景：
  # 场景 A: CA Secret 不存在 → 创建 CA Secret
  kubectl create secret tls <ca-secret-name> --cert=/path/to/ca.crt --key=/path/to/ca.key -n cert-manager
  # 场景 B: ACME 配置错误 → 修正 server/email
  kubectl edit clusterissuer <issuer-name>
  # 场景 C: Vault token 过期 → 更新 token Secret
  kubectl create secret generic vault-token --from-literal=token=<new-vault-token> -n cert-manager --dry-run=client -o yaml | kubectl apply -f -
  ```
- **后置验证**:
  ```bash
  # 等待 Issuer 状态恢复
  kubectl get clusterissuer <issuer-name> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
  # 预期: True

  # 检查之前失败的 Certificate 是否开始重新签发
  kubectl get certificates -A | grep False
  # 预期: 失败的 Certificate 状态逐步变为 True
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/issuer-backup.yaml
  ```

#### REM-005: 修复 NTP 时间同步
- **适用根因**: RC-007
- **影响说明**: 修复 NTP 同步后，节点时钟将跳变到正确时间。如果时钟偏差较大（>分钟级），可能导致正在运行的应用出现短暂异常（如 session 过期、token 失效）。
- **审批提示**: "节点 `<node-name>` 时钟偏差 `<drift>` 秒。建议重启 NTP 服务以修复时间同步。时钟校正可能导致运行中应用的短暂异常。是否批准？"
- **前置检查**:
  ```bash
  # 确认当前时间偏差
  ssh <node-ip> "date -u && chronyc tracking 2>/dev/null"
  ```
- **执行命令**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 重启 chronyd（或 ntpd / systemd-timesyncd）
  ssh <node-ip> "systemctl restart chronyd 2>/dev/null || systemctl restart ntpd 2>/dev/null || systemctl restart systemd-timesyncd 2>/dev/null"
  # 强制同步时间
  ssh <node-ip> "chronyc makestep 2>/dev/null || ntpdate -u pool.ntp.org 2>/dev/null"
  sleep 10
  ```
- **后置验证**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  ssh <node-ip> "timedatectl status"
  # 预期: System clock synchronized: yes
  # 如果是因时间偏差导致的证书验证失败，重启 kubelet 后验证节点恢复
  ssh <node-ip> "systemctl restart kubelet"
  kubectl get node <node-name>  # 预期: Ready
  ```
- **回滚命令**:
  ```bash
  # 时间同步修复不应回滚。如果 NTP 不准确，检查配置：
  ssh <node-ip> "cat /etc/chrony.conf 2>/dev/null || cat /etc/ntp.conf 2>/dev/null"
  ```

#### REM-006: 更新 Webhook caBundle
- **适用根因**: RC-008
- **影响说明**: 更新 Webhook caBundle 后，apiserver 将使用新的 CA 验证 webhook service 的 TLS 证书。如果新 caBundle 不正确，所有经过该 webhook 的 API 请求都将失败。
- **审批提示**: "建议更新 Webhook `<webhook-name>` 的 caBundle 以匹配当前 webhook 服务的 CA 证书。更新期间该 webhook 的验证/变更功能可能短暂中断。是否批准？"
- **前置检查**:
  ```bash
  # 获取 webhook service 当前使用的 CA 证书
  kubectl get secret <webhook-tls-secret> -n <webhook-namespace> -o jsonpath='{.data.ca\.crt}' | base64 -d | openssl x509 -noout -dates -subject

  # 备份当前 webhook 配置
  kubectl get validatingwebhookconfiguration <webhook-name> -o yaml > /tmp/webhook-backup.yaml
  # 或
  kubectl get mutatingwebhookconfiguration <webhook-name> -o yaml > /tmp/webhook-backup.yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 获取正确的 CA 证书并 base64 编码
  CA_BUNDLE=$(kubectl get secret <webhook-tls-secret> -n <webhook-namespace> -o jsonpath='{.data.ca\.crt}')

  # 更新 webhook 的 caBundle（以 ValidatingWebhookConfiguration 为例）
  kubectl patch validatingwebhookconfiguration <webhook-name> \
    --type='json' \
    -p="[{\"op\": \"replace\", \"path\": \"/webhooks/0/clientConfig/caBundle\", \"value\": \"${CA_BUNDLE}\"}]"

  # 如果使用 cert-manager 的 ca-injector，确保 annotation 正确
  kubectl annotate validatingwebhookconfiguration <webhook-name> \
    cert-manager.io/inject-ca-from=<namespace>/<certificate-name> --overwrite
  ```
- **后置验证**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 测试 webhook 是否正常工作（尝试创建/更新一个受该 webhook 管理的资源）
  kubectl create namespace test-webhook-verify --dry-run=server 2>&1
  # 预期: 无 webhook 错误

  # 确认 webhook caBundle 已更新
  kubectl get validatingwebhookconfiguration <webhook-name> -o jsonpath='{.webhooks[0].clientConfig.caBundle}' | base64 -d | openssl x509 -noout -dates
  # 预期: 显示正确的 CA 证书有效期
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  kubectl apply -f /tmp/webhook-backup.yaml
  ```

#### REM-012: cert-manager 证书手动续期
- **适用根因**: RC-004, RC-005, RC-013
- **影响说明**: 手动触发 cert-manager 证书续期。如果 Issuer 配置仍然错误，续期仍将失败。该操作会替换现有 TLS Secret，可能导致使用该证书的应用短暂中断（直到应用 reload 新证书）。
- **审批提示**: "建议手动触发 cert-manager Certificate `<namespace>/<cert-name>` 的证书续期。续期成功后 TLS Secret 将被更新，依赖该证书的应用可能需要重新加载。是否批准？"
- **前置检查**:
  ```bash
  # 确认 Issuer 当前状态正常
  kubectl get clusterissuer <issuer-name> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
  # 或
  kubectl get issuer <issuer-name> -n <namespace> -o jsonpath='{.status.conditions[?(@.type=="Ready")].status}'
  # 预期: True
  
  # 检查当前 Certificate 状态
  kubectl get certificate <cert-name> -n <namespace> -o yaml
  
  # 备份当前 Secret
  kubectl get secret <tls-secret-name> -n <namespace> -o yaml > /tmp/cert-backup-<cert-name>-$(date +%Y%m%d%H%M%S).yaml
  ```
- **执行命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

  ```bash
  # 方法 1（推荐）: 使用 cmctl CLI 触发续期
  cmctl renew <cert-name> -n <namespace>
  
  # 方法 2: 通过删除 Secret 触发重新签发
  kubectl delete secret <tls-secret-name> -n <namespace>
  # cert-manager 会检测到 Secret 缺失并重新签发
  
  # 方法 3: 通过添加 annotation 触发续期
  kubectl annotate certificate <cert-name> -n <namespace> \
    cert-manager.io/renew-time="$(date -u +%Y-%m-%dT%H:%M:%SZ)" --overwrite
  
  # 等待 cert-manager 完成签发
  kubectl get certificate <cert-name> -n <namespace> -w
  ```
- **后置验证**:
  ```bash
  # 确认 Certificate 状态恢复为 Ready
  kubectl get certificate <cert-name> -n <namespace>
  # 预期: READY=True
  
  # 确认新 Secret 已创建且证书有效
  kubectl get secret <tls-secret-name> -n <namespace> -o jsonpath='{.data.tls\.crt}' | \
    base64 -d | openssl x509 -noout -dates
  # 预期: notAfter 为新的到期时间
  
  # 检查 CertificateRequest 状态
  kubectl get certificaterequest -n <namespace> -l cert-manager.io/certificate-name=<cert-name> --sort-by=.metadata.creationTimestamp | tail -1
  # 预期: READY=True, APPROVED=True
  
  # 如果是 Ingress TLS，验证外部访问
  echo | openssl s_client -connect <host>:443 -servername <host> 2>/dev/null | openssl x509 -noout -dates
  # 预期: 显示新证书的有效期（可能需要等待 Ingress Controller reload）
  ```
- **回滚命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

  ```bash
  # 从备份恢复旧 Secret
  kubectl apply -f /tmp/cert-backup-<cert-name>-<timestamp>.yaml
  ```

---

### 6.3 🔴 高风险（Agent 仅提供指导，人工执行）

#### REM-007: 使用 kubeadm 续签所有控制面证书
- **适用根因**: RC-001, RC-003, RC-010
- **影响说明**: `kubeadm certs renew all` 将续签所有 kubeadm 管理的证书。**续签后必须重启所有控制平面组件**（apiserver、controller-manager、scheduler、etcd），否则组件仍使用内存中的旧证书。在重启期间，apiserver 将短暂不可用（几秒到几十秒，取决于 HA 配置）。
- **操作步骤**:
  1. **备份现有证书**:
     ```bash
     # SSH 到控制平面节点
     cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%Y%m%d%H%M%S)
     cp /etc/kubernetes/*.conf /etc/kubernetes/conf.bak.$(date +%Y%m%d%H%M%S)/
     ```
  2. **检查当前证书状态**:
     ```bash
     kubeadm certs check-expiration
     ```
  3. **续签所有证书**:
     ```bash
     kubeadm certs renew all
     ```
  4. **验证新证书已生成**:
     ```bash
     kubeadm certs check-expiration
     # 预期: 所有证书的 RESIDUAL TIME 为 364d（约 1 年）
     ```
  5. **重启控制平面组件**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

     ```bash
     # Static pod 方式：重启 kubelet 会重建所有 static pod
     systemctl restart kubelet
     # 或逐个移动 manifest 触发重建：
     # cd /etc/kubernetes/manifests && mv kube-apiserver.yaml /tmp/ && sleep 5 && mv /tmp/kube-apiserver.yaml .
     ```
  6. **更新管理员 kubeconfig**:
     ```bash
     cp /etc/kubernetes/admin.conf ~/.kube/config
     chown $(id -u):$(id -g) ~/.kube/config
     ```
  7. **多控制平面集群：在每个 control plane 节点重复步骤 1-6**
- **安全检查**:
  - 确认当前节点是否为 HA 集群中的一个 control plane 节点
  - 如果是 HA 集群，建议逐个节点操作，确保始终有可用的 control plane
  - 确认 etcd 集群在重启期间仍有 quorum（至少 N/2+1 成员可用）
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复备份的证书和配置
  cp -r /etc/kubernetes/pki.bak.<timestamp>/* /etc/kubernetes/pki/
  cp /etc/kubernetes/conf.bak.<timestamp>/* /etc/kubernetes/

  # 重启组件以使用恢复的证书
  systemctl restart kubelet
  ```

#### REM-008: 手动轮换 kubelet 证书
- **适用根因**: RC-002
- **影响说明**: 手动重建 kubelet 的客户端证书。需要删除旧证书文件并重启 kubelet 触发 bootstrap。在新证书签发前，节点将处于 NotReady 状态。
- **操作步骤**:
  1. **备份现有证书**:
     ```bash
     ssh <node-ip> "cp -r /var/lib/kubelet/pki /var/lib/kubelet/pki.bak.$(date +%Y%m%d%H%M%S)"
     ```
  2. **检查 bootstrap token 可用性**:
     ```bash
     # 在能够访问 apiserver 的节点上执行
     kubeadm token list
     # 如果无有效 token，创建新的
     kubeadm token create --ttl 1h
     ```
  3. **删除旧的 kubelet 客户端证书**:
     ```bash
     ssh <node-ip> "rm -f /var/lib/kubelet/pki/kubelet-client-current.pem"
     ssh <node-ip> "rm -f /var/lib/kubelet/pki/kubelet-client-*.pem"
     ```
  4. **确保 kubelet 配置了 bootstrap kubeconfig**:
     ```bash
     ssh <node-ip> "ls -la /etc/kubernetes/bootstrap-kubelet.conf"
     # 如果不存在，需要创建（使用步骤 2 中的 token）
     ```
  5. **重启 kubelet**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

     ```bash
     ssh <node-ip> "systemctl restart kubelet"
     ```
  6. **批准新的 CSR**:
     ```bash
     # 等待新的 CSR 出现（通常在 kubelet 重启后几秒内）
     kubectl get csr --watch

     # 确认 CSR 来源正确后批准
     kubectl certificate approve <new-csr-name>
     ```
  7. **验证节点恢复**:
     ```bash
     kubectl get node <node-name>
     # 预期: Ready
     ```
- **安全检查**:
  - 确认 CSR 的 requestor 是目标节点
  - 确认 CSR 的 Subject 中 O=system:nodes, CN=system:node:<expected-node-name>
  - 不要批准 Subject 中包含非预期节点名称的 CSR
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复备份的证书
  ssh <node-ip> "cp /var/lib/kubelet/pki.bak.<timestamp>/* /var/lib/kubelet/pki/"
  ssh <node-ip> "systemctl restart kubelet"
  ```

#### REM-009: 续签 etcd 证书
- **适用根因**: RC-003
- **影响说明**: 续签 etcd 证书后必须重启 etcd。在非 HA 集群中，重启 etcd 将导致 apiserver 短暂不可用。在 HA 集群中，需确保逐个节点操作以维持 quorum。**etcd 证书操作风险极高，错误操作可能导致数据丢失。**
- **操作步骤**:
  1. **备份 etcd 数据和证书**:
     ```bash
     # 证书备份
     cp -r /etc/kubernetes/pki/etcd /etc/kubernetes/pki/etcd.bak.$(date +%Y%m%d%H%M%S)

     # etcd 数据快照备份（极其重要！）
     ETCDCTL_API=3 etcdctl snapshot save /tmp/etcd-backup-$(date +%Y%m%d%H%M%S).db \
       --endpoints=https://127.0.0.1:2379 \
       --cacert=/etc/kubernetes/pki/etcd/ca.crt \
       --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
       --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

     # 验证备份
     ETCDCTL_API=3 etcdctl snapshot status /tmp/etcd-backup-*.db --write-out=table
     ```
  2. **使用 kubeadm 续签 etcd 证书**:
     ```bash
     kubeadm certs renew etcd-server
     kubeadm certs renew etcd-peer
     kubeadm certs renew etcd-healthcheck-client
     kubeadm certs renew apiserver-etcd-client
     ```
  3. **验证新证书**:
     ```bash
     kubeadm certs check-expiration | grep etcd
     ```
  4. **重启 etcd**:
     ```bash
     # Static pod 方式
     mv /etc/kubernetes/manifests/etcd.yaml /tmp/
     sleep 10
     mv /tmp/etcd.yaml /etc/kubernetes/manifests/

     # 等待 etcd 恢复
     sleep 30
     ```
  5. **重启 apiserver（使其使用新的 apiserver-etcd-client 证书）**:
     ```bash
     mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
     sleep 5
     mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
     ```
  6. **HA 集群：在每个 etcd 节点重复步骤 1-5，逐个操作**
- **安全检查**:
  - **必须在操作前完成 etcd 快照备份**
  - 确认 etcd 集群当前 quorum 状态
  - HA 集群中确认一次只操作一个 etcd 成员
  - 验证备份快照完整性
- **回滚方案**:

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `etcdctl snapshot restore`：用快照覆盖 etcd 数据目录，集群状态强制回退
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复证书
  cp -r /etc/kubernetes/pki/etcd.bak.<timestamp>/* /etc/kubernetes/pki/etcd/

  # 如果 etcd 无法恢复，使用快照恢复
  ETCDCTL_API=3 etcdctl snapshot restore /tmp/etcd-backup-<timestamp>.db \
    --data-dir=/var/lib/etcd-restore

  # 替换 etcd 数据目录
  mv /var/lib/etcd /var/lib/etcd.broken
  mv /var/lib/etcd-restore /var/lib/etcd

  # 重启 etcd
  systemctl restart kubelet
  ```

#### REM-013: mTLS CA 信任链修复
- **适用根因**: RC-014, RC-015
- **影响说明**: 更新 mTLS 场景下的 CA bundle。如果更新不当，可能导致所有依赖 mTLS 的服务间通信完全中断。更新后需要重启所有依赖该 CA 的服务 Pod。
- **操作步骤**:
  1. **备份所有相关证书和 Secret**:
     ```bash
     # 备份客户端 CA bundle
     kubectl get configmap <client-ca-bundle> -n <namespace> -o yaml > /tmp/client-ca-bundle-backup.yaml
     
     # 备份服务端 TLS Secret
     kubectl get secret <server-tls-secret> -n <namespace> -o yaml > /tmp/server-tls-backup.yaml
     
     # 如果涉及多个 namespace，批量备份
     for ns in <ns1> <ns2> <ns3>; do
       kubectl get secrets -n $ns -l app=<app-name> -o yaml > /tmp/${ns}-secrets-backup.yaml
     done
     ```
  2. **获取正确的 CA 证书**:
     ```bash
     # 从权威来源获取 CA 证书
     # 如果是 cert-manager 管理的 CA:
     kubectl get secret <ca-secret-name> -n cert-manager -o jsonpath='{.data.ca\.crt}' | base64 -d > /tmp/correct-ca.crt
     
     # 或者从 Issuer 配置获取:
     kubectl get clusterissuer <issuer-name> -o jsonpath='{.spec.ca.secretName}'
     
     # 验证 CA 证书有效
     openssl x509 -in /tmp/correct-ca.crt -noout -dates -subject
     ```
  3. **更新客户端 CA bundle ConfigMap**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源

     ```bash
     # 更新 ConfigMap
     kubectl create configmap <client-ca-bundle> -n <namespace> \
       --from-file=ca.crt=/tmp/correct-ca.crt \
       --dry-run=client -o yaml | kubectl apply -f -
     
     # 或使用 patch
     CA_B64=$(cat /tmp/correct-ca.crt | base64 -w0)
     kubectl patch configmap <client-ca-bundle> -n <namespace> \
       --type='json' -p="[{\"op\": \"replace\", \"path\": \"/data/ca.crt\", \"value\": \"$(cat /tmp/correct-ca.crt)\"}]"
     ```
  4. **更新服务端证书（如果需要）**:
     ```bash
     # 如果服务端证书未由新 CA 签发，需要重新签发
     # 对于 cert-manager 管理的证书:
     cmctl renew <server-cert-name> -n <namespace>
     
     # 等待签发完成
     kubectl get certificate <server-cert-name> -n <namespace> -w
     ```
  5. **重启依赖 Pod 以加载新证书**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

     ```bash
     # 重启客户端应用
     kubectl rollout restart deployment/<client-app> -n <client-namespace>
     
     # 重启服务端应用
     kubectl rollout restart deployment/<server-app> -n <server-namespace>
     
     # 等待滚动更新完成
     kubectl rollout status deployment/<client-app> -n <client-namespace> --timeout=300s
     kubectl rollout status deployment/<server-app> -n <server-namespace> --timeout=300s
     ```
  6. **验证 mTLS 连接**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

     ```bash
     # 从客户端 Pod 测试 mTLS 连接
     kubectl exec <client-pod> -n <client-namespace> -- \
       curl -v --cacert /path/to/ca.crt \
       --cert /path/to/client.crt \
       --key /path/to/client.key \
       https://<server-service>.<server-namespace>.svc.cluster.local:<port>/healthz
     # 预期: TLS 握手成功，HTTP 200
     ```
- **安全检查**:
  - 确认新 CA 证书有效且未过期
  - 确认所有现有服务端证书由新 CA 签发（或由受信任的 CA 链签发）
  - 在非生产环境验证 CA 更新流程
  - 确认滚动重启策略（maxUnavailable）不会导致服务完全不可用
- **回滚方案**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

  ```bash
  # 恢复旧 CA bundle
  kubectl apply -f /tmp/client-ca-bundle-backup.yaml
  
  # 恢复旧 TLS Secret
  kubectl apply -f /tmp/server-tls-backup.yaml
  
  # 重启 Pod 以加载旧证书
  kubectl rollout restart deployment/<client-app> -n <client-namespace>
  kubectl rollout restart deployment/<server-app> -n <server-namespace>
  ```

---

### 6.4 ⬤ 严重（需高级 SRE 审批）

#### REM-010: CA 证书轮换
- **适用根因**: RC-006
- **审批要求**: 需要高级 SRE + 安全团队 + 架构团队联合审批
- **影响说明**: CA 证书过期意味着**所有由该 CA 签发的证书都不再可信**。CA 轮换需要重新签发所有下游证书、更新所有组件的 CA bundle、并重启集群中的每一个组件。这是集群级别的破坏性操作，需要维护窗口。
- **数据备份**:
  ```bash
  # 完整备份 PKI 目录
  tar czf /tmp/k8s-pki-backup-$(date +%Y%m%d%H%M%S).tar.gz /etc/kubernetes/pki/

  # etcd 数据快照
  ETCDCTL_API=3 etcdctl snapshot save /tmp/etcd-full-backup-$(date +%Y%m%d%H%M%S).db \
    --endpoints=https://127.0.0.1:2379 \
    --cacert=/etc/kubernetes/pki/etcd/ca.crt \
    --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
    --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

  # 备份所有 kubeconfig 文件
  cp /etc/kubernetes/*.conf /tmp/kubeconfig-backup/
  ```
- **操作步骤**:
  1. **生成新的 CA 证书**:
     ```bash
     cp /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/ca.crt.old
     cp /etc/kubernetes/pki/ca.key /etc/kubernetes/pki/ca.key.old
     openssl req -x509 -new -nodes -key /etc/kubernetes/pki/ca.key \
       -sha256 -days 3650 -out /etc/kubernetes/pki/ca.crt -subj "/CN=kubernetes"
     ```
  2. **使用新 CA 重新签发所有下游证书**: `kubeadm certs renew all`
  3. **分发新 CA 到所有节点并重启 kubelet**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

     ```bash
     for node in $(kubectl get nodes -o jsonpath='{.items[*].status.addresses[?(@.type=="InternalIP")].address}'); do
       scp /etc/kubernetes/pki/ca.crt ${node}:/etc/kubernetes/pki/ca.crt
       ssh ${node} "systemctl restart kubelet"
     done
     ```
  4. **重启所有控制平面组件**（参见 REM-007 步骤 5）
  5. **更新所有 Webhook 的 caBundle**（参见 REM-006）
  6. **验证整个集群功能**
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复旧 CA 和证书
  tar xzf /tmp/k8s-pki-backup-<timestamp>.tar.gz -C /

  # 恢复 kubeconfig
  cp /tmp/kubeconfig-backup/* /etc/kubernetes/

  # 重启所有组件
  systemctl restart kubelet

  # 如果 etcd 受损，从快照恢复
  ```

#### REM-011: kubectl 不可用时的紧急证书恢复
- **适用根因**: RC-001（apiserver 证书过期导致 kubectl 完全不可用）
- **审批要求**: 需要高级 SRE 审批，必须有控制平面节点 SSH 权限
- **影响说明**: 最紧急的恢复场景 —— apiserver 证书过期时所有 kubectl 操作失败，必须通过 SSH 直接操作。
- **操作步骤**:
  1. **SSH 到控制平面节点**: `ssh <control-plane-node-ip>`
  2. **确认证书过期状态**: `kubeadm certs check-expiration`
  3. **备份现有证书**:
     ```bash
     cp -r /etc/kubernetes/pki /etc/kubernetes/pki.emergency.bak
     cp /etc/kubernetes/admin.conf /etc/kubernetes/admin.conf.emergency.bak
     ```
  4. **续签证书**: `kubeadm certs renew all`
  5. **重启 apiserver**:
     ```bash
     # Static Pod 方式
     crictl pods --name kube-apiserver -q | xargs -I {} crictl stopp {}
     # 或移动 manifest
     mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
     sleep 5
     mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
     ```
  6. **更新 admin kubeconfig**:
     ```bash
     cp /etc/kubernetes/admin.conf ~/.kube/config
     ```
  7. **验证 kubectl 恢复**:
     ```bash
     kubectl get nodes
     kubectl cluster-info
     ```
  8. **重启其余控制平面组件**:
     ```bash
     crictl pods --name kube-controller-manager -q | xargs -I {} crictl stopp {}
     crictl pods --name kube-scheduler -q | xargs -I {} crictl stopp {}
     sleep 30
     kubectl get pods -n kube-system  # 验证所有组件恢复
     ```
- **回滚方案**:

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

  ```bash
  # 恢复备份
  cp -r /etc/kubernetes/pki.emergency.bak/* /etc/kubernetes/pki/
  cp /etc/kubernetes/admin.conf.emergency.bak /etc/kubernetes/admin.conf
  systemctl restart kubelet
  ```

---

## 7. 验证确认

### 7.1 即时验证（修复后 1-2 分钟内）

> ⚠️ **🔴 灾难性操作** — 含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案
> - `kubectl delete namespace`：永久删除命名空间及全部资源，不可恢复
> - `kubectl apply/create/replace`：创建/变更集群资源

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# V1: 确认 kubeadm 管理的证书有效期已更新
kubeadm certs check-expiration
# 预期: 所有证书的 RESIDUAL TIME 为正值（约 364d）

# V2: 确认 kubectl 命令正常工作
kubectl get nodes
kubectl get pods -n kube-system
kubectl cluster-info
# 预期: 命令正常返回结果，无 x509 或 TLS 错误

# V3: 确认 kubelet 与 apiserver 通信正常
kubectl get nodes -o wide
# 预期: 所有节点状态为 Ready

# V4: 确认 etcd 集群健康
kubectl get pods -n kube-system -l component=etcd
# 预期: etcd Pod 状态为 Running
# 或在控制平面节点上：
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
# 预期: 127.0.0.1:2379 is healthy

# V5: 确认 cert-manager 证书状态恢复
kubectl get certificates -A
# 预期: 所有 Certificate 的 READY 列为 True

# V6: 确认 apiserver TLS 证书有效
echo | openssl s_client -connect <apiserver-host>:6443 2>/dev/null | openssl x509 -noout -dates
# 预期: notAfter 为续签后的新日期

# V7: 确认 Webhook 功能正常
kubectl create namespace test-webhook-verify --dry-run=server 2>&1
kubectl delete namespace test-webhook-verify 2>/dev/null  # ⚠️ 不可逆：永久删除命名空间及全部资源
# 预期: 无 webhook 错误
```
### 7.2 短期监控（5-30 分钟）

| 监控项 | 命令/指标 | 预期趋势 | 异常阈值 |
|-------|----------|---------|---------|
| apiserver 可用性 | `apiserver_request_total` 或 `kubectl get --raw /healthz` | 持续返回 ok | 任何 healthz 检查失败 |
| 客户端证书剩余有效期 | `apiserver_client_certificate_expiration_seconds` | 值大幅增加（新证书） | 值仍接近 0 或为负 |
| cert-manager Certificate Ready 状态 | `certmanager_certificate_ready_status` | 值为 1（Ready=True） | 值为 0（仍未 Ready） |
| cert-manager 证书到期时间戳 | `certmanager_certificate_expiration_timestamp_seconds` | 值为未来时间戳 | 值为过去时间戳 |
| etcd 集群健康 | `etcd_server_has_leader` | 持续为 1 | 值为 0（无 leader） |
| kubelet 证书轮换成功 | `kubelet_certificate_manager_client_expiration_renew_errors` | 无新增错误 | 计数器增加 |
| 节点 Ready 状态 | `kube_node_status_condition{condition="Ready",status="true"}` | 所有节点为 1 | 任何节点变为 0 |
| TLS 握手错误 | 组件日志中 `x509` 或 `TLS handshake` 错误 | 无新增 TLS 错误 | 出现新的 TLS 错误 |

### 7.3 解决确认标准

以下条件**全部满足**时，可确认问题已解决：

- [ ] `kubeadm certs check-expiration` 显示所有证书在有效期内（>7 天）
- [ ] `kubectl` 命令正常工作，无 x509 或 TLS 错误
- [ ] 所有节点状态为 Ready，kubelet 与 apiserver 通信正常
- [ ] etcd 集群健康，所有成员在线且有 leader
- [ ] cert-manager 管理的 Certificate 全部 Ready=True（如适用）
- [ ] Webhook 功能正常，无证书验证错误
- [ ] Ingress TLS 证书有效，外部 HTTPS 访问正常（如适用）
- [ ] 控制平面组件（apiserver, controller-manager, scheduler）日志中无 TLS 相关错误
- [ ] 根因已记录，预防措施已就位（如证书过期监控告警）

### 7.4 回归检测（24 小时内关注）

| 关注项 | 检查方法 | 频率 | 异常行动 |
|-------|---------|------|---------|
| 证书有效性持久化 | `kubeadm certs check-expiration`（SSH 到控制平面） | 续签后 1h、6h、24h | 如果证书有效期回退 → 检查是否有进程覆盖了证书文件 |
| 组件重启后证书加载 | 检查 apiserver/etcd/kubelet 日志中无 TLS 错误 | 持续 | 如果组件重启后出现 TLS 错误 → 证书文件可能未被正确替换 |
| cert-manager 续签机制 | `kubectl get certificates -A` 确认 Ready 状态持续 | 每 4 小时 | 如果 Certificate 再次变为 Not Ready → 检查 Issuer 配置和网络 |
| kubelet 证书自动轮换 | `kubelet_certificate_manager_client_expiration_renew_errors` 指标 | 持续 | 错误计数增加 → 检查 CSR 审批机制 |
| 时间同步稳定性 | `timedatectl status` 或 NTP 指标 | 每 6 小时 | 时间再次漂移 → 检查 NTP 服务配置和网络 |
| 证书到期监控告警 | Prometheus 告警规则是否正常触发 | 每日 | 确保告警可在证书到期前 30 天触发 |
| 外部 HTTPS 可访问性 | `curl -sI https://<domain>` 或外部监控探针 | 每小时 | HTTPS 访问异常 → 检查 Ingress TLS 证书 |

---

## 8. 升级协议

### 8.1 自动升级条件

| 条件 | 说明 | 触发时机 |
|------|------|---------|
| **kubectl 不可用** | apiserver 证书过期导致所有远程管理命令失败，需要 SSH 到控制平面节点操作 | T1 判断结果：kubectl 完全不可用 |
| **etcd 证书过期** | etcd 集群通信中断，存在数据不一致或丢失风险 | D2.3 确认 etcd 证书已过期 |
| **CA 证书过期** | 影响所有下游证书，需要集群范围的证书重新签发 | D2.7 确认 CA 证书已过期 |
| **诊断超时** | 诊断工作流执行超过 **15 分钟**未能确认根因 | Phase 2 结束后仍无明确根因 |
| **修复失败** | 同一修复操作执行 **2 次**仍未通过后置验证 | REM-xxx 执行后 V1-V7 验证失败 |
| **多组件证书同时过期** | 两个或以上不同类型的证书同时过期（如 apiserver + etcd） | D1.1 显示多类证书过期 |
| **未知根因** | 完成所有诊断步骤但无法匹配任何已知根因 | Phase 3 完成后无明确发现 |

### 8.2 升级消息模板

```
# 🟢 低风险：只读/信息收集，通常无副作用
【{severity}】证书过期与 TLS 问题 - {cluster_name}
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
- 问题概述: {certificate_type} 证书过期/TLS 问题，持续 {duration}
- 影响范围:
  - kubectl 可用性: {kubectl_available}
  - 受影响组件: {affected_components}
  - 受影响节点: {affected_nodes_count}/{total_nodes}
  - 受影响 Certificate 资源: {affected_cert_resources}
  - 外部服务影响: {external_impact}
- 过期证书清单:
  - {cert_1_name}: 过期时间 {cert_1_expiry}
  - {cert_2_name}: 过期时间 {cert_2_expiry}
- 已完成诊断:
  - Phase 1 快速检查: {phase1_summary}
  - Phase 2 深度检查: {phase2_summary}
  - Phase 3 主动探测: {phase3_summary}
- 初步发现:
  - 可能根因: {suspected_root_cause} ({root_cause_id})
  - 关键证据: {key_evidence}
- 已尝试修复:
  - {attempted_remediation} → 结果: {remediation_result}
- 需要: {action_needed}
- 工单编号: {ticket_id}
- Skill 版本: SKILL-SEC-001 v1.0
━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━
```
### 8.3 交接信息包

升级时，Agent 需准备以下完整信息供人工接手：

1. **完整诊断路径**: 按时间顺序列出已执行的每个诊断步骤及每步输出摘要
2. **证书状态快照**:
   ```bash
   kubeadm certs check-expiration > cert-status.txt
   for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
     echo -n "$cert: " && openssl x509 -in $cert -noout -enddate 2>/dev/null || echo "UNREADABLE"
   done > cert-expiry-all.txt
   kubectl get certificates -A -o wide > cert-manager-status.txt 2>/dev/null
   ```
3. **已排除的根因**: 列出已排除的根因及依据（例: "RC-007 已排除 — D2.6 显示时间同步正常，偏差 <0.1s"）
4. **可能的根因假设**: 基于证据提出的根因假设及置信度
5. **事件时间线**: 故障检测 → 诊断开始 → 关键发现 → 修复尝试 → 结果 → 升级决定

---

## 9. K8s 版本兼容矩阵

### 9.1 功能差异表

| 功能/行为 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| kubeadm 默认证书有效期 | 1 年（CA 10 年） | 1 年（CA 10 年） | 1 年（CA 10 年） | 1 年（CA 10 年） | 1 年（CA 10 年） |
| kubeadm `certs renew` 命令 | GA | GA | GA | GA | GA |
| kubeadm 升级时自动续签证书 | 是（`kubeadm upgrade apply` 时自动续签） | 是 | 是 | 是 | 是 |
| RotateKubeletClientCertificate | GA（默认启用） | GA | GA | GA | GA |
| RotateKubeletServerCertificate | beta（默认启用） | beta | GA | GA | GA |
| KMS v2 (Secret 加密) | beta | GA | GA | GA | GA |
| Structured Authorization Config | N/A | alpha | beta | beta | GA |
| certificates.k8s.io/v1 API | GA | GA | GA | GA | GA |
| CSR 自动批准 (csrapproving controller) | 默认启用 | 默认启用 | 默认启用 | 默认启用 | 默认启用 |
| ClusterTrustBundle | alpha | alpha | alpha | beta | beta |

### 9.2 诊断命令差异

| 命令/参数 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|----------|-------|-------|-------|-------|-------|
| `kubeadm certs check-expiration` | 支持，显示所有 kubeadm 管理的证书 | 同左 | 同左，增加 EXTERNALLY MANAGED 显示 | 同左 | 同左 |
| `kubeadm certs renew all` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `kubeadm certs renew <cert-name>` | 支持（可选择性续签单个证书） | 同左 | 同左 | 同左 | 同左 |
| `kubectl get csr` | certificates.k8s.io/v1 | 同左 | 同左 | 同左 | 同左 |
| `kubectl certificate approve <csr>` | 支持 | 支持 | 支持 | 支持 | 支持 |
| `cmctl` (cert-manager CLI) | 取决于 cert-manager 版本 | 同左 | 同左 | 同左 | 同左 |

### 9.3 关键 API 版本

| 资源 | v1.28 | v1.29 | v1.30 | v1.31 | v1.32 |
|------|-------|-------|-------|-------|-------|
| CertificateSigningRequest | certificates.k8s.io/v1 | v1 | v1 | v1 | v1 |
| Secret (TLS type) | v1 (core) | v1 | v1 | v1 | v1 |
| ValidatingWebhookConfiguration | admissionregistration.k8s.io/v1 | v1 | v1 | v1 | v1 |
| MutatingWebhookConfiguration | admissionregistration.k8s.io/v1 | v1 | v1 | v1 | v1 |
| ClusterTrustBundle | certificates.k8s.io/v1alpha1 | v1alpha1 | v1alpha1 | certificates.k8s.io/v1beta1 | v1beta1 |

### 9.4 版本相关的证书诊断注意事项

- **[v1.28+]**: kubelet 客户端证书自动轮换已 GA 并默认启用。如果 kubelet 证书过期，首先检查自动轮换机制为何失败（CSR 未提交、未批准、kube-controller-manager 的 csrsigning controller 异常），而非直接手动轮换。

- **[v1.29+]**: KMS v2 GA。如果集群启用了 KMS v2 加密，etcd 中的 Secret 数据是加密的。在恢复 etcd 证书或从备份恢复时，需确保 KMS provider 可用，否则可能无法解密 Secret。

- **[v1.30+]**: Structured Authorization Configuration (beta)。新的结构化授权配置可能影响 CSR 的自动批准策略。如果升级到 v1.30+ 后 kubelet CSR 不再自动批准，检查新的授权配置是否覆盖了旧的 RBAC 规则。

- **[v1.31+]**: RotateKubeletServerCertificate GA。kubelet server 证书轮换默认启用。如果之前依赖手动管理 kubelet server 证书，升级后行为可能改变。

- **[v1.32+]**: ClusterTrustBundle (beta)。新的 ClusterTrustBundle API 提供了集群范围的 CA 分发机制。如果使用此功能，CA 轮换时需同步更新 ClusterTrustBundle 资源。

### 9.5 cert-manager 版本兼容性说明

> **注意**: cert-manager 版本独立于 Kubernetes 版本。v1.13+ 支持 K8s v1.25+，v1.16+ 支持 K8s v1.28+。确保 cert-manager 版本与集群 K8s 版本兼容。

---

## 10. 知识进化

### 10.1 常见误诊模式

| 误诊场景 | 表面现象 | 实际根因 | 避免方法 |
|---------|---------|---------|---------|
| **时间偏移误判为证书过期** | 组件日志出现 `x509: certificate has expired or is not yet valid`，看似证书过期 | 节点 NTP 未同步，时钟偏移导致有效证书被判定为过期或未生效。证书实际 notAfter 仍在未来 | 在诊断证书过期时（D1.2/D1.3），同步执行 D2.6 检查时间同步。如果 `openssl x509 -dates` 显示证书有效期覆盖当前 UTC 时间，但组件报错，优先检查节点时间 |
| **CA bundle 不匹配误判为证书过期** | Webhook 或应用报 `x509: certificate signed by unknown authority`，误以为证书过期 | Webhook 的 caBundle 与实际 webhook 服务的 CA 不一致（如 cert-manager 轮换了 CA 但 caBundle 未更新） | 区分 `certificate has expired` 和 `signed by unknown authority` 两种错误。后者通常不是过期问题，而是 CA 信任链断裂。检查 D2.8 中 caBundle 与实际 CA 的一致性 |
| **kubeconfig 过期误判为 apiserver 证书过期** | `kubectl` 命令失败返回 x509 错误，误判 apiserver 证书过期 | 实际上 apiserver 证书有效，但用户 kubeconfig 中的客户端证书（client-certificate-data）过期 | 在 D1.2 中通过 `openssl s_client` 直接检查 apiserver serving cert。如果 serving cert 有效，检查用户 kubeconfig 中的客户端证书：`kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates` |
| **中间 CA 过期误判为叶证书过期** | Ingress TLS 验证失败，检查叶证书有效期发现仍有效 | 证书链中的中间 CA 过期，导致完整链验证失败，但叶证书本身未过期 | 在 D2.9 中使用 `openssl s_client -showcerts` 检查完整证书链，逐级验证每个证书的有效期 |
| **cert-manager 限流误判为配置错误** | cert-manager Certificate 持续 Ready=False，Issuer 看似正常 | Let's Encrypt 对该域名/账号触发了速率限制（rate limit），cert-manager 无法完成 ACME challenge | 在 D2.5 中检查 cert-manager 日志和 CertificateRequest Events 中是否包含 `rateLimited` 或 `too many certificates already issued`。等待限流窗口过期或使用 staging 环境 |
| **证书格式错误误判为过期** | TLS 握手失败，查看证书有效期发现未过期 | TLS Secret 中的证书格式错误（如 PEM 编码损坏、证书链顺序错误、包含多余空格/换行） | 使用 `openssl x509 -in <cert> -text` 验证证书可以被正确解析。检查 Secret 中 `tls.crt` 和 `tls.key` 的 base64 编码是否正确 |

### 10.2 深度知识引用

需要深入了解根因机制时，参考以下资源：

| 主题 | 引用路径 | 适用场景 |
|------|---------|---------|
| Kubernetes PKI 体系架构 | `domain-7-security-compliance/` | 理解 K8s 证书体系的完整设计、各证书的角色和关系 |
| kubeadm 证书管理机制 | `domain-7-security-compliance/` | 理解 kubeadm 如何生成、管理和续签证书 |
| 证书故障树分析 | `19-故障诊断/06-FTA故障树/list/certificate-fta.md` | 证书过期的完整因果链和概率模型 |
| 结构化故障排查方法论 | `19-故障诊断/04-高级排障/structural-` | 系统化证书排查的理论基础 |
| Kubernetes 故障排查总论 | `19-故障诊断/` | 跨组件的故障排查方法论 |
| 节点 NotReady 诊断 | `SKILL-NODE-001` (01-node-notready.md) | 当证书过期导致节点 NotReady 时的关联诊断 |
| 网络故障诊断 | `SKILL-NET-002` | 区分网络问题和 TLS 问题 |
| etcd 运维与恢复 | `19-故障诊断/` | etcd 证书恢复后的集群健康验证 |

### 10.3 预防措施与最佳实践

#### 证书过期监控告警配置

推荐在 Prometheus 中配置以下告警规则，实现证书过期的**提前预警**：

```yaml
- alert: KubernetesClientCertificateExpiringSoon
  expr: histogram_quantile(0.01, rate(apiserver_client_certificate_expiration_seconds_bucket[5m])) < 604800
  for: 5m
  labels: { severity: warning }
  annotations:
    summary: "Kubernetes client certificate expiring in less than 7 days"

- alert: CertManagerCertificateNotReady
  expr: certmanager_certificate_ready_status == 0
  for: 10m
  labels: { severity: warning }
  annotations:
    summary: "cert-manager Certificate {{ $labels.namespace }}/{{ $labels.name }} is not Ready"

- alert: CertManagerCertificateExpiringSoon
  expr: (certmanager_certificate_expiration_timestamp_seconds - time()) < 604800
  for: 5m
  labels: { severity: warning }
  annotations:
    summary: "cert-manager Certificate {{ $labels.namespace }}/{{ $labels.name }} expires in less than 7 days"

```

#### kubeadm 默认证书有效期

| 证书类型 | 默认有效期 | 续签方式 |
|---------|-----------|---------|
| CA 证书（ca.crt, etcd-ca.crt, front-proxy-ca.crt） | 10 年 | 手动重新生成 |
| 叶证书（apiserver.crt, etcd-server.crt 等） | 1 年 | `kubeadm certs renew` 或 `kubeadm upgrade` 时自动续签 |
| kubelet 客户端证书 | 1 年（自动轮换） | kubelet RotateKubeletClientCertificate（自动） |
| admin.conf / controller-manager.conf / scheduler.conf | 1 年 | `kubeadm certs renew` |

#### 预防性维护建议

1. **设置证书过期监控**: 至少在到期前 30 天触发告警
2. **定期执行 `kubeadm certs check-expiration`**: 建议每月检查一次
3. **保持 kubeadm 升级节奏**: `kubeadm upgrade apply` 会自动续签证书
4. **确保 kubelet 证书自动轮换开启**: 检查 `rotateCertificates: true`
5. **cert-manager 证书设置合理的 `renewBefore`**: 建议设置为有效期的 1/3
6. **备份 PKI 目录**: 定期备份 `/etc/kubernetes/pki/` 到安全位置
7. **NTP 时间同步**: 确保所有节点配置并运行 NTP 服务

### 10.4 Skill 改进记录

| 日期 | 版本 | 变更 | 原因 |
|------|------|------|------|
| 2026-03 | v1.0 | 初始版本发布。覆盖 K8s v1.28-v1.32，包含 12 个根因、11 个修复操作 | 首批 Skill 库建设，证书过期为高影响 P0 场景，由 SKILL-NODE-001 中的 RC-007 扩展为独立 Skill |

### 10.5 待补充的知识空白

1. **外部 CA 集成（Vault, AWS ACM, Google CAS）**: 外部 CA 管理的证书诊断差异
2. **Istio / Linkerd mTLS 证书**: Service Mesh sidecar 证书管理
3. **SPIFFE/SPIRE 集成**: 基于 SPIFFE 的工作负载身份证书管理
4. **多集群证书管理**: 跨集群的证书同步和轮换策略
5. **Air-gapped 环境**: 离线环境中无法使用 ACME 时的证书管理策略

## 修复动作

> **本章定位**: 基于 Section 6 修复操作的快速决策摘要，供 Agent 在 QA 语料和运行时直接引用。

### 修复动作速查表

| 根因 | 修复动作 | 风险 | 验证命令 |
|------|---------|------|---------|
| RC-004/RC-005 cert-manager 证书问题 | `kubectl delete secret <tls-secret> -n <ns>`（cert-manager 自动重新签发） | 🟢 低风险 | `kubectl get certificate <cert> -n <ns>` |
| RC-002 kubelet 证书过期 | 重启 kubelet 触发自动轮换: `ssh <node> "systemctl restart kubelet"` | 🟢 低风险 | `openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates` |
| RC-002 Pending CSR | `kubectl certificate approve <csr-name>` | 🟢 低风险 | `kubectl get csr <csr-name>` |
| RC-007 NTP 时间偏差 | `ssh <node> "systemctl restart chronyd || systemctl restart ntpd"` | 🟢 低风险 | `ssh <node> "timedatectl status"` |
| RC-009 手动 TLS Secret 过期 | 重新创建 Secret 或触发 cert-manager 续签 | 🟡 中风险（Ingress 需重新加载证书） | `kubectl get secret <tls-secret> -n <ns> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates` |
| RC-001/RC-003 kubeadm/etcd 证书过期 | `kubeadm certs renew <cert-name>`（需重启对应组件） | 🟡 中风险（组件重启期间短暂不可用） | `kubeadm certs check-expiration` |
| RC-010 front-proxy 过期 | `kubeadm certs renew front-proxy-client` + 重启受影响聚合组件 | 🟡 中风险（metrics-server 等聚合 API 短暂不可用） | `openssl verify -CAfile /etc/kubernetes/pki/front-proxy-ca.crt /etc/kubernetes/pki/front-proxy-client.crt` |

### danger_operations 高风险操作标注

```yaml
danger_operations:
  - operation: "kubeadm certs renew apiserver / etcd-server / ca"
    risk: "证书轮转后必须重启对应组件，控制平面在重启期间可能短暂不可用；etcd 证书操作不当可能导致集群失去 quorum"
    prerequisite:
      - "优先在非生产环境验证相同版本的 kubeadm 证书轮转流程"
      - "备份 /etc/kubernetes/pki/: cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%s)"
      - "etcd 证书: 确保逐个节点操作，确认集群健康后再操作下一节点"
    rollback: "从 /etc/kubernetes/pki.bak.* 恢复备份证书，重启对应组件"

  - operation: "kubectl delete secret <tls-secret>"
    risk: "删除 Secret 后 cert-manager 重新签发需要一定时间，期间使用该 Secret 的 Ingress/Webhook 可能出现 TLS 握手失败"
    prerequisite:
      - "确认 Issuer/ClusterIssuer 当前 Ready=True"
      - "记录 Secret 名称和关联的 Ingress/Webhook"
    mitigation: "在低流量时段操作，准备备用回滚 Secret（如适用）"

  - operation: "CA 证书过期后的全集群重新签发"
    risk: "CA 轮换是最危险的证书操作，所有依赖该 CA 的组件需要同步更新信任链，错误操作可能导致整个集群不可用"
    prerequisite:
      - "必须在维护窗口执行，通知所有相关团队"
      - "完整备份 /etc/kubernetes/pki/"
      - "准备从快照恢复集群的应急方案"
    escalation: "必须升级至高级 SRE / 架构师执行"
```

### 通用验证步骤

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 证书有效期检查
kubeadm certs check-expiration

# 2. apiserver 证书验证
echo | openssl s_client -connect <apiserver-host>:6443 2>/dev/null | openssl x509 -noout -dates

# 3. kubelet 证书验证
ssh <node> "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates"

# 4. cert-manager 证书状态
kubectl get certificates -A

# 5. Ingress TLS 验证
kubectl get secret <tls-secret> -n <ns> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -noout -dates -subject
```
## Related

- [[21-生态参考/03-领域索引/cert-index.md|Certificate / TLS 证书知识图谱索引]]

```

<!-- risk-assessed -->
