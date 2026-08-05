---
title: 'Day 21: K8S 组件运维'
description: 'title: Day 21: K8S 组件运维'
summary: 'title: Day 21: K8S 组件运维'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- cilium
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 21: K8S 组件运维 是什么'
- '如何 Day 21: K8S 组件运维'
trigger_keywords:
- Day
- '21:'
- K8S
- 组件运维
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- cilium-basics
- cni-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: Day 21: K8S 组件运维
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[entities/kubernetes.md|[[Kubernetes|kubernetes]]]] control plane components运维
  - [[CoreDNS|CoreDNS]] troubleshooting DNS resolution
  - kube-proxy iptables IPVS mode
  - CNI Terway Flannel network troubleshooting
  - API Server etcd health check
trigger_keywords:
  - component operations
  - 组件运维
  - CoreDNS
  - kube-proxy
  - CNI
  - Terway
  - Flannel
  - CSI
  - API Server
  - etcd
  - health check
  - component upgrade
reading_level: advanced
audience:
  - ACK operators
  - SRE engineers
  - Platform engineers
estimated_read_time: 45min
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
  - domain-12-cloud-providers
related_topics:
  - apiserver-deep-dive
  - etcd-deep-dive
  - coredns-troubleshooting
  - kube-proxy-troubleshooting
---

# Day 21: K8S 组件运维

> **学习时间**: 4-5 小时 | **主题**: 核心组件状态检查与故障处理

---

## 概述

本文是 Kubernetes 组件运维的实战指南，帮助你掌握 K8s 核心组件（API Server、etcd、Controller Manager、Scheduler）以及 kube-system 关键插件（CoreDNS、kube-proxy、CNI、CSI）的状态检查、故障排查和升级管理。在 ACK 托管版中，管控面组件由阿里云托管运维，但了解它们的工作原理和排障方法对于理解集群行为和处理数据面问题至关重要。

### 学习目标

- 理解 ACK 托管版与专有版的组件架构差异
- 掌握核心组件（apiserver / etcd / controller-manager / scheduler）状态检查方法
- 能够排查 kube-system 命名空间中关键组件异常
- 了解 ACK 集群组件升级与自定义参数调整流程
- 掌握组件级问题的应急响应和根因分析思路

---

## 核心概念详解

### K8S 控制平面组件架构

Kubernetes 控制平面由四个核心组件构成，它们共同维护集群的期望状态：

**kube-apiserver** 是所有操作的入口。它提供 RESTful API，处理认证（Authentication）、授权（Authorization）和准入控制（Admission Control），然后将数据持久化到 etcd。所有其他组件（kubectl、kubelet、scheduler、controller-manager）都通过 API Server 进行交互，不会直接访问 etcd。API Server 支持多种认证方式：客户端证书、Service Account Token、OIDC Token 等。它还支持 Watch 机制，允许客户端订阅资源变化事件，这是控制器模式的基础。

**etcd** 是一个分布式的、一致的键值存储引擎，使用 Raft 共识协议保证数据一致性。etcd 存储了 K8s 集群的所有状态数据——Pod 定义、Service 配置、ConfigMap、Secret 等。etcd 的性能对整个集群至关重要：磁盘写入延迟直接影响 API Server 的响应速度（建议使用 SSD，fsync 延迟低于 10ms），数据库大小影响内存使用和查询性能（建议不超过 8GB）。在 ACK 托管版中，etcd 由阿里云托管，用户无需直接管理。在专有版中，etcd 通常以 3 节点或 5 节点集群形式部署在 Master 节点上。

**kube-controller-manager** 运行着多种控制器。每个控制器遵循 Reconcile（调和）模式：通过 Watch/List 监听资源变化 → 对比期望状态与实际状态 → 执行操作使实际状态趋向期望状态。核心控制器包括：Deployment Controller（管理滚动更新）、ReplicaSet Controller（维护 Pod 副本数）、Node Controller（监控节点健康）、Service Account Controller（管理服务账号）、Namespace Controller（清理已删除命名空间的资源）等。

