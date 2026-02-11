# Kubernetes CoreDNS 全栈进阶培训 (从入门到专家)

> **适用版本**: Kubernetes v1.28 - v1.32 | **文档类型**: 全栈技术实战指南
> **目标受众**: 网络初学者、SRE、架构师
> **核心原则**: 掌握服务发现入口、极致性能调优、深度故障排查

---

## 🔰 第一阶段：快速入门与基础概念

### 1.1 什么是 CoreDNS？
*   **定义**: 集群内部的“电话簿”。
*   **功能**: 当 Pod 想要访问 `my-service` 时，它会去问 CoreDNS：“`my-service` 的 IP 是什么？”。
*   **默认集成**: 从 v1.13 开始成为 K8S 默认 DNS 方案。

### 1.2 DNS 解析规则 (新手必知)
*   **全限定域名 (FQDN)**: `<service>.<namespace>.svc.cluster.local`。
*   **短域名**: 同一 Namespace 下直接写 `service-name` 即可。

---

## 📘 第二阶段：核心架构与深度原理 (Deep Dive)

### 2.1 架构设计
*   **插件化**: 基于中间件模式。常用的有 `kubernetes`, `forward`, `cache`, `prometheus`。
*   **Informer 机制**: 实时同步集群 Service 状态，保持解析结果最新。

### 2.2 NodeLocal DNSCache
*   **解决痛点**: 绕过 conntrack 竞态导致的 5s 超时丢包问题，极大地提升了解析性能和稳定性。

---

## ⚡ 第三阶段：生产部署与极致调优

### 2.1 性能调优
*   **ndots 陷阱**: 默认 `ndots:5` 会导致外部域名解析产生多次无效查询。建议优化为 `2` 或使用 FQDN。
*   **Cache 配置**: 增加缓存 TTL，减少对上游 DNS 的压力。

### 2.2 监控指标
*   `coredns_dns_request_duration_seconds`: 解析延迟。
*   `coredns_dns_responses_total{rcode="SERVFAIL"}`: 解析错误率。

---

## 🛠️ 第四阶段：故障诊断与 SRE 运维 (SRE Ops)

### 3.1 诊断工具
*   `dig @<dns-ip> <domain>`: 手动探测。
*   `kubectl logs -n kube-system -l k8s-app=kube-dns`: 检查系统日志。

### 🏆 SRE 运维红线
*   *红线 1: 生产环境必须配置 Replica >= 2 且跨节点部署。*
*   *红线 2: 必须监控解析延迟，P99 建议 < 50ms。*
*   *红线 3: 任何 Corefile 变更必须经过灰度测试。*
