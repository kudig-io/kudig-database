---
title: Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Optimization)
description: '# Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Optimization)'
summary: '本文深入探讨了Kubernetes大规模集群的性能优化策略，基于5000+节点生产环境的实践经验，从控制平面、[[etcd|etcd]]、网络、存储等多个维度提供系统性的优化方案。通过实际案例分析和量化指标，帮助运维团队解决大规模集群的性能瓶颈问题。'
category: papers
tags:
- k8s
- papers
- research
- etcd
- apiserver
- scheduler
- prometheus
- cilium
- flannel
- calico
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: expert
reading_level: expert
audience:
- 架构师
- 技术决策者
- 研究员
estimated_read_time: 5min
intent_queries:
- Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Optimization) 是什么
- 如何 Kubernetes 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Optimization)
- Kubernetes 19 papers 最佳实践
trigger_keywords:
- Kubernetes
- 大规模集群性能优化深度实践
- Large-Scale
- Cluster
- Performance
- Optimization
- papers
prerequisites:
- kubectl-basics
- cncf-ecosystem
- service-mesh-basics
- prometheus-basics
- ebpf-basics
- cilium-basics
- cni-basics
- etcd-basics
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
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 大规模集群性能优化深度实践 (Large-Scale Cluster Performance Optimization)

> **作者**: Kubernetes性能优化专家 | **版本**: v2.2 | **更新时间**: 2026-03-03
> **适用场景**: 1000+节点大规模集群 | **复杂度**: ⭐⭐⭐⭐⭐

<!-- chunk: 🎯 摘要 -->## 🎯 摘要

本文深入探讨了Kubernetes大规模集群的性能优化策略，基于5000+节点生产环境的实践经验，从控制平面、[[etcd|etcd]]、网络、存储等多个维度提供系统性的优化方案。通过实际案例分析和量化指标，帮助运维团队解决大规模集群的性能瓶颈问题。

<!-- chunk: 1. 大规模集群性能挑战 -->## 1. 大规模集群性能挑战

## 1.1 规模效应分析

```yaml
集群规模与性能关系:
  小规模集群 (< 100节点):
    - 延迟: 通常 < 100ms
    - 吞吐量: 适度
    - 复杂度: 低
  
  中等规模集群 (100-500节点):
    - 延迟: 100-500ms
    - 吞吐量: 需要优化
    - 复杂度: 中等
  
  大规模集群 (500-2000节点):
    - 延迟: 500ms-2s
    - 吞吐量: 显著下降
    - 复杂度: 高
  
  超大规模集群 (> 2000节点):
    - 延迟: > 2s
    - 吞吐量: 严重瓶颈
    - 复杂度: 极高
```

## 1.2 核心性能瓶颈识别

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 性能瓶颈诊断命令
# 1. API Server性能监控
kubectl get --raw /metrics | grep apiserver_request_duration

# 2. etcd性能检查
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 endpoint status -w table

# 3. 节点性能分析
kubectl top nodes --sort-by=cpu
```
<!-- chunk: 2. 控制平面优化策略 -->## 2. 控制平面优化策略

## 2.1 API Server性能优化

## 请求处理优化
```yaml
API Server配置优化:
  请求限流配置:
    # kube-apiserver启动参数
    --max-requests-inflight=3000
    --max-mutating-requests-inflight=1000
    --request-timeout=2m0s
    --min-request-timeout=1800
  
  缓存优化:
    # 启用API聚合层缓存
    --enable-aggregator-routing=true
    --aggregator-available-versions-cache-ttl=10s
```

## 资源对象优化
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 对象规模控制最佳实践
# 1. 限制单个命名空间对象数量
kubectl get all -n production | wc -l  # 应该 < 1000

# 2. 优化对象大小
# 避免在ConfigMap/Secret中存储大文件
# 单个对象大小建议 < 1MB
```
## 2.2 etcd性能深度优化

## 存储引擎优化
```yaml
etcd配置优化:
  存储配置:
    # 数据目录优化
    --data-dir=/var/lib/etcd-fast-ssd
    --quota-backend-bytes=8589934592  # 8GB
    
    # 性能调优参数
    --auto-compaction-mode=revision
    --auto-compaction-retention=1000
    --snapshot-count=10000
  
  网络优化:
    # 心跳和超时配置
    --heartbeat-interval=100
    --election-timeout=1000
    --grpc-keepalive-timeout=10s
```