**kube-scheduler** 负责将未调度的 Pod 分配到合适的节点。调度过程分为两个阶段：Filter（过滤不满足条件的节点，如资源不足、污点不匹配）和 Score（对候选节点打分排序，如资源均衡、亲和性偏好）。调度器的性能直接影响 Pod 的启动速度，在大规模集群中尤其重要。

### ACK 托管版 vs 专有版的组件运维差异

| 维度 | ACK 托管版 | ACK 专有版 |
|------|-----------|-----------|
| 管控面部署 | 阿里云管理，用户不可见 | 部署在用户的 Master ECS 上 |
| apiserver | 阿里云托管，高可用 | 用户自行维护，需配置 HA |
| etcd | 阿里云托管，自动备份 | 用户自行运维，需配置备份 |
| controller-manager | 阿里云托管 | 运行在 Master 节点 |
| scheduler | 阿里云托管 | 运行在 Master 节点 |
| 管控面升级 | 控制台一键升级 | 用户手动执行升级 |
| 管控面证书 | 自动轮换 | 需要手动或脚本处理 |
| 故障恢复 | 阿里云自动处理 | 用户自行排障和恢复 |

### kube-system 关键插件

除了控制平面组件，kube-system 命名空间中还运行着多个关键插件：

**CoreDNS** 提供 DNS 解析服务。所有集群内的服务发现都依赖 CoreDNS——Service 的 DNS 格式为 `<service-name>.<namespace>.svc.cluster.local`。CoreDNS 支持自定义配置（通过 ConfigMap），可以添加上游 DNS、配置存根域、启用日志等。CoreDNS 通常以 Deployment 方式部署，根据集群规模调整副本数。

**kube-proxy** 负责实现 Service 的网络转发规则。它监听 API Server 上 Service 和 Endpoints 的变化，并在节点上维护相应的 iptables 或 IPVS 规则。IPVS 模式在大规模集群中性能优于 iptables 模式，因为它使用哈希表查找而非线性遍历。kube-proxy 以 DaemonSet 方式部署在每个节点上。

**CNI 插件**（Terway 或 Flannel）负责为 Pod 分配 IP 和配置网络连通性。Terway 使用阿里云 ENI 为 Pod 分配 IP，支持网络策略；Flannel 使用 VxLAN 覆盖网络，配置简单但不支持 NetworkPolicy。

**CSI 插件** 负责存储卷的生命周期管理——创建、挂载、卸载、删除。阿里云提供了 disk-csi、nas-csi、oss-csi 等驱动。

### 组件健康检查机制

K8s 组件通过 HTTP 端点暴露健康状态：

- `/healthz`: 综合健康检查（包含所有子检查）
- `/livez`: 存活检查（失败时重启进程）
- `/readyz`: 就绪检查（失败时从负载均衡中移除）

这些端点支持 `verbose` 参数查看详细检查项，例如 `kubectl get --raw /readyz?verbose`。

---

## 实战演练

### 任务 1: 集群组件总览与状态检查 (40min)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看 kube-system 命名空间中的所有 Pod
kubectl get pods -n kube-system -o wide

# 预期输出示例:
# NAME                                    READY   STATUS    RESTARTS   AGE   IP            NODE
# coredns-7f6cb4b4f7-abc12               1/1     Running   0          2d    172.20.0.10   node-1
# coredns-7f6cb4b4f7-def34               1/1     Running   0          2d    172.20.1.11   node-2
# kube-proxy-abcde                       1/1     Running   0          2d    172.20.0.10   node-1
# kube-proxy-fghij                       1/1     Running   0          2d    172.20.1.11   node-2

# 查看各组件 Pod 运行状态，按状态排序
kubectl get pods -n kube-system --sort-by='.status.phase'

# 检查 apiserver 健康状态（托管版通过 kubeconfig 端点）
kubectl get --raw /healthz
# 预期输出: ok

kubectl get --raw /livez
# 预期输出: ok

kubectl get --raw /readyz
# 预期输出: ok

