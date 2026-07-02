---
title: 生产安全运维手册
description: 面向 Kubernetes 生产环境的安全运维运行手册，覆盖 PSP 到 PSS 迁移、Secret 轮换、CIS 基线修复、漏洞响应、审计日志与事件隔离。
summary: 面向 Kubernetes 生产环境的安全运维手册，覆盖 PSP→PSS 迁移、Secret 轮换、CIS 加固、漏洞响应、审计日志与事件隔离。
category: production-operations
tags:
- production
- best-practices
- playbook
- security
- operations
- psp
- pss
- cis
- secret-rotation
- incident-response
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
- 安全工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 生产安全运维怎么做
- PSP 迁移到 PSS 的步骤
- Kubernetes Secret 轮换流程
- CIS 基线修复与漏洞响应
- Kubernetes 审计日志配置
- 容器安全事件隔离
trigger_keywords:
- 安全运维
- PSP
- PSS
- Pod Security Standards
- Secret rotation
- CIS
- 审计日志
- 事件隔离
- 漏洞响应
- 安全加固
prerequisites:
- kubectl-basics
- rbac-basics
- kubernetes-security-basics
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


# 生产安全运维手册

本手册面向 Kubernetes 生产环境的 SRE、平台工程师与安全工程师，提供 PSP 到 PSS 迁移、Secret 轮换、CIS 基线修复、漏洞响应、审计日志与事件隔离的标准操作流程。Kubernetes 的安全不是一次性配置，而是需要贯穿集群生命周期、工作负载部署、凭据管理与事件响应的持续过程。通过建立可重复、可审计、可验证的安全运维动作，团队可以在不影响业务敏捷性的前提下，显著降低容器逃逸、凭据泄露、横向移动等安全风险。本手册中的命令均可在配置好 `kubectl` 与相关工具的环境中直接执行，所有变更操作应遵循 [[domain-11-production-operations/02-change-management-guide.md|变更管理指南]] 并在非生产环境验证。安全变更尤其需要谨慎，因为错误的策略可能导致合法工作负载被拒绝、监控中断或服务可用性下降。

## 1. 适用场景与范围

本手册适用于以下场景：

- 需要将已弃用的 PodSecurityPolicy（PSP）迁移到 Pod Security Admission（PSS）。
- 需要建立 ServiceAccount token、镜像仓库凭据、TLS 证书等 Secret 的轮换机制。
- 需要执行 CIS Kubernetes Benchmark 修复与持续合规检查。
- 需要响应镜像漏洞、容器逃逸、凭据泄露等安全事件。
- 需要配置审计日志并基于审计数据进行溯源与告警。
- 需要将安全基线纳入日常巡检与 [[domain-11-production-operations/99-production-readiness-operations-guide.md|生产就绪运维框架]]。

## 2. 前置条件与工具

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 必需工具
kubectl version
helm version

