---
title: Kubernetes 集群组件健康检查清单
description: 汇总 Kubernetes 核心组件（控制平面、工作节点、网络、DNS、存储、附加组件）的日常健康检查项、诊断命令与告警建议。
summary: 汇总 Kubernetes 核心组件的日常健康检查项、诊断命令与告警建议，作为生产环境巡检、故障排查和可观测性配置的统一入口。
category: 集群基础
tags:
- k8s
- checklist
- health-check
- control-plane
- kubelet
- etcd
- cni
- coredns
- csi
- monitoring
tier: core
created: '2026-07-23'
last_updated: 2026-07
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Kubernetes 组件健康检查清单
- K8s 集群巡检项目
- 控制平面健康检查
trigger_keywords:
- health checklist
- 健康检查
- 巡检
- component health
prerequisites:
- kubectl-basics
- prometheus-basics
- kubernetes-concepts
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




# Kubernetes 集群组件健康检查清单

> **用途**：每日/每周巡检、变更前基线检查、故障恢复后验证、监控告警配置参考。

---

## 1. 检查清单总览

| 组件层级 | 检查对象 | 关键指标 | 推荐频率 |
|---------|---------|---------|---------|
| 控制平面 | kube-apiserver、etcd、scheduler、controller-manager | 可用性、延迟、Leader、证书 | 每日 |
| 工作节点 | kubelet、kube-proxy、containerd | 状态、资源、证书、运行时 | 每日 |
| 网络 | CNI、CoreDNS、Service、Ingress | Pod 连通性、DNS、LB、网络策略 | 每日 |
| 存储 | CSI Driver、PV/PVC、StorageClass | 挂载成功率、后端健康 | 每周 |
| 附加组件 | metrics-server、Dashboard、CCM | Pod 状态、APIService、云集成 | 每周 |

---

## 2. 控制平面组件

### 2.1 kube-apiserver

- [ ] API Server Pod 全部 Running
- [ ] `/healthz`、`/livez`、`/readyz` 返回 200
- [ ] 请求延迟 P99 < 1s
- [ ] 当前并发请求未达上限
- [ ] APF 无队列积压
- [ ] 审计日志正常写入
- [ ] PKI 证书有效期 > 30 天
- [ ] Admission Webhook 响应正常

```bash
# 🟢 健康检查
kubectl get --raw /healthz
kubectl get --raw /livez
kubectl get --raw /readyz

# 🟢 关键指标
kubectl get --raw /metrics | grep apiserver_request_duration_seconds_bucket
kubectl get --raw /metrics | grep apiserver_current_inflight_requests
kubectl get flowschema
kubectl get prioritylevelconfiguration

# 🟢 证书检查
for cert in /etc/kubernetes/pki/*.crt; do
  echo "$cert: $(openssl x509 -in $cert -noout -enddate)"
done
```

### 2.2 etcd

- [ ] etcd 成员健康，quorum 正常
- [ ] Leader 稳定，无频繁切换
- [ ] 磁盘 fsync 延迟 < 10ms
- [ ] 数据库大小 < quota-backend-bytes 的 80%
- [ ] 自动压缩与碎片整理正常运行
- [ ] 备份策略有效且可恢复
- [ ] peer/client 证书有效期 > 30 天

```bash
# 🟢 成员健康
etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

# 🟢 状态检查
etcdctl endpoint status --write-out=table
etcdctl endpoint hashkv
```

### 2.3 kube-scheduler

- [ ] Scheduler Pod 全部 Running
- [ ] Leader Election Lease 稳定
- [ ] 无大量 Pending Pod 堆积
- [ ] 调度延迟 P99 < 100ms
- [ ] 自定义调度器配置有效

```bash
# 🟢 检查 Leader
kubectl get lease kube-scheduler -n kube-system

# 🟢 检查 Pending Pod
kubectl get pods -A --field-selector=status.phase=Pending
kubectl get events -A --field-selector reason=FailedScheduling
```

### 2.4 kube-controller-manager