# 查看详细的健康检查项
kubectl get --raw '/readyz?verbose'
# 预期输出:
# [+]ping ok
# [+]log ok
# [+]etcd ok
# [+]poststarthook/start-kube-apiserver-admission-initializer ok
# [+]poststarthook/generic-apiserver-start-informers ok
# ...

# 查看集群版本信息
kubectl version --short 2>/dev/null || kubectl version -o yaml
# 预期输出:
# Client Version: v1.28.4
# Kustomize Version: v5.0.4-0.20230601165947-6ce0bf390ce3
# Server Version: v1.28.3-aliyun.1

# 查看集群组件状态（专有版）
kubectl get componentstatuses 2>/dev/null || echo "托管版不支持此命令"

# 查看集群信息
kubectl cluster-info
# 预期输出:
# Kubernetes control plane is running at https://xxx.cn-hangzhou.alicontainer.com:6443
# CoreDNS is running at https://xxx.cn-hangzhou.alicontainer.com:6443/api/v1/namespaces/kube-system/services/kube-dns:dns/proxy
```
### 任务 2: CoreDNS 检查与排查 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 CoreDNS 运行状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
# 预期输出:
# NAME                       READY   STATUS    RESTARTS   AGE
# coredns-7f6cb4b4f7-abc12   1/1     Running   0          2d
# coredns-7f6cb4b4f7-def34   1/1     Running   0          2d

# 查看 DNS Service
kubectl get svc -n kube-system kube-dns
# 预期输出:
# NAME       TYPE        CLUSTER-IP    EXTERNAL-IP   PORT(S)         AGE
# kube-dns   ClusterIP   172.21.0.10   <none>        53/UDP,53/TCP   30d

# 查看 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml
# 预期输出包含 Corefile:
# .:53 {
#     errors
#     health {
#        lameduck 5s
#     }
#     ready
#     kubernetes cluster.local in-addr.arpa ip6.arpa {
#        pods insecure
#        fallthrough in-addr.arpa ip6.arpa
#        ttl 30
#     }
#     prometheus :9153
#     forward . /etc/resolv.conf {
#        max_concurrent 1000
#     }
#     cache 30
#     loop
#     reload
#     loadbalance
# }

# DNS 解析测试
kubectl run dns-test --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- nslookup kubernetes.default
# 预期输出:
# Server:    172.21.0.10
# Address 1: 172.21.0.10 kube-dns.kube-system.svc.cluster.local
# Name:      kubernetes.default
# Address 1: 172.21.0.1 kubernetes.default.svc.cluster.local

# 查看 CoreDNS 日志
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=30
# 预期输出:
# [INFO] plugin/ready: Still waiting on: "kubernetes"
# [INFO] plugin/ready: Still waiting on: "kubernetes"
# .:53
# [INFO] plugin/reload: Running configuration SHA256 = ...
# [INFO] CoreDNS-1.9.3
# [INFO] linux/amd64, go1.19.3, ...

# 开启 CoreDNS 日志调试（修改 ConfigMap 添加 log 插件）
kubectl get configmap coredns -n kube-system -o yaml | \
  sed 's/errors/errors\n    log/' | \
  kubectl apply -f -
# 等待 CoreDNS 重载配置
kubectl rollout restart deployment coredns -n kube-system
```
### 任务 3: kube-proxy 与网络插件检查 (40min)

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 kube-proxy 运行状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
# 预期输出:
# NAME               READY   STATUS    RESTARTS   AGE
# kube-proxy-abcde   1/1     Running   0          2d
# kube-proxy-fghij   1/1     Running   0          2d

# 查看 kube-proxy DaemonSet
kubectl get ds -n kube-system kube-proxy
# 预期输出:
# NAME         DESIRED   CURRENT   READY   UP-TO-DATE   AVAILABLE   NODE SELECTOR            AGE
# kube-proxy   3         3         3       3            3           kubernetes.io/os=linux   30d

# 查看 kube-proxy 配置（检查模式: iptables vs IPVS）
kubectl get configmap kube-proxy -n kube-system -o yaml | head -50
# 关键字段:
# mode: ""  (空表示自动选择，通常为 iptables)
# mode: "ipvs"  (显式使用 IPVS 模式)