# 推荐工具
# kube-bench: CIS 基线扫描
# kyverno / OPA Gatekeeper: 策略执行
# Falco / Tetragon: 运行时威胁检测
# audit2rbac: 审计日志分析
# cert-manager: 证书生命周期管理
# trivy / grype: 镜像漏洞扫描
```
在执行安全变更前，应确保具备以下权限：
- 集群管理员权限或能够修改 ClusterRole、ClusterPolicy、Namespace 标签。
- 对审计日志存储与 SIEM 的访问权限。
- 对镜像仓库、Secret 管理系统的管理权限。
- 已建立安全事件响应流程与升级路径。

## 3. 核心概念与架构

### 3.1 PSP → PSS 迁移

PodSecurityPolicy 曾是 Kubernetes 中用于限制 Pod 安全属性的主要机制，但由于其权限模型复杂、容易配置错误且与 RBAC 耦合紧密，社区在 Kubernetes 1.21 中将其标记为弃用，并在 1.25 中彻底移除。Pod Security Standards 与 Pod Security Admission 是其官方替代方案，内置于 kube-apiserver，无需额外安装准入控制器即可使用。

- **PSP（PodSecurityPolicy）**：已在 Kubernetes 1.21 弃用，1.25 移除。如果集群中仍有自定义 PSP，必须在升级前完成迁移，否则升级后相关策略将失效，可能导致安全风险。
- **PSS（Pod Security Standards）**：内置的三种策略级别，由 Kubernetes 社区维护：
  - **privileged**：无限制，仅用于系统级基础设施组件或已知需要高度特权的场景。
  - **baseline**：禁止已知危险配置（如 hostPath、hostNetwork、privileged），适合大多数生产业务命名空间。
  - **restricted**：最严格，要求非 root、只读根文件系统、禁止特权提升等，适合高安全要求场景或受监管行业。
- **PSA（Pod Security Admission）**：内置准入控制器，通过 Namespace 标签实施 PSS，无需额外安装第三方组件。PSA 支持 audit、warn、enforce 三种模式，可以平滑迁移而不影响现有业务。

### 3.2 Secret 轮换

Secret 轮换是降低凭据泄露影响的关键控制。即使 Secret 被意外泄露，如果轮换周期短、影响范围可控，攻击者能够利用的时间窗口也会大幅缩小。Secret 轮换应覆盖以下类型：
- **ServiceAccount token**：清理 legacy long-lived token，使用 Bound ServiceAccount Token（自动轮换，默认有效期 1 小时）。
- **镜像仓库凭据（imagePullSecrets）**：定期更换镜像仓库密码或 token，避免长期有效凭据被滥用。
- **TLS 证书**：使用 cert-manager 实现自动续期与轮换，避免证书过期导致服务中断。
- **etcd 加密密钥**：定期轮换 encryption config 中的密钥，降低历史数据泄露后被解密的风险。
- **数据库密码与应用凭据**：通过 External Secrets Operator 或 Vault 实现动态注入与自动轮换。

### 3.3 CIS 基线

CIS Kubernetes Benchmark 是业界公认的安全配置基线，覆盖控制面组件配置（API server、etcd、scheduler、controller-manager）、节点配置（kubelet、文件权限）、网络策略、RBAC、日志与审计。生产环境应至少每季度执行一次扫描，并对 FAIL 项制定修复计划。扫描结果应分类处理：高风险项（如匿名访问、不安全的端口绑定）应立即修复；中低风险项（如某些审计配置）可根据业务影响安排修复；无法修复的项应记录风险接受理由与责任人。

### 3.4 运行时威胁检测

Falco 与 Tetragon 可以检测容器内的异常行为，是 Kubernetes 运行时安全的重要补充。它们通过监控系统调用、内核事件或 eBPF 数据，识别偏离正常基线的行为。典型检测场景包括：
- 特权提升或容器逃逸尝试，例如挂载 `/proc`、`/sys` 或访问 Docker socket。
- 敏感目录挂载或文件访问，例如读取 `/etc/shadow` 或写入系统二进制目录。
- 反向 shell 或可疑网络连接，例如 Pod 内启动 bash 并建立出站连接。
- 未授权进程执行，例如在只允许静态二进制运行的环境中启动未预期进程。

运行时检测规则应经过调优，避免过多误报导致告警疲劳。建议将 Falco/Tetragon 告警接入 SIEM，并设置合理的严重级别与响应 SLA。初始部署时可以先启用规则子集，观察一周后再逐步扩大覆盖范围。

## 4. 标准操作流程

### 4.1 PSP 到 PSS 迁移

**Step 1：评估当前 PSP 使用情况**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get psp
kubectl get clusterrole -o yaml | grep -B5 -A5 podsecuritypolicies
```
记录每个 PSP 对应的命名空间与工作负载，确认哪些特权是业务必需的。对于必须保留的特权，应在迁移后通过 PSS 的 privileged 级别或自定义策略（Kyverno/OPA）显式授权。迁移前应建立完整的清单，包括 PSP 名称、绑定的 ServiceAccount、限制的 Pod 安全属性以及影响的命名空间。该清单是迁移后验证的重要依据。

**Step 2：在 Namespace 启用审计模式**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 先以 audit 模式观察影响，不阻止 Pod 创建
kubectl label --overwrite ns production \
  pod-security.kubernetes.io/audit=baseline \
  pod-security.kubernetes.io/audit-version=latest
