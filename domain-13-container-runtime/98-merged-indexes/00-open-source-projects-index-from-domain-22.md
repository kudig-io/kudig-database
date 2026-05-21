---
title: Domain-22 容器镜像管理 — 开源项目索引
description: '# Domain-22 容器镜像管理 — 开源项目索引'
category: container-image-management
tags:
- k8s
- container
- image
- registry
- harbor
- scheduler
- helm
- docker
- opa
- falco
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 开发工程师
estimated_read_time: 5min
intent_queries:
- Domain-22 容器镜像管理 — 开源项目索引 是什么
- 如何 Domain-22 容器镜像管理 — 开源项目索引
- Kubernetes 22 container image management 最佳实践
trigger_keywords:
- Domain-22
- 容器镜像管理
- 开源项目索引
- container
- image
- management
prerequisites:
- kubectl-basics
- helm-basics
- redis-basics
- policy-basics
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

# Domain-22 容器镜像管理 — 开源项目索引

> **最后更新**: 2026-04-24  
> **适用版本**: Harbor v2.13 / Dragonfly v2.2 / cosign v2.4

---

## 📋 目录

- [一、核心项目总览](#一核心项目总览)
- [二、Harbor (CNCF Graduated)](#二harbor-cncf-graduated)
- [三、Dragonfly (CNCF Graduated)](#三dragonfly-cncf-graduated)
- [四、镜像安全与签名](#四镜像安全与签名)
- [五、企业级镜像仓库](#五企业级镜像仓库)
- [六、镜像构建工具](#六镜像构建工具)
- [七、版本与兼容矩阵](#七版本与兼容矩阵)
- [八、镜像管理架构选型](#八镜像管理架构选型)

---

## 一、核心项目总览

| 项目 | 作用 | CNCF 状态 | 最新版本 | Stars | License |
|:---|:---|:---|:---|:---|:---|
| **Harbor** | 企业镜像仓库 | Graduated | v2.13.0 | 25k+ | Apache-2.0 |
| **Dragonfly** | P2P 镜像分发 | Graduated | v2.2.0 | 2k+ | Apache-2.0 |
| **Notary** | 镜像内容信任 | Incubating | v2.0.0 | 3k+ | Apache-2.0 |
| **cosign** | 镜像签名 (Sigstore) | OpenSSF | v2.4.0 | 4k+ | Apache-2.0 |
| **Syft** | SBOM 生成 | Anchore | v1.22.0 | 6k+ | Apache-2.0 |
| **Grype** | 漏洞扫描 (Syft 配套) | Anchore | v0.87.0 | 8k+ | Apache-2.0 |
| **Trivy** | 镜像漏洞扫描 | Aqua | v0.61.0 | 24k+ | Apache-2.0 |
| **Quay** | Red Hat 镜像仓库 | Red Hat | v3.14.0 | 7k+ | Apache-2.0 |
| **JFrog Artifactory** | 通用制品库 | JFrog | 7.x | - | 商业 |
| **GitLab Registry** | 集成镜像仓库 | GitLab | v17.10.0 | - | EE/CE |
| **Amazon ECR** | AWS 托管仓库 | AWS | - | - | 商业 |
| **Distribution** | Docker Registry v2 | CNCF | v2.8.3 | 8k+ | Apache-2.0 |

---

## 二、Harbor (CNCF Graduated)

### 2.1 核心特性

```yaml
# 企业级功能
- 多租户项目管理
- 镜像复制 (跨 Harbor/ACR/ECR/GCR/AliCR)
- 漏洞扫描 (Trivy/Clair 集成)
- 镜像签名 (Notary/cosign 集成)
-  Helm Chart 管理
- 机器人账户与细粒度 RBAC
- 垃圾回收与保留策略
- OIDC/LDAP/UA 认证集成
```

### 2.2 架构组件

| 组件 | 作用 |
|:---|:---|
| core | API 与业务逻辑 |
| registry | Docker Distribution 后端 |
| portal | Web UI |
| jobservice | 异步任务 (复制/扫描/GC) |
| trivy-adapter | 漏洞扫描适配器 |
| notary-server/signer | 内容信任服务 |
| chartmuseum | Helm Chart 存储 |
| redis | 缓存与任务队列 |
| PostgreSQL | 元数据数据库 |

### 2.3 部署模式

```yaml
# Helm 部署 (推荐)
helm repo add harbor https://helm.goharbor.io
helm install harbor harbor/harbor --version 1.16.0

# 关键配置
expose:
  type: ingress
  tls:
    enabled: true
    certSource: auto  # 或 certManager
```

**GitHub**: https://github.com/goharbor/harbor
**文档**: https://goharbor.io/docs/

---

## 三、Dragonfly (CNCF Graduated)

### 3.1 P2P 镜像与文件分发

```yaml
# 核心特性
- P2P 技术加速大规模镜像分发
- 减少 Registry 带宽压力 (最高 99% 节省)
- 支持多种协议 (HTTP/HTTPS/HDFS/OSS/S3)
- 与 Harbor 集成作为镜像缓存层
- 支持预热 (Preheat) 策略
```

### 3.2 架构组件

| 组件 | 作用 |
|:---|:---|
| manager | 调度与配置管理 |
| scheduler | 下载任务调度 |
| dfdaemon | 节点代理 (seed peer / peer) |
| seed peer | 预缓存完整内容的超级节点 |

### 3.3 Harbor + Dragonfly 集成

```yaml
# 在 Harbor 中配置 Dragonfly 预热
# 当镜像推送时，自动分发到各节点 seed peer
# 避免大规模拉取时的 Registry 单点瓶颈
```

**GitHub**: https://github.com/dragonflyoss/Dragonfly2
**文档**: https://d7y.io/docs/

---

## 四、镜像安全与签名

### 4.1 cosign / Sigstore

```yaml
# 核心能力
- 密钥对签名 (keypair)
- 密钥less签名 (Fulcio + Rekor)
- 签名存储在 OCI registry (无需额外服务)
- SBOM 签名与验证
- 与 Kyverno/OPA 集成策略验证
```

**使用示例**
```bash
# 生成密钥对
cosign generate-key-pair

# 签名镜像
cosign sign --key cosign.key harbor.example.com/project/image:tag

# 验证镜像
cosign verify --key cosign.pub harbor.example.com/project/image:tag

# 密钥less签名 (推荐 CI/CD)
cosign sign --oidc-issuer https://token.actions.githubusercontent.com \
  harbor.example.com/project/image:tag
```

**GitHub**: https://github.com/sigstore/cosign

### 4.2 Notary v2

- OCI 原生内容信任标准
- 与 Harbor 集成
- 支持多签名、阈值签名
- 渐进式替代 Docker Content Trust (Notary v1)

### 4.3 漏洞扫描

| 工具 | 扫描范围 | 集成方式 |
|:---|:---|:---|
| Trivy | OS + 语言包 + 配置 | CLI / CI / Harbor / Defender |
| Grype | OS + 语言包 | CLI / CI |
| Clair | OS 包 | Harbor 内置 |
| Snyk | 全面 + SAST | CI / IDE / Harbor |

---

## 五、企业级镜像仓库

### 5.1 托管 vs 自建对比

| 维度 | Harbor (自建) | Quay (自建/托管) | JFrog Artifactory | ECR (托管) |
|:---|:---|:---|:---|:---|
| 成本 | 基础设施成本 | 基础设施/订阅 | 订阅许可 | 按用量计费 |
| 多租户 | ✅ 项目级 | ✅ 组织级 | ✅ 全面 | ✅ IAM |
| 漏洞扫描 | ✅ Trivy/Clair | ✅ Clair | ✅ Xray | ✅ Inspector |
| 镜像签名 | ✅ Notary/cosign | ✅ cosign | ✅ 多方案 | ✅ Notary |
| Helm 支持 | ✅ Chartmuseum | ✅ | ✅ | ❌ |
| 多 Registry 复制 | ✅ | ✅ | ✅ | ❌ |
| 高可用 | 需自行配置 | 需自行配置 | 企业版内置 | 托管 |
| 合规认证 | 需自行认证 | 需自行认证 | 企业版内置 | 托管认证 |

### 5.2 Quay (Red Hat)

- 与 OpenShift 深度集成
- 地理复制 (Geo-replication)
- 镜像流镜 (Repository mirroring)
- 构建触发器 (Git webhook 自动构建)

**GitHub**: https://github.com/quay/quay

---

## 六、镜像构建工具

| 工具 | 特点 | 适用场景 |
|:---|:---|:---|
| **Docker BuildKit** | 并发构建、缓存优化、多平台 | 通用构建 |
| **Buildah** | 无守护进程、Rootless | 安全构建环境 |
| **kaniko** | 纯 K8s 内构建、无需特权 | CI/CD 流水线 |
| **ko** | Go 项目专用、极简 | Go 微服务 |
| **buildpacks** | 源代码→镜像、自动检测 | 开发者平台 |
| **Podman** | Docker 兼容、无守护进程 | 开发/CI 替代 |

---

## 七、版本与兼容矩阵

| 组件 | K8s v1.31 | v1.32 | v1.33 | 备注 |
|:---|:---|:---|:---|:---|
| Harbor v2.13 | ✅ | ✅ | ✅ | Helm Chart v1.16 |
| Dragonfly v2.2 | ✅ | ✅ | ✅ | 作为 DaemonSet |
| cosign v2.4 | ✅ | ✅ | ✅ | 客户端工具 |
| Notary v2 | ✅ | ✅ | ✅ | 需配置信任根 |
| Trivy v0.61 | ✅ | ✅ | ✅ | 独立扫描 |
| Distribution v2.8 | ✅ | ✅ | ✅ | Registry 后端 |

---

## 八、镜像管理架构选型

```
┌─────────────────────────────────────────────────────────────┐
│                企业镜像管理参考架构                            │
└─────────────────────────────────────────────────────────────┘

开发阶段
  ├── Dockerfile / buildpacks ──► 构建镜像
  ├── Trivy/Grype ──► 漏洞扫描 (阻断高危漏洞)
  ├── cosign ──► 镜像签名
  └── Syft ──► SBOM 生成与归档

CI/CD 流水线
  ├── 推送到 Harbor Dev 项目
  ├── 触发 Dragonfly 预热 (如需要)
  └── 晋升到 Staging/Prod 项目 (复制策略)

生产环境
  ├── Dragonfly P2P ──► 加速节点拉取
  ├── Kyverno/OPA ──► 准入控制 (仅允许签名镜像)
  ├── Falco ──► 运行时检测异常镜像启动
  └── 定期重新扫描 (新 CVE 发现时)

合规与审计
  ├── Harbor 保留策略 ──► 自动清理旧镜像
  ├── Notary/cosign ──► 签名链验证
  ├── SBOM 归档 ──► 供应链追溯
  └── 镜像复制 ──► 异地灾备
```

---

## 参考链接

- [Harbor 官方文档](https://goharbor.io/docs/)
- [Dragonfly 官方文档](https://d7y.io/docs/)
- [Sigstore/cosign 文档](https://docs.sigstore.dev/)
- [Trivy 官方文档](https://aquasecurity.github.io/trivy/)
- [OCI Distribution Spec](https://github.com/opencontainers/distribution-spec)
- [CNCF 软件供应链安全最佳实践](https://github.com/cncf/tag-security/blob/main/supply-chain-security/supply-chain-security-paper/CNCF_SSCP_v1.pdf)

---

## Obsidian 相关文档

- [[domain-13-container-runtime/MOC.md|domain-22-container-image-management MOC]]
- [[domain-13-container-runtime/README.md|Domain 22: 容器镜像管理 (Container Image Management)]]
- [[domain-13-container-runtime/01-harbor-enterprise-image-registry.md|Harbor企业级容器镜像仓库深度实践]]
- [[domain-13-container-runtime/02-docker-registry-enterprise-distribution.md|Docker Registry企业级镜像分发深度实践]]
- [[domain-13-container-runtime/03-jfrog-artifactory-enterprise.md|JFrog Artifactory Enterprise Container Registry Platform]]
- [[domain-13-container-runtime/04-harbor-enterprise-security-scanning.md|Harbor企业级镜像安全扫描深度实践]]
- [[domain-13-container-runtime/04-quay-enterprise-registry.md|Quay Container Registry 企业级镜像管理深度实践]]
- [[domain-13-container-runtime/05-gitlab-container-registry-enterprise.md|GitLab Container Registry Enterprise 深度实践]]
- [[domain-13-container-runtime/06-amazon-ecr-enterprise.md|Amazon ECR (Elastic Container Registry) Enterprise 深度实践]]
- [[domain-13-container-runtime/99-harbor-enterprise-guide.md|Harbor 企业级镜像仓库部署指南]]
