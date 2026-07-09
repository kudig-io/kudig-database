---
title: 安全与合规 生产就绪运维指南
description: 面向 SRE 的安全与合规域生产就绪检查、风险缓解与日常运维操作手册
summary: 安全与合规生产就绪检查清单、关键风险缓解、日常运维与故障排查速查
category: security
tags:
- production
- best-practices
- security
- operations
- compliance
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- 安全与合规 生产就绪运维指南是什么
- 如何按生产环境要求运维 安全与合规
trigger_keywords:
- 生产就绪
- 运维指南
- security
- compliance
- 安全合规
prerequisites:
- kubectl-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 安全与合规 生产就绪运维指南

> **目标读者**：SRE、平台工程师、安全运维。
> **核心目标**：在业务上线前，系统性地验证 Kubernetes 安全与合规域是否达到生产可用状态，并建立可重复的日常运维与故障排查节奏。

本指南聚焦 [[安全/README.md|安全]] 的五大子域：身份访问、网络安全、运行时安全、策略治理、供应链与合规审计。对于各子域的详细原理与配置示例，请参阅对应专题文档；本文仅保留生产就绪必须验证的关键项与可执行命令。

在生产环境中，安全与合规的失效往往不是单一配置错误，而是监控、流程、权限、供应链与响应机制的多重缺口叠加。因此本指南强调「可验证、可审计、可回滚」三个原则：每一项检查都附带验证命令，每一次高风险操作都要求备份与变更窗口，每一条告警都需要明确的响应路径。

---

## 1. 生产环境检查清单

在将安全与合规相关能力标记为生产就绪前，建议逐项确认以下检查点。

| 编号 | 检查项 | 验证命令 / 方法 | 通过标准 |
|---|---|---|---|
| 1 | API Server 审计日志已启用并持久化 | 检查 `--audit-log-path` 与 `--audit-policy-file` | 审计策略覆盖认证、RBAC、Secrets、Pod 创建事件 |
| 2 | Pod Security Admission 已按命名空间分级 | `kubectl get ns -o jsonpath='...pod-security...'` | 业务命名空间 enforce=baseline 或 restricted，系统命名空间已豁免 |
| 3 | 不存在过度授权的 RBAC 绑定 | `kubectl get clusterrolebindings -o json \| jq` | 无普通 ServiceAccount 绑定 cluster-admin |
| 4 | ServiceAccount Token 自动挂载已最小化 | `kubectl get sa -A -o yaml \| grep automountServiceAccountToken` | 非系统组件 SA 默认关闭自动挂载 |
| 5 | NetworkPolicy 已覆盖关键命名空间 | `kubectl get networkpolicy -A` | 生产命名空间存在默认拒绝或显式放行策略 |
| 6 | 镜像签名与准入校验已启用 | `kubectl get validatingwebhookconfiguration` | Kyverno / OPA Gatekeeper / 策略控制器在线且 enforce 镜像签名 |
| 7 | 运行时威胁检测组件在线 | `kubectl get pods -n falco` | Falco / Sysdig / 等价组件 DaemonSet 全节点运行且告警通道可用 |
| 8 | 证书生命周期可监控可轮换 | `kubeadm certs check-expiration` 或 cert-manager metrics | 集群组件证书剩余有效期 > 90 天，应用证书自动续期正常 |
| 9 | Secrets 加密与轮换机制就绪 | `kubectl get encryption-config` / KMS 状态 | etcd 加密开启，敏感 Secret 具备轮换 runbook |
| 10 | 安全事件响应流程与联系人已同步 | 检查 `07-incident-response/20-incident-response-process.md` | on-call 轮值、上报路径、取证工具包已更新 |
| 11 | 合规扫描与 CIS Benchmark 基线已落地 | `kubectl get cronjob -n compliance` | kube-bench / 云厂商合规检查定时运行，结果可回溯 |
| 12 | 特权容器与白名单镜像 registry 已约束 | 检查 PSA / Kyverno / Gatekeeper 策略 | 禁止 privileged、hostPath、root 用户等高危配置 |

> 建议将上述检查项纳入平台工程部门的 **生产上线门禁（Production Readiness Gate）**，由 SRE 与安全团队双签后方可放行。

