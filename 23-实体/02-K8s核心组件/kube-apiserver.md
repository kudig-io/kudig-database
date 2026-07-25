---
title: kube-apiserver
description: kube-apiserver — Kubernetes 生产运维知识库
summary: kube-apiserver — Kubernetes 生产运维知识库
category: entities
tags:
- k8s
- apiserver
- control-plane
- api
- authentication
- authorization
- etcd
- kubelet
- scheduler
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kube-apiserver 是什么
- 如何 kube-apiserver
trigger_keywords:
- kube-apiserver
prerequisites:
- kubectl-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-apiserver

## Role

kube-apiserver is the single entry point for all Kubernetes operations. Every component (kubectl, controllers, [[kubelet|kubelet]], schedulers) communicates exclusively through the API Server. It is stateless, enabling horizontal scaling.

## Request Processing Pipeline

Each API request flows through:
1. **Authentication**: Verify identity (X.509 certs, tokens, OIDC, webhook)
2. **Authorization**: Check permissions (RBAC, ABAC, Node, webhook)
3. **Admission Control**: Mutating webhooks (modify) then Validating webhooks (reject)
4. **Schema Validation**: Ensure object structure is valid
5. **Persistence**: Write to etcd

## API Priority and Fairness (APF)

APF prevents API Server overload by classifying requests into priority levels and assigning flow schemas:
- Exempt: System-critical requests (no queuing)
- Higher priority: Control plane operations
- Lower priority: Bulk operations (list, watch)

## Key Configuration

| Parameter | Purpose | Production Default |
|-----------|---------|-------------------|
| `--etcd-servers` | etcd cluster endpoints | https://etcd1:2379,etcd2:2379,etcd3:2379 |
| `--max-requests-inflight` | Concurrent read requests | 400 (large clusters) |
| `--max-mutating-requests-inflight` | Concurrent write requests | 200 (large clusters) |
| `--event-ttl` | Event retention time | 1h (reduce etcd load) |
| `--encryption-provider-config` | Secret encryption at rest | KMS v2 or aescbc |

## Ports

| Port | Protocol | Purpose |
|------|----------|--------|
| 6443 | HTTPS | Main API endpoint |
| 8080 | HTTP | Insecure port (deprecated, disabled by default) |

## 运维操作

```bash
# 🟢 查看 API Server 健康状态
kubectl get --raw /healthz
kubectl get --raw /livez
kubectl get --raw /readyz

# 🟢 查看 API Server 指标
kubectl get --raw /metrics | grep apiserver_request_total
kubectl get --raw /metrics | grep apiserver_request_duration_seconds

# 🟢 查看当前请求负载
kubectl get --raw /metrics | grep apiserver_current_inflight_requests

# 🟢 查看 APF 状态
kubectl get --raw /metrics | grep apiserver_flowcontrol
kubectl get flowschema
kubectl get prioritylevelconfiguration

# 🟢 查看审计日志
# 静态 Pod 部署时查看日志
journalctl -u kubelet | grep apiserver
# 或查看审计日志文件
cat /var/log/kubernetes/audit.log | jq 'select(.verb=="delete")'

# 🟡 调整请求并发限制（修改静态 Pod manifest）
# /etc/kubernetes/manifests/kube-apiserver.yaml:
#   --max-requests-inflight=600
#   --max-mutating-requests-inflight=300

# 🟡 启用/禁用 Admission Plugin
# --enable-admission-plugins=NodeRestriction,PodSecurity,ResourceQuota
# --disable-admission-plugins=ServiceAccount

# 🔴 重启 API Server（静态 Pod 方式）
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
sleep 5
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方法 |
|------|----------|----------|----------|
| API Server 无响应 | etcd 不可用 | `etcdctl endpoint health` | 恢复 etcd 集群 |
| 401 Unauthorized | 证书过期 | `openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates` | 更新证书 |
| 403 Forbidden | RBAC 配置错误 | `kubectl auth can-i <verb> <resource> --as=<user>` | 修复 Role/RoleBinding |
| 请求延迟高 | APF 队列积压 | `kubectl get --raw /metrics \| grep apiserver_flowcontrol` | 调整 FlowSchema |
| 502/504 错误 | 后端 etcd 慢 | `etcdctl endpoint status` | 优化 etcd 磁盘 I/O |
| Webhook 超时 | Admission Webhook 无响应 | `kubectl get validatingwebhookconfigurations` | 设置 timeoutSeconds/failurePolicy |

```bash
# 排查流程
# 1. 检查 API Server 进程状态
crictl ps | grep kube-apiserver
kubectl get componentstatuses

# 2. 检查 etcd 连接
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/apiserver-etcd-client.crt \
  --key=/etc/kubernetes/pki/apiserver-etcd-client.key \
  endpoint health

# 3. 检查证书有效期
for cert in /etc/kubernetes/pki/*.crt; do
  echo "$cert: $(openssl x509 -in $cert -noout -enddate)"
done

# 4. 检查请求延迟
kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket | head -20

# 5. 检查 Admission Webhook 状态
kubectl get mutatingwebhookconfigurations -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.webhooks[0].clientConfig.service.name}{"\n"}{end}'
```

## 生产案例

### 案例1：API Server 高可用优化
- **场景**：5000 节点集群，API Server 请求延迟 P99 > 2s
- **方案**：调整 APF 配置，将批量 List 请求降级到低优先级；增加 max-requests-inflight 到 800；部署 3 副本 API Server + 负载均衡
- **效果**：P99 延迟从 2s 降到 200ms，控制平面可用性 99.99%

### 案例2：Secret 加密存储
- **场景**：安全审计要求 etcd 中的 Secret 必须加密存储
- **方案**：配置 EncryptionConfiguration 使用 KMS v2 provider；集成云厂商 KMS 服务；滚动重新加密所有现有 Secret
- **效果**：通过安全审计，Secret 在 etcd 中以密文存储，密钥由 KMS 托管

## 检查清单

- [ ] API Server 多副本部署（>= 3）
- [ ] etcd 连接健康且延迟 < 10ms
- [ ] 所有 PKI 证书有效期 > 30天
- [ ] APF FlowSchema 已配置且无队列积压
- [ ] Admission Webhook 已设置 timeout 和 failurePolicy
- [ ] 审计日志已启用且保留策略已配置
- [ ] Secret 加密存储已启用
- [ ] 监控告警已配置（延迟/错误率/队列深度）

## Related

- [[23-实体/02-K8s核心组件/kubelet.md|kubelet]] — kubelet
- [[etcd]] — etcd
- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[22-概念/05-安全/security-defense-depth.md|security-defense-depth]] — Defense-in-Depth Security
- [[22-概念/01-核心架构/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[etcd|etcd]]
- [[22-概念/05-安全/security-defense-depth.md|Defense-in-Depth Security]]
- [[operator-pattern|Operator Pattern]]
- [[22-概念/01-核心架构/Kubernetes Core Concepts.md|Kubernetes Core Concepts]]


<!-- risk-assessed -->
