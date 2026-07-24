---
title: kube-dns legacy 指南
description: kube-dns 的历史定位、与 CoreDNS 的差异对比、以及从 kube-dns 迁移到 CoreDNS 的完整操作指南。
summary: kube-dns 的历史定位、与 CoreDNS 的差异对比、以及从 kube-dns 迁移到 CoreDNS 的完整操作指南。
category: 网络
tags:
- k8s
- kube-dns
- coredns
- dns
- legacy
- migration
- network
- kubeadm
tier: supporting
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 10min
intent_queries:
- kube-dns 是什么
- kube-dns 与 CoreDNS 的区别
- 如何从 kube-dns 迁移到 CoreDNS
trigger_keywords:
- kube-dns
- coredns
- dns migration
prerequisites:
- kubectl-basics
- coredns-basics
- networking-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# kube-dns legacy 指南

> **状态**：自 Kubernetes 1.13 起，CoreDNS 取代 kube-dns 成为默认 DNS 服务器。kube-dns 仅建议在维护旧集群或特殊兼容性场景下参考。

---

## 1. kube-dns 架构

kube-dns 由三个容器组成，部署为 `kube-system` 命名空间中的 Deployment：

| 容器 | 组件 | 作用 |
|------|------|------|
| `kubedns` | SkyDNS | 服务发现，监听 API Server 变化 |
| `dnsmasq` | dnsmasq | DNS 缓存与转发 |
| `sidecar` | kube-dns-sidecar | 健康检查与指标上报 |

```
Pod (kube-dns)
├── kubedns  ← 监听 Service/Endpoints，生成 DNS 记录
├── dnsmasq  ← 缓存并转发到 kubedns 或上游
└── sidecar  ← /healthcheck 与 Prometheus 指标
```

---

## 2. kube-dns vs CoreDNS

| 维度 | kube-dns | CoreDNS |
|------|----------|---------|
| **默认版本** | K8s ≤ 1.12 | K8s ≥ 1.13 |
| **架构** | 多容器组合（kubedns + dnsmasq + sidecar） | 单进程插件链 |
| **插件扩展** | 有限 | 高度可扩展，支持自定义插件 |
| **性能** | dnsmasq 缓存，但扩展性有限 | 原生缓存、负载均衡、NodeLocal DNSCache |
| **配置方式** | ConfigMap `kube-dns` | ConfigMap `coredns`（Corefile） |
| **内存占用** | 较高 | 较低 |
| **维护状态** | 已弃用，不再新增功能 | 活跃维护 |

---

## 3. 检测集群是否使用 kube-dns

```bash
# 🟢 查看 kube-system 中 DNS 相关 Pod
kubectl get pods -n kube-system -l k8s-app=kube-dns

# 🟢 查看 DNS Service 后端
kubectl get endpoints -n kube-system kube-dns

# 🟢 查看 DNS Deployment
kubectl get deployment -n kube-system kube-dns

# 🟢 查看 DNS 配置
kubectl get configmap -n kube-system kube-dns -o yaml
```

---

## 4. 从 kube-dns 迁移到 CoreDNS

### 4.1 kubeadm 集群迁移

kubeadm 提供了官方迁移工具，可自动将 kube-dns ConfigMap 转换为 Corefile：

```bash
# 🟢 下载并安装迁移工具（在已配置 kubectl 的节点执行）
# 工具路径：https://github.com/coredns/corefile-migration
GO111MODULE=on go install github.com/coredns/corefile-migration/migration@latest

# 🟡 生成 CoreDNS Corefile（基于现有 kube-dns 配置）
kubectl create configmap coredns \
  --from-file=Corefile=./Corefile \
  -n kube-system --dry-run=client -o yaml | kubectl apply -f -
```

### 4.2 手动迁移步骤

