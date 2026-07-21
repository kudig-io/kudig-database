---
title: 70 - RuntimeClass配置
description: '# 70 - RuntimeClass配置'
summary: 'TypeUrl = "io.containerd.runsc.v1.options"'
category: workloads
tags:
- k8s
- workload
- pod
- deployment
- statefulset
- containerd
- gpu
- cuda
- nvidia
- wasm
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 开发工程师
- 运维工程师
estimated_read_time: 5min
intent_queries:
- RuntimeClass配置 是什么
- 如何 RuntimeClass配置
- Kubernetes 4 workloads 最佳实践
trigger_keywords:
- RuntimeClass配置
- workloads
prerequisites:
- kubectl-basics
- pod-lifecycle
- gpu-scheduling-basics
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
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 70 - RuntimeClass配置

<!-- chunk: RuntimeClass概述 -->
## RuntimeClass概述

| 字段 | 说明 |
|-----|------|
| `handler` | 运行时处理器名称(与CRI配置对应) |
| `overhead` | 运行时额外资源开销 |
| `scheduling` | 调度约束(nodeSelector/tolerations) |

<!-- chunk: RuntimeClass配置 -->
## RuntimeClass配置

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
scheduling:
  nodeSelector:
    runtime: gvisor
  tolerations:
  - key: runtime
    value: gvisor
    effect: NoSchedule
```

<!-- chunk: 常用RuntimeClass -->
## 常用RuntimeClass

| 名称 | Handler | 用途 | 隔离级别 |
|-----|---------|------|---------|
| runc | runc | 默认运行时 | 进程隔离 |
| gvisor | runsc | 安全沙箱 | 内核隔离 |
| kata | kata-runtime | 轻量级VM | 虚拟化隔离 |
| nvidia | nvidia | GPU容器 | 进程隔离 |
| [[WasmEdge|wasmedge]] | wasmedge | WebAssembly | Wasm沙箱 |

<!-- chunk: containerd配置 -->
## containerd配置

```toml
# /etc/containerd/config.toml
version = 2

[plugins."io.containerd.grpc.v1.cri".containerd]
  default_runtime_name = "runc"

[plugins."io.containerd.grpc.v1.cri".containerd.runtimes]

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc]
    runtime_type = "io.containerd.runc.v2"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runc.options]
      SystemdCgroup = true

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc]
    runtime_type = "io.containerd.runsc.v1"
    [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.runsc.options]
      TypeUrl = "io.containerd.runsc.v1.options"

  [plugins."io.containerd.grpc.v1.cri".containerd.runtimes.kata]
    runtime_type = "io.containerd.kata.v2"
    privileged_without_host_devices = true
```

<!-- chunk: gVisor RuntimeClass -->
## gVisor RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
---
# 使用gVisor的Pod
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  runtimeClassName: gvisor
  containers:
  - name: app
    image: nginx
    resources:
      limits:
        cpu: "1"
        memory: 512Mi
```

<!-- chunk: Kata Containers RuntimeClass -->
## Kata Containers RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata-runtime
overhead:
  podFixed:
    cpu: "500m"
    memory: "160Mi"
scheduling:
  nodeSelector:
    kata-runtime: "true"
```

<!-- chunk: 运行时对比 -->
## 运行时对比

| 特性 | runc | gVisor | Kata |
|-----|------|--------|------|
| 启动时间 | <100ms | <500ms | 1-2s |
| 内存开销 | 0 | ~50MB | ~100MB |
| 系统调用兼容性 | 100% | ~90% | ~99% |
| 性能开销 | 0 | 5-30% | 10-20% |
| 安全隔离 | 低 | 高 | 最高 |
| 适用场景 | 通用 | 多租户 | 高安全 |

<!-- chunk: NVIDIA RuntimeClass -->
## NVIDIA RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: nvidia
handler: nvidia
scheduling:
  nodeSelector:
    nvidia.com/gpu.present: "true"
---
# GPU Pod
apiVersion: v1
kind: Pod
metadata:
  name: gpu-pod
spec:
  runtimeClassName: nvidia
  containers:
  - name: cuda
    image: nvcr.io/nvidia/cuda:12.0-base
    resources:
      limits:
        nvidia.com/gpu: 1
```

<!-- chunk: WebAssembly RuntimeClass -->
## WebAssembly RuntimeClass

```yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: wasmedge
handler: wasmedge
overhead:
  podFixed:
    cpu: "50m"
    memory: "10Mi"
---
# Wasm Pod
apiVersion: v1
kind: Pod
metadata:
  name: wasm-app
spec:
  runtimeClassName: wasmedge
  containers:
  - name: wasm
    image: myregistry/wasm-app:v1
```