### 1.1 自动化验证建议

检查清单不应仅停留在文档层面，建议在 CI/CD 或集群巡检平台中落地为可执行脚本：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# pre-production-security-gate.sh
# 返回值 0 表示通过，非 0 表示存在阻塞项

FAIL=0

# 检查是否有业务命名空间未设置 PSA enforce
UNLABELED=$(kubectl get ns -o json | jq -r '
  .items[] | select(.metadata.labels | has("pod-security.kubernetes.io/enforce") | not) |
  select(.metadata.name | startswith("kube-") | not) | .metadata.name')
if [ -n "$UNLABELED" ]; then
  echo "阻塞：以下命名空间未设置 PSA 标签: $UNLABELED"
  FAIL=1
fi

# 检查 cluster-admin 绑定中是否包含非系统 SA
OVERPRIV=$(kubectl get clusterrolebindings -o json | jq -r '
  .items[] | select(.roleRef.name=="cluster-admin") |
  select(.subjects[]? | select(.kind=="ServiceAccount" and .namespace != "kube-system")) |
  .metadata.name')
if [ -n "$OVERPRIV" ]; then
  echo "阻塞：发现非系统 ServiceAccount 绑定 cluster-admin: $OVERPRIV"
  FAIL=1
fi

# 检查证书是否将在 30 天内过期
kubeadm certs check-expiration 2>/dev/null | awk '/days/{if ($0 ~ /[0-9]+ days/){split($0,a," "); if (a[1] < 30) print "证书即将过期: " $0}}' && FAIL=1

exit $FAIL
```
将脚本接入发布流水线后，任何新增命名空间、RBAC 绑定或证书异常都会在上线前被拦截。

---

## 2. 关键风险与缓解措施

本章节选取五个对生产环境影响最大的安全风险。评估维度包括：**影响范围**（单应用、单命名空间、集群级、多集群）、**发现难度**（是否容易被监控覆盖）、**修复窗口**（是否需要计划停机）。对于每一项风险，我们都给出可直接复制执行的缓解命令或配置片段。

### 2.1 集群证书过期导致控制平面不可用

**风险**：kubeadm 默认证书有效期 1 年，若未监控或未及时轮换，API Server / etcd / kubelet 证书过期将触发集群级故障。

**缓解**：

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
# 每日监控：检查所有控制平面证书有效期
kubeadm certs check-expiration

# 距离过期 30 天内启动轮换（控制平面节点执行，需变更窗口）
BACKUP_DIR="/etc/kubernetes/pki.backup.$(date +%Y%m%d%H%M%S)"
cp -r /etc/kubernetes/pki "$BACKUP_DIR"
kubeadm certs renew all
systemctl restart kubelet
kubectl get nodes
```
更完整的轮换流程与回滚方案参见 [[安全/06-compliance/10-certificate-management.md|证书管理与 TLS 配置]]。

### 2.2 RBAC 过度授权与横向移动

**风险**：开发命名空间 ServiceAccount 被授予 cluster-admin、edit 包含 `pods/exec` 等权限，攻击者获取 Token 后可在全集群横向移动。

**缓解**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 扫描 cluster-admin 绑定
kubectl get clusterrolebindings -o json | jq -r '
  .items[] | select(.roleRef.name=="cluster-admin") |
  "\(.metadata.name): \([.subjects[]? | "\(.kind):\(.namespace // "N/A")/\(.name)"] | join(", "))"'

# 扫描可创建特权 Pod 的角色
kubectl get clusterroles -o json | jq -r '
  .items[] | select(.rules[]? |
    (.resources[]? == "pods" and (.verbs[]? == "create" or .verbs[]? == "*"))) |
  .metadata.name'
```
推荐采用最小权限角色模板，具体矩阵参考 [[安全/01-identity-access/07-rbac-matrix-configuration.md|RBAC 权限矩阵表]]。

### 2.3 容器镜像供应链污染

**风险**：未签名的镜像或基础镜像 CVE 被部署到生产，导致后门、挖矿或数据泄露。

**缓解**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Cosign 验证镜像签名
cosign verify --key cosign.pub <registry>/<image>:<tag>

# Helm 部署 Kyverno 镜像签名校验策略
helm upgrade --install kyverno kyverno/kyverno -n kyverno --create-namespace
kubectl apply -f policies/image-signature-verification.yaml
```
签名、SBOM 与 SLSA 实施细节参见 [[安全/05-supply-chain/01-supply-chain-security-overview.md|供应链安全概览]]。

### 2.4 运行时逃逸与异常行为未检测

**风险**：特权容器、可疑系统调用、敏感文件读取等运行时行为缺乏告警，攻击者已提权后才发现。

**缓解**：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认 Falco 规则加载与事件输出
kubectl logs -n falco -l app.kubernetes.io/name=falco | tail -n 50

# 检查节点 Seccomp / AppArmor / SELinux 状态
kubectl get nodes -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.nodeInfo.osImage}{"\t"}{.status.nodeInfo.kernelVersion}{"\n"}{end}'
```
规则调优与事件响应流程参考 [[安全/03-runtime-security/99-falco-runtime-security-guide.md|Falco 运行时安全指南]]。

### 2.5 网络平面默认互通导致东西向扩散

**风险**：默认允许所有 Pod 互通，单点失陷后攻击者可扫描内部服务、访问数据库或 Metadata 服务。

**缓解**：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 为命名空间设置默认拒绝
cat <<EOF | kubectl apply -f -
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: production
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
EOF

# ACK 等云厂商可叠加安全组 + CloudFirewall / Security Group 限制节点级流量
```
网络分段与零信任架构设计参考 [[安全/02-network-security/19-zero-trust-architecture.md|零信任架构]]。

---

## 3. 日常运维操作

### 3.1 每日安全巡检

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# daily-security-round.sh

set -euo pipefail

DATE=$(date +%Y-%m-%d)
REPORT="/var/log/kudig/security-round-${DATE}.log"

# 1. 证书有效期
kubeadm certs check-expiration >> "$REPORT" 2>&1 || true

# 2. 高危 RBAC 绑定
kubectl get clusterrolebindings -o json | jq -r '
  .items[] | select(.roleRef.name=="cluster-admin") |
  "CLUSTER-ADMIN: \(.metadata.name) -> \([.subjects[]? | "\(.kind):\(.namespace // "N/A")/\(.name)"] | join(", "))"' >> "$REPORT"

# 3. PSA 标签合规
kubectl get ns -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.metadata.labels}{"\n"}{end}' >> "$REPORT"

# 4. 异常事件
kubectl get events -A --field-selector type=Warning --sort-by='.lastTimestamp' | tail -n 50 >> "$REPORT"

# 5. 运行时告警
kubectl logs -n falco --since=24h -l app.kubernetes.io/name=falco | grep -i "priority=Critical\|priority=Error" >> "$REPORT" || true
```
巡检报告应至少保留 90 天，关键告警（如 cluster-admin 绑定新增、证书 30 天内过期、Critical 级运行时事件）需自动触发工单或 PagerDuty。

### 3.2 临时访问授权与复核

生产环境应尽量避免长期有效的管理员权限。对于紧急排障所需的 break-glass 访问，建议按以下节奏运维：

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
# 创建限时 ClusterRoleBinding（示例：授予 alice 1 小时 cluster-admin）
cat <<EOF | kubectl apply -f -
apiVersion: rbac.authorization.k8s.io/v1
kind: ClusterRoleBinding
metadata:
  name: break-glass-alice-$(date +%s)
  annotations:
    break-glass.expiry: "$(date -u -d '+1 hour' +%Y-%m-%dT%H:%M:%SZ)"
    break-glass.ticket: "INC-2026-0701-001"
subjects:
- kind: User
  name: alice
  apiGroup: rbac.authorization.k8s.io
roleRef:
  kind: ClusterRole
  name: cluster-admin
  apiGroup: rbac.authorization.k8s.io
EOF

# 每日复核即将过期或已超期的 break-glass 绑定
kubectl get clusterrolebindings -o json | jq -r '
  .items[] | select(.metadata.annotations["break-glass.expiry"] != null) |
  "\(.metadata.name) 过期时间: \(.metadata.annotations["break-glass.expiry"]) 工单: \(.metadata.annotations["break-glass.ticket"] // "N/A")"'

# 清理已过期绑定
kubectl get clusterrolebindings -o json | jq -r '
  .items[] | select(.metadata.annotations["break-glass.expiry"] != null) |
  select(.metadata.annotations["break-glass.expiry"] < now | strftime("%Y-%m-%dT%H:%M:%SZ")) |
  .metadata.name' | xargs -r kubectl delete clusterrolebinding
```
> 注：完整 JIT / PAM 生命周期方案规划为 `01-identity-access/12-production-iam-lifecycle.md`，待补充后与本指南联动使用。

### 3.3 Secret 与加密配置检查

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 etcd 加密配置已生效
kubectl get secrets -n production <secret-name> -o json | jq -r '.metadata.annotations["encryption.kubernetes.io"]'

# 列出所有明文 Secret（用于验证加密是否遗漏）
kubectl get secrets -A -o json | jq -r '
  .items[] | select(.type == "Opaque") |
  select(.metadata.annotations["encryption.kubernetes.io"] == null) |
  "\(.metadata.namespace)/\(.metadata.name)"' | head -n 20

# 手动轮换业务 Secret
kubectl create secret generic db-credentials \
  --from-literal=password="$(openssl rand -base64 32)" \
  -n production --dry-run=client -o yaml | kubectl apply -f -
```
### 3.3 策略引擎健康检查

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# Kyverno
kubectl get policyreport -A
kubectl get clusterpolicy
kubectl logs -n kyverno -l app.kubernetes.io/component=admission-controller --tail=100

# OPA Gatekeeper
kubectl get constraint
kubectl get constrainttemplate
kubectl logs -n gatekeeper-system -l control-plane=controller-manager --tail=100

# Pod Security Admission 审计日志
kubectl logs -n kube-system -l component=kube-apiserver | grep "pod-security" | tail -n 20
```
### 3.4 云上身份与访问临时授权

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# ACK RAM 临时 kubeconfig（示例）
aliyun cs GET /k8s/<cluster-id>/user_config \
  --TemporaryDurationMinutes 60 \
  --RoleArn acs:ram::<account-id>:role/<break-glass-role>

# EKS IRSA 关联检查
kubectl get sa -n production <app-sa> -o json | jq '.metadata.annotations["eks.amazonaws.com/role-arn"]'
```
---

## 4. 故障排查速查

以下速查表覆盖安全与合规域最常见的生产故障。排查时建议先确认症状影响范围（单 Pod、单命名空间、集群级），再按「确认命令 → 修复」顺序执行。所有涉及写操作的修复命令均应在变更窗口内执行，并提前备份相关配置。

| 现象 | 可能根因 | 确认命令 | 修复 / 缓解 |
|---|---|---|---|
| Pod 无法创建，事件提示 `exceeded pod-security` | PSA enforce 策略拦截 | `kubectl describe pod <name> -n <ns>` | 调整 Pod securityContext 或提升命名空间 PSA 豁免 |
| `kubectl auth can-i` 返回意外 `yes` | RBAC 过度授权 / 组绑定错误 | `kubectl get rolebindings,clusterrolebindings -A -o json \| jq` | 删除或替换为最小权限角色 |
| 证书告警 `KubernetesCertificateExpiration` | kubeadm / cert-manager 证书即将过期 | `kubeadm certs check-expiration` | 按变更窗口执行证书轮换 |
| Falco 无告警输出 | 规则未加载或输出通道阻塞 | `kubectl logs -n falco -l app.kubernetes.io/name=falco` | 检查 falco.yaml 输出配置、Sidekick / Falcosidekick 状态 |
| 镜像拉取失败 `signature verification failed` | Kyverno / Gatekeeper 策略拒绝未签名镜像 | `kubectl get policyreport -A` / `kubectl get events` | 确认镜像已 Cosign 签名，或临时放行白名单 registry |
| Pod 可访问 Metadata 服务 / 数据库 | NetworkPolicy 缺失或顺序错误 | `kubectl get networkpolicy -n <ns>` | 添加 default-deny + 显式白名单策略 |
| etcd 加密注解缺失 | EncryptionConfiguration 未覆盖该 Secret | `kubectl get secrets -A -o json \| jq` | 滚动重启 API Server 并重新写入 Secret |
| 大量 403 / 401 audit 事件 | OIDC / webhook 认证异常或 RBAC 策略变更 | `grep "responseStatus.code" /var/log/kubernetes/audit.log \| jq` | 检查 IDP、webhook、ClusterRoleBinding |
| Admission Webhook 超时导致所有 Pod 创建失败 | Kyverno / Gatekeeper / cert-manager webhook 异常 | `kubectl get validatingwebhookconfiguration -o yaml` | 临时设置 `failurePolicy=Ignore` 或扩容 webhook Pod |
| kube-bench 报告节点不符合 CIS | kubelet 参数、文件权限或内核参数偏离基线 | `kubectl logs <kube-bench-pod>` | 通过 Ansible / cloud-init 修复节点配置并重新扫描 |
| 业务 Pod 被 OOM / 调度失败且带有 seccomp 报错 | seccomp 配置阻止了必要系统调用 | `kubectl describe pod` / `dmesg` on node | 将 seccomp 从 Localhost 调整为 RuntimeDefault，或更新自定义 profile |

---

## 5. 与其他域的协作边界

安全与合规不是独立域，而是贯穿集群生命周期的横切关注点。任何单一安全控制都可能被其他域的变更绕过，因此需与以下域建立清晰的职责接口与变更通知机制：

- **[[网络/README.md|网络]]**：NetworkPolicy、服务网格 mTLS、Ingress TLS 由网络域负责配置与 SLO，安全域负责策略审查与审计。网络域提供流量可见性，安全域基于流量特征生成检测规则。
- **[[可观测性/README.md|可观测性]]**：审计日志、Falco 事件、策略告警需统一接入日志平台与告警路由。观测域负责采集、存储与告警通道，安全域负责规则语义、分级与响应。
- **[[发布变更/README.md|发布变更]]**：镜像签名、SBOM、GitOps Secret 管理（Sealed Secrets / External Secrets / SOPS）在发布域落地，安全域负责准入校验与合规审计。
- **[[生产运维/README.md|生产运维]]**：变更管理、on-call、事件响应由生产运维域统筹，安全域提供安全专项 runbook 与取证支持。
- **[[可靠性/README.md|可靠性]]**：安全组件（Vault、cert-manager、OPA、Kyverno、Falco）本身的高可用与灾备由可靠性域负责架构，安全域负责组件选型、策略备份与恢复验证。

---

## 6. 推荐阅读

### 本域核心文档

- [[安全/01-identity-access/07-rbac-matrix-configuration.md|RBAC 权限矩阵表]] — 最小权限角色设计参考
- [[安全/04-policy-governance/06-pod-security-standards.md|Pod 安全标准详解]] — PSA 分级与迁移
- [[安全/06-compliance/10-certificate-management.md|证书管理与 TLS 配置]] — 集群证书与 cert-manager 运维
- [[安全/02-network-security/19-zero-trust-architecture.md|零信任架构]] — 网络分段与身份驱动访问
- [[安全/03-runtime-security/99-falco-runtime-security-guide.md|Falco 运行时安全指南]] — 运行时威胁检测
- [[安全/05-supply-chain/01-supply-chain-security-overview.md|供应链安全概览]] — 镜像签名与 SBOM
- [[安全/07-incident-response/20-incident-response-process.md|安全事件响应流程]] — 事件响应与取证

### 相关域参考

- [[网络/README.md|网络]] — 网络安全策略与服务网格
- [[可观测性/README.md|可观测性]] — 安全审计日志与监控告警
- [[生产运维/README.md|生产运维]] — 生产运维与 on-call 体系

---

> **维护提示**：本指南应根据每次安全演练、证书轮换、策略引擎升级的结果持续更新。建议每季度由 SRE 与安全团队联合评审一次，并将修订记录写入 `_meta/journal/`。每次评审应至少覆盖清单有效性、告警误报率与响应 SLI 三项指标。


<!-- risk-assessed -->
