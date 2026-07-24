---
title: kubelet
description: kubelet — Kubernetes 生产运维知识库
summary: 'kubelet runs on every worker node and is responsible for:'
category: entities
tags:
- k8s
- kubelet
- node
- agent
- cri
- cgroups
- containerd
- cri-o
- statefulset
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- kubelet 是什么
- 如何 kubelet
trigger_keywords:
- kubelet
prerequisites:
- kubectl-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# kubelet

## Role

kubelet runs on every worker node and is responsible for:
- Watching API Server for Pod assignments
- Managing container lifecycle via CRI ([[containerd|containerd]]/CRI-O)
- Mounting volumes via CSI
- Running health probes (liveness, readiness, startup)
- Reporting node and Pod status
- Evicting [[Pods|Pods]] under resource pressure

## Key Subsystems

| Subsystem | Function |
|-----------|----------|
| **PLEG** (Pod Lifecycle Event Generator) | Monitors [[概念/container-runtime.md|container runtime]], generates state change events that trigger syncPod |
| **Probe Manager** | Runs liveness, readiness, and startup probes |
| **Volume Manager** | Mounts/unmounts volumes, interacts with CSI drivers |
| **Eviction Manager** | Monitors node resources, evicts Pods when thresholds crossed |
| **cAdvisor** | Collects container resource metrics (CPU, memory, network, disk I/O) |
| **Status Manager** | Reports Pod and Node status to API Server |

## CRI (Container Runtime Interface)

kubelet communicates with container runtimes via gRPC-based CRI:
- `RunPodSandbox`: Create Pod network namespace
- `CreateContainer` / `StartContainer`: Container lifecycle
- `PullImage`: Pull container images
- `ListImages` / `RemoveImage`: Image management

## Key Configuration

| Parameter | Purpose | Recommended |
|-----------|---------|-------------|
| `--container-runtime-endpoint` | CRI socket | unix:///run/containerd/containerd.sock |
| `--cgroup-driver` | cgroup driver | systemd (must match runtime) |
| `--max-pods` | Max Pods per node | 110 (default), 500+ in cloud |
| `--eviction-hard` | Hard eviction threshold | memory.available<100Mi |
| `--pod-infra-container-image` | pause container image | registry.k8s.io/pause:3.9 |

## Certificate Rotation

kubelet auto-rotates its client certificate (`--rotate-certificates`), preventing certificate expiration issues.

## 运维操作

```bash
# 🟢 检查 kubelet 状态
systemctl status kubelet
journalctl -u kubelet --since "10 min ago" -f

# 🟢 查看 kubelet 配置
cat /var/lib/kubelet/config.yaml
kubectl get node <node> -o jsonpath='{.status.conditions}'

# 🟢 检查 kubelet 指标
curl -sk https://localhost:10250/metrics | grep kubelet_ | head -20
curl -sk https://localhost:10250/metrics/cadvisor | head -20

# 🟢 查看 Pod 状态和事件
kubectl get pods --field-selector spec.nodeName=<node> -A
kubectl get events --field-selector source.component=kubelet

# 🟢 检查 kubelet 证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 🟡 重启 kubelet（会短暂影响节点上 Pod）
systemctl restart kubelet

# 🟢 检查驱逐状态
kubectl describe node <node> | grep -A10 "Conditions"
kubectl get pods -A --field-selector=status.phase=Failed
```

### KubeletConfiguration 完整示例

```yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
# 资源管理
maxPods: 110
podsPerCore: 10
systemReserved:
  cpu: 200m
  memory: 512Mi
  ephemeral-storage: 1Gi
kubeReserved:
  cpu: 200m
  memory: 512Mi
  ephemeral-storage: 1Gi
# 驱逐策略
evictionHard:
  memory.available: "100Mi"
  nodefs.available: "10%"
  nodefs.inodesFree: "5%"
  imagefs.available: "15%"
evictionSoft:
  memory.available: "200Mi"
  nodefs.available: "15%"
evictionSoftGracePeriod:
  memory.available: "1m30s"
  nodefs.available: "1m30s"
# 运行时
containerRuntimeEndpoint: unix:///run/containerd/containerd.sock
cgroupDriver: systemd
# 证书
rotateCertificates: true
serverTLSBootstrap: true
# 日志
containerLogMaxSize: "50Mi"
containerLogMaxFiles: 5
# 探针
nodeStatusUpdateFrequency: 10s
nodeStatusReportFrequency: 5m
```

## 故障排查

