# OpenYurt

> **成熟度**: Incubating | **加入时间**: 2020-09 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://openyurt.io |
| **GitHub** | https://github.com/openyurtio/openyurt |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Edge Computing |

---

## 项目概述

OpenYurt 是阿里云开源的边缘计算平台，将原生 Kubernetes 能力无缝扩展到边缘场景。它解决了边缘网络不稳定、节点自治、多区域管理等边缘计算特有挑战。

## 核心特性

- **边缘自治**: 边缘节点与云端断连时仍可正常工作
- **单元化管理**: NodePool 实现边缘节点分组管理
- **流量闭环**: 确保应用流量在同一 NodePool 内闭环
- **云边协同**: 统一的云端控制面管理边缘节点
- **无侵入增强**: 无需修改 Kubernetes 核心组件
- **边缘设备管理**: 集成 EdgeX Foundry 管理 IoT 设备

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                     OpenYurt Architecture                        │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Cloud Control Plane                     │ │
│  │                                                            │ │
│  │  ┌──────────────┐  ┌───────────────┐  ┌────────────────┐ │ │
│  │  │  kube-api-   │  │   Yurt-App-   │  │  Yurt-Device-  │ │ │
│  │  │   server     │  │   Manager     │  │   Controller   │ │ │
│  │  └──────────────┘  └───────────────┘  └────────────────┘ │ │
│  │                                                            │ │
│  │  ┌──────────────┐  ┌───────────────┐  ┌────────────────┐ │ │
│  │  │  Yurt-       │  │  Yurt-        │  │   Pool-        │ │ │
│  │  │  Controller  │  │  Coordinator  │  │   Coordinator  │ │ │
│  │  │  Manager     │  │               │  │                │ │ │
│  │  └──────────────┘  └───────────────┘  └────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                       Cloud-Edge Tunnel                          │
│                              │                                   │
│  ┌──────────────────────────┬┴───────────────────────────────┐  │
│  │       NodePool A         │         NodePool B              │  │
│  │       (Factory)          │         (Retail)                │  │
│  │                          │                                 │  │
│  │  ┌─────────────────┐    │    ┌─────────────────────┐     │  │
│  │  │   Edge Node 1   │    │    │    Edge Node 3      │     │  │
│  │  │ ┌─────────────┐ │    │    │ ┌─────────────────┐ │     │  │
│  │  │ │ Yurt-Hub    │ │    │    │ │   Yurt-Hub      │ │     │  │
│  │  │ │ (Local API  │ │    │    │ │                 │ │     │  │
│  │  │ │  Cache)     │ │    │    │ └─────────────────┘ │     │  │
│  │  │ └─────────────┘ │    │    │                     │     │  │
│  │  │ ┌─────────────┐ │    │    │    ┌───────────┐   │     │  │
│  │  │ │   Pods      │ │    │    │    │   Pods    │   │     │  │
│  │  │ └─────────────┘ │    │    │    └───────────┘   │     │  │
│  │  └─────────────────┘    │    └─────────────────────┘     │  │
│  │                          │                                 │  │
│  │  ┌─────────────────┐    │    ┌─────────────────────┐     │  │
│  │  │   Edge Node 2   │    │    │    Edge Node 4      │     │  │
│  │  └─────────────────┘    │    └─────────────────────┘     │  │
│  └──────────────────────────┴───────────────────────────────┘  │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 位置 | 功能 |
|------|------|------|
| Yurt-Controller-Manager | 云端 | 管理边缘节点、NodePool、流量策略 |
| Yurt-App-Manager | 云端 | 边缘应用生命周期管理 |
| Yurt-Hub | 边缘 | 本地 API 缓存，实现边缘自治 |
| Yurt-Tunnel | 云边 | 云边安全隧道 |
| Pool-Coordinator | 边缘 | NodePool 内 leader 选举和协调 |

---

## 快速开始

### 安装 OpenYurt

```bash
# 使用 yurtadm 安装
curl -LO https://github.com/openyurtio/openyurt/releases/download/v1.4.0/yurtadm
chmod +x yurtadm

# 初始化控制面
yurtadm init --apiserver-advertise-address=<master-ip>

# 或 Helm 安装组件
helm repo add openyurt https://openyurtio.github.io/openyurt-helm
helm install yurt-manager openyurt/yurt-manager -n kube-system
```

### 转换现有节点

```bash
# 将普通节点转换为边缘节点
yurtadm join <master-ip>:6443 \
  --token <token> \
  --discovery-token-ca-cert-hash sha256:<hash> \
  --node-type=edge
```

