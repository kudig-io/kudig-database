---
title: 'Domain 5: Networking 网络'
description: '# Domain 5: Networking 网络'
summary: '# Domain 5: Networking 网络'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- prometheus
- grafana
- istio
- envoy
- cilium
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 网络工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 'Domain 5: Networking 网络 是什么'
- '如何 Domain 5: Networking 网络'
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- Domain
- '5:'
- Networking
- 网络
- networking
prerequisites:
- kubectl-basics
- networking-basics
- service-mesh-basics
- prometheus-basics
- monitoring-basics
- ebpf-basics
- cilium-basics
- cni-basics
- tls-basics
- logging-basics
cross_refs:
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: domain
  path: ../domain-03-networking-traffic/
  label: '相关知识域: domain-03-networking-traffic'
- type: domain
  path: ../domain-06-observability/
  label: '相关知识域: domain-06-observability'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Domain 5: Networking 网络

> **文档数量**: 47 篇 | **最后更新**: 2026-05 | **状态**: 生产环境就绪 (专家级审查完成) | **适用版本**: Kubernetes 1.25 - 1.33+

## 目录结构

### 1. 网络基础架构 (01-05)
- [01-network-architecture-overview.md](01-network-architecture-overview.md) - 网络架构概览与核心组件
- [02-cni-architecture-fundamentals.md](02-cni-architecture-fundamentals.md) - CNI 架构基础与核心原理
- [03-cni-plugins-comparison.md](03-cni-plugins-comparison.md) - CNI 插件对比与选型指南
- [04-flannel-complete-guide.md](04-flannel-complete-guide.md) - Flannel 完整指南
- [05-terway-advanced-guide.md](05-terway-advanced-guide.md) - Terway 高级指南

### 2. Service 服务 (06-10)
- [06-service-concepts-types.md](06-service-concepts-types.md) - Service 概念与类型深度解析
- [07-service-implementation-details.md](07-service-implementation-details.md) - Service 实现细节
- [08-service-topology-aware.md](08-service-topology-aware.md) - Service 拓扑感知
- [09-kube-proxy-modes-performance.md](09-kube-proxy-modes-performance.md) - kube-proxy 模式与性能优化
- [10-service-advanced-features.md](10-service-advanced-features.md) - Service 高级特性

### 3. DNS 服务发现 (11-15)
- [11-dns-service-discovery-coredns.md](11-dns-service-discovery-coredns.md) - DNS 服务发现与 CoreDNS 调优
- [12-dns-service-discovery.md](12-dns-service-discovery.md) - DNS 服务发现
- [13-coredns-architecture-principles.md](13-coredns-architecture-principles.md) - CoreDNS 架构原理
- [14-coredns-configuration-corefile.md](14-coredns-configuration-corefile.md) - CoreDNS 配置 Corefile
- [15-coredns-plugins-reference.md](15-coredns-plugins-reference.md) - CoreDNS 插件参考

### 4. 网络策略与安全 (16-18)
- [16-networkpolicy-deep-practice.md](16-networkpolicy-deep-practice.md) - NetworkPolicy 深度实践指南
- [17-network-policy-advanced.md](17-network-policy-advanced.md) - NetworkPolicy 高级配置
- [18-network-encryption-mtls.md](18-network-encryption-mtls.md) - 网络加密与 mTLS

### 5. Ingress 入站流量 (19-26)
- [19-ingress-fundamentals.md](19-ingress-fundamentals.md) - Ingress 基础概念与核心原理
- [20-ingress-controller-deep-dive.md](20-ingress-controller-deep-dive.md) - Ingress Controller 深入解析
- [21-nginx-ingress-complete-guide.md](21-nginx-ingress-complete-guide.md) - Nginx Ingress 完整指南
- [22-ingress-tls-certificate.md](22-ingress-tls-certificate.md) - Ingress TLS 证书管理
- [23-ingress-advanced-routing.md](23-ingress-advanced-routing.md) - Ingress 高级路由配置
- [24-ingress-security-hardening.md](24-ingress-security-hardening.md) - Ingress 安全加固
- [25-ingress-monitoring-troubleshooting.md](25-ingress-monitoring-troubleshooting.md) - Ingress 监控与排错
- [26-ingress-production-best-practices.md](26-ingress-production-best-practices.md) - Ingress 生产最佳实践

### 6. 网络故障排查 (27-29)
- [27-cni-troubleshooting-optimization.md](27-cni-troubleshooting-optimization.md) - CNI 故障排查与优化
- [28-coredns-troubleshooting-optimization.md](28-coredns-troubleshooting-optimization.md) - CoreDNS 故障排查与优化
- [29-egress-traffic-management.md](29-egress-traffic-management.md) - 出站流量管理

