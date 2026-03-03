# SPIRE

> **成熟度**: Graduated | **加入时间**: 2018-03 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://spiffe.io/spire |
| **GitHub** | https://github.com/spiffe/spire |
| **文档** | https://spiffe.io/docs/latest/spire-about |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Security |

---

## 项目概述

### 简介
SPIRE (SPIFFE Runtime Environment) 是 SPIFFE 规范的生产就绪实现，提供了一套完整的服务身份管理系统，用于在分布式环境中自动颁发和验证工作负载身份。

### 发展历程
| 时间 | 里程碑 |
|:---|:---|
| 2017 | 由 Scytale 公司创建 |
| 2018-03 | 与 SPIFFE 一起加入 CNCF |
| 2020-06 | 晋升为 CNCF Incubating |
| 2022-09 | 晋升为 CNCF Graduated |

### 核心定位
SPIRE 是实现零信任安全架构的关键基础设施，为 Kubernetes、虚拟机、裸金属等多种环境提供统一的身份管理能力。

---

## 架构设计

### 整体架构

```
┌─────────────────────────────────────────────────────────────────┐
│                        SPIRE 架构                                │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    SPIRE Server                             │ │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐        │ │
│  │  │ Registration │ │   CA/KMS     │ │  DataStore   │        │ │
│  │  │    API       │ │  (签发SVID)  │ │ (注册数据)   │        │ │
│  │  └──────────────┘ └──────────────┘ └──────────────┘        │ │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐        │ │
│  │  │ Node Attestor│ │ Workload     │ │   Notifier   │        │ │
│  │  │  (节点证明)  │ │ Registrar    │ │  (事件通知)  │        │ │
│  │  └──────────────┘ └──────────────┘ └──────────────┘        │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              │ gRPC/mTLS                         │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    SPIRE Agent                              │ │
│  │  ┌──────────────┐ ┌──────────────┐ ┌──────────────┐        │ │
│  │  │ Node Attestor│ │  Workload    │ │  Workload    │        │ │
│  │  │              │ │  Attestor    │ │   API        │        │ │
│  │  └──────────────┘ └──────────────┘ └──────────────┘        │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              │ Unix Socket                       │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Workloads                                │ │
│  │  ┌────────┐  ┌────────┐  ┌────────┐  ┌────────┐            │ │
│  │  │ Pod A  │  │ Pod B  │  │ VM App │  │Process │            │ │
│  │  └────────┘  └────────┘  └────────┘  └────────┘            │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 功能 | 说明 |
|:---|:---|:---|
| **SPIRE Server** | 身份颁发中心 | 管理注册条目，签发 SVID |
| **SPIRE Agent** | 节点代理 | 每个节点运行，提供 Workload API |
| **DataStore** | 数据存储 | 存储注册和 CA 数据 |
| **Node Attestor** | 节点证明 | 验证节点身份 |
| **Workload Attestor** | 工作负载证明 | 验证工作负载身份 |

---

## 身份颁发流程

### 节点证明 (Node Attestation)

```
┌──────────────────────────────────────────────────────────────┐
│                    节点证明流程                               │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│   SPIRE Agent                         SPIRE Server           │
│       │                                    │                  │
│       │  1. 节点证明请求                   │                  │
│       │    (包含平台特定证据)              │                  │
│       │───────────────────────────────────►│                  │
│       │                                    │                  │
│       │               2. 验证节点身份       │                  │
│       │                   ┌────────────────┤                  │
│       │                   │ Node Attestor  │                  │
│       │                   │ • AWS IID      │                  │
│       │                   │ • GCP IIT      │                  │
│       │                   │ • Azure MSI    │                  │
│       │                   │ • K8s PSAT     │                  │
│       │                   └────────────────┤                  │
│       │                                    │                  │
│       │  3. 返回节点 SVID                  │                  │
│       │◄───────────────────────────────────│                  │
│       │                                    │                  │
└──────────────────────────────────────────────────────────────┘
```

### 工作负载证明 (Workload Attestation)

```
┌──────────────────────────────────────────────────────────────┐
│                   工作负载证明流程                             │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│   Workload              SPIRE Agent         SPIRE Server     │
│       │                      │                   │            │
│       │  1. 请求 SVID        │                   │            │
│       │  (Unix Socket)       │                   │            │
│       │─────────────────────►│                   │            │
│       │                      │                   │            │
│       │      2. 工作负载证明  │                   │            │
│       │      ┌───────────────┤                   │            │
│       │      │ 检查 PID 属性 │                   │            │
│       │      │ • K8s Pod     │                   │            │
│       │      │ • Docker      │                   │            │
│       │      │ • Unix User   │                   │            │
│       │      └───────────────┤                   │            │
│       │                      │                   │            │
│       │                      │  3. 获取工作负载   │            │
│       │                      │     注册条目       │            │
│       │                      │──────────────────►│            │
│       │                      │                   │            │
│       │                      │  4. 签发 SVID     │            │
│       │                      │◄──────────────────│            │
│       │                      │                   │            │
│       │  5. 返回 SVID        │                   │            │
│       │◄─────────────────────│                   │            │
│       │                      │                   │            │
└──────────────────────────────────────────────────────────────┘
```

---

## 安装部署

### Kubernetes 部署

```bash
# 使用 Helm 安装 SPIRE
helm repo add spiffe https://spiffe.github.io/helm-charts
helm repo update

