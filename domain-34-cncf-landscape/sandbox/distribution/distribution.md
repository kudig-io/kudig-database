---
title: Distribution
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- prometheus
- containerd
- cri-o
- docker
- webhook
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
- Distribution 是什么
- 如何 Distribution
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Distribution
- cncf
- landscape
---


# Distribution

> **成熟度**: Sandbox | **加入时间**: 2022-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://distribution.github.io/distribution |
| **GitHub** | https://github.com/distribution/distribution |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 分类** | Provisioning |
| **适用场景** | 容器镜像仓库 |

---

## 项目概述

Distribution (原 Docker Registry) 是 OCI 容器镜像分发的参考实现。它提供了一个符合 OCI Distribution Specification 的镜像仓库服务器，用于存储和分发容器镜像及其他 OCI 工件。Distribution 是 Docker Hub、GitHub Container Registry 等大型容器仓库的底层实现。

---

## 核心特性

- **OCI 兼容**: 完整支持 OCI Distribution Spec
- **多存储后端**: 文件系统、S3、Azure Blob、GCS
- **镜像代理**: 作为上游仓库的 pull-through 缓存
- **Webhook 通知**: 镜像推送/拉取事件通知
- **垃圾回收**: 清理未使用的镜像层
- **认证集成**: Bearer Token、Basic Auth、LDAP
- **TLS 支持**: 原生 HTTPS 支持

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                  Distribution Architecture                       │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌─────────────────────────────────────────────────────────┐   │
│  │                      Clients                              │   │
│  │  ┌───────────┐  ┌───────────┐  ┌───────────────────────┐ │   │
│  │  │  Docker   │  │   Podman  │  │  containerd/cri-o     │ │   │
│  │  │  CLI      │  │   CLI     │  │  (Kubernetes)         │ │   │
│  │  └─────┬─────┘  └─────┬─────┘  └───────────┬───────────┘ │   │
│  └────────┼──────────────┼─────────────────────┼────────────┘   │
│           │              │                     │                │
│           └──────────────┼─────────────────────┘                │
│                          │  OCI Distribution API                │
│                          │  (HTTP/HTTPS)                        │
│  ┌───────────────────────▼───────────────────────────────────┐  │
│  │                 Distribution Server                        │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │                   HTTP API                           │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │  │  │
│  │  │  │   /v2/      │  │  /v2/{name}/│  │  /v2/{name}/│  │  │  │
│  │  │  │   (Base)    │  │   manifests │  │   blobs    │  │  │  │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │  │  │
│  │  │  ┌─────────────┐  ┌─────────────┐  ┌────────────┐  │  │  │
│  │  │  │   Tags      │  │  Catalog    │  │  Upload    │  │  │  │
│  │  │  │   List      │  │  List       │  │  Blob      │  │  │  │
│  │  │  └─────────────┘  └─────────────┘  └────────────┘  │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                           │                                │  │
│  │  ┌────────────────────────▼────────────────────────────┐  │  │
│  │  │                  Storage Driver                      │  │  │
│  │  │  ┌──────────┐ ┌──────────┐ ┌──────────┐ ┌────────┐  │  │  │
│  │  │  │ Filesystem│ │   S3    │ │  Azure   │ │  GCS   │  │  │  │
│  │  │  └──────────┘ └──────────┘ └──────────┘ └────────┘  │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  │                                                            │  │
│  │  ┌─────────────────────────────────────────────────────┐  │  │
│  │  │                   Auth / Middleware                   │  │  │
│  │  │  ┌──────────┐ ┌──────────┐ ┌────────────────────┐  │  │  │
│  │  │  │  Token   │ │  Basic   │ │  Proxy Cache       │  │  │  │
│  │  │  │  Auth    │ │  Auth    │ │  (Pull-through)    │  │  │  │
│  │  │  └──────────┘ └──────────┘ └────────────────────┘  │  │  │
│  │  └─────────────────────────────────────────────────────┘  │  │
│  └────────────────────────────────────────────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### Docker 运行

```bash
# 启动本地仓库
docker run -d -p 5000:5000 --name registry registry:2

# 推送镜像
docker tag my-app localhost:5000/my-app:v1
docker push localhost:5000/my-app:v1

# 拉取镜像
docker pull localhost:5000/my-app:v1
```

### 持久化存储

```bash
docker run -d \
  -p 5000:5000 \
  --name registry \
  -v /data/registry:/var/lib/registry \
  -e REGISTRY_STORAGE_DELETE_ENABLED=true \
  registry:2
```

### Kubernetes 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: registry
  namespace: registry
spec:
  replicas: 1
  selector:
    matchLabels:
      app: registry
  template:
    metadata:
      labels:
        app: registry
    spec:
      containers:
        - name: registry
          image: registry:2
          ports:
            - containerPort: 5000
          volumeMounts:
            - name: registry-data
              mountPath: /var/lib/registry
            - name: registry-config
              mountPath: /etc/docker/registry
          env:
            - name: REGISTRY_STORAGE_DELETE_ENABLED
              value: "true"
      volumes:
        - name: registry-data
          persistentVolumeClaim:
            claimName: registry-data
        - name: registry-config
          configMap:
            name: registry-config

---
apiVersion: v1
kind: Service
metadata:
  name: registry
  namespace: registry
