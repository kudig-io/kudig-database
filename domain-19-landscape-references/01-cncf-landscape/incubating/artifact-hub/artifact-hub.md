---
title: Artifact Hub
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- coredns
- helm
- docker
- opa
- falco
- redis
- postgresql
- operator
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Artifact Hub 是什么
- 如何 Artifact Hub
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- Artifact
- Hub
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
- helm-basics
- redis-basics
- policy-basics
---

title: Artifact Hub
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- coredns
- helm
- docker
- opa
- falco
- redis
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Artifact Hub 是什么
- 如何 Artifact Hub
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Artifact
- Hub
- cncf
- landscape
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---
# Artifact Hub

> **成熟度**: Incubating | **加入时间**: 2020-06 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://artifacthub.io |
| **GitHub** | https://github.com/artifacthub/hub |
| **许可证** | Apache-2.0 |
| **主要语言** | Go, TypeScript |
| **CNCF 分类** | App Definition & Discovery |

---

## 项目概述

Artifact Hub 是云原生制品的发现和分发平台。它是 CNCF 生态系统的中央枢纽，支持搜索、发现和发布 Helm charts、OPA 策略、Falco 规则、KEDA scalers 等多种制品类型。

## 核心特性

- **统一搜索**: 跨多种制品类型的全文搜索
- **丰富元数据**: 版本、依赖、安全评级、维护者信息
- **安全扫描**: 自动检测镜像漏洞和安全问题
- **签名验证**: 支持 Cosign 签名的制品验证
- **订阅通知**: 跟踪制品更新，接收变更通知
- **私有仓库**: 支持托管私有制品仓库

---

## 支持的制品类型

| 类型 | 说明 |
|------|------|
| Helm charts | Kubernetes 应用包 |
| OPA policies | Open Policy Agent 策略 |
| Falco rules | 运行时安全规则 |
| KEDA scalers | 事件驱动自动扩缩器 |
| Tekton tasks | CI/CD 任务 |
| Krew plugins | kubectl 插件 |
| OLM operators | Operator 目录 |
| Tinkerbell actions | 裸机配置动作 |
| CoreDNS plugins | DNS 插件 |
| KubeArmor policies | 安全策略 |
| Headlamp plugins | UI 插件 |

---

## 快速开始

### 搜索和使用制品

```bash
# 搜索 Helm chart
# 访问 https://artifacthub.io 或使用 helm search

# 添加仓库（以 Bitnami 为例）
helm repo add bitnami https://charts.bitnami.com/bitnami

# 安装 chart
helm install my-redis bitnami/redis
```

### 使用 CLI

```bash
# 安装 Artifact Hub CLI
brew install artifacthub/tap/ah

# 搜索制品
ah search nginx

# 获取制品详情
ah get helm/bitnami/nginx

# 列出版本
ah list helm/bitnami/nginx
```

---

## 发布制品

### Helm Chart 发布

1. **创建仓库配置文件**

```yaml
# artifacthub-repo.yml
repositoryID: xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
owners:
  - name: maintainer
    email: maintainer@example.com
```

2. **添加 Chart 元数据**

```yaml
# Chart.yaml
apiVersion: v2
name: my-app
version: 1.0.0
description: My awesome application
home: https://github.com/example/my-app
maintainers:
  - name: John Doe
    email: john@example.com
annotations:
  artifacthub.io/changes: |
    - Added new feature X
    - Fixed bug Y
  artifacthub.io/images: |
    - name: my-app
      image: myregistry/my-app:1.0.0
  artifacthub.io/license: Apache-2.0
  artifacthub.io/links: |
    - name: Documentation
      url: https://docs.example.com
  artifacthub.io/signKey: |
    fingerprint: XXXX
    url: https://example.com/pgp-key.asc
```

3. **注册仓库**

在 Artifact Hub 网站上添加仓库 URL，系统会自动发现和索引制品。

### OPA Policy 发布

```yaml
# artifacthub-pkg.yml
version: 1.0.0
name: my-policy
displayName: My Security Policy
description: |
  This policy ensures containers run as non-root.
provider:
  name: Example Inc
links:
  - name: Source
    url: https://github.com/example/policies
install: |
  kubectl apply -f https://raw.githubusercontent.com/example/policies/main/my-policy.yaml
```

---

## 安全特性

### 安全评级

Artifact Hub 自动计算安全评级（A-F），基于：

- 是否提供来源信息
- 是否有安全策略文档
- 镜像漏洞扫描结果
- 签名验证状态
- 维护活跃度

### 签名验证

```yaml
# 使用 Cosign 签名
annotations:
  artifacthub.io/signKey: |
    fingerprint: 1234567890ABCDEF
    url: https://example.com/cosign.pub
```

### 漏洞扫描

Artifact Hub 自动扫描引用的容器镜像，显示：
- CVE 漏洞列表
- 严重级别分布
- 修复版本建议

---

## 高级功能

### 订阅和通知

```bash
# 通过 UI 订阅制品更新
# 支持 Email 和 Webhook 通知

# Webhook 示例
POST https://your-server/webhook
{
  "event": "package.update",
  "package": {
    "name": "nginx",
    "version": "1.2.0",
    "repository": "bitnami"
  }
}
```

### API 使用

```bash
# 搜索 API
curl "https://artifacthub.io/api/v1/packages/search?ts_query_web=nginx&kind=0"

# 获取制品详情
curl "https://artifacthub.io/api/v1/packages/helm/bitnami/nginx"

# 获取统计数据
curl "https://artifacthub.io/api/v1/packages/helm/bitnami/nginx/stats"
```

---

## 自托管部署

```yaml
# docker-compose.yml
version: '3.8'
services:
  hub:
    image: artifacthub/hub:latest
    environment:
      - AH_DB_HOST=db
      - AH_DB_DATABASE=hub
      - AH_DB_USER=hub
      - AH_DB_PASSWORD=secret
    ports:
      - "8000:8000"
    depends_on:
      - db
      
  db:
    image: postgres:15
    environment:
      - POSTGRES_USER=hub
      - POSTGRES_PASSWORD=secret
      - POSTGRES_DB=hub
    volumes:
      - pgdata:/var/lib/postgresql/data

  tracker:
    image: artifacthub/tracker:latest
    environment:
      - AH_DB_HOST=db
      - AH_DB_DATABASE=hub
      - AH_DB_USER=hub
      - AH_DB_PASSWORD=secret

volumes:
  pgdata:
```

```bash
# Helm 部署
helm repo add artifact-hub https://artifacthub.github.io/hub/
helm install artifact-hub artifact-hub/artifact-hub \
  --namespace artifact-hub \
  --create-namespace
```

---

## 最佳实践

1. **完善元数据**: 提供详细的描述、截图、安装说明
2. **版本语义化**: 遵循 SemVer 规范管理版本
3. **安全扫描**: 定期更新镜像，修复漏洞
4. **签名制品**: 使用 Cosign 签名增加可信度
5. **保持活跃**: 定期更新和响应用户反馈

---

## 参考资源

- [Artifact Hub](https://artifacthub.io)
- [GitHub Repo](https://github.com/artifacthub/hub)
- [API 文档](https://artifacthub.io/docs/api/)
- [发布指南](https://artifacthub.io/docs/topics/repositories/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/helm.md|helm]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/cncf-cicd|CNCF CI/CD 与发布管理项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/gitops-cicd-index|GitOps / CI-CD 全局索引]]
