---
title: Kubernetes 全量故障树分析(FTA)排查手册 (domain-10-troubleshooting-diagnostics)
description: 'title: Kubernetes 全量故障树分析(FTA)排查手册'
category: fta
tags:
- fta
- troubleshooting
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- grafana
- cilium
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 90min
intent_queries:
- Kubernetes 全量故障树分析(FTA)排查手册 是什么
- 如何 Kubernetes 全量故障树分析(FTA)排查手册
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- Kubernetes 全量故障树分析(FTA)排查手册 故障排查
- Kubernetes 全量故障树分析(FTA)排查手册 排障步骤
- Kubernetes 全量故障树分析(FTA)排查手册 根因分析
trigger_keywords:
- Kubernetes
- 全量故障树分析
- FTA
- 排查手册
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- prometheus-basics
- monitoring-basics
- cilium-basics
- cni-basics
- etcd-basics
- gpu-scheduling-basics
fta_id: FTA-KUBERNETES_FULL_ANALYSIS-001
component: Kubernetes Full Analysis
severity: critical
created: "2026-05-23"
---

title: [[Kubernetes|Kubernetes]] 全量故障树分析(FTA)排查手册
description: '# Kubernetes 全量故障树分析(FTA)排查手册'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- [[etcd|etcd]]
- apiserver
- [[kubelet|kubelet]]
- scheduler
- controller-manager
- prometheus
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- Kubernetes 全量故障树分析(FTA)排查手册 是什么
- 如何 Kubernetes 全量故障树分析(FTA)排查手册
- Kubernetes 全量故障树分析(FTA)排查手册 根因分析
- Kubernetes 全量故障树分析(FTA)排查手册 故障树
trigger_keywords:
- Kubernetes
- 全量故障树分析
- FTA
- 排查手册
- fta
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# Kubernetes 全量故障树分析(FTA)排查手册

> **文档版本**: v1.0  
> **适用范围**: Kubernetes 生产环境全量问题场景  
> **更新日期**: 2024年

---

<!-- chunk: 一、故障树总览 -->## 一、故障树总览

## 1.1 顶部事件定义表

| 编号 | 顶部事件 | 严重程度 | 影响范围 | 典型症状 |
|------|----------|----------|----------|----------|
| TE-1 | 集群完全不可用 | 🔴 P0 | 整个集群 | kubectl无法连接，所有服务中断 |
| TE-2 | 应用服务不可用 | 🔴 P0 | 特定应用 | 用户无法访问应用，HTTP 5xx错误 |
| TE-3 | Pod启动失败 | 🟠 P1 | 特定Pod | Pod处于Pending/Error状态 |
| TE-4 | 网络通信异常 | 🟠 P1 | 网络层面 | DNS解析失败，Pod间无法通信 |
| TE-5 | 存储访问失败 | 🟠 P1 | 存储层面 | PVC无法绑定，卷挂载失败 |
| TE-6 | 资源调度异常 | 🟡 P2 | 调度层面 | Pod无法调度，调度结果异常 |
| TE-7 | 安全认证失败 | 🟠 P1 | 安全层面 | 认证/授权失败，证书过期 |
| TE-8 | 监控告警异常 | 🟡 P2 | 监控层面 | 指标丢失，告警不触发 |

## 1.2 故障树总览图 (ASCII)

```
                                    ┌─────────────────────────────────────┐
                                    │         Kubernetes 问题空间          │
                                    └──────────────────┬──────────────────┘
                                                       │
           ┌───────────────┬───────────────┬───────────┴───────────┬───────────────┬───────────────┐
           │               │               │                       │               │               │
           ▼               ▼               ▼                       ▼               ▼               ▼
    ┌────────────┐  ┌────────────┐  ┌────────────┐          ┌────────────┐  ┌────────────┐  ┌────────────┐
    │   TE-1     │  │   TE-2     │  │   TE-3     │          │   TE-4     │  │   TE-5     │  │   TE-6     │
    │ 集群完全   │  │ 应用服务   │  │ Pod启动    │          │ 网络通信   │  │ 存储访问   │  │ 资源调度   │
    │ 不可用     │  │ 不可用     │  │ 失败       │          │ 异常       │  │ 失败       │  │ 异常       │
    │   🔴 P0    │  │   🔴 P0    │  │   🟠 P1    │          │   🟠 P1    │  │   🟠 P1    │  │   🟡 P2    │
    └──────┬─────┘  └──────┬─────┘  └──────┬─────┘          └──────┬─────┘  └──────┬─────┘  └──────┬─────┘
           │               │               │                       │               │               │
           │               │               │                       │               │               │
           ▼               ▼               ▼                       ▼               ▼               ▼
    ┌────────────┐  ┌────────────┐  ┌────────────┐          ┌────────────┐  ┌────────────┐  ┌────────────┐
    │  TE-7      │  │  TE-8      │  │            │          │            │  │            │  │            │
    │ 安全认证   │  │ 监控告警   │  │            │          │            │  │            │  │            │
    │ 失败       │  │ 异常       │  │            │          │            │  │            │  │            │
    │   🟠 P1    │  │   🟡 P2    │  │            │          │            │  │            │  │            │
    └────────────┘  └────────────┘  └────────────┘          └────────────┘  └────────────┘  └────────────┘


详细故障树结构:
═══════════════════════════════════════════════════════════════════════════════════════════════════════════

TE-1: 集群完全不可用 [OR门]
│
├── IE-1.1 控制平面问题 [OR门]
│   ├── BE-1.1 API Server问题
│   ├── BE-1.2 etcd集群问题
│   ├── BE-1.3 Scheduler问题
│   └── BE-1.4 Controller Manager问题
│
├── IE-1.2 工作节点批量问题 [AND门 - 多数节点]
│   ├── BE-1.5 Kubelet服务问题
│   ├── BE-1.6 容器运行时问题
│   └── BE-1.7 节点网络问题
│
└── IE-1.3 网络基础设施问题 [OR门]
    ├── BE-1.8 CNI插件问题
    └── BE-1.9 核心网络设备问题


TE-2: 应用服务不可用 [OR门]
│
├── IE-2.1 Pod运行异常 [OR门]
│   ├── BE-2.1 CrashLoopBackOff
│   ├── BE-2.2 ImagePullBackOff
│   ├── BE-2.3 OOMKilled
│   └── BE-2.4 Evicted
│
├── IE-2.2 Service访问异常 [OR门]
│   ├── BE-2.5 无可用Endpoint
│   ├── BE-2.6 端口配置错误
│   └── BE-2.7 kube-proxy问题
│
└── IE-2.3 Ingress访问异常 [OR门]
    ├── BE-2.8 Ingress Controller问题
    ├── BE-2.9 Ingress规则配置错误
    └── BE-2.10 负载均衡器问题


TE-3: Pod启动失败 [OR门]
│
├── IE-3.1 调度失败 [OR门]
│   ├── BE-3.1 节点资源不足
│   ├── BE-3.2 节点选择器不匹配
│   ├── BE-3.3 污点阻止调度
│   └── BE-3.4 资源配额超限
│
├── IE-3.2 镜像拉取失败 [OR门]
│   ├── BE-3.5 镜像不存在
│   ├── BE-3.6 镜像仓库认证失败
│   └── BE-3.7 网络不可达
│
└── IE-3.3 容器创建失败 [OR门]
    ├── BE-3.8 CNI配置失败
    ├── BE-3.9 存储挂载失败
    └── BE-3.10 Init容器失败


TE-4: 网络通信异常 [OR门]
│
├── IE-4.1 DNS解析异常 [OR门]
│   ├── BE-4.1 CoreDNS Pod问题
│   ├── BE-4.2 DNS配置错误
│   └── BE-4.3 网络策略阻止DNS
│
├── IE-4.2 Pod间通信异常 [OR门]
│   ├── BE-4.4 CNI插件问题
│   ├── BE-4.5 网络策略阻止
│   └── BE-4.6 iptables规则错误
│
└── IE-4.3 集群外部访问异常 [OR门]
    ├── BE-4.7 Egress配置错误
    ├── BE-4.8 NAT配置问题
    └── BE-4.9 防火墙阻止


TE-5: 存储访问失败 [OR门]
│
├── IE-5.1 PVC绑定失败 [OR门]
│   ├── BE-5.1 StorageClass配置错误
│   ├── BE-5.2 PV资源不足
│   └── BE-5.3 CSI驱动异常
│
├── IE-5.2 存储卷挂载失败 [OR门]
│   ├── BE-5.4 挂载参数错误
│   ├── BE-5.5 权限不足
│   └── BE-5.6 文件系统损坏
│
└── IE-5.3 存储性能/数据异常 [OR门]
    ├── BE-5.7 存储后端性能下降
    ├── BE-5.8 数据损坏
    └── BE-5.9 快照恢复失败


TE-6: 资源调度异常 [OR门]
│
├── IE-6.1 Pod无法调度 [OR门]
│   ├── BE-6.1 节点资源不足
│   ├── BE-6.2 亲和性冲突
│   └── BE-6.3 污点不匹配
│
├── IE-6.2 调度结果不符合预期 [OR门]
│   ├── BE-6.4 调度器配置错误
│   └── BE-6.5 优先级抢占问题
│
└── IE-6.3 自定义调度器问题 [OR门]
    ├── BE-6.6 调度器插件错误
    └── BE-6.7 扩展点配置错误


TE-7: 安全认证失败 [OR门]
│
├── IE-7.1 证书相关问题 [OR门]
│   ├── BE-7.1 证书过期
│   ├── BE-7.2 证书链不完整
│   └── BE-7.3 CA配置错误
│
├── IE-7.2 RBAC权限问题 [OR门]
│   ├── BE-7.4 Role配置错误
│   ├── BE-7.5 RoleBinding缺失
│   └── BE-7.6 ServiceAccount问题
│
└── IE-7.3 准入控制问题 [OR门]
    ├── BE-7.7 Webhook不可用
    ├── BE-7.8 Validating配置错误
    └── BE-7.9 Mutating配置错误


TE-8: 监控告警异常 [OR门]
│
├── IE-8.1 监控数据采集异常 [OR门]
│   ├── BE-8.1 Prometheus问题
│   ├── BE-8.2 ServiceMonitor错误
│   └── BE-8.3 指标丢失
│
├── IE-8.2 告警系统异常 [OR门]
│   ├── BE-8.4 Alertmanager问题
│   ├── BE-8.5 告警规则错误
│   └── BE-8.6 通知渠道失败
│
└── IE-8.3 可视化系统异常 [OR门]
    ├── BE-8.7 Grafana问题
    ├── BE-8.8 Dashboard配置错误
    └── BE-8.9 数据源连接失败
```

## 1.3 问题传播逻辑说明

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        问题传播逻辑说明                                       │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  【AND门 - 与门】                                                            │
│  符号: ──┬──                                                                │
│          │                                                                  │
│  含义: 所有输入事件同时发生时，输出事件才会发生                                   │
│  示例: 多数节点问题(需要多个节点同时问题才会导致服务不可用)                        │
│                                                                             │
│  【OR门 - 或门】                                                             │
│  符号: ──┬──                                                                │
│        ──┴──                                                                │
│  含义: 任一输入事件发生，输出事件就会发生                                        │
│  示例: 控制平面问题(任一组件问题都可能导致集群异常)                               │
│                                                                             │
│  【问题传播路径】                                                             │
│                                                                             │
│  底部事件(BE) → 中间事件(IE) → 顶部事件(TE)                                   │
│                                                                             │
│  示例传播链:                                                                 │
│  证书过期(BE-7.1) → 证书相关问题(IE-7.1) → 安全认证失败(TE-7)                  │
│                  → API Server无法启动 → 集群完全不可用(TE-1)                  │
│                                                                             │
│  【严重程度分级】                                                             │
│  🔴 P0 - 集群/核心服务完全不可用，需立即处理                                     │
│  🟠 P1 - 重要功能受影响，需在4小时内处理                                        │
│  🟡 P2 - 部分功能受限，需在24小时内处理                                         │
│  🟢 P3 - 轻微影响，可计划性处理                                                 │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

---

<!-- chunk: 二、顶部事件详细分解 -->## 二、顶部事件详细分解

## 2.1 顶部事件1: 集群完全不可用 🔴 P0

> **问题定义**: 整个Kubernetes集群无法正常工作，kubectl无法连接，所有应用服务中断
> **业务影响**: 所有业务完全中断，数据可能丢失
> **响应时间**: 立即响应(15分钟内)

## 中间事件 IE-1.1: 控制平面问题

**问题现象**: kubectl命令超时或返回连接错误，无法获取集群资源

```
IE-1.1 控制平面问题 [OR门]
│
├── BE-1.1 API Server问题
├── BE-1.2 etcd集群问题
├── BE-1.3 Scheduler问题
└── BE-1.4 Controller Manager问题
```

---

## BE-1.1: API Server问题

**问题现象**:
- `kubectl` 命令返回 `Unable to connect to the server: dial tcp <ip>:6443: connect: connection refused`
- `kubectl` 命令超时无响应
- API Server Pod处于 `CrashLoopBackOff` 或 `Error` 状态

**可能原因**:
- API Server证书过期或无效
- etcd连接失败，API Server无法访问数据存储
- API Server资源配置不足(CPU/内存)
- API Server配置文件错误
- 网络问题导致API Server端口不可达
- 准入控制器Webhook配置错误导致启动失败

**排查命令**:
```bash
# 检查API Server Pod状态
kubectl get pods -n kube-system | grep kube-apiserver

# 检查API Server日志
kubectl logs -n kube-system kube-apiserver-<node-name>
# 或直接在Master节点查看
journalctl -u kube-apiserver -f

# 检查API Server证书有效期
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# 检查etcd连接
kubectl get componentstatuses

# 检查API Server进程
ps aux | grep kube-apiserver

# 检查端口监听
netstat -tlnp | grep 6443

# 测试API Server连通性
curl -k https://<apiserver-ip>:6443/healthz

# 检查资源使用
top -p $(pgrep kube-apiserver)
```

**解决方案**:
1. **证书过期**: 使用 `kubeadm certs renew all` 续期证书，重启API Server
2. **etcd连接失败**: 检查etcd集群健康状态，修复etcd问题
3. **资源不足**: 增加Master节点资源或调整API Server资源限制
4. **配置错误**: 检查 `/etc/kubernetes/manifests/kube-apiserver.yaml` 配置
5. **Webhook问题**: 临时禁用问题Webhook，修复后重新启用

---

## BE-1.2: etcd集群问题

**问题现象**:
- API Server日志显示etcd连接错误
- etcd Pod处于异常状态
- `etcdctl endpoint health` 显示节点不健康
- 集群状态变为只读或无法写入

**可能原因**:
- etcd集群多数节点问题(失去quorum)
- etcd数据目录损坏
- etcd磁盘空间不足
- 网络分区导致etcd节点间无法通信
- etcd证书过期
- etcd版本不兼容