# 在节点上检查 kube-proxy 模式
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}') -- curl -s http://localhost:10249/proxyMode
# 预期输出: iptables 或 ipvs

# 查看 CNI 插件状态（Terway 或 Flannel）
kubectl get pods -n kube-system | grep -E "terway|flannel"
kubectl get ds -n kube-system | grep -E "terway|flannel"
# Terway 示例输出:
# NAME           DESIRED   CURRENT   READY   UP-TO-DATE   AVAILABLE   NODE SELECTOR   AGE
# terway-eniip   3         3         3       3            3           <none>          30d

# 查看 CSI 存储插件
kubectl get pods -n kube-system | grep csi
# 预期输出:
# csi-plugin-abcde                        4/4     Running   0          2d
# csi-plugin-fghij                        4/4     Running   0          2d
# csi-provisioner-0                       1/1     Running   0          2d
```
### 任务 4: ACK 集群组件（Addon）管理 (30min)

```bash
# 通过 API 查看集群已安装的组件
aliyun cs GET /clusters/<cluster_id>/components
# 预期输出（JSON 数组）:
# [
#   {
#     "name": "terway-eniip",
#     "version": "v1.0.0",
#     "status": "installed"
#   },
#   {
#     "name": "csi-plugin",
#     "version": "v1.0.0",
#     "status": "installed"
#   },
#   ...
# ]

# 查看指定组件详情
aliyun cs GET /clusters/<cluster_id>/components/<component_name>

# 升级组件（如 CoreDNS）
# 注意: 生产环境请先在测试集群验证
aliyun cs POST /clusters/<cluster_id>/components/<component_name>/upgrade

# 查看组件升级状态
aliyun cs GET /clusters/<cluster_id>/components/<component_name>

# 通过控制台路径: 集群详情 → 组件管理 → 查看/升级

# 查看组件版本和可用升级
aliyun cs GET /clusters/<cluster_id>/components --body '{"name":"coredns"}'
```

### 任务 5: 组件故障排查演练 (30min)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `kubectl scale --replicas=0`：缩容到 0，立即停服
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 场景 1: CoreDNS Pod 异常
# 模拟 CoreDNS 问题
kubectl scale deployment coredns -n kube-system --replicas=0

# 观察影响
kubectl run test-dns --image=registry.cn-hangzhou.aliyuncs.com/acs-sample/busybox:1.36 \
  --rm -it --restart=Never -- nslookup kubernetes.default
# 预期: 解析失败或超时

# 恢复
kubectl scale deployment coredns -n kube-system --replicas=2

# 场景 2: kube-proxy 异常导致 Service 不可达
# 查看 kube-proxy 日志排查
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50

# 查看 iptables 规则
kubectl exec -n kube-system $(kubectl get pods -n kube-system -l k8s-app=kube-proxy -o jsonpath='{.items[0].metadata.name}') -- iptables -t nat -L KUBE-SERVICES | head -20

# 场景 3: 节点 NotReady 导致组件异常
kubectl get nodes -o wide
kubectl describe node <problem-node> | grep -A 10 "Conditions"

# 查看节点事件
kubectl get events -A --field-selector reason=NodeNotReady
```
---

## 配置示例

### CoreDNS 自定义配置 ConfigMap

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: coredns
  namespace: kube-system
data:
  Corefile: |
    .:53 {
        errors
        health {
           lameduck 5s
        }
        ready
        log
        kubernetes cluster.local in-addr.arpa ip6.arpa {
           pods verified
           fallthrough in-addr.arpa ip6.arpa
           ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
           max_concurrent 1000
        }
        cache 30
        loop
        reload
        loadbalance
    }
    example.com:53 {
        errors
        cache 30
        forward . 10.0.0.1:53
    }
