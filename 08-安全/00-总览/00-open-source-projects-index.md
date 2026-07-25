---
title: Security & Compliance Open Source Projects Index
description: '# 安全合规开源项目索引'
summary: '# 安全合规开源项目索引'
category: reference
tags:
- security
- compliance
- open-source
- index
- opa
- falco
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Security & Compliance Open Source Projects Index 是什么
- 如何 Security & Compliance Open Source Projects Index
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- Security
- Compliance
- Open
- Source
- Projects
- Index
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- tls-basics
- policy-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 安全合规开源项目索引

> 本索引合并了原 `domain-7-security`、`domain-25-cloud-native-security`、`domain-39-supply-chain-security` 三个域的开源项目信息。

## 身份与访问

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| Vault | Secret 管理 | HashiCorp 企业级 Secret 管理 | `01-identity-access/05-vault-enterprise-secrets-management.md` |
| cert-manager | 证书管理 | K8s 自动 TLS 证书 | `06-compliance/99-cert-manager-tls-guide.md` |

## 运行时安全

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| [[Falco|Falco]] | 威胁检测 | 云原生运行时安全 | `03-runtime-security/01-falco-cloud-native-security.md` |
| Sysdig | 安全监控 | 容器与系统监控 | `03-runtime-security/02-sysdig-enterprise-container-security.md` |
| Aqua | 容器安全 | 企业级容器安全平台 | `03-runtime-security/03-aqua-enterprise-container-security.md` |
| gVisor | 容器沙箱 | 用户空间内核沙箱 | `03-runtime-security/17-gvisor-container-sandbox.md` |

## 策略治理

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| OPA / Gatekeeper | 策略引擎 | 通用策略执行 | `04-policy-governance/09-opa-gatekeeper-policy.md` |
| Kyverno | K8s 策略 | 原生 K8s 策略管理 | `04-policy-governance/04-kyverno-enterprise-policy-management.md` |

## 供应链安全

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| Sigstore | 签名验证 | 开源软件签名生态 | `05-supply-chain/07-sigstore-cosign-signing.md` |
| Cosign | 容器签名 | 容器镜像签名工具 | `05-supply-chain/07-sigstore-cosign-signing.md` |
| SLSA | 标准 | 软件供应链安全级别 | `05-supply-chain/05-slsa-levels-implementation.md` |
| Syft / Grype | SBOM/扫描 | 生成 SBOM 并扫描漏洞 | `05-supply-chain/03-sbom-generation-management.md` |

## 原始索引保留

更详细的索引见：
- `98-merged-indexes/00-open-source-projects-index-from-domain-7.md`
- `98-merged-indexes/00-open-source-projects-index-from-domain-25.md`
- `98-merged-indexes/00-open-source-projects-index-from-domain-39.md`

## 网络安全

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| Cilium | CNI/网络策略 | eBPF 驱动的网络与安全 | `02-network-security/01-cilium-network-policy.md` |
| Calico | CNI/网络策略 | 企业级网络策略 | `02-network-security/02-calico-network-policy.md` |
| Istio | Service Mesh | mTLS 与流量控制 | `02-network-security/03-istio-mtls.md` |

## 合规审计

| 项目 | 类型 | 说明 | 文档位置 |
|------|------|------|----------|
| kube-bench | 基线检查 | CIS Kubernetes Benchmark | `06-compliance/01-kube-bench-cis.md` |
| kubeaudit | 安全审计 | K8s 资源安全审计 | `06-compliance/02-kubeaudit.md` |
| Trivy | 漏洞扫描 | 镜像/配置扫描 | `05-supply-chain/01-trivy-scanning.md` |

## 工具链推荐

### 小型团队（< 50 人）

```
推荐组合:
- Sealed Secrets (密钥管理)
- Trivy (镜像扫描)
- kube-bench (基线检查)
- Calico (网络策略)
```

### 中型团队（50-500 人）

