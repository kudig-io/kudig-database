# 26 - GPU 调度与管理 (GPU Scheduling & Management)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [NVIDIA Device Plugin](https://github.com/NVIDIA/k8s-device-plugin)

## GPU 虚拟化技术对比 (Virtualization)

| 技术 (Technology) | 隔离级别 (Isolation) | 粒度 (Granularity) | 适用场景 (Use Case) |
|-------------------|-------------------|-------------------|--------------------|
| **Passthrough** | 强 (硬件隔离) | 物理卡 (Whole) | 预训练, 密集计算 |
| **MIG** (A100+) | 强 (QoS 隔离) | 硬件切片 (1/7) | 多租户, 混合推理 |
| **Time-Slicing** | 弱 (仅显存上限) | 模拟多卡 | 开发调试, 小模型推理 |
| **vGPU** | 中 (软件隔离) | 显存/算力百分比 | 混部, 工作站 |
| **DRA** (v1.31+) | 灵活 (动态) | 资源声明式 | 下一代动态资源管理 |

## NVIDIA Device Plugin 共享配置
```yaml
config:
  version: v1
  sharing:
    timeSlicing:
      resources:
      - name: nvidia.com/gpu
        replicas: 10
```

## 运维诊断 (Diagnosis)
- **显存碎片**: 关注 `nvidia-smi` 中的 `Memory Usage` 与 `Processes`。
- **XID 故障**: `dmesg | grep -i xid` 是排查 GPU 硬件/驱动崩溃的核心。


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)