---
title: 阿里云专有云（Apsara Stack）组件与 K8s 运维关联索引
description: 面向 SRE 的 Apsara Stack 底座与 ACK 专有云版组件映射、排障入口与升级路径速查
category: cloud-provider
tags:
  - alibaba-cloud
  - apsara-stack
  - private-cloud
  - ack
  - kubernetes
  - aso
  - tianji
  - apsara
sources:
  - 阿里云专有云产品文档
  - ACK 专有云运维手册
  - ASO 运维控制台用户指南
created: 2026-06-26
updated: 2026-06-26
last_updated: 2026-06-26
summary: 阿里云专有云（Apsara Stack）飞天底座、ASO、天基、盘古、女娲、洛神、伏羲等组件与 Kubernetes 的集成点、常见工单场景、排障入口及升级路径索引。
relationships:
  - target: "[[domain-12-cloud-providers/01-alibaba-cloud/01-专有云架构概述.md]]"
    type: related_to
  - target: "[[domain-12-cloud-providers/01-alibaba-cloud/02-ACK集群运维.md]]"
    type: related_to
  - target: "[[domain-12-cloud-providers/01-alibaba-cloud/03-Terway-CNI网络.md]]"
    type: related_to
  - target: "[[domain-12-cloud-providers/01-alibaba-cloud/04-阿里云存储集成.md]]"
    type: related_to
  - target: "[[domain-12-cloud-providers/01-alibaba-cloud/05-阿里云SLB与Ingress.md]]"
    type: related_to
  - target: "[[domain-12-cloud-providers/01-alibaba-cloud/06-阿里云专有云远程顾问指南.md]]"
    type: related_to
difficulty: advanced
audience:
- AI Agent
- SRE
- 运维工程师
estimated_read_time: 15min
intent_queries:
- 专有云底座组件有哪些
- ASO 天基 伏羲 洛神 盘古 女娲 是什么
trigger_keywords:
- 专有云
- Apsara Stack
- ASO
- 天基
- 飞天
prerequisites:
- alicloud-basics
- k8s-architecture
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

# 阿里云专有云（Apsara Stack）组件与 K8s 运维关联索引