---

## NodePool（节点池）

```yaml
apiVersion: apps.openyurt.io/v1beta1
kind: NodePool
metadata:
  name: beijing-factory
spec:
  type: Edge
  annotations:
    location: beijing
  labels:
    region: beijing
    type: factory
  taints:
    - key: apps.openyurt.io/nodepool
      value: beijing-factory
      effect: NoSchedule
```

### 将节点加入 NodePool

```bash
kubectl label node edge-node-1 apps.openyurt.io/nodepool=beijing-factory
```

---

## 边缘应用部署

### YurtAppSet（边缘应用集）

```yaml
apiVersion: apps.openyurt.io/v1alpha1
kind: YurtAppSet
metadata:
  name: edge-app
spec:
  selector:
    matchLabels:
      app: edge-app
  workloadTemplate:
    deploymentTemplate:
      metadata:
        labels:
          app: edge-app
      spec:
        replicas: 2
        selector:
          matchLabels:
            app: edge-app
        template:
          metadata:
            labels:
              app: edge-app
          spec:
            containers:
              - name: app
                image: nginx:latest
  topology:
    pools:
      - name: beijing-factory
        replicas: 3
      - name: shanghai-factory
        replicas: 2
```

### YurtAppDaemon（边缘守护应用）

```yaml
apiVersion: apps.openyurt.io/v1alpha1
kind: YurtAppDaemon
metadata:
  name: edge-agent
spec:
  selector:
    matchLabels:
      app: edge-agent
  nodepoolSelector:
    matchLabels:
      type: factory
  workloadTemplate:
    deploymentTemplate:
      metadata:
        labels:
          app: edge-agent
      spec:
        replicas: 1
        selector:
          matchLabels:
            app: edge-agent
        template:
          spec:
            containers:
              - name: agent
                image: edge-agent:latest
```

---

## 边缘自治

Yurt-Hub 在边缘节点本地缓存 API 数据，确保断网时应用正常运行。

```yaml
# 配置 Yurt-Hub 缓存策略
apiVersion: v1
kind: ConfigMap
metadata:
  name: yurt-hub-cfg
  namespace: kube-system
data:
  cache-agents: "kubelet,kube-proxy,flannel"
  filter-response-headers: "X-Request-Id"
```

---

## 服务拓扑（流量闭环）

```yaml
# 确保服务流量在 NodePool 内闭环
apiVersion: v1
kind: Service
metadata:
  name: my-service
  annotations:
    openyurt.io/topologyKeys: "openyurt.io/nodepool"
spec:
  selector:
    app: my-app
  ports:
    - port: 80
      targetPort: 8080
```

---

## 设备管理 (IoT)

```yaml
# 集成 EdgeX Foundry
apiVersion: iot.openyurt.io/v1alpha1
kind: DeviceProfile
metadata:
  name: temperature-sensor
spec:
  deviceResources:
    - name: Temperature
      properties:
        valueType: Float64
        readWrite: R
---
apiVersion: iot.openyurt.io/v1alpha1
kind: Device
metadata:
  name: sensor-001
spec:
  nodePool: beijing-factory
  deviceProfileRef:
    name: temperature-sensor
  protocols:
    modbus-tcp:
      Address: "192.168.1.100"
      Port: 502
```

---

## 云边隧道

```bash
# 从云端访问边缘节点
kubectl exec -it <cloud-pod> -- curl http://<edge-service>

# 隧道自动建立，无需额外配置
```

---

## 监控

```yaml
# 关键指标
- yurt_hub_in_flight_requests
- yurt_hub_cache_hit_count
- yurt_tunnel_connection_count
- node_pool_ready_nodes
```

---

## 最佳实践

1. **NodePool 规划**: 按地理位置、业务单元划分 NodePool
2. **本地缓存**: 配置合适的 Yurt-Hub 缓存策略
3. **流量闭环**: 为边缘服务配置拓扑感知
4. **弹性部署**: 使用 YurtAppSet 管理跨 NodePool 应用
5. **网络规划**: 确保云边隧道的网络可达性

---

## 参考资源

- [官方文档](https://openyurt.io/docs)
- [GitHub Repo](https://github.com/openyurtio/openyurt)
- [用户案例](https://openyurt.io/docs/user-cases/)
- [API 参考](https://openyurt.io/docs/reference/)

---

**维护者**: Kudig Team | **许可证**: MIT
