---
title: Kubernetes 边缘计算生产运维 Runbook
description: 覆盖 KubeEdge CloudCore/EdgeCore 高可用、边缘节点上下线、离线自治、带宽受限可观测性、边缘证书生命周期与边缘灾难恢复的生产级运维手册
summary: 覆盖 KubeEdge CloudCore/EdgeCore 高可用、边缘节点上下线、离线自治、带宽受限可观测性、边缘证书生命周期与边缘灾难恢复的生产级运维手册
category: specialized-tech
tags:
- production
- best-practices
- playbook
- edge-computing
- kubeedge
- cloudcore
- edgecore
- offline-autonomy
- observability
- certificate
- disaster-recovery
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 边缘计算生产运维 Runbook 是什么
- KubeEdge CloudCore EdgeCore HA 怎么做
- 边缘节点上下线流程
- 边缘离线自治怎么实现
- 边缘带宽受限可观测性
- 边缘证书生命周期管理
- 边缘灾难恢复
trigger_keywords:
- edge computing
- kubeedge
- cloudcore
- edgecore
- offline autonomy
- edge observability
- edge certificate
- edge dr
- edge node onboarding
prerequisites:
- kubectl-basics
- kubeedge-basics
- networking-basics
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 边缘计算生产运维 Runbook

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产运维 Runbook

本 Runbook 面向管理 Kubernetes 边缘计算平台的 SRE 与边缘运维工程师，聚焦 KubeEdge 生产环境的核心运维场景：CloudCore/EdgeCore 高可用、边缘节点安全上下线、离线自治、带宽受限条件下的可观测性、边缘证书生命周期管理以及边缘灾难恢复。边缘场景具有网络不稳定、节点分散、现场运维困难等特点，必须在架构设计与运维流程中充分考虑断网自治、低带宽与安全准入。

---

## 1. 适用场景与范围

- **KubeEdge 集群**：CloudCore 部署在云端/中心机房，EdgeCore 部署在边缘节点。
- **CloudCore 高可用**：多实例负载均衡 + 持久化消息队列（MQTT/Quic）。
- **边缘节点生命周期**：批量 onboarding、退役、替换、固件/系统升级。
- **离线自治**：边缘节点与云端断联时，本地 Pod 仍能持续运行并处理本地事件。
- **带宽受限可观测性**：边缘侧指标/日志的压缩、采样、边缘聚合与断网缓存。
- **边缘证书生命周期**：EdgeCore 与 CloudCore 之间 TLS 证书的申请、续期与轮换。
- **边缘灾难恢复**：节点替换、EdgeCore 重装、本地数据保护与业务快速切换。

---

## 2. 前置条件与工具

### 2.1 基础设施前提

- 云端已部署 KubeEdge CloudCore，配置 HA（≥2 副本）与外部 LoadBalancer。
- 边缘节点满足 KubeEdge 运行要求：systemd、container runtime、网络可达 CloudCore。
- 已配置 MQTT/Quic 消息通道的持久化后端（如 EMQ X、RabbitMQ、NATS）。
- 边缘节点具备本地持久化存储用于离线自治与数据缓存。

### 2.2 必备工具

| 工具 | 用途 | 推荐版本 |
|------|------|----------|
| `keadm` | KubeEdge 安装与节点管理 | v1.18+ |
| `kubectl` | 云端资源管理 | v1.28+ |
| `edgecore` | 边缘节点组件 | 与 CloudCore 一致 |
| `cloudcore` | 云端组件 | v1.18+ |
| MQTT/Quic Broker | 云边消息通道 | EMQ X 5.x / NATS 2.10+ |
| OpenTelemetry Collector | 边缘指标/日志采集 | 0.104+ |
| cert-manager / 自签 CA | 边缘证书管理 | v1.15+ |

---

## 3. 标准操作流程

### 3.1 CloudCore 高可用部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: cloudcore
  namespace: kubeedge
