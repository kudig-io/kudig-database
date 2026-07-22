---
title: ContainerSSH (entities)
description: '## 概述'
summary: 'ContainerSSH 是一个 SSH 服务器，它为每个 SSH 连接动态启动一个容器或 Kubernetes Pod，提供隔离的 shell 环境。用户通过 SSH 连接时，ContainerSSH 调用外部认证服务验证用户身份，然后根据配置为该用户启动专属的容器实例。这种架构非常适合提供安全的沙箱环境、蜜罐系统、CI/CD 执行器或多租户开发环境。'
category: entities
tags:
- k8s
- cncf
- security
- containerssh
- crd
- operator
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
- ContainerSSH 是什么
- 如何 ContainerSSH
trigger_keywords:
- ContainerSSH
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# ContainerSSH

> **CNCF 状态**: Sandbox | **类别**: Security | **主要语言**: Go

## 概述

ContainerSSH 是一个 CNCF 沙箱项目，由 Red Hat 和 GESIS 开发，是一个 SSH 服务器，为每个 SSH 连接启动独立的容器作为用户的 Shell 环境。它将 SSH 访问的隔离性提升到容器级别——每个用户登录后获得一个隔离的容器，而非共享主机环境。ContainerSSH 特别适合教育平台、CI/CD 系统和需要为用户提供安全隔离 Shell 访问的场景。

## Key Features（核心能力）

- **每连接容器**：每个 SSH 会话获得独立的容器，会话结束容器销毁
- **多后端支持**：支持 Kubernetes、Docker 作为容器后端
- **配置热加载**：支持运行时动态更新配置
- **审计日志**：记录完整的命令执行审计日志
- **认证集成**：支持密码、公钥、OAuth2 等多种认证方式
- **安全审计**：支持将用户的 TTY 输出录制为审计视频

## 架构与工作原理

ContainerSSH 由 SSH Server 和后端执行器两部分组成。SSH Server 接收客户端 SSH 连接，通过配置的认证后端（HTTP Auth、Password File、OAuth2）验证用户身份。认证通过后，通过 Backend Provider（K8s/Docker）创建一个容器作为用户的 Shell 环境，将 SSH 的 stdin/stdout/stderr 流转发到容器。会话结束后容器自动销毁。

## K8s 集成

当配置 Kubernetes 后端时，ContainerSSH 在用户登录时创建一个 K8s Pod，将 SSH 会话的 TTY 连接到 Pod 的容器进程。Pod 配置（镜像、资源限制、挂载）可通过 ConfigMap 动态定义。通过 RBAC ServiceAccount 控制 ContainerSSH 创建 Pod 的权限。会话结束后 Pod 自动删除。支持通过 PVC 为用户持久化主目录。

## 生产用例

- **教育平台**：为学生提供隔离的编程环境
- **安全 Shell 访问**：为外部用户提供审计的 SSH 访问
- **CI/CD 构建节点**：通过 SSH 提供 CI/CD 构建环境
- **调试环境**：为开发者提供临时的容器化 Shell 环境

## 安装与配置

```bash
# 🟢 Docker 快速启动
docker run -p 2222:2222 \
  -v $(pwd)/config.yaml:/config.yaml \
  containerssh/containerssh

# 🟢 Helm 部署到 K8s
helm repo add containerssh https://containerssh.github.io/helm-charts/
helm repo update
helm install containerssh containerssh/containerssh \
  -n containerssh --create-namespace \
  --set config.backend=kubernetes \
  --set config.audit.enable=true

# 🟢 验证安装
kubectl get pods -n containerssh
kubectl get svc -n containerssh

# 🟢 测试 SSH 连接
ssh -p 2222 user@containerssh-host

# 🟢 创建认证服务（HTTP Auth Backend）
kubectl apply -f auth-backend.yaml
```

### ContainerSSH 配置示例

```yaml
# config.yaml
log:
  level: info
  format: ljson

ssh:
  hostkeys:
    - /etc/containerssh/host.key
  port: 2222
  ciphers:
    - chacha20-poly1305@openssh.com
    - aes256-gcm@openssh.com

authentication:
  password:
    enabled: true
    passwordBackend:
      url: http://auth-backend:8080/password
  publicKey:
    enabled: true
    publicKeyBackend:
      url: http://auth-backend:8080/pubkey

backend:
  kubernetes:
    host: https://kubernetes.default.svc
    cacert: /var/run/secrets/kubernetes.io/serviceaccount/ca.crt
    bearerTokenPath: /var/run/secrets/kubernetes.io/serviceaccount/token
    namespace: containerssh-sessions
    pod:
      spec:
        containers:
          - name: shell
            image: ubuntu:22.04
            command: ["/bin/bash"]
            resources:
              limits:
                cpu: "1"
                memory: 512Mi
              requests:
                cpu: 100m
                memory: 128Mi
        serviceAccountName: containerssh-session
        automountServiceAccountToken: false

audit:
  enable: true
  format: ljson
  storage: file
  file:
    directory: /var/log/containerssh/audit
  intercept:
    stdin:
      enabled: true
    stdout:
      enabled: true
```