# 安装 SPIRE Server
helm install spire-server spiffe/spire-server \
  --namespace spire \
  --create-namespace \
  --set trustDomain=example.org

# 安装 SPIRE Agent (DaemonSet)
helm install spire-agent spiffe/spire-agent \
  --namespace spire \
  --set server.address=spire-server.spire.svc:8081
```

### 配置示例

```hcl
# server.conf
server {
    bind_address = "0.0.0.0"
    bind_port = "8081"
    trust_domain = "example.org"
    data_dir = "/run/spire/server/data"
    log_level = "INFO"
    
    ca_ttl = "24h"
    default_x509_svid_ttl = "1h"
    default_jwt_svid_ttl = "5m"
}

plugins {
    DataStore "sql" {
        plugin_data {
            database_type = "sqlite3"
            connection_string = "/run/spire/server/data/datastore.sqlite3"
        }
    }
    
    NodeAttestor "k8s_psat" {
        plugin_data {
            clusters = {
                "demo-cluster" = {
                    service_account_allow_list = ["spire:spire-agent"]
                }
            }
        }
    }
    
    KeyManager "disk" {
        plugin_data {
            keys_path = "/run/spire/server/data/keys.json"
        }
    }
}
```

```hcl
# agent.conf
agent {
    data_dir = "/run/spire/agent/data"
    log_level = "INFO"
    server_address = "spire-server"
    server_port = "8081"
    socket_path = "/run/spire/agent/sockets/spire-agent.sock"
    trust_domain = "example.org"
}

plugins {
    NodeAttestor "k8s_psat" {
        plugin_data {
            cluster = "demo-cluster"
        }
    }
    
    WorkloadAttestor "k8s" {
        plugin_data {
            skip_kubelet_verification = true
        }
    }
    
    KeyManager "memory" {}
}
```

---

## 注册条目管理

### 创建注册条目

```bash
# 注册 Kubernetes 工作负载
spire-server entry create \
    -spiffeID spiffe://example.org/ns/default/sa/webapp \
    -parentID spiffe://example.org/spire/agent/k8s_psat/demo-cluster/xxx \
    -selector k8s:ns:default \
    -selector k8s:sa:webapp

# 注册带有 DNS 名称的条目
spire-server entry create \
    -spiffeID spiffe://example.org/api-server \
    -parentID spiffe://example.org/spire/agent/xxx \
    -selector unix:uid:1000 \
    -dns api.example.org \
    -dns api.example.com