spec:
  ports:
    - port: 5000
      targetPort: 5000
  selector:
    app: registry
```

---

## 配置文件

### 基本配置

```yaml
# config.yml
version: 0.1
log:
  level: info
  formatter: text
  fields:
    service: registry

storage:
  filesystem:
    rootdirectory: /var/lib/registry
  delete:
    enabled: true
  cache:
    blobdescriptor: inmemory
  maintenance:
    uploadpurging:
      enabled: true
      age: 168h
      interval: 24h
      dryrun: false

http:
  addr: :5000
  headers:
    X-Content-Type-Options: [nosniff]
  http2:
    disabled: false
```

### S3 存储后端

```yaml
version: 0.1
storage:
  s3:
    accesskey: AKIAXXXXXXXXXXXX
    secretkey: xxxxxxxxxxxxxxxxxxxxx
    region: us-east-1
    bucket: my-registry-bucket
    rootdirectory: /registry
    encrypt: true
    secure: true
    v4auth: true
    chunksize: 5242880  # 5MB
    multipartcopychunksize: 33554432  # 32MB
    multipartcopymaxconcurrency: 100
    multipartcopythresholdsize: 33554432

http:
  addr: :5000
```

### TLS 配置

```yaml
version: 0.1
http:
  addr: :5000
  tls:
    certificate: /certs/domain.crt
    key: /certs/domain.key
    clientcas:
      - /certs/ca.crt
```

### 认证配置

```yaml
version: 0.1
auth:
  htpasswd:
    realm: basic-realm
    path: /etc/registry/htpasswd

# 或使用 Token 认证
auth:
  token:
    realm: https://auth.example.com/token
    service: registry
    issuer: registry-token-issuer
    rootcertbundle: /etc/registry/auth.crt
```

```bash
# 创建 htpasswd 文件
htpasswd -Bbn admin password > /etc/registry/htpasswd
```

---

## Pull-Through 缓存代理

### 配置 Docker Hub 缓存

```yaml
version: 0.1
proxy:
  remoteurl: https://registry-1.docker.io
  username: [optional]
  password: [optional]

storage:
  filesystem:
    rootdirectory: /var/lib/registry
  delete:
    enabled: true

http:
  addr: :5000
```

### 使用缓存代理

```json
// /etc/docker/daemon.json
{
  "registry-mirrors": ["https://my-registry-mirror:5000"]
}
```

---

## Webhook 通知

```yaml
version: 0.1
notifications:
  endpoints:
    - name: webhook
      url: https://webhook.example.com/registry
      headers:
        Authorization: [Bearer xxx]
      timeout: 1s
      threshold: 5
      backoff: 1s
      disabled: false
      
    - name: slack
      url: https://hooks.slack.com/services/xxx
      timeout: 3s
      threshold: 10
```

---

## 垃圾回收

```bash
# 查看可回收的数据
docker exec registry bin/registry garbage-collect /etc/docker/registry/config.yml --dry-run

# 执行垃圾回收
docker exec registry bin/registry garbage-collect /etc/docker/registry/config.yml

# 删除未引用的 manifest 后执行
docker exec registry bin/registry garbage-collect /etc/docker/registry/config.yml --delete-untagged
```

---

## API 使用

```bash
# 检查 API 可用性
curl https://registry.example.com/v2/

# 列出仓库
curl https://registry.example.com/v2/_catalog

# 列出标签
curl https://registry.example.com/v2/my-app/tags/list

# 获取 Manifest
curl -H "Accept: application/vnd.oci.image.manifest.v1+json" \
  https://registry.example.com/v2/my-app/manifests/v1.0

# 删除镜像 (需要先获取 digest)
DIGEST=$(curl -sI -H "Accept: application/vnd.oci.image.manifest.v1+json" \
  https://registry.example.com/v2/my-app/manifests/v1.0 | grep Docker-Content-Digest | awk '{print $2}' | tr -d '\r')
curl -X DELETE https://registry.example.com/v2/my-app/manifests/$DIGEST
```

---

## 监控

### Prometheus 指标

```yaml
version: 0.1
http:
  debug:
    addr: :5001
    prometheus:
      enabled: true
      path: /metrics
```

| 指标 | 说明 |
|:---|:---|
| `registry_http_requests_total` | HTTP 请求总数 |
| `registry_http_request_duration_seconds` | 请求延迟 |
| `registry_storage_action_seconds` | 存储操作延迟 |
| `registry_storage_cache_total` | 缓存命中/未命中 |

---

## 最佳实践

1. **TLS 加密**: 生产环境必须启用 TLS
2. **认证授权**: 配置 Token 或 htpasswd 认证
3. **存储选择**: 生产环境使用对象存储 (S3/GCS/Azure)
4. **垃圾回收**: 定期执行垃圾回收释放空间
5. **高可用**: 使用共享存储后端部署多副本
6. **缓存代理**: 使用 pull-through cache 减少外部流量

---

## 参考资源

- [官方文档](https://distribution.github.io/distribution/)
- [GitHub Repo](https://github.com/distribution/distribution)
- [OCI Distribution Spec](https://github.com/opencontainers/distribution-spec)
- [API 参考](https://distribution.github.io/distribution/spec/api/)
- [配置参考](https://distribution.github.io/distribution/about/configuration/)

---

**维护者**: Kudig Team | **许可证**: MIT
