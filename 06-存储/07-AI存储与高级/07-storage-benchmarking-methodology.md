---
title: "存储性能基准测试方法论"
description: "K8s 环境存储基准测试工具链、AI 训练 I/O 建模与测试报告规范"
summary: "覆盖 fio/IOR/mdtest/dlio 工具使用、AI 训练 I/O 模式建模、K8s 基准测试 Job 设计、指标解读与云磁盘/本地 NVMe/分布式存储对比"
category: 存储
tags:
- storage
- benchmark
- fio
- performance
- ai
tier: core
created: '2026-07-19'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 平台工程师
- AI 工程师
estimated_read_time: 20min
intent_queries:
- "如何对 K8s 存储进行性能基准测试"
- "AI 训练的 I/O 模式如何建模和测试"
- "fio 测试结果如何解读"
trigger_keywords:
- 基准测试
- benchmark
- fio
- IOR
- IOPS
- 吞吐量
- 延迟
prerequisites:
- kubectl-basics
- storage-basics
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

# 存储性能基准测试方法论

## 概述

存储性能基准测试是容量规划、存储选型和性能问题诊断的基础。在 AI 训练场景中，存储性能直接决定 GPU 利用率——如果数据加载速度跟不上计算速度，昂贵的 GPU 将处于空闲等待状态。然而，不正确的基准测试方法会得出误导性结论，导致错误的存储投资决策。

本文建立一套系统化的存储基准测试方法论，覆盖工具选择、AI 训练 I/O 模式建模、Kubernetes 环境中的测试执行、指标解读和报告规范，确保测试结果能够真实反映生产工作负载的存储需求。

## 架构与核心概念

### 基准测试工具矩阵

| 工具 | 测试类型 | 适用场景 | 并行能力 | AI 相关度 |
|------|---------|---------|---------|----------|
| fio | 块/文件 I/O | 通用存储性能 | 多线程/多进程 | ⭐⭐⭐⭐ |
| IOR | 并行文件 I/O | HPC/并行文件系统 | MPI 多进程 | ⭐⭐⭐⭐⭐ |
| mdtest | 元数据操作 | 小文件/目录操作 | MPI 多进程 | ⭐⭐⭐⭐ |
| DLIO | AI 训练 I/O | 模拟真实训练 I/O | 多进程 | ⭐⭐⭐⭐⭐ |
| sysbench | 数据库 I/O | OLTP 场景 | 多线程 | ⭐⭐ |
| dd | 顺序 I/O | 快速粗测 | 单进程 | ⭐ |

### AI 训练 I/O 模式建模

AI 训练的 I/O 模式与传统应用有本质区别：

```
AI Training I/O Patterns:

1. 数据加载 (Data Loading)
   - 模式：大量小文件随机读 或 大文件顺序读
   - 特征：高并发（num_workers × num_gpus）
   - 典型：ImageNet (14M 小文件) / LLM corpus (大 parquet)

2. Checkpoint 写入
   - 模式：周期性大块顺序写
   - 特征：突发高带宽（数十 GB 在秒级写入）
   - 典型：每 N 个 epoch 写入模型参数

3. 日志/指标写入
   - 模式：持续小量追加写
   - 特征：低带宽但高频
   - 典型：TensorBoard events, wandb logs

4. 模型加载 (Inference)
   - 模式：一次性大块顺序读
   - 特征：冷启动延迟敏感
   - 典型：加载 ONNX/TensorRT 模型文件
```

### 关键性能指标

- **IOPS**：每秒 I/O 操作数（小文件/随机 I/O 关键）
- **Throughput**：吞吐量 MB/s（大文件/顺序 I/O 关键）
- **Latency Percentile**：延迟分布（p50/p99/p999）
- **Metadata Ops/s**：元数据操作速率（open/stat/readdir）
- **GPU Utilization Impact**：存储瓶颈导致的 GPU 空闲比例

## 生产部署

### K8s fio 基准测试 Job

🟡 中风险：基准测试会消耗大量 I/O 资源，影响同节点其他工作负载

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: storage-bench-fio
  namespace: storage-benchmark