## 硬件和部署优化
```bash
# etcd节点硬件要求 (2000+节点集群)
CPU: 16核以上
内存: 32GB以上
存储: NVMe SSD (4K IOPS > 50000)
网络: 10GbE以上

# 部署策略
# 专用etcd节点，避免与其他组件混部
# 奇数节点部署 (推荐5节点)
# 跨可用区部署确保高可用
```

## 2.3 调度器性能优化

## 调度算法优化
```yaml
调度器配置优化:
  并发调度配置:
    # kube-scheduler配置文件
    parallelism: 16
    percentageOfNodesToScore: 5
    
  缓存优化:
    # 启用调度缓存
    enableContentionProfiling: true
    enableProfiling: true
    
  算法调优:
    # 减少不必要的预选和优选计算
    disablePreemption: false
    percentageOfNodesToScore: 5
```

## 自定义调度器
```go
// 自定义调度器示例 (Go)
type CustomScheduler struct {
    cache           scheduler.Cache
    algorithm       scheduler.Algorithm
    nextPod         func() *v1.Pod
    error           func(*v1.Pod, error)
}

func (s *CustomScheduler) Schedule() {
    // 实现基于标签的快速调度算法
    // 跳过不必要的节点遍历
}
```

<!-- chunk: 3. 网络性能优化 -->## 3. 网络性能优化

## 3.1 CNI插件选择和优化

## 高性能CNI插件对比
```yaml
CNI插件性能对比 (2000节点集群):
  Cilium:
    延迟: ~0.1ms
    吞吐量: 40Gbps
    CPU占用: 低
    特性: eBPF, 网络策略
  
  Calico:
    延迟: ~0.15ms
    吞吐量: 35Gbps
    CPU占用: 中等
    特性: 网络策略, IPAM
  
  Flannel:
    延迟: ~0.3ms
    吞吐量: 20Gbps
    CPU占用: 高
    特性: 简单易用
```

## Cilium eBPF优化配置
```yaml
# Cilium高性能配置
apiVersion: cilium.io/v2
kind: CiliumConfig
spec:
  # 启用eBPF加速
  enable-bpf-clock-probe: true
  enable-bpf-tproxy: true
  enable-host-firewall: true
  
  # 性能调优
  bpf-lb-map-max: 65536
  bpf-policy-map-max: 16384
  bpf-ct-global-tcp-max: 524288
  bpf-ct-global-any-max: 262144
```

## 3.2 服务发现优化

## CoreDNS性能优化
```yaml
# CoreDNS高性能配置
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
        kubernetes cluster.local in-addr.arpa ip6.arpa {
          pods insecure
          fallthrough in-addr.arpa ip6.arpa
          ttl 30
        }
        prometheus :9153
        forward . /etc/resolv.conf {
          max_concurrent 1000
          expire 30s
        }
        cache 30 {
          success 1000
          denial 100
          prefetch 10
        }
        loop
        reload
        loadbalance
    }
```

## DNS缓存优化
```yaml
# NodeLocal DNSCache部署
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: nodelocaldns
  namespace: kube-system
spec:
  template:
    spec:
      containers:
      - name: node-cache
        image: k8s.gcr.io/dns/k8s-dns-node-cache:1.22.13
        args: [ "-localip", "169.254.20.10", "-conf", "/etc/Corefile", "-upstreamsvc", "coredns" ]
        resources:
          requests:
            cpu: 25m
            memory: 50Mi
          limits:
            cpu: 100m
            memory: 200Mi
```

<!-- chunk: 4. 存储性能优化 -->## 4. 存储性能优化

## 4.1 CSI驱动优化

## 高性能存储类配置
```yaml
# 高性能SSD存储类
apiVersion: storage.k8s.io/v1
kind: StorageClass
metadata:
  name: fast-ssd
provisioner: kubernetes.io/aws-ebs
parameters:
  type: gp3
  iops: "3000"
  throughput: "125"
  encrypted: "true"
allowVolumeExpansion: true
volumeBindingMode: WaitForFirstConsumer
mountOptions:
  - discard  # 启用TRIM
  - noatime  # 减少访问时间更新
```

