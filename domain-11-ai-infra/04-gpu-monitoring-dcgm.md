# GPU监控与可观测性

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [DCGM Exporter](https://github.com/NVIDIA/dcgm-exporter) | [GPU Operator](https://docs.nvidia.com/datacenter/cloud-native/gpu-operator/)

## GPU监控架构

```
┌─────────────────────────────────────────────────────────────────┐
│                     GPU指标采集层                                │
│  ┌──────────────────────────────────────────────────────────┐  │
│  │  NVIDIA DCGM (Data Center GPU Manager)                  │  │
│  │  - GPU利用率、温度、功率                                 │  │
│  │  │  - 显存使用、ECC错误                                  │  │
│  │  │  - NVLink拓扑、PCIe流量                              │  │
│  └──────────────┬───────────────────────────────────────────┘  │
└─────────────────┼──────────────────────────────────────────────┘
                  │ DCGM Exporter (Prometheus格式)
                  v
┌─────────────────────────────────────────────────────────────────┐
│                     Prometheus (时序数据库)                       │
│  - 15s粒度采集                                                   │
│  - 30天本地保留                                                  │
│  - Thanos长期存储                                                │
└─────────────────┬───────────────────────────────────────────────┘
                  │
                  v
┌─────────────────────────────────────────────────────────────────┐
│                     可视化与告警层                                │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐            │
│  │  Grafana    │  │ AlertManager│  │  Slack/     │            │
│  │  Dashboard  │  │  (告警)     │  │  PagerDuty  │            │
│  └─────────────┘  └─────────────┘  └─────────────┘            │
└─────────────────────────────────────────────────────────────────┘
```

---

## 一、DCGM Exporter部署

### 1. GPU Operator安装(推荐)

```bash
# 添加NVIDIA Helm仓库
helm repo add nvidia https://helm.ngc.nvidia.com/nvidia
helm repo update

# 安装GPU Operator(包含DCGM Exporter)
helm install --wait --generate-name \
  -n gpu-operator --create-namespace \
  nvidia/gpu-operator \
  --set dcgmExporter.enabled=true \
  --set dcgmExporter.serviceMonitor.enabled=true \
  --set toolkit.enabled=true
```

### 2. 独立部署DCGM Exporter

```yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: dcgm-exporter
  namespace: gpu-monitoring
spec:
  selector:
    matchLabels:
      app: dcgm-exporter
  template:
    metadata:
      labels:
        app: dcgm-exporter
    spec:
      hostNetwork: true
      hostPID: true
      tolerations:
        - key: nvidia.com/gpu
          operator: Exists
          effect: NoSchedule
      
      containers:
        - name: dcgm-exporter
          image: nvcr.io/nvidia/k8s/dcgm-exporter:3.3.0-3.2.0-ubuntu22.04
          
          env:
            # 采集指标配置
            - name: DCGM_EXPORTER_LISTEN
              value: ":9400"
            - name: DCGM_EXPORTER_KUBERNETES
              value: "true"
            - name: DCGM_EXPORTER_COLLECTORS
              value: "/etc/dcgm-exporter/dcp-metrics-included.csv"
          
          ports:
            - name: metrics
              containerPort: 9400
          
          securityContext:
            privileged: true
            capabilities:
              add: ["SYS_ADMIN"]
          
          volumeMounts:
            - name: pod-gpu-resources
              mountPath: /var/lib/kubelet/pod-resources
              readOnly: true
            - name: device-metrics
              mountPath: /run/prometheus
          
          livenessProbe:
            httpGet:
              path: /health
              port: 9400
            initialDelaySeconds: 45
            periodSeconds: 5
          
          readinessProbe:
            httpGet:
              path: /health
              port: 9400
            initialDelaySeconds: 45
      
      volumes:
        - name: pod-gpu-resources
          hostPath:
            path: /var/lib/kubelet/pod-resources
        - name: device-metrics
          emptyDir: {}
---
apiVersion: v1
kind: Service
metadata:
  name: dcgm-exporter
  namespace: gpu-monitoring
  labels:
    app: dcgm-exporter
spec:
  selector:
    app: dcgm-exporter
  ports:
    - name: metrics
      port: 9400
      targetPort: 9400
  type: ClusterIP
```

### 3. ServiceMonitor配置

```yaml
apiVersion: monitoring.coreos.com/v1
kind: ServiceMonitor
metadata:
  name: dcgm-exporter
  namespace: gpu-monitoring
  labels:
    app: dcgm-exporter
spec:
  selector:
    matchLabels:
      app: dcgm-exporter
  endpoints:
    - port: metrics
      interval: 15s
      path: /metrics
      relabelings:
        # 添加节点标签
        - sourceLabels: [__meta_kubernetes_pod_node_name]
          targetLabel: node
        # 添加命名空间标签
        - sourceLabels: [__meta_kubernetes_namespace]
          targetLabel: namespace
        # 添加Pod标签
        - sourceLabels: [__meta_kubernetes_pod_name]
          targetLabel: pod
```

---

## 二、核心GPU指标

### 1. 关键指标清单

| 指标 | Prometheus Metric | 说明 | 告警阈值 |
|------|------------------|------|---------|
| **GPU利用率** | `DCGM_FI_DEV_GPU_UTIL` | GPU计算单元使用率 | <30%(浪费) >95%(饱和) |
| **显存使用** | `DCGM_FI_DEV_FB_USED` | 已用显存(MB) | >90% |
| **显存总量** | `DCGM_FI_DEV_FB_TOTAL` | 总显存(MB) | - |
| **GPU温度** | `DCGM_FI_DEV_GPU_TEMP` | GPU温度(℃) | >85℃ |
| **功率消耗** | `DCGM_FI_DEV_POWER_USAGE` | 当前功率(W) | >350W(A100) |
| **显存温度** | `DCGM_FI_DEV_MEMORY_TEMP` | 显存温度(℃) | >95℃ |
| **SM活跃度** | `DCGM_FI_PROF_SM_ACTIVE` | 流处理器活跃度 | <50% |
| **SM占用率** | `DCGM_FI_PROF_SM_OCCUPANCY` | SM占用率 | <60% |
| **Tensor Core利用率** | `DCGM_FI_PROF_PIPE_TENSOR_ACTIVE` | Tensor Core活跃度 | <30%(AI训练) |
| **FP16活跃度** | `DCGM_FI_PROF_PIPE_FP16_ACTIVE` | FP16计算活跃度 | - |
| **显存带宽利用率** | `DCGM_FI_PROF_DRAM_ACTIVE` | 显存带宽使用 | <70% |
| **PCIe发送** | `DCGM_FI_PROF_PCIE_TX_BYTES` | PCIe发送字节 | >10GB/s |
| **PCIe接收** | `DCGM_FI_PROF_PCIE_RX_BYTES` | PCIe接收字节 | >10GB/s |
| **NVLink流量** | `DCGM_FI_PROF_NVLINK_TX_BYTES` | NVLink发送 | >50GB/s |
| **XID错误** | `DCGM_FI_DEV_XID_ERRORS` | 硬件错误码 | >0 |
| **ECC单比特错误** | `DCGM_FI_DEV_ECC_SBE_VOL_TOTAL` | 可纠正显存错误 | >100/天 |
| **ECC双比特错误** | `DCGM_FI_DEV_ECC_DBE_VOL_TOTAL` | 不可纠正错误 | >0 |

---

### 2. PromQL查询示例

#### GPU利用率

```promql
# 单GPU利用率
DCGM_FI_DEV_GPU_UTIL{gpu="0", kubernetes_node="node-1"}

# 集群平均GPU利用率
avg(DCGM_FI_DEV_GPU_UTIL)

# 按节点统计平均GPU利用率
avg(DCGM_FI_DEV_GPU_UTIL) by (kubernetes_node)

# 按Pod统计GPU利用率
avg(DCGM_FI_DEV_GPU_UTIL{pod=~"training-.*"}) by (pod)

# GPU利用率低于30%(资源浪费)
DCGM_FI_DEV_GPU_UTIL < 30
```

#### 显存使用率

```promql
# 显存使用率(%)
(DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_TOTAL) * 100

# 显存使用率 > 90%
(DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_TOTAL) * 100 > 90

# 按命名空间统计显存使用
sum(DCGM_FI_DEV_FB_USED) by (namespace)

# 可用显存
DCGM_FI_DEV_FB_FREE
```

#### Tensor Core利用率

```promql
# Tensor Core活跃度(AI训练关键指标)
DCGM_FI_PROF_PIPE_TENSOR_ACTIVE

# 平均Tensor Core利用率
avg(DCGM_FI_PROF_PIPE_TENSOR_ACTIVE)

# Tensor Core利用率低于30%(训练效率低)
DCGM_FI_PROF_PIPE_TENSOR_ACTIVE < 30
```

#### GPU温度与功率

```promql
# GPU温度超过85℃
DCGM_FI_DEV_GPU_TEMP > 85

# 功率消耗趋势(5分钟平均)
avg_over_time(DCGM_FI_DEV_POWER_USAGE[5m])

# 功率超过额定值(A100=400W)
DCGM_FI_DEV_POWER_USAGE > 400
```

#### NVLink流量

```promql
# NVLink发送速率(GB/s)
rate(DCGM_FI_PROF_NVLINK_TX_BYTES[1m]) / 1024 / 1024 / 1024

# NVLink接收速率
rate(DCGM_FI_PROF_NVLINK_RX_BYTES[1m]) / 1024 / 1024 / 1024

# 总NVLink带宽
(rate(DCGM_FI_PROF_NVLINK_TX_BYTES[1m]) + 
 rate(DCGM_FI_PROF_NVLINK_RX_BYTES[1m])) / 1024 / 1024 / 1024
```

#### 错误检测

```promql
# XID错误(硬件故障)
increase(DCGM_FI_DEV_XID_ERRORS[1h]) > 0

# ECC单比特错误趋势
rate(DCGM_FI_DEV_ECC_SBE_VOL_TOTAL[1h])

# ECC双比特错误(严重)
increase(DCGM_FI_DEV_ECC_DBE_VOL_TOTAL[1h]) > 0
```

---

## 三、Prometheus告警规则

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: gpu-alerts
  namespace: gpu-monitoring
spec:
  groups:
    - name: gpu-health
      interval: 30s
      rules:
        # ========== 资源利用率告警 ==========
        
        - alert: GPULowUtilization
          expr: |
            avg_over_time(DCGM_FI_DEV_GPU_UTIL[10m]) < 30
          for: 30m
          labels:
            severity: warning
            team: ml-platform
          annotations:
            summary: "GPU利用率过低"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              利用率过低: {{ $value | humanizePercentage }}
              
              可能原因:
              - 训练任务配置不当
              - 数据加载瓶颈
              - 模型计算强度低
              
              排查命令:
              kubectl exec -it {{ $labels.pod }} -n {{ $labels.namespace }} -- nvidia-smi
        
        - alert: GPUHighMemoryUsage
          expr: |
            (DCGM_FI_DEV_FB_USED / DCGM_FI_DEV_FB_TOTAL) * 100 > 95
          for: 5m
          labels:
            severity: critical
            team: ml-platform
          annotations:
            summary: "GPU显存即将耗尽"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              显存使用率: {{ $value | humanizePercentage }}
              
              已用: {{ query "DCGM_FI_DEV_FB_USED" | first | value }}MB
              总量: {{ query "DCGM_FI_DEV_FB_TOTAL" | first | value }}MB
              
              可能导致OOM崩溃！
        
        - alert: LowTensorCoreUtilization
          expr: |
            avg_over_time(DCGM_FI_PROF_PIPE_TENSOR_ACTIVE[10m]) < 30
          for: 30m
          labels:
            severity: warning
            team: ml-platform
          annotations:
            summary: "Tensor Core利用率低"
            description: |
              Pod {{ $labels.pod }} 的 Tensor Core 利用率仅 {{ $value | humanizePercentage }}
              
              优化建议:
              - 启用混合精度训练(FP16/BF16)
              - 检查是否使用Tensor Core优化算子
              - 增大batch size
        
        # ========== 硬件健康告警 ==========
        
        - alert: GPUHighTemperature
          expr: |
            DCGM_FI_DEV_GPU_TEMP > 85
          for: 10m
          labels:
            severity: critical
            team: infra
          annotations:
            summary: "GPU温度过高"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              温度达到 {{ $value }}℃ (阈值: 85℃)
              
              可能原因:
              - 散热故障
              - 环境温度过高
              - 负载过大
              
              立即检查节点物理状态！
        
        - alert: GPUMemoryTemperatureHigh
          expr: |
            DCGM_FI_DEV_MEMORY_TEMP > 95
          for: 5m
          labels:
            severity: critical
            team: infra
          annotations:
            summary: "显存温度异常"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              显存温度: {{ $value }}℃ (阈值: 95℃)
              
              可能导致硬件损坏！
        
        - alert: GPUXIDError
          expr: |
            increase(DCGM_FI_DEV_XID_ERRORS[5m]) > 0
          labels:
            severity: critical
            team: infra
          annotations:
            summary: "检测到GPU硬件错误"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              发生 XID 错误 (错误码: {{ $value }})
              
              常见XID错误:
              - 13: 图形引擎异常
              - 31/32: GPU内存页错误
              - 43: GPU驱动错误
              - 48: 双比特ECC错误
              - 79: GPU陷入死循环
              
              建议立即隔离该GPU！
        
        - alert: ECCDoubleBitError
          expr: |
            increase(DCGM_FI_DEV_ECC_DBE_VOL_TOTAL[1h]) > 0
          labels:
            severity: critical
            team: infra
          annotations:
            summary: "检测到ECC双比特错误"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              发生不可纠正的显存错误
              
              该GPU需要更换！
        
        - alert: ECCSingleBitErrorHigh
          expr: |
            rate(DCGM_FI_DEV_ECC_SBE_VOL_TOTAL[1h]) > 100
          for: 1h
          labels:
            severity: warning
            team: infra
          annotations:
            summary: "ECC单比特错误率过高"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              单比特ECC错误率: {{ $value }}/小时
              
              虽可自动纠正，但错误率过高可能预示硬件老化
        
        # ========== 通信性能告警 ==========
        
        - alert: LowNVLinkBandwidth
          expr: |
            (rate(DCGM_FI_PROF_NVLINK_TX_BYTES[5m]) + 
             rate(DCGM_FI_PROF_NVLINK_RX_BYTES[5m])) 
            / 1024 / 1024 / 1024 < 10
          for: 15m
          labels:
            severity: warning
            team: ml-platform
          annotations:
            summary: "NVLink带宽异常低"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 NVLink 总带宽: {{ $value }}GB/s
              
              可能原因:
              - 分布式训练通信不频繁
              - NVLink硬件故障
              - 训练框架配置问题
        
        - alert: GPUThrottling
          expr: |
            DCGM_FI_DEV_CLOCK_THROTTLE_REASONS > 0
          for: 10m
          labels:
            severity: warning
            team: infra
          annotations:
            summary: "GPU被降频"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 GPU {{ $labels.gpu }}
              被降频 (原因码: {{ $value }})
              
              降频原因:
              - 1: GPU空闲
              - 2: 应用时钟设置
              - 4: 软件功率限制
              - 8: 硬件慢速阈值
              - 16: 硬件热慢速
              - 32: 软件热慢速
              - 64: 同步Boost
              
              影响训练性能！
        
        # ========== 集群级告警 ==========
        
        - alert: GPUClusterLowUtilization
          expr: |
            avg(DCGM_FI_DEV_GPU_UTIL) < 40
          for: 1h
          labels:
            severity: info
            team: ml-platform
          annotations:
            summary: "集群GPU利用率低"
            description: |
              集群整体GPU利用率: {{ $value | humanizePercentage }}
              
              成本优化建议:
              - 缩减GPU节点数量
              - 启用GPU共享(MIG/cGPU)
              - 调整任务调度策略
        
        - alert: GPUNodeDown
          expr: |
            up{job="dcgm-exporter"} == 0
          for: 5m
          labels:
            severity: critical
            team: infra
          annotations:
            summary: "GPU节点离线"
            description: |
              节点 {{ $labels.kubernetes_node }} 的 DCGM Exporter 无法访问
              
              排查步骤:
              1. kubectl get nodes {{ $labels.kubernetes_node }}
              2. kubectl describe node {{ $labels.kubernetes_node }}
              3. kubectl logs -n gpu-monitoring -l app=dcgm-exporter --tail=100
```

---

## 四、Grafana Dashboard

### 1. GPU集群总览Dashboard

```json
{
  "dashboard": {
    "title": "GPU Cluster Overview",
    "panels": [
      {
        "title": "GPU总数与在线率",
        "type": "stat",
        "targets": [
          {
            "expr": "count(DCGM_FI_DEV_GPU_UTIL)",
            "legendFormat": "Total GPUs"
          },
          {
            "expr": "count(DCGM_FI_DEV_GPU_UTIL > 0)",
            "legendFormat": "Active GPUs"
          }
        ]
      },
      {
        "title": "集群GPU利用率",
        "type": "graph",
        "targets": [
          {
            "expr": "avg(DCGM_FI_DEV_GPU_UTIL)",
            "legendFormat": "Average Utilization"
          }
        ]
      },
      {
        "title": "GPU利用率分布(热力图)",
        "type": "heatmap",
        "targets": [
          {
            "expr": "DCGM_FI_DEV_GPU_UTIL",
            "format": "time_series"
          }
        ]
      },
      {
        "title": "显存使用情况",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(DCGM_FI_DEV_FB_USED) / 1024",
            "legendFormat": "Used (GB)"
          },
          {
            "expr": "sum(DCGM_FI_DEV_FB_TOTAL) / 1024",
            "legendFormat": "Total (GB)"
          }
        ]
      },
      {
        "title": "GPU温度分布",
        "type": "graph",
        "targets": [
          {
            "expr": "DCGM_FI_DEV_GPU_TEMP",
            "legendFormat": "{{kubernetes_node}}-gpu{{gpu}}"
          }
        ],
        "alert": {
          "conditions": [
            {
              "evaluator": {
                "params": [85],
                "type": "gt"
              },
              "operator": {
                "type": "and"
              },
              "query": {
                "params": ["A", "5m", "now"]
              },
              "reducer": {
                "type": "avg"
              },
              "type": "query"
            }
          ]
        }
      },
      {
        "title": "Tensor Core利用率(训练任务)",
        "type": "graph",
        "targets": [
          {
            "expr": "avg(DCGM_FI_PROF_PIPE_TENSOR_ACTIVE{namespace=\"ai-training\"})",
            "legendFormat": "Tensor Core Active"
          }
        ]
      },
      {
        "title": "NVLink总带宽",
        "type": "graph",
        "targets": [
          {
            "expr": "sum(rate(DCGM_FI_PROF_NVLINK_TX_BYTES[1m]) + rate(DCGM_FI_PROF_NVLINK_RX_BYTES[1m])) / 1024 / 1024 / 1024",
            "legendFormat": "Total Bandwidth (GB/s)"
          }
        ]
      },
      {
        "title": "GPU错误统计",
        "type": "table",
        "targets": [
          {
            "expr": "DCGM_FI_DEV_XID_ERRORS > 0",
            "format": "table"
          }
        ]
      }
    ]
  }
}
```

---

## 五、训练任务专属监控

### 1. PyTorchJob监控

```yaml
apiVersion: kubeflow.org/v1
kind: PyTorchJob
metadata:
  name: monitored-training
  namespace: ai-training