```bash
# 1. 🟢 备份 kube-dns 配置
kubectl get configmap kube-dns -n kube-system -o yaml > kube-dns-config-backup.yaml

# 2. 🟡 部署 CoreDNS
kubectl apply -f https://raw.githubusercontent.com/coredns/deployment/master/kubernetes/coredns.yaml.sed
# 注意：根据集群实际 Service CIDR 修改 coredns.yaml.sed 中的 CLUSTER_DNS_IP 和 CLUSTER_DOMAIN

# 3. 🟡 修改 kube-dns Service selector，使其指向 CoreDNS
kubectl patch service kube-dns -n kube-system -p '{"spec":{"selector":{"k8s-app":"kube-dns"}}}'
# 注意：CoreDNS 默认使用 label k8s-app=kube-dns，无需修改

# 4. 🟢 验证 DNS 解析
kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- nslookup kubernetes.default.svc.cluster.local

# 5. 🟡 删除 kube-dns Deployment（确认解析正常后执行）
kubectl delete deployment kube-dns -n kube-system
```

### 4.3 关键配置映射

| kube-dns 配置项 | CoreDNS Corefile 等价配置 |
|----------------|--------------------------|
| `stubDomains` | `forward <domain> <server>` |
| `upstreamNameservers` | `forward . <server>` |
| `federations` | 需手动配置 federation 插件（已弃用） |

示例转换：

```yaml
# kube-dns ConfigMap
data:
  stubDomains: |
    {"corp.example.com": ["10.0.0.1"]}
  upstreamNameservers: |
    ["8.8.8.8", "8.8.4.4"]
```

```
# CoreDNS Corefile
.:53 {
    errors
    health
    kubernetes cluster.local in-addr.arpa ip6.arpa {
       pods insecure
       fallthrough in-addr.arpa ip6.arpa
       ttl 30
    }
    prometheus :9153
    forward . 8.8.8.8 8.8.4.4
    cache 30
    loop
    reload
    loadbalance
}
corp.example.com:53 {
    forward . 10.0.0.1
}
```

---

## 5. 故障排查

| 症状 | 可能原因 | 修复方法 |
|------|----------|----------|
| kube-dns Pod CrashLoop | dnsmasq 配置错误 | 检查 ConfigMap，重置 dnsmasq 参数 |
| DNS 解析慢 | dnsmasq 缓存失效 | 增加缓存 TTL，或迁移到 CoreDNS |
| Service 无法解析 | kubedns 未同步 Endpoints | 重启 kubedns 容器，检查 API Server 连通性 |
| 迁移后解析失败 | Corefile 错误 | 检查 CoreDNS 日志，验证 Corefile 语法 |

```bash
# 🟢 查看 kube-dns 日志
kubectl logs -n kube-system -l k8s-app=kube-dns -c kubedns
kubectl logs -n kube-system -l k8s-app=kube-dns -c dnsmasq

# 🟢 查看 CoreDNS 日志（迁移后）
kubectl logs -n kube-system -l k8s-app=kube-dns -c coredns
```

---

## 6. 检查清单

- [ ] 确认集群是否仍在使用 kube-dns
- [ ] 备份 kube-dns ConfigMap 与 Deployment
- [ ] 制定迁移窗口与回滚方案
- [ ] 转换 stubDomains 与 upstreamNameservers 到 Corefile
- [ ] 部署 CoreDNS 并验证解析
- [ ] 删除 kube-dns Deployment
- [ ] 更新监控告警目标到 CoreDNS

---

## Related

- [[网络/K8s网络核心/13-coredns-architecture-principles.md|CoreDNS 架构原理]]
- [[网络/K8s网络核心/14-coredns-configuration-corefile.md|CoreDNS Corefile 配置]]
- [[网络/K8s网络核心/28-coredns-troubleshooting-optimization.md|CoreDNS 故障排查与优化]]
- [[故障诊断/FTA故障树/list/dns-fta.md|DNS 异常故障树分析]]


<!-- risk-assessed -->