```
推荐组合:
- External Secrets Operator + 云 KMS
- Trivy + Grype (多层扫描)
- OPA/Gatekeeper (策略引擎)
- Falco (运行时检测)
- Cilium (网络策略)
```

### 大型企业（> 500 人）

```
推荐组合:
- HashiCorp Vault (密钥管理)
- Sigstore/Cosign (供应链签名)
- OPA/Gatekeeper + Kyverno (策略)
- Falco + Sysdig (运行时安全)
- Istio (零信任网络)
- kube-bench + kubeaudit (合规)
```

## 快速开始

### 安装 Trivy

```bash
# 安装
brew install aquasecurity/trivy/trivy

# 扫描镜像
trivy image myapp:latest

# 扫描 K8s 配置
trivy config ./k8s/

# 扫描 Secret
trivy fs --scanners secret .
```

### 安装 kube-bench

```bash
# 在节点上运行
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml

# 查看结果
kubectl logs job/kube-bench
```

### 安装 OPA/Gatekeeper

```bash
# 安装 Gatekeeper
helm repo add gatekeeper https://open-policy-agent.github.io/gatekeeper/charts
helm install gatekeeper gatekeeper/gatekeeper -n gatekeeper-system --create-namespace

# 验证
kubectl get pods -n gatekeeper-system
```

## 学习路径

```
入门:
1. 理解 K8s 安全模型 (RBAC, PSA, NetworkPolicy)
2. 启用 etcd 加密
3. 部署 Trivy 扫描镜像

进阶:
4. 部署 OPA/Gatekeeper 策略引擎
5. 配置 Falco 运行时检测
6. 实施供应链安全 (Sigstore)

专家:
7. 部署 Vault 企业级密钥管理
8. 实施零信任架构 (Istio mTLS)
9. 建立安全合规体系 (CIS, SOC2)
```

## 项目成熟度评估

| 项目 | CNCF 状态 | 生产就绪 | 社区活跃度 |
|------|-----------|----------|------------|
| Vault | 非 CNCF | ✅ | 高 |
| OPA/Gatekeeper | 毕业 | ✅ | 高 |
| Falco | 毕业 | ✅ | 高 |
| Cilium | 毕业 | ✅ | 高 |
| Trivy | 非 CNCF | ✅ | 高 |
| Sigstore | 毕业 | ✅ | 中 |
| Kyverno | 孵化 | ✅ | 高 |
| Sealed Secrets | 非 CNCF | ✅ | 中 |
| kube-bench | 非 CNCF | ✅ | 中 |

## 常见问题

### Q: 应该选择 OPA 还是 Kyverno？

A: 两者都是优秀的策略引擎，选择取决于场景：

| 维度 | OPA/Gatekeeper | Kyverno |
|------|----------------|----------|
| 策略语言 | Rego (学习曲线陡) | YAML (K8s 原生) |
| 适用场景 | 复杂策略、多系统 | K8s 专用策略 |
| 变异能力 | 有限 | 强 |
| 社区 | 更大 | 快速增长 |

**建议**: 如果团队熟悉 YAML 且主要管理 K8s，选 Kyverno；如果需要跨系统策略或已有 Rego 经验，选 OPA。

### Q: 如何开始供应链安全？

A: 分阶段实施：

```
第一阶段: 基础扫描
- 部署 Trivy 扫描镜像漏洞
- CI 中集成镜像扫描
- 设置漏洞门禁

第二阶段: 签名验证
- 使用 Cosign 签名镜像
- 配置 Admission Controller 验证签名
- 仅允许签名镜像部署

第三阶段: SBOM
- 生成 SBOM (Syft)
- 存储 SBOM 到镜像仓库
- 定期扫描 SBOM 中的漏洞
```

### Q: 运行时安全检测如何落地？

A: Falco 部署建议：

```bash
# 安装 Falco
helm repo add falcosecurity https://falcosecurity.github.io/charts
helm install falco falcosecurity/falco -n falco --create-namespace

# 查看告警
kubectl logs -n falco -l app.kubernetes.io/name=falco

# 自定义规则
kubectl create configmap falco-custom-rules \
  --from-file=custom-rules.yaml \
  -n falco
```

