---
title: 'Skill: Headless Service DNS 解析失败的诊断和修复'
summary: 'Skill: Headless Service DNS 解析失败的诊断和修复：Headless Service 的 DNS 解析返回异常结果，导致
  StatefulSet Pod 无法通过稳定网络标识互相发现，或客户端无法获取后端 Pod IP 列表。远程顾问模式下需从 Service 定义、DNS 配置和网络策略三个层面排查。'
category: skill
tags:
- skill
- domain-10
- visibility/public
tier: supporting
sources:
- KUDIG Gap Analysis 2026-05-21
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Skill: Headless Service DNS 解析失败的诊断和修复

## 问题描述
Headless Service 的 DNS 解析返回异常结果，导致 StatefulSet Pod 无法通过稳定网络标识互相发现，或客户端无法获取后端 Pod IP 列表。远程顾问模式下需从 Service 定义、DNS 配置和网络策略三个层面排查。

## 常见症状
- `nslookup <pod-name>.<service-name>` 返回 NXDOMAIN 或超时
- StatefulSet Pod 启动时无法解析同伴地址，集群初始化失败
- dig 命令返回的 A 记录数量与 Ready Pod 数量不一致
- 仅部分 Pod 能解析 Headless Service，其他 Pod 解析失败
- 解析结果包含已删除或未就绪 Pod 的 IP

## 诊断步骤

### 步骤1: 确认 Headless Service 定义正确
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.clusterIP}{.spec.selector}{.spec.publishNotReadyAddresses}'
```
> 确认 `clusterIP` 为 `"None"`，`selector` 与目标 Pod 标签匹配。对于需要提前发现所有成员的集群初始化场景，检查 `publishNotReadyAddresses` 是否为 `true`。
> 如果无法执行，替代方案：请用户提供 Service 的 YAML 定义文件内容，或从集群管理控制台截图 Service 详情。

### 步骤2: 验证 CoreDNS 及 DNS 配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl exec <pod-name> -n <namespace> -- cat /etc/resolv.conf
kubectl exec <pod-name> -n <namespace> -- nslookup <service-name>.<namespace>.svc.cluster.local
```
> 确认 CoreDNS Pod 正常运行，`/etc/resolv.conf` 中的搜索域包含 `<namespace>.svc.cluster.local`，nameserver 指向 ClusterDNS IP。

### 步骤3: 检查网络策略及 Pod 就绪状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n <namespace> -l <selector> --field-selector=status.phase=Running
kubectl get networkpolicy -n <namespace>
```
> 确认后端 Pod 处于 Running 且 Ready 状态，没有 NetworkPolicy 阻断 Pod 与 CoreDNS 之间的 53/UDP 通信。

## 修复措施
- **Service 定义错误**：将 `clusterIP` 显式设置为 `"None"`，修正 `selector` 与 Pod 标签匹配
- **CoreDNS 异常**：重启 CoreDNS Deployment，检查 ConfigMap 中的转发配置和插件链
- **DNS 搜索域缺失**：检查 Pod 的 `dnsPolicy` 是否为 `ClusterFirst`，必要时手动配置 `dnsConfig`
- **Pod 未就绪**：排查 Pod 未通过 readinessProbe 的原因，或在 Service 中设置 `publishNotReadyAddresses: true`
- **NetworkPolicy 阻断**：在 NetworkPolicy 中放行 CoreDNS（53/UDP、53/TCP）及 Pod 间通信所需端口
- **缓存污染**：在客户端 Pod 中执行 `nscd -i hosts` 或重启应用进程刷新 DNS 缓存

## 预防性措施
- StatefulSet 创建前预先验证 Headless Service 的 selector 与 Pod 模板标签一致
- 在 CI 流水线中加入 DNS 解析冒烟测试，覆盖 Pod FQDN 与 SRV 记录
- 对需要提前发现的集群，显式启用 `publishNotReadyAddresses: true`

## 生产案例

### 案例 1：StatefulSet Pod FQDN 解析失败导致集群初始化失败

**背景**：某 Elasticsearch 集群使用 StatefulSet + Headless Service，新节点加入时无法解析 `es-0.es-headless.default.svc.cluster.local`，集群无法形成。

**根因**：Headless Service 的 selector 中 `app: elasticsearch` 与 StatefulSet Pod 模板标签 `app: es` 不匹配，导致 DNS A 记录未生成。

**修复**：
``` bash
# 🟡 中风险：修正 Headless Service selector
kubectl patch svc es-headless -n default -p '{"spec":{"selector":{"app":"es"}}}'
# 验证 DNS 解析
kubectl run test --rm -it --image=busybox -- nslookup es-0.es-headless.default.svc.cluster.local
```

### 案例 2：publishNotReadyAddresses 未设置导致滚动更新死锁

**背景**：Cassandra StatefulSet 滚动更新时，新 Pod 启动需要发现其他节点，但其他 Pod 也在重启中（Not Ready），Headless Service 不返回 Not Ready Pod 的 IP，导致所有节点互相等待。

**修复**：
``` bash
# 🟡 中风险：启用 publishNotReadyAddresses
kubectl patch svc cassandra-headless -n prod -p '{"spec":{"publishNotReadyAddresses":true}}'
```

## 升级决策点

- **P0（立即处理）**：Headless Service DNS 完全失效，StatefulSet 集群无法形成/通信
- **P1（30分钟内）**：部分 Pod FQDN 解析失败，影响数据同步但服务仍可用
- **P2（下一工作日）**：仅新 Pod 发现异常，当前集群运行正常

## 面试要点

1. **Q: Headless Service 与普通 Service 的区别是什么？**
   A: Headless Service 设置 `clusterIP: None`，不分配虚拟 IP，不经过 kube-proxy 转发。DNS 查询直接返回后端 Pod 的 A 记录（每个 Pod 一个 IP），适用于 StatefulSet 需要稳定网络标识的场景（如数据库集群、消息队列）。

2. **Q: StatefulSet 的 DNS 记录格式是什么？**
   A: Pod 记录：`<pod-name>.<svc-name>.<namespace>.svc.cluster.local`。SRV 记录：`_<port-name>._<protocol>.<svc-name>.<namespace>.svc.cluster.local`。普通 Headless Service 查询返回所有匹配 Pod 的 A 记录。

3. **Q: publishNotReadyAddresses 的作用和适用场景？**
   A: 默认 Headless Service 仅返回 Ready Pod 的 DNS 记录。设置 `publishNotReadyAddresses: true` 后，即使 Pod 未就绪也会返回其 IP。适用于：分布式系统初始化发现（如 ZooKeeper、Cassandra 节点互相发现）、滚动更新期间保持集群通信。

## 相关概念

- [[概念/headless-service.md|Headless Service]] — Headless Service DNS 解析与 StatefulSet 网络标识
- [[概念/cni-networking-model.md|CNI 网络模型]] — Kubernetes 容器网络接口与 Pod 间通信原理
- [[概念/service-networking.md|Service 网络模型]] — Kubernetes Service 核心概念与流量转发机制

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
