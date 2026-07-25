---
title: 生产环境故障诊断 (Production Troubleshooting)
description: 'description: ''**目标读者**: SRE团队、故障处理工程师、运维人员'''
summary: 'description: ''**目标读者**: SRE团队、故障处理工程师、运维人员'''
category: general
tags:
- k8s
- devops
- daily-ops
- troubleshooting
- production
- apiserver
- scheduler
- calico
- ingress
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 15-production-troubleshooting常见问题有哪些？
- 如何排查15-production-troubleshooting相关问题？
- 15-production-troubleshooting的故障处理方法
trigger_keywords:
- 生产环境故障诊断
- Production
- Troubleshooting
- platform
- engineering
prerequisites:
- kubectl-basics
- platform-engineering-basics
- cni-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: 生产环境故障诊断 (Production Troubleshooting)
description: '**目标读者**: SRE团队、故障处理工程师、运维人员'
category: platform-ops
tags:
- k8s
- platform
- operations
- devops
- scheduler
- [[Ingress|ingress]]
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 平台工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- 生产环境故障诊断 (Production Troubleshooting) 是什么
- 如何 生产环境故障诊断 (Production Troubleshooting)
- [[Kubernetes|Kubernetes]] 9 platform ops 最佳实践
- 生产环境故障诊断 (Production Troubleshooting) 故障排查
- 生产环境故障诊断 (Production Troubleshooting) 排障步骤
trigger_keywords:
- 生产环境故障诊断
- Production
- Troubleshooting
- platform
- ops
cross_refs:
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: domain
  path: ../专项技术/
  label: '相关知识域: 专项技术'
- type: domain
  path: ../故障诊断/
  label: '相关知识域: 故障诊断'
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
# 生产环境故障诊断 (Production Troubleshooting)

> **适用版本**: Kubernetes v1.25 - v1.32 | **文档版本**: v1.0 | **最后更新**: 2026-02
> **目标读者**: SRE团队、故障处理工程师、运维人员

<!-- chunk: 概述 -->
## 概述

生产环境故障诊断是保障业务连续性的关键能力。本文档提供系统性的故障诊断方法论、常见问题排查技巧和自动化诊断工具，帮助运维团队快速定位和解决生产环境问题。

<!-- chunk: 故障诊断体系架构 -->
## 故障诊断体系架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          生产环境故障诊断体系                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │
│  │   问题发现      │  │   信息收集      │  │   根因分析      │            │
│  │                 │  │                 │  │                 │            │
│  │ • 监控告警      │  │ • 日志收集      │  │ • 关联分析      │            │
│  │ • 用户反馈      │  │ • 指标分析      │  │ • 模式识别      │            │
│  │ • 自动检测      │  │ • 链路追踪      │  │ • 假设验证      │            │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘            │
│          │                     │                     │                     │
│          ▼                     ▼                     ▼                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │
│  │   解决方案      │  │   验证修复      │  │   复盘总结      │            │
│  │                 │  │                 │  │                 │            │
│  │ • 临时缓解      │  │ • 效果确认      │  │ • 根因归档      │            │
│  │ • 根本解决      │  │ • 回归测试      │  │ • 预防措施      │            │
│  │ • 预防机制      │  │ • 监控加强      │  │ • 知识沉淀      │            │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

<!-- chunk: 系统性故障诊断方法 -->
## 系统性故障诊断方法

### 1. 黄金信号诊断法
```yaml
golden_signals_troubleshooting:
  latency_issues:
    diagnosis_steps:
      - check_service_mesh_metrics: "检查服务网格延迟"
      - analyze_application_logs: "分析应用日志中的慢查询"
      - examine_network_latency: "检查网络延迟"
      - review_resource_limits: "检查CPU/内存限制"
      
  traffic_issues:
    diagnosis_steps:
      - verify_load_balancer_config: "验证负载均衡配置"
      - check_service_discovery: "检查服务发现问题"
      - analyze_ingress_controller: "分析Ingress控制器状态"
      - review_autoscaling_settings: "检查自动扩缩容配置"
      
  error_issues:
    diagnosis_steps:
      - examine_error_logs: "检查错误日志"
      - analyze_return_codes: "分析返回码分布"
      - check_dependency_services: "检查依赖服务状态"
      - review_security_policies: "检查安全策略影响"
      
  saturation_issues:
    diagnosis_steps:
      - monitor_resource_utilization: "监控资源使用率"
      - check_pending_workloads: "检查待处理工作负载"
      - analyze_queue_depth: "分析队列深度"
      - review_capacity_planning: "检查容量规划"
```