```
观察审计日志中的 `violation` 事件，识别需要调整的工作负载。建议持续观察至少一个完整业务周期，确保覆盖所有发布、批处理任务与异常恢复流程。可以使用 `kubectl get events --field-selector reason=Violation` 快速筛选相关事件。

**Step 3：切换到 warn 模式**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl label --overwrite ns production \
  pod-security.kubernetes.io/warn=baseline \
  pod-security.kubernetes.io/warn-version=latest
```
warn 模式会在用户创建违反策略的 Pod 时返回警告，但不会阻止创建。此阶段用于推动开发团队修复清单，同时不影响现有业务运行。可以结合 CI/CD 准入检查，在构建阶段就阻止不符合 PSS 的清单进入仓库。

**Step 4：切换到 enforce 模式**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl label --overwrite ns production \
  pod-security.kubernetes.io/enforce=baseline \
  pod-security.kubernetes.io/enforce-version=latest \
  pod-security.kubernetes.io/warn=baseline \
  pod-security.kubernetes.io/audit=baseline
```
enforce 模式会阻止违反策略的 Pod 创建。切换前应确保所有工作负载已通过 warn 阶段验证，并准备回滚标签的预案。建议在低峰期执行 enforce 切换，并安排专人值守，以便在业务受到影响时快速回滚到 warn 模式。

**Step 5：删除旧 PSP 与绑定**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl delete psp <psp-name>
# 清理 ClusterRole/RoleBinding 中对 podsecuritypolicies 的引用
```
### 4.2 Secret 轮换

**ServiceAccount Token 轮换**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 legacy token
kubectl get secrets -A | grep kubernetes.io/service-account-token

# 删除长期有效的 Secret（应用需使用 projected ServiceAccount token）
kubectl delete secret <legacy-token-secret> -n <ns>

# 确认应用使用 Bound ServiceAccount Token
kubectl get pod <pod> -o jsonpath='{.spec.volumes[?(@.projected)]}'
```
Kubernetes 1.24+ 默认不再自动创建 long-lived ServiceAccount token。对于旧集群，应手动清理并改用 TokenRequest API。清理前必须确认所有依赖 legacy token 的工作负载已完成改造，否则会导致服务认证失败。建议先在一个非关键命名空间试点，验证 Bound Token 的兼容性后再推广到全集群。此外，应定期审计集群中的 ServiceAccount 权限，删除不再使用的 ServiceAccount 与 RoleBinding，遵循最小权限原则。

**镜像仓库凭据轮换**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建新凭据 Secret
kubectl create secret docker-registry regcred-new \
  --docker-server=<REGISTRY> \
  --docker-username=<USER> \
  --docker-password=<NEW_PASSWORD> \
  -n production

# 滚动更新 ServiceAccount 引用
kubectl patch serviceaccount default -n production -p '{"imagePullSecrets": [{"name":"regcred-new"}]}'

# 触发工作负载滚动更新
kubectl rollout restart deployment/<app> -n production

# 验证新 Pod 正常运行后删除旧 Secret
kubectl delete secret regcred-old -n production
```
镜像仓库凭据轮换应配合仓库侧的密码策略，例如设置较短的有效期、启用双因素认证、限制可访问的仓库与命名空间。对于使用云厂商容器镜像服务（如 ACR、GCR、ECR）的场景，可以结合 Workload Identity 或 IAM 角色，避免在 Kubernetes 中管理长期有效的 pull secret。

**TLS 证书自动轮换（cert-manager）**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 确认 cert-manager 正常
kubectl get pods -n cert-manager

# 检查证书有效期
kubectl get certificates -A
kubectl get certificaterequests -A

# 创建 ClusterIssuer（Let's Encrypt 示例）
cat <<EOF | kubectl apply -f -
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: letsencrypt-prod
spec:
  acme:
    server: https://acme-v02.api.letsencrypt.org/directory
    email: admin@example.com
    privateKeySecretRef:
      name: letsencrypt-prod
    solvers:
    - http01:
        ingress:
          class: nginx
EOF
```
cert-manager 可以自动管理 Let's Encrypt、私有 CA 与云厂商证书的生命周期，显著降低证书过期导致的服务中断风险。建议为所有入口证书配置 PrometheusRule 告警，提前 30 天通知续期。

### 4.3 CIS 基线扫描与修复

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 kube-bench
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job-master.yaml

# 查看扫描结果
kubectl logs job/kube-bench-master -n default

# 常见修复示例：限制 kubelet 匿名访问
# 编辑 /var/lib/kubelet/config.yaml
authentication:
  anonymous:
    enabled: false
  webhook:
    enabled: true
authorization:
  mode: Webhook
```
对于无法立即修复的项，应记录风险接受理由与计划修复时间。高风险项应优先处理，必要时申请临时例外并设置过期时间。CIS 扫描应自动化执行，并将结果持久化到安全运营平台，便于追踪修复进度与合规趋势。

