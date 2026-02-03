# Domain 5: Networking 网络

## 目录结构

### 1. 网络策略与安全 (01-01)
- [01-networkpolicy-deep-practice.md](01-networkpolicy-deep-practice.md) - NetworkPolicy 深度实践指南

### 2. 服务网格 (02-02)
- [02-service-mesh-deep-dive.md](02-service-mesh-deep-dive.md) - Service Mesh 深度解析与生产实践

### 3. 多集群网络 (03-03)
- [03-multi-cluster-federation.md](03-multi-cluster-federation.md) - 多集群网络联邦与跨集群通信

### 4. DNS 服务发现 (04-04)
- [04-dns-service-discovery-coredns.md](04-dns-service-discovery-coredns.md) - DNS 服务发现与 CoreDNS 调优

### 5. 网络基础架构 (05-07)
- [05-network-architecture-overview.md](05-network-architecture-overview.md) - 网络架构概览
- [06-cni-architecture-fundamentals.md](06-cni-architecture-fundamentals.md) - CNI 架构基础
- [07-cni-plugins-comparison.md](07-cni-plugins-comparison.md) - CNI 插件对比

### 6. CNI 插件详解 (08-10)
- [08-flannel-complete-guide.md](08-flannel-complete-guide.md) - Flannel 完整指南
- [09-terway-advanced-guide.md](09-terway-advanced-guide.md) - Terway 高级指南
- [10-cni-troubleshooting-optimization.md](10-cni-troubleshooting-optimization.md) - CNI 故障排查与优化

### 7. Service 服务 (11-15)
- [11-service-concepts-types.md](11-service-concepts-types.md) - Service 概念与类型
- [12-service-implementation-details.md](12-service-implementation-details.md) - Service 实现细节
- [13-service-topology-aware.md](13-service-topology-aware.md) - Service 拓扑感知
- [14-kube-proxy-modes-performance.md](14-kube-proxy-modes-performance.md) - kube-proxy 模式与性能
- [15-service-advanced-features.md](15-service-advanced-features.md) - Service 高级特性

### 8. DNS 服务发现 (16-20)
- [16-dns-service-discovery.md](16-dns-service-discovery.md) - DNS 服务发现
- [17-coredns-architecture-principles.md](17-coredns-architecture-principles.md) - CoreDNS 架构原理
- [18-coredns-configuration-corefile.md](18-coredns-configuration-corefile.md) - CoreDNS 配置 Corefile
- [19-coredns-plugins-reference.md](19-coredns-plugins-reference.md) - CoreDNS 插件参考
- [20-coredns-troubleshooting-optimization.md](20-coredns-troubleshooting-optimization.md) - CoreDNS 故障排查与优化

### 9. 网络策略与安全 (21-22)
- [21-network-policy-advanced.md](21-network-policy-advanced.md) - NetworkPolicy 高级配置
- [22-network-encryption-mtls.md](22-network-encryption-mtls.md) - 网络加密与 mTLS

### 10. 出站流量管理 (23)
- [23-egress-traffic-management.md](23-egress-traffic-management.md) - 出站流量管理

### 11. 多集群网络 (24)
- [24-multi-cluster-networking.md](24-multi-cluster-networking.md) - 多集群网络

### 12. 网络故障排查 (25)
- [25-network-troubleshooting.md](25-network-troubleshooting.md) - 网络故障排查

### 13. 网络性能调优 (26)
- [26-network-performance-tuning.md](26-network-performance-tuning.md) - 网络性能调优

### 14. Ingress 入站流量 (27-34)
- [27-ingress-fundamentals.md](27-ingress-fundamentals.md) - Ingress 基础
- [28-ingress-controller-deep-dive.md](28-ingress-controller-deep-dive.md) - Ingress Controller 深入解析
- [29-nginx-ingress-complete-guide.md](29-nginx-ingress-complete-guide.md) - Nginx Ingress 完整指南
- [30-ingress-tls-certificate.md](30-ingress-tls-certificate.md) - Ingress TLS 证书
- [31-ingress-advanced-routing.md](31-ingress-advanced-routing.md) - Ingress 高级路由
- [32-ingress-security-hardening.md](32-ingress-security-hardening.md) - Ingress 安全加固
- [33-ingress-monitoring-troubleshooting.md](33-ingress-monitoring-troubleshooting.md) - Ingress 监控与排错
- [34-ingress-production-best-practices.md](34-ingress-production-best-practices.md) - Ingress 生产最佳实践

### 15. Gateway API (35-36)
- [35-gateway-api-overview.md](35-gateway-api-overview.md) - Gateway API 概览
- [36-api-gateway-patterns.md](36-api-gateway-patterns.md) - API 网关模式

## 学习路径建议

### 初级阶段
1. 先学习网络基础架构 (05-07)
2. 掌握 CNI 插件基本概念 (08-10)
3. 理解 Service 核心概念 (11-12)

### 中级阶段
1. 深入学习 DNS 服务发现 (16-20)
2. 掌握 NetworkPolicy 基础配置 (21)
3. 学习 Ingress 入站流量管理 (27-29)

### 高级阶段
1. 网络策略深度实践 (01)
2. 服务网格生产部署 (02)
3. 多集群网络联邦 (03)
4. DNS 性能优化 (04)

## 生产环境最佳实践

### 核心原则
- **零信任安全模型**: 从默认拒绝开始，按需开放最小权限
- **分层防护**: 网络层、服务层、应用层多重安全控制
- **可观测性**: 完善的监控、日志和追踪体系
- **高可用设计**: 多副本、故障转移、健康检查

### 关键技术栈
- **CNI 插件**: Calico/Cilium (支持 NetworkPolicy)
- **服务网格**: Istio/Linkerd (生产环境推荐)
- **DNS**: CoreDNS + NodeLocal DNSCache
- **Ingress**: Nginx Controller + Cert-Manager
- **监控**: Prometheus + Grafana + Loki

## 更新日志

### 2026-02
- 新增 NetworkPolicy 深度实践指南 (01)
- 新增 Service Mesh 生产实践文档 (02)
- 新增多集群网络联邦方案 (03)
- 新增 DNS 服务发现与 CoreDNS 调优 (04)
- 重新整理目录结构，按技术重要性重新排序