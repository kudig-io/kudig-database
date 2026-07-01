---
title: "Skill: NetworkPolicy 不生效的诊断和修复"
category: skill
tags: ["skill", "domain-10", "visibility/public"]
sources: ["KUDIG Gap Analysis 2026-05-21"]
created: 2026-05-21
updated: 2026-05-21
last_updated: 2026-05-21
status: reviewed
---

# Skill: NetworkPolicy 不生效的诊断和修复

## 问题描述
NetworkPolicy 已创建但流量未被正确拦截或放行，表现为应用间通信异常、跨命名空间访问失败，或策略存在但无实际效果。远程顾问模式下需通过用户提供的输出来定位 CNI 支持性、规则语义及标签匹配问题。

## 常见症状
- Pod 间通信超时或连接被拒绝，但 Service 和 Endpoints 正常
- 已创建 deny-all 策略，但 Pod 仍可被任意访问
- 跨命名空间流量被阻断，尽管已创建允许规则
- 应用使用 UDP（如 DNS）时通信失败
- 启用 egress 限制后 Pod 无法解析域名或访问外部服务

## 诊断步骤

### 步骤1: 确认 CNI 是否支持 NetworkPolicy
```bash
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l k8s-app=cilium
kubectl get pods -n kube-system -l app=flannel
```
> 如果无法执行，替代方案：询问集群管理员使用的 CNI 插件名称和版本。若使用 Flannel，则 NetworkPolicy 完全不生效。

### 步骤2: 检查策略规则与标签匹配
```bash
kubectl get networkpolicy -n <namespace> -o yaml
kubectl get pods -n <namespace> --show-labels
```
> 重点核对 `podSelector` 是否匹配目标 Pod 标签，`namespaceSelector` 匹配的是来源命名空间的标签而非名称。

### 步骤3: 验证端口、协议与方向配置
```bash
kubectl describe networkpolicy <policy-name> -n <namespace>
```
> 确认 `policyTypes` 包含所需方向（Ingress/Egress），端口协议是否为 TCP/UDP/SCTP，并检查 egress 中是否放行了 CoreDNS 的 53/UDP 和 53/TCP。

## 修复措施
- **CNI 不支持**：迁移至 Calico、Cilium 等支持 NetworkPolicy 的 CNI
- **标签不匹配**：修正 `podSelector` 或 `namespaceSelector` 使其与实际标签一致
- **端口/协议遗漏**：补充 UDP 端口规则，显式声明 WebSocket、gRPC 所需的 TCP 端口
- **DNS 被阻断**：在 egress 规则中放行 CoreDNS（53/UDP、53/TCP）
- **方向缺失**：将 `Egress` 加入 `policyTypes` 并补充出站允许规则

## 预防性措施
- 启用 NetworkPolicy 前确认 CNI 支持矩阵，避免在 Flannel 环境部署策略
- 使用命名空间标签而非名称进行跨命名空间放行，减少策略变更频率
- 在测试环境预先验证 deny-all + allow-list 组合是否符合预期连通性

## 相关概念

- [[concepts/network-policy.md|Network Policy]] — NetworkPolicy 规则语义、CNI 实现与标签匹配原理
- [[concepts/cni-networking-model.md|CNI 网络模型]] — Kubernetes 容器网络接口与网络策略支持矩阵
- [[concepts/security-defense-depth.md|纵深防御]] — 网络安全分层防御策略与零信任实践