### 4.4 漏洞响应

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 扫描镜像漏洞（Trivy 示例）
trivy image <REGISTRY>/<IMAGE>:<TAG>

# 阻止存在严重漏洞的镜像（Kyverno 策略示例）
cat <<EOF | kubectl apply -f -
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-image-signature
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-signature
    match:
      any:
      - resources:
          kinds:
          - Pod
    verifyImages:
    - imageReferences:
      - "ghcr.io/example/*"
      attestors:
      - entries:
        - keys:
            publicKeys: |
              -----BEGIN PUBLIC KEY-----
              ...
              -----END PUBLIC KEY-----
EOF
```
漏洞响应流程：
1. 通过镜像扫描识别受影响镜像与版本。建议将镜像扫描集成到 CI/CD 流水线，在构建阶段即发现漏洞。
2. 评估漏洞严重性与业务暴露面。对于 CVSS 评分高、存在公开 EXP 且面向互联网的漏洞，应优先处理。
3. 更新基础镜像或应用依赖，重新构建并推送。优先使用官方修复后的基础镜像版本。
4. 滚动更新生产工作负载。对于无法立即修复的漏洞，应评估临时缓解措施，如 WAF 规则、NetworkPolicy 限制或降级服务暴露面。
5. 验证修复后重新扫描，并记录漏洞处理时间线用于合规审计。

### 4.5 审计日志配置

```bash
# 应用审计策略
cat <<EOF | sudo tee /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
- level: Metadata
  resources:
  - group: ""
    resources: ["pods"]
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["roles", "rolebindings", "clusterroles", "clusterrolebindings"]
- level: Metadata
  omitStages:
  - RequestReceived
EOF
```

API server 启动参数（kubeadm 场景修改 `/etc/kubernetes/manifests/kube-apiserver.yaml`）：

```
--audit-policy-file=/etc/kubernetes/audit-policy.yaml
--audit-log-path=/var/log/kubernetes/audit.log
--audit-log-maxage=30
--audit-log-maxbackup=10
--audit-log-maxsize=100
```

审计日志应集中采集到 SIEM 或日志平台，并配置告警规则检测异常操作，如 RBAC 变更、特权 Pod 创建、Secret 大规模读取等。审计策略应平衡覆盖范围与存储成本：对敏感操作使用 RequestResponse 级别以保留完整请求体；对高频只读操作使用 Metadata 级别以减少日志量。建议将审计日志保留期设置为至少 90 天，以满足安全审计与合规要求。

### 4.6 运行时威胁检测

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 部署 Falco
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm install falco falcosecurity/falco -n security --create-namespace \
  --set driver.kind=modern_ebpf

# 查看 Falco 告警
kubectl logs -l app.kubernetes.io/name=falco -n security
```
### 4.7 事件隔离

当发现安全事件（如容器逃逸、恶意镜像）时：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 隔离受影响节点
cordon <NODE>
drain <NODE> --ignore-daemonsets --delete-emptydir-data

# 2. 隔离命名空间
kubectl label ns compromised-ns pod-security.kubernetes.io/enforce=restricted

# 3. 阻止恶意镜像（Kyverno/OPA 策略）
kubectl create configmap blocked-images \
  --from-literal=image=<MALICIOUS_IMAGE> -n gatekeeper-system

