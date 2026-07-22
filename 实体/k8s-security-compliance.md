---
title: 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
description: '# 安全合规'
summary: '# 安全合规'
category: reference
tags:
- k8s
- security
- rbac
- networkpolicy
- runtime-security
- zero-trust
- istio
- falco
- ebpf
- rag
tier: supporting
created: '2026-05-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全合规：RBAC、网络安全策略、运行时安全与零信任架构 是什么
- 如何 安全合规：RBAC、网络安全策略、运行时安全与零信任架构
trigger_keywords:
- 安全合规：RBAC
- 网络安全策略
- 运行时安全与零信任架构
prerequisites:
- kubectl-basics
- service-mesh-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# 安全合规

> **CNCF 状态**: 实践指南 | **类别**: Security & Compliance | **主要语言**: YAML, Go

## 概述

Kubernetes 安全合规实践是一个涵盖集群安全加固、合规审计、策略执行的综合性方法论体系。它整合了 CIS Benchmark、NIST SP 800-190、Pod Security Standards、OPA Gatekeeper 等多个安全框架和工具，为 K8s 生产环境提供从控制平面到工作负载的全栈安全保障。该体系涵盖身份认证、RBAC、网络策略、Pod 安全、密钥管理、审计日志、供应链安全等多个维度，帮助企业满足 SOC2、PCI-DSS、等保 2.0 等合规要求。

## Key Features（核心能力）

- **CIS Benchmark 合规**：对 K8s 控制平面和节点进行 CIS 安全基线扫描（kube-bench）
- **Pod Security Standards**：通过 PSA 强制执行 Privileged/Baseline/Restricted 三个安全级别
- **策略即代码**：使用 OPA Gatekeeper 或 Kyverno 定义和执行安全策略
- **运行时安全**：通过 Falco、Tracee 等工具检测异常行为和容器逃逸
- **供应链安全**：SBOM 生成、镜像签名验证、SLSA 合规
- **审计与合规报告**：自动生成合规报告，满足各类审计要求

## 架构与工作原理

安全合规体系分为四个层次：基础设施层（etcd 加密、API Server TLS、节点加固）、集群层（RBAC、NetworkPolicy、Admission Controller）、工作负载层（Pod Security、镜像扫描、运行时检测）、合规层（审计日志、合规报告、策略引擎）。通过纵深防御策略（Defense in Depth）在各层实施安全控制，形成多层防护网。

## K8s 集成

安全合规实践直接在 Kubernetes API 对象层面实施：RBAC（ClusterRole/RoleBinding）控制 API 访问；NetworkPolicy 限制 Pod 间通信；Pod Security Admission 替代旧版 PSP；ValidatingWebhook 在 API 准入阶段执行安全策略；Audit Policy 记录 API 访问日志。kube-bench、kube-hunter 等工具可自动扫描集群安全配置和漏洞。

## 生产用例

- **金融行业合规**：满足 PCI-DSS 等金融安全标准对容器化工作负载的要求
- **多租户隔离**：通过 RBAC + NetworkPolicy + Pod Security 实现租户间强隔离
- **供应链安全**：在 CI/CD 中强制镜像签名验证和安全扫描
- **安全事件响应**：通过审计日志和运行时检测快速定位和响应安全事件

## 安装与配置

### 安全工具链部署

```bash
# 🟢 kube-bench CIS Benchmark 扫描
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml
kubectl logs job/kube-bench -f

# 🟢 Kyverno 策略引擎
helm repo add kyverno https://kyverno.github.io/kyverno/
helm install kyverno kyverno/kyverno \
  -n kyverno --create-namespace \
  --set replicaCount=3 \
  --set admissionController.replicas=3

# 🟢 Falco 运行时安全检测
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm install falco falcosecurity/falco \
  -n falco --create-namespace \
  --set driver.kind=ebpf \
  --set falcosidekick.enabled=true

# 🟢 Trivy 镜像扫描
helm install trivy-operator aqua-security/trivy-operator \
  -n trivy-system --create-namespace
```

