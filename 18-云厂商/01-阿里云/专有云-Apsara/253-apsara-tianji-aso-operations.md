---
title: 专有云（Apsara Stack）- 天基 / ASO 运维流程
description: 专有飞天底座天基（Tianji）部署编排与 ASO 运维控制台的运维流程、控制台操作路径、变更/巡检/自愈与排障入口
summary: 专有云飞天底座天基（Tianji）与 ASO 运维控制台的部署、升级、配置下发、巡检、变更中心与自愈运维流程，含 ASO 控制台路径表与排障入口。
category: cloud-provider
tags:
- alibaba-cloud
- apsara-stack
- private-cloud
- tianji
- aso
- operations
- deployment
- kubernetes
tier: core
sources:
- 阿里云专有云运维规范
- 天基/ASO 控制台用户指南
created: 2026-07-23
last_updated: 2026-07-23
relationships:
- target: '[[18-云厂商/01-阿里云/01-专有云架构概述.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/apsara-stack-components.md]]'
  type: related_to
- target: '[[18-云厂商/01-阿里云/专有云-Apsara/254-apsara-upgrade-patch-management.md]]'
  type: related_to
difficulty: advanced
audience:
- SRE
- 平台工程师
- 远程顾问
- 驻场运维
estimated_read_time: 18min
intent_queries:
- 天基 Tianji 是什么
- ASO 控制台怎么操作
- 专有云部署升级配置下发流程
- 天基自愈巡检怎么用
trigger_keywords:
- 天基
- Tianji
- ASO
- 部署编排
- 变更中心
- 巡检
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。

# 专有云（Apsara Stack）- 天基 / ASO 运维流程

本文档面向在客户数据中心运维 [[18-云厂商/01-阿里云/01-专有云架构概述.md|阿里云专有云（Apsara Stack）]] 的 SRE、平台工程师与远程顾问。聚焦飞天底座两大运维中枢——**天基（Tianji / Apsara Infrastructure）**与 **ASO（Apsara Stack Operation）**，梳理部署、升级、配置下发、巡检、变更中心与自愈的完整运维流程，并给出 ASO 控制台操作路径与排障入口。

> **关键认知**：天基是底座的「自动化部署与编排引擎」，ASO 是底座的「运维与运营控制台」。ACK 专有云版的管控面容器由天基托管，日常运维操作大多经 ASO 下发。

---

## 1. 天基与 ASO 的定位与分工

| 维度 | 天基（Tianji） | ASO（Apsara Stack Operation） |
|------|----------------|-------------------------------|
| **定位** | 飞天底座自动化部署/编排/自愈引擎 | 运维与运营总入口控制台 |
| **职责** | 产品部署、升级、配置下发、基线管理、监控采集、自愈 | 租户管理、资源配额、产品运维入口、告警/变更中心 |
| **使用者** | 驻场工程师、底座运维 | 平台运维、SRE、租户管理员 |
| **底层形态** | 分布式服务集群（天基集群） | Web 控制台 + 后端服务 |
| **K8s 关系** | 托管 ACK 管控面容器平台 | 提供 ACK 集群运维入口（扩缩容、升级、Addon） |

```
┌─────────────────────────────────────────────────────┐
│              ASO 控制台（运维/运营总入口）             │
│   租户管理 | 资源配额 | 产品运维 | 告警/变更中心        │
└───────────────────────┬─────────────────────────────┘
                        │ 调用
                        ↓
┌─────────────────────────────────────────────────────┐
│              天基（Tianji）编排引擎                    │
│   部署编排 | 升级编排 | 配置下发 | 基线 | 监控 | 自愈    │
└───────────────────────┬─────────────────────────────┘
                        │ 托管/调度
            ┌───────────┼───────────┐
            ↓           ↓           ↓
       ┌────────┐  ┌────────┐  ┌────────────┐
       │ ACK    │  │ ECS/   │  │ 网络/存储/  │
       │ 管控面 │  │ 神龙   │  │ 安全产品    │
       └────────┘  └────────┘  └────────────┘
```

---

## 2. 部署流程

### 2.1 产品部署总流程

天基以「产品」为单位进行部署编排，每个产品（如 ACK、ECS、SLB）定义在天基的部署模板中。