spec:
  backoffLimit: 0
  template:
    metadata:
      labels:
        benchmark: fio
    spec:
      restartPolicy: Never
      nodeSelector:
        benchmark-node: "true"  # 专用测试节点
      containers:
        - name: fio
          image: ljishen/fio:latest
          command:
            - /bin/sh
            - -c
            - |
              echo "=== Sequential Read (AI Data Loading) ==="
              fio --name=seq-read \
                --directory=/bench \
                --rw=read \
                --bs=1m \
                --size=10G \
                --numjobs=8 \
                --iodepth=32 \
                --direct=1 \
                --runtime=60 \
                --time_based \
                --group_reporting \
                --output-format=json \
                --output=/results/seq-read.json

              echo "=== Random Read (Small File Dataset) ==="
              fio --name=rand-read \
                --directory=/bench \
                --rw=randread \
                --bs=4k \
                --size=10G \
                --numjobs=16 \
                --iodepth=64 \
                --direct=1 \
                --runtime=60 \
                --time_based \
                --group_reporting \
                --output-format=json \
                --output=/results/rand-read.json

              echo "=== Sequential Write (Checkpoint) ==="
              fio --name=seq-write \
                --directory=/bench \
                --rw=write \
                --bs=4m \
                --size=10G \
                --numjobs=4 \
                --iodepth=16 \
                --direct=1 \
                --runtime=60 \
                --time_based \
                --group_reporting \
                --output-format=json \
                --output=/results/seq-write.json

              echo "=== Mixed Read/Write (Training + Logging) ==="
              fio --name=mixed-rw \
                --directory=/bench \
                --rw=randrw \
                --rwmixread=80 \
                --bs=64k \
                --size=10G \
                --numjobs=8 \
                --iodepth=32 \
                --direct=1 \
                --runtime=60 \
                --time_based \
                --group_reporting \
                --output-format=json \
                --output=/results/mixed-rw.json
          resources:
            requests:
              cpu: "8"
              memory: 16Gi
          volumeMounts:
            - name: bench-volume
              mountPath: /bench
            - name: results
              mountPath: /results
      volumes:
        - name: bench-volume
          persistentVolumeClaim:
            claimName: bench-pvc
        - name: results
          emptyDir: {}
---
apiVersion: v1
kind: PersistentVolumeClaim
metadata:
  name: bench-pvc
  namespace: storage-benchmark
spec:
  accessModes: ["ReadWriteOnce"]
  storageClassName: <target-storage-class>
  resources:
    requests:
      storage: 100Gi
```

### IOR 并行文件系统测试

🟡 中风险：IOR 测试需要多节点协调，消耗集群网络和存储资源

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: storage-bench-ior
  namespace: storage-benchmark
spec:
  parallelism: 4
  completions: 4
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: ior
          image: ai-platform/ior-benchmark:3.3
          command:
            - mpirun
            - --allow-run-as-root
            - -np
            - "4"
            - -hostfile
            - /etc/mpi/hostfile
            - ior
            - -F  # 每进程独立文件
            - -w  # 写入测试
            - -r  # 读取测试
            - -t 1m  # 传输大小
            - -b 1g  # 块大小
            - -o /shared-bench/ior-testfile
            - -C  # 每进程创建文件
            - -e  # fsync
          volumeMounts:
            - name: shared-storage
              mountPath: /shared-bench
      volumes:
        - name: shared-storage
          persistentVolumeClaim:
            claimName: parallel-fs-pvc  # WekaFS/Lustre/BeeGFS PVC
```

### DLIO AI 训练 I/O 模拟

🟡 中风险：模拟真实训练 I/O 负载

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: dlio-benchmark
  namespace: storage-benchmark
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: dlio
          image: ai-platform/dlio-benchmark:latest
          command:
            - python
            - -m
            - dlio_benchmark.main
            - --dataset.num_files_train=10000
            - --dataset.record_length=1048576
            - --dataset.record_length_std=0
            - --reader.data_loader=pytorch
            - --reader.batch_size=256
            - --reader.read_threads=8
            - --reader.dont_shuffle=false
            - --storage.storage_root=/data
            - --storage.storage_type=local_fs
            - --framework=pytorch
          volumeMounts:
            - name: training-data
              mountPath: /data
      volumes:
        - name: training-data
          persistentVolumeClaim:
            claimName: bench-pvc
```

## 运维操作

### 测试结果解读

🟢 低风险/只读：分析 fio 输出

```bash
# 解析 fio JSON 输出
kubectl logs job/storage-bench-fio -n storage-benchmark | \
  jq '{
    test: .jobs[0].jobname,
    read_iops: .jobs[0].read.iops,
    read_bw_MBps: (.jobs[0].read.bw / 1024),
    read_lat_p99_us: .jobs[0].read.clat_ns.percentile["99.000000"] / 1000,
    write_iops: .jobs[0].write.iops,
    write_bw_MBps: (.jobs[0].write.bw / 1024),
    write_lat_p99_us: .jobs[0].write.clat_ns.percentile["99.000000"] / 1000
  }'