### Kyverno 安全策略示例

```yaml
# 禁止特权容器
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: disallow-privileged-containers
spec:
  validationFailureAction: Enforce
  background: true
  rules:
  - name: deny-privileged
    match:
      any:
      - resources:
          kinds: ["Pod"]
    validate:
      message: "特权容器被禁止。设置 securityContext.privileged=false"
      pattern:
        spec:
          containers:
          - securityContext:
              privileged: false
---
# 强制只读根文件系统
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: require-readonly-rootfs
spec:
  validationFailureAction: Enforce
  rules:
  - name: check-readonly-rootfs
    match:
      any:
      - resources:
          kinds: ["Pod"]
          namespaces: ["production", "staging"]
    validate:
      message: "容器必须设置 readOnlyRootFilesystem=true"
      pattern:
        spec:
          containers:
          - securityContext:
              readOnlyRootFilesystem: true
---
# 要求镜像签名验证
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  rules:
  - name: verify-cosign-signature
    match:
      any:
      - resources:
          kinds: ["Pod"]
    verifyImages:
    - imageReferences:
      - "registry.example.com/*"
      attestors:
      - count: 1
        entries:
        - keys:
            publicKeys: |-
              -----BEGIN PUBLIC KEY-----
              MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
              -----END PUBLIC KEY-----
```

### NetworkPolicy 默认拒绝

```yaml
# 默认拒绝所有入站流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes: [Ingress]
---
# 允许特定服务间通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-frontend-to-backend
  namespace: production
spec:
  podSelector:
    matchLabels:
      app: backend
  policyTypes: [Ingress]
  ingress:
  - from:
    - podSelector:
        matchLabels:
          app: frontend
    ports:
    - protocol: TCP
      port: 8080
```

### 审计策略配置

```yaml
# /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
# 记录所有对 Secret 的访问（不记录内容）
- level: Metadata
  resources:
  - group: ""
    resources: ["secrets", "configmaps"]
  verbs: ["get", "list", "watch"]
# 记录所有认证失败
- level: Metadata
  userGroups: ["system:unauthenticated"]
# 记录所有删除操作
- level: RequestResponse
  verbs: ["delete"]
  resources:
  - group: ""
    resources: ["pods", "services", "namespaces"]
# 记录 RBAC 变更
- level: RequestResponse
  resources:
  - group: "rbac.authorization.k8s.io"
    resources: ["clusterroles", "clusterrolebindings", "roles", "rolebindings"]
# 其他请求仅记录元数据
- level: Metadata
  omitStages: ["RequestReceived"]
```

## 运维操作

