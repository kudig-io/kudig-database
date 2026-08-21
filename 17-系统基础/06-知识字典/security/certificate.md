---
title: 证书
description: Certificate（证书）是 Kubernetes 中用于 TLS 加密通信的数字凭证。集群内部组件之间的通信、API Server
  的 HTTPS 端点和...
summary: Certificate（证书）是 Kubernetes 中用于 TLS 加密通信的数字凭证。集群内部组件之间的通信、API Server 的 HTTPS
  端点和...
category: dictionary
tags:
- k8s
- glossary
- security
- certificate
- tls
tier: core
created: '2026-06-24'
last_updated: 2026-06
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 证书 是什么
- Certificate 详解
trigger_keywords:
- 证书
- Certificate
- dictionary
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 证书

> **英文名**: Certificate

## 概述

Certificate（证书）是 Kubernetes 中用于 TLS 加密通信的数字凭证。集群内部组件之间的通信、API Server 的 HTTPS 端点和 Ingress TLS 终止都依赖证书。

## 核心概念/原理

### 证书用途

- **组件间通信**：API Server、etcd、kubelet 之间的 mTLS。
- **API Server HTTPS**：对外提供 HTTPS 服务。
- **Ingress TLS**：终止 HTTPS 流量。
- **Webhook**：准入 Webhook 的 TLS 认证。

### 证书管理方式

- **kubeadm 自动生成**：集群初始化时自动生成所有证书。
- **cert-manager**：CNCF 项目，自动化证书生命周期管理。
- **手动管理**：使用 cfssl 或 openssl 生成。

## 关键机制或特性

- Kubernetes 使用 PKI（Public Key Infrastructure）管理证书。
- CA（Certificate Authority）是根证书，签发其他证书。
- 证书有有效期，需要定期轮转。
- cert-manager 支持 Let's Encrypt、Vault 等多种 Issuer。

## 使用场景与最佳实践

- 使用 cert-manager 自动化证书管理（推荐）。
- 监控证书过期时间，设置过期前告警。
- 为 Ingress TLS 使用 Let's Encrypt 免费证书。
- 定期检查证书链完整性。

## 架构深度解析

### Kubernetes TLS 证书体系

```
┌──────────────────────────────────────────────────────────────┐
│  集群 PKI 根（自签 CA）                                        │
│  ├─ CA 证书：/etc/kubernetes/pki/ca.crt + ca.key（控制面）     │
│  │                                                            │
│  ├─ 服务端证书（apiserver 对外暴露）：                          │
│  │  ├─ kube-apiserver 证书（SAN：集群 IP/DNS/域名）            │
│  │  ├─ etcd 证书（server/client 双向）                        │
│  │  ├─ kubelet 证书（server：10250 端口）                     │
│  │  └─ 聚合层证书（front-proxy-ca 独立签发）                  │
│  │                                                            │
│  ├─ 客户端证书：                                               │
│  │  ├─ admin/kube-controller-manager/kube-scheduler           │
│  │  └─ kubelet 客户端（请求 apiserver）                       │
│  │                                                            │
│  └─ 应用层证书：                                               │
│     ├─ Ingress TLS（Secret 类型 kubernetes.io/tls）           │
│     ├─ Webhook 证书（caBundle 注入）                           │
│     └─ Service Mesh mTLS（自签，独立体系）                    │
└──────────────────────────────────────────────────────────────┘
```

### 源码关键路径（kubernetes/kubernetes）

| 模块 | 文件路径 | 职责 |
| --- | --- | --- |
| 证书生成 | `cmd/kubeadm/app/phases/certs/` | kubeadm 证书签发与轮换 |
| 证书轮换 | `pkg/kubelet/certificate/` | kubelet 客户端证书自动轮换 |
| TLS 配置 | `staging/src/k8s.io/apiserver/pkg/server/options/` | apiserver 证书加载 |
| Secret 类型 | `pkg/registry/core/secret/` | tls 类型 Secret 校验 |

### 流程步骤