### 2. 分层诊断方法
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# 分层故障诊断脚本

layered_troubleshooting() {
    local service_name=$1
    local namespace=${2:-default}
    
    echo "=== 分层故障诊断: $service_name ==="
    
    # 第一层: 基础设施层检查
    echo "1. 基础设施层诊断:"
    echo "   □ 节点状态检查"
    kubectl get nodes | grep -v Ready
    echo "   □ 网络连通性检查"
    kubectl run debug-pod --image=busybox --rm -it -- ping -c 3 8.8.8.8
    
    # 第二层: Kubernetes组件层检查
    echo "2. Kubernetes组件层诊断:"
    echo "   □ API Server状态"
    kubectl get componentstatuses
    echo "   □ 控制器状态"
    kubectl get pods -n kube-system | grep -E "(controller|scheduler)"
    
    # 第三层: 应用层检查
    echo "3. 应用层诊断:"
    echo "   □ Pod状态检查"
    kubectl get pods -n $namespace | grep $service_name
    echo "   □ 服务状态检查"
    kubectl get svc -n $namespace | grep $service_name
    
    # 第四层: 业务逻辑层检查
    echo "4. 业务逻辑层诊断:"
    echo "   □ 应用日志分析"
    kubectl logs -n $namespace -l app=$service_name --tail=100
    echo "   □ 依赖服务检查"
    kubectl get endpoints -n $namespace
}

# 使用示例
layered_troubleshooting "user-service" "production"
```
<!-- chunk: 常见问题场景及解决方案 -->
## 常见问题场景及解决方案

### 1. Pod相关问题

#### Pod Pending状态
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# Pod Pending故障诊断脚本

diagnose_pending_pods() {
    local namespace=${1:-default}
    
    echo "=== Pod Pending状态诊断 ==="
    
    # 检查Pending的Pod
    pending_pods=$(kubectl get pods -n $namespace --field-selector=status.phase=Pending -o name)
    
    if [ -z "$pending_pods" ]; then
        echo "没有Pending状态的Pod"
        return
    fi
    
    echo "发现Pending Pods:"
    echo "$pending_pods"
    echo ""
    
    # 逐个分析Pending原因
    for pod in $pending_pods; do
        echo "分析Pod: $pod"
        
        # 检查事件
        echo "相关事件:"
        kubectl describe $pod -n $namespace | grep -A 10 "Events:"
        echo ""
        
        # 检查资源请求
        echo "资源请求:"
        kubectl get $pod -n $namespace -o jsonpath='{.spec.containers[*].resources}'
        echo ""
        
        # 检查节点选择器
        echo "节点选择器:"
        kubectl get $pod -n $namespace -o jsonpath='{.spec.nodeSelector}'
        echo ""
        
        echo "---"
    done
}

# 使用示例
diagnose_pending_pods "production"
```
通过系统性的故障诊断方法和工具，可以显著提升故障处理效率，减少业务中断时间，保障生产环境的稳定运行。

<!-- chunk: 控制平面故障排查 -->
## 控制平面故障排查

### API Server 故障

```bash
# 🟢 低风险：只读检查
# API Server 状态检查
kubectl get componentstatuses 2>/dev/null || kubectl get --raw /healthz

# API Server 日志
kubectl logs -n kube-system -l component=kube-apiserver --tail=100

# 检查 API Server 指标
curl -k https://<apiserver>:6443/metrics | grep apiserver_request

# 常见 API Server 问题
# 1. 证书过期
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# 2. etcd 连接问题
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/apiserver-etcd-client.crt \
  --key=/etc/kubernetes/pki/apiserver-etcd-client.key \
  endpoint health

# 3. 请求延迟高
kubectl get --raw /metrics | grep apiserver_request_duration_seconds
```

### etcd 故障排查

```bash
# 🟢 低风险：只读检查
# etcd 集群状态
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status --write-out=table

# etcd 磁盘使用
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status --write-out=table | awk '{print $4}'

# etcd 告警
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  alarm list

# 磁盘碎片整理（🟡 中风险）
# ETCDCTL_API=3 etcdctl defrag --cluster
```

### 控制器/调度器故障

```bash
# 🟢 低风险：只读检查
# 控制器状态
kubectl get pods -n kube-system -l component=kube-controller-manager
kubectl logs -n kube-system -l component=kube-controller-manager --tail=50

# 调度器状态
kubectl get pods -n kube-system -l component=kube-scheduler
kubectl logs -n kube-system -l component=kube-scheduler --tail=50

# 检查 Pending Pod（调度失败）
kubectl get pods -A --field-selector=status.phase=Pending
kubectl describe pod <pending-pod> -n <ns> | grep -A 20 "Events:"
```