| 阶段 | 操作位置 | 关键动作 | 远程顾问角色 |
|------|----------|----------|--------------|
| 1. 前置检查 | 天基 `运维大盘` | 确认底座资源池、网络、存储基线就绪 | 指导客户检查项 |
| 2. 模板加载 | 天基 `产品运维 > 部署` | 加载产品部署模板（版本号对齐） | 确认模板版本 |
| 3. 参数填写 | 天基部署表单 | 填写 Region、VPC、规格、AZ 映射 | 提供推荐值 |
| 4. 预校验 | 天基自动执行 | 资源/网络/配额校验 | 查看校验报告 |
| 5. 执行部署 | 天基部署任务 | 滚动部署各组件，实时显示进度 | 监控进度与告警 |
| 6. 部署验收 | ASO 产品列表 | 确认产品状态为「运行中」 | 执行验收清单 |

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 驻场节点：查看天基部署任务进度（具体命令以客户天基版本为准）
# 天基通常提供 CLI 或后台查询接口
kubectl get pods -n tianji-system
kubectl get pods -n tianji-system -o wide | grep -i deploy
# 查看天基托管的 ACK 管控面容器
kubectl get pods -A | grep -E 'ack|cs|managed'
```

### 2.2 ACK 集群部署要点

ACK 专有云版的集群创建可通过 **ASO 控制台** 或 **专有云 OpenAPI（POP）** 触发，底层由天基编排 ECS/神龙节点创建与 Addon 部署。

```yaml
# 通过 ASCM/ASO OpenAPI 创建 ACK 专有版集群（POP 接口示例）
# POST /api/v1/clusters
{
  "cluster_type": "Dedicated",
  "name": "prod-k8s-apsara",
  "region_id": "cn-apsara-local",
  "vpc_id": "vpc-apsara-xxx",
  "vswitch_ids": ["vsw-apsara-1", "vsw-apsara-2"],
  "master_instance_types": ["ecs.g7.2xlarge"],
  "num_of_masters": 3,
  "worker_instance_types": ["ecs.g7.2xlarge"],
  "num_of_nodes": 3,
  "container_cidr": "172.20.0.0/16",
  "service_cidr": "172.21.0.0/20",
  "addons": [
    {"name": "terway-eniip"},
    {"name": "csi-plugin"},
    {"name": "metrics-server"},
    {"name": "logtail-ds"}
  ],
  "kubernetes_version": "1.30-aliyun.1"
}
```

> **远程顾问注意**：集群创建卡住时，优先查 ASO「变更中心」对应任务的状态与天基部署日志，而非直接查 kubectl——很多前置步骤（ECS 创建、网络打通）发生在 K8s 之前。

---

## 3. 升级与配置下发

### 3.1 升级编排流程

| 步骤 | 操作 | 位置 | 风险 |
|------|------|------|------|
| 1 | 确认升级目标版本与兼容性矩阵 | ASO `产品运维 > 容器服务 > ACK 集群 > 升级` | 🟡 |
| 2 | 执行前置检查（节点水位、Addon 版本、API 废弃） | 天基预校验 | 🟡 |
| 3 | 选择升级策略（滚动/分批） | ASO 升级表单 | 🟡 |
| 4 | 触发升级任务 | ASO/天基编排 | 🔴 |
| 5 | 监控升级进度与 Pod 重启 | ASO 变更中心 + kubectl | 🟡 |
| 6 | 升级验收 | kubectl 版本/Pod 健康 | 🟢 |

> 详细的 ACK 版本升级与飞天底座补丁流程见 [[18-云厂商/01-阿里云/专有云-Apsara/254-apsara-upgrade-patch-management.md|升级与补丁管理]]。

### 3.2 配置下发机制

天基的配置下发是「基线驱动」的：产品的期望配置定义为基线，天基持续比对实际状态与基线，发现漂移则下发纠正（这也是自愈的基础）。

| 配置类型 | 下发方式 | 生效范围 |
|----------|----------|----------|
| Addon 版本/参数 | ASO 集群详情 > Addon 管理 | 单集群 |
| 节点池配置 | ASO 集群详情 > 节点池 | 节点池 |
| 底座产品参数 | 天基产品运维 > 配置 | 全集群 |
| 安全/证书 | 天基/ASO 安全模块 | 跨产品 |

```bash
# 🟢 低风险：只读
# 确认 Addon 当前版本与期望基线是否一致
kubectl get pods -n kube-system -o wide | grep -E 'terway|csi|cloud-controller'
# 检查节点池节点是否就绪
kubectl get nodes -o wide
```

---

## 4. 巡检与监控

### 4.1 天目（Tianmu）统一监控

天目是专有云的统一监控平台，采集物理机、底座组件与云产品指标。

| 监控层 | 采集方 | 关键指标 |
|--------|--------|----------|
| 物理机 | 天目 Agent | CPU、内存、磁盘、温度、电源 |
| 底座组件 | 天基采集 | 伏羲/洛神/盘古/女娲健康度 |
| 云产品 | POP 接口抓取 | SLB/RDS/ACK 健康状态 |
| K8s | Prometheus Agent | Pod/Node/资源水位 |

### 4.2 巡检清单

- [ ] **底座健康**：天基 `运维大盘` 伏羲/洛神/盘古/女娲/天基/ASO 全绿
- [ ] **ACK 集群**：`kubectl get nodes` 全 Ready；`kubectl get --raw=/livez,/readyz` 正常
- [ ] **Addon 一致性**：Addon Pod 版本与 ASO 显示基线一致
- [ ] **告警处理**：ASO `底座运维 > 告警中心 > 当前告警` 无未确认告警
- [ ] **变更审计**：ASO `底座运维 > 变更中心` 近期变更无异常
- [ ] **容量水位**：ASO 资源管理，CPU/内存/存储/ENI 配额未触顶

```bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 一键集群健康巡检（驻场或堡垒机执行）
kubectl get nodes -o wide
kubectl get --raw=/livez && kubectl get --raw=/readyz
kubectl get pods -A --field-selector status.phase!=Running
kubectl get events -A --sort-by='.lastTimestamp' | tail -30
# 底座/Addon Pod 健康
kubectl get pods -n kube-system | grep -vE 'Running|Completed'
```

---

## 5. 变更中心与自愈

### 5.1 变更中心

ASO 变更中心记录所有经 ASO/天基下发的变更任务，是排障与回溯的第一现场。

| 场景 | 变更中心查看要点 |
|------|------------------|
| 升级失败 | 找到对应升级任务，查看失败步骤与天基日志 |
| 控制台功能异常 | 查看近期是否有产品配置变更未完成 |
| 告警风暴 | 按时间窗筛选变更，定位触发变更 |
| 配置不生效 | 确认变更任务状态是否「成功」，而非「进行中」 |

### 5.2 天基自愈机制

天基基于基线比对执行自愈，常见自愈行为：

| 异常 | 自愈动作 | 影响 |
|------|----------|------|
| 管控面 Pod 挂掉 | 天基重新拉起 | 短暂控制面抖动 |
| 节点失联 | 天基/伏羲迁移或重建 | Pod 重调度 |
| 配置漂移 | 天基下发纠正配置 | 无感 |
| 证书过期 | 天基续期（如配置自动续期） | 可能短暂 API 抖动 |

> ⚠️ **高危边界**：涉及盘古/女娲/伏羲/洛神核心集群的自愈失败，**禁止**客户自行重启，必须联系阿里云 TAM 或驻场工程师。

---

## 6. ASO 控制台操作路径速查

| 场景 | ASO 控制台路径 |
|------|----------------|
| ECS 实例 | `产品运维 > 计算 > ECS > 实例列表` |
| 神龙实例 | `产品运维 > 计算 > 神龙 > 实例健康` |
| ACK 集群 | `产品运维 > 容器服务 > ACK 集群 > 集群详情` |
| Addon 管理 | `产品运维 > 容器服务 > ACK 集群 > 集群详情 > 组件管理` |
| 节点池 | `产品运维 > 容器服务 > ACK 集群 > 节点池` |
| 集群升级 | `产品运维 > 容器服务 > ACK 集群 > 升级` |
| VPC/VSwitch | `产品运维 > 网络 > VPC` |
| SLB | `产品运维 > 网络 > SLB > 实例监听` |
| EIP | `产品运维 > 网络 > EIP` |
| 云盘 | `产品运维 > 存储 > 云盘 > 磁盘列表` |
| NAS | `产品运维 > 存储 > NAS > 文件系统` |
| OSS | `产品运维 > 存储 > OSS` |
| RAM 角色 | `产品运维 > 安全 > RAM > 角色` |
| RRSA | `产品运维 > 容器服务 > ACK 集群 > 集群认证 > RRSA` |
| KMS | `产品运维 > 安全 > KMS` |
| ActionTrail | `产品运维 > 安全 > ActionTrail` |
| 云安全中心 | `产品运维 > 安全 > 云安全中心` |
| ARMS | `产品运维 > 中间件与监控 > ARMS` |
| SLS | `产品运维 > 大数据与日志 > SLS` |
| Prometheus | `产品运维 > 中间件与监控 > Prometheus` |
| 告警 | `底座运维 > 告警中心 > 当前告警` |
| 变更任务 | `底座运维 > 变更中心 > 变更列表` |
| 天基服务 | `产品运维 > 天基 > 集群 > 服务实例` |
| 盘古 | `底座运维 > 存储 > 盘古集群` |
| 女娲 | `底座运维 > 一致性服务 > 女娲` |
| 伏羲 | `底座运维 > 调度 > 伏羲` |
| 资源配额 | `资源管理 > 配额管理` |

---

## 7. 常见问题与排障

### 7.1 控制台无法登录/操作无响应

| 可能原因 | 排查 | 处理 |
|----------|------|------|
| 天基/ASO 自身服务异常 | 浏览器访问 ASO 健康检查 API | 联系驻场工程师 |
| 女娲一致性异常 | 天基 `运维大盘 > 一致性服务` | 联系 TAM，禁止自行重启 |
| 配置下发卡住 | ASO 变更中心是否有「进行中」任务 | 等待或终止后重试 |

### 7.2 部署/升级任务卡住

```bash
# 🟢 低风险：只读
# 1. ASO 变更中心定位卡住的任务与步骤
# 2. 天基查看对应组件部署日志
kubectl get pods -n tianji-system
# 3. 确认底层依赖（伏羲资源池、洛神网络、盘古存储）是否健康
# 4. 若底层异常 → 联系 TAM/驻场；若 ACK 层 → 查 kubectl 事件
kubectl get events -A --sort-by='.lastTimestamp' | tail -50
```

### 7.3 何时必须联系阿里云 TAM / 驻场工程师

| 操作类型 | 建议处理方 | 说明 |
|----------|------------|------|
| 天基/ASO 自身升级或重启 | 驻场工程师 | 底座核心，禁止客户自行操作 |
| 盘古/女娲/伏羲/洛神集群变更 | 驻场工程师 | 影响范围大，需专家评估 |
| ACK 版本升级失败回滚 | TAM + 客户窗口 | 需评估业务影响 |
| 全局网络/存储中断 | TAM 立即升级 | 通常触发 P0 响应 |
| ASO 控制台无法登录且影响排障 | 驻场工程师 | 先通过天基 CLI 或后台恢复 |

---

## 8. 排障检查清单

- [ ] **确认症状层级**：单 Pod/节点/命名空间，还是全集群/底座？
- [ ] **ASO 告警中心**：是否有伏羲/洛神/盘古/女娲/天基/ASO 告警
- [ ] **ASO 变更中心**：近期是否有未完成/失败的变更任务
- [ ] **天基运维大盘**：底座组件健康度是否全绿
- [ ] **ACK 集群状态**：`kubectl get nodes`、`kubectl get --raw=/livez,/readyz`
- [ ] **区分 K8s 层与底座层**：ACK 组件日志说明症状，底座组件状态决定根因
- [ ] **评估升级路径**：涉及天基/ASO/盘古/女娲/伏羲/洛神 → 必须 TAM/驻场
- [ ] **记录关键证据**：保存 `kubectl cluster-info dump`、ASO/天基截图、aliyun CLI 输出

---

## 相关文档

- [[18-云厂商/01-阿里云/01-专有云架构概述.md|01 专有云架构概述]]
- [[18-云厂商/01-阿里云/apsara-stack-components.md|Apsara Stack 组件索引]]
- [[18-云厂商/01-阿里云/专有云-Apsara/254-apsara-upgrade-patch-management.md|254 升级与补丁管理]]
- [[18-云厂商/01-阿里云/专有云-Apsara/255-apsara-compliance-hardening.md|255 合规加固（等保/国密）]]
- [[18-云厂商/01-阿里云/专有云-Apsara/256-apsara-pangu-storage-troubleshooting.md|256 盘古存储排障]]
- [[18-云厂商/01-阿里云/专有云-Apsara/99-apsara-stack-troubleshooting-runbook.md|99 专有云故障手册]]
- [[18-云厂商/01-阿里云/专有云-Apsara/252-apsara-stack-pop-operations.md|252 POP 平台运维（ASOP）]]

## Related

- [[23-实体/02-K8s核心组件/kubernetes.md|Kubernetes]]
- [[23-实体/02-K8s核心组件/etcd.md|etcd]]
- [[17-系统基础/06-知识字典/networking/ingress.md|Ingress]]

<!-- risk-assessed -->