本文档面向在客户数据中心运维 [[domain-12-cloud-providers/01-alibaba-cloud/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 上 [[domain-12-cloud-providers/01-alibaba-cloud/02-ACK集群运维.md|ACK 专有云版]] 集群的 SRE、平台工程师及远程顾问。它将飞天底座、ASO、天基、盘古、女娲、洛神、伏羲等专有云核心组件与 Kubernetes 运维场景进行映射，帮助工程师在 Pod 调度失败、网络抖动、存储挂载异常、安全凭证过期等典型工单中快速定位应排查的底座组件，并明确何时需要联系阿里云 TAM 或驻场工程师。

---

## 1. 专有云整体架构概述

Apsara Stack 是阿里云公有云能力的私有化输出形态，其核心是 **飞天分布式操作系统（Apsara）**。在专有云环境中，ACK 专有云版直接构建在飞天底座提供的计算、网络、存储、安全、调度能力之上。

```
┌─────────────────────────────────────────────────────────────────────┐
│                       Apsara Stack 企业版 / 敏捷版                    │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                    运维与运营控制台                            │   │
│  │   ASO（Apsara Stack Operation）    ASCM（运营控制台）           │   │
│  │   天基（Tianji / Apsara Infrastructure）                       │   │
│  └─────────────────────────────────────────────────────────────┘   │
│  ┌──────────┐  ┌──────────┐  ┌──────────┐  ┌──────────┐          │
│  │   伏羲    │  │   洛神    │  │   盘古    │  │   女娲    │          │
│  │  分布式调度│  │  分布式网络│  │  分布式存储│  │ 一致性服务│          │
│  └────┬─────┘  └────┬─────┘  └────┬─────┘  └────┬─────┘          │
│       │             │             │             │                 │
│  ┌────┴─────┐  ┌────┴─────┐  ┌────┴─────┐  ┌────┴─────┐          │
│  │   ECS    │  │   VPC    │  │  ESSD    │  │   KMS    │          │
│  │  神龙     │  │ SLB/ALB  │  │  NAS/OSS │  │ ActionTrail│         │
│  │  ACK 专有版│  │ Terway   │  │  CPFS    │  │ RAM/RRSA │          │
│  └──────────┘  └──────────┘  └──────────┘  └──────────┘          │
└─────────────────────────────────────────────────────────────────────┘
```

### 1.1 核心底座组件定义

| 组件 | 定位 | 与 K8s 的关系 | 常见异常表现 |
|------|------|---------------|--------------|
| **飞天（Apsara）** | 分布式操作系统 | 计算/网络/存储/安全统一抽象 | 多产品同时异常 |
| **伏羲** | 分布式调度 | 决定 ECS/神龙实例创建、迁移、释放 | 节点无法扩容、调度 Pending |
| **洛神** | 分布式网络 | 提供 VPC、SLB、EIP、Terway 网络能力 | Service 不通、DNS 异常 |
| **盘古** | 分布式存储 | ESSD/NAS/OSS/CPFS 后端 | PVC 无法绑定、IO 延迟高 |
| **女娲** | 一致性服务 | 承载部分控制面元数据 | 控制面选主异常、API 超时 |
| **天基** | 底座运维编排 | 产品部署、升级、配置下发、监控采集 | 控制台不可用、升级失败 |
| **ASO** | Apsara Stack Operation 控制台 | 运维人员管理底座与云产品的入口 | 无排障入口时首先检查 |

### 1.2 ACK 专有云版在架构中的位置

ACK 专有云版由三部分组成：

- **ACK 管控面**：部署在天基托管的底座容器平台上，负责用户集群生命周期管理。
- **ACK 数据面**：用户购买的 ECS 或神龙 Worker 节点。
- **Addon 组件**：kube-proxy、Terway、CSI Plugin、CCM、metrics-server、ARMS/Prometheus Agent 等，依赖底座 OpenAPI 与网络/存储/安全组件交互。

> **关键认知**：专有云中很多 K8s 症状并非 ACK 自身 Bug，而是底座组件（洛神、盘古、伏羲、女娲）异常在容器层的映射。

---

## 2. 与 K8s 集群相关的专有云组件清单

### 2.1 计算组件

| 组件 | 作用 | K8s 集成点 | 常见工单场景 | 排障入口 | 升级路径 |
|------|------|------------|--------------|----------|----------|
| **ECS** | 虚拟机计算资源 | CCM 调用 ECS OpenAPI 管理节点 Label/Taint、SLB 后端挂载 | 节点 NotReady；ProviderID 为空；CA 无法弹节点 | ASO `产品运维 > 计算 > ECS`；`aliyun ecs DescribeInstances`；`kubectl describe node` | 规格变更/热迁移失败 → TAM；本地盘停机维护 → 协调窗口 |
| **神龙** | 裸金属服务器 | 用于大数据/GPU/网络密集型 Pod；神龙 Agent 上报硬件状态 | 节点重启后无法加入集群；MOC 卡驱动异常导致网络中断；GPU 异常 | ASO `产品运维 > 计算 > 神龙 > 实例健康`；`kubectl get node -o yaml`；BMC 日志 | 固件/BIOS/MOC 驱动升级 → 驻场工程师 |
| **ACK 专有云版** | 私有化 Kubernetes 发行版 | 深度集成 Terway、CSI、CCM、RRSA 等组件 | APIServer 高延迟；etcd 空间不足；Scheduler 无法分配节点 | ASO `产品运维 > 容器服务 > ACK 集群`；`kubectl cluster-info dump`；天基 `运维大盘 > ACK` | 版本升级/补丁回滚 → ASO 编排；失败 → TAM |

### 2.2 网络组件

| 组件 | 作用 | K8s 集成点 | 常见工单场景 | 排障入口 | 升级路径 |
|------|------|------------|--------------|----------|----------|
| **VPC / VSwitch** | 隔离二层网络与子网 | 集群创建于 VPC；Pod/Service CIDR 不能与 VPC CIDR 冲突 | 节点无法加入集群；跨 VSwitch 通信异常；IP 耗尽 | ASO `产品运维 > 网络 > VPC`；`aliyun vpc DescribeVSwitchAttributes`；`kubectl get nodes -o wide` | 路由表/ACL 变更走变更流程；核心网络设备升级 → 驻场 |
| **SLB / NLB / ALB** | 四/七层负载均衡 | CCM 监听 `type: LoadBalancer` Service 与 Ingress，维护后端服务器组 | `LoadBalancer` Service 长期 Pending；Ingress 502/504；健康检查失败 | ASO `产品运维 > 网络 > SLB`；`aliyun slb DescribeLoadBalancerAttribute`；`kubectl get svc -o yaml` | 规格升配/证书更新可自助；底层调度异常 → TAM |
| **EIP** | 弹性公网 IP | 公网 Ingress/Service 可能绑定 EIP | 公网 Ingress 无法访问；带宽打满；绑定关系丢失 | ASO `产品运维 > 网络 > EIP`；`aliyun vpc DescribeEipAddresses`；`kubectl get svc -o wide` | 带宽升配可自助；EIP 网关异常 → 阿里云网络团队 |
| **Terway / CNI** | 阿里云自研容器网络插件 | 分配 VPC 原生 IP 或 Overlay 地址；NetworkPolicy | `ContainerCreating` 且 `allocate eni failed`；跨节点丢包；NetworkPolicy 不生效 | ASO `产品运维 > 容器服务 > ACK 集群 > 网络插件`；`kubectl -n kube-system logs -l app=terway-daemon`；节点 `/var/log/messages \| grep terway` | ASO 集群运维页面触发；评估对业务 Pod 影响 |

### 2.3 存储组件

| 组件 | 作用 | K8s 集成点 | 常见工单场景 | 排障入口 | 升级路径 |
|------|------|------------|--------------|----------|----------|
| **ESSD** | 高性能块存储 | 阿里云 CSI Plugin 提供 `diskplugin.csi.alibabacloud.com` StorageClass | PVC Pending；`Multi-Attach error`；IO 抖动 | ASO `产品运维 > 存储 > 云盘`；`aliyun ecs DescribeDisks`；`kubectl describe pvc` | ASO 扩容可自助；盘古后端扩容 → 阿里云存储团队 |
| **NAS** | 分布式文件存储 | NAS CSI Plugin 提供动态/静态 Provision | 多 Pod 挂载权限不一致；挂载点无响应；CSI Node CrashLoop | ASO `产品运维 > 存储 > NAS`；`aliyun nas DescribeFileSystems`；`kubectl -n kube-system logs -l app=csi-plugin -c csi-plugin` | 后端维护窗口 → TAM；协议变更可自助 |
| **OSS** | 对象存储 | OSS CSI / OSSFS 挂载或 SDK 访问 | OSS 挂载目录卡顿；Bucket 权限拒绝；大文件写入失败 | ASO `产品运维 > 存储 > OSS`；`aliyun oss ls oss://<bucket>`；`kubectl -n kube-system logs -l app=csi-plugin -c ossfs` | 服务端升级由阿里云统一；客户端 OSSFS 可自助 |
| **CPFS** | 并行文件系统 | CPFS CSI 挂载 GPU/神龙节点 | 训练任务带宽不足；客户端与内核不兼容 | ASO `产品运维 > 存储 > CPFS`；`kubectl -n kube-system logs -l app=csi-cpfs-plugin` | 后端/协议栈升级 → 阿里云存储专家 |
| **盘古** | 飞天分布式存储底座 | ESSD/NAS/OSS/CPFS 的专有云底层实现 | 全集群存储 IO 异常；快照/扩容全局失败 | 天基 `运维大盘 > 存储 > 盘古集群`；ASO `底座运维 > 盘古` | 任何变更必须由驻场工程师执行 |

### 2.4 安全组件

| 组件 | 作用 | K8s 集成点 | 常见工单场景 | 排障入口 | 升级路径 |
|------|------|------------|--------------|----------|----------|
| **RAM** | 身份与访问管理 | ECS 绑定 RAM Role；CCM/CSI/CNI 通过 RAM 凭据调用 OpenAPI | CCM 无法创建 SLB；CSI 无法创建云盘；实例元数据访问异常 | ASO `产品运维 > 安全 > RAM`；`aliyun ram GetRole`；`kubectl -n kube-system logs -l app=cloud-controller-manager` | RAM 策略变更可自助；AK 泄漏/根账号异常 → TAM |
| **RRSA** | Pod 级 RAM Role 联邦 | ServiceAccount 通过 OIDC 令牌扮演 RAM Role | Pod 访问 OSS/SLS/KMS 报 `InvalidAccessKeyId.NotFound`；凭证过期 | ASO `产品运维 > 容器服务 > ACK 集群 > 集群认证 > RRSA`；`kubectl get sa -o yaml`；Pod 内 `aliyun sts GetCallerIdentity` | 功能开关升级 → TAM；角色策略变更可自助 |
| **KMS** | 密钥管理服务 | 加密 Secret、云盘、OSS；KMS Provider 加解密 | 加密 PVC 创建失败；Secret 解密失败；证书过期 | ASO `产品运维 > 安全 > KMS`；`aliyun kms GetKeyInstance`；`kubectl -n kube-system logs -l app=kms-plugin` | 实例扩容/证书续期 → 阿里云安全团队 |
| **ActionTrail** | 操作审计 | 审计 ACK 管控面与底座组件变更 | 追溯 SLB 监听被谁修改；集群被删除 | ASO `产品运维 > 安全 > ActionTrail`；`aliyun actiontrail LookupEvents` | 保留策略变更可自助；服务端异常 → TAM |
| **云安全中心** | 主机与容器安全防护 | DaemonSet Agent 扫描镜像与节点风险 | 镜像高危漏洞；节点异常进程；Agent 资源占用高 | ASO `产品运维 > 安全 > 云安全中心`；`kubectl -n kube-system get ds -l app=sas-agent` | Agent 升级可自助；病毒/入侵 → 阿里云安全专家 |

### 2.5 可观测性组件

| 组件 | 作用 | K8s 集成点 | 常见工单场景 | 排障入口 | 升级路径 |
|------|------|------------|--------------|----------|----------|
| **ARMS** | 应用实时监控/APM | Agent Sidecar/InitContainer 注入应用 Pod | 接口延迟高但基础设施正常；链路数据缺失 | ASO `产品运维 > 中间件与监控 > ARMS`；`kubectl logs <pod> -c arms-agent-init` | 服务端升级 → TAM；Agent 升级可自助 |
| **SLS** | 日志服务 | Logtail DaemonSet 采集容器与节点日志；审计日志投递 | 日志采集延迟/丢失；Logtail OOM；`ShardReadQuotaExceed` | ASO `产品运维 > 大数据与日志 > SLS`；`aliyun log get_project`；`kubectl -n kube-system logs -l app=logtail` | Shard 扩容可自助；索引重建 → TAM |
| **Prometheus / Grafana** | 托管时序库与可视化 | Prometheus Agent 抓取 K8s 指标；Grafana 展示大盘 | 大盘无数据；Remote Write 失败；`DatasourceError` | ASO `产品运维 > 中间件与监控 > Prometheus`；`kubectl -n arms-prom get pods` | 实例规格升级 → TAM；Dashboard 变更可自助 |

### 2.6 底座组件

| 组件 | 作用 | K8s 集成点 | 常见工单场景 | 排障入口 | 升级路径 |
|------|------|------------|--------------|----------|----------|
| **ASO** | 专有云运维总入口 | ACK 扩缩容、升级、Addon 管理通过 ASO/天基下发 | 控制台无法登录；升级任务失败；告警风暴 | ASO `底座运维 > 告警中心 / 变更中心 / 产品列表`；天基 `运维大盘 > ASO 自身健康` | 驻场工程师执行 |
| **天基** | 底座部署/配置/编排/监控/自愈 | ACK 管控面容器由天基托管 | 控制台按钮无响应；产品 Pod 反复重启；配置下发不生效 | 天基 `产品运维 > 天基 > 集群 > 服务实例`；`kubectl get pods -n tianji-system` | 底座核心变更；驻场工程师执行 |
| **女娲** | 飞天一致性服务 | ACK 管控面可能直接复用女娲作为元数据存储 | APIServer 响应极慢；etcd Leader 切换；状态不一致 | 天基 `运维大盘 > 一致性服务 > 女娲`；ASO `底座运维 > 女娲` | 高危操作；必须联系 TAM/驻场 |
| **伏羲** | 飞天分布式调度 | 节点创建/迁移/释放依赖伏羲 | CA 无法弹节点；热迁移失败；资源池不足长期 Pending | 天基 `运维大盘 > 调度 > 伏羲`；ASO `底座运维 > 伏羲 > 资源池` | 调度策略/资源池变更 → TAM |

---

## 3. 专有云 K8s 工单 → 可能涉及的底座组件对照表

| K8s 工单现象 | 优先排查组件 | 排查方向 | 升级触发条件 |
|--------------|--------------|----------|--------------|
| Pod 一直 `Pending` | 伏羲、ECS、VSwitch、Terway | 资源池/ENI/IP/调度约束 | 全局资源池不足 |
| 节点 `NotReady` | 伏羲、ECS、神龙、洛神 | 实例状态、网络连通性、Kubelet | 宿主机/网络设备故障 |
| `LoadBalancer` Service 无 IP | SLB/NLB/ALB、CCM、RAM | OpenAPI 调用、配额、监听配置 | 底层负载均衡调度异常 |
| Ingress 访问 502/504 | SLB/ALB、Terway、后端 Pod | 健康检查、后端服务器组、网络策略 | ALB 控制面异常 |
| PVC 无法绑定 | ESSD/NAS/OSS CSI、盘古 | 存储配额、后端集群状态、CSI 插件 | 盘古集群异常 |
| Pod 挂载云盘失败 | ESSD、CSI、伏羲 | 云盘状态、多挂载冲突、节点漂移 | 存储网关异常 |
| Pod 无法访问 OSS/SLS/KMS | RRSA、RAM、KMS | 角色绑定、OIDC、凭据过期 | KMS/RRSA 服务端异常 |
| 容器标准输出丢失 | SLS、Logtail | 采集配置、Shard 配额、Agent 状态 | SLS 服务端异常 |
| 监控大盘无数据 | Prometheus、Grafana、SLS | Remote Write、Datasource、Agent | Prometheus 服务端异常 |
| 控制台无法登录/操作 | ASO、天基、女娲 | ASO 自身服务、配置下发、元数据 | 天基/ASO 自身故障 |
| 全集群网络异常 | 洛神、VPC、Terway | 路由、VXLAN/ENI、网关 | 洛神控制面异常 |
| 全集群存储 IO 异常 | 盘古、ESSD CSI | 存储集群水位、延迟、副本 | 盘古集群异常 |

---

## 4. 底座组件异常 → K8s 症状对照表

| 底座组件异常 | 典型 K8s 症状 | 快速验证命令 | 建议动作 |
|--------------|---------------|--------------|----------|
| **伏羲调度失败** | Pod `Pending`；`FailedScheduling` 显示 `0/xx nodes available` | `kubectl describe pod <pod>` | 检查 ASO 资源池水位；联系 TAM 释放或扩容资源 |
| **洛神网络异常** | 跨节点 Pod 不通；Service ClusterIP 异常；DNS 失败 | `kubectl run -it --rm debug --image=nicolaka/netshoot` | 检查 VPC 路由表、VSwitch、安全组；联系网络驻场 |
| **盘古存储异常** | 所有 PVC `Pending`；ESSD IO 延迟飙升；NAS 无响应 | `kubectl get pvc,pv -A`、`iostat -x 1` | 检查天基盘古集群健康；联系存储团队 |
| **女娲一致性异常** | APIServer 频繁超时；etcd Leader 切换；状态不一致 | `kubectl get --raw=/healthz`、`etcdctl endpoint status` | 检查女娲节点状态；禁止自行重启；联系 TAM |
| **天基配置下发异常** | Addon Pod 版本不一致；ACK 控制台功能缺失 | `kubectl -n kube-system get pods -o wide` | 检查天基变更任务；联系驻场工程师 |
| **ASO 自身异常** | 无法登录控制台；告警丢失；升级任务卡死 | 浏览器访问 ASO 健康检查 API | 检查天基中 ASO 服务实例；联系驻场 |
| **RAM/RRSA 异常** | CCM/CSI 报权限错误；Pod 访问云服务失败 | `kubectl logs -n kube-system -l app=cloud-controller-manager` | 检查 RAM 角色策略；RRSA 重新关联；联系安全团队 |
| **KMS 异常** | 加密 PVC 创建失败；Secret 解密报错 | `kubectl describe pvc <pvc>`、`kubectl -n kube-system logs -l app=kms-plugin` | 检查 KMS 密钥状态；联系安全团队 |
| **SLS 异常** | 容器日志缺失；审计日志查不到 | `kubectl -n kube-system logs -l app=logtail` | 检查 SLS Project/Logstore/Shard；联系日志团队 |
| **Prometheus 异常** | Grafana 无数据；告警规则不触发 | `kubectl -n arms-prom get pods` | 检查 Prometheus 实例状态；联系监控团队 |

---

## 5. 排障检查清单

- [ ] **确认症状层级**：单个 Pod、单个节点、单个命名空间，还是全集群？
- [ ] **确认 ACK 集群状态**：`kubectl get nodes`、`kubectl get --raw=/livez,/readyz`
- [ ] **查看 ASO 告警中心**：是否有洛神/盘古/女娲/伏羲/天基告警
- [ ] **查看 ASO 集群详情**：节点、Addon、事件、版本是否异常
- [ ] **按组件清单定位**：计算→伏羲/ECS/神龙；网络→洛神/VPC/SLB/Terway；存储→盘古/ESSD/NAS/OSS；安全→RAM/RRSA/KMS
- [ ] **区分 K8s 层与底座层**：ACK 组件日志说明症状，底座组件状态决定根因
- [ ] **评估升级路径**：涉及天基/ASO/盘古/女娲/伏羲/洛神底层变更，必须联系 TAM/驻场
- [ ] **记录关键证据**：保存 `kubectl cluster-info dump`、ASO 截图、aliyun CLI 输出

---

## 6. 常用命令速查

```bash
# 集群状态
kubectl get nodes -o wide
kubectl get pods -n kube-system
kubectl get --raw=/livez && kubectl get --raw=/readyz
kubectl cluster-info dump --all-namespaces --output-directory=/tmp/k8s-dump

# aliyun CLI
aliyun ecs DescribeInstances --RegionId cn-stack-xxx --InstanceIds '["i-xxx"]' --endpoint xxx.stack.aliyuncs.com
aliyun vpc DescribeVSwitchAttributes --VSwitchId vsw-xxx --endpoint vpc.aliyuncs.com
aliyun slb DescribeLoadBalancerAttribute --LoadBalancerId lb-xxx --endpoint slb.aliyuncs.com
aliyun nas DescribeFileSystems --RegionId cn-stack-xxx --endpoint nas.aliyuncs.com
aliyun ram GetRole --RoleName KubernetesWorkerRole-xxx
aliyun actiontrail LookupEvents --LookupAttributes '[{"Key":"EventName","Value":"CreateCluster"}]'
```

### ASO 控制台快速路径

| 场景 | ASO 控制台路径 |
|------|----------------|
| ECS 实例 | `产品运维 > 计算 > ECS > 实例列表` |
| ACK 集群 | `产品运维 > 容器服务 > ACK 集群 > 集群详情` |
| VPC/VSwitch | `产品运维 > 网络 > VPC` |
| SLB | `产品运维 > 网络 > SLB > 实例监听` |
| 云盘 | `产品运维 > 存储 > 云盘 > 磁盘列表` |
| NAS | `产品运维 > 存储 > NAS > 文件系统` |
| RAM 角色 | `产品运维 > 安全 > RAM > 角色` |
| 告警 | `底座运维 > 告警中心 > 当前告警` |
| 变更任务 | `底座运维 > 变更中心 > 变更列表` |
| 天基服务 | `产品运维 > 天基 > 集群 > 服务实例` |
| 盘古 | `底座运维 > 存储 > 盘古集群` |
| 女娲 | `底座运维 > 一致性服务 > 女娲` |

---

## 7. 何时联系阿里云 TAM / 驻场工程师

| 操作类型 | 建议处理方 | 说明 |
|----------|------------|------|
| ACK 版本升级 / 补丁回滚 | TAM + 客户窗口期 | 需评估业务影响 |
| 天基 / ASO 自身升级或重启 | 驻场工程师 | 底座核心，禁止客户自行操作 |
| 盘古 / 女娲 / 伏羲 / 洛神 集群变更 | 驻场工程师 | 影响范围大，需专家评估 |
| 神龙固件 / MOC 驱动 / BIOS 升级 | 驻场工程师 | 需进入机房或 BMC |
| 全局网络/存储中断 | TAM 立即升级 | 通常触发 P0 响应流程 |
| RAM/RRSA/KMS 根证书或密钥问题 | 阿里云安全团队 | 涉及身份信任根 |
| ASO 控制台无法登录且影响排障 | 驻场工程师 | 先通过天基 CLI 或后台恢复 |

---

## 8. 总结

专有云 K8s 运维的核心差异在于：**ACK 只是飞天底座能力在容器层的暴露**。当 Pod 调度失败、网络不通、存储挂载异常时，SRE 需要同时查看 ACK 组件日志与 ASO / 天基 / 飞天底座组件状态。本索引文档将常见工单现象与底座组件建立了双向映射，帮助团队快速判断问题归属，并明确何时必须引入阿里云 TAM 或驻场工程师。

---

## Related

- [[domain-12-cloud-providers/01-alibaba-cloud/01-专有云架构概述.md|01 专有云架构概述]]
- [[domain-12-cloud-providers/01-alibaba-cloud/02-ACK集群运维.md|02 ACK集群运维]]
- [[domain-12-cloud-providers/01-alibaba-cloud/03-Terway-CNI网络.md|03 Terway CNI网络]]
- [[domain-12-cloud-providers/01-alibaba-cloud/04-阿里云存储集成.md|04 阿里云存储集成]]
- [[domain-12-cloud-providers/01-alibaba-cloud/05-阿里云SLB与Ingress.md|05 阿里云SLB与Ingress]]
- [[domain-12-cloud-providers/01-alibaba-cloud/06-阿里云专有云远程顾问指南.md|06 阿里云专有云远程顾问指南]]

## See Also

- K8s 标准诊断工作流
- Prometheus 排障指南
- RRSA 配置指南