1. kubeadm 初始化时生成集群 CA 与各组件证书（默认有效期 1 年）。
2. apiserver 启动时加载服务端证书，客户端用 CA 校验服务端身份。
3. 组件间双向认证：apiserver 与 etcd、kubelet 之间互相校验证书。
4. kubelet 客户端证书临近过期时自动申请轮换（CSR 流程）。
5. 应用层 Ingress TLS 由 cert-manager 等签发并存入 TLS Secret。

## 生产案例

### 案例 1：CA 证书过期导致集群组件集体失联

| 时间 | 事件 |
| --- | --- |
| T-90d | 集群由 kubeadm v1.19 搭建，证书有效期 1 年，未配置自动轮换 |
| T+0 | kubelet 客户端证书过期，节点状态全部 NotReady |
| T+1h | apiserver 到 etcd 的客户端证书过期，控制面只读 |
| T+3h | 定位：全部组件证书过期，kubeadm 证书体系未启用自动续期 |
| T+6h | 用 `kubeadm certs renew all` 续期并重启组件，集群恢复 |

- **根因分析**：控制面证书有效期默认 1 年，kubeadm 高版本自动续期依赖定时任务（kube-controller-manager 与 kubeadm 的 CSR 签发），未升级或手动部署的集群易踩坑。
- **修复命令**：
```bash
# 1. 检查证书过期时间（只读）
kubeadm certs check-expiration
# 2. 批量续期（🔴 高风险：需维护窗口，续期后必须重启组件）
kubeadm certs renew all
# 3. 重启控制面静态 Pod（kubeadm 方式）
systemctl restart kubelet
kubectl -n kube-system rollout restart deployment/coredns
# 4. 验证
kubeadm certs check-expiration
kubectl get nodes  # 🟢 只读
```

### 案例 2：Ingress 证书过期导致线上服务大面积 TLS 报错

| 时间 | 事件 |
| --- | --- |
| T-11month | 手动申请 1 年期证书并创建 TLS Secret |
| T+0 | 证书到期，移动端 App 全部报 SSL 握手失败 |
| T+1h | 监控告警触发，定位为 Ingress Secret 证书过期 |
| T+2h | 临时续期并更新 Secret，服务恢复 |
| T+1w | 接入 cert-manager 自动签发与续期，问题根除 |

- **根因分析**：手工证书生命周期管理靠人肉记忆，缺少过期监控与自动续期；TLS Secret 更新后 Ingress 控制器也需要热加载（nginx 控制器默认 1-2s 重载）。
- **修复命令**：
```bash
# 1. 快速定位过期证书（只读）
kubectl get secret -A -o json | jq -r '.items[] | select(.type=="kubernetes.io/tls") | .metadata.namespace + "/" + .metadata.name' | while read s; do
  kubectl get secret $s -o json | jq -r '.data."tls.crt"' | base64 -d | openssl x509 -noout -enddate 2>/dev/null
done
# 2. 更新证书（🟡 中风险：切换新证书）
kubectl create secret tls my-tls --cert=tls.crt --key=tls.key -n prod --dry-run=client -o yaml | kubectl apply -f -
# 3. 验证 Ingress 生效
kubectl get ingress -A -o jsonpath='{.items[*].spec.tls[].secretName}'  # 🟢 只读
```

## 对比评测

| 维度 | 自签 CA（kubeadm） | cert-manager（Let's Encrypt） | 企业 PKI（内部 CA） | Service Mesh mTLS |
| --- | --- | --- | --- | --- |
| 签发对象 | 控制面组件 | Ingress/应用证书 | 全场景 | 服务间流量 |
| 自动化 | 部分（kubelet 自动） | 全自动（ACME） | 流程审批 | 全自动（Istio） |
| 有效期 | 1 年（可配置） | 90 天 | 按策略 | 24h 轮换 |
| 适用场景 | 集群内部 | 公网应用 | 合规企业 | 服务网格 |

**选型建议**：集群控制面用 kubeadm 证书体系并确保自动续期；应用证书统一 cert-manager；企业合规需求接内部 PKI。