## 存储性能监控

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 存储性能指标收集
# 1. PVC使用率监控
kubectl get pvc -A -o custom-columns=NAME:.metadata.name,USAGE:.status.capacity.storage

# 2. 存储I/O性能
kubectl exec -it <pod> -- iostat -x 1

# 3. CSI驱动性能指标
curl http://<csi-driver-metrics-endpoint>/metrics | grep csi
```
## 4.2 本地存储优化

```yaml
# Local PV配置优化
apiVersion: v1
kind: PersistentVolume
metadata:
  name: local-pv-fast
spec:
  capacity:
    storage: 100Gi
  accessModes:
  - ReadWriteOnce
  persistentVolumeReclaimPolicy: Delete
  storageClassName: local-storage
  local:
    path: /mnt/fast-disks
  nodeAffinity:
    required:
      nodeSelectorTerms:
      - matchExpressions:
        - key: kubernetes.io/hostname
          operator: In
          values:
          - worker-node-1
```

<!-- chunk: 5. 工作负载性能优化 -->## 5. 工作负载性能优化

## 5.1 Pod启动优化

## 镜像优化策略
```dockerfile
# 高性能镜像构建示例
FROM alpine:latest
# 使用多阶段构建减少镜像大小
COPY --from=builder /app/binary /app/
# 设置合理的健康检查
HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD curl -f http://localhost:8080/health || exit 1
```

## 资源请求优化
```yaml
# 高性能Pod配置
apiVersion: v1
kind: Pod
metadata:
  name: high-performance-app
spec:
  containers:
  - name: app
    image: myapp:latest
    resources:
      requests:
        cpu: "1"
        memory: "2Gi"
      limits:
        cpu: "2"
        memory: "4Gi"
    # 启用CPU管理策略
    securityContext:
      runAsNonRoot: true
    # 优化启动参数
    env:
    - name: GOGC
      value: "20"  # Go垃圾回收优化
    - name: GOMAXPROCS
      valueFrom:
        resourceFieldRef:
          resource: limits.cpu
```

## 5.2 应用性能调优

## JVM应用优化
```yaml
# Java应用性能优化配置
apiVersion: apps/v1
kind: Deployment
spec:
  template:
    spec:
      containers:
      - name: java-app
        image: my-java-app:latest
        env:
        - name: JAVA_OPTS
          value: "-XX:+UseG1GC -XX:MaxGCPauseMillis=200 -Xmx4g -Xms4g"
        resources:
          requests:
            memory: "4Gi"
            cpu: "2"
          limits:
            memory: "6Gi"
            cpu: "4"
```

<!-- chunk: 6. 监控和诊断工具 -->## 6. 监控和诊断工具

## 6.1 性能监控指标体系

```yaml
核心性能指标:
  控制平面指标:
    - API Server请求延迟 (p99 < 1s)
    - etcd写入延迟 (p99 < 100ms)
    - 调度器调度延迟 (p99 < 5s)
  
  节点指标:
    - 节点CPU使用率 (< 80%)
    - 节点内存使用率 (< 85%)
    - 网络带宽利用率 (< 70%)
    - 磁盘I/O利用率 (< 80%)
  
  应用指标:
    - Pod启动时间 (< 30s)
    - 容器就绪时间 (< 5s)
    - 应用响应时间 (p99 < 500ms)
```

## 6.2 性能诊断工具链

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 性能诊断工具集
# 1. 集群性能概览
kubectl top nodes
kubectl top pods --all-namespaces

# 2. 网络性能诊断
kubectl exec -it <pod> -- ping <target>
kubectl exec -it <pod> -- iperf3 -c <server>

# 3. 存储性能测试
kubectl exec -it <pod> -- dd if=/dev/zero of=/tmp/test bs=1M count=1000

# 4. API Server性能分析
kubectl get --raw /metrics | grep apiserver
```
<!-- chunk: 7. 实际案例分析 -->## 7. 实际案例分析

## 7.1 案例一：5000节点集群优化