## 相关资源

- [CNCF 云原生安全白皮书](https://github.com/cncf/tag-security)
- [CIS Kubernetes Benchmark](https://www.cisecurity.org/benchmark/kubernetes)
- [NIST 容器安全指南](https://csrc.nist.gov/publications/detail/sp/800-190/final)
- [OWASP Kubernetes Security](https://owasp.org/www-project-kubernetes-security/)

## 安全架构参考

### 零信任架构

```
┌─────────────────────────────────────────────────────────┐
│                    零信任安全架构                       │
├─────────────────────────────────────────────────────────┤
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐    │
│  │  身份认证   │  │  授权策略   │  │  审计日志   │    │
│  │  (OIDC)     │  │  (OPA)      │  │  (Falco)    │    │
│  └──────┬──────┘  └──────┬──────┘  └──────┬──────┘    │
│         │                │                │             │
│  ┌──────▼──────────────▼──────────────▼──────┐    │
│  │              mTLS 加密通信 (Istio)            │    │
│  └────────────────────┬────────────────────┘    │
│                       │                              │
│  ┌────────────────────▼────────────────────┐    │
│  │           网络微分段 (Cilium/Calico)        │    │
│  └─────────────────────────────────────────┘    │
└─────────────────────────────────────────────────────────┘
```

### 供应链安全架构

```
代码提交 → CI 构建 → 镜像签名 → 扫描验证 → 部署
    │          │          │          │          │
    ▼          ▼          ▼          ▼          ▼
  SAST      SBOM      Cosign     Trivy    Admission
  扫描      生成      签名       扫描     Controller
                                         验证签名
```

## 安全度量指标

| 指标 | 目标 | 采集方式 |
|------|------|----------|
| 镜像漏洞修复时间 | < 7 天 | Trivy 扫描报告 |
| 策略合规率 | > 95% | OPA/Kyverno 审计 |
| Secret 轮换周期 | < 90 天 | Vault/ESO 日志 |
| 运行时告警响应 | < 15 分钟 | Falco 告警 |
| 供应链签名覆盖率 | 100% | Cosign 验证 |

## 相关域索引

- [[09-可观测性/README.md|可观测性域]] — 安全监控与审计日志
- [[05-网络/README.md|网络域]] — 网络策略与加密
- [[10-平台工程/README.md|平台工程域]] — 安全工具链集成
- [[12-可靠性/README.md|可靠性域]] — 安全事件响应

## 版本兼容性

| 工具 | 最低 K8s 版本 | 推荐版本 |
|------|--------------|----------|
| OPA/Gatekeeper | v1.26 | v1.28+ |
| Kyverno | v1.26 | v1.28+ |
| Falco | v1.24 | v1.28+ |
| Cilium | v1.26 | v1.28+ |
| Trivy | 无限制 | 最新 |
| Vault | v1.24 | v1.28+ |

> **注意**: 安全工具应与 Kubernetes 版本保持兼容，升级 K8s 前先验证工具兼容性。

## 故障排查

| 问题 | 可能原因 | 解决方案 |
|------|----------|----------|
| OPA 策略不生效 | ConstraintTemplate 未应用 | `kubectl get constrainttemplate` |
| Falco 无告警 | 规则未加载 | 检查 ConfigMap 挂载 |
| Trivy 扫描失败 | 网络无法访问镜像仓库 | 配置代理或镜像缓存 |
| Vault 连接失败 | Token 过期 | 刷新 ServiceAccount Token |

## Related

- [[08-安全/README.md|返回目录]]
- [[21-生态参考/03-领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[21-生态参考/03-领域索引/etcd-index.md|etcd 知识图谱索引]]
- [[21-生态参考/03-领域索引/observability-index.md|Observability 可观测性知识图谱索引]]
- [[21-生态参考/03-领域索引/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