# 4. 保存证据
kubectl logs <pod> -n <ns> --previous > /evidence/<pod>-logs.txt
kubectl get pod <pod> -n <ns> -o yaml > /evidence/<pod>-spec.yaml
kubectl describe node <node> > /evidence/<node>-describe.txt
```
事件隔离后，应立即启动 [[domain-11-production-operations/04-incident-response-template.md|事故响应流程]]，并按 [[domain-11-production-operations/03-on-call-playbook.md|值班手册]] 进行升级。证据保存应包括容器镜像、运行时日志、网络连接记录与节点状态快照。在完成取证后，应彻底重建受感染节点，并轮换所有可能暴露的凭据，包括 ServiceAccount token、镜像仓库凭据、数据库密码与 TLS 证书。同时应进行事后复盘，更新检测规则与防护策略，防止类似事件再次发生。

## 5. 关键检查点与验证命令

| 检查项 | 命令 | 通过标准 |
|---|---|---|
| PSS 策略 | `kubectl get ns -L pod-security.kubernetes.io/enforce` | 生产命名空间非 privileged |
| PSP 残留 | `kubectl get psp` | 无自定义 PSP（1.25+ 已移除） |
| Secret 有效期 | `kubectl get certificates -A` | 所有证书剩余 > 30 天 |
| CIS 扫描 | `kubectl logs job/kube-bench-master` | 无 FAIL（或已记录修复计划） |
| 审计日志 | `ls /var/log/kubernetes/audit.log` | 日志正常写入，保留 ≥ 30 天 |
| 运行时告警 | `kubectl logs -l app=falco -n security` | 无未处理高危告警 |
| RBAC 权限 | `kubectl auth can-i --list -n production` | 无过度授权 |
| 镜像签名 | `kubectl get clusterpolicies` | 核心命名空间启用镜像签名验证 |

## 6. 常见故障与 remediation

| 现象 | 根因 | 处理命令/步骤 |
|---|---|---|
| Pod 被 PSS 拒绝 | 镜像要求 root、使用 hostPath 等 | `kubectl describe pod` 查看 PSA 事件；调整 SecurityContext 或申请例外 |
| Secret 轮换后应用报错 | 缓存旧凭据、未触发滚动更新 | 检查 Pod 是否使用新 Secret；必要时重启 workload |
| CIS 扫描发现 kubelet 匿名访问 | 配置未更新 | 修改 kubelet 配置并重启 kubelet |
| cert-manager 续期失败 | Challenge 失败、issuer 配置错误 | `kubectl describe certificaterequest`；检查 DNS/HTTP-01 challenge |
| Falco 告警频繁误报 | 规则过严 | 调整 Falco rules；建立白名单 |
| 审计日志过大 | 策略覆盖过宽 | 调整 audit policy level，排除只读健康检查 |
| 节点被入侵 | 容器逃逸、凭据泄露 | cordon/drain 节点；保存证据；重建节点并轮换相关凭据 |
| 镜像扫描发现高危漏洞 | 基础镜像或依赖存在 CVE | 更新基础镜像；重新构建并滚动更新 |

## 7. 风险与注意事项

1. **PSS enforce 可能导致业务中断**：先在 audit/warn 模式运行足够周期，确认无违规后再 enforce。
2. **Secret 轮换需考虑应用兼容性**：部分应用不会自动重载 Secret，需要设计滚动更新或热加载机制。
3. **CIS 修复可能影响可用性**：例如禁用匿名访问需确保所有监控探针已配置认证。
4. **审计日志量可能很大**：建议配置合理的保留策略，并将日志集中到 SIEM。
5. **事件隔离不等于删除证据**：在清理或重建前必须完成取证与日志归档。
6. **安全变更是高风险变更**：所有安全基线调整应通过变更管理流程，并在非生产环境验证。
7. **不要过度依赖默认配置**：Kubernetes 默认设置以满足通用性为主，生产环境需要根据业务场景进行加固。
8. **持续监控比一次性扫描更重要**：安全状态会随时间漂移，应建立持续监控与定期复核机制。建议每月召开一次安全运营 review 会议，回顾告警趋势、漏洞修复进度与策略例外情况。

## 8. 相关 Runbook / 推荐阅读

- [[domain-11-production-operations/99-production-readiness-operations-guide.md|生产运维域生产就绪运维指南]]
- [[domain-11-production-operations/02-change-management-guide.md|变更管理指南]]
- [[domain-11-production-operations/03-on-call-playbook.md|值班手册与告警响应规范]]
- [[domain-11-production-operations/04-incident-response-template.md|事故响应模板与流程规范]]
- [[domain-05-security-compliance/README.md|安全合规域]]
- [[domain-05-security-compliance/04-policy-governance/index.md|策略治理]]
- [[domain-13-container-runtime/03-containerd-cri-o/06-runtime-security-hardening.md|运行时安全加固]]
- [[domain-06-observability/README.md|可观测性域]]


<!-- risk-assessed -->