### 7. 高级主题 (30-39)
- [30-service-mesh-deep-dive.md](30-service-mesh-deep-dive.md) - Service Mesh 深度解析与生产实践 (Istio Ambient Mesh)
- [31-multi-cluster-federation.md](31-multi-cluster-federation.md) - 多集群网络联邦与跨集群通信 (Karmada & MCS)
- [32-multi-cluster-networking.md](32-multi-cluster-networking.md) - 多集群网络
- [33-network-troubleshooting.md](33-network-troubleshooting.md) - 网络故障排查
- [34-network-performance-tuning.md](34-network-performance-tuning.md) - 网络性能调优
- [35-gateway-api-overview.md](35-gateway-api-overview.md) - Gateway API 概览 (GAMMA & Mesh)
- [36-api-gateway-patterns.md](36-api-gateway-patterns.md) - API 网关模式 (Envoy Gateway)
- [37-terway-resources-crud-operations.md](37-terway-resources-crud-operations.md) - Terway 实例 CRUD 操作指南
- [38-terway-gc-mechanism.md](38-terway-gc-mechanism.md) - Terway GC (垃圾回收) 机制详解
- [39-csi-cni-version-matrix.md](39-csi-cni-version-matrix.md) - CSI/CNI 版本兼容矩阵

### 8. Terway 阿里云 CNI 专题 (40-47)
- [40-terway-product-overview.md](40-terway-product-overview.md) - Terway 产品概览：定位、版本历史、5 种网络模式、CNI 对比
- [41-terway-architecture-deep-dive.md](41-terway-architecture-deep-dive.md) - Terway 架构原理：控制面/数据面、IPAM、CRD 模型、安全体系
- [42-terway-usage-guide.md](42-terway-usage-guide.md) - Terway 使用指南：安装配置、NetworkPolicy、固定 IP、容量规划
- [43-terway-crd-operations.md](43-terway-crd-operations.md) - Terway CRD 深度操作：PodENI/NodeNetworking/PodNetworking/ReservedIP 全量 CRUD
- [44-terway-operations-manual.md](44-terway-operations-manual.md) - Terway 运维手册：健康检查、GC 调优、告警规则、升级回滚、巡检清单
- [45-terway-testing-validation.md](45-terway-testing-validation.md) - Terway 测试验证：端到端测试套件、ENI 压测、性能基准、MTU 测试
- [46-terway-performance-tuning.md](46-terway-performance-tuning.md) - Terway 性能调优：内核调优、eBPF 加速、IP 池预热、生产基线
- [47-terway-troubleshooting-fta.md](47-terway-troubleshooting-fta.md) - Terway 故障树速查：FTA 全景图、6 大问题类别、32 条错误信息目录

> **深度参考**: 关于各主流 API 网关产品（Higress、APISIX、Kong、Envoy Gateway、Traefik）的企业级实践，请参考 [Domain-98: 云原生 API 网关](../domain-40-cloud-native-api-gateway)。


## 学习路径建议

### 初级阶段 (01-10)
1. 先学习网络架构概览 (01)
2. 掌握 CNI 基础概念 (02-05)
3. 理解 Service 核心概念 (06-10)

### 中级阶段 (11-20)
1. 深入学习 DNS 服务发现 (11-15)
2. 掌握 NetworkPolicy 基础配置 (16-17)
3. 学习 Ingress 入站流量管理 (19-21)

### 高级阶段 (21-47)
1. 网络策略深度实践 (16)
2. 服务网格生产部署 (30)
3. 多集群网络联邦 (31)
4. 网络性能优化 (34)
5. Terway 实例管理 (37)
6. Terway 全栈专题 (40-47)：产品概览 → 架构原理 → 使用配置 → 运维排障

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

### 运维工具推荐
- **诊断工具**: nicolaka/netshoot 镜像
- **性能测试**: iperf3, hping3
- **抓包分析**: tcpdump, wireshark
- **监控告警**: Prometheus + Alertmanager

## 质量保证检查清单

### ✅ 内容完整性检查
- [x] 所有47个文档均已创建并命名规范
- [x] 核心文档包含生产环境专家级内容
- [x] 提供实用的故障诊断脚本
- [x] 包含高可用设计模式和配置
- [x] 涵盖安全加固和最佳实践

### ✅ 结构组织检查
- [x] 文件按逻辑顺序编号 01-47
- [x] 目录结构清晰分类
- [x] 学习路径建议合理
- [x] 内容层次分明

### ✅ 实用性验证
- [x] 提供可执行的运维脚本
- [x] 包含真实场景的配置示例
- [x] 有详细的故障排查指南
- [x] 涵盖性能优化方案

## 更新日志

### 2026-02
- ✅ 新增生产环境运维专家级内容增强
- ✅ 重新组织文件命名顺序 (01-37)
- ✅ 补充网络高可用设计模式
- ✅ 增加故障诊断与恢复脚本
- ✅ 完善安全加固配置方案
- ✅ 优化容量规划与监控配置
- ✅ 建立完整的文档体系结构
- ✅ 新增 Terway 实例 CRUD 操作指南 (37)

### 2026-05
- ✅ 合并 topic-terway 专题到 domain-5-networking，新增 8 篇 Terway 深度文档 (40-47)
- ✅ 文档总数从 38 篇扩展至 47 篇

### 2026-04
- ✅ 新增 Terway GC (垃圾回收) 机制详解 (38)

---
**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com)

## Related

- [[README]]
- [[README]]

- 相关知识域: domain-01-cluster-fundamentals
- 相关知识域: domain-03-networking-traffic
- 相关知识域: domain-06-observability
- [[domain-17-system-foundation/速查卡/networking.md|速查卡: networking]]

<!-- risk-assessed -->