<!-- chunk: 网络故障排查 -->
## 网络故障排查

### DNS 故障排查

```bash
# 🟢 低风险：只读检查
# CoreDNS 状态
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# DNS 解析测试
kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- \
  nslookup kubernetes.default.svc.cluster.local

# CoreDNS 指标
kubectl exec -n kube-system -l k8s-app=kube-dns -- \
  wget -qO- http://localhost:9153/metrics | grep coredns_dns

# 检查 CoreDNS 配置
kubectl get configmap coredns -n kube-system -o yaml
```

### Service 连通性排查

```bash
# 🟢 低风险：只读检查
# 检查 Service 和 Endpoints
kubectl get svc -n <ns>
kubectl get endpoints -n <ns>

# 检查 kube-proxy 状态
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy --tail=50

# 测试 Service 连通性
kubectl run curl-test --image=curlimages/curl --rm -it --restart=Never -- \
  curl -v http://<service>.<ns>.svc.cluster.local:port

# 检查 iptables/ipvs 规则
kubectl exec -n kube-system <kube-proxy-pod> -- iptables -t nat -L -n | grep <service>
```

### CNI 故障排查

```bash
# 🟢 低风险：只读检查
# Calico 状态
kubectl get pods -n kube-system -l k8s-app=calico-node
kubectl logs -n kube-system -l k8s-app=calico-node --tail=50

# Calico 节点状态
kubectl exec -n kube-system <calico-node-pod> -- calicoctl node status

# Cilium 状态
kubectl get pods -n kube-system -l k8s-app=cilium
kubectl exec -n kube-system <cilium-pod> -- cilium status
kubectl exec -n kube-system <cilium-pod> -- cilium endpoint list

# 跨节点 Pod 连通性测试
kubectl run net-test-1 --image=busybox:1.36 --rm -it --restart=Never -- sleep 3600 &
kubectl run net-test-2 --image=busybox:1.36 --rm -it --restart=Never -- sleep 3600 &
kubectl exec net-test-1 -- ping <net-test-2-pod-ip>
```

<!-- chunk: 存储故障排查 -->
## 存储故障排查

### PVC/PV 故障排查

```bash
# 🟢 低风险：只读检查
# PVC 状态
kubectl get pvc -A | grep -v Bound

# PVC 事件
kubectl describe pvc <pvc-name> -n <ns>

# PV 状态
kubectl get pv | grep -v Bound

# CSI Driver 状态
kubectl get csidrivers
kubectl get csinodes
kubectl get pods -n kube-system -l app=*csi*

# 卷挂载问题
kubectl describe pod <pod-name> -n <ns> | grep -A 10 "Events:"
kubectl logs -n kube-system -l app=*csi-node* --tail=50
```

### 存储性能问题

```bash
# 🟢 低风险：只读检查
# 节点磁盘 I/O
kubectl exec -it <pod> -- iostat -x 1 5

# 磁盘使用率
kubectl exec -it <pod> -- df -h

# 文件系统 inode 使用
kubectl exec -it <pod> -- df -i

# 存储 I/O 延迟（需要节点访问）
# iostat -x 1
# await > 10ms 表示延迟较高
```

<!-- chunk: 自动化诊断工具 -->
## 自动化诊断工具

### 一键诊断脚本

```bash
#!/bin/bash
# 🟢 低风险：只读检查
# 集群健康一键诊断

REPORT="/tmp/cluster-diagnosis-$(date +%Y%m%d-%H%M).txt"

echo "=== 集群健康诊断报告 ===" | tee "$REPORT"
echo "时间: $(date)" | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

# 1. 节点状态
echo "--- 1. 节点状态 ---" | tee -a "$REPORT"
kubectl get nodes -o wide | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

# 2. 系统组件状态
echo "--- 2. 系统组件 ---" | tee -a "$REPORT"
kubectl get pods -n kube-system --field-selector=status.phase!=Running | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

# 3. 异常 Pod
echo "--- 3. 异常 Pod ---" | tee -a "$REPORT"
kubectl get pods -A --field-selector=status.phase!=Running,status.phase!=Succeeded | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

# 4. 资源使用 Top 10
echo "--- 4. CPU Top 10 ---" | tee -a "$REPORT"
kubectl top pods -A --sort-by=cpu 2>/dev/null | head -11 | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

echo "--- 5. Memory Top 10 ---" | tee -a "$REPORT"
kubectl top pods -A --sort-by=memory 2>/dev/null | head -11 | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

# 6. 最近告警事件
echo "--- 6. 最近告警事件 ---" | tee -a "$REPORT"
kubectl get events -A --sort-by='.lastTimestamp' 2>/dev/null | grep -i "warning\|error" | tail -20 | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

# 7. PVC 状态
echo "--- 7. PVC 状态 ---" | tee -a "$REPORT"
kubectl get pvc -A | grep -v Bound | tee -a "$REPORT"
echo "" | tee -a "$REPORT"

echo "=== 诊断完成，报告已保存至: $REPORT ===" | tee -a "$REPORT"
```