<!-- chunk: 验证运行时 -->
## 验证运行时

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看RuntimeClass
kubectl get runtimeclass

# 查看Pod使用的运行时
kubectl get pod <pod-name> -o jsonpath='{.spec.runtimeClassName}'

# 检查节点运行时
crictl info | jq '.config.containerd.runtimes'

# 测试运行时
kubectl run test --image=nginx --runtime-class=gvisor --rm -it -- cat /proc/version
```
<!-- chunk: ACK运行时支持 -->
## ACK运行时支持

| 运行时 | 支持状态 | 说明 |
|-------|---------|------|
| containerd | ✅ 默认 | 标准运行时 |
| 安全沙箱 | ✅ | 基于Kata的隔离 |
| 神龙裸金属 | ✅ | 高性能计算 |

<!-- chunk: 版本变更记录 -->
## 版本变更记录

| 版本 | 变更内容 |
|------|---------|
| v1.20 | RuntimeClass GA |
| v1.24 | RuntimeClass overhead改进 |
| v1.27 | 用户命名空间支持 |
| v1.29 | Wasm运行时支持改进 |

<!-- chunk: RuntimeClass治理与准入控制 -->
## RuntimeClass治理与准入控制

在多租户集群中，需要限制用户可选择的运行时，防止未经批准的 RuntimeClass 被使用。

### 治理策略对比

| 方案 | 适用场景 | 优势 | 劣势 |
|-----|---------|------|------|
| OPA/Gatekeeper | 企业级多租户 | 灵活策略语言、审计日志 | 学习曲线较陡 |
| Kyverno | 云原生原生 | YAML 策略、K8s 风格 | 社区相对年轻 |
| Pod Security Admission | 基础安全 | 内置、零依赖 | 粒度有限 |
| 自定义 Admission Webhook | 特殊需求 | 完全自定义 | 维护成本高 |

### OPA/Gatekeeper 准入策略

```yaml
# 限制只允许使用已批准的 RuntimeClass
apiVersion: constraints.gatekeeper.sh/v1beta1
kind: K8sAllowedRuntimeClasses
metadata:
  name: restrict-runtime-classes
spec:
  match:
    kinds:
      - apiGroups: [""]
        kinds: ["Pod"]
    excludedNamespaces: ["kube-system", "monitoring"]
  parameters:
    allowedRuntimeClasses:
      - runc
      - gvisor
      - nvidia
---
apiVersion: templates.gatekeeper.sh/v1
kind: ConstraintTemplate
metadata:
  name: k8sallowedruntimeclasses
spec:
  crd:
    spec:
      names:
        kind: K8sAllowedRuntimeClasses
      validation:
        openAPIV3Schema:
          type: object
          properties:
            allowedRuntimeClasses:
              type: array
              items:
                type: string
  targets:
    - target: admission.k8s.gatekeeper.sh
      rego: |
        package k8sallowedruntimeclasses

        violation[{"msg": msg}] {
          input.review.object.spec.runtimeClassName
          not allowed_runtime
          msg := sprintf("RuntimeClass '%v' is not allowed. Approved: %v", [
            input.review.object.spec.runtimeClassName,
            input.parameters.allowedRuntimeClasses
          ])
        }

        allowed_runtime {
          input.review.object.spec.runtimeClassName == input.parameters.allowedRuntimeClasses[_]
        }
```

### Kyverno 准入策略

```yaml
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: restrict-runtimeclass
spec:
  validationFailureAction: Enforce
  background: true
  rules:
    - name: validate-runtimeclass
      match:
        any:
          - resources:
              kinds: ["Pod"]
      exclude:
        any:
          - resources:
              namespaces: ["kube-system", "monitoring", "ingress-nginx"]
      validate:
        message: >-
          Only approved RuntimeClasses (runc, gvisor, nvidia) are allowed.
        pattern:
          spec:
            =(runtimeClassName): "runc | gvisor | nvidia"
```

<!-- chunk: 多租户运行时隔离策略 -->
## 多租户运行时隔离策略

### 运行时选择决策树

```
工作负载安全评估
├── 是否运行不可信代码？
│   ├── 是 → 是否需要完整 Linux 兼容性？
│   │   ├── 是 → Kata Containers（VM 级隔离）
│   │   └── 否 → gVisor（用户空间内核，性能更优）
│   └── 否 → 是否需要 GPU 加速？
│       ├── 是 → NVIDIA Runtime + GPU Operator
│       └── 否 → 默认 runc
├── 是否为边缘/IoT 场景？
│   ├── 是 → WasmEdge（超轻量、快速冷启动）
│   └── 否 → 标准运行时
└── 合规要求？
    ├── 金融/医疗 → Kata + 加密存储
    └── 通用 → runc + Pod Security Standards
