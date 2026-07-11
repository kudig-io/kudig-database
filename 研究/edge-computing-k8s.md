---
title: 边缘计算与 K8s 轻量化运行时研究
summary: 深入研究 K8s 在边缘计算场景的适配方案，覆盖 K3s、MicroK8s、KubeEdge、OpenYurt 等轻量化运行时的架构和选型。
category: research
tags:
- research
- edge-computing
- k3s
- kubeedge
- openyurt
- lightweight
tier: supporting
created: '2026-07-11'
updated: '2026-07-11'
last_updated: '2026-07-11'
status: done
---

# 边缘计算与 K8s 轻量化运行时研究

## 研究背景

Kubernetes 正在从云端向边缘延伸，IoT、5G MEC、零售门店、工厂车间等边缘场景需要 K8s 的编排能力，但面临严格约束：

- **资源受限**：边缘设备通常仅 1-4GB 内存、ARM/x86 低功耗 CPU
- **网络不稳定**：边缘到云的网络可能频繁断连、高延迟
- **大规模节点**：数千到数万个边缘节点需要统一管理
- **离线运行**：网络断连时边缘必须继续提供服务
- **远程运维**：物理访问成本极高，需要远程管理和自动恢复

## 核心问题

1. K3s、MicroK8s、K0s 在资源占用和功能裁剪上的具体差异？
2. KubeEdge 和 OpenYurt 如何解决边缘离线运行和云边协同问题？
3. 边缘场景的应用分发、配置管理和安全策略如何设计？
4. 大规模边缘节点的监控和运维体系如何构建？

## 调研发现

### 发现一：轻量 K8s 发行版对比

| 维度 | K3s | K0s | MicroK8s | k3d |
|------|-----|-----|----------|-----|
| **开发者** | Rancher/SUSE | Mirantis | Canonical | Rancher/Docker |
| **二进制大小** | ~60MB | ~150MB | ~300MB | ~60MB(K3s+Docker) |
| **最低内存** | 512MB | 1GB | 2GB | 512MB |
| **etcd** | ❌（SQLite/etcd 可选） | ✅（默认） | ✅ | ❌（SQLite） |
| **CNI** | Flannel（内置） | Calico/Kube-router | Calico | Flannel |
| **Ingress** | Traefik（内置） | 需手动安装 | 需手动安装 | Traefik |
| **ARM 支持** | ✅ 原生 | ✅ | ✅ | ✅ |
| **单二进制** | ✅ | ✅ | ❌（Snap） | ✅ |
| **推荐场景** | 边缘/IoT/CI | 生产轻量集群 | 开发/测试 | Docker 内 K8s |

### 发现二：边缘协同方案对比

| 维度 | KubeEdge | OpenYurt | AWS IoT Greengrass |
|------|----------|----------|-------------------|
| **设计哲学** | 边缘自治+云端管控 | 无侵入 K8s 扩展 | AWS IoT 生态集成 |
| **边缘自治** | ✅（断网后继续运行） | ✅（节点池自治） | ✅ |
| **云边通信** | WebSocket + QUIC | 反向隧道 | MQTT |
| **设备管理** | ✅（MQTT 设备孪生） | ⚠️ | ✅（IoT 设备） |
| **K8s 兼容性** | 定制 Kubelet | 100% 兼容 | 非 K8s 原生 |
| **多租户** | ⚠️ | ✅ | ✅ |
| **推荐场景** | IoT 设备密集 | K8s 原生边缘 | AWS 用户 |

### 发现三：边缘架构参考

```
┌──────────────────────────────┐
│  云端控制面                    │
│  → K8s 控制集群 (K3s/标准K8s)  │
│  → 边缘节点注册中心             │
│  → 镜像仓库（边缘 CDN 分发）    │
│  → 统一监控（遥测聚合）         │
└──────────┬───────────────────┘
           │ WebSocket/MQTT（断连容忍）
           ↓
    ┌──────┴──────┬──────────┬──────────┐
    ↓             ↓          ↓          ↓
┌───────┐  ┌───────┐  ┌───────┐  ┌───────┐
│ 边缘1  │  │ 边缘2  │  │ 边缘3  │  │ 边缘N  │
│ K3s    │  │ K3s    │  │ K3s    │  │ K3s    │
│ IoT 设备│  │ 摄像头 │  │ PLC    │  │ 传感器 │
└───────┘  └───────┘  └───────┘  └───────┘
```

### 发现四：边缘应用分发策略

```yaml
# 使用 K3s + GitOps 边缘部署
apiVersion: apps.cattle.io/v1
kind: GitRepo          # Fleet: Rancher 边缘 GitOps
metadata:
  name: edge-app-bundle
spec:
  repo: https://github.com/org/edge-apps
  branch: main
  paths:
  - apps/sensor-gateway
  targets:
  - clusterSelector:
      matchLabels:
        edge-site: "true"      # 只部署到边缘节点
  - clusterGroup: factories     # 按组分发到工厂集群
```

## 结论与建议

1. **K3s 是边缘 K8s 的事实标准**：最小资源占用、ARM 原生支持、单二进制部署。
2. **KubeEdge 适合 IoT 设备密集场景**：MQTT 设备管理是独特优势。
3. **OpenYurt 适合已有 K8s 集群扩展边缘**：100% K8s 兼容，无侵入式扩展。
4. **边缘自治（离线运行）是刚需**：云边网络断连时边缘必须继续工作。
5. **镜像分发需要边缘 CDN**：数千边缘节点同时拉取镜像需要 P2P 镜像分发（Dragonfly/Kraken）。

## 参考资料

- K3s: https://k3s.io/
- KubeEdge: https://kubeedge.io/
- OpenYurt: https://openyurt.io/
- [[专项技术/index.md|专项技术目录]]
- [[集群基础/index.md|集群基础目录]]

## Related

- [[研究/multi-cluster-management.md|多集群管理]]
- [[容器运行时/index.md|容器运行时目录]]