```bash
# 🟢 检查集群安全状态
kubectl get clusterpolicy -o custom-columns=NAME:.metadata.name,ACTION:.spec.validationFailureAction
kubectl get networkpolicy -A
kubectl get psp 2>/dev/null || echo "PSA mode"

# 🟢 检查命名空间安全级别
kubectl get ns -o custom-columns=NAME:.metadata.name,ENFORCE:.metadata.labels.'pod-security\.kubernetes\.io/enforce'

# 🟢 检查 RBAC 权限
kubectl auth can-i --list --as=system:serviceaccount:default:my-sa
kubectl get clusterrolebindings -o custom-columns=NAME:.metadata.name,ROLE:.roleRef.name,SUBJECT:.subjects[0].name

# 🟢 检查镜像漏洞报告
kubectl get vulnerabilityreports -A
kubectl get clustercompliancereports

# 🟢 Falco 告警查看
kubectl logs -n falco -l app.kubernetes.io/name=falco --tail=50 | grep -i "warning\|critical"

# 🟢 检查 Pod 安全上下文
kubectl get pods -n production -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.spec.securityContext}{"\n"}{end}'
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| Pod 被拒绝创建 | Kyverno/PSA 策略拦截 | `kubectl describe pod`; `kubectl get events` | 调整 Pod spec 符合策略 |
| 网络不通 | NetworkPolicy 过严 | `kubectl get netpol -n <ns>` | 添加允许的 ingress/egress 规则 |
| 权限不足 | RBAC 配置缺失 | `kubectl auth can-i <verb> <resource>` | 创建 Role/RoleBinding |
| Falco 误报 | 规则过于敏感 | 检查 Falco 日志 | 调整规则优先级/添加例外 |
| 审计日志缺失 | Audit Policy 未配置 | 检查 apiserver 参数 | 配置 --audit-policy-file |

### 安全事件响应流程

```
安全告警触发
├── 确认告警真实性（误报 vs 真实攻击）
│   ├── Falco 告警 → 检查对应 Pod/进程行为
│   └── 审计日志 → 确认操作来源和意图
├── 遏制影响
│   ├── 🔴 隔离受影响 Pod（NetworkPolicy deny-all）
│   ├── 🔴 撤销相关 ServiceAccount token
│   └── 🟡 暂停相关 Deployment 滚动
├── 根因分析
│   ├── 审计日志回溯操作时间线
│   ├── 检查镜像来源和签名
│   └── 检查 RBAC 权限链路
└── 修复与预防
    ├── 修复漏洞/更新策略
    ├── 加强监控告警
    └── 更新安全基线
```

## 生产案例

### 案例1：供应链攻击防御

- **场景**：CI/CD 被入侵，攻击者尝试部署包含后门的镜像
- **排查**：Kyverno verifyImages 策略拒绝未签名镜像部署；Falco 检测到异常 outbound 连接
- **方案**：强制 Cosign 签名验证 + SBOM 检查 + 镜像来源白名单
- **效果**：未签名镜像无法部署，攻击链被截断

### 案例2：多租户网络隔离

- **场景**：租户 A 的 Pod 能够访问租户 B 的 Service，存在数据泄露风险
- **排查**：命名空间未配置默认拒绝 NetworkPolicy；CNI 插件未启用 NetworkPolicy 支持
- **方案**：每个租户命名空间部署 default-deny + 显式允许规则；启用 Cilium NetworkPolicy
- **效果**：租户间完全网络隔离，满足合规要求

## 对比替代方案

| 方案 | 优势 | 劣势 | 适用场景 |
|------|------|------|----------|
| Kyverno | K8s原生、YAML策略、易上手 | 仅K8s资源 | K8s 策略执行 |
| OPA Gatekeeper | 通用策略引擎、Rego强大 | 学习曲线陡 | 复杂策略/多系统 |
| Falco | 运行时检测、eBPF | 仅检测不阻断 | 安全监控/审计 |
| KubeArmor | 运行时强制、阻断 | 较新、社区小 | 强隔离需求 |
| Istio mTLS | 服务间加密、零信任 | 资源开销大 | 服务网格安全 |

## 检查清单

- [ ] CIS Benchmark 扫描通过（kube-bench）
- [ ] PSA enforce 设置为 restricted（生产命名空间）
- [ ] 默认拒绝 NetworkPolicy 已部署
- [ ] Kyverno/Gatekeeper 策略引擎已部署
- [ ] 镜像签名验证已启用
- [ ] 运行时安全检测已部署（Falco/Tetragon）
- [ ] 审计日志已配置并发送到 SIEM
- [ ] Secret 加密存储已启用（EncryptionConfiguration）
- [ ] RBAC 最小权限原则已执行
- [ ] 安全事件响应流程已制定

## Related

- [[实体/tetragon.md|tetragon]] — Tetragon
- [[istio]] — Istio
- [[falco]] — Falco
- [[linkerd]] — Linkerd
- [[kubearmor]] — KubeArmor

- [[概念/Deployment × Secret 管理.md|Deployment × Secret 管理]]
- [[概念/CNI 插件 × NetworkPolicy.md|CNI 插件 × NetworkPolicy]]


<!-- risk-assessed -->