- [ ] KCM Pod 全部 Running
- [ ] Leader Election Lease 稳定
- [ ] 关键控制器（node、deployment、endpointslice、pv）运行正常
- [ ] 节点驱逐参数符合 RTO 要求
- [ ] 证书有效

```bash
# 🟢 检查 Leader
kubectl get lease kube-controller-manager -n kube-system

# 🟢 检查控制器 Pod
kubectl get pods -n kube-system -l component=kube-controller-manager
```

---

## 3. 工作节点组件

### 3.1 kubelet

- [ ] 所有节点 Ready
- [ ] kubelet 服务运行正常
- [ ] 证书自动轮换启用且未过期
- [ ] cgroup driver 与容器运行时一致
- [ ] 资源预留（systemReserved / kubeReserved）已配置
- [ ] 驱逐阈值合理
- [ ] PLEG 健康

```bash
# 🟢 节点状态
kubectl get nodes -o wide
kubectl describe node <node>

# 🟢 kubelet 日志
journalctl -u kubelet --since "10 min ago"

# 🟢 证书检查
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

### 3.2 kube-proxy

- [ ] kube-proxy DaemonSet 所有 Pod Running
- [ ] 代理模式符合预期（iptables/ipvs/nftables）
- [ ] conntrack 表未满
- [ ] Service 转发规则在所有节点一致
- [ ] IPVS 内核模块已加载（ipvs 模式）

```bash
# 🟢 状态检查
kubectl get pods -n kube-system -l k8s-app=kube-proxy
kubectl logs -n kube-system -l k8s-app=kube-proxy | grep -i "Using"

# 🟢 规则检查
iptables -t nat -L KUBE-SERVICES -n | head
# 或
ipvsadm -Ln

# 🟢 conntrack 检查
conntrack -L | wc -l
sysctl net.netfilter.nf_conntrack_max
```

### 3.3 容器运行时（containerd）

- [ ] containerd 服务运行正常
- [ ] CRI socket 可访问
- [ ] 磁盘/inode 使用率 < 80%
- [ ] 无大量僵尸容器或 shim 泄漏
- [ ] 镜像拉取链路正常

```bash
# 🟢 运行时检查
crictl info
crictl ps -a | head
systemctl status containerd

# 🟢 磁盘检查
df -h /var/lib/containerd
df -i /var/lib/containerd