**排查命令**:
```bash
# 检查etcd Pod状态
kubectl get pods -n kube-system | grep etcd

# 检查etcd日志
kubectl logs -n kube-system etcd-<node-name>
journalctl -u etcd -f

# 检查etcd集群健康状态(在etcd容器内执行)
etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key \
        endpoint health --cluster

# 检查etcd集群成员列表
etcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key \
        member list

# 检查etcd数据目录
du -sh /var/lib/etcd
ls -la /var/lib/etcd/member/

# 检查磁盘空间
df -h

# 检查etcd证书有效期
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates
```

**解决方案**:
1. **失去quorum**: 从备份恢复etcd数据，或重新初始化集群
2. **数据损坏**: 停止etcd，删除数据目录，从备份恢复
3. **磁盘空间不足**: 清理磁盘或扩展存储，清理etcd历史数据
4. **网络分区**: 修复网络连接，确保etcd节点互通
5. **证书过期**: 续期etcd证书，重启etcd服务

---

## BE-1.3: Scheduler问题

**问题现象**:
- 新Pod一直处于Pending状态，无法调度
- Scheduler Pod处于异常状态
- 调度器日志显示错误

**可能原因**:
- Scheduler配置文件错误
- API Server连接失败
- Leader选举失败(多副本场景)
- 调度器资源不足

**排查命令**:
```bash
# 检查Scheduler Pod状态
kubectl get pods -n kube-system | grep kube-scheduler

# 查看Scheduler日志
kubectl logs -n kube-system kube-scheduler-<node-name>
journalctl -u kube-scheduler -f

# 检查调度器配置
cat /etc/kubernetes/manifests/kube-scheduler.yaml

# 检查Leader选举状态
kubectl get leases -n kube-system kube-scheduler -o yaml

# 检查Pending Pod的调度事件
kubectl describe pod <pod-name> | grep -A 10 Events
```

**解决方案**:
1. **配置错误**: 修正调度器配置文件
2. **API Server连接**: 检查API Server可用性
3. **Leader选举**: 删除Lease资源强制重新选举
4. **重启调度器**: 删除Scheduler Pod让其重新创建

---

## BE-1.4: Controller Manager问题

**问题现象**:
- Deployment/ReplicaSet无法创建Pod
- Service无法创建Endpoint
- Node状态不更新
- Controller Manager Pod异常

**可能原因**:
- Controller Manager配置错误
- API Server连接失败
- Leader选举失败
- 控制器资源不足

**排查命令**:
```bash
# 检查Controller Manager Pod状态
kubectl get pods -n kube-system | grep kube-controller-manager

# 查看Controller Manager日志
kubectl logs -n kube-system kube-controller-manager-<node-name>
journalctl -u kube-controller-manager -f

# 检查Leader选举状态
kubectl get leases -n kube-system kube-controller-manager -o yaml

# 检查Deployment状态
kubectl get deployments --all-namespaces
kubectl describe deployment <deployment-name>

# 检查ReplicaSet
kubectl get rs --all-namespaces
```

**解决方案**:
1. **配置错误**: 修正Controller Manager配置
2. **重启控制器**: 删除Controller Manager Pod重新创建
3. **检查资源**: 确保控制器有足够资源

---

## 中间事件 IE-1.2: 工作节点批量问题

**问题现象**: 多个工作节点同时变为NotReady状态，节点上的Pod无法访问

```
IE-1.2 工作节点批量问题 [AND门 - 多数节点]
│
├── BE-1.5 Kubelet服务问题
├── BE-1.6 容器运行时问题
└── BE-1.7 节点网络问题
```

---

## BE-1.5: Kubelet服务问题

**问题现象**:
- 节点状态显示 `NotReady`
- `kubectl get nodes` 显示节点异常
- 节点上Pod状态不更新

**可能原因**:
- Kubelet服务停止或崩溃
- Kubelet配置错误
- API Server证书问题导致无法连接
- 节点磁盘压力(DiskPressure)
- 节点内存压力(MemoryPressure)
- 节点PID压力(PIDPressure)

**排查命令**:
```bash
# 检查Kubelet服务状态
systemctl status kubelet

# 查看Kubelet日志
journalctl -u kubelet -f

# 检查Kubelet配置
cat /var/lib/kubelet/config.yaml
cat /etc/kubernetes/kubelet.conf

# 检查节点状态
kubectl describe node <node-name>

# 检查节点条件
kubectl get node <node-name> -o yaml | grep -A 20 conditions

# 检查磁盘空间
df -h

# 检查内存使用
free -h

# 检查Kubelet证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

**解决方案**:
1. **服务停止**: `systemctl restart kubelet`
2. **配置错误**: 修正Kubelet配置文件后重启
3. **证书问题**: 更新Kubelet证书
4. **磁盘压力**: 清理磁盘空间，删除无用镜像/容器
5. **内存压力**: 释放内存或增加节点资源

---

## BE-1.6: 容器运行时问题

**问题现象**:
- Kubelet日志显示容器运行时连接错误
- 无法创建/启动容器
- `crictl`/`docker` 命令无法执行

**可能原因**:
- Docker/containerd服务停止
- 容器运行时配置错误
- 容器运行时数据目录损坏
- 容器运行时资源不足

**排查命令**:
```bash
# 检查Docker状态
systemctl status docker

# 检查containerd状态
systemctl status containerd

# 查看Docker日志
journalctl -u docker -f

# 查看containerd日志
journalctl -u containerd -f

# 检查容器运行时socket
ls -la /var/run/docker.sock
ls -la /run/containerd/containerd.sock

# 使用crictl检查(cri接口)
crictl version
crictl ps
crictl pods

# 检查容器运行时数据目录
du -sh /var/lib/docker
du -sh /var/lib/containerd
```

**解决方案**:
1. **服务停止**: `systemctl restart docker` 或 `systemctl restart containerd`
2. **配置错误**: 修正daemon.json配置后重启
3. **数据损坏**: 清理数据目录(会丢失容器数据)
4. **资源不足**: 清理无用镜像和容器

---

## BE-1.7: 节点网络问题

**问题现象**:
- 节点网络接口异常
- Pod无法分配IP
- 节点间无法通信

**可能原因**:
- 网络接口问题
- CNI插件未正确安装或配置
- 网络策略阻止关键流量
- 防火墙规则错误
- IP地址池耗尽

**排查命令**:
```bash
# 检查网络接口
ip addr
ip link

# 检查路由表
ip route

# 检查CNI配置
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conf

# 检查CNI插件
ls -la /opt/cni/bin/

# 检查iptables规则
iptables -L -n -v
iptables -t nat -L -n -v

# 检查IPVS规则(ipvs模式)
ipvsadm -Ln

# 检查网桥
cni0网桥
ip link show cni0
brctl show

# 检查Pod CIDR
kubectl get node <node-name> -o jsonpath='{.spec.podCIDR}'
```

**解决方案**:
1. **网络接口**: 重启网络服务或接口
2. **CNI问题**: 重新安装/配置CNI插件
3. **IP池耗尽**: 扩展Pod CIDR范围
4. **防火墙**: 调整防火墙规则允许K8s流量

---

## 中间事件 IE-1.3: 网络基础设施问题

**问题现象**: 集群网络完全不可用，节点间无法通信

```
IE-1.3 网络基础设施问题 [OR门]
│
├── BE-1.8 CNI插件问题
└── BE-1.9 核心网络设备问题
```

---

## BE-1.8: CNI插件问题

**问题现象**:
- Pod无法分配IP地址
- Pod网络不通
- CNI Pod处于异常状态

**可能原因**:
- CNI Pod崩溃或无法启动
- CNI配置错误
- CNI与Kubernetes版本不兼容
- 底层网络问题

**排查命令**:
```bash
# 检查CNI Pod状态(Calico/Flannel/Weave等)
kubectl get pods -n kube-system | grep -E 'calico|flannel|weave|cilium'

# 查看CNI Pod日志
kubectl logs -n kube-system <cni-pod-name>

# 检查CNI配置
cat /etc/cni/net.d/10-*.conf

# 检查CNI二进制文件
ls -la /opt/cni/bin/

# 检查Calico特定配置(Calico场景)
kubectl get ippool
calicoctl node status

# 检查Flannel配置(Flannel场景)
kubectl get configmap kube-flannel-cfg -n kube-system -o yaml
```

**解决方案**:
1. **CNI Pod异常**: 删除CNI Pod让其重新创建
2. **配置错误**: 修正CNI配置并重新部署
3. **版本不兼容**: 升级/降级CNI版本
4. **完全重装**: 删除并重新安装CNI插件

---

## BE-1.9: 核心网络设备问题

**问题现象**:
- 整个集群网络中断
- 节点间无法ping通
- 外部无法访问集群

**可能原因**:
- 物理网络设备问题(交换机/路由器)
- 云厂商网络服务异常
- VPC/子网配置错误
- 负载均衡器问题

**排查命令**:
```bash
# 测试节点间连通性
ping <node-ip>

# 测试Pod间连通性
kubectl run test --image=busybox --rm -it -- ping <pod-ip>

# 检查路由
ip route
traceroute <destination>

# 检查云厂商网络状态(云环境)
# AWS: 检查VPC、子网、路由表
# Azure: 检查VNet、子网
# GCP: 检查VPC网络

# 检查负载均衡器状态
kubectl get svc -n ingress-nginx
kubectl get ingress --all-namespaces
```

**解决方案**:
1. **物理设备**: 联系网络团队或厂商修复
2. **云服务**: 检查云厂商状态页面，提交工单
3. **VPC配置**: 修正VPC/子网/路由表配置
4. **负载均衡器**: 重启或重新配置负载均衡器

---

## 2.2 顶部事件2: 应用服务不可用 🔴 P0

> **问题定义**: 用户无法正常访问应用服务，HTTP请求失败或超时
> **业务影响**: 业务功能不可用，用户体验受损
> **响应时间**: 立即响应(30分钟内)

## 中间事件 IE-2.1: Pod运行异常

```
IE-2.1 Pod运行异常 [OR门]
│
├── BE-2.1 CrashLoopBackOff
├── BE-2.2 ImagePullBackOff
├── BE-2.3 OOMKilled
└── BE-2.4 Evicted
```

---

## BE-2.1: CrashLoopBackOff

**问题现象**:
- Pod状态显示 `CrashLoopBackOff`
- Pod反复重启
- 应用容器启动后立即退出

**可能原因**:
- 应用程序启动错误(配置错误/依赖缺失)
- 健康检查配置错误
- 资源限制过严导致启动失败
- 启动命令/参数错误
- 环境变量缺失或错误
- 依赖服务不可用
- 权限不足无法访问文件

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看Pod状态和重启次数
kubectl get pod <pod-name> -o wide

# 查看Pod详细描述
kubectl describe pod <pod-name>

# 查看容器日志(当前)
kubectl logs <pod-name>

# 查看容器日志(之前崩溃的实例)
kubectl logs <pod-name> --previous

# 查看Pod事件
kubectl get events --field-selector involvedObject.name=<pod-name>

# 进入容器调试(如果可以启动)
kubectl exec -it <pod-name> -- /bin/sh

# 检查Pod配置
kubectl get pod <pod-name> -o yaml

# 检查Deployment配置
kubectl get deployment <deployment-name> -o yaml
```

**解决方案**:
1. **应用错误**: 修复应用程序代码或配置
2. **健康检查**: 调整livenessProbe/readinessProbe配置
3. **资源限制**: 增加memory limit和request
4. **启动命令**: 修正command/args配置
5. **环境变量**: 添加/修正环境变量
6. **依赖服务**: 确保依赖服务可用

---

## BE-2.2: ImagePullBackOff

**问题现象**:
- Pod状态显示 `ImagePullBackOff` 或 `ErrImagePull`
- 事件显示镜像拉取失败

**可能原因**:
- 镜像名称或标签错误
- 镜像在仓库中不存在
- 镜像仓库认证失败
- 网络问题无法访问镜像仓库
- 镜像仓库服务不可用
- 私有仓库Secret配置错误

**排查命令**:
```bash
# 查看Pod事件
kubectl describe pod <pod-name> | grep -A 5 Events

# 查看Pod使用的镜像
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].image}'

# 检查镜像Pull Secret
kubectl get pod <pod-name> -o jsonpath='{.spec.imagePullSecrets}'
kubectl get secret <secret-name> -o yaml

# 手动测试镜像拉取
docker pull <image-name>:<tag>

# 检查Docker配置(节点上)
cat ~/.docker/config.json

# 检查私有仓库认证
echo <base64-encoded-auth> | base64 -d
```

**解决方案**:
1. **镜像名称错误**: 修正镜像名称和标签
2. **镜像不存在**: 推送正确镜像到仓库
3. **认证失败**: 创建/更新imagePullSecret
4. **网络问题**: 检查节点到镜像仓库的网络连通性
5. **仓库不可用**: 等待仓库恢复或使用备用仓库

---

## BE-2.3: OOMKilled

**问题现象**:
- Pod状态显示 `OOMKilled`
- 容器退出代码为137(128+9 SIGKILL)
- Pod频繁重启

**可能原因**:
- 内存限制(memory limit)设置过低
- 应用程序内存泄漏
- 应用负载增加导致内存使用上升
- 未正确配置JVM/应用堆内存

**排查命令**:
```bash
# 查看Pod状态
kubectl get pod <pod-name> -o wide

# 查看Pod描述
kubectl describe pod <pod-name>

# 查看容器指标(如果metrics-server可用)
kubectl top pod <pod-name>

# 查看Pod资源使用历史(如果Prometheus可用)
# 查询container_memory_usage_bytes等指标

# 查看Pod配置中的资源限制
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].resources}'

# 查看节点内存使用情况
kubectl top node <node-name>
free -h
```

**解决方案**:
1. **增加内存限制**: 提高Pod的memory limit
2. **内存泄漏**: 修复应用程序内存泄漏问题
3. **JVM调优**: 正确配置JVM堆内存参数
4. **水平扩展**: 增加Pod副本数分散负载
5. **垂直扩展**: 使用VPA自动调整资源

---

## BE-2.4: Evicted

**问题现象**:
- Pod状态显示 `Evicted`
- Pod被驱逐出节点
- 事件显示驱逐原因

**可能原因**:
- 节点磁盘压力(DiskPressure)
- 节点内存压力(MemoryPressure)
- 节点PID压力(PIDPressure)
- 节点不可达
- 优先级抢占(高优先级Pod抢占低优先级Pod)
- 污点驱逐(Taint-based eviction)

**排查命令**:
```bash
# 查看被驱逐的Pod
kubectl get pods --all-namespaces | grep Evicted

# 查看驱逐事件
kubectl get events --field-selector reason=Evicted

# 查看节点状态
kubectl describe node <node-name>

# 查看节点条件
kubectl get node <node-name> -o yaml | grep -A 30 conditions

# 检查节点资源使用
kubectl top node <node-name>
df -h
free -h

# 查看Pod优先级
kubectl get pod <pod-name> -o jsonpath='{.spec.priorityClassName}'
```

