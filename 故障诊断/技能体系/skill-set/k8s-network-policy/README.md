---
title: 'Skill: NetworkPolicy 不生效的诊断和修复'
summary: 'Skill: NetworkPolicy 不生效的诊断和修复：NetworkPolicy 已创建但流量未被正确拦截或放行，表现为应用间通信异常、跨命名空间访问失败，或策略存在但无实际效果。远程顾问模式下需通过用户提供的输出来定位
  CNI 支持性、规则语义及标签匹配问题。'
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
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl get pods -n kube-system -l k8s-app=cilium
kubectl get pods -n kube-system -l app=flannel
```
> 如果无法执行，替代方案：询问集群管理员使用的 CNI 插件名称和版本。若使用 Flannel，则 NetworkPolicy 完全不生效。

### 步骤2: 检查策略规则与标签匹配
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get networkpolicy -n <namespace> -o yaml
kubectl get pods -n <namespace> --show-labels
```
> 重点核对 `podSelector` 是否匹配目标 Pod 标签，`namespaceSelector` 匹配的是来源命名空间的标签而非名称。

### 步骤3: 验证端口、协议与方向配置
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
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

## 生产案例

### 案例 1：Flannel 环境下 NetworkPolicy 完全不生效

**背景**：团队在 Flannel CNI 集群中部署了 deny-all NetworkPolicy，但安全扫描发现所有 Pod 间流量仍未被拦截。

**根因**：Flannel 不支持 NetworkPolicy 执行，策略对象被 apiserver 接受但无任何组件执行。需迁移到 Calico/Cilium 或添加 Calico policy-only 模式。

**修复**：
``` bash
# 🟡 中风险：安装 Calico policy 组件（与 Flannel 共存）
kubectl apply -f https://raw.githubusercontent.com/projectcalico/calico/manifests/canal.yaml
# 等待 Canal Pod 就绪后验证策略生效
kubectl run test --rm -it --image=busybox -- wget -qO- --timeout=3 http://<target-pod-ip>
```

### 案例 2：Egress 策略阻断 DNS 导致全业务不可用

**背景**：应用 egress deny-all 策略后未放行 CoreDNS，所有 Pod 无法解析域名。

**根因**：egress 规则中未包含 UDP 53 端口放行规则，且未指定 CoreDNS 的 namespaceSelector。

**修复**：
``` bash
# 🟡 中风险：添加 DNS egress 放行规则
kubectl apply -f - <<EOF
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-dns-egress
  namespace: prod
spec:
  podSelector: {}
  policyTypes: [Egress]
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          kubernetes.io/metadata.name: kube-system
    ports:
    - protocol: UDP
      port: 53
    - protocol: TCP
      port: 53
EOF
```

## 升级决策点

- **P0（立即处理）**：NetworkPolicy 误配置导致核心业务流量被阻断
- **P1（30分钟内）**：策略未生效但安全合规要求必须生效，存在安全风险
- **P2（下一工作日）**：非关键命名空间的策略调整，不影响业务

## 面试要点

1. **Q: NetworkPolicy 的默认行为是什么？**
   A: 默认情况下 Pod 允许所有入站和出站流量（全开放）。一旦某个 Pod 被任何 NetworkPolicy 的 podSelector 匹配，该 Pod 就变为“白名单模式”，仅允许被策略显式放行的流量。未被任何策略匹配的 Pod 仍保持全开放。

2. **Q: 哪些 CNI 支持 NetworkPolicy？实现原理有何不同？**
   A: Calico（默认用 Linux iptables，新版本支持 eBPF）、Cilium（eBPF 实现，L3/L4/L7）、Weave（NPC 组件用 iptables）、Antrea（OVS 流表）。Flannel 不支持。Calico iptables 模式性能随策略数线性下降，eBPF 模式性能更优。

3. **Q: 如何测试 NetworkPolicy 是否符合预期？**
   A: ① 使用 `kubectl run test --rm -it` 创建测试 Pod 验证连通性；② 使用 Cilium 的 `cilium policy trace` 命令模拟策略执行；③ 使用 Calico 的 `calicoctl get networkpolicy -o yaml` 检查渲染结果；④ 在 CI/CD 中使用网络策略测试框架（如 Cyclonus）自动化验证。

## 相关概念

- [[概念/network-policy.md|Network Policy]] — NetworkPolicy 规则语义、CNI 实现与标签匹配原理
- [[概念/cni-networking-model.md|CNI 网络模型]] — Kubernetes 容器网络接口与网络策略支持矩阵
- [[概念/security-defense-depth.md|纵深防御]] — 网络安全分层防御策略与零信任实践

## Related

- [[visibility-public|#visibility/public Hub]] — tag hub


<!-- risk-assessed -->