spec:
  pytorchReplicaSpecs:
    Master:
      replicas: 1
      template:
        metadata:
          annotations:
            # Prometheus抓取配置
            prometheus.io/scrape: "true"
            prometheus.io/port: "8000"
            prometheus.io/path: "/metrics"
        spec:
          containers:
            - name: pytorch
              image: pytorch/pytorch:2.1.0
              command:
                - python
                - train.py
              ports:
                - containerPort: 8000  # Prometheus metrics端口
              env:
                - name: RANK
                  value: "0"
                - name: WORLD_SIZE
                  value: "8"
              resources:
                limits:
                  nvidia.com/gpu: 8
```

### 2. 训练指标导出

```python
from prometheus_client import start_http_server, Gauge, Counter

# 定义指标
training_loss = Gauge('training_loss', 'Current training loss')
training_accuracy = Gauge('training_accuracy', 'Current training accuracy')
training_throughput = Gauge('training_throughput_samples_per_sec', 
                            'Training throughput')
gpu_memory_allocated = Gauge('gpu_memory_allocated_bytes', 
                             'GPU memory allocated', ['gpu_id'])
gpu_memory_reserved = Gauge('gpu_memory_reserved_bytes', 
                            'GPU memory reserved', ['gpu_id'])
training_step = Counter('training_steps_total', 'Total training steps')