**解决方案**:
1. **磁盘压力**: 清理节点磁盘空间，删除无用镜像/日志
2. **内存压力**: 释放内存或增加节点资源
3. **PID压力**: 增加节点PID限制或分散Pod
4. **优先级问题**: 调整Pod优先级或资源配额
5. **污点驱逐**: 检查并调整节点污点配置

---

## 中间事件 IE-2.2: Service访问异常

```
IE-2.2 Service访问异常 [OR门]
│
├── BE-2.5 无可用Endpoint
├── BE-2.6 端口配置错误
└── BE-2.7 kube-proxy问题
```

---

## BE-2.5: 无可用Endpoint

**问题现象**:
- Service无Endpoint
- 访问Service返回连接拒绝
- `kubectl get endpoints` 显示为空

**可能原因**:
- 无匹配的Pod(Selector不匹配)
- Pod未通过健康检查
- Pod处于非Running状态
- 所有Pod都被驱逐或删除

**排查命令**:
```bash
# 查看Service详情
kubectl describe svc <service-name>

# 查看Endpoint
kubectl get endpoints <service-name>
kubectl get endpoints <service-name> -o yaml

# 查看Service Selector
kubectl get svc <service-name> -o jsonpath='{.spec.selector}'

# 查看匹配的Pod
kubectl get pods -l <selector-key>=<selector-value>

# 查看Pod状态
kubectl get pods --all-namespaces -o wide

# 检查Pod健康检查
kubectl describe pod <pod-name> | grep -A 5 "Liveness\|Readiness"
```

**解决方案**:
1. **Selector不匹配**: 修正Service的Selector或Pod的Label
2. **Pod未就绪**: 检查Pod健康检查配置和状态
3. **无Pod**: 创建或恢复Pod
4. **健康检查失败**: 调整健康检查参数或修复应用

---

## BE-2.6: 端口配置错误

**问题现象**:
- Service端口与Pod端口不匹配
- 访问Service返回连接错误

**可能原因**:
- Service的targetPort与Pod暴露端口不一致
- Service端口配置错误
- Pod容器端口配置错误

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看Service端口配置
kubectl get svc <service-name> -o yaml | grep -A 10 ports

# 查看Pod端口配置
kubectl get pod <pod-name> -o yaml | grep -A 10 containerPort

# 测试Pod端口连通性
kubectl exec -it <pod-name> -- netstat -tlnp

# 测试Service连通性
kubectl run test --image=busybox --rm -it -- wget -O- <service-ip>:<port>
```

**解决方案**:
1. **修正targetPort**: 确保Service targetPort与Pod端口一致
2. **修正Service端口**: 配置正确的Service端口
3. **修正Pod端口**: 确保Pod容器暴露正确端口

---

## BE-2.7: kube-proxy问题

**问题现象**:
- Service无法访问
- iptables/IPVS规则未正确创建
- kube-proxy Pod异常

**可能原因**:
- kube-proxy Pod崩溃
- kube-proxy配置错误
- iptables规则冲突
- IPVS模块未加载

**排查命令**:
```bash
# 检查kube-proxy Pod状态
kubectl get pods -n kube-system | grep kube-proxy

# 查看kube-proxy日志
kubectl logs -n kube-system kube-proxy-<node-name>

# 检查kube-proxy配置
kubectl get configmap kube-proxy -n kube-system -o yaml

# 检查iptables规则
iptables -t nat -L KUBE-SERVICES -n -v

# 检查IPVS规则
ipvsadm -Ln

# 检查IPVS模块
lsmod | grep ip_vs

# 检查kube-proxy模式
kubectl get configmap kube-proxy -n kube-system -o yaml | grep mode
```

**解决方案**:
1. **重启kube-proxy**: 删除kube-proxy Pod重新创建
2. **切换模式**: 在iptables和IPVS模式间切换
3. **清理规则**: 清理冲突的iptables规则
4. **加载模块**: 加载必要的IPVS内核模块

---

## 中间事件 IE-2.3: Ingress访问异常

```
IE-2.3 Ingress访问异常 [OR门]
│
├── BE-2.8 Ingress Controller问题
├── BE-2.9 Ingress规则配置错误
└── BE-2.10 负载均衡器问题
```

---

## BE-2.8: Ingress Controller问题

**问题现象**:
- Ingress Controller Pod异常
- 无法访问Ingress路由
- 返回404/502/503错误

**可能原因**:
- Ingress Controller Pod崩溃
- Ingress Controller配置错误
- Ingress Controller资源不足
- 与API Server连接失败

**排查命令**:
```bash
# 检查Ingress Controller Pod状态
kubectl get pods -n ingress-nginx | grep controller

# 查看Ingress Controller日志
kubectl logs -n ingress-nginx ingress-nginx-controller-<pod-id>

# 检查Ingress Controller配置
kubectl get configmap ingress-nginx-controller -n ingress-nginx -o yaml

# 检查Ingress资源
kubectl get ingress --all-namespaces
kubectl describe ingress <ingress-name>

# 检查Ingress Controller服务
kubectl get svc -n ingress-nginx
```

**解决方案**:
1. **重启Controller**: 删除Ingress Controller Pod重新创建
2. **修正配置**: 检查并修正Ingress Controller配置
3. **增加资源**: 增加Ingress Controller资源限制
4. **检查连接**: 确保Controller能连接API Server

---

## BE-2.9: Ingress规则配置错误

**问题现象**:
- Ingress规则不生效
- 路由匹配错误
- 返回404错误

**可能原因**:
- host配置错误
- path配置错误
- backend service配置错误
- TLS证书配置错误
- 注解配置错误

**排查命令**:
```bash
# 查看Ingress配置
kubectl get ingress <ingress-name> -o yaml

# 检查backend service
kubectl get svc <backend-service>

# 检查service endpoints
kubectl get endpoints <backend-service>

# 测试直接访问service
kubectl run test --image=busybox --rm -it -- wget -O- <service-ip>:<port>

# 检查TLS证书
kubectl get secret <tls-secret> -o yaml
openssl x509 -in <(kubectl get secret <tls-secret> -o jsonpath='{.data.tls\.crt}' | base64 -d) -noout -text
```

**解决方案**:
1. **修正host**: 确保host配置与访问域名一致
2. **修正path**: 确保path配置正确
3. **修正backend**: 确保backend service和port正确
4. **修正TLS**: 确保证书和密钥正确
5. **修正注解**: 检查Ingress注解配置

---

## BE-2.10: 负载均衡器问题

**问题现象**:
- 外部无法访问Ingress
- 负载均衡器状态异常
- 云厂商LB服务异常

**可能原因**:
- 云厂商负载均衡器配置错误
- 负载均衡器健康检查失败
- 负载均衡器证书问题
- 云厂商服务异常

**排查命令**:
```bash
# 检查Ingress Controller Service
kubectl get svc -n ingress-nginx ingress-nginx-controller

# 检查LoadBalancer状态
kubectl describe svc -n ingress-nginx ingress-nginx-controller | grep -A 5 "LoadBalancer"

# 检查云厂商LB状态(云环境)
# AWS: 检查ELB/ALB状态
# Azure: 检查Load Balancer状态
# GCP: 检查Forwarding Rule状态

# 检查安全组/防火墙规则
# 确保80/443端口开放
```

**解决方案**:
1. **检查LB配置**: 在云控制台检查负载均衡器配置
2. **健康检查**: 确保健康检查路径正确
3. **证书配置**: 配置正确的SSL证书
4. **安全组**: 确保安全组允许流量
5. **云厂商支持**: 联系云厂商技术支持

---

## 2.3 顶部事件3: Pod启动失败 🟠 P1

> **问题定义**: Pod无法成功启动，长时间处于Pending或其他非Running状态
> **业务影响**: 新应用无法部署，扩容失败
> **响应时间**: 1小时内响应

## 中间事件 IE-3.1: 调度失败

```
IE-3.1 调度失败 [OR门]
│
├── BE-3.1 节点资源不足
├── BE-3.2 节点选择器不匹配
├── BE-3.3 污点阻止调度
└── BE-3.4 资源配额超限
```

---

## BE-3.1: 节点资源不足

**问题现象**:
- Pod处于Pending状态
- 调度事件显示资源不足
- `kubectl describe pod` 显示Insufficient资源

**可能原因**:
- CPU资源不足
- 内存资源不足
- 临时存储(ephemeral-storage)不足
- 可调度Pod数量限制

**排查命令**:
```bash
# 查看Pod调度事件
kubectl describe pod <pod-name> | grep -A 10 Events

# 查看节点资源分配
kubectl describe node <node-name> | grep -A 20 "Allocated resources"

# 查看节点资源使用
kubectl top node

# 查看节点可分配资源
kubectl get node <node-name> -o jsonpath='{.status.allocatable}'

# 查看节点已分配资源
kubectl get node <node-name> -o jsonpath='{.status.capacity}'

# 计算资源使用率
kubectl describe node | grep -E "Name:|Allocated resources:|pods|cpu|memory"
```

**解决方案**:
1. **增加节点**: 扩容集群添加新节点
2. **降低资源请求**: 调整Pod的resource request
3. **清理资源**: 删除无用Pod释放资源
4. **节点升级**: 升级节点规格
5. **Pod重调度**: 使用亲和性/反亲和性优化调度

---

## BE-3.2: 节点选择器不匹配

**问题现象**:
- Pod处于Pending状态
- 调度事件显示节点Selector不匹配

**可能原因**:
- nodeSelector配置的Label不存在
- 节点Label配置错误
- Pod的nodeAffinity配置错误

**排查命令**:
```bash
# 查看Pod的nodeSelector
kubectl get pod <pod-name> -o jsonpath='{.spec.nodeSelector}'

# 查看Pod的nodeAffinity
kubectl get pod <pod-name> -o yaml | grep -A 20 nodeAffinity

# 查看节点Label
kubectl get node --show-labels

# 查看特定节点的Label
kubectl get node <node-name> --show-labels

# 检查匹配的节点
kubectl get nodes -l <selector-key>=<selector-value>
```

**解决方案**:
1. **修正nodeSelector**: 使用正确的节点Label
2. **添加Label**: 给节点添加缺失的Label
3. **修正affinity**: 调整nodeAffinity规则
4. **删除限制**: 如果不需要，删除节点选择限制

---

## BE-3.3: 污点阻止调度

**问题现象**:
- Pod处于Pending状态
- 调度事件显示污点容忍不匹配

**可能原因**:
- 节点有Taint，Pod没有对应的Toleration
- Toleration配置错误
- 污点配置过于严格

**排查命令**:
```bash
# 查看Pod的Tolerations
kubectl get pod <pod-name> -o jsonpath='{.spec.tolerations}'

# 查看节点Taints
kubectl get node <node-name> -o jsonpath='{.spec.taints}'

# 查看所有节点的Taints
kubectl get nodes -o custom-columns=NAME:.metadata.name,TAINTS:.spec.taints

# 查看Pod调度事件
kubectl describe pod <pod-name> | grep -i taint
```

**解决方案**:
1. **添加Toleration**: 给Pod添加对应的Toleration
2. **移除Taint**: 从节点移除不必要的Taint
3. **修正Toleration**: 确保Toleration与Taint匹配
4. **使用NoSchedule**: 使用正确的污点效果

---

## BE-3.4: 资源配额超限

**问题现象**:
- Pod处于Pending状态
- 事件显示Quota exceeded
- 无法创建新资源

**可能原因**:
- ResourceQuota限制达到上限
- LimitRange限制
- 命名空间级别配额超限

**排查命令**:
```bash
# 查看ResourceQuota
kubectl get resourcequota -n <namespace>
kubectl describe resourcequota <quota-name> -n <namespace>

# 查看LimitRange
kubectl get limitrange -n <namespace>
kubectl describe limitrange <limitrange-name> -n <namespace>

# 查看命名空间资源使用
kubectl describe namespace <namespace>

# 查看配额事件
kubectl get events -n <namespace> | grep -i quota
```

**解决方案**:
1. **增加配额**: 提高ResourceQuota限制
2. **清理资源**: 删除无用资源释放配额
3. **调整LimitRange**: 修改LimitRange配置
4. **新命名空间**: 在新命名空间部署

---

## 中间事件 IE-3.2: 镜像拉取失败

```
IE-3.2 镜像拉取失败 [OR门]
│
├── BE-3.5 镜像不存在
├── BE-3.6 镜像仓库认证失败
└── BE-3.7 网络不可达
```

---

## BE-3.5: 镜像不存在

**问题现象**:
- Pod状态 `ErrImagePull` 或 `ImagePullBackOff`
- 事件显示manifest unknown或repository not found

**可能原因**:
- 镜像名称拼写错误
- 镜像标签不存在
- 镜像已被删除
- 镜像仓库路径错误

**排查命令**:
```bash
# 查看Pod使用的镜像
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].image}'

# 查看Pod事件
kubectl describe pod <pod-name> | grep -A 5 Events

# 手动验证镜像存在
docker pull <image>:<tag>  # 或 crictl pull

# 在镜像仓库Web界面搜索镜像
# Docker Hub: https://hub.docker.com
# Harbor: 登录Harbor查看
```

**解决方案**:
1. **修正镜像名**: 使用正确的镜像名称
2. **修正标签**: 使用存在的镜像标签
3. **推送镜像**: 将镜像推送到仓库
4. **检查路径**: 确保镜像路径正确

---

## BE-3.6: 镜像仓库认证失败

**问题现象**:
- Pod状态 `ImagePullBackOff`
- 事件显示unauthorized或authentication required

**可能原因**:
- imagePullSecret不存在
- imagePullSecret配置错误
- 仓库认证信息过期
- ServiceAccount未绑定imagePullSecret

**排查命令**:
```bash
# 查看Pod的imagePullSecrets
kubectl get pod <pod-name> -o jsonpath='{.spec.imagePullSecrets}'

# 查看Secret内容
kubectl get secret <secret-name> -o yaml

# 解码Secret
echo '<base64-data>' | base64 -d

# 查看ServiceAccount
kubectl get sa default -o yaml

# 验证认证信息
docker login <registry> -u <username> -p <password>
```

**解决方案**:
1. **创建Secret**: 创建正确的imagePullSecret
2. **更新Secret**: 更新过期的认证信息
3. **绑定SA**: 将Secret绑定到ServiceAccount
4. **检查权限**: 确保账号有拉取权限

---

## BE-3.7: 网络不可达

**问题现象**:
- Pod状态 `ErrImagePull`
- 事件显示timeout或connection refused

**可能原因**:
- 节点无法访问镜像仓库
- 防火墙阻止出站连接
- DNS解析失败
- 代理配置错误

**排查命令**:
```bash
# 测试网络连通性(在节点上执行)
ping <registry-host>
curl -v <registry-url>

# 测试DNS解析
nslookup <registry-host>
dig <registry-host>

# 检查防火墙规则
iptables -L -n | grep <registry-port>

# 检查代理配置
echo $HTTP_PROXY
echo $HTTPS_PROXY
cat /etc/systemd/system/docker.service.d/proxy.conf

