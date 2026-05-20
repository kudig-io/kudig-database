---
title: ContainerSSH
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- docker
- rbac
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
- ContainerSSH 是什么
- 如何 ContainerSSH
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- ContainerSSH
- cncf
- landscape
---

# ContainerSSH

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://containerssh.io/ |
| **GitHub** | https://github.com/ContainerSSH/ContainerSSH |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

ContainerSSH 是一个 SSH 服务器，它为每个 SSH 连接动态启动一个容器或 Kubernetes Pod，提供隔离的 shell 环境。用户通过 SSH 连接时，ContainerSSH 调用外部认证服务验证用户身份，然后根据配置为该用户启动专属的容器实例。这种架构非常适合提供安全的沙箱环境、蜜罐系统、CI/CD 执行器或多租户开发环境。

### 核心特性

- **动态容器**: 每个 SSH 会话启动独立容器，会话结束后容器销毁
- **多后端支持**: 支持 Docker、Kubernetes 和 Podman 作为容器运行后端
- **外部认证**: 通过 Webhook 调用外部认证服务，支持 LDAP、OAuth 等
- **审计日志**: 完整记录用户会话的所有输入输出，支持回放
- **安全隔离**: 用户之间完全隔离，每个用户获得独立的容器环境
- **SFTP 支持**: 支持 SFTP 协议进行文件传输

---

## 架构设计

```
┌──────────────────────────────────────────────────────┐
│                      SSH Clients                       │
│                                                       │
│  ┌─────────┐  ┌─────────┐  ┌─────────┐              │
│  │ User A  │  │ User B  │  │ User C  │              │
│  │ (ssh)   │  │ (ssh)   │  │ (sftp)  │              │
│  └────┬────┘  └────┬────┘  └────┬────┘              │
└───────┼────────────┼────────────┼────────────────────┘
        │            │            │
        └────────────┼────────────┘
                     │ SSH Protocol
              ┌──────▼──────┐
              │ ContainerSSH │
              │   Server     │
              │              │
              │ ┌──────────┐ │
              │ │ Auth     │ │ ──► External Auth Webhook
              │ │ Handler  │ │     (LDAP/OAuth/Custom)
              │ └──────────┘ │
              │ ┌──────────┐ │
              │ │ Config   │ │ ──► External Config Webhook
              │ │ Provider │ │     (用户自定义镜像)
              │ └──────────┘ │
              │ ┌──────────┐ │
              │ │ Audit    │ │ ──► 审计日志存储
              │ │ Logger   │ │     (S3/文件/自定义)
              │ └──────────┘ │
              └──────┬───────┘
                     │
      ┌──────────────┼──────────────┐
      │              │              │
┌─────▼────┐  ┌──────▼─────┐ ┌─────▼──────┐
│  Docker   │  │ Kubernetes │ │  Podman    │
│  Backend  │  │  Backend   │ │  Backend   │
└─────┬─────┘  └──────┬─────┘ └─────┬──────┘
      │               │              │
┌─────▼─────┐  ┌──────▼──────┐ ┌────▼──────┐
│Container A│  │   Pod B     │ │Container C│
│(User A)   │  │  (User B)   │ │(User C)   │
└───────────┘  └─────────────┘ └───────────┘
```

---

## 快速开始

### 安装

```bash
# 下载二进制
curl -LO https://github.com/ContainerSSH/ContainerSSH/releases/latest/download/containerssh-linux-amd64
chmod +x containerssh-linux-amd64
sudo mv containerssh-linux-amd64 /usr/local/bin/containerssh

# 或使用 Docker
docker pull containerssh/containerssh:latest
```

### 基本配置

```yaml
# config.yaml
ssh:
  hostkeys:
    - /etc/containerssh/ssh_host_rsa_key
  listen: "0.0.0.0:2222"

# Docker 后端配置
backend: docker
docker:
  connection:
    host: unix:///var/run/docker.sock
  execution:
    container:
      image: ubuntu:22.04

# 认证配置 (Webhook)
auth:
  url: "http://auth-server:8080/auth"
  timeout: 10s

# 配置服务 (Webhook，可选)
configserver:
  url: "http://config-server:8080/config"

# 审计日志
audit:
  enable: true
  format: binary
  storage: file
  file:
    directory: /var/log/containerssh/audit
```

### 生成 SSH Host Key

```bash
ssh-keygen -t rsa -b 4096 -f /etc/containerssh/ssh_host_rsa_key -N ""
```

### 启动服务

```bash
containerssh --config /etc/containerssh/config.yaml
```