```

### kube-proxy IPVS 模式 ConfigMap 片段

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-proxy
  namespace: kube-system
data:
  config.conf: |
    apiVersion: kubeproxy.config.k8s.io/v1alpha1
    bindAddress: 0.0.0.0
    clientConnection:
      acceptContentTypes: ""
      burst: 10
      contentType: application/vnd.kubernetes.protobuf
      qps: 5
    clusterCIDR: "172.20.0.0/16"
    configSyncPeriod: 15m0s
    conntrack:
      maxPerCore: 32768
      min: 131072
      tcpCloseWaitTimeout: 1h0m0s
      tcpEstablishedTimeout: 24h0m0s
    enableProfiling: false
    healthzBindAddress: 0.0.0.0:10256
    iptables:
      masqueradeAll: false
      masqueradeBit: 14
      minSyncPeriod: 0s
      syncPeriod: 30s
    ipvs:
      excludeCIDRs: null
      minSyncPeriod: 0s
      scheduler: "rr"
      syncPeriod: 30s
      strictARP: false
    kind: KubeProxyConfiguration
    metricsBindAddress: 127.0.0.1:10249
    mode: "ipvs"
    nodePortAddresses: null
    oomScoreAdj: -999
    portRange: ""
    udpIdleTimeout: 250ms
```

### 组件状态检查脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# component-health-check.sh - K8s 组件健康检查脚本

echo "=========================================="
echo "  Kubernetes 组件健康检查报告"
echo "  时间: $(date '+%Y-%m-%d %H:%M:%S')"
echo "=========================================="

echo ""
echo "=== 1. 集群基本信息 ==="
kubectl cluster-info 2>/dev/null | head -3
kubectl version -o yaml 2>/dev/null | grep -E "gitVersion|platform"

echo ""
echo "=== 2. 节点状态 ==="
kubectl get nodes -o wide

echo ""
echo "=== 3. kube-system Pod 状态 ==="
kubectl get pods -n kube-system -o wide

echo ""
echo "=== 4. API Server 健康检查 ==="
echo -n "/healthz:  "; kubectl get --raw /healthz 2>/dev/null
echo -n "/livez:    "; kubectl get --raw /livez 2>/dev/null
echo -n "/readyz:   "; kubectl get --raw /readyz 2>/dev/null

echo ""
echo "=== 5. CoreDNS 状态 ==="
kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
kubectl get svc -n kube-system kube-dns

echo ""
echo "=== 6. kube-proxy 状态 ==="
kubectl get ds -n kube-system kube-proxy
kubectl get pods -n kube-system -l k8s-app=kube-proxy -o wide

echo ""
echo "=== 7. CNI 插件状态 ==="
kubectl get ds -n kube-system | grep -E "terway|flannel|calico|cilium"

echo ""
echo "=== 8. CSI 插件状态 ==="
kubectl get pods -n kube-system | grep csi

echo ""
echo "=== 9. 异常 Pod 检查 ==="
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded -o wide 2>/dev/null

echo ""
echo "=== 10. 近期事件 ==="
kubectl get events -A --sort-by='.lastTimestamp' 2>/dev/null | tail -10