## 故障排查速查

| 现象 | 可能原因 | 处理命令 |
| --- | --- | --- |
| x509: certificate has expired | 证书过期 | `kubeadm certs check-expiration`；`openssl x509 -noout -dates` |
| x509: certificate signed by unknown authority | CA 不匹配/客户端缺 CA | 检查 kubeconfig 的 certificate-authority-data |
| 组件证书不匹配（SAN 错误） | 集群 IP/域名变更 | `kubeadm certs renew` 后用新 SAN 重建 |
| Ingress 502/525 | 后端证书过期或 secret 缺失 | `kubectl get secret -n <ns>` 核对 |
| kubelet 证书轮换失败 | CSR 被拒绝/权限不足 | `kubectl get csr` 检查 Pending |

## 生产部署清单

- [ ] 控制面证书纳入 `kubeadm certs check-expiration` 月度巡检
- [ ] 确认 kubelet 客户端证书自动轮换开启（`rotateCertificates: true`）
- [ ] 应用证书统一 cert-manager 管理，禁用手工证书
- [ ] 证书过期监控告警（提前 30/14/7/1 天）
- [ ] 高可用集群确保各控制面副本证书一致（同一 CA 签发）

## 升级决策点

| 级别 | 条件 | 动作 |
| --- | --- | --- |
| P0 | 任意控制面证书 30 天内过期 | 立即续期并重启组件，评估 CA 轮换预案 |
| P1 | 应用证书手工管理且无监控 | 迁移 cert-manager 并配置自动续期 |
| P2 | 集群 IP/DNS 变更历史未记录 | 审计 SAN 覆盖并更新证书 |

## 面试要点

1. **Q：Kubernetes 集群里有哪些证书，分别保护什么？**
   A：四类：apiserver 服务端证书（对外 TLS）、etcd 双向证书（存储链路）、kubelet 证书（节点通道）、聚合层证书（metrics 等扩展 API）。另有应用层 Ingress/Webhook/Service Mesh 证书。控制面证书由 kubeadm 自签 CA 签发，kubelet 客户端证书支持自动轮换。
2. **Q：证书过期前如何避免集群故障？**
   A：三层防线：一是自动轮换（kubelet `rotateCertificates`、kubeadm 版本 ≥1.19 的自动续期）；二是巡检（`kubeadm certs check-expiration` 月度执行）；三是监控告警（提前 30 天预警证书有效期）。生产事故多为手工集群未配置轮换或监控缺失。
3. **Q：apiserver 证书需要包含哪些 SAN？**
   A：集群 IP（Service 10.96.0.1）、节点 IP、控制面域名（lb.k8s.example.com）、本地主机名/127.0.0.1 等；kubeconfig 中的 server 地址必须被 SAN 覆盖，否则客户端报证书校验失败。变更集群入口地址后需重新签发包含新 SAN 的证书。

## 运维要点

- 巡检：`kubeadm certs check-expiration` 纳入月度巡检与升级前置检查。
- 证书分层：控制面 CA 与应用 CA 分离，控制面密钥严格保护。
- 监控：apiserver 启动日志与 Prometheus `x509` 相关指标联动告警。
- 变更管理：集群 VIP/DNS 变更前先评估证书 SAN 覆盖。
- 排障入口：先看错误类型（expired/unknown authority/SAN mismatch），再决定续期还是重建。

## 参考链接

- [Certificate - Official Documentation](https://kubernetes.io/docs/tasks/tls/managing-tls-in-a-cluster/)

## Related

- [[17-系统基础/06-知识字典/security/rbac.md|Rbac]]
- [[17-系统基础/06-知识字典/security/role.md|Role]]
- [[17-系统基础/06-知识字典/security/clusterrole.md|Clusterrole]]
- [[17-系统基础/06-知识字典/security/rolebinding.md|Rolebinding]]
- [[17-系统基础/06-知识字典/security/clusterrolebinding.md|Clusterrolebinding]]


<!-- risk-assessed -->