spec:
  replicas: 3
  selector:
    matchLabels:
      app: cloudcore
  template:
    metadata:
      labels:
        app: cloudcore
    spec:
      affinity:
        podAntiAffinity:
          requiredDuringSchedulingIgnoredDuringExecution:
          - labelSelector:
              matchLabels:
                app: cloudcore
            topologyKey: topology.kubernetes.io/zone
      containers:
      - name: cloudcore
        image: kubeedge/cloudcore:v1.18.0
        volumeMounts:
        - name: config
          mountPath: /etc/kubeedge/config
        - name: certs
          mountPath: /etc/kubeedge/certs
      volumes:
      - name: config
        configMap:
          name: cloudcore-config
      - name: certs
        secret:
          secretName: cloudcore-certs
```

前置外部负载均衡，将 EdgeCore 指向 CloudCore 的 VIP 或 DNS 域名，避免单实例故障。

### 3.2 边缘节点上线（Onboarding）

#### 云端准备

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 获取 join token
keadm gettoken

# 可选：预创建 Node 对象并绑定标签
kubectl label node edge-node-01 node-type=edge region=south
```
#### 边缘节点执行

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 EdgeCore
keadm join --cloudcore-ipport=<CLOUDCORE_VIP>:10000 \
  --token=<TOKEN> \
  --kubeedge-version=v1.18.0 \
  --certPath=/etc/kubeedge/certs \
  --remote-runtime-endpoint=unix:///run/containerd/containerd.sock

# 验证
systemctl status edgecore
kubectl get node edge-node-01
```
#### 上线检查清单

- [ ] EdgeCore 已注册，节点状态 Ready。
- [ ] 本地业务镜像已预加载或可从边缘镜像仓库拉取。
- [ ] 节点标签、污点、资源容量正确。
- [ ] 离线自治目录（默认 `/var/lib/edged`）磁盘空间充足。

### 3.3 边缘节点下线/替换

#### 优雅下线

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 云端：隔离节点并驱逐工作负载
kubectl cordon edge-node-01
kubectl drain edge-node-01 --ignore-daemonsets --delete-emptydir-data --force

# 边缘：重置 EdgeCore
keadm reset

# 云端：删除节点对象
kubectl delete node edge-node-01
```
#### 替换节点

新节点使用相同主机名与 token 执行 `keadm join`，云端原 Node 对象被替换。若需保留本地 PV 数据，先备份 `/var/lib/edged` 与 `/var/lib/kubelet`。

### 3.4 离线自治

KubeEdge EdgeCore 默认在断网时继续运行已调度的 Pod，并监听本地设备孪生（device twin）事件。生产建议：

- 配置 `edgecore.yaml` 的 `metaManager` 缓存：
  ```yaml
  metaManager:
    metaServer:
      enable: true
    context:
      sendModule:
        metaManager:
          enable: true
  ```
- 对关键业务 Pod 设置 `restartPolicy: Always`，避免 EdgeCore 重启后无法恢复。
- 使用 `edgeApplication` 或本地 DaemonSet 确保服务在断网期间自治。

### 3.5 带宽受限可观测性

#### 边缘侧 OpenTelemetry Collector 配置

```yaml
apiVersion: opentelemetry.io/v1beta1
kind: OpenTelemetryCollector
metadata:
  name: edge-collector
  namespace: observability
spec:
  mode: daemonset
  config: |
    receivers:
      prometheus:
        config:
          scrape_configs:
          - job_name: 'edge-metrics'
            scrape_interval: 60s
            static_configs:
            - targets: ['localhost:9100']
    processors:
      batch:
        timeout: 60s
        send_batch_size: 1024
      resource:
        attributes:
        - key: edge.node
          value: ${NODE_NAME}
          action: upsert
    exporters:
      otlphttp:
        endpoint: https://otel-gateway.cloud.internal:4318
        retry_on_failure:
          enabled: true
        sending_queue:
          enabled: true
          queue_size: 1000
    service:
      pipelines:
        metrics:
          receivers: [prometheus]
          processors: [batch, resource]
          exporters: [otlphttp]
```

#### 关键策略

- **增大采集间隔**：边缘节点指标 30s–60s，日志批量上传。
- **边缘预聚合**：使用 OpenTelemetry 的 delta→cumulative 转换减少上传量。
- **断网缓存**：配置发送队列与本地 WAL，网络恢复后补发。
- **关键告警本地触发**：在边缘节点部署轻量 Alertmanager 或规则引擎，避免依赖云端告警。