echo ""
echo "=========================================="
echo "  检查完成"
echo "=========================================="
```
---

## 常见问题

### Q1: CoreDNS Pod 重启次数增多怎么排查？

检查 CoreDNS 日志中是否有错误信息：`kubectl logs -n kube-system -l k8s-app=kube-dns --previous`。常见原因包括：上游 DNS 不可达（检查 forward 配置）、CoreDNS 内存不足（调大 resources.limits）、配置错误（检查 Corefile 语法）。如果日志中有 `too many open files`，需要调大 ulimit 或重启节点上的 containerd。

### Q2: kube-proxy 切换到 IPVS 模式后 Service 不通怎么办？

首先确认 IPVS 内核模块已加载：`lsmod | grep ip_vs`。如果模块未加载，需要在所有节点上执行 `modprobe ip_vs ip_vs_rr ip_vs_wrr ip_vs_sh nf_conntrack`。然后检查 kube-proxy 日志确认 IPVS 模式生效：`kubectl logs -n kube-system -l k8s-app=kube-proxy | grep "Using ipvs Proxier"`。最后检查 IPVS 规则：`ipvsadm -Ln`。

### Q3: ACK 托管版的 API Server 偶尔超时怎么处理？

托管版的 API Server 由阿里云保障可用性，但以下情况可能导致超时：1）大规模 List 请求（如 `kubectl get pods -A` 在数千 Pod 的集群中）占用过多内存；2）etcd 写入延迟波动；3）客户端网络抖动。建议：使用 `--limit` 参数分页查询、避免频繁全量 List、在应用中使用 Informer/SharedInformerFactory 缓存而非直接 List/Watch。

### Q4: 如何判断是 CNI 插件问题还是 kube-proxy 问题？

关键区分方法：如果是 Pod 之间通过 Pod IP 无法通信，通常是 CNI 问题；如果是通过 Service ClusterIP 无法访问但 Pod IP 可以，通常是 kube-proxy 问题。排查步骤：1）测试 Pod IP 直连；2）测试 Service ClusterIP；3）测试 NodePort；4）检查 iptables/IPVS 规则；5）检查 CNI 插件日志。

### Q5: 组件升级时服务会中断吗？

控制平面组件升级时，API Server 通常会有短暂不可用（几秒到几十秒），但因为多副本部署，客户端请求会自动重试到健康的副本。kubelet 升级时，节点上的 Pod 不会重启（除非 kubelet 重启时间超过 `pod-eviction-timeout`）。CoreDNS 升级采用滚动更新，至少保留一个副本可用。建议在业务低峰期执行组件升级。

### Q6: 如何监控组件的性能指标？

API Server 暴露了 Prometheus 指标（`:443/metrics`），关键指标包括 `apiserver_request_total`（请求总数）、`apiserver_request_duration_seconds`（请求延迟）、`etcd_request_duration_seconds`（etcd 请求延迟）。kube-proxy 暴露指标在 `:10249/metrics`，关注 `kube_proxy_sync_rules_duration_seconds`（规则同步延迟）。CoreDNS 指标在 `:9153/metrics`，关注 `coredns_dns_request_duration_seconds`（DNS 解析延迟）。

---

## 要点总结

| 组件 | 作用 | 托管版维护方 | 检查方式 | 关键指标 |
|------|------|-------------|---------|---------|
| apiserver | API 入口 | 阿里云 | `/healthz` `/readyz` | 请求延迟、错误率 |
| etcd | 状态存储 | 阿里云 | 托管版无需关注 | 磁盘延迟、DB 大小 |
| controller-manager | 控制循环 | 阿里云 | 托管版无需关注 | 队列深度 |
| scheduler | Pod 调度 | 阿里云 | 托管版无需关注 | 调度延迟 |
| CoreDNS | DNS 解析 | 用户 | `kubectl get pods -n kube-system` | 解析延迟、错误率 |
| kube-proxy | Service 转发 | 用户 | `kubectl get ds kube-proxy` | 规则同步延迟 |
| Terway/Flannel | Pod 网络 | 用户 | `kubectl get ds -n kube-system` | Pod 分配延迟 |
| CSI 插件 | 存储卷 | 用户 | `kubectl get pods -n kube-system` | 卷操作延迟 |

---

## 延伸阅读

- [K8s 架构与组件深入](32-发布/package/2026-07-02_18-53/corpus/core/domain-01-cluster-fundamentals/01-architecture-overview/01-core-components-deep-dive.md)
- [etcd 深入分析](32-发布/package/2026-07-02_18-53/corpus/supporting/domain-01-cluster-fundamentals/03-control-plane/04-etcd-deep-dive.md)
- [API Server 深入分析](32-发布/package/2026-07-02_18-53/corpus/supporting/domain-01-cluster-fundamentals/03-control-plane/05-apiserver-deep-dive.md)
- [ACK 集群管理](../../domain-12-cloud-providers/04-alicloud-ack/210-ack-cluster-management.md)
- [组件故障排查总览](../../domain-10-troubleshooting-diagnostics/01-troubleshooting-overview.md)
- [CoreDNS 排障指南](../../domain-10-troubleshooting-diagnostics/11-coredns-troubleshooting.md)
- [kube-proxy 排障指南](../../domain-10-troubleshooting-diagnostics/10-kube-proxy-troubleshooting.md)

```

<!-- risk-assessed -->