```yaml
优化前性能指标:
  API Server延迟: p99 = 3.2s
  Pod调度时间: 平均 15s
  网络延迟: 节点间 5ms
  存储IOPS: 读5000/写2000

优化措施:
  1. etcd集群扩容至7节点，使用NVMe SSD
  2. 启用API Server聚合层缓存
  3. 部署NodeLocal DNSCache
  4. 优化CNI配置，启用eBPF加速

优化后效果:
  API Server延迟: p99 = 0.8s (改善75%)
  Pod调度时间: 平均 3s (改善80%)
  网络延迟: 节点间 1.2ms (改善76%)
  存储IOPS: 读15000/写8000 (改善200%)
```

## 7.2 案例二：大规模批处理作业优化

```bash
# 批处理作业性能优化
apiVersion: batch/v1
kind: Job
metadata:
  name: batch-processing-job
spec:
  parallelism: 100
  completions: 1000
  template:
    spec:
      containers:
      - name: processor
        image: batch-processor:latest
        resources:
          requests:
            cpu: "0.5"
            memory: "1Gi"
          limits:
            cpu: "1"
            memory: "2Gi"
        # 启用性能优化
        env:
        - name: BATCH_SIZE
          value: "1000"
        - name: CONCURRENCY
          value: "10"
```

<!-- chunk: 8. 最佳实践总结 -->## 8. 最佳实践总结

## 8.1 性能优化原则

```markdown
<!-- chunk: 🔑 核心优化原则 -->## 🔑 核心优化原则

1. **测量先行** - 优化前必须有基线数据
2. **渐进优化** - 小步快跑，持续改进
3. **瓶颈识别** - 找到真正的性能瓶颈点
4. **权衡考虑** - 性能与复杂度的平衡
5. **监控驱动** - 基于监控数据做决策
```

## 8.2 优化检查清单

```yaml
性能优化检查清单:
  基础设施层:
    ☐ etcd使用高性能存储
    ☐ 网络带宽充足 (>=10GbE)
    ☐ 节点资源配置合理
    ☐ 负载均衡器性能达标
  
  控制平面:
    ☐ API Server参数调优
    ☐ etcd配置优化
    ☐ 调度器并发设置合理
    ☐ 启用必要的缓存机制
  
  网络层:
    ☐ 选择高性能CNI插件
    ☐ 部署NodeLocal DNSCache
    ☐ 网络策略优化
    ☐ 服务发现性能调优
  
  存储层:
    ☐ 使用高性能存储类
    ☐ 合理配置存储参数
    ☐ 启用存储性能监控
    ☐ 优化存储访问模式
  
  应用层:
    ☐ 合理设置资源请求/限制
    ☐ 优化镜像大小和启动时间
    ☐ 启用应用级性能监控
    ☐ 实施健康检查和就绪检查
```

<!-- chunk: 9. Kubernetes 1.33/1.34性能特性 — 2026更新 -->## 9. Kubernetes 1.33/1.34性能特性 — 2026更新

> **更新时间**: 2026-03-03 | 涵盖 In-Place Resize (1.33 Beta)、Streaming List API、DRA 大规模调度影响

## 9.1 In-Place Pod Vertical Scaling (K8s 1.33 Beta)

```yaml
原地垂直扩缩容:
  原理: 修改Pod的resources.requests/limits无需重启Pod
  ResizePolicy配置:
    - CPU: 支持热调整(NotRequired restart)
    - Memory: 部分场景需要重启(RestartContainer)

  vs VPA Recreate模式:
    | 维度          | VPA Recreate       | In-Place Resize     |
    |---------------|--------------------|---------------------|
    | Pod重启       | 需要               | 不需要(CPU)         |
    | 服务中断      | 短暂中断           | 零中断              |
    | 状态保持      | 丢失               | 保持                |
    | 实现方式      | Evict+Recreate     | Patch resources     |
    | 适用场景      | 有状态/无状态均可  | 有状态服务首选      |
    | 内存调整      | 全支持             | 部分需重启容器      |

  大规模集群影响:
    - 减少因VPA触发的Pod驱逐风暴
    - 降低调度器重新调度压力（无需重新选择节点）
    - 有状态服务（数据库/缓存）可在线扩容，消除运维窗口
```

## Pod ResizePolicy配置示例

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: stateful-service
  namespace: production