```

### 存储类型基准对比参考

| 存储类型 | 顺序读 (MB/s) | 随机读 IOPS (4K) | p99 延迟 | 适用 AI 场景 |
|---------|-------------|-----------------|---------|-------------|
| 本地 NVMe (单盘) | 3,000-7,000 | 500K-1M | < 100μs | Checkpoint 缓冲 |
| AWS EBS io2 | 1,000-4,000 | 64K-256K | 1-5ms | 通用训练数据 |
| AWS EBS gp3 | 125-1,000 | 3K-16K | 1-5ms | 开发/测试 |
| WekaFS (4节点) | 20,000-100,000 | 1M+ | < 1ms | 大规模训练 |
| Lustre (8 OSS) | 50,000-200,000 | 500K+ | < 2ms | HPC/LLM 训练 |
| MinIO (4节点) | 5,000-20,000 | N/A (对象) | 5-20ms | 数据集存储 |
| NFS (企业级) | 1,000-10,000 | 10K-50K | 1-10ms | 共享配置 |

### 测试报告模板

```markdown
# 存储基准测试报告

## 测试环境
- 集群版本: K8s 1.31
- 存储类型: [StorageClass 名称]
- 节点配置: [CPU/内存/磁盘/网络]
- 测试日期: YYYY-MM-DD

## 测试配置
- 工具: fio 3.35 / IOR 3.3 / DLIO 1.0
- 测试文件: [大小/数量]
- 并发度: [numjobs/进程数]
- 持续时间: [秒]

## 测试结果
| 测试项 | IOPS | 吞吐 (MB/s) | p50 延迟 | p99 延迟 |
|--------|------|------------|---------|---------|
| 顺序读 1M | - | xxx | - | xxx |
| 随机读 4K | xxx | - | xxx | xxx |
| 顺序写 4M | - | xxx | - | xxx |
| 混合读写 | xxx | xxx | xxx | xxx |

## AI 训练适配评估
- 数据加载瓶颈: [是/否]
- Checkpoint 写入时间: [秒/GB]
- GPU 利用率影响: [百分比]

## 结论与建议
[基于测试结果的存储选型/调优建议]
```

## 故障排查

### 基准测试结果异常

🟢 低风险/只读：诊断测试环境问题

```bash
# 检查是否有其他 I/O 负载干扰
kubectl top nodes
iostat -x 1 5  # 在测试节点上执行

# 检查 I/O 调度器设置
cat /sys/block/nvme0n1/queue/scheduler

# 检查是否有 I/O 限流（cgroup）
cat /sys/fs/cgroup/io.max

# 检查文件系统缓存影响（是否需要 drop cache）
# 🟡 中风险：清除缓存影响其他进程
sync && echo 3 > /proc/sys/vm/drop_caches

# 检查网络存储延迟（NFS/分布式文件系统）
ping -c 10 <storage-server-ip>
nfsstat -c  # NFS 客户端统计
```

### 常见测试误区

| 误区 | 正确做法 | 影响 |
|------|---------|------|
| 测试文件小于内存 | 使用大于 RAM 的测试文件 | 结果反映缓存而非磁盘 |
| 未使用 direct I/O | 添加 `--direct=1` | 绕过 page cache |
| 单线程测试 | 模拟实际并发度 | 低估存储能力 |
| 只测顺序 I/O | 覆盖随机/混合模式 | 遗漏元数据瓶颈 |
| 测试时间太短 | 至少 60s，推荐 300s | 未达稳态 |
| 忽略延迟分布 | 关注 p99/p999 | 平均值掩盖尾部延迟 |

## 最佳实践

1. **模拟真实负载**：使用 DLIO 或从生产训练任务提取 I/O trace，而非仅用 fio 默认配置
2. **隔离测试环境**：基准测试在专用节点执行，避免其他工作负载干扰，参考 [[06-存储/07-AI存储与高级/08-storage-multitenant-isolation.md|存储多租户隔离]]
3. **多维度测试**：覆盖顺序/随机、读/写/混合、不同块大小、不同并发度
4. **关注延迟尾部**：AI 训练对 p99 延迟敏感，单个慢 I/O 可能阻塞整个 batch
5. **对比测试**：同一测试配置在不同存储类型上执行，确保可比性
6. **预热与稳态**：SSD 需要预写入达到稳态后再测试，避免新盘虚高结果
7. **记录环境**：完整记录内核版本、文件系统参数、驱动版本，确保可复现
8. **定期回归**：存储变更后重新执行基准测试，建立性能基线，参考 [[06-存储/01-K8s存储/08-storage-performance-tuning.md|存储性能调优]]
9. **GPU 联动**：最终验证需在实际训练任务中观察 GPU utilization，存储基准只是参考

## Related

- [[06-存储/07-AI存储与高级/02-high-perf-ai-storage-weka-lustre.md|AI 高性能存储]]
- [[06-存储/07-AI存储与高级/06-filesystem-comparison-ext4-xfs-zfs.md|文件系统对比]]
- [[06-存储/01-K8s存储/08-storage-performance-tuning.md|存储性能调优]]
- [[06-存储/02-存储基础/06-storage-performance-iops.md|存储性能与 IOPS]]
- [[15-AI基础设施/01-基础设施/06-ai-data-pipeline.md|AI 数据管线]]