```

### 多租户节点池配置

```yaml
# 安全沙箱专用节点池
apiVersion: v1
kind: Node
metadata:
  labels:
    runtime: gvisor
    tenant-tier: secure
    node-pool: secure-sandbox
spec:
  taints:
    - key: runtime
      value: gvisor
      effect: NoSchedule
---
# 对应的 RuntimeClass 绑定调度约束
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor-secure
  labels:
    tier: production
handler: runsc
overhead:
  podFixed:
    cpu: "250m"
    memory: "120Mi"
scheduling:
  nodeSelector:
    runtime: gvisor
    tenant-tier: secure
  tolerations:
    - key: runtime
      value: gvisor
      effect: NoSchedule
---
# GPU 专用节点池
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: nvidia-a100
handler: nvidia
scheduling:
  nodeSelector:
    nvidia.com/gpu.product: "A100-SXM4-80GB"
    node-pool: gpu-compute
  tolerations:
    - key: nvidia.com/gpu
      operator: Exists
      effect: NoSchedule
```

### 租户运行时配额

```yaml
# 限制租户只能使用特定 RuntimeClass
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-runtime-quota
  namespace: tenant-a
spec:
  hard:
    # 限制 gVisor Pod 数量
    pods: "50"
  scopeSelector:
    matchExpressions:
      - operator: In
        scopeName: PriorityClass
        values: ["standard"]
---
# 配合 LimitRange 设置默认运行时开销
apiVersion: v1
kind: LimitRange
metadata:
  name: runtime-overhead-limits
  namespace: tenant-a
spec:
  limits:
    - type: Pod
      max:
        cpu: "8"
        memory: 32Gi
    - type: Container
      default:
        cpu: "1"
        memory: 2Gi
      defaultRequest:
        cpu: "250m"
        memory: 512Mi
```

<!-- chunk: 运行时监控与告警 -->
## 运行时监控与告警

### 关键监控指标

| 指标 | 含义 | 告警阈值 | PromQL |
|-----|------|---------|--------|
| `container_runtime_operations_total` | 运行时操作计数 | - | `rate(...[5m])` |
| `container_runtime_operations_errors_total` | 操作错误数 | >5/min | `rate(...[5m]) > 0.08` |
| `container_runtime_operations_duration_seconds` | 操作延迟 | P99>5s | `histogram_quantile(0.99, ...)` |
| `kubelet_runtime_operations_total` | kubelet 运行时调用 | - | `sum by (operation_type)` |
| `container_start_time_seconds` | 容器启动时间 | >30s | `time() - container_start_time_seconds` |

### PrometheusRule 告警配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: runtime-class-alerts
  namespace: monitoring
spec:
  groups:
    - name: runtime.rules
      rules:
        - alert: ContainerRuntimeHighErrorRate
          expr: |
            rate(container_runtime_operations_errors_total{job="kubelet"}[5m])
            / rate(container_runtime_operations_total{job="kubelet"}[5m]) > 0.05
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} 容器运行时错误率超过 5%"
            runbook: "检查 containerd 日志: journalctl -u containerd --since '10min ago'"

        - alert: ContainerRuntimeOperationSlow
          expr: |
            histogram_quantile(0.99,
              rate(container_runtime_operations_duration_seconds_bucket{job="kubelet"}[5m])
            ) > 5
          for: 10m
          labels:
            severity: warning
          annotations:
            summary: "节点 {{ $labels.node }} 运行时操作 P99 延迟超过 5s"

        - alert: RuntimeClassPodSchedulingFailed
          expr: |
            kube_pod_status_reason{reason="FailedScheduling"} == 1
            and on(pod, namespace) kube_pod_spec_runtime_class_name != ""
          for: 5m
          labels:
            severity: critical
          annotations:
            summary: "Pod {{ $labels.namespace }}/{{ $labels.pod }} 因 RuntimeClass 调度失败"

        - alert: GVisorOverheadExcessive
          expr: |
            container_memory_working_set_bytes{container!="POD"}
            / on(pod, namespace) group_left
            kube_pod_container_resource_limits{resource="memory"} > 0.95
            and on(pod, namespace)
            kube_pod_spec_runtime_class_name{runtime_class="gvisor"} == 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "gVisor Pod {{ $labels.pod }} 内存接近限制（含 overhead）"
```