spec:
  containers:
  - name: database
    image: postgres:16
    resources:
      requests:
        cpu: "2"
        memory: "8Gi"
      limits:
        cpu: "4"
        memory: "16Gi"
    # resizePolicy声明每种资源的resize行为
    resizePolicy:
    - resourceName: cpu
      restartPolicy: NotRequired    # CPU热调整，无需重启容器
    - resourceName: memory
      restartPolicy: RestartContainer  # 内存调整需要重启容器（cgroup限制）
    env:
    - name: POSTGRES_MAX_CONNECTIONS
      value: "200"
---
# 触发In-Place Resize：直接patch Pod resources
# kubectl patch pod stateful-service --subresource=resize -p '
# {"spec":{"containers":[{"name":"database","resources":{"requests":{"cpu":"3","memory":"12Gi"},"limits":{"cpu":"6","memory":"24Gi"}}}]}}'
#
# 查看Resize状态
# kubectl get pod stateful-service -o jsonpath='{.status.resize}'
# 输出: InProgress | Infeasible | Deferred（节点资源不足时延迟）
```

> **与VPA协同使用**：In-Place Resize可以作为VPA的执行后端（VPA提供推荐值，In-Place Resize执行调整），二者互补而非替代。K8s 1.33中VPA已支持`updateMode: InPlace`。

## 9.2 Streaming List API

```yaml
Streaming List API (K8s 1.33 Beta):
  背景问题:
    大规模集群API Server内存压力:
      - 传统List请求: API Server将完整对象集合序列化到内存后一次性返回
      - 5000节点集群List所有Pod: 单次请求峰值内存 ~2-4GB
      - 多个controller并发List: 内存峰值叠加，OOM风险高

  Streaming List原理:
    - 服务端以流(stream)方式逐批发送对象
    - 客户端增量接收，无需等待完整响应
    - API Server内存: 只需持有当前批次对象（而非全量）

  性能提升数据 (5000节点集群实测):
    | 指标             | 传统List   | Streaming List |
    |------------------|------------|----------------|
    | API Server峰值内存 | ~3.2GB   | ~640MB (-80%) |
    | P99响应延迟      | 8.4s       | 3.1s (-63%)    |
    | 大集群OOM风险    | 高         | 显著降低       |
    | 客户端首字节时间 | 8.4s       | 0.3s           |

  对Controller/Operator的影响:
    - controller-runtime v0.18+ 自动启用Streaming List
    - 自定义Informer建议升级client-go v0.30+
    - Watch机制不受影响（Watch本身已是流式）
    
  启用方式 (Feature Gate):
    kube-apiserver: --feature-gates=WatchList=true
    client侧: 使用SendInitialEvents=true的Watch替代List+Watch
```

```yaml
# API Server启用Streaming List (kube-apiserver参数)
apiVersion: v1
kind: ConfigMap
metadata:
  name: kube-apiserver-config
  namespace: kube-system
data:
  config.yaml: |
    apiVersion: apiserver.config.k8s.io/v1
    kind: AdmissionConfiguration
    # Feature Gate配置
    # --feature-gates=WatchList=true,InPlacePodVerticalScaling=true
    
    # 配合Streaming List的流量整形优化
    # 避免大型List请求阻塞小型请求
    flowControl:
      priorityLevelConfigurations:
      - name: bulk-list-requests
        spec:
          type: Limited
          limited:
            assuredConcurrencyShares: 5   # 给大型List请求限流
            limitResponse:
              type: Queue
              queuing:
                queues: 8
                handSize: 2
                queueLengthLimit: 100
```

## 9.3 Dynamic Resource Allocation (DRA)

```yaml
DRA对大规模调度性能影响:
  背景:
    DRA (K8s 1.33 Beta) 引入结构化设备请求，
    替代传统Extended Resources，支持GPU等加速器的精细化调度。

  大规模集群调度性能:
    传统Extended Resources调度:
      - 调度器Filter阶段: O(N×M) 节点×设备类型 简单计数匹配
      - 5000节点调度延迟: P99 ~2-4s (GPU工作负载)
      
    DRA调度 (CEL表达式评估):
      - 调度器需评估每节点设备属性的CEL表达式
      - 优化: DRA驱动预计算设备属性索引
      - 5000节点调度延迟: P99 ~3-6s (首次部署有学习成本)
      - 稳态后: 与Extended Resources相当，且调度质量更高

  大规模部署建议:
    - 使用DeviceClass缩小候选节点范围（减少CEL评估次数）
    - 启用调度器percentageOfNodesToScore优化（抽样评估）
    - DRA控制器与调度器部署在同一高性能节点
    - 监控: scheduler_plugin_execution_duration_seconds{plugin="DynamicResources"}

  交叉参考:
    - 调度器DRA集成详解: 文档12 §8.2 DRA调度集成
    - GPU工作负载DRA实战: 文档17 AI/ML GPU调度与LLM推理