# 启动metrics服务器
start_http_server(8000)

# 训练循环中更新指标
for epoch in range(num_epochs):
    for batch in dataloader:
        loss = train_step(batch)
        
        # 更新指标
        training_loss.set(loss.item())
        training_step.inc()
        
        # GPU显存监控
        for i in range(torch.cuda.device_count()):
            allocated = torch.cuda.memory_allocated(i)
            reserved = torch.cuda.memory_reserved(i)
            gpu_memory_allocated.labels(gpu_id=str(i)).set(allocated)
            gpu_memory_reserved.labels(gpu_id=str(i)).set(reserved)
```

---

## 六、成本监控

### 1. GPU成本计算

```promql
# 每小时GPU成本(假设A100=$3/GPU/小时)
sum(DCGM_FI_DEV_GPU_UTIL > 0) * 3

# 按命名空间统计GPU使用成本
sum(DCGM_FI_DEV_GPU_UTIL > 0) by (namespace) * 3

# 低效GPU成本(利用率<30%)
sum(DCGM_FI_DEV_GPU_UTIL < 30 and DCGM_FI_DEV_GPU_UTIL > 0) * 3

# 每日成本估算
sum(DCGM_FI_DEV_GPU_UTIL > 0) * 3 * 24
```

---

## 七、生产最佳实践

### GPU监控检查清单

- ✅ 部署DCGM Exporter到所有GPU节点
- ✅ 配置Prometheus 15s粒度采集
- ✅ 启用ServiceMonitor自动发现
- ✅ 配置30天本地数据保留
- ✅ 集成Thanos长期存储
- ✅ 创建Grafana GPU总览Dashboard
- ✅ 配置GPU温度/显存告警
- ✅ 监控ECC错误和XID错误
- ✅ 跟踪Tensor Core利用率
- ✅ 监控NVLink通信带宽
- ✅ 集成训练任务自定义指标
- ✅ 配置Slack/PagerDuty告警通知
- ✅ 建立GPU成本分析Dashboard

---

**表格维护**: Kusheet Project | **作者**: Allen Galler (allengaller@gmail.com)