### Grafana Dashboard 面板布局

| 面板 | 数据源 | 用途 |
|-----|--------|------|
| Runtime Operations Rate | Prometheus | 各节点运行时操作速率 |
| Runtime Error Ratio | Prometheus | 错误率趋势（按节点/操作类型） |
| Pod Startup Latency | Prometheus | 容器启动延迟分布 |
| RuntimeClass Distribution | kube-state-metrics | 各 RuntimeClass 使用分布 |
| Node Runtime Health | Node Exporter | 节点级运行时健康状态 |

<!-- chunk: 运行时性能基准测试 -->
## 运行时性能基准测试

### 自动化基准测试 Job

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: runtime-benchmark
  namespace: benchmarking
spec:
  template:
    spec:
      runtimeClassName: gvisor  # 切换测试不同运行时
      restartPolicy: Never
      containers:
        - name: bench
          image: polinux/stress:latest
          command:
            - /bin/sh
            - -c
            - |
              echo "=== Runtime Benchmark ==="
              echo "Runtime: $(cat /proc/version)"
              echo "--- CPU Benchmark ---"
              time dd if=/dev/zero of=/dev/null bs=1M count=10000
              echo "--- Memory Benchmark ---"
              time stress --vm 2 --vm-bytes 256M --timeout 30s
              echo "--- I/O Benchmark ---"
              time dd if=/dev/zero of=/tmp/testfile bs=4k count=100000 oflag=direct
              echo "--- Syscall Benchmark ---"
              time strace -c -e trace=all ls / > /dev/null 2>&1
              echo "=== Done ==="
          resources:
            requests:
              cpu: "2"
              memory: 4Gi
            limits:
              cpu: "2"
              memory: 4Gi
          volumeMounts:
            - name: tmp
              mountPath: /tmp
      volumes:
        - name: tmp
          emptyDir: {}
  backoffLimit: 0
```

### 性能基准对比表

| 测试项 | runc (基线) | gVisor | Kata | WasmEdge |
|-------|------------|--------|------|----------|
| CPU 密集 (sysbench) | 100% | 85-95% | 90-95% | N/A |
| 内存带宽 (STREAM) | 100% | 90-95% | 92-97% | N/A |
| 磁盘 I/O (fio 4k rand) | 100% | 60-80% | 80-90% | N/A |
| 网络吞吐 (iperf3) | 100% | 70-85% | 85-92% | N/A |
| 系统调用延迟 (getpid) | ~80ns | ~2000ns | ~500ns | N/A |
| 冷启动时间 | <100ms | <500ms | 1-2s | <50ms |
| 内存 Footprint | ~5MB | ~50MB | ~100MB | ~2MB |

> **结论**: gVisor 适合安全优先、I/O 不敏感的场景；Kata 适合需要完整 Linux 兼容性的强隔离场景；WasmEdge 适合轻量级边缘函数。

<!-- chunk: 运行时故障排查决策树 -->
## 运行时故障排查决策树

```
Pod 创建/启动失败
├── 事件显示 "no runtime for class" ?
│   ├── 是 → RuntimeClass 资源不存在或 handler 名称不匹配
│   │   ├── kubectl get runtimeclass（确认资源存在）
│   │   ├── crictl info | jq '.config.containerd.runtimes'（确认 handler 配置）
│   │   └── 修复：创建 RuntimeClass 或修正 handler 名称
│   └── 否 → 继续
├── 事件显示 "FailedScheduling" + nodeSelector ?
│   ├── 是 → 无匹配节点
│   │   ├── kubectl get nodes -l runtime=gvisor（检查节点标签）
│   │   └── 修复：添加节点标签或调整 scheduling 配置
│   └── 否 → 继续
├── 容器启动后立即退出 ?
│   ├── 是 → 运行时兼容性问题
│   │   ├── kubectl logs --previous（查看退出日志）
│   │   ├── gVisor: 检查不支持的系统调用 (dmesg | grep runsc)
│   │   └── 修复：切换运行时或调整应用
│   └── 否 → 继续
└── 性能异常（延迟高、吞吐低）?
    ├── 检查 overhead 是否正确计入资源限制
    ├── 对比 runc 基线性能
    └── 考虑切换到更低开销的运行时