```

### 选择器类型

| 类型 | 选择器示例 | 说明 |
|:---|:---|:---|
| **Kubernetes** | `k8s:ns:default` | 命名空间 |
|  | `k8s:sa:myapp` | ServiceAccount |
|  | `k8s:pod-label:app:web` | Pod 标签 |
| **Docker** | `docker:label:com.example:web` | 容器标签 |
|  | `docker:image_id:sha256:xxx` | 镜像 ID |
| **Unix** | `unix:uid:1000` | 用户 ID |
|  | `unix:gid:1000` | 组 ID |
|  | `unix:path:/usr/bin/app` | 可执行文件路径 |

---

## 高可用部署

### Server 集群架构

```
┌─────────────────────────────────────────────────────────────────┐
│                   SPIRE Server 高可用架构                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│    ┌─────────────────────────────────────────────────────────┐  │
│    │                    Load Balancer                         │  │
│    └─────────────────────┬───────────────────────────────────┘  │
│                          │                                       │
│          ┌───────────────┼───────────────┐                      │
│          ▼               ▼               ▼                      │
│    ┌──────────┐    ┌──────────┐    ┌──────────┐                │
│    │ Server 1 │    │ Server 2 │    │ Server 3 │                │
│    │ (Active) │    │ (Active) │    │ (Active) │                │
│    └────┬─────┘    └────┬─────┘    └────┬─────┘                │
│         │               │               │                       │
│         └───────────────┼───────────────┘                       │
│                         │                                        │
│                         ▼                                        │
│    ┌─────────────────────────────────────────────────────────┐  │
│    │              Shared DataStore                            │  │
│    │              (PostgreSQL / MySQL)                        │  │
│    └─────────────────────────────────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 生产配置

```hcl
# 高可用 Server 配置
server {
    bind_address = "0.0.0.0"
    bind_port = "8081"
    trust_domain = "prod.example.org"
    
    # CA 配置
    ca_ttl = "168h"  # 7 天
    default_x509_svid_ttl = "4h"
    default_jwt_svid_ttl = "5m"
}

plugins {
    DataStore "sql" {
        plugin_data {
            database_type = "postgres"
            connection_string = "host=pg.example.org user=spire dbname=spire sslmode=verify-full"
        }
    }
    
    KeyManager "aws_kms" {
        plugin_data {
            region = "us-west-2"
            key_identifier = "alias/spire-root-key"
        }
    }
    
    UpstreamAuthority "aws_pca" {
        plugin_data {
            region = "us-west-2"
            certificate_authority_arn = "arn:aws:acm-pca:..."
        }
    }
}
```

---

## 使用场景

### 1. 服务网格集成
```yaml
# Istio 使用 SPIRE 作为身份提供者
apiVersion: install.istio.io/v1alpha1
kind: IstioOperator
spec:
  meshConfig:
    trustDomain: example.org
  values:
    global:
      caAddress: spire-server.spire:8081
```

### 2. 数据库身份认证
```go
// 使用 SPIFFE 身份连接数据库
func connectDB() (*sql.DB, error) {
    source, _ := workloadapi.NewX509Source(ctx)
    tlsConfig := tlsconfig.MTLSClientConfig(
        source.GetX509SVID(), 
        source.GetX509BundleForTrustDomain(trustDomain),
        tlsconfig.AuthorizeID(spiffeid.RequireFromString("spiffe://example.org/database")),
    )
    
    connector := &mysql.Config{
        TLSConfig: "spiffe",
    }
    mysql.RegisterTLSConfig("spiffe", tlsConfig)
    return sql.Open("mysql", connector.FormatDSN())
}
```

---

## 参考资源

- [官方文档](https://spiffe.io/docs/latest/spire-about)
- [GitHub Repo](https://github.com/spiffe/spire)
- [CNCF 项目页面](https://www.cncf.io/projects/spire/)
- [Helm Charts](https://github.com/spiffe/helm-charts)
- [SPIRE 插件目录](https://github.com/spiffe/spire/tree/main/doc)

---

**维护者**: Kudig Team | **许可证**: MIT