| 症状 | 可能原因 | 诊断命令 | 修复方案 |
|------|----------|----------|----------|
| 节点 NotReady | kubelet 停止/证书过期 | `systemctl status kubelet`; `journalctl -u kubelet` | 重启 kubelet/轮换证书 |
| PLEG is not healthy | 容器运行时响应慢 | `journalctl -u kubelet` 查看 PLEG | 检查 containerd/磁盘 IO |
| Pod 驱逐 | 资源压力触发阈值 | `kubectl describe node` 查看 Conditions | 调整 eviction 阈值/扩容 |
| 探针失败重启 | 探针配置不合理 | `kubectl describe pod` 查看 Events | 调整 initialDelaySeconds/timeout |
| 磁盘压力 | 镜像/日志占满磁盘 | `df -h`; `du -sh /var/lib/containerd` | 清理镜像/调整 GC 阈值 |
| 证书过期 | 轮换失败 | `openssl x509 -dates` | 手动轮换/检查 CSR 审批 |

### 排查流程

```
kubelet 异常排查
├── 节点 NotReady？
│   ├── kubelet 服务运行？→ systemctl status kubelet
│   ├── 证书有效？→ openssl x509 -dates
│   ├── API Server 可达？→ curl -k https://<apiserver>:6443/healthz
│   └── 容器运行时正常？→ crictl info
├── Pod 异常？
│   ├── PLEG 错误 → 检查容器运行时/磁盘 IO
│   ├── 探针失败 → 检查应用健康端点/调整探针参数
│   └── 驱逐 → 检查节点资源/调整阈值
└── 性能问题？
    ├── CPU 高 → 检查 Pod 数量/探针频率
    ├── 内存高 → 检查 cAdvisor/日志缓冲
    └── 磁盘 IO → 检查镜像拉取/日志写入
```

## 生产案例

### 案例1：PLEG 不健康导致节点 NotReady

- **场景**：多个节点同时报 "PLEG is not healthy"，Pod 状态不更新
- **排查**：`journalctl -u kubelet` 显示 PLEG relist 超时；`iostat` 显示磁盘 IO 等待 90%+
- **方案**：升级存储驱动（HDD→SSD）；调整 containerd 并发数；设置 imageGCHighThresholdPercent=70 提前回收
- **效果**：PLEG relist 时间从 10s 降至 500ms，节点稳定

### 案例2：kubelet 证书过期导致节点失联

- **场景**：集群运行 1 年后多个节点突然 NotReady
- **排查**：`openssl x509 -in kubelet-client-current.pem -noout -dates` 显示证书已过期；CSR 未被审批
- **方案**：手动审批挂起的 CSR；确认 `--rotate-certificates=true`；设置证书过期监控告警
- **效果**：添加证书过期前 30 天告警，永不再发生

## 对比替代方案

| 组件 | 角色 | 与 kubelet 关系 |
|------|------|------|
| kubelet | 节点代理，管理 Pod 生命周期 | 核心组件 |
| virtual-kubelet | 虚拟节点，连接外部计算 | 替代 kubelet 的节点抽象 |
| KubeEdge edgecore | 边缘节点代理 | 边缘场景的 kubelet 替代 |
| k3s agent | 轻量节点代理 | 包含精简版 kubelet |

## 检查清单

- [ ] kubelet 服务正常运行且开机自启
- [ ] 证书轮换已启用 (rotateCertificates: true)
- [ ] cgroup driver 与容器运行时一致 (systemd)
- [ ] 驱逐阈值已配置且合理
- [ ] 系统资源预留已配置 (systemReserved/kubeReserved)
- [ ] 容器日志大小限制已配置
- [ ] kubelet 指标已接入 Prometheus
- [ ] 证书过期监控告警已配置

## Related

- [[kubernetes]] — Kubernetes (CNCF Graduated)
- [[概念/resource-management.md|resource-management]] — Resource Management (Requests, Limits, QoS)
- [[概念/kubernetes-architecture-overview.md|kubernetes-architecture-overview]] — Kubernetes Architecture Overview
- [[pod-lifecycle]] — Pod Lifecycle
- [[实体/container-runtime.md|container-runtime]] — Container Runtime
- [[pod-lifecycle|Pod Lifecycle]]
- [[概念/resource-management.md|Resource Management]]
- [[概念/kubernetes-architecture-overview.md|Kubernetes Architecture Overview]]
- [[实体/container-runtime.md|Container Runtime]]

- 15-kubelet-deep-dive
- 33-kubelet-eviction-thresholds
- 20-kubelet-configuration
- [[故障诊断/高级排障/02-node-components/01-kubelet-troubleshooting.md|01-kubelet-troubleshooting]]
- virtual-kubelet
- [[技能/节点/node-fta.md|Node 异常故障树分析]] — Cross-reference
- [[技能/工作负载/deployment/deployment-fta.md|Deployment 异常故障树分析]] — Cross-reference
- [[技能/工作负载/statefulset/statefulset-fta.md|StatefulSet 异常故障树分析]] — Cross-reference


<!-- risk-assessed -->
