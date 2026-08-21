---
title: 密钥
description: Secret 是 Kubernetes 中用于存储敏感数据的 API 资源，如密码、Token、TLS 证书等。它提供了比 ConfigMap
  更强的安全控制机...
summary: Secret 是 Kubernetes 中用于存储敏感数据的 API 资源，如密码、Token、TLS 证书等。它提供了比 ConfigMap 更强的安全控制机...
category: dictionary
tags:
- k8s
- glossary
- secret
- security
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 密钥 是什么
- Secret 详解
trigger_keywords:
- 密钥
- Secret
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 密钥

> **英文名**: Secret

## 概述

Secret 是 Kubernetes 中用于存储敏感数据的 API 资源，如密码、Token、TLS 证书等。它提供了比 ConfigMap 更强的安全控制机制。

## 核心概念/原理

### Secret 类型

- **Opaque**：通用 Secret（默认类型）。
- **kubernetes.io/tls**：TLS 证书和私钥。
- **kubernetes.io/dockerconfigjson**：容器镜像仓库认证。
- **kubernetes.io/basic-auth**：基本认证凭据。
- **kubernetes.io/ssh-auth**：SSH 认证密钥。
- **kubernetes.io/service-account-token**：ServiceAccount Token。

### 安全措施

- etcd 加密：启用 EncryptionConfiguration 加密 Secret 数据。
- RBAC：限制对 Secret 资源的访问权限。
- 外部密钥管理：集成 Vault、AWS Secrets Manager 等。

## 关键机制或特性

- Secret 数据以 Base64 编码存储（非加密），需配合 etcd 加密。
- Secret 大小限制 1MB。
- 使用 `stringData` 字段可以用明文方式创建 Secret（自动转换为 Base64）。
- Volume 挂载的 Secret 更新会自动传播。

## 使用场景与最佳实践

- 生产环境使用 External Secrets Operator 集成外部密钥管理系统。
- 启用 etcd 加密确保 Secret 数据安全。
- 通过 RBAC 严格控制 Secret 的访问权限。
- 避免将 Secret 硬编码在 YAML 文件中并提交到 Git。

## 架构深度解析

### Secret 生命周期与数据流

```
┌──────────────────────────────────────────────────────────────┐
│  用户 / 控制器                                                  │
│   │  ① kubectl create secret / apply -f（stringData 明文）     │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ API Server（apiserver）                                  │  │
│  │ ├─ 准入：限制 namespace 级 / RBAC 鉴权                    │  │
│  │ ├─ 编码：明文 → Base64 存储于 etcd                        │  │
│  │ └─ 审计：记录谁读取/修改（kube-apiserver audit）          │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ② 读取：Kubelet 通过 SA Token 访问 Secret API            │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ Kubelet（kubelet）                                        │  │
│  │ ├─ Volume 模式：写入 secret 卷 → 挂载到容器路径            │  │
│  │ ├─ envFrom 模式：注入环境变量（更新不生效）                │  │
│  │ └─ imagePullSecrets：传递给容器运行时拉取镜像              │  │
│  └─────────────────────────────────────────────────────────┘  │
│   │  ③ 容器内应用读取（一次性/定期同步）                       │
│   ▼                                                           │
│  ┌─────────────────────────────────────────────────────────┐  │
│  │ 应用（Pod）                                               │  │
│  │ ├─ 文件读取：/var/run/secrets/...（文件模式自动更新）      │  │
│  │ └─ 环境变量：进程启动时快照（不自动更新）                  │  │
│  └─────────────────────────────────────────────────────────┘  │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| Secret 存储 | `pkg/registry/core/secret/strategy.go` | 创建/更新校验、Base64 解码验证 |
| 准入控制 | `plugin/pkg/admission/limits/` | 命名空间 Secret 计数限制 |
| Kubelet 卷插件 | `pkg/volume/secret/secret.go` | Secret 卷挂载与内容同步 |
| env 注入 | `pkg/kubelet/envvars/` | envFrom 环境变量注入逻辑 |

### 流程步骤

1. 用户通过 API 创建 Secret，`stringData` 字段明文在请求中传输，存储时统一转为 `data`（Base64）。
2. 等距加密（KMS/静态加密）在 etcd 层执行，Base64 仅编码非加密。
3. Pod 调度后 Kubelet 用自身 SA Token 向 API Server 拉取 Secret 内容。
4. Volume 模式按 `optional` 与版本号机制实现内容更新（kubelet 周期性同步）。
5. 应用通过文件读取时实现热更新，通过环境变量读取则需重建 Pod。

## 生产案例

### 案例 1：etcd 未启用静态加密导致 Secret 明文泄露

| 时间 | 事件 |
| --- | --- |
| T+0 | 安全团队例行扫描发现备份的 etcd 快照中包含数据库密码明文（Base64 可解码） |
| T+2h | 确认集群从未开启 `EncryptionConfiguration`，所有 Secret 以 Base64 存储于磁盘 |
| T+6h | 审查备份权限，确认快照曾外泄给第三方运维团队 |
| T+24h | 批量轮换全部数据库密码、API Token、TLS 私钥 |
| T+48h | 启用 KMS 静态加密并验证 etcd 重写完成 |

- **根因分析**：Kubernetes Secret 的 Base64 编码是数据表示而非加密，任何能读取 etcd 快照的主体都可直接解码；备份链路缺乏加密与访问控制。
- **修复命令**：
```bash
# 1. 生成 KMS 加密配置（以 AWS KMS 为例）
cat <<EOF > /etc/kubernetes/encryption-config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources: ["secrets"]
    providers:
      - kms:
          name: aws-kms
          endpoint: unix:///var/run/kms-plugin.sock
      - aescbc:
          keys:
            - name: key1
              secret: $(openssl rand -base64 32)
      - identity: {}