### 认证 Webhook 服务示例

```go
// auth-server.go
package main

import (
    "encoding/json"
    "net/http"
)

type AuthRequest struct {
    Username  string `json:"username"`
    RemoteAddress string `json:"remoteAddress"`
    ConnectionID  string `json:"connectionId"`
    PublicKey     string `json:"publicKey,omitempty"`
    Password      string `json:"password,omitempty"`
}

type AuthResponse struct {
    Success bool `json:"success"`
}

func main() {
    http.HandleFunc("/auth", func(w http.ResponseWriter, r *http.Request) {
        var req AuthRequest
        json.NewDecoder(r.Body).Decode(&req)
        
        // 验证逻辑
        success := validateUser(req.Username, req.Password)
        
        json.NewEncoder(w).Encode(AuthResponse{Success: success})
    })
    http.ListenAndServe(":8080", nil)
}
```

---

## 高级功能

### Kubernetes 后端

```yaml
backend: kubernetes
kubernetes:
  connection:
    host: https://kubernetes.default.svc
    cacertFile: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
    bearerTokenFile: /var/run/secrets/kubernetes.io/serviceaccount/token
  pod:
    metadata:
      namespace: user-sandboxes
    spec:
      containers:
        - name: shell
          image: ubuntu:22.04
          command: ["/bin/bash"]
          resources:
            limits:
              cpu: "500m"
              memory: "512Mi"
```

### 用户自定义镜像 (Config Webhook)

```go
// config-server.go - 根据用户返回不同镜像
type ConfigRequest struct {
    Username string `json:"username"`
}

type ConfigResponse struct {
    Config struct {
        Docker struct {
            Execution struct {
                Container struct {
                    Image string `json:"image"`
                } `json:"container"`
            } `json:"execution"`
        } `json:"docker"`
    } `json:"config"`
}

func configHandler(w http.ResponseWriter, r *http.Request) {
    var req ConfigRequest
    json.NewDecoder(r.Body).Decode(&req)
    
    // 根据用户名选择镜像
    image := "ubuntu:22.04"
    if req.Username == "developer" {
        image = "custom-dev-env:latest"
    }
    
    resp := ConfigResponse{}
    resp.Config.Docker.Execution.Container.Image = image
    json.NewEncoder(w).Encode(resp)
}
```

### 审计日志和回放

```bash
# 审计日志自动记录会话
# 使用 containerssh-auditlog-decoder 回放
containerssh-auditlog-decoder \
  --input /var/log/containerssh/audit/session-123.bin \
  --output session-123.cast

# 使用 asciinema 播放
asciinema play session-123.cast
```

### 蜜罐配置

```yaml
# 蜜罐环境：记录所有操作但限制危险行为
backend: docker
docker:
  execution:
    container:
      image: honeypot-os:latest
      network: none  # 禁用网络
      capDrop:
        - ALL  # 移除所有 capabilities
    idleCommand:
      - "/bin/bash"

audit:
  enable: true
  interceptInput: true   # 记录所有输入
  interceptOutput: true  # 记录所有输出
```

---

## 与其他方案对比

| 特性 | ContainerSSH | Teleport | Bastillion | Guacamole |
|:---|:---|:---|:---|:---|
| 动态容器 | 每会话独立容器 | 静态主机 | 静态主机 | 静态主机 |
| 后端 | Docker/K8s/Podman | SSH | SSH | SSH/VNC/RDP |
| 认证 | Webhook 集成 | SSO/RBAC | LDAP | 内置/LDAP |
| 审计 | 完整会话录制 | 完整审计 | 有限 | 有限 |
| 隔离 | 容器隔离 | 无 | 无 | 无 |
| SFTP | 支持 | 支持 | 有限 | 支持 |

---

## 最佳实践

1. **安全基线镜像**: 为沙箱环境准备安全加固的基础镜像，移除不必要的工具
2. **资源限制**: 配置容器 CPU/内存限制，防止资源滥用
3. **网络隔离**: 对不需要网络的场景禁用容器网络
4. **审计保留**: 审计日志存储到持久化存储（S3/NFS），保留足够时间
5. **会话超时**: 配置 idle timeout 自动清理闲置会话

---

## 参考资源

- [ContainerSSH 官方文档](https://containerssh.io/docs/)
- [ContainerSSH GitHub](https://github.com/ContainerSSH/ContainerSSH)
- [审计日志格式](https://containerssh.io/docs/audit/)
- [Webhook API 规范](https://containerssh.io/docs/api/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
