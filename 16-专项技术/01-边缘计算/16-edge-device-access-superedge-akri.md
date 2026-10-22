---
title: 边缘设备接入与轻量边缘管理（SuperEdge + Akri）
description: 用 Akri 把 Modbus/ONVIF/OPC-UA 协议设备接入 K8s，用 SuperEdge 做云边隧道增强与边缘自治的完整实践，含四方案对比矩阵与工业产线数采案例
summary: SuperEdge + Akri 深度实践 — 边缘设备接入问题域（工业协议 vs K8s API）、Akri 架构（Agent 发现→Configuration/Instance CRD→broker Pod 与 EndpointSlice 自动生成）、ONVIF 摄像头接入实操、SuperEdge 云边隧道/边缘自治/edge-health 广播、与 KubeEdge/OpenYurt/K3s 对比矩阵、弱网自治机制、工业产线 Modbus→Akri→MQTT 生产案例与故障排查
category: edge-computing
tags:
- k8s
- edge
- akri
- superedge
- kubeedge
- openyurt
- onvif
- modbus
tier: supporting
created: '2026-10-15'
last_updated: '2026-10-15'
difficulty: advanced
audience:
- 边缘计算工程师
- SRE
- IoT 工程师
estimated_read_time: 18min
intent_queries:
- Akri 是什么，如何把 ONVIF 摄像头接入 Kubernetes
- SuperEdge 和 KubeEdge OpenYurt K3s 怎么选
- 如何用 Akri 接 Modbus 设备并经 MQTT 上云
- 边缘节点 NotReady 但业务正常是怎么回事
- 边缘节点弱网自治如何避免 Pod 被驱逐
trigger_keywords:
- Akri
- SuperEdge
- 设备接入
- ONVIF
- Modbus
- 边缘自治
- 云边隧道
prerequisites:
- kubectl-basics
- helm-basics
- edge-basics
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
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 边缘设备接入与轻量边缘管理（SuperEdge + Akri）

> **适用版本**: Akri v0.14.0（project-akri/akri，2026-10 核对）；SuperEdge 以官方仓库 main 分支为准
> **前置阅读**: [[16-专项技术/01-边缘计算/01-edge-computing-architecture.md|边缘计算架构]] / [[16-专项技术/01-边缘计算/07-superedge-architecture.md|SuperEdge 架构实践]] **难度**: 高级

---

## 📋 目录