# 检查CNI网络
cat /etc/resolv.conf
```

**解决方案**:
1. **检查网络**: 确保节点能访问镜像仓库
2. **配置防火墙**: 开放必要的出站端口
3. **配置DNS**: 确保DNS能解析仓库域名
4. **配置代理**: 正确配置HTTP/HTTPS代理
5. **使用镜像仓库代理**: 配置本地镜像仓库缓存

---

## 中间事件 IE-3.3: 容器创建失败

```
IE-3.3 容器创建失败 [OR门]
│
├── BE-3.8 CNI配置失败
├── BE-3.9 存储挂载失败
└── BE-3.10 Init容器失败
```

---

## BE-3.8: CNI配置失败

**问题现象**:
- Pod处于ContainerCreating状态
- 事件显示CNI相关错误
- Pod无法分配IP

**可能原因**:
- CNI插件未安装
- CNI配置错误
- CNI二进制文件缺失
- IP地址池耗尽

**排查命令**:
```bash
# 查看Pod事件
kubectl describe pod <pod-name> | grep -A 10 Events

# 检查CNI配置
ls -la /etc/cni/net.d/
cat /etc/cni/net.d/*.conf

# 检查CNI二进制
ls -la /opt/cni/bin/

# 检查CNI Pod状态
kubectl get pods -n kube-system | grep -E 'calico|flannel|weave|cilium'

# 检查IP池
kubectl get ippools  # Calico
kubectl get configmap kube-flannel-cfg -n kube-system -o yaml  # Flannel

# 查看kubelet日志
journalctl -u kubelet | grep -i cni
```

**解决方案**:
1. **安装CNI**: 安装CNI插件
2. **修正配置**: 修复CNI配置文件
3. **扩展IP池**: 扩大Pod CIDR范围
4. **重启CNI**: 重启CNI Pod

---

## BE-3.9: 存储挂载失败

**问题现象**:
- Pod处于ContainerCreating状态
- 事件显示mount volume失败
- PVC无法绑定或挂载

**可能原因**:
- PVC未绑定
- PV不存在或不可用
- 存储后端问题
- 挂载参数错误
- 权限不足

**排查命令**:
```bash
# 查看Pod事件
kubectl describe pod <pod-name> | grep -A 10 Events

# 检查PVC状态
kubectl get pvc <pvc-name>
kubectl describe pvc <pvc-name>

# 检查PV状态
kubectl get pv <pv-name>
kubectl describe pv <pv-name>

# 检查StorageClass
kubectl get sc
kubectl describe sc <sc-name>

# 检查CSI Pod
kubectl get pods -n kube-system | grep csi

# 查看CSI日志
kubectl logs -n kube-system <csi-pod-name>
```

**解决方案**:
1. **绑定PVC**: 确保PVC正确绑定到PV
2. **创建PV**: 手动创建或使用动态供应
3. **修复存储**: 修复存储后端问题
4. **修正参数**: 调整挂载参数
5. **检查权限**: 确保有挂载权限

---

## BE-3.10: Init容器失败

**问题现象**:
- Pod处于Init状态
- Init容器反复重启
- 主容器未启动

**可能原因**:
- Init容器命令执行失败
- Init容器依赖服务不可用
- Init容器配置错误
- Init容器资源不足

**排查命令**:
```bash
# 查看Pod状态
kubectl get pod <pod-name>

# 查看Pod详情
kubectl describe pod <pod-name>

# 查看Init容器日志
kubectl logs <pod-name> -c <init-container-name>

# 查看所有容器状态
kubectl get pod <pod-name> -o jsonpath='{.status.initContainerStatuses}'

# 检查Init容器配置
kubectl get pod <pod-name> -o yaml | grep -A 30 initContainers
```

**解决方案**:
1. **修复命令**: 修正Init容器执行命令
2. **检查依赖**: 确保依赖服务可用
3. **增加资源**: 增加Init容器资源限制
4. **调整顺序**: 优化Init容器执行顺序

---

## 2.4 顶部事件4: 网络通信异常 🟠 P1

> **问题定义**: 集群内网络通信异常，包括DNS解析、Pod间通信、外部访问等问题
> **业务影响**: 服务间调用失败，外部访问中断
> **响应时间**: 1小时内响应

## 中间事件 IE-4.1: DNS解析异常

```
IE-4.1 DNS解析异常 [OR门]
│
├── BE-4.1 CoreDNS Pod问题
├── BE-4.2 DNS配置错误
└── BE-4.3 网络策略阻止DNS
```

---

## BE-4.1: CoreDNS Pod问题

**问题现象**:
- Pod内DNS解析失败
- `nslookup` 超时
- CoreDNS Pod异常

**可能原因**:
- CoreDNS Pod崩溃
- CoreDNS配置错误
- CoreDNS资源不足
- CoreDNS与上游DNS通信失败

**排查命令**:
```bash
# 检查CoreDNS Pod状态
kubectl get pods -n kube-system | grep coredns

# 查看CoreDNS日志
kubectl logs -n kube-system coredns-<pod-id>

# 检查CoreDNS配置
kubectl get configmap coredns -n kube-system -o yaml

# 测试DNS解析
kubectl run test --image=busybox --rm -it -- nslookup kubernetes.default

# 检查CoreDNS Service
kubectl get svc kube-dns -n kube-system

# 查看DNS服务IP
kubectl get svc kube-dns -n kube-system -o jsonpath='{.spec.clusterIP}'
```

**解决方案**:
1. **重启CoreDNS**: 删除CoreDNS Pod重新创建
2. **增加资源**: 增加CoreDNS资源限制
3. **修正配置**: 修复CoreDNS配置
4. **检查上游**: 确保上游DNS可用

---

## BE-4.2: DNS配置错误

**问题现象**:
- 特定域名解析失败
- 解析结果不正确
- 解析超时

**可能原因**:
- CoreDNS配置错误
- Pod的dnsPolicy配置错误
- Pod的dnsConfig配置错误
- /etc/resolv.conf配置错误

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看Pod的DNS配置
kubectl get pod <pod-name> -o jsonpath='{.spec.dnsPolicy}'
kubectl get pod <pod-name> -o jsonpath='{.spec.dnsConfig}'

# 查看Pod的resolv.conf
kubectl exec <pod-name> -- cat /etc/resolv.conf

# 查看CoreDNS配置
kubectl get configmap coredns -n kube-system -o yaml

# 测试DNS解析
kubectl exec <pod-name> -- nslookup <domain>
kubectl exec <pod-name> -- dig <domain>
```

**解决方案**:
1. **修正Corefile**: 修复CoreDNS Corefile配置
2. **调整dnsPolicy**: 设置正确的DNS策略
3. **配置dnsConfig**: 添加自定义DNS配置
4. **添加hosts**: 使用hostAliases添加静态解析

---

## BE-4.3: 网络策略阻止DNS

**问题现象**:
- DNS解析失败
- 网络策略阻止UDP 53端口

**可能原因**:
- NetworkPolicy阻止DNS流量
- 防火墙规则阻止DNS
- CNI网络策略配置错误

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看NetworkPolicy
kubectl get networkpolicy --all-namespaces

# 查看NetworkPolicy详情
kubectl describe networkpolicy <policy-name>

# 检查DNS流量规则
kubectl get networkpolicy -o yaml | grep -A 10 -B 5 53

# 测试DNS连通性
kubectl exec <pod-name> -- nc -zv <dns-ip> 53
```

**解决方案**:
1. **允许DNS流量**: 在NetworkPolicy中允许UDP/TCP 53端口
2. **配置egress规则**: 添加允许访问DNS的egress规则
3. **检查防火墙**: 确保防火墙不阻止DNS流量

---

## 中间事件 IE-4.2: Pod间通信异常

```
IE-4.2 Pod间通信异常 [OR门]
│
├── BE-4.4 CNI插件问题
├── BE-4.5 网络策略阻止
└── BE-4.6 iptables规则错误
```

---

## BE-4.4: CNI插件问题

**问题现象**:
- Pod间无法ping通
- 跨节点Pod通信失败
- Pod IP分配异常

**可能原因**:
- CNI插件未运行
- CNI配置错误
- CNI与Kubernetes版本不兼容
- 底层网络问题

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 检查CNI Pod状态
kubectl get pods -n kube-system | grep -E 'calico|flannel|weave|cilium'

# 查看CNI Pod日志
kubectl logs -n kube-system <cni-pod-name>

# 检查CNI配置
ls -la /etc/cni/net.d/

# 检查Pod IP分配
kubectl get pod -o wide

# 测试Pod间连通性
kubectl exec <pod-a> -- ping <pod-b-ip>

# 检查路由
ip route
```

**解决方案**:
1. **重启CNI**: 重启CNI插件Pod
2. **修正配置**: 修复CNI配置
3. **升级CNI**: 升级CNI到兼容版本
4. **检查网络**: 修复底层网络问题

---

## BE-4.5: 网络策略阻止

**问题现象**:
- 特定Pod间无法通信
- 网络策略阻止合法流量

**可能原因**:
- NetworkPolicy配置过于严格
- NetworkPolicy选择器错误
- 缺少必要的ingress/egress规则

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看所有NetworkPolicy
kubectl get networkpolicy --all-namespaces

# 查看NetworkPolicy详情
kubectl describe networkpolicy <policy-name>

# 检查Pod标签
kubectl get pod <pod-name> --show-labels

# 测试连通性
kubectl exec <source-pod> -- nc -zv <target-ip> <port>
```

**解决方案**:
1. **调整策略**: 放宽NetworkPolicy限制
2. **修正选择器**: 确保选择器匹配正确
3. **添加规则**: 添加必要的ingress/egress规则
4. **删除策略**: 临时删除策略测试连通性

---

## BE-4.6: iptables规则错误

**问题现象**:
- Pod间通信异常
- Service访问失败
- 路由不正确

**可能原因**:
- iptables规则冲突
- kube-proxy规则错误
- 自定义iptables规则干扰
- iptables规则过多导致性能问题

**排查命令**:
```bash
# 查看iptables规则
iptables -L -n -v
iptables -t nat -L -n -v

# 查看KUBE链
iptables -t nat -L KUBE-SERVICES -n -v

# 查看规则数量
iptables -L | wc -l

# 查看kube-proxy日志
kubectl logs -n kube-system kube-proxy-<node-name>

# 检查IPVS规则
ipvsadm -Ln
```

**解决方案**:
1. **清理规则**: 清理冲突的iptables规则
2. **重启kube-proxy**: 重新生成规则
3. **使用IPVS**: 切换到IPVS模式提高性能
4. **检查自定义规则**: 移除干扰的自定义规则

---

## 中间事件 IE-4.3: 集群外部访问异常

```
IE-4.3 集群外部访问异常 [OR门]
│
├── BE-4.7 Egress配置错误
├── BE-4.8 NAT配置问题
└── BE-4.9 防火墙阻止
```

---

## BE-4.7: Egress配置错误

**问题现象**:
- Pod无法访问外部网络
- 外部API调用失败
- 出站连接超时

**可能原因**:
- Egress NetworkPolicy阻止出站流量
- 缺少Egress网关配置
- 外部路由配置错误

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看Egress NetworkPolicy
kubectl get networkpolicy --all-namespaces

# 查看Policy详情
kubectl describe networkpolicy <policy-name>

# 测试外部连通性
kubectl exec <pod-name> -- ping 8.8.8.8
kubectl exec <pod-name> -- curl -v https://www.google.com

# 检查节点路由
ip route
```

**解决方案**:
1. **允许Egress**: 在NetworkPolicy中允许出站流量
2. **配置Egress网关**: 设置Egress网关
3. **检查路由**: 确保外部路由正确

---

## BE-4.8: NAT配置问题

**问题现象**:
- Pod可以ping通外部但TCP/UDP连接失败
- SNAT/DNAT配置错误
- 外部无法访问Service

**可能原因**:
- NAT规则缺失
- NAT规则错误
- 源IP地址问题

**排查命令**:
```bash
# 查看NAT规则
iptables -t nat -L -n -v

# 查看POSTROUTING链
iptables -t nat -L POSTROUTING -n -v

# 查看MASQUERADE规则
iptables -t nat -L | grep MASQUERADE

# 检查IP转发
cat /proc/sys/net/ipv4/ip_forward

# 查看节点IP
ip addr
```

**解决方案**:
1. **添加NAT规则**: 添加正确的MASQUERADE规则
2. **启用IP转发**: 启用内核IP转发
3. **检查源IP**: 确保源IP地址正确
4. **配置SNAT**: 正确配置源地址转换

---

## BE-4.9: 防火墙阻止

**问题现象**:
- 外部连接被拒绝
- 特定端口无法访问
- 连接超时

**可能原因**:
- 云厂商安全组规则
- 节点防火墙规则
- 网络ACL规则
- 外部防火墙阻止

**排查命令**:
```bash
# 查看iptables规则
iptables -L -n -v

# 查看云厂商安全组(云环境)
# AWS: aws ec2 describe-security-groups
# Azure: az network nsg list
# GCP: gcloud compute firewall-rules list

# 测试端口连通性
nc -zv <ip> <port>
telnet <ip> <port>

# 检查端口监听
netstat -tlnp
ss -tlnp
```

**解决方案**:
1. **配置安全组**: 开放必要的入站/出站端口
2. **调整防火墙**: 修改iptables规则
3. **检查ACL**: 调整网络ACL规则
4. **白名单**: 添加必要的IP白名单

---

## 2.5 顶部事件5: 存储访问失败 🟠 P1

> **问题定义**: 存储卷无法正常挂载或访问，PVC绑定失败
> **业务影响**: 有状态应用无法启动，数据无法持久化
> **响应时间**: 1小时内响应

## 中间事件 IE-5.1: PVC绑定失败

```
IE-5.1 PVC绑定失败 [OR门]
│
├── BE-5.1 StorageClass配置错误
├── BE-5.2 PV资源不足
└── BE-5.3 CSI驱动异常
```

---

## BE-5.1: StorageClass配置错误

**问题现象**:
- PVC一直处于Pending状态
- 事件显示StorageClass相关错误
- 无法动态创建PV

**可能原因**:
- StorageClass不存在
- StorageClass配置错误
- StorageClass不是默认类但PVC未指定
- provisioner配置错误

**排查命令**:
```bash
# 查看StorageClass
kubectl get storageclass

# 查看StorageClass详情
kubectl describe storageclass <sc-name>

# 查看PVC使用的StorageClass
kubectl get pvc <pvc-name> -o jsonpath='{.spec.storageClassName}'

# 查看默认StorageClass
kubectl get storageclass -o jsonpath='{.items[?(@.metadata.annotations.storageclass\.kubernetes\.io/is-default-class=="true")].metadata.name}'

# 查看PVC事件
kubectl describe pvc <pvc-name>
```

**解决方案**:
1. **创建StorageClass**: 创建缺失的StorageClass
2. **修正配置**: 修复StorageClass配置
3. **设置默认**: 设置默认StorageClass
4. **指定SC**: 在PVC中明确指定StorageClass

---

## BE-5.2: PV资源不足

**问题现象**:
- PVC无法绑定
- 事件显示no volume available
- 动态供应失败

**可能原因**:
- 可用PV不足
- PV大小不满足要求
- PV访问模式不匹配
- 存储后端容量不足

**排查命令**:
```bash
# 查看PV列表
kubectl get pv

# 查看可用PV
kubectl get pv | grep Available

# 查看PV详情
kubectl describe pv <pv-name>

# 查看PVC需求
kubectl get pvc <pvc-name> -o yaml | grep -A 5 resources

# 检查存储后端容量
# 根据存储类型检查(如NFS、iSCSI、云盘等)
```

**解决方案**:
1. **创建PV**: 手动创建新的PV
2. **动态供应**: 启用动态卷供应
3. **扩展存储**: 扩展存储后端容量
4. **释放PV**: 删除无用的PVC释放PV

---

## BE-5.3: CSI驱动异常

**问题现象**:
- 动态供应失败
- CSI Pod异常
- 存储操作超时

**可能原因**:
- CSI Controller Pod问题
- CSI Node Plugin问题
- CSI驱动配置错误
- CSI与存储后端通信失败

**排查命令**:
```bash
# 查看CSI Pod
kubectl get pods -n kube-system | grep csi

# 查看CSI Controller日志
kubectl logs -n kube-system <csi-controller-pod>

# 查看CSI Node日志
kubectl logs -n kube-system <csi-node-pod>

# 查看CSI驱动
kubectl get csidriver

# 查看StorageClass使用的provisioner
kubectl get storageclass <sc-name> -o jsonpath='{.provisioner}'
```

**解决方案**:
1. **重启CSI**: 重启CSI Pod
2. **检查配置**: 验证CSI配置
3. **检查后端**: 确保存储后端可用
4. **升级CSI**: 升级CSI驱动版本

---

## 中间事件 IE-5.2: 存储卷挂载失败

```
IE-5.2 存储卷挂载失败 [OR门]
│
├── BE-5.4 挂载参数错误
├── BE-5.5 权限不足
└── BE-5.6 文件系统损坏
```

---

## BE-5.4: 挂载参数错误

**问题现象**:
- Pod处于ContainerCreating状态
- 事件显示mount失败
- 挂载选项错误

**可能原因**:
- mountOptions配置错误
- 文件系统类型错误
- 挂载点不存在
- 不支持的挂载参数

**排查命令**:
```bash
# 查看Pod事件
kubectl describe pod <pod-name> | grep -A 10 Events

# 查看PV挂载选项
kubectl get pv <pv-name> -o jsonpath='{.spec.mountOptions}'

# 查看StorageClass挂载选项
kubectl get storageclass <sc-name> -o yaml | grep mountOptions

# 查看kubelet日志
journalctl -u kubelet | grep -i mount
```

**解决方案**:
1. **修正mountOptions**: 使用正确的挂载选项
2. **检查fsType**: 确保文件系统类型正确
3. **创建挂载点**: 确保挂载点目录存在
4. **验证参数**: 验证挂载参数支持情况

---

## BE-5.5: 权限不足

**问题现象**:
- 挂载成功但无法读写
- 权限被拒绝错误
- 文件所有权错误

**可能原因**:
- Pod securityContext配置错误
- fsGroup配置错误
- 存储端权限配置错误
- SELinux限制

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看Pod securityContext
kubectl get pod <pod-name> -o jsonpath='{.spec.securityContext}'

# 查看容器securityContext
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].securityContext}'

# 检查挂载目录权限
kubectl exec <pod-name> -- ls -la <mount-path>

# 检查SELinux状态
kubectl exec <pod-name> -- getenforce

# 查看Pod UID/GID
kubectl get pod <pod-name> -o jsonpath='{.spec.securityContext.fsGroup}'
```

**解决方案**:
1. **配置fsGroup**: 设置正确的fsGroup
2. **配置runAsUser**: 设置正确的运行用户
3. **调整存储权限**: 修改存储端权限
4. **禁用SELinux**: 临时禁用SELinux测试

---

## BE-5.6: 文件系统损坏

**问题现象**:
- 挂载失败
- 文件系统错误
- 数据无法读取

**可能原因**:
- 文件系统损坏
- 不干净的卸载
- 存储硬件问题
- 文件系统类型不匹配

**排查命令**:
```bash
# 查看Pod事件
kubectl describe pod <pod-name> | grep -i filesystem

# 检查文件系统(在节点上)
fsck -n <device-path>

# 查看dmesg日志
dmesg | grep -i error

# 检查存储健康状态
# 根据存储类型使用相应工具
```

**解决方案**:
1. **修复文件系统**: 使用fsck修复文件系统
2. **重新格式化**: 备份数据后重新格式化
3. **更换存储**: 更换问题存储设备
4. **数据恢复**: 从备份恢复数据

---

## 中间事件 IE-5.3: 存储性能/数据异常

```
IE-5.3 存储性能/数据异常 [OR门]
│
├── BE-5.7 存储后端性能下降
├── BE-5.8 数据损坏
└── BE-5.9 快照恢复失败
```

---

## BE-5.7: 存储后端性能下降

**问题现象**:
- 应用响应变慢
- I/O延迟增加
- 存储吞吐量下降

**可能原因**:
- 存储后端负载过高
- 网络延迟增加
- 存储硬件老化
- 存储配置不当

**排查命令**:
```bash
# 监控存储性能指标
# 使用Prometheus查询存储相关指标

# 检查节点I/O统计
iostat -x 1

# 检查存储延迟
# 根据存储类型使用相应监控工具

# 查看CSI指标
kubectl top pod -n kube-system | grep csi
```

**解决方案**:
1. **扩容存储**: 增加存储资源
2. **优化配置**: 调整存储配置参数
3. **升级硬件**: 更换性能更好的存储
4. **负载均衡**: 分散存储负载

---

## BE-5.8: 数据损坏

**问题现象**:
- 应用报告数据错误
- 文件读取失败
- 校验和不匹配

**可能原因**:
- 存储硬件问题
- 网络传输错误
- 软件bug
- 不完整的写入操作

**排查命令**:
```bash
# 检查应用日志
kubectl logs <pod-name> | grep -i error

# 检查文件完整性
# 使用应用特定的校验工具

# 检查存储健康
# 使用存储厂商提供的工具

# 查看事件
kubectl get events | grep -i data
```

**解决方案**:
1. **从备份恢复**: 使用最近的备份恢复数据
2. **数据修复**: 使用数据修复工具
3. **更换存储**: 更换问题存储设备
4. **启用校验**: 启用数据校验机制

---

## BE-5.9: 快照恢复失败

**问题现象**:
- 快照恢复操作失败
- 从快照创建的PVC无法使用
- 恢复的数据不完整

**可能原因**:
- 快照损坏
- 快照与当前版本不兼容
- CSI快照驱动问题
- 存储后端不支持快照恢复

**排查命令**:
```bash
# 查看VolumeSnapshot
kubectl get volumesnapshot

# 查看VolumeSnapshotContent
kubectl get volumesnapshotcontent

# 查看CSI快照控制器日志
kubectl logs -n kube-system <csi-snapshot-controller>

# 检查快照状态
kubectl describe volumesnapshot <snapshot-name>
```

**解决方案**:
1. **检查快照完整性**: 验证快照状态
2. **更新CSI驱动**: 升级CSI快照驱动
3. **使用其他快照**: 尝试其他可用快照
4. **手动恢复**: 从存储后端手动恢复

---

## 2.6 顶部事件6: 资源调度异常 🟡 P2

> **问题定义**: Pod调度行为异常，包括无法调度、调度结果不符合预期等问题
> **业务影响**: 资源利用率低，应用部署延迟
> **响应时间**: 4小时内响应

## 中间事件 IE-6.1: Pod无法调度

```
IE-6.1 Pod无法调度 [OR门]
│
├── BE-6.1 节点资源不足
├── BE-6.2 亲和性冲突
└── BE-6.3 污点不匹配
```

---

## BE-6.1: 节点资源不足

**问题现象**:
- Pod一直处于Pending状态
- 调度事件显示Insufficient资源

**可能原因**:
- CPU资源不足
- 内存资源不足
- 临时存储不足
- GPU等特殊资源不足

**排查命令**:
```bash
# 查看Pod调度事件
kubectl describe pod <pod-name> | grep -A 10 Events

# 查看节点资源分配
kubectl describe node <node-name> | grep -A 15 "Allocated resources"

# 查看节点资源使用
kubectl top node

# 查看Pod资源请求
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].resources}'
```

**解决方案**:
1. **增加节点**: 扩容集群
2. **降低请求**: 调整Pod资源请求
3. **清理Pod**: 删除无用Pod
4. **升级节点**: 使用更大规格节点

---

## BE-6.2: 亲和性冲突

**问题现象**:
- Pod无法调度
- 调度事件显示亲和性规则不满足

**可能原因**:
- nodeAffinity规则过于严格
- podAffinity规则无法满足
- podAntiAffinity规则冲突
- 拓扑域配置错误

**排查命令**:
```bash
# 查看Pod亲和性配置
kubectl get pod <pod-name> -o yaml | grep -A 30 affinity

# 查看节点标签
kubectl get node --show-labels

# 查看其他Pod标签
kubectl get pods --show-labels

# 检查拓扑域
kubectl get node <node-name> -o jsonpath='{.metadata.labels}'
```

**解决方案**:
1. **放宽规则**: 调整亲和性规则
2. **添加标签**: 给节点/Pod添加匹配标签
3. **调整拓扑域**: 使用正确的拓扑域
4. **删除亲和性**: 临时删除亲和性规则

---

## BE-6.3: 污点不匹配

**问题现象**:
- Pod无法调度
- 调度事件显示污点容忍不匹配

**排查命令**:
```bash
# 查看Pod Tolerations
kubectl get pod <pod-name> -o jsonpath='{.spec.tolerations}'

# 查看节点Taints
kubectl get node <node-name> -o jsonpath='{.spec.taints}'

# 查看调度事件
kubectl describe pod <pod-name> | grep -i taint
```

**解决方案**:
1. **添加Toleration**: 给Pod添加对应的Toleration
2. **移除Taint**: 从节点移除Taint
3. **修正匹配**: 确保Toleration与Taint匹配

---

## 中间事件 IE-6.2: 调度结果不符合预期

```
IE-6.2 调度结果不符合预期 [OR门]
│
├── BE-6.4 调度器配置错误
└── BE-6.5 优先级抢占问题
```

---

## BE-6.4: 调度器配置错误

**问题现象**:
- Pod调度到不期望的节点
- 调度策略不生效
- 调度器插件行为异常

**可能原因**:
- 调度器策略配置错误
- 调度器配置文件错误
- 调度器版本不兼容
- 自定义调度器配置错误

**排查命令**:
```bash
# 查看调度器配置
kubectl get configmap scheduler-config -n kube-system -o yaml

# 查看调度器日志
kubectl logs -n kube-system kube-scheduler-<node-name>

# 查看调度事件
kubectl describe pod <pod-name> | grep -A 5 Events

# 检查调度器策略
cat /etc/kubernetes/scheduler-policy-config.json
```

**解决方案**:
1. **修正配置**: 修复调度器配置
2. **更新策略**: 调整调度策略
3. **升级调度器**: 升级到兼容版本
4. **重启调度器**: 重启调度器应用配置

---

## BE-6.5: 优先级抢占问题

**问题现象**:
- 高优先级Pod无法抢占低优先级Pod
- 抢占导致意外Pod终止
- 优先级调度不生效

**可能原因**:
- PriorityClass配置错误
- Pod未指定优先级
- 抢占策略禁用
- 资源不足无法抢占

**排查命令**:
```bash
# 查看PriorityClass
kubectl get priorityclass

# 查看Pod优先级
kubectl get pod <pod-name> -o jsonpath='{.spec.priorityClassName}'

# 查看Pod优先级值
kubectl get pod <pod-name> -o jsonpath='{.spec.priority}'

# 查看抢占事件
kubectl get events | grep -i preempt
```

**解决方案**:
1. **配置PriorityClass**: 创建/更新PriorityClass
2. **指定优先级**: 在Pod中指定priorityClassName
3. **启用抢占**: 确保抢占策略启用
4. **调整优先级**: 调整Pod优先级设置

---

## 中间事件 IE-6.3: 自定义调度器问题

```
IE-6.3 自定义调度器问题 [OR门]
│
├── BE-6.6 调度器插件错误
└── BE-6.7 扩展点配置错误
```

---

## BE-6.6: 调度器插件错误

**问题现象**:
- 自定义调度器插件不生效
- 插件导致调度失败
- 插件性能问题

**可能原因**:
- 插件配置错误
- 插件与调度器版本不兼容
- 插件代码bug
- 插件资源不足

**排查命令**:
```bash
# 查看调度器配置
kubectl get configmap scheduler-config -n kube-system -o yaml

# 查看调度器日志
kubectl logs -n kube-system kube-scheduler-<node-name>

# 查看插件状态
# 根据插件类型查看相应状态
```

**解决方案**:
1. **修正配置**: 修复插件配置
2. **升级插件**: 升级到兼容版本
3. **禁用插件**: 临时禁用问题插件
4. **调试插件**: 启用插件调试日志

---

## BE-6.7: 扩展点配置错误

**问题现象**:
- 调度扩展点不生效
- Webhook扩展调度失败
- 扩展调度结果异常

**可能原因**:
- 扩展点配置错误
- Webhook服务不可用
- 扩展点权限不足
- 扩展点超时

**排查命令**:
```bash
# 查看调度器扩展配置
kubectl get configmap scheduler-config -n kube-system -o yaml

# 检查Webhook服务
kubectl get svc <webhook-service>

# 检查Webhook Pod
kubectl get pods -l app=<webhook-app>

# 查看Webhook日志
kubectl logs <webhook-pod>
```

**解决方案**:
1. **修正配置**: 修复扩展点配置
2. **检查Webhook**: 确保Webhook服务可用
3. **调整超时**: 增加扩展点超时时间
4. **检查权限**: 确保扩展点有正确权限

---

## 2.7 顶部事件7: 安全认证失败 🟠 P1

> **问题定义**: Kubernetes安全认证或授权失败，包括证书、RBAC、准入控制等问题
> **业务影响**: 用户/服务无法访问集群，安全策略无法生效
> **响应时间**: 1小时内响应

## 中间事件 IE-7.1: 证书相关问题

```
IE-7.1 证书相关问题 [OR门]
│
├── BE-7.1 证书过期
├── BE-7.2 证书链不完整
└── BE-7.3 CA配置错误
```

---

## BE-7.1: 证书过期

**问题现象**:
- API Server无法启动
- kubectl连接失败，显示证书错误
- 组件间通信失败

**可能原因**:
- 证书有效期已过
- 系统时间不正确
- 证书未正确续期

**排查命令**:
```bash
# 检查API Server证书
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# 检查etcd证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates

# 检查Kubelet证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 检查CA证书
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -dates

# 检查所有证书(kubeadm)
kubeadm certs check-expiration

# 检查系统时间
date
timedatectl status
```

**解决方案**:
1. **续期证书**: 使用kubeadm续期证书

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

   ```bash
   kubeadm certs renew all
   systemctl restart kubelet
   ```
2. **手动续期**: 使用openssl生成新证书
3. **同步时间**: 确保系统时间正确
4. **自动续期**: 配置证书自动续期

---

## BE-7.2: 证书链不完整

**问题现象**:
- TLS握手失败
- 证书验证错误
- 客户端无法验证服务器证书

**可能原因**:
- 中间证书缺失
- 证书链配置错误
- 根证书不信任

**排查命令**:
```bash
# 检查证书链
openssl crl2pkcs7 -nocrl -certfile /etc/kubernetes/pki/apiserver.crt | openssl pkcs7 -print_certs -noout

# 验证证书链
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# 检查证书详情
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text
```

**解决方案**:
1. **添加中间证书**: 在证书文件中包含完整证书链
2. **更新CA**: 确保客户端信任正确的CA
3. **重新生成**: 使用正确的CA重新生成证书

---

## BE-7.3: CA配置错误

**问题现象**:
- 证书签名失败
- 组件间认证失败
- 客户端无法连接

**可能原因**:
- CA证书错误
- CA私钥丢失
- 使用了错误的CA
- CA配置不一致

**排查命令**:
```bash
# 检查CA证书
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -text

# 检查CA私钥
openssl rsa -in /etc/kubernetes/pki/ca.key -check

# 检查kubeconfig中的CA
cat ~/.kube/config | grep certificate-authority-data

# 解码查看
echo '<base64-data>' | base64 -d | openssl x509 -noout -text
```

**解决方案**:
1. **恢复CA**: 从备份恢复CA证书和私钥
2. **重新生成**: 重新生成整个PKI
3. **更新配置**: 更新所有组件的CA配置
4. **保持一致**: 确保所有节点使用相同CA

---

## 中间事件 IE-7.2: RBAC权限问题

```
IE-7.2 RBAC权限问题 [OR门]
│
├── BE-7.4 Role配置错误
├── BE-7.5 RoleBinding缺失
└── BE-7.6 ServiceAccount问题
```

---

## BE-7.4: Role配置错误

**问题现象**:
- 用户/服务操作被拒绝
- 返回RBAC授权错误
- 权限不足

**可能原因**:
- Role规则配置错误
- 资源名称错误
- 动词权限不足
- API组配置错误

**排查命令**:
```bash
# 查看Role
kubectl get role <role-name> -n <namespace> -o yaml

# 查看ClusterRole
kubectl get clusterrole <clusterrole-name> -o yaml

# 检查权限
kubectl auth can-i <verb> <resource> --as=<user>

# 检查ServiceAccount权限
kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<namespace>:<sa-name>
```

**解决方案**:
1. **修正Role**: 添加缺失的权限规则
2. **检查资源名**: 确保资源名称正确
3. **增加权限**: 添加必要的动词权限
4. **验证API组**: 确保API组配置正确

---

## BE-7.5: RoleBinding缺失

**问题现象**:
- 用户有Role但无法使用权限
- ServiceAccount权限不生效
- 权限绑定错误

**可能原因**:
- RoleBinding不存在
- RoleBinding引用错误的Role
- RoleBinding绑定错误的用户/SA
- RoleBinding在错误的命名空间

**排查命令**:
```bash
# 查看RoleBinding
kubectl get rolebinding -n <namespace>
kubectl describe rolebinding <binding-name> -n <namespace>

# 查看ClusterRoleBinding
kubectl get clusterrolebinding
kubectl describe clusterrolebinding <binding-name>

# 检查绑定关系
kubectl get rolebinding <binding-name> -o yaml
```

**解决方案**:
1. **创建Binding**: 创建缺失的RoleBinding
2. **修正引用**: 确保引用正确的Role
3. **修正主体**: 绑定正确的用户/ServiceAccount
4. **检查命名空间**: 确保Binding在正确的命名空间

---

## BE-7.6: ServiceAccount问题

**问题现象**:
- Pod无法访问API Server
- ServiceAccount令牌无效
- 自动化任务权限不足

**可能原因**:
- ServiceAccount不存在
- ServiceAccount令牌未挂载
- ServiceAccount被禁用
- 令牌过期

**排查命令**:
```bash
# 查看ServiceAccount
kubectl get sa -n <namespace>
kubectl describe sa <sa-name> -n <namespace>

# 查看ServiceAccount Secret
kubectl get secret -n <namespace> | grep <sa-name>

# 查看Token
kubectl get secret <sa-secret> -n <namespace> -o jsonpath='{.data.token}' | base64 -d

# 检查Pod的ServiceAccount
kubectl get pod <pod-name> -o jsonpath='{.spec.serviceAccountName}'
```

**解决方案**:
1. **创建SA**: 创建缺失的ServiceAccount
2. **挂载Token**: 确保Token正确挂载到Pod
3. **启用SA**: 启用被禁用的ServiceAccount
4. **更新Token**: 更新过期的Token

---

## 中间事件 IE-7.3: 准入控制问题

```
IE-7.3 准入控制问题 [OR门]
│
├── BE-7.7 Webhook不可用
├── BE-7.8 Validating配置错误
└── BE-7.9 Mutating配置错误
```

---

## BE-7.7: Webhook不可用

**问题现象**:
- API请求被Webhook拒绝
- Webhook服务无法访问
- 请求超时

**可能原因**:
- Webhook Pod未运行
- Webhook Service无法访问
- Webhook证书问题
- 网络策略阻止Webhook

**排查命令**:
```bash
# 查看Webhook配置
kubectl get validatingwebhookconfiguration
kubectl get mutatingwebhookconfiguration

# 查看Webhook详情
kubectl describe validatingwebhookconfiguration <webhook-name>

# 检查Webhook Service
kubectl get svc <webhook-service> -n <webhook-namespace>

# 检查Webhook Pod
kubectl get pods -n <webhook-namespace> -l <webhook-label>

# 查看Webhook日志
kubectl logs -n <webhook-namespace> <webhook-pod>

# 测试Webhook连通性
kubectl run test --image=busybox --rm -it -- wget -O- <webhook-service>.<namespace>.svc:443
```

**解决方案**:
1. **启动Webhook**: 启动停止的Webhook Pod
2. **修复Service**: 确保Service配置正确
3. **修复证书**: 更新Webhook证书
4. **调整网络策略**: 允许Webhook流量
5. **临时禁用**: 紧急情况下临时禁用Webhook

---

## BE-7.8: Validating配置错误

**问题现象**:
- 资源创建被错误拒绝
- 验证规则过于严格
- 验证逻辑错误

**可能原因**:
- ValidatingWebhook配置错误
- 验证规则逻辑错误
- 规则匹配范围错误
- 失败策略配置不当

**排查命令**:
```bash
# 查看ValidatingWebhook配置
kubectl get validatingwebhookconfiguration <webhook-name> -o yaml

# 查看规则配置
kubectl get validatingwebhookconfiguration <webhook-name> -o jsonpath='{.webhooks[*].rules}'

# 查看失败策略
kubectl get validatingwebhookconfiguration <webhook-name> -o jsonpath='{.webhooks[*].failurePolicy}'

# 查看Webhook日志
kubectl logs <webhook-pod> -n <webhook-namespace>
```

**解决方案**:
1. **修正规则**: 调整验证规则
2. **调整范围**: 修正规则匹配范围
3. **修改策略**: 将failurePolicy改为Ignore临时绕过
4. **修复逻辑**: 修复Webhook验证逻辑

---

## BE-7.9: Mutating配置错误

**问题现象**:
- 资源被错误修改
- 注入的sidecar不正确
- 变异后资源无法使用

**可能原因**:
- MutatingWebhook配置错误
- 变异逻辑错误
- 注入配置错误
- 变异顺序问题

**排查命令**:
```bash
# 查看MutatingWebhook配置
kubectl get mutatingwebhookconfiguration <webhook-name> -o yaml

# 查看变异规则
kubectl get mutatingwebhookconfiguration <webhook-name> -o jsonpath='{.webhooks[*].rules}'

# 查看reinvocationPolicy
kubectl get mutatingwebhookconfiguration <webhook-name> -o jsonpath='{.webhooks[*].reinvocationPolicy}'

# 对比变异前后资源
kubectl get pod <pod-name> -o yaml
```

**解决方案**:
1. **修正配置**: 调整MutatingWebhook配置
2. **修复逻辑**: 修复变异逻辑
3. **调整顺序**: 调整Webhook执行顺序
4. **禁用变异**: 临时禁用问题Webhook

---

## 2.8 顶部事件8: 监控告警异常 🟡 P2

> **问题定义**: 监控告警系统异常，包括数据采集、告警触发、通知发送等问题
> **业务影响**: 无法及时发现问题，告警风暴或告警遗漏
> **响应时间**: 4小时内响应

## 中间事件 IE-8.1: 监控数据采集异常

```
IE-8.1 监控数据采集异常 [OR门]
│
├── BE-8.1 Prometheus问题
├── BE-8.2 ServiceMonitor错误
└── BE-8.3 指标丢失
```

---

## BE-8.1: Prometheus问题

**问题现象**:
- Prometheus UI无法访问
- 查询无数据返回
- Prometheus Pod异常

**可能原因**:
- Prometheus Pod崩溃
- 存储空间不足
- 配置重新加载失败
- 查询负载过高
- 内存不足OOM

**排查命令**:
```bash
# 检查Prometheus Pod状态
kubectl get pods -n monitoring | grep prometheus

# 查看Prometheus日志
kubectl logs -n monitoring prometheus-<pod-id>

# 检查Prometheus配置
kubectl get configmap prometheus-config -n monitoring -o yaml

# 检查存储
kubectl get pvc -n monitoring | grep prometheus
kubectl describe pvc prometheus-data -n monitoring

# 检查Prometheus Target
# 访问Prometheus UI: Status -> Targets

# 检查Prometheus规则
kubectl get prometheusrules -n monitoring
```

**解决方案**:
1. **重启Prometheus**: 删除Pod重新创建
2. **扩展存储**: 增加PVC容量
3. **修正配置**: 修复Prometheus配置
4. **增加资源**: 增加内存和CPU限制
5. **优化查询**: 优化耗时查询

---

## BE-8.2: ServiceMonitor错误

**问题现象**:
- 特定Target无法发现
- 指标采集失败
- ServiceMonitor配置不生效

**可能原因**:
- ServiceMonitor Selector不匹配
- Service端口配置错误
- Endpoint无可用Pod
- ServiceMonitor标签错误

**排查命令**:
```bash
# 查看ServiceMonitor
kubectl get servicemonitor -n monitoring
kubectl describe servicemonitor <sm-name> -n monitoring

# 查看ServiceMonitor配置
kubectl get servicemonitor <sm-name> -o yaml

# 检查匹配的Service
kubectl get svc -l <selector-key>=<selector-value>

# 检查Service Endpoint
kubectl get endpoints <service-name>

# 检查Pod标签
kubectl get pods --show-labels
```

**解决方案**:
1. **修正Selector**: 确保Selector匹配正确的Service
2. **修正端口**: 配置正确的端口名称
3. **检查Endpoint**: 确保Service有可用Endpoint
4. **添加标签**: 给Service添加匹配标签

---

## BE-8.3: 指标丢失

**问题现象**:
- 特定指标无法查询
- 指标数据不完整
- 历史数据缺失

**可能原因**:
- 指标采集失败
- 指标被丢弃
- 保留策略删除
- 采集目标不可用
- 网络问题

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 检查指标是否存在
# 在Prometheus UI查询指标

# 检查采集目标状态
# 访问Prometheus UI: Status -> Targets

# 检查采集配置
kubectl get configmap prometheus-config -o yaml | grep -A 10 scrape_configs

# 检查指标端点
curl http://<target>:<port>/metrics

# 检查网络连通性
kubectl exec -it prometheus-pod -- wget -O- <target>:<port>/metrics
```

**解决方案**:
1. **修复采集**: 确保采集端点正常
2. **调整保留**: 增加数据保留时间
3. **检查网络**: 确保网络连通
4. **重新配置**: 重新配置指标采集

---

## 中间事件 IE-8.2: 告警系统异常

```
IE-8.2 告警系统异常 [OR门]
│
├── BE-8.4 Alertmanager问题
├── BE-8.5 告警规则错误
└── BE-8.6 通知渠道失败
```

---

## BE-8.4: Alertmanager问题

**问题现象**:
- 告警不发送
- Alertmanager UI无法访问
- 告警路由错误

**可能原因**:
- Alertmanager Pod异常
- 配置重新加载失败
- 存储问题
- 高可用配置错误

**排查命令**:
```bash
# 检查Alertmanager Pod状态
kubectl get pods -n monitoring | grep alertmanager

# 查看Alertmanager日志
kubectl logs -n monitoring alertmanager-<pod-id>

# 检查Alertmanager配置
kubectl get secret alertmanager-config -n monitoring -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d

# 检查Alertmanager状态
# 访问Alertmanager UI: Status

# 检查告警
# 访问Alertmanager UI: Alerts
```

**解决方案**:
1. **重启Alertmanager**: 删除Pod重新创建
2. **修正配置**: 修复Alertmanager配置
3. **检查存储**: 确保存储正常
4. **修复HA**: 修复高可用配置

---

## BE-8.5: 告警规则错误

**问题现象**:
- 告警不触发
- 告警触发条件错误
- 告警表达式错误

**可能原因**:
- PromQL表达式错误
- 阈值设置不当
- for持续时间配置错误
- 标签匹配错误

**排查命令**:
```bash
# 查看PrometheusRule
kubectl get prometheusrules -n monitoring
kubectl describe prometheusrules <rule-name> -n monitoring

# 查看告警规则
kubectl get prometheusrules <rule-name> -o yaml

# 测试PromQL表达式
# 在Prometheus UI中测试查询

# 查看告警状态
# 访问Prometheus UI: Alerts

# 检查规则加载
kubectl logs prometheus-pod | grep -i rule
```

**解决方案**:
1. **修正表达式**: 修复PromQL表达式
2. **调整阈值**: 设置合理的告警阈值
3. **调整持续时间**: 配置合适的for时间
4. **修正标签**: 确保标签匹配正确

---

## BE-8.6: 通知渠道失败

**问题现象**:
- 告警发送失败
- 通知渠道无响应
- 告警丢失

**可能原因**:
- 通知渠道配置错误
- 接收方服务不可用
- 认证信息错误
- 网络问题
- 限流导致丢弃

**排查命令**:
```bash
# 查看Alertmanager配置
kubectl get secret alertmanager-config -n monitoring -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d

# 查看Alertmanager日志
kubectl logs alertmanager-pod | grep -i notify

# 检查接收方状态
# 测试接收方连通性

# 查看告警历史
# 访问Alertmanager UI: Silences / Status
```

**解决方案**:
1. **修正配置**: 修复通知渠道配置
2. **检查接收方**: 确保接收方服务可用
3. **更新认证**: 更新认证信息
4. **检查网络**: 确保网络连通
5. **调整限流**: 调整告警发送限流

---

## 中间事件 IE-8.3: 可视化系统异常

```
IE-8.3 可视化系统异常 [OR门]
│
├── BE-8.7 Grafana问题
├── BE-8.8 Dashboard配置错误
└── BE-8.9 数据源连接失败
```

---

## BE-8.7: Grafana问题

**问题现象**:
- Grafana UI无法访问
- Grafana Pod异常
- 查询超时

**可能原因**:
- Grafana Pod崩溃
- 数据库连接失败
- 配置错误
- 资源不足
- 插件问题

**排查命令**:
```bash
# 检查Grafana Pod状态
kubectl get pods -n monitoring | grep grafana

# 查看Grafana日志
kubectl logs -n monitoring grafana-<pod-id>

# 检查Grafana配置
kubectl get configmap grafana-config -n monitoring -o yaml

# 检查Grafana Service
kubectl get svc grafana -n monitoring

# 检查存储
kubectl get pvc -n monitoring | grep grafana
```

**解决方案**:
1. **重启Grafana**: 删除Pod重新创建
2. **检查数据库**: 确保数据库连接正常
3. **修正配置**: 修复Grafana配置
4. **增加资源**: 增加资源限制
5. **禁用插件**: 禁用问题插件

---

## BE-8.8: Dashboard配置错误

**问题现象**:
- Dashboard显示错误
- 图表无数据
- 查询错误

**可能原因**:
- Dashboard JSON配置错误
- 数据源变量错误
- Panel查询错误
- 模板变量错误

**排查命令**:
```bash
# 查看Dashboard ConfigMap
kubectl get configmap -n monitoring | grep dashboard

# 导出Dashboard JSON
# 在Grafana UI: Dashboard -> Settings -> JSON Model

# 检查变量配置
# 在Grafana UI: Dashboard -> Settings -> Variables

# 检查Panel查询
# 在Grafana UI: Panel -> Edit -> Query
```

**解决方案**:
1. **修正JSON**: 修复Dashboard JSON配置
2. **修正变量**: 修改变量定义
3. **修正查询**: 修复Panel查询语句
4. **重新导入**: 重新导入正确的Dashboard

---

## BE-8.9: 数据源连接失败

**问题现象**:
- Dashboard无数据
- 数据源测试失败
- 查询超时

**可能原因**:
- 数据源URL错误
- 认证信息错误
- 数据源服务不可用
- 网络问题
- 证书问题

**排查命令**:

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 查看Grafana数据源配置
# 在Grafana UI: Configuration -> Data Sources

# 检查Prometheus服务
kubectl get svc prometheus -n monitoring

# 测试数据源连通性
kubectl exec grafana-pod -- wget -O- prometheus.monitoring.svc:9090

# 检查Grafana Secret
kubectl get secret grafana-datasources -n monitoring -o yaml
```

**解决方案**:
1. **修正URL**: 配置正确的数据源URL
2. **更新认证**: 更新认证信息
3. **检查服务**: 确保数据源服务可用
4. **检查网络**: 确保网络连通
5. **配置证书**: 配置正确的TLS证书

---

<!-- chunk: 三、排查命令速查表 -->## 三、排查命令速查表

## 3.1 集群健康检查命令

```bash
# ============================================
# 集群健康检查命令速查表
# ============================================

# 检查节点状态
kubectl get nodes
kubectl get nodes -o wide
kubectl describe node <node-name>

# 检查组件状态
kubectl get componentstatuses

# 检查所有Pod状态
kubectl get pods --all-namespaces
kubectl get pods --all-namespaces -o wide

# 检查事件
kubectl get events --sort-by='.lastTimestamp'
kubectl get events --field-selector type=Warning

# 检查集群信息
kubectl cluster-info
kubectl version

# 检查API Server
kubectl get --raw='/healthz'
kubectl get --raw='/readyz'

# 检查etcd健康
ectcdctl --cacert=/etc/kubernetes/pki/etcd/ca.crt \
        --cert=/etc/kubernetes/pki/etcd/server.crt \
        --key=/etc/kubernetes/pki/etcd/server.key \
        endpoint health --cluster
```

## 3.2 控制平面组件排查命令

```bash
# ============================================
# 控制平面组件排查命令速查表
# ============================================

# API Server
kubectl get pods -n kube-system | grep apiserver
kubectl logs -n kube-system kube-apiserver-<node-name>
journalctl -u kube-apiserver -f
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# etcd
kubectl get pods -n kube-system | grep etcd
kubectl logs -n kube-system etcd-<node-name>
journalctl -u etcd -f
ls -la /var/lib/etcd/

# Scheduler
kubectl get pods -n kube-system | grep scheduler
kubectl logs -n kube-system kube-scheduler-<node-name>
kubectl get leases -n kube-system kube-scheduler -o yaml

# Controller Manager
kubectl get pods -n kube-system | grep controller-manager
kubectl logs -n kube-system kube-controller-manager-<node-name>
kubectl get leases -n kube-system kube-controller-manager -o yaml
```

## 3.3 工作节点排查命令

```bash
# ============================================
# 工作节点排查命令速查表
# ============================================

# Kubelet
systemctl status kubelet
journalctl -u kubelet -f
cat /var/lib/kubelet/config.yaml
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 容器运行时
systemctl status docker
systemctl status containerd
journalctl -u docker -f
docker ps
crictl ps

# 节点资源
df -h
free -h
top
kubectl top node <node-name>

# 节点压力检查
kubectl describe node <node-name> | grep -A 10 Conditions
```

## 3.4 网络排查命令

```bash
# ============================================
# 网络排查命令速查表
# ============================================

# 基础网络检查
ip addr
ip route
ip link
netstat -tlnp
ss -tlnp

# CNI检查
ls -la /etc/cni/net.d/
ls -la /opt/cni/bin/
cat /etc/cni/net.d/*.conf

# iptables检查
iptables -L -n -v
iptables -t nat -L -n -v
iptables -t nat -L KUBE-SERVICES -n -v

# IPVS检查
ipvsadm -Ln
lsmod | grep ip_vs

# DNS检查
kubectl get pods -n kube-system | grep dns
kubectl logs -n kube-system coredns-<pod-id>
kubectl get svc kube-dns -n kube-system

# Service检查
kubectl get svc --all-namespaces
kubectl get endpoints --all-namespaces
kubectl get endpoints <service-name>

# Ingress检查
kubectl get ingress --all-namespaces
kubectl get pods -n ingress-nginx
kubectl logs -n ingress-nginx ingress-nginx-controller-<pod-id>

# 网络策略检查
kubectl get networkpolicy --all-namespaces
kubectl describe networkpolicy <policy-name>
```

## 3.5 存储排查命令

```bash
# ============================================
# 存储排查命令速查表
# ============================================

# PVC检查
kubectl get pvc --all-namespaces
kubectl describe pvc <pvc-name>
kubectl get pvc <pvc-name> -o yaml

# PV检查
kubectl get pv
kubectl describe pv <pv-name>
kubectl get pv <pv-name> -o yaml

# StorageClass检查
kubectl get storageclass
kubectl describe storageclass <sc-name>

# CSI检查
kubectl get pods -n kube-system | grep csi
kubectl logs -n kube-system <csi-pod-name>
kubectl get csidriver

# 存储事件
kubectl get events | grep -i volume
kubectl get events | grep -i mount

# 节点存储检查
df -h
lsblk
mount | grep <volume>
```

## 3.6 Pod排查命令

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# ============================================
# Pod排查命令速查表
# ============================================

# Pod状态检查
kubectl get pods --all-namespaces
kubectl get pods -o wide
kubectl get pod <pod-name> -o yaml

# Pod详情
kubectl describe pod <pod-name>
kubectl get events --field-selector involvedObject.name=<pod-name>

# Pod日志
kubectl logs <pod-name>
kubectl logs <pod-name> --previous
kubectl logs <pod-name> -c <container-name>
kubectl logs <pod-name> --tail=100 -f

# 进入容器
kubectl exec -it <pod-name> -- /bin/sh
kubectl exec -it <pod-name> -c <container-name> -- /bin/bash

# Pod资源使用
kubectl top pod <pod-name>
kubectl top pod --all-namespaces

# Pod调试
kubectl debug <pod-name> -it --image=busybox --target=<container-name>
kubectl cp <pod-name>:<path> <local-path>
```

## 3.7 安全排查命令

```bash
# ============================================
# 安全排查命令速查表
# ============================================

# 证书检查
kubeadm certs check-expiration
openssl x509 -in /etc/kubernetes/pki/ca.crt -noout -dates
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# RBAC检查
kubectl get role --all-namespaces
kubectl get clusterrole
kubectl get rolebinding --all-namespaces
kubectl get clusterrolebinding

# 权限检查
kubectl auth can-i <verb> <resource> --as=<user>
kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<ns>:<sa>
kubectl auth can-i --list

# ServiceAccount检查
kubectl get sa --all-namespaces
kubectl get secret -n <namespace> | grep <sa-name>

# Webhook检查
kubectl get validatingwebhookconfiguration
kubectl get mutatingwebhookconfiguration
kubectl describe validatingwebhookconfiguration <name>

# 网络策略检查
kubectl get networkpolicy --all-namespaces
kubectl get pod <pod-name> -o jsonpath='{.spec.securityContext}'
```

---

<!-- chunk: 四、故障处理优先级建议 -->## 四、故障处理优先级建议

## 4.1 问题严重程度分级表(P0-P3)

| 级别 | 名称 | 定义 | 响应时间 | 升级条件 | 典型场景 |
|------|------|------|----------|----------|----------|
| 🔴 P0 | 紧急 | 集群/核心服务完全不可用 | 15分钟 | 30分钟未恢复 | 集群完全不可用、应用服务不可用 |
| 🟠 P1 | 高 | 重要功能受影响 | 1小时 | 4小时未恢复 | Pod启动失败、网络异常、存储失败、安全认证失败 |
| 🟡 P2 | 中 | 部分功能受限 | 4小时 | 24小时未恢复 | 资源调度异常、监控告警异常 |
| 🟢 P3 | 低 | 轻微影响 | 24小时 | 72小时未处理 | 性能下降、非关键功能异常 |

## 4.2 故障处理标准流程图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        故障处理标准流程                                       │
└─────────────────────────────────────────────────────────────────────────────┘

    ┌───────────────┐
    │  问题发现     │
    │ (监控/告警/   │
    │  用户反馈)    │
    └───────┬───────┘
            │
            ▼
    ┌───────────────┐
    │  问题定级     │◄────────────────────────────────────────┐
    │ (P0/P1/P2/P3) │                                         │
    └───────┬───────┘                                         │
            │                                                 │
    ┌───────┴───────┐                                         │
    │               │                                         │
    ▼               ▼                                         │
┌───────┐     ┌───────────┐                                   │
│ P0/P1 │     │  P2/P3    │                                   │
│ 紧急  │     │  普通     │                                   │
└───┬───┘     └─────┬─────┘                                   │
    │               │                                         │
    ▼               ▼                                         │
┌───────────┐   ┌───────────┐                                 │
│立即响应   │   │计划处理   │                                 │
│(15分钟内) │   │(4-24小时) │                                 │
└─────┬─────┘   └─────┬─────┘                                 │
      │               │                                       │
      └───────┬───────┘                                       │
              │                                               │
              ▼                                               │
      ┌───────────────┐                                       │
      │  故障诊断     │                                       │
      │ (FTA分析)     │                                       │
      └───────┬───────┘                                       │
              │                                               │
              ▼                                               │
      ┌───────────────┐                                       │
      │  根因定位     │                                       │
      │ (底部事件)    │                                       │
      └───────┬───────┘                                       │
              │                                               │
              ▼                                               │
      ┌───────────────┐                                       │
      │  问题修复     │                                       │
      │ (执行方案)    │                                       │
      └───────┬───────┘                                       │
              │                                               │
              ▼                                               │
      ┌───────────────┐                                       │
      │  验证恢复     │                                       │
      │ (确认解决)    │                                       │
      └───────┬───────┘                                       │
              │                                               │
      ┌───────┴───────┐                                       │
      │               │                                       │
      ▼               ▼                                       │
  ┌────────┐    ┌──────────┐                                  │
  │ 未恢复  │    │ 已恢复    │                                 │
  └───┬────┘    └────┬─────┘                                  │
      │              │                                        │
      │              ▼                                        │
      │       ┌──────────┐                                    │
      │       │ 事后复盘  │                                    │
      │       │ 文档更新  │                                    │
      │       └──────────┘                                    │
      │                                                       │
      └───────────────────────────────────────────────────────┘
```

## 4.3 紧急恢复措施(P0/P1级)

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        P0/P1级问题紧急恢复措施                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  【控制平面问题】                                                            │
│  ├─ 证书过期: kubeadm certs renew all && systemctl restart kubelet         │
│  ├─ API Server: 检查日志，重启Pod或修复配置                                   │
│  ├─ etcd问题: 从备份恢复或重新初始化                                         │
│  └─ 组件崩溃: 删除Pod让其重新创建                                            │
│                                                                             │
│  【工作节点问题】                                                            │
│  ├─ Kubelet: systemctl restart kubelet                                      │
│  ├─ Docker: systemctl restart docker                                        │
│  ├─ 容器运行时: systemctl restart containerd                                │
│  └─ 节点NotReady: 驱逐Pod，重启节点或替换节点                                 │
│                                                                             │
│  【网络问题】                                                                │
│  ├─ CNI问题: 重启CNI Pod或重新部署CNI插件                                    │
│  ├─ kube-proxy: 删除Pod重新创建                                             │
│  ├─ DNS问题: 重启CoreDNS Pod                                                │
│  └─ 网络策略: 临时删除阻止的网络策略                                          │
│                                                                             │
│  【存储问题】                                                                │
│  ├─ PVC绑定: 检查StorageClass，手动创建PV                                    │
│  ├─ 挂载失败: 检查权限，修复文件系统                                         │
│  └─ CSI问题: 重启CSI Pod                                                    │
│                                                                             │
│  【应用问题】                                                                │
│  ├─ Pod崩溃: 查看日志，修复应用或配置                                         │
│  ├─ OOMKilled: 增加内存限制                                                 │
│  ├─ ImagePull: 检查镜像和认证                                               │
│  └─ 服务不可用: 检查Service和Endpoint配置                                    │
│                                                                             │
│  【安全问题】                                                                │
│  ├─ 证书问题: 紧急续期证书                                                   │
│  ├─ RBAC问题: 临时授予权限，再修正配置                                        │
│  └─ Webhook: 临时禁用问题Webhook                                             │
│                                                                             │
│  【紧急回滚】                                                                │
│  ├─ Deployment: kubectl rollout undo deployment/<name>                      │
│  ├─ StatefulSet: 使用历史版本重新部署                                        │
│  └─ 配置回滚: kubectl apply -f <backup-config>                              │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 4.4 故障排查决策树

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        故障排查决策树                                         │
└─────────────────────────────────────────────────────────────────────────────┘

                        开始故障排查
                             │
                             ▼
              ┌────────────────────────────┐
              │   kubectl能否连接集群?      │
              └─────────────┬──────────────┘
                            │
              ┌─────────────┴─────────────┐
              │                           │
              ▼                           ▼
           否/超时                      是
              │                           │
              ▼                           ▼
    ┌─────────────────┐      ┌─────────────────────────┐
    │  TE-1: 集群完全  │      │  查看节点状态            │
    │  不可用          │      │  kubectl get nodes      │
    │  (检查API Server │      └───────────┬─────────────┘
    │   etcd 网络)     │                  │
    └─────────────────┘      ┌─────────────┴─────────────┐
                             │                           │
                             ▼                           ▼
                         有NotReady                   全Ready
                             │                           │
                             ▼                           ▼
                   ┌─────────────────┐      ┌─────────────────────────┐
                   │ 检查节点详情     │      │  查看Pod状态            │
                   │ 和资源压力       │      │  kubectl get pods       │
                   │ (IE-1.2)         │      │  --all-namespaces       │
                   └─────────────────┘      └───────────┬─────────────┘
                                                        │
                                           ┌────────────┼────────────┐
                                           │            │            │
                                           ▼            ▼            ▼
                                        有异常Pod    全Running    有Pending
                                           │            │            │
                                           ▼            │            ▼
                                 ┌─────────────────┐   │  ┌─────────────────┐
                                 │ 查看Pod事件和日志 │   │  │ 检查调度事件     │
                                 │ 确定问题类型     │   │  │ (IE-3.1/IE-6.1)  │
                                 │ (IE-2.1)         │   │  └─────────────────┘
                                 └─────────────────┘   │
                                                        │
                                                        ▼
                                            ┌─────────────────────┐
                                            │ 检查Service/Ingress  │
                                            │ 访问是否正常         │
                                            └──────────┬──────────┘
                                                       │
                                          ┌────────────┴────────────┐
                                          │                         │
                                          ▼                         ▼
                                       访问异常                  访问正常
                                          │                         │
                                          ▼                         ▼
                                ┌─────────────────┐      ┌─────────────────┐
                                │ 检查Service/    │      │ 检查监控告警     │
                                │ Endpoint/Ingress│      │ 是否正常         │
                                │ (IE-2.2/IE-2.3) │      │ (TE-8)          │
                                └─────────────────┘      └─────────────────┘
```

## 4.5 关键指标监控阈值

| 指标类别 | 指标名称 | 警告阈值 | 严重阈值 | 紧急阈值 | 说明 |
|----------|----------|----------|----------|----------|------|
| **节点资源** | CPU使用率 | 70% | 85% | 95% | 节点CPU使用率 |
| | 内存使用率 | 75% | 85% | 95% | 节点内存使用率 |
| | 磁盘使用率 | 75% | 85% | 90% | 节点磁盘使用率 |
| | 磁盘I/O延迟 | 20ms | 50ms | 100ms | 磁盘I/O延迟 |
| | 节点NotReady | - | 1个 | 3个或30% | 异常节点数量 |
| **Pod状态** | Pod重启次数 | 3次/小时 | 10次/小时 | 30次/小时 | Pod重启频率 |
| | Pod等待时间 | 5min | 15min | 30min | Pod处于Pending时间 |
| | 异常Pod比例 | 5% | 15% | 30% | 非Running状态Pod比例 |
| **控制平面** | API Server延迟 | 100ms | 500ms | 1000ms | API请求延迟 |
| | etcd延迟 | 50ms | 100ms | 300ms | etcd请求延迟 |
| | etcd DB大小 | 2GB | 4GB | 8GB | etcd数据库大小 |
| | 证书过期时间 | 30天 | 14天 | 7天 | 证书剩余有效期 |
| **网络** | DNS查询延迟 | 10ms | 50ms | 100ms | DNS解析延迟 |
| | 丢包率 | 1% | 5% | 10% | 网络丢包率 |
| | Service延迟 | 50ms | 200ms | 500ms | Service访问延迟 |
| **存储** | PVC绑定时间 | 1min | 5min | 15min | PVC绑定耗时 |
| | 存储I/O延迟 | 10ms | 50ms | 100ms | 存储I/O延迟 |
| | 存储使用率 | 75% | 85% | 90% | 存储使用率 |

---

<!-- chunk: 五、附录 -->## 五、附录

## 5.1 Kubernetes组件依赖关系图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Kubernetes 组件依赖关系图                                  │
└─────────────────────────────────────────────────────────────────────────────┘

                              ┌─────────────┐
                              │    User     │
                              │   (kubectl) │
                              └──────┬──────┘
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              控制平面 (Control Plane)                         │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌───────────────┐      ┌───────────────┐      ┌───────────────┐          │
│   │  API Server   │◄────►│     etcd      │      │   Scheduler   │          │
│   │   (6443)      │      │   (2379)      │      │               │          │
│   └───────┬───────┘      └───────────────┘      └───────┬───────┘          │
│           │                                             │                   │
│           │              ┌───────────────┐              │                   │
│           └─────────────►│  Controller   │◄─────────────┘                   │
│                          │   Manager     │                                  │
│                          └───────────────┘                                  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     │ (10250/10255)
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              工作节点 (Worker Nodes)                          │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌───────────────┐      ┌───────────────┐      ┌───────────────┐          │
│   │    Kubelet    │─────►│   Container   │─────►│    Pod        │          │
│   │               │      │   Runtime     │      │   (Container) │          │
│   └───────┬───────┘      │(docker/ctr)   │      └───────────────┘          │
│           │              └───────────────┘                                  │
│           │                                                                 │
│           │              ┌───────────────┐      ┌───────────────┐          │
│           └─────────────►│  kube-proxy   │─────►│   iptables/   │          │
│                          │               │      │    IPVS       │          │
│                          └───────────────┘      └───────────────┘          │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
                                     │
                                     │
                                     ▼
┌─────────────────────────────────────────────────────────────────────────────┐
│                              插件和扩展 (Addons)                              │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │    CNI      │  │   CoreDNS   │  │   Ingress   │  │    CSI      │        │
│  │  (Calico/   │  │             │  │ Controller  │  │  (Storage)  │        │
│  │  Flannel)   │  │             │  │  (nginx)    │  │             │        │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘        │
│                                                                             │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐        │
│  │ Prometheus  │  │  Grafana    │  │  Metrics    │  │  Dashboard  │        │
│  │             │  │             │  │   Server    │  │             │        │
│  └─────────────┘  └─────────────┘  └─────────────┘  └─────────────┘        │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘


依赖关系说明:
═══════════════════════════════════════════════════════════════════════════════

1. API Server 依赖:
   - etcd: 数据持久化存储
   - 证书: TLS认证

2. Scheduler 依赖:
   - API Server: 获取资源和节点信息

3. Controller Manager 依赖:
   - API Server: 获取和更新资源状态

4. Kubelet 依赖:
   - API Server: 接收指令和上报状态
   - Container Runtime: 管理容器生命周期
   - CNI: 网络配置

5. kube-proxy 依赖:
   - API Server: 获取Service和Endpoint信息

6. CoreDNS 依赖:
   - API Server: 获取Service信息

7. Ingress Controller 依赖:
   - API Server: 获取Ingress资源
   - Service: 后端转发

8. CSI 依赖:
   - API Server: 获取存储相关资源
   - 存储后端: 实际存储操作
```

## 5.2 故障排查检查清单

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                    Kubernetes 故障排查检查清单                                │
└─────────────────────────────────────────────────────────────────────────────┘

□ 集群健康检查
  □ kubectl能否连接集群?
  □ 所有节点是否Ready?
  □ 控制平面组件是否正常?
  □ etcd集群是否健康?
  □ 系统时间是否同步?

□ 节点检查
  □ 节点资源使用(CPU/内存/磁盘)
  □ Kubelet服务状态
  □ 容器运行时状态
  □ 节点压力状态(Disk/Memory/PID)
  □ 节点网络连通性

□ Pod检查
  □ Pod状态和重启次数
  □ Pod事件和日志
  □ 资源限制配置
  □ 健康检查配置
  □ 环境变量和配置
  □ 存储挂载状态

□ 网络检查
  □ CNI插件状态
  □ DNS解析是否正常
  □ Service和Endpoint
  □ 网络策略配置
  □ iptables/IPVS规则
  □ 防火墙和安全组

□ 存储检查
  □ PVC绑定状态
  □ PV可用性
  □ StorageClass配置
  □ CSI驱动状态
  □ 存储后端健康

□ 调度检查
  □ 资源配额
  □ 节点选择器
  □ 亲和性/反亲和性
  □ 污点/容忍

□ 安全检查
  □ 证书有效期
  □ RBAC配置
  □ ServiceAccount
  □ 网络策略
  □ Pod安全策略

□ 监控检查
  □ Prometheus状态
  □ 指标采集
  □ 告警规则
  □ Alertmanager
  □ Grafana

□ 日志收集
  □ 控制平面组件日志
  □ Kubelet日志
  □ 容器日志
  □ 审计日志
  □ 系统日志

□ 恢复验证
  □ 问题是否已解决
  □ 服务是否恢复正常
  □ 监控指标是否正常
  □ 用户访问是否正常
  □ 是否需要后续优化
```

---

<!-- chunk: 文档信息 -->## 文档信息

| 项目 | 内容 |
|------|------|
| 文档名称 | Kubernetes全量FTA故障排查手册 |
| 版本 | v1.0 |
| 适用范围 | Kubernetes生产环境 |
| 顶部事件 | 8个 |
| 中间事件 | 24个 |
| 底部事件 | 63个 |
| 排查命令 | 200+ |

---

*本文档基于Kubernetes故障树分析(FTA)方法编制，覆盖生产环境全量问题场景。*
┌────────────┐                ┌───────────────┐
    │ 检查集群状态  │                │ 检查性能指标  │
    │ kubectl get   │                │ 监控Dashboard │
    │ nodes         │                │               │
    └───────┬───────┘                └───────┬───────┘
            │                                │
    ┌───────┴───────┐                ┌───────┴───────┐
    │ 节点是否Ready?│                │ 是否有告警?   │
    └───────┬───────┘                └───────┬───────┘
            │                                │
    ┌───────┴───────┐                ┌───────┴───────┐
    │ 否    │ 是    │                │ 是    │ 否    │
    ▼       │       ▼                ▼       │       ▼
┌───────┐   │  ┌───────┐         ┌───────┐  │  ┌───────┐
│检查   │   │  │检查   │         │按告警 │  │  │服务   │
│节点   │   │  │Pod    │         │处理   │  │  │正常   │
│状态   │   │  │状态   │         │       │  │  │       │
└───────┘   │  └───────┘         └───────┘  │  └───────┘
```

## 4.5 关键指标监控阈值

| 指标 | 警告阈值 | 严重阈值 | 灾难阈值 |
|------|----------|----------|----------|
| 节点CPU使用率 | >70% | >85% | >95% |
| 节点内存使用率 | >80% | >90% | >95% |
| 节点磁盘使用率 | >80% | >90% | >95% |
| Pod重启次数 | >3次/小时 | >5次/小时 | >10次/小时 |
| API Server延迟 | >1s | >3s | >5s |
| etcd请求延迟 | >100ms | >300ms | >500ms |
| 节点NotReady时间 | >1min | >5min | >10min |

---

<!-- chunk: 五、附录 -->## 五、附录

## 5.1 Kubernetes组件依赖关系图

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                        Kubernetes 组件依赖关系                               │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐                                                            │
│  │   用户/API  │                                                            │
│  └──────┬──────┘                                                            │
│         │                                                                   │
│         ▼                                                                   │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐                   │
│  │ kube-apiserver│◀───│   etcd      │     │  kubectl    │                   │
│  └──────┬──────┘     └─────────────┘     └─────────────┘                   │
│         │                                                                   │
│    ┌────┴────┐                                                              │
│    │         │                                                              │
│    ▼         ▼                                                              │
│  ┌────────┐ ┌────────┐                                                      │
│  │Scheduler│ │Controller│                                                   │
│  └────┬───┘ └────┬───┘                                                      │
│       │          │                                                          │
│       └────┬─────┘                                                          │
│            │                                                                │
│            ▼                                                                │
│  ┌─────────────────────────────────────┐                                   │
│  │           工作节点                   │                                   │
│  │  ┌─────────┐  ┌─────────┐          │                                   │
│  │  │ kubelet │  │kube-proxy│          │                                   │
│  │  └────┬────┘  └────┬────┘          │                                   │
│  │       │            │               │                                   │
│  │       ▼            ▼               │                                   │
│  │  ┌─────────┐  ┌─────────┐          │                                   │
│  │  │container│  │  CNI    │          │                                   │
│  │  │ runtime │  │ plugin  │          │                                   │
│  │  └────┬────┘  └────┬────┘          │                                   │
│  │       │            │               │                                   │
│  │       ▼            ▼               │                                   │
│  │  ┌─────────────────────────┐      │                                   │
│  │  │        Pod              │      │                                   │
│  │  │  ┌─────┐  ┌─────┐      │      │                                   │
│  │  │  │App  │  │Sidecar│     │      │                                   │
│  │  │  └─────┘  └─────┘      │      │                                   │
│  │  └─────────────────────────┘      │                                   │
│  └─────────────────────────────────────┘                                   │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 5.2 故障排查检查清单

## 集群健康检查清单
- [ ] 所有节点状态为Ready
- [ ] 控制平面组件运行正常
- [ ] 核心DNS服务可用
- [ ] 网络插件运行正常
- [ ] 存储插件运行正常

## Pod故障排查清单
- [ ] Pod状态不是Error或CrashLoopBackOff
- [ ] 镜像拉取成功
- [ ] 资源限制合理
- [ ] 健康检查配置正确
- [ ] 依赖服务可用

## 网络故障排查清单
- [ ] Pod可以解析DNS
- [ ] Pod间可以互相通信
- [ ] Service可以访问
- [ ] Ingress路由正常
- [ ] 外部访问正常

## 存储故障排查清单
- [ ] PVC已绑定
- [ ] 存储卷可以挂载
- [ ] 文件系统没有损坏
- [ ] 存储后端可用
- [ ] 权限配置正确

---

**文档版本**: v1.0  
**适用范围**: Kubernetes v1.20+  
**最后更新**: 2024年

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-fta/MOC.md|topic-fta MOC]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md|fta-methodology-and-agentic-practices]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/kubernetes-fta-full-analysis-v2.md|kubernetes-fta-full-analysis-v2]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/problem-solving-architecture.md|problem-solving-architecture]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/symptom-vector-matcher.md|symptom-vector-matcher]]

```