### RBAC 配置

```yaml
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: containerssh-pod-manager
  namespace: containerssh-sessions
rules:
  - apiGroups: [""]
    resources: ["pods", "pods/exec", "pods/log"]
    verbs: ["create", "get", "list", "delete"]
  - apiGroups: [""]
    resources: ["configmaps"]
    verbs: ["get", "list"]
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: containerssh-pod-manager
  namespace: containerssh-sessions
roleRef:
  apiGroup: rbac.authorization.k8s.io
  kind: Role
  name: containerssh-pod-manager
subjects:
  - kind: ServiceAccount
    name: containerssh
    namespace: containerssh
```

## 运维操作

```bash
# 🟢 查看活跃 SSH 会话
kubectl get pods -n containerssh-sessions

# 🟢 查看 ContainerSSH 日志
kubectl logs -n containerssh -l app=containerssh --tail=100

# 🟢 查看审计日志
kubectl exec -n containerssh deploy/containerssh -- ls /var/log/containerssh/audit/

# 🟡 强制断开用户会话（删除 Pod）
kubectl delete pod -n containerssh-sessions -l containerssh.io/username=<user>

# 🟡 更新容器镜像配置
kubectl edit configmap containerssh-config -n containerssh
kubectl rollout restart deploy/containerssh -n containerssh

# 🔴 清除所有活跃会话
kubectl delete pods -n containerssh-sessions --all
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| SSH 连接被拒绝 | 认证服务不可用 | `kubectl logs -l app=containerssh` | 检查 Auth Backend URL |
| Pod 创建失败 | RBAC 权限不足 | `kubectl get events -n containerssh-sessions` | 检查 Role/RoleBinding |
| 会话立即断开 | 容器镜像缺少 shell | 查看 Pod 日志 | 确保镜像包含 /bin/bash |
| 审计日志缺失 | 存储卷未挂载 | `kubectl describe pod containerssh` | 检查 PVC/Volume 配置 |

```bash
# 排查流程
# 1. 检查 ContainerSSH 服务状态
kubectl get pods -n containerssh
kubectl get svc -n containerssh

# 2. 检查认证服务连通性
kubectl exec -n containerssh deploy/containerssh -- wget -qO- http://auth-backend:8080/health

# 3. 检查会话 Pod 事件
kubectl get events -n containerssh-sessions --sort-by='.lastTimestamp' | tail -20

# 4. 检查 RBAC 权限
kubectl auth can-i create pods -n containerssh-sessions --as=system:serviceaccount:containerssh:containerssh
```

## 生产案例

### 案例1：大学编程教学平台
- **场景**：500+ 学生同时通过 SSH 登录进行 Linux 编程实验
- **方案**：ContainerSSH + K8s 后端；每学生获得独立 Pod（Ubuntu + GCC/Python）；资源限制防止单用户耗尽集群；审计日志记录所有操作
- **效果**：环境一致性 100%，学生无法影响其他用户，环境准备时间从 30min 降到 5s

### 案例2：安全审计 Shell 访问
- **场景**：金融企业需要为外部审计员提供受控的 SSH 访问
- **方案**：ContainerSSH + 只读容器镜像；完整 TTY 录制；会话超时自动断开；所有操作可回放审计
- **效果**：通过安全审计，审计员访问完全可追溯，零安全风险

## 对比替代方案

| 维度 | ContainerSSH | Teleport | 传统 SSH | Gotty/Wetty |
|------|-------------|----------|---------|------------|
| 隔离级别 | 容器级 | 主机级 | 主机级 | 进程级 |
| 审计能力 | 强(TTY录制) | 强 | 弱 | 弱 |
| 多租户 | 原生 | 支持 | 无 | 无 |
| 资源控制 | K8s limits | 无 | cgroup | 无 |
| 部署复杂度 | 中 | 高 | 低 | 低 |

## 检查清单

- [ ] ContainerSSH 已部署且 Pod Running
- [ ] 认证服务已配置且可达
- [ ] RBAC 权限已正确配置（Pod 创建/删除）
- [ ] 容器镜像包含所需工具和 shell
- [ ] 资源限制已配置（CPU/内存）
- [ ] 审计日志已启用且存储已配置
- [ ] 会话超时已设置
- [ ] 网络策略已配置（限制 Pod 出站访问）

## Related

- [[easegress]] — Easegress
- [[perses]] — Perses
- [[tremor]] — Tremor
- [[drasi]] — Drasi
- [[kubernetes]] — Kubernetes (CNCF Graduated)

- containerssh
- [[实体/cncf-security.md|[[CNCF 安全与合规项目全景|CNCF 安全与合规项目全景]]]] — Cross-reference
- index/etcd-index|etcd 知识图谱索引]]


<!-- risk-assessed -->