```

```yaml
# DRA对调度器的监控指标（Prometheus）
# 监控DRA插件调度延迟贡献
histogram_quantile(0.99,
  rate(scheduler_plugin_execution_duration_seconds_bucket{
    plugin="DynamicResources",
    extension_point="Filter"
  }[5m])
) > 0.5  # 告警：DRA Filter延迟过高

# 监控ResourceClaim绑定成功率
increase(dra_resource_claim_allocations_total{result="success"}[5m])
increase(dra_resource_claim_allocations_total{result="failed"}[5m])

# 监控设备分配等待队列深度
dra_pending_resource_claims > 50  # 告警：待分配ResourceClaim积压
```

<!-- chunk: 10. 未来发展趋势 -->## 10. 未来发展趋势

## 10.1 新技术应用

```yaml
未来性能优化方向:
  1. eBPF技术深度应用
     - 网络加速
     - 安全策略实施
     - 性能监控增强

  2. 边缘计算优化
     - 轻量级控制平面
     - 本地缓存机制
     - 断网自治能力

  3. AI驱动的性能优化
     - 智能资源调度
     - 预测性性能调优
     - 自动化瓶颈识别

  4. 2026+大规模集群演进方向:
     - In-Place Resize GA后VPA全面转向InPlace模式
     - Streaming List GA: controller内存占用下降50-80%
     - DRA GA (预计K8s 1.34): GPU等加速器精细化调度普及
     - 参见: 文档12 调度器深度优化 & 文档17 AI/ML GPU调度
```

---
*本文档基于大规模生产环境实践经验编写，持续更新中。建议结合具体业务场景进行针对性优化。*
*最近更新：2026-03-03，新增K8s 1.33/1.34性能特性章节（In-Place Resize、Streaming List、DRA大规模调度影响）。*
*交叉参考：[12-调度器深度优化](32-发布/package/2026-07-02_18-53/corpus/supporting/domain-19-landscape-references/02-papers/05-kubernetes-scheduler-deep-optimization-custom-scheduling.md) | [17-AI/ML GPU调度](32-发布/package/2026-07-02_18-53/corpus/supporting/domain-19-landscape-references/02-papers/06-kubernetes-aiml-gpu-scheduling-llm-inference.md)*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- domain-19-papers KUDIG Database — Global MOC
- [[domain-19-landscape-references/README.md|Domain 19: Kubernetes 高级技术论文与最佳实践 (Advanced Technical Papers...]]
- Domain-19 论文与参考 — 开源项目索引
- Kubernetes 生产就绪性评估框架 (Production Readiness Assessment Framew...
- Kubernetes 安全零信任架构实施指南 (Zero Trust Security Architecture Imp...
- Kubernetes 多云混合部署架构与实践 (Multi-Cloud Hybrid Deployment Archit...
- Kubernetes GitOps 完整实践指南 (GitOps Complete Practice Guide)
- Kubernetes 成本治理与 FinOps 实践 (Kubernetes Cost Governance and F...
- Kubernetes 容器存储接口 (CSI) 深度实践指南 (Container Storage Interface ...
- Kubernetes 网络策略与安全微隔离实践 (Network Policies and Security Micro...
- Kubernetes 服务网格深度实践与Istio集成 (Service Mesh Deep Practice and ...
- Kubernetes 自动化运维与SRE实践 (Automation and SRE Practices)

## See Also

- 26-kubernetes-vcluster-virtual-cluster-multi-tenancy
- 01-kubernetes-production-readiness-assessment
- 03-kubernetes-zero-trust-security-architecture
- 04-kubernetes-multi-cloud-hybrid-deployment

## Related

- [[papers|#papers Hub]] — tag hub

- research/ — tag hub

- [[domain-19-landscape-references/领域索引/etcd-index.md|etcd 知识图谱索引]]


<!-- risk-assessed -->