# 🟢 日志检查
journalctl -u containerd --since "10 min ago"
```

---

## 4. 网络与 DNS

### 4.1 CNI

- [ ] CNI 插件 Pod 全部 Running
- [ ] Pod IP 分配正常
- [ ] 同节点/跨节点 Pod 互通
- [ ] 节点路由表完整
- [ ] NetworkPolicy 按预期生效

```bash
# 🟢 CNI 检查
kubectl get pods -n kube-system -l k8s-app=<cni>
ls /etc/cni/net.d/
cat /etc/cni/net.d/*.conflist

# 🟢 路由检查
ip route
```

### 4.2 CoreDNS

- [ ] CoreDNS Pod 全部 Running
- [ ] Service DNS 解析正常
- [ ] 上游 DNS 可达
- [ ] Corefile 配置有效
- [ ] 缓存命中率合理

```bash
# 🟢 CoreDNS 检查
kubectl get pods -n kube-system -l k8s-app=kube-dns
kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50

# 🟢 DNS 测试
kubectl run -it --rm debug --image=busybox:1.28 --restart=Never -- nslookup kubernetes.default.svc.cluster.local
```

### 4.3 Service / Ingress

- [ ] Service ClusterIP / NodePort / LoadBalancer 可访问
- [ ] EndpointSlice 与 Pod 状态同步
- [ ] Ingress Controller 正常运行
- [ ] TLS 证书有效

```bash
# 🟢 Service 检查
kubectl get svc -A
kubectl get endpointslices -A | head

# 🟢 Ingress 检查
kubectl get ingress -A
kubectl get pods -n <ingress-namespace>
```

---

## 5. 存储

### 5.1 CSI / PV / PVC

- [ ] CSI Node Driver 所有节点 Running
- [ ] PVC 绑定正常
- [ ] PV 挂载/卸载无失败
- [ ] 存储后端健康
- [ ] VolumeSnapshot 功能正常（如使用）

```bash
# 🟢 CSI 检查
kubectl get csidrivers
kubectl get pods -n kube-system -l app=csi-node-driver

# 🟢 PVC/PV 检查
kubectl get pvc,pv -A
kubectl describe pvc <pvc> -n <ns>
```

---

## 6. 附加组件

### 6.1 metrics-server

- [ ] metrics-server Pod Running
- [ ] `v1beta1.metrics.k8s.io` APIService 状态 True
- [ ] `kubectl top node/pod` 返回数据
- [ ] HPA/VPA 能获取指标

```bash
# 🟢 metrics-server 检查
kubectl get apiservice v1beta1.metrics.k8s.io -o yaml
kubectl top nodes
kubectl top pods -A
```

### 6.2 cloud-controller-manager

- [ ] CCM Pod Running
- [ ] 节点 providerID 已写入
- [ ] LoadBalancer Service 能分配 EXTERNAL-IP
- [ ] 云路由表包含所有节点 Pod CIDR
- [ ] 云凭证有效

```bash
# 🟢 CCM 检查
kubectl get pods -n kube-system | grep cloud-controller
kubectl get node <node> -o jsonpath='{.spec.providerID}'
```

### 6.3 Kubernetes Dashboard / Headlamp

- [ ] 可视化组件 Pod Running
- [ ] 通过 Ingress / 端口转发可访问
- [ ] RBAC 权限最小化
- [ ] OIDC / Token 认证生效
- [ ] 访问审计开启

---

## 7. 告警建议

| 组件 | 告警项 | 阈值建议 |
|------|--------|---------|
| API Server | 请求延迟 P99 | > 1s 持续 5m |
| etcd | fsync 延迟 | > 100ms 持续 5m |
| etcd | 数据库大小 | > 80% quota |
| kubelet | 节点 NotReady | 任意节点 NotReady > 2m |
| kube-proxy | conntrack 使用率 | > 80% |
| CoreDNS | 解析失败率 | > 1% |
| metrics-server | APIService 不可用 | > 5m |

---

## 8. 一键巡检脚本模板

```bash
#!/bin/bash
# 🟢 低风险：只读检查
set -e

echo "=== Nodes ==="
kubectl get nodes -o wide

echo "=== Control Plane Pods ==="
kubectl get pods -n kube-system -l tier=control-plane

echo "=== CoreDNS ==="
kubectl get pods -n kube-system -l k8s-app=kube-dns

echo "=== kube-proxy ==="
kubectl get pods -n kube-system -l k8s-app=kube-proxy

echo "=== Pending Pods ==="
kubectl get pods -A --field-selector=status.phase=Pending

echo "=== API Server Health ==="
kubectl get --raw /healthz

echo "=== Metrics API ==="
kubectl get --raw /apis/metrics.k8s.io/v1beta1/nodes 2>/dev/null || echo "metrics-server unavailable"
```

---

## Related

- [[集群基础/架构总览/02-core-components-deep-dive.md|Kubernetes 核心组件深度剖析]]
- [[集群基础/架构总览/03-managed-kubernetes-control-plane-differences.md|托管 Kubernetes 控制平面组件差异]]
- [[集群基础/架构总览/05-pod-creation-end-to-end-flow.md|Pod 创建端到端流程]]
- [[故障诊断/FTA故障树/list/apiserver-fta.md|API Server 异常故障树分析]]
- [[故障诊断/FTA故障树/list/etcd-fta.md|etcd 异常故障树分析]]
- [[故障诊断/FTA故障树/list/kubelet-fta.md|kubelet 异常故障树分析]]
- [[故障诊断/FTA故障树/list/cni-fta.md|CNI 异常故障树分析]]


<!-- risk-assessed -->