### kubectl 诊断插件

```bash
# 安装诊断插件
kubectl krew install doctor
kubectl krew install resource-capacity
kubectl krew install view-utilization

# 使用
kubectl doctor
kubectl resource-capacity
kubectl view-utilization
```

<!-- chunk: 事故响应流程 -->
## 事故响应流程

### 事故分级

| 级别 | 定义 | 响应时间 | 通知范围 |
|------|------|----------|----------|
| P0 | 核心业务完全不可用 | 5 分钟内 | 全员 + 管理层 |
| P1 | 核心业务部分降级 | 15 分钟内 | On-Call + 相关团队 |
| P2 | 非核心业务受影响 | 30 分钟内 | On-Call |
| P3 | 潜在风险/性能下降 | 4 小时内 | 相关团队 |

### 事故响应检查清单

```
事故发生
├── 1. 确认影响范围
│   ├── 受影响服务/用户
│   ├── 业务影响程度
│   └── 确定事故级别
├── 2. 组建响应团队
│   ├── 事故指挥官 (IC)
│   ├── 技术负责人
│   └── 沟通负责人
├── 3. 缓解措施
│   ├── 回滚最近变更
│   ├── 扩容/降级
│   └── 流量切换
├── 4. 根因分析
│   ├── 日志分析
│   ├── 指标关联
│   └── 变更追溯
├── 5. 修复验证
│   ├── 服务恢复确认
│   ├── 监控指标正常
│   └── 用户反馈确认
└── 6. 事后复盘
    ├── 时间线记录
    ├── 根因报告
    └── 改进措施
```

### 事故复盘模板

```markdown
# 事故复盘报告

## 基本信息
- 事故级别: P?
- 发生时间: YYYY-MM-DD HH:MM
- 恢复时间: YYYY-MM-DD HH:MM
- 持续时长: ? 分钟
- 影响范围: ?

## 时间线
| 时间 | 事件 |
|------|------|
| HH:MM | 告警触发 |
| HH:MM | On-Call 响应 |
| HH:MM | 初步定位 |
| HH:MM | 执行修复 |
| HH:MM | 服务恢复 |

## 根因分析
### 直接原因
?

### 根本原因（5 Whys）
1. Why: ?
2. Why: ?
3. Why: ?
4. Why: ?
5. Why: ?

## 改进措施
| 措施 | 负责人 | 截止日期 | 状态 |
|------|--------|----------|------|
| ? | ? | ? | □ |

## 经验教训
?
```

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 平台工程 MOC
- [[10-平台工程/README.md|Platform Ops Domain (平台运维领域)]]
- Domain-9 平台运维 — 开源项目索引
- 平台运维概述
- 集群生命周期管理
- 容量规划与资源评估 (Capacity Planning & Resource Assessment)
- 性能基准测试与调优 (Performance Benchmarking & Tuning)
- 运维指标体系建设 (Operations Metrics System)
- 监控告警体系
- GitOps配置管理 (GitOps Configuration Management)
- 运维自动化工具链 (Operations Automation Toolchain)
- 成本优化与FinOps实践 (Cost Optimization & FinOps)
- [[19-故障诊断/06-FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/backup-restore-fta.md|备份/恢复异常故障树分析]]
- [[19-故障诊断/06-FTA故障树/list/calico-fta.md|calico FTA 树：Calico CNI 故障诊断]]

## Related

- 22-production-checklist
- [[kudig-prompts-catalog]]
- [[02-工作负载/02-Java-on-K8s/02-spring-boot-kubernetes-production.md|02-spring-boot-kubernetes-production]]

## See Also

- 13-multi-cluster-management
- 14-large-scale-cluster-optimization
- 16-platform-upgrade-migration
- 17-multi-tenant-management


<!-- risk-assessed -->
