---
title: "Skill: Headless Service DNS 解析失败的诊断和修复"
category: skill
tags: ["skill", "domain-10", "visibility/public"]
sources: ["KUDIG Gap Analysis 2026-05-21"]
created: 2026-05-21
updated: 2026-05-21
status: reviewed
---

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
```bash
kubectl get service <service-name> -n <namespace> -o jsonpath='{.spec.clusterIP}{.spec.selector}{.spec.publishNotReadyAddresses}'
```
> 确认 `clusterIP` 为 `"None"`，`selector` 与目标 Pod 标签匹配。对于需要提前发现所有成员的集群初始化场景，检查 `publishNotReadyAddresses` 是否为 `true`。
> 如果无法执行，替代方案：请用户提供 Service 的 YAML 定义文件内容，或从集群管理控制台截图 Service 详情。

### 步骤2: 验证 CoreDNS 及 DNS 配置
```bash
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl exec <pod-name> -n <namespace> -- cat /etc/resolv.conf
kubectl exec <pod-name> -n <namespace> -- nslookup <service-name>.<namespace>.svc.cluster.local
```
> 确认 CoreDNS Pod 正常运行，`/etc/resolv.conf` 中的搜索域包含 `<namespace>.svc.cluster.local`，nameserver 指向 ClusterDNS IP。

### 步骤3: 检查网络策略及 Pod 就绪状态
```bash
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

## 相关概念

- [[concepts/headless-service|Headless Service]] — Headless Service DNS 解析与 StatefulSet 网络标识
- [[concepts/cni-networking-model|CNI 网络模型]] — Kubernetes 容器网络接口与 Pod 间通信原理
- [[concepts/service-networking|Service 网络模型]] — Kubernetes Service 核心概念与流量转发机制
