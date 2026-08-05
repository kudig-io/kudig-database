---
title: 供应链安全事件响应：镜像篡改与 CVE
description: 面向阿里云/专有云 K8s 的供应链安全事件响应方案，涵盖镜像篡改检测、CVE 应急响应、SBOM 溯源与修复流程。
summary: 面向阿里云/专有云 K8s 的供应链安全事件响应方案，涵盖镜像篡改检测、CVE 应急响应、SBOM 溯源与修复流程。
category: security
tags:
- k8s
- security
- supply-chain
- image-tampering
- cve
- sbom
- cosign
- sigstore
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- DevOps 工程师
estimated_read_time: 20min
intent_queries:
- 供应链安全事件响应
- 镜像篡改检测
- K8s CVE 应急响应
trigger_keywords:
- supply chain
- 镜像篡改
- CVE
- SBOM
- cosign
- 供应链安全
prerequisites:
- kubectl-basics
- security-basics
- image-basics
- registry-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 供应链安全事件响应：镜像篡改与 CVE

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 环境，讲解供应链安全事件的检测、响应与修复，重点覆盖镜像篡改与 CVE。

## 目录

1. [供应链威胁概述](#供应链威胁概述)
2. [镜像篡改检测](#镜像篡改检测)
3. [CVE 应急响应](#cve-应急响应)
4. [SBOM 与溯源](#sbom-与溯源)
5. [镜像签名与验证](#镜像签名与验证)
6. [事件响应流程](#事件响应流程)
7. [修复与加固](#修复与加固)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. 供应链威胁概述

### 1.1 常见供应链攻击

| 攻击类型 | 示例 | 影响 |
|:---|:---|:---|
| 镜像篡改 | 攻击者替换镜像层注入后门 | 运行时恶意代码执行 |
| 依赖投毒 | 恶意 npm/pypi/maven 包 | 构建时感染 |
| CI/CD 泄露 | 私钥、凭证泄露 | 构建产物被篡改 |
| 基础镜像 CVE | glibc、OpenSSL 漏洞 | 容器可被利用 |
| Registry 劫持 | 镜像仓库被入侵 | 全量镜像不可信 |

### 1.2 防御层次

```
镜像构建 → 镜像签名 → 镜像扫描 → 准入控制 → 运行时监控
```

---

## 2. 镜像篡改检测

### 2.1 镜像摘要比对

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 获取本地运行镜像的 digest
kubectl get pod <pod> -n <ns> -o jsonpath='{.status.containerStatuses[0].imageID}'

# 与 Registry 中的 digest 比对
aliyun cr GetRepoTag --RepoName <repo> --Tag <tag>
```
### 2.2 Trivy 镜像扫描

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 扫描镜像是否存在恶意软件与已知漏洞
kubectl run trivy-scan --rm -it --image=aquasec/trivy -- \
  image --severity HIGH,CRITICAL \
  registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.0
```
### 2.3 异常镜像启动告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: image-tampering-alerts
  namespace: monitoring
spec:
  groups:
    - name: image.tampering
      rules:
        - alert: ImageDigestMismatch
          expr: |
            kube_pod_container_status_running == 1
            and on(pod,namespace,container)
            (
              kube_pod_container_info{image!~".*@(sha256:[a-f0-9]+)"}
            )
          for: 1m
          labels:
            severity: warning
          annotations:
            summary: "存在未使用固定 digest 的容器镜像"
        - alert: ImageFromUntrustedRegistry
          expr: |
            kube_pod_container_info{image!~"^registry\.cn-hangzhou\.aliyuncs\.com/.*"}
          for: 1m
          labels:
            severity: critical
          annotations:
            summary: "Pod 使用了非信任仓库镜像"
```

---

## 3. CVE 应急响应

### 3.1 CVE 发现渠道

- 官方安全公告：Kubernetes Security Announcements
- 镜像扫描报告：Trivy、Snyk、Clair
- 阿里云安全公告
- NVD / CVE 数据库

### 3.2 CVE 影响评估

| 评估项 | 问题 |
|:---|:---|
| 受影响组件 | 哪个镜像/包存在漏洞 |
| 利用条件 | 是否需要网络可达、认证 |
| 实际风险 | 集群是否暴露相关接口 |
| 修复版本 | 官方是否已发布补丁 |

### 3.3 快速定位受影响 Pod

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 扫描所有运行镜像，输出 CVE 列表
for img in $(kubectl get pods --all-namespaces -o jsonpath='{.items[*].spec.containers[*].image}' | tr ' ' '\n' | sort -u); do
  echo "Scanning: $img"
  kubectl run trivy-$RANDOM --rm -i --restart=Never --image=aquasec/trivy -- \
    image --severity CRITICAL "$img" 2>/dev/null | grep -E "^|" || true
done
```
---

## 4. SBOM 与溯源

### 4.1 生成 SBOM

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 使用 Syft 生成镜像 SBOM
kubectl run syft-sbom --rm -it --image=anchore/syft -- \
  registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.0 -o spdx-json \
  > order-service-v1.2.0.sbom.json
```
### 4.2 SBOM 存储与查询

```bash
# 上传 SBOM 到 OSS
ossutil cp order-service-v1.2.0.sbom.json oss://sbom-bucket/production/

# 查询包含特定包的镜像
jq '.packages[] | select(.name=="log4j-core") | .version' order-service-v1.2.0.sbom.json
```

---

## 5. 镜像签名与验证

### 5.1 Cosign 签名镜像

```bash
# 生成密钥对
cosign generate-key-pair

# 签名镜像
cosign sign --key cosign.key \
  registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.0

# 验证签名
cosign verify --key cosign.pub \
  registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.0
```

### 5.2 准入控制验证

```yaml
apiVersion: admissionregistration.k8s.io/v1
kind: ValidatingWebhookConfiguration
metadata:
  name: image-signature-webhook
webhooks:
  - name: verify-image.example.com
    rules:
      - operations: ["CREATE", "UPDATE"]
        apiGroups: [""]
        apiVersions: ["v1"]
        resources: ["pods"]
    clientConfig:
      service:
        name: image-signature-webhook
        namespace: security
        path: "/validate"
    admissionReviewVersions: ["v1"]
    sideEffects: None
```

---

## 6. 事件响应流程

### 6.1 镜像篡改响应

```
检测到异常镜像
    │
    ▼
隔离运行该镜像的 Pod
    │
    ▼
阻断镜像仓库或删除镜像 tag
    │
    ▼
从干净源代码重新构建并签名
    │
    ▼
重新部署并验证签名
    │
    ▼
审计 CI/CD 链路，查找泄露点
```

### 6.2 CVE 响应

```
扫描发现 CVE
    │
    ▼
评估 CVSS 与实际影响
    │
    ▼
确定修复版本或临时缓解措施
    │
    ▼
制定升级计划（变更窗口）
    │
    ▼
执行升级并验证
    │
    ▼
更新漏洞库与扫描策略
```

---

## 7. 修复与加固

### 7.1 镜像修复流程

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 1. 更新基础镜像
sed -i 's/FROM alpine:3.17/FROM alpine:3.20/' Dockerfile

# 2. 重新构建并签名
docker build -t registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.1 .
docker push registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.1
cosign sign --key cosign.key registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.1

# 3. 更新 K8s 资源
kubectl set image deployment/order-service order-service=registry.cn-hangzhou.aliyuncs.com/app/order-service:v1.2.1 -n production
```
### 7.2 供应链加固清单

| 加固项 | 说明 |
|:---|:---|
| 使用最小基础镜像 | distroless / alpine |
| 镜像签名 | cosign / notation |
| 镜像扫描 | CI/CD 流水线集成 |
| 私有 Registry | 访问控制与审计 |
| SBOM 管理 | 每次构建生成并存储 |
| 依赖锁定 | 使用 lockfile |
| 密钥管理 | Vault / KMS |

---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| 镜像扫描 | CI/CD 与运行时均扫描 | 流水线配置 |
| 镜像签名 | 生产镜像强制签名 | cosign verify |
| 准入控制 | 未签名镜像拒绝启动 | Webhook 日志 |
| SBOM | 每次发布生成 | OSS 桶 |
| CVE 响应流程 | 24 小时内评估 | 响应记录 |
| 可信仓库 | 白名单策略 | NetworkPolicy |

---

## SLSA 与供应链完整性

SLSA（Supply-chain Levels for Software Artifacts）提供从源码到交付的分级安全框架。建议在 CI/CD 中逐步实现 SLSA Level 2/3。

| 级别 | 要求 | 能力 |
|:---:|:---|:---|
| 1 | 生成 SBOM 与构建 provenance | 可追溯 |
| 2 | 使用版本控制与托管构建 | 可审计 |
| 3 | 源码与构建平台符合安全标准 | 防篡改 |
| 4 | 可复现构建、双人审查 | 高可信 |

### 阿里云镜像安全产品

- **容器镜像服务 ACR**：集成镜像扫描与签名
- **云安全中心**：检测镜像中的恶意文件与漏洞
- **密钥管理服务 KMS**：安全存储签名密钥

### 供应链事件复盘

发生镜像篡改或 CVE 事件后，需复盘以下问题：

1. 恶意代码或漏洞如何进入构建流程？
2. 准入控制为何未拦截？
3. 哪些集群/应用已部署受影响镜像？
4. 如何防止同类事件再次发生？

复盘结论应更新到 CI/CD 安全策略与准入规则中。

## 供应链安全治理框架

建立从源码到运行的全链路安全治理，是防止供应链事件的根本。

### 治理环节

```
源码仓库 → CI/CD 构建 → 镜像扫描 → 镜像签名 → 镜像仓库 → 准入控制 → 运行时监控
```

每个环节都需设置检查点与责任人：

| 环节 | 检查点 | 责任人 |
|:---|:---|:---|
| 源码 | 依赖审计、代码审查 | 开发 / 安全 |
| 构建 | 构建环境安全、不可变构建 | DevOps |
| 镜像扫描 | 无高危 CVE、无敏感信息 | SRE / 安全 |
| 镜像签名 | Cosign 签名验证 | 平台工程 |
| 准入控制 | 仅允许签名镜像 | SRE |
| 运行时 | 异常行为检测 | 安全运营 |

### 阿里云/专有云产品对应

| 能力 | 阿里云 | 专有云 |
|:---|:---|:---|
| 镜像扫描 | ACR 镜像扫描 | 自建 Trivy/Clair |
| 镜像签名 | ACR 镜像签名 / Cosign | Cosign |
| 密钥管理 | KMS | 自建 Vault |
| 运行时防护 | 云安全中心 | Falco/Tetragon |

## 典型工单场景与处理

**场景**：镜像扫描发现某基础镜像存在多个 CRITICAL CVE。

处理步骤：
1. 确认受影响镜像版本与部署范围。
2. 评估是否可被利用，是否需要立即下线。
3. 升级基础镜像并重新构建应用镜像。
4. 重新扫描并验证无高危 CVE。
5. 滚动更新所有使用该镜像的工作负载。

## 供应链安全事件分类响应

| 事件类型 | 紧急程度 | 响应重点 |
|:---|:---:|:---|
| 镜像被篡改 | P0 | 立即隔离、溯源、重建 |
| 发现 CRITICAL CVE | P0/P1 | 评估可利用性、紧急修复 |
| 依赖包被投毒 | P1 | 锁定版本、替换依赖、扫描 |
| 构建凭证泄露 | P1 | 吊销凭证、轮换密钥 |
| SBOM 缺失 | P2 | 补全 SBOM、建立流程 |

### 供应链事件沟通模板

```markdown
【供应链安全事件通报】
事件类型：镜像篡改 / CVE / 依赖投毒
影响镜像：registry/xxx:v1.2.3
影响范围：集群 A、集群 B 的 production 命名空间
已采取措施：隔离镜像、禁止该 digest 部署、启动重建
后续计划：扫描所有运行中镜像、修复后重新发布
```

### 供应链安全成熟度

| 等级 | 能力 |
|:---:|:---|
| 1 | 镜像漏洞扫描 |
| 2 | SBOM 生成与存档 |
| 3 | 镜像签名与准入控制 |
| 4 | 可复现构建与 SLSA L3 |
| 5 | 全链路自动化风险阻断 |

## Related

- [[13-生产运维/03-事件响应/09-incident-response-process.md|安全事件响应与应急处理流程]]
- [[08-安全/05-供应链/01-supply-chain-security-overview.md|供应链安全概述]]

## See Also

- [[08-安全/05-供应链/07-sigstore-cosign-signing.md|Sigstore Cosign 镜像签名]]
- [[08-安全/05-供应链/04-sbom-vulnerability-analysis.md|SBOM 与漏洞分析]]


<!-- risk-assessed -->