- [一、概述：边缘设备接入的问题域](#一概述边缘设备接入的问题域)
- [二、Akri 架构深度解析](#二akri-架构深度解析)
- [三、Akri 实操：ONVIF 摄像头接入](#三akri-实操onvif-摄像头接入)
- [四、SuperEdge 定位与四方案对比](#四superedge-定位与四方案对比)
- [五、边缘节点弱网自治](#五边缘节点弱网自治)
- [六、生产案例：工业产线 Modbus 数采上云](#六生产案例工业产线-modbus-数采上云)
- [七、故障排查与诊断流程](#七故障排查与诊断流程)
- [八、最佳实践与反模式](#八最佳实践与反模式)
- [九、版本兼容性与升级注意](#九版本兼容性与升级注意)

---

## 一、概述：边缘设备接入的问题域

### 1.1 协议世界的"两个宇宙"

边缘现场的真实设备说的是**工业/物联协议**，而 Kubernetes 的世界只有 **REST/gRPC 的 API 对象**：

| 现实侧 | K8s 侧 |
|--------|--------|
| Modbus TCP/RTU（寄存器读写、轮询） | Pod/Deployment（无设备概念） |
| ONVIF（摄像头发现、RTSP 取流、PTZ 控制） | Service/EndpointSlice（IP+端口） |
| OPC-UA（地址空间、订阅、安全策略） | CRD（需自定义 Device 模型） |
| 设备动态上下线、IP 由 DHCP 分配 | 调度器只认节点，不认设备 |

矛盾的三个具体表现：

1. **设备不是节点**：摄像头不可能跑 kubelet，但消费它的 Pod 必须被调度到"连得上它"的节点；**设备会移动**（DHCP 续租、无线漫游让 IP 变化），手工维护 Endpoint 不可持续。
2. **设备容量有限**：一条 Modbus 总线同时只能有一个主站轮询，"一个设备能被几个 Pod 共享"必须显式建模（capacity）。

### 1.2 两条解决路线

| 路线 | 代表 | 思路 |
|------|------|------|
| 设备管理走 CRD + 边缘侧 Mapper/Agent | KubeEdge（DeviceModel/DeviceInstance + Mapper） | 云端声明设备期望状态，边缘执行，见 [[16-专项技术/01-边缘计算/05-kubeedge-device-edge-apps.md|KubeEdge 设备与应用]] |
| 设备即资源，按需生成消费 Pod | Akri | "发现设备 → 生成 broker Pod + Service/EndpointSlice"，复用 K8s 原生调度 |

本文主线是第二条（Akri）负责设备接入，叠加 SuperEdge 解决"边缘节点自身在弱网下的管理"问题。整体架构背景参见 [[16-专项技术/01-边缘计算/02-cloud-edge-collaboration.md|云边协同]]。

---

## 二、Akri 架构深度解析

### 2.1 组件与数据流

Akri（CNCF Sandbox）由两个核心组件构成，均以标准工作负载运行在（云或边缘的）K8s 上：

```text
┌─ 每节点 ────────────────────────────────────────────┐
│ akri-agent (DaemonSet)                               │
│  ├─ 调用 Discovery Handler（udev/ONVIF/OPC-UA…）     │
│  └─ 发现/失联设备 → 创建/删除 Akri Instance CR       │
└──────────────────────────────────────────────────────┘
┌─ 集群级 ────────────────────────────────────────────┐
│ akri-controller (Deployment)                         │
│  ├─ watch Instance → 生成 broker Pod（受设备可达性    │
│  │   约束的调度）                                     │
│  ├─ 直接创建/维护 EndpointSlice（不经 endpoints      │
│  │   controller 从 Pod readiness 派生）              │
│  └─ 生成两级 Service：每设备一个 + 每配置一个        │
└──────────────────────────────────────────────────────┘
```

关键对象：

| CRD/资源 | 作用 | 生命周期归属 |
|-----------|------|--------------|
| `Configuration`（akri.sh/v0） | 声明"如何发现设备 + 用什么 broker 消费 + capacity" | 用户创建 |
| `Instance` | 一个已被发现的设备实例（含设备连接信息） | Agent 自动创建/删除 |
| broker Pod | 真正连设备的适配器（视频流网关、Modbus 采集器…） | Controller 创建 |
| Service + EndpointSlice | 设备的服务化入口，端点只指向可达节点上的 broker | Controller 维护 |

**"绕过 API server"的两层含义**：其一，Controller 直接写 EndpointSlice，端点列表反映**设备可达性**而非 Pod readiness——broker 刚启动但设备已失联时端点即刻摘除，不等探针；其二，业务 Pod 访问设备流量走 Service → kube-proxy → 节点本地 broker → 设备，数据面不经过 API server，云边断连时同节点访问不受影响。

### 2.2 capacity：设备共享语义

`Configuration.spec.capacity` 决定一个设备 Instance 最多被几个节点同时消费：

```yaml
capacity: 1   # 独占：Modbus 主站轮询、串口设备必须为 1
capacity: 3   # 多个节点各起一个 broker 连同一台 ONVIF 摄像头（RTSP 多消费者）
```

Controller 把 capacity 编码进 Instance，超出容量的节点不会被分配 broker——这是 Akri 相比"手写 DaemonSet + hostNetwork"最本质的增益：**设备成了一等可调度资源**。

**发现协议扩展性**：内置 Discovery Handler 覆盖 `udev`（本地硬件）、`onvif`、`opcua` 及 `opcua-monitoring`；其他协议（如 Modbus）通过**自定义 Discovery Handler**（独立 gRPC 服务，Agent 动态发现并调用）扩展——第六章的工业案例演示这条路。

---

## 三、Akri 实操：ONVIF 摄像头接入

### 3.1 安装 Akri 并启用 ONVIF 发现

```bash
# 🟢 低风险：添加仓库
helm repo add akri https://project-akri.github.io/akri/
helm repo update

# 🟡 中风险：安装 Akri，只启用 ONVIF 发现（不启用其默认样例配置）
helm install akri akri/akri --version 0.14.0 \
  --namespace akri-system --create-namespace \
  --set onvif.discovery.enabled=true \
  --set onvif.configuration.enabled=false

# 🟢 低风险：确认 agent/controller 就绪
kubectl -n akri-system get ds,deploy
```

### 3.2 ONVIF Configuration（完整可拷贝）

```yaml
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: onvif-camera
spec:
  # 发现过滤：只接入支持 Profile Streaming 的摄像头
  protocol:
    onvif:
      discoveryUri: /onvif/device_service   # WS-Discovery 端点路径
      securityLevel: 0                      # 0=none 1=TLS，按摄像头能力调整
      scopes:
      - "onvif://www.onvif.org/Profile/Streaming"
  capacity: 2                # 每台摄像头允许 2 个节点各自起 broker
  # 设备上下线时的 Pod 处理策略
  brokerPodSpec:
    terminationGracePeriodSeconds: 10
    tolerations:
    - key: "node-type"
      operator: "Equal"
      value: "edge"
      effect: "NoSchedule"   # 允许 broker 落到打了污点的边缘节点
    containers:
    - name: onvif-broker
      image: ghcr.io/project-akri/akri/onvif-video-broker:v0.14.0  # demo broker，tag 以仓库 release 为准
      resources:
        requests:
          memory: "64Mi"
          cpu: "50m"
        limits:
          memory: "256Mi"
      env:
      - name: RTSP_URL        # broker 向消费者暴露 RTSP 流，变量以所选镜像文档为准
        value: "auto"
  # 每个设备 Instance 自动创建的 Service 模板
  instanceServiceSpec:
    type: ClusterIP
    ports:
    - name: grpc
      port: 8083
      targetPort: 8083
  # 同一 Configuration 全部设备聚合的 Service 模板
  configurationServiceSpec:
    type: ClusterIP
    ports:
    - name: grpc
      port: 8083
      targetPort: 8083
```

### 3.3 验证发现链路与消费

```bash
# 🟢 低风险：查看发现的设备 Instance（每个摄像头一个）
kubectl get akrii -o wide
# NAME                    CONFIGURATION   NODES         AGE
# onvif-camera-<hash>     onvif-camera    [edge-node-1] 5m

# 🟢 低风险：检查自动生成的 broker Pod、两级 Service 与 EndpointSlice
kubectl get pods -o wide | grep onvif-camera
kubectl get svc,endpointslice | grep onvif
```

拔掉摄像头网线，观察 `akrii` 在下一次发现周期消失、broker Pod 被回收、EndpointSlice 端点摘除——整条链路无需人工介入。

---

## 四、SuperEdge 定位与四方案对比

### 4.1 SuperEdge 是什么

SuperEdge 是腾讯开源的 Kubernetes 原生边缘托管框架（CNCF Sandbox），定位是**给标准 K8s 补上边缘能力**，而不是fork一套新 API。核心组件：

| 组件 | 部署位置 | 职责 |
|------|----------|------|
| tunnel-cloud / tunnel-edge | 云 / 边 | 云边反向隧道（边缘主动 outbound 长连接），云上可 `kubectl exec/logs` 到无公网入站能力的边缘节点 |
| lite-apiserver | 边 | 边缘本地 API 代理 + 元数据缓存；断网时 kubelet 从本地缓存继续工作（边缘自治的基石） |
| edge-health | 边 | 节点间健康探测广播：边缘节点互相探测并广播结果，避免云上失联误判节点死亡 |
| edge-health-admission | 云 | 准入控制器，依据广播结果改写调度决策（失联但同侪确认存活的节点不参与误驱逐） |
| application-grid-controller / wrapper | 云 / 边 | Service 拓扑感知：边缘节点上的 Service 流量优先终结在同侧边缘，避免绕云一圈 |

组件内部实现细节见 [[16-专项技术/01-边缘计算/07-superedge-architecture.md|SuperEdge 架构实践]]，本章关注它与同类方案的选型差异。

### 4.2 四方案对比矩阵

| 维度 | SuperEdge | KubeEdge | OpenYurt | K3s |
|------|-----------|----------|----------|-----|
| 出身/基金会 | 腾讯开源，CNCF Sandbox | 华为开源，CNCF Incubating | 阿里开源，CNCF Sandbox | Rancher/SUSE |
| 架构组件 | tunnel + lite-apiserver + edge-health + application-grid | 云端 cloudcore（CloudHub/EdgeHub 通道）+ 边缘 edgecore（edged/metamanager/eventbus） | YurtHub（节点侧缓存代理）+ YurtTunnel + YurtCoordinator + NodePool | 单二进制发行版（内嵌 SQLite/etcd、containerd、flannel） |
| 边缘自治 | lite-apiserver 本地缓存，断网继续 | metamanager 本地元数据 + edged 独立运行 | YurtHub 缓存 + 节点池自治 | 弱（单集群完整控制面通常在云端或本地小机，断云即失控） |
| 设备协议支持 | 无内置（通常搭配 Akri） | 内置 Device CRD + Mapper（Modbus/OPC-UA/Bluetooth/MQTT 等） | 无内置（通常搭配 Akri） | 无（普通 K8s） |
| 云边通道 | 反向隧道，云端可运维边缘 | CloudHub-EdgeHub（WebSocket/quic） | YurtTunnel | 无（原生 K8s 网络） |
| Service 边缘感知 | application-grid（服务分组就近访问） | EdgeMesh / 华为 CIE 方案 | Raven（跨节点池组网）+ 节点池内原生 Service | 无 |
| 对标准 K8s 侵入 | 低（复用 kubelet） | 中（边缘用 edgecore 替代 kubelet，版本有绑定） | 低（复用 kubelet） | 无（就是个 K8s） |
| 适用规模 | 大规模节点分组运维 | 设备管理诉求重的 IoT 场景 | 云边同构、阿里云生态 | 单站点轻量集群/边缘单机 |
| 典型组合 | SuperEdge + Akri | KubeEdge 原生 Mapper | OpenYurt + Akri | K3s + Akri（单站点） |

**选型速记**：要"标准 K8s + 边缘自治 + 云边隧道"选 SuperEdge；要"设备数字孪生/云端控制设备"选 KubeEdge；已有 ACK/阿里云栈选 OpenYurt；边缘站点本身是一台能自持的小服务器，选 K3s。OpenYurt 细节见 [[16-专项技术/01-边缘计算/06-openyurt-architecture.md|OpenYurt 架构]]，KubeEdge 见 [[16-专项技术/01-边缘计算/04-kubeedge-architecture-deployment.md|KubeEdge 架构与部署]]。

---

## 五、边缘节点弱网自治

### 5.1 断网时的三层保护

弱网自治要同时回答三个问题：**元数据从哪来（缓存）、节点会不会被误杀（健康广播）、Pod 会不会被驱逐（驱逐保护）**。

1. **本地缓存**：SuperEdge lite-apiserver / KubeEdge metamanager / OpenYurt YurtHub 都在边缘保留全量所需元数据（Pod/ConfigMap/Secret），kubelet 断云后从本地读，业务继续跑。
2. **健康广播**：云边断连时，默认 K8s 会因 Node Lease 失败把节点标 NotReady 并打 `node.kubernetes.io/unreachable` 污点。edge-health 让边缘节点互探并对云广播真实状态，配合 edge-health-admission 阻止"整片边缘被误判死亡"引发的批量重调度。
3. **Pod 驱逐保护**：kubelet 不会因 API server 失联驱逐存量 Pod（驱逐只由资源压力触发）；真正风险是 `NoExecute` 污点 + 默认 tolerationSeconds（300s）到期，边缘集群应显式加长：

```yaml
# 🟡 中风险：给边缘工作负载加长不可达污点容忍（防弱网驱逐）
spec:
  tolerations:
  - key: "node.kubernetes.io/unreachable"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 86400      # 弱网场景按最长预期断连时长设置
  - key: "node.kubernetes.io/not-ready"
    operator: "Exists"
    effect: "NoExecute"
    tolerationSeconds: 86400
```

### 5.2 断网恢复的一致性风险

恢复连接后，边缘侧本地状态与云端期望可能分叉（断网期间云端发生过 rollout）。三种方案的收敛策略不同：SuperEdge/OpenYurt 以云端为准重建差异对象；KubeEdge 由 metamanager 重放事件队列。**上线前必须演练"断网 24h 期间云端发布新版本"场景**，确认收敛行为符合预期。网络与存储层面的弱网设计见 [[16-专项技术/01-边缘计算/09-edge-storage-network.md|边缘存储与网络]]，边缘侧安全加固见 [[16-专项技术/01-边缘计算/10-edge-security.md|边缘安全]]。

---

## 六、生产案例：工业产线 Modbus 数采上云

### 6.1 场景与架构

某汽车零部件产线：32 台 PLC/仪表通过 Modbus TCP 暴露寄存器，产线边缘机房 3 台边缘节点（SuperEdge 接管），数据要求 500ms 周期采集、本地缓冲、经 MQTT 上云。链路：

```text
Modbus 设备(32) --Modbus TCP--> Modbus broker Pod（Akri 按 IP 发现生成）
                                │ 寄存器映射→JSON
                                ▼
                          MQTT (edge namespace 内 EMQX)
                                │ 断网本地缓冲/续传
                                ▼
                          云端 Kafka → 数仓/实时看板
```

### 6.2 发现层：自定义 Modbus 发现

Modbus 不在 Akri 内置协议中，采用**自定义 Discovery Handler** 模式：写一个 gRPC 服务扫描产线网段的 Modbus TCP 从站（尝试读保持寄存器功能码 03 应答判定存活）并注册给 akri-agent。要点：扫描并发与超时受限（产线网络敏感），发现周期 30s 即可，handler 只做只读探测、不做写线圈操作。

### 6.3 消费层：broker 与 MQTT 出口

```yaml
apiVersion: akri.sh/v0
kind: Configuration
metadata:
  name: modbus-plc
spec:
  protocol:
    # 自定义 handler 名与细节由 handler 实现决定（此处为示意结构）
    universal: {}
  capacity: 1                    # Modbus 主站独占，一设备一 broker
  brokerPodSpec:
    containers:
    - name: modbus-collector
      image: registry.internal/edge/modbus-collector:1.4.2   # 自研采集器
      args:
      - --poll=500ms
      - --register-map=/etc/collector/map.yaml
      - --mqtt-endpoint=tcp://emqx.edge-iot:1883
      - --buffer-path=/data/buffer              # 断网本地缓冲
      resources:
        requests: {cpu: 100m, memory: 128Mi}
        limits:   {cpu: 500m, memory: 256Mi}
      volumeMounts:
      - name: reg-map
        mountPath: /etc/collector
      - name: buffer
        mountPath: /data
    volumes:
    - name: reg-map
      configMap:
        name: modbus-register-map
    - name: buffer
      hostPath:                   # 边缘节点本地盘做缓冲卷
        path: /var/lib/collector-buffer
        type: DirectoryOrCreate
  instanceServiceSpec: {}
  configurationServiceSpec: {}
```

> MQTT 出口用标准 broker（如同 namespace 的 EMQX `emqx/emqx:5.8` 单副本 StatefulSet，暴露 1883）即可，与 broker 采集器以 ClusterIP Service 对接，属常规部署不再展开。
```

### 6.4 运行验证与效果

```bash
# 🟢 低风险：32 台设备全部被发现、broker 分布在可达节点
kubectl get akrii -l akri.sh/configuration=modbus-plc | wc -l
kubectl get pods -o wide | grep modbus-collector

# 🟡 中风险：断网演练——拔掉边缘上联链路 30 分钟，验收 Pod 不驱逐/采集不断/缓冲增长/恢复续传
```

上线后：设备上下线自动生成/回收采集器，产线换 PLC 从"改脚本+重启采集服务"变为"改 Configuration/零操作"，月度数采缺口率从 2.1% 降到 0.3%。更多场景参考 [[16-专项技术/01-边缘计算/11-edge-use-cases.md|边缘用例集]] 与 [[16-专项技术/01-边缘计算/12-edge-production-runbook.md|边缘生产 Runbook]]。

---

## 七、故障排查与诊断流程

### 7.1 三类高频故障

**现象一：边缘节点 NotReady，但业务 Pod 全部正常**

```bash
# 🟢 低风险：确认是云边链路问题而非节点死亡
kubectl get nodes -o wide                     # NotReady
# 证据链：edge-health 同侪探测存活 + lite-apiserver 缓存命中 → 判定"假死"
# （边缘侧隧道日志需在边缘节点本地查看）
```

处置：这是 SuperEdge 类方案的**预期行为**（业务自治优先）。要做的是确认 Service 拓扑没有把流量绕到云端断点、以及 tolerationSeconds 足够覆盖断连时长；恢复后节点自动 Ready。

**现象二：云边隧道断连（云端无法 exec/logs 边缘 Pod）**

```bash
# 🟢 低风险：云端隧道组件日志
kubectl -n edge-system logs deploy/tunnel-cloud --tail=100
# 常见根因：边缘出口防火墙回收空闲连接 → 调小隧道心跳间隔
#           DNS 漂移 → 隧道端点固定 IP + 证书 SAN 包含该 IP
```

**现象三：设备发现失败（akrii 为空）**

```bash
# 🟢 低风险：逐层收敛
kubectl -n akri-system logs ds/akri-agent --field-selector spec.nodeName=<边缘节点> --tail=50
# ① handler 未启用？helm values 里 onvif.discovery.enabled
# ② 网络不通？在边缘节点 curl 设备 WS-Discovery/Modbus 端口
# ③ scope 过滤太严 / capacity 已满？放宽 scopes，kubectl get akrii -o yaml 查 nodes 列表
```

### 7.2 诊断速查表

| 现象 | 首查命令（🟢） | 高频根因 |
|------|----------------|----------|
| broker Pod Pending | `kubectl describe pod <broker>` | 边缘节点污点未加 toleration |
| Instance 反复创建删除 | `kubectl get akrii -w` | 设备网络抖动 / 发现周期过短 |
| 断网后 Pod 仍被驱逐 | `kubectl describe node` 看污点 | tolerationSeconds 未覆盖断连窗口 |
| 恢复后云端/边缘状态分叉 | `kubectl get pods --field-selector spec.nodeName=<edge>` | 断网期间云端 rollout，重调度未收敛 |

---

## 八、最佳实践与反模式

### 8.1 最佳实践

1. **设备独占语义显式化**：Modbus/串口设备 `capacity: 1`，网络流媒体类才放宽；capacity 配错会造成主站冲突、寄存器读乱。
2. **broker 无状态化 + 缓冲外置**：断网续传数据放 hostPath/PV，broker 重建不丢数据；发现过滤从紧（ONVIF scopes、Modbus 网段白名单），避免误接产线外设备。
3. **弱网演练常态化**：每季度做一次"断网 24h + 云端发布"演练，验证自治与收敛。
4. **边缘节点污点 + broker 容忍成对管理**：防止 broker 被调度到连不到设备网段的节点。
5. **隧道心跳与防火墙空闲超时对齐**：心跳间隔小于防火墙 idle timeout 的 1/2。

### 8.2 反模式

| 反模式 | 后果 | 纠正 |
|--------|------|------|
| 把设备 IP 手工写进 ConfigMap 当"设备管理" | 设备换 IP 全线断采，还要人肉重启 | 用 Akri 发现机制自动跟踪 |
| 为"看起来正常"把 NotReady 节点强制 Ready | 掩盖真实链路故障，调度错判 | 以 edge-health 同侪探测为准做判定 |
| broker 里塞业务逻辑 | 设备接入层膨胀成单体，改一处全产线重启 | broker 只做协议适配，业务在消费侧 |
| 断网窗口配置过短的 tolerationSeconds | 弱网抖动即驱逐，产线数据缺口 | 按最长预期断连时长 + 余量设置 |
| 在云端直接轮询设备（绕过边缘 broker）或 K3s 上强求完整自治 | 产线网络暴露、断网双杀 / 整站失控 | 数据在边缘终结上云走 MQTT；站点级自持换 SuperEdge/OpenYurt |

---

## 九、版本兼容性与升级注意

### 9.1 版本现状（2026-10 核对）

- Akri：`v0.14.0`（`gh api repos/project-akri/akri/releases/latest`）。CRD apiVersion 保持 `akri.sh/v0`，跨小版本升级前 `helm diff` 检查 agent/controller 镜像与 CRD 变更。
- SuperEdge：无统一 latest release 节奏，以官方仓库 release/文档为准；tunnel 证书与集群 CA 联动，升级 kubelet 前先升级 lite-apiserver 并核对证书 SAN。

### 9.2 升级检查单

1. Akri 升级后 `kubectl get akrii` 数量与升级前一致（发现逻辑回归）；broker 镜像 tag 固定 digest，避免 `latest` 在升级窗口漂移。
2. K8s 版本升级与边缘框架版本联动矩阵核对（edgecore/lite-apiserver 对 kubelet 版本有绑定窗口）。
3. 断网演练在升级后重跑一次（自治是最容易被升级破坏的能力）。

### 9.3 参考链接与延伸阅读

- Akri：https://github.com/project-akri/akri （v0.14.0）
- SuperEdge：https://github.com/superedge/superedge
- KubeEdge：https://kubeedge.io ；OpenYurt：https://openyurt.io ；K3s：https://k3s.io
- ONVIF 协议：https://www.onvif.org ；Modbus 规范：https://modbus.org ；OPC-UA：https://opcfoundation.org

## Related

- [[16-专项技术/01-边缘计算/07-superedge-architecture.md|SuperEdge 架构实践]]
- [[16-专项技术/01-边缘计算/04-kubeedge-architecture-deployment.md|KubeEdge 架构与部署]]
- [[16-专项技术/01-边缘计算/05-kubeedge-device-edge-apps.md|KubeEdge 设备与应用]]
- [[16-专项技术/01-边缘计算/06-openyurt-architecture.md|OpenYurt 架构]]
- [[16-专项技术/01-边缘计算/01-edge-computing-architecture.md|边缘计算架构]]
- [[16-专项技术/01-边缘计算/09-edge-storage-network.md|边缘存储与网络]]
- [[16-专项技术/01-边缘计算/12-edge-production-runbook.md|边缘生产 Runbook]]
