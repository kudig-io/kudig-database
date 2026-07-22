---
title: 76 - CNI插件深度对比
description: '# 76 - CNI插件深度对比'
summary: '# 76 - CNI插件深度对比'
category: networking
tags:
- k8s
- networking
- service
- ingress
- cni
- cilium
- flannel
- calico
- coredns
- helm
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
- CNI插件深度对比 是什么
- 如何 CNI插件深度对比
- Kubernetes 5 networking 最佳实践
trigger_keywords:
- CNI插件深度对比
- networking
prerequisites:
- kubectl-basics
- networking-basics
- helm-basics
- ebpf-basics
- cilium-basics
- cni-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../网络/
  label: '相关知识域: 网络'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/networking.md
  label: '速查卡: networking'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 76 - CNI插件深度对比

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [[实体/kubernetes.md|kubernetes]].io/docs/concepts/cluster-administration/networking](https://kubernetes.io/docs/concepts/cluster-administration/networking/)

<!-- chunk: CNI插件功能对比 -->
## CNI插件功能对比

| 功能 | Calico | [[Cilium|Cilium]] | Flannel | Terway(ACK) | [[Antrea|Antrea]] |
|-----|--------|--------|---------|-------------|--------|
| **网络模式** | VXLAN/IPIP/BGP | VXLAN/Native | VXLAN/host-gw | VPC/ENIIP | VXLAN/Geneve |
| **[[NetworkPolicy|NetworkPolicy]]** | ✅ 完整 | ✅ 完整+L7 | ❌ | ✅ 完整 | ✅ 完整 |
| **eBPF数据面** | ✅ (可选) | ✅ (原生) | ❌ | ✅ | ❌ |
| **服务网格** | ❌ | ✅ (Cilium Mesh) | ❌ | ASM集成 | ❌ |
| **带宽限制** | ✅ | ✅ | ❌ | ✅ | ✅ |
| **多集群** | ✅ | ✅ ClusterMesh | ❌ | ACK One | ✅ |
| **Windows** | ✅ | ⚠️ Beta | ✅ | ✅ | ✅ |
| **IPv6** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **双栈** | ✅ | ✅ | ✅ | ✅ | ✅ |
| **加密** | WireGuard | WireGuard/IPsec | ❌ | ✅ | IPsec |
| **可观测性** | ✅ | ✅ Hubble | 基础 | ARMS集成 | ✅ |

<!-- chunk: Calico配置 -->
## Calico配置

```yaml
apiVersion: projectcalico.org/v3
kind: IPPool
metadata:
  name: default-ipv4-ippool
spec:
  cidr: 10.244.0.0/16
  ipipMode: Always  # Always/CrossSubnet/Never
  vxlanMode: Never  # Always/CrossSubnet/Never
  natOutgoing: true
  nodeSelector: all()
  blockSize: 26
---
apiVersion: projectcalico.org/v3
kind: FelixConfiguration
metadata:
  name: default
spec:
  bpfEnabled: true
  bpfDataIfacePattern: "^(en|eth|ens|eno).*"
  bpfConnectTimeLoadBalancingEnabled: true
  bpfExternalServiceMode: DSR
  ipipEnabled: false
  vxlanEnabled: false
  wireguardEnabled: true  # 加密
```

<!-- chunk: Cilium配置 -->
## Cilium配置

```yaml
# Helm values
apiVersion: v1
kind: ConfigMap
metadata:
  name: cilium-config
data:
  values.yaml: |
    cluster:
      name: production
      id: 1
    
    ipam:
      mode: cluster-pool
      operator:
        clusterPoolIPv4PodCIDRList:
          - 10.244.0.0/16
        clusterPoolIPv4MaskSize: 24
    
    kubeProxyReplacement: true
    k8sServiceHost: kubernetes.default.svc
    k8sServicePort: 443
    
    bpf:
      masquerade: true
      hostLegacyRouting: false
    
    loadBalancer:
      mode: dsr
      algorithm: maglev
    
    hubble:
      enabled: true
      relay:
        enabled: true
      ui:
        enabled: true
    
    encryption:
      enabled: true
      type: wireguard
    
    bandwidthManager:
      enabled: true
    
    egressGateway:
      enabled: true
```

<!-- chunk: Flannel配置 -->
## Flannel配置

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-flannel-cfg
  namespace: kube-flannel
data:
  net-conf.json: |
    {
      "Network": "10.244.0.0/16",
      "Backend": {
        "Type": "vxlan",
        "VNI": 1,
        "Port": 8472,
        "DirectRouting": true
      }
    }
```

<!-- chunk: Terway配置(ACK) -->
## Terway配置(ACK)

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: eni-config
  namespace: kube-system
data:
  eni_conf: |
    {
      "version": "1",
      "max_pool_size": 25,
      "min_pool_size": 10,
      "credential_path": "/var/addon/token-config",
      "vswitches": {
        "cn-hangzhou-h": ["vsw-xxx"],
        "cn-hangzhou-i": ["vsw-yyy"]
      },
      "security_groups": ["sg-xxx"],
      "service_cidr": "172.21.0.0/20"
    }

```

<!-- chunk: CNI性能对比 -->
## CNI性能对比

| CNI | Pod启动时间 | 吞吐量 | 延迟 | CPU开销 |
|-----|-----------|-------|------|--------|
| Calico VXLAN | 中 | 高 | 中 | 中 |
| Calico eBPF | 快 | 很高 | 低 | 低 |
| Cilium | 快 | 很高 | 低 | 低 |
| Flannel VXLAN | 中 | 中 | 中 | 低 |
| Terway ENIIP | 快 | 最高 | 最低 | 最低 |

<!-- chunk: CNI选型建议 -->
## CNI选型建议

| 场景 | 推荐CNI | 原因 |
|-----|--------|------|
| 通用生产 | Calico | 成熟稳定,功能全面 |
| 高性能/安全 | Cilium | eBPF原生,L7策略 |
| 简单场景 | Flannel | 配置简单 |
| 阿里云 | Terway | VPC原生,性能最优 |
| 多集群 | Cilium | ClusterMesh |

<!-- chunk: CNI故障排查 -->
## CNI故障排查

| 问题 | 诊断命令 | 解决方向 |
|-----|---------|---------|
| Pod无IP | `kubectl describe pod` | 检查IPAM配置 |
| 跨节点不通 | `calicoctl node status` | 检查BGP/隧道状态 |
| 策略不生效 | `cilium policy get` | 检查策略配置 |
| 性能差 | `cilium monitor` | 检查数据路径 |
| DNS解析失败 | `kubectl exec -- nslookup` | 检查CoreDNS |
| 服务不可达 | `kubectl get endpoints` | 检查kube-proxy |

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Calico诊断
calicoctl node status
calicoctl get ippool -o wide
calicoctl get workloadendpoint

# Cilium诊断
cilium status
cilium connectivity test
cilium hubble observe --follow

# 网络连通性测试
kubectl run test --rm -it --image=nicolaka/netshoot -- bash
# 在容器内: ping, curl, dig, traceroute, iperf3

# 查看CNI配置
cat /etc/cni/net.d/*.conf
ls -la /opt/cni/bin/

# 检查iptables规则
iptables -t nat -L -n -v
iptables -t filter -L -n -v
```
<!-- chunk: 版本兼容性 -->
## 版本兼容性

| CNI | v1.25 | v1.28 | v1.32 | 推荐版本 |
|-----|-------|-------|-------|---------|
| Calico | 3.24+ | 3.26+ | 3.28+ | 3.28 |
| Cilium | 1.12+ | 1.14+ | 1.16+ | 1.16 |
| Flannel | 0.20+ | 0.22+ | 0.25+ | 0.25 |
| Terway | 1.5+ | 1.7+ | 1.9+ | 1.9 |

---

**CNI选型原则**: 根据场景选择(通用Calico/高性能Cilium/云原生Terway) + 评估功能需求 + 考虑运维复杂度

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 网络 MOC
- [[网络/README.md|Domain 03: Networking 网络]]
- Kubernetes 网络基础 Network in a Nutshell
- Domain-5 网络 — 开源项目索引
- FAQ 文档
- 网络核心组件
- CNI 架构与核心原理
- 142 - Flannel 完整指南 (Flannel Complete Guide)
- Flannel WireGuard 加密后端配置
- Flannel IPv6 Dual Stack 支持
- Flannel Windows 节点支持
- Flannel 多集群场景与子网冲突处理

## 生产最佳实践

| 维度 | 建议 | 说明 |
|------|------|------|
| **选型** | 根据场景选择 | 通用 Calico/高性能 Cilium/云原生 Terway |
| **版本** | 使用稳定版本 | 参考兼容性矩阵 |
| **监控** | 部署 CNI 指标 | 实时掌握网络状态 |
| **升级** | 滚动升级 | 先测试环境验证 |
| **备份** | 定期备份配置 | 便于灾难恢复 |
| **安全** | 启用 NetworkPolicy | 默认拒绝 + 显式放行 |

## 相关工具

| 工具 | 用途 |
|------|------|
| `calicoctl` | Calico 管理 |
| `cilium` | Cilium CLI |
| `helm` | CNI 部署管理 |

## See Also

- 01-network-architecture-overview
- 02-cni-architecture-fundamentals
- 04-flannel-complete-guide
- 04a-flannel-wireguard-backend

## Related

- [[生态参考/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[生态参考/领域索引/network-index.md|Network 网络知识图谱索引]]

```

<!-- risk-assessed -->