```

### 常见故障修复表

| 故障现象 | 根因 | 诊断命令 | 修复方案 |
|---------|------|---------|----------|
| `runtimeclass not found` | RuntimeClass 未创建 | `kubectl get runtimeclass` | 创建对应 RuntimeClass 资源 |
| `no runtime handler` | containerd 未配置 handler | `crictl info` | 编辑 config.toml 添加 handler |
| Pod Pending + 无匹配节点 | nodeSelector 无匹配 | `kubectl get nodes --show-labels` | 添加节点标签或修改 scheduling |
| gVisor 应用崩溃 | 不支持的系统调用 | `dmesg \| grep -i runsc` | 检查 gVisor 兼容性列表 |
| Kata 启动超时 | VM 资源不足 | `journalctl -u containerd` | 增加节点 CPU/内存或调整 overhead |
| GPU 容器无法访问设备 | nvidia-container-toolkit 异常 | `nvidia-smi` + `kubectl describe pod` | 重启 nvidia-device-plugin |

<!-- chunk: 生产部署检查清单 -->
## 生产部署检查清单

### 上线前检查

| 序号 | 检查项 | 验证命令 | 通过标准 |
|-----|--------|---------|----------|
| 1 | RuntimeClass 资源已创建 | `kubectl get runtimeclass` | 所有需要的 class 存在 |
| 2 | containerd handler 已配置 | `crictl info \| jq '.config.containerd.runtimes'` | handler 名称匹配 |
| 3 | 节点标签正确 | `kubectl get nodes -l runtime=<name>` | 有足够可用节点 |
| 4 | overhead 已声明 | `kubectl get runtimeclass -o yaml` | podFixed 字段非空 |
| 5 | 准入策略已生效 | 尝试创建未批准的 RuntimeClass Pod | 被拒绝 |
| 6 | 监控告警已配置 | 检查 PrometheusRule | 告警规则存在且有效 |
| 7 | 性能基准已通过 | 运行 benchmark Job | 满足 SLA 要求 |
| 8 | 回滚方案已准备 | 确认默认 runc 可用 | 可快速切换回 runc |

### 运行时切换回滚流程

```bash
#!/bin/bash
# 🟡 中风险：运行时切换回滚脚本
# 将指定 Deployment 从安全运行时回滚到默认 runc
set -euo pipefail

DEPLOYMENT=${1:?"Usage: $0 <deployment> [namespace]"}
NAMESPACE=${2:-default}

echo "=== 运行时回滚: ${NAMESPACE}/${DEPLOYMENT} ==="

# 1. 记录当前状态
CURRENT_RUNTIME=$(kubectl get deployment "$DEPLOYMENT" -n "$NAMESPACE" \
  -o jsonpath='{.spec.template.spec.runtimeClassName}')
echo "当前运行时: ${CURRENT_RUNTIME:-runc(default)}"

# 2. 移除 runtimeClassName（回滚到默认）
kubectl patch deployment "$DEPLOYMENT" -n "$NAMESPACE" \
  --type='json' \
  -p='[{"op": "remove", "path": "/spec/template/spec/runtimeClassName"}]' 2>/dev/null || \
  echo "runtimeClassName 已为默认"

# 3. 等待滚动更新完成
echo "等待滚动更新..."
kubectl rollout status deployment/"$DEPLOYMENT" -n "$NAMESPACE" --timeout=300s

# 4. 验证
echo "=== 验证 ==="
kubectl get pods -n "$NAMESPACE" -l app="$DEPLOYMENT" \
  -o custom-columns='NAME:.metadata.name,RUNTIME:.spec.runtimeClassName,STATUS:.status.phase'

echo "=== 回滚完成 ==="
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 工作负载 KUDIG Database — Global MOC
- [[工作负载/README.md|Domain-4: Kubernetes工作负载管理]]
- index.md|Domain-4 工作负载 — 开源项目索引]]
- 01 - [[概念/kubernetes-architecture-overview.md|kubernetes architecture overview]]
- 02 - Deployment 生产模式与最佳实践 (Deployment Production Patterns)
- 03 - StatefulSet 高级运维指南 (StatefulSet Advanced Operations)
- 04 - DaemonSet 管理策略与最佳实践 (DaemonSet Management Strategies)
- 05 - Job 与 CronJob 高级用法 (Job & CronJob Advanced Usage)
- 06 - 工作负载监控与告警体系 (Workload Monitoring & Alerting System)
- 07 - 工作负载故障排查与应急响应手册 (Workload Troubleshooting & Incident Re...
- 08 - 多云混合部署工作负载管理策略 (Multi-Cloud Hybrid Deployment Workload ...
- 09 - 边缘计算工作负载部署模式 (Edge Computing Workload Deployment Patter...

## See Also

- 14-sidecar-containers-patterns
- 15-container-runtime-interfaces
- 17-container-images-registry
- 18-node-management-operations


<!-- risk-assessed -->