### 3.6 边缘证书生命周期

KubeEdge CloudCore 与 EdgeCore 之间默认通过自签 CA 建立 TLS。证书过期会导致 EdgeCore 无法重连。

#### 证书检查

```bash
# CloudCore 侧
openssl x509 -in /etc/kubeedge/certs/server.crt -noout -dates

# EdgeCore 侧
openssl x509 -in /etc/kubeedge/certs/edge.crt -noout -dates
```

#### 轮换流程

1. 在 CloudCore 生成新 CA 与 server 证书。
2. 滚动更新 CloudCore Deployment，加载新证书。
3. 通过边缘现场或 OTA 方式分发新 `ca.crt` 与 `edge.crt` 到 EdgeCore。
4. 重启 EdgeCore。
5. 验证所有边缘节点 Ready。

> 大规模边缘场景建议接入 cert-manager + SPIFFE/SPIRE，实现自动证书签发与续期。

---

## 4. 关键检查点与验证命令

| 检查项 | 命令 | 合格标准 |
|--------|------|----------|
| CloudCore 健康 | `kubectl get pods -n kubeedge -l app=cloudcore` | 3/3 Running |
| 边缘节点注册 | `kubectl get nodes -l node-type=edge` | Ready |
| EdgeCore 状态 | 边缘节点 `systemctl status edgecore` | active |
| 云边通道 | 边缘节点 `curl https://<cloudcore>:10002/healthz` | 返回 ok |
| 离线自治 | 断开边缘网络 5 分钟，本地 Pod 仍运行 | Pod 未重启 |
| 证书有效期 | `openssl x509 -in /etc/kubeedge/certs/edge.crt -noout -dates` | > 30 天 |
| 指标上传 | 云端 Thanos 可查询边缘节点指标 | 数据存在 |

---

## 5. 回滚/应急方案

- **CloudCore 单实例故障**：负载均衡自动切换，EdgeCore 重连健康实例。
- **EdgeCore 无法注册**：检查 token、证书、防火墙、CloudCore VIP 可达性。
  ```bash
  keadm reset && keadm join --cloudcore-ipport=<VIP>:10000 --token=<TOKEN>
  ```
- **边缘节点离线超阈值**：通过现场运维或 IPMI 重启节点；若硬件故障，执行替换流程。
- **证书过期导致大规模掉线**：在边缘现场使用 USB/脚本批量更新证书，或启用 cert-manager 自动续期。
- **本地存储损坏**：从云端 Velero 备份或对象存储恢复业务配置与数据。

---

## 6. 风险与注意事项

1. **边缘节点不可远程 SSH 时运维成本高**：必须建立 OTA 升级、远程重启、IPMI/BMC 访问能力。
2. **断网期间 Pod 无法被云端调度**：EdgeCore 仅保证本地状态不丢失，新增/删除操作需联网后同步。
3. **边缘证书分发不可依赖云端**：证书过期时 EdgeCore 已无法连接，必须预置或现场更新。
4. **镜像拉取受带宽限制**：关键镜像应在节点上线前预加载，或部署边缘镜像仓库缓存。
5. **边缘数据安全**：本地持久化数据需加密，设备丢失后能够远程擦除证书与敏感文件。

---

## 7. 相关 Runbook / 推荐阅读

- [[专项技术/99-production-readiness-operations-guide.md|专项技术 生产就绪运维指南]]
- [[生产运维/99-production-readiness-operations-guide.md|生产运维 生产就绪运维指南]]
- [[专项技术/边缘计算/03-kubeedge-architecture-deployment.md|KubeEdge 架构与部署]]
- [[专项技术/边缘计算/04-kubeedge-device-edge-apps.md|KubeEdge 设备与边缘应用]]
- [[专项技术/边缘计算/08-edge-storage-network.md|边缘存储与网络]]
- [[专项技术/边缘计算/09-edge-security.md|边缘安全]]
- [[集群基础/控制平面/34-certificate-pki-lifecycle-runbook.md|Kubernetes 证书与 PKI 生命周期运维 Runbook]]


<!-- risk-assessed -->
