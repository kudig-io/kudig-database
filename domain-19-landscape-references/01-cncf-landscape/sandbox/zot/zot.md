---
title: zot
description: 'description: ''## 项目概述'''
category: general
tags:
- cncf
- ecosystem
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- zot 是什么
- 如何 zot
- Kubernetes 19 landscape references 最佳实践
trigger_keywords:
- zot
- landscape
- references
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

title: zot
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- zot 是什么
- 如何 zot
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- zot
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
# zot

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://zotregistry.dev/ |
| **GitHub** | https://github.com/project-zot/zot |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

zot 是一个生产就绪的、OCI 原生的容器镜像注册表，完全基于 OCI Distribution Specification 构建。它以单一二进制文件的形式提供，内置镜像存储、搜索、签名验证、漏洞扫描等功能，无需依赖外部数据库或缓存服务。

### 核心特性

- **OCI 原生**: 完全基于 OCI Distribution Spec 和 OCI Image Spec
- **单一二进制**: 零外部依赖，部署简单
- **内置搜索**: 集成全文搜索，无需外部搜索引擎
- **签名验证**: 支持 cosign 和 Notary v2 签名验证
- **漏洞扫描**: 集成 Trivy 进行镜像漏洞分析
- **多存储后端**: 本地文件系统、S3、GCS 等
- **镜像同步**: 支持从上游注册表同步镜像
- **用户认证**: LDAP、htpasswd、Bearer Token 认证

---

## 快速开始

### 安装

```bash
# 二进制安装
curl -fsSL https://github.com/project-zot/zot/releases/latest/download/zot-linux-amd64 -o zot
chmod +x zot && sudo mv zot /usr/local/bin/

# Docker 运行
docker run -d -p 5000:5000 \
  -v /data/zot:/var/lib/registry \
  ghcr.io/project-zot/zot-linux-amd64:latest
```

### 基本配置

```json
{
  "distSpecVersion": "1.1.0",
  "storage": {
    "rootDirectory": "/var/lib/registry",
    "gc": true,
    "gcDelay": "1h",
    "gcInterval": "24h"
  },
  "http": {
    "address": "0.0.0.0",
    "port": "5000",
    "tls": {
      "cert": "/etc/zot/tls/server.crt",
      "key": "/etc/zot/tls/server.key"
    }
  },
  "log": {
    "level": "info"
  }
}
```

### 使用

```bash
# 推送镜像
docker tag my-app:latest localhost:5000/my-app:latest
docker push localhost:5000/my-app:latest

# 拉取镜像
docker pull localhost:5000/my-app:latest

# 搜索镜像
curl http://localhost:5000/v2/_catalog
curl http://localhost:5000/v2/my-app/tags/list
```

---

## 高级配置

### 认证

```json
{
  "http": {
    "auth": {
      "htpasswd": {
        "path": "/etc/zot/htpasswd"
      },
      "ldap": {
        "address": "ldap://ldap.example.com",
        "port": 389,
        "baseDN": "ou=users,dc=example,dc=com",
        "userAttribute": "uid"
      }
    },
    "accessControl": {
      "repositories": {
        "**": {
          "policies": [{
            "users": ["admin"],
            "actions": ["read", "create", "update", "delete"]
          }],
          "defaultPolicy": ["read"]
        },
        "private/**": {
          "policies": [{
            "users": ["dev-team"],
            "actions": ["read", "create"]
          }]
        }
      }
    }
  }
}
```

### 镜像同步

```json
{
  "extensions": {
    "sync": {
      "registries": [{
        "urls": ["https://registry-1.docker.io"],
        "content": [{
          "prefix": "library/nginx",
          "tags": {
            "regex": "^1\\.2[0-9].*$"
          }
        }],
        "onDemand": true,
        "pollInterval": "6h",
        "tlsVerify": true
      }]
    }
  }
}
```

### S3 存储后端

```json
{
  "storage": {
    "rootDirectory": "/var/lib/registry",
    "storageDriver": {
      "name": "s3",
      "region": "us-east-1",
      "bucket": "my-zot-registry",
      "secure": true
    }
  }
}
```

### 漏洞扫描和签名

```json
{
  "extensions": {
    "search": {
      "enable": true,
      "cve": {
        "updateInterval": "2h",
        "trivy": {
          "dbRepository": "ghcr.io/aquasecurity/trivy-db"
        }
      }
    },
    "trust": {
      "cosign": true,
      "notation": true
    }
  }
}
```

---

## Kubernetes 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: zot-registry
spec:
  replicas: 1
  selector:
    matchLabels:
      app: zot
  template:
    metadata:
      labels:
        app: zot
    spec:
      containers:
        - name: zot
          image: ghcr.io/project-zot/zot-linux-amd64:latest
          ports:
            - containerPort: 5000
          volumeMounts:
            - name: config
              mountPath: /etc/zot
            - name: data
              mountPath: /var/lib/registry
      volumes:
        - name: config
          configMap:
            name: zot-config
        - name: data
          persistentVolumeClaim:
            claimName: zot-data
```

---

## 最佳实践

1. **TLS 加密**: 生产环境始终启用 TLS 加密通信
2. **垃圾回收**: 启用 GC 定期清理未引用的镜像层
3. **访问控制**: 配置细粒度的仓库级别访问策略
4. **镜像同步**: 使用 onDemand 模式减少不必要的镜像拉取
5. **漏洞扫描**: 启用 Trivy 集成，定期扫描镜像漏洞
6. **高可用**: 使用 S3 等共享存储后端实现多副本部署

---

## 参考资源

- [zot 官方文档](https://zotregistry.dev/docs/)
- [zot GitHub](https://github.com/project-zot/zot)
- [OCI Distribution Spec](https://github.com/opencontainers/distribution-spec)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT

## Related

- [[domain-17-system-foundation/topic-cheat-sheet/go.md|go]]
- [[domain-17-system-foundation/topic-cheat-sheet/k8s.md|k8s]]
- [[domain-17-system-foundation/topic-cheat-sheet/git.md|git]]
- [[domain-17-system-foundation/topic-cheat-sheet/docker.md|docker]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/cncf-storage|CNCF 存储与数据库项目全景]] — Cross-reference
- [[domain-19-landscape-references/topic-index/etcd-index|etcd 知识图谱索引]]