EOF
# 2. 重启 kube-apiserver 并验证
kubectl get secrets --all-namespaces -o json | jq '.items[].data' | head -5  # 🟢 只读
# 3. 强制重写存量 Secret 使其进入加密状态
kubectl get secrets --all-namespaces -o json | kubectl replace -f -
```

### 案例 2：Secret 轮转后应用未感知导致全线故障

| 时间 | 事件 |
| --- | --- |
| T+0 | DBA 轮换数据库密码并更新 Secret |
| T+30min | 支付服务批量报 401，订单接口失败率 100% |
| T+1h | 定位：应用通过环境变量读取密码，环境变量在容器启动时固化 |
| T+2h | 滚动重启全部关联 Deployment，服务恢复 |

- **根因分析**：环境变量注入的 Secret 不会随 Secret 更新而刷新，轮转后旧值仍在运行；未建立"轮转 → 重启工作负载"的联动机制。
- **修复命令**：
```bash
# 1. 更新 Secret
kubectl create secret generic db-creds --from-literal=password='new-pass' --dry-run=client -o yaml | kubectl apply -f -
# 2. 重启依赖该 Secret 的工作负载（🟡 中风险：滚动更新）
kubectl rollout restart deployment/payment -n prod
# 3. 若使用 Volume 挂载，可启用 subPath 注意项或等待 kubelet 同步周期（60~90s）
kubectl rollout status deployment/payment -n prod  # 🟢 只读
```

## 对比评测

| 维度 | Kubernetes Secret | External Secrets Operator | Vault（原生） |
| --- | --- | --- | --- |
| 存储位置 | etcd（Base64，可加密） | 外部 KMS，K8s 内仅存引用 | Vault 存储后端 |
| 轮转能力 | 手动/控制器驱动 | 自动（按外部源同步） | 动态密钥自动轮转 |
| 审计能力 | API Server audit | 同步状态可观测 | 完整审计日志 |
| 适用规模 | 中小集群、简单场景 | 多云/多集群统一管理 | 高安全合规场景 |
| 复杂度 | 低 | 中 | 高 |

**选型建议**：默认使用原生 Secret + 静态加密；需要自动轮转与多云统一时引入 External Secrets Operator；金融/合规场景采用 Vault。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| Pod 无法挂载 Secret | 引用不存在 / 命名空间错误 | `kubectl get secret -n <ns>`；`kubectl describe pod` 检查事件 |
| 文件内容未更新 | kubelet 同步周期 / subPath 限制 | 等待 60~90s；避免 subPath 挂载整文件 |
| envFrom 注入失败 | Secret 键名非法（如含 `-`） | 检查键名合法性，改用 volume 挂载 |
| Secret 无法删除 | 被 Pod 引用（terminating 卡住） | `kubectl get pods --all-namespaces` 查找引用方 |
| 拉取私有镜像 401 | imagePullSecrets 未关联 SA | `kubectl patch sa default -p '{"imagePullSecrets":[{"name":"regcred"}]}'` |

## 生产部署清单

- [ ] 启用 etcd 静态加密（aescbc 或 KMS），并验证存量 Secret 已重写
- [ ] 配置 RBAC：仅授权应用自身 SA 读取所需 Secret
- [ ] 审计策略：开启 `secrets` 资源读取审计，对接 SIEM
- [ ] 建立轮转 SOP：更新 → 重启依赖工作负载 → 验证
- [ ] 禁止 Secret 提交 Git：启用 Secret 扫描（gitleaks/trivy config）

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 集群未启用静态加密且暴露公网 | 立即启用 EncryptionConfiguration 并轮换全部密钥 |
| P1 | Secret 通过环境变量注入且需定期轮转 | 迁移到 Volume 挂载或引入 ESO 自动同步 |
| P2 | 存在明文硬编码的 Secret YAML | 清理仓库、轮换密钥、接入扫描器 |

## 面试要点

1. **Q：Kubernetes Secret 为什么说不安全？如何加固？**
   A：Secret 默认仅 Base64 编码存储于 etcd，任何可读 etcd 的主体均可解码。加固措施：启用静态加密（KMS/aescbc）、严格 RBAC、开启审计、限制访问 Secret 的 API 权限、必要时引入外部密钥管理。
2. **Q：Secret 以 Volume 和环境变量注入有什么区别？**
   A：Volume 注入后 kubelet 会周期性同步内容（约 60~90s），应用可通过重读文件实现热更新；环境变量在容器启动时固化，更新 Secret 后必须重建 Pod 才生效。因此需要热更新的场景应优先使用 Volume。
3. **Q：如何实现 Secret 的自动轮转？**
   A：方案有三：一是外部密钥源变化后由 External Secrets Operator 同步并触发工作负载滚动更新；二是使用 Vault 动态密钥（数据库短时凭证）；三是自定义控制器监听 Secret 变更并 `rollout restart`。核心是保证"轮转-同步-重启-验证"闭环。

## 运维要点

- 监控：跟踪 `kube_secret_info` 数量与等距加密状态，Secret 数超限（1MB）会拒绝写入。
- 备份：etcd 备份必须视为敏感数据，与 Secret 同级别保护。
- 容量规划：大量 Secret 会增加 API Server 与 etcd 压力，避免为每个 Pod 单独创建 Secret。
- 密钥分层：静态加密密钥独立于业务密钥管理，轮换周期建议 ≤90 天。
- 排障入口：先看 Pod 事件 → kubelet 日志 → API Server 审计，确定是挂载、同步还是权限问题。

## 参考链接

- [Secret - Official Documentation](https://kubernetes.io/docs/concepts/configuration/secret/)

## Related

[[17-系统基础/06-知识字典/configuration/secrets.md|Secrets]]


<!-- risk-assessed -->
