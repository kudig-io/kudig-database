---
title: 第五章：FTA 构建完整流程
description: '**所属部分**: 第二部分 - FTA 构建实践指南'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- etcd
- kubelet
- scheduler
- coredns
- statefulset
- daemonset
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- 第五章：FTA 构建完整流程 是什么
- 如何 第五章：FTA 构建完整流程
- 第五章：FTA 构建完整流程 根因分析
- 第五章：FTA 构建完整流程 故障树
trigger_keywords:
- 第五章：FTA
- 构建完整流程
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# 第五章：FTA 构建完整流程

> **所属部分**: 第二部分 - FTA 构建实践指南  
> **关联主文档**: [FTA 方法论与 AI Agent 智能运维实践](./fta-methodology-and-agentic-practices.md)  
> **上一章**: [第四章：FTA 方法论核心原则](./04-fta-core-principles.md)  
> **下一章**: [第六章：FTA 验证与质量保证](./06-fta-verification-and-quality.md)

---

## 5.1 总体流程概览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         FTA 构建五阶段流程                                   │
└─────────────────────────────────────────────────────────────────────────────┘

阶段1              阶段2              阶段3              阶段4              阶段5
系统定义            故障模式识别        故障树构建          定性/定量分析       验证与优化
(20%)              (30%)              (30%)              (15%)              (5%)
                                                                           
┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐   ┌──────────┐
│ 明确边界  │   │ FMEA分析  │   │ 顶事件   │   │ 最小割集  │   │ 专家评审  │
│ 定义范围  │──►│ 历史数据  │──►│ 中间事件  │──►│ 概率计算  │──►│ 问题回溯  │
│ 确定深度  │   │ 架构分析  │   │ 底事件   │   │ 重要度   │   │ 混沌验证  │
└──────────┘   └──────────┘   │ 逻辑门   │   │ RPN排序  │   │ 迭代优化  │
                              └──────────┘   └──────────┘   └──────────┘
                                                                           
产出物:            产出物:            产出物:            产出物:            产出物:
- 系统边界文档     - 故障模式清单     - 完整故障树       - 最小割集列表     - 评审报告
- 顶事件定义       - 历史问题统计     - 事件编号表       - 概率矩阵         - 优化记录
- 分析深度规格     - 架构依赖图       - 逻辑关系表       - 重要度排序       - 版本变更日志
```

## 5.2 阶段一：系统定义

系统定义是 FTA 构建的基础，决定了分析的范围和深度。

**步骤 1：明确系统边界**

```yaml
# 系统边界定义模板
system_boundary:
  name: "[[23-实体/02-K8s核心组件/kubernetes|kubernetes]] 生产集群"
  version: "v1.28"
  scope:
    included:
      - 控制平面组件 (API Server, etcd, Scheduler, Controller Manager)
      - 工作节点组件 (Kubelet, Container Runtime, kube-proxy)
      - 核心插件 (CoreDNS, CNI, CSI, Ingress Controller)
      - 工作负载 (Deployment, StatefulSet, DaemonSet, Job)
    excluded:
      - 底层硬件问题 (由基础设施团队负责)
      - 应用代码层面的 bug (由开发团队负责)
      - 云厂商底层问题 (如 AWS us-east-1 区域问题)
  interfaces:
    upstream:
      - 用户请求 (kubectl, API 调用)
      - CI/CD Pipeline (GitOps 推送)
    downstream:
      - 云厂商 IaaS (ELB, EBS, VPC)
      - 外部服务 (镜像仓库, 日志平台, 监控平台)
```

**步骤 2：定义顶事件**

顶事件的选择直接影响 FTA 的价值。好的顶事件应当满足：

| 要求 | 说明 | 示例 |
|------|------|------|
| **与 SLO 直接关联** | 顶事件发生 = SLO 违约 | "API 响应成功率 < 99.9%" |
| **可明确判定** | 有清晰的判定条件 | "kubectl 命令超时 > 30s" |
| **影响范围明确** | 知道影响了谁、影响到什么程度 | "所有用户无法访问应用" |
| **严重程度已分级** | 每个顶事件有 P0-P3 分级 | TE-1: P0, TE-6: P2 |

参考本知识库 [kubernetes-fta-full-analysis.md](./kubernetes-fta-full-analysis.md) 定义的 8 个顶事件：

| 编号 | 顶事件 | 严重程度 | SLO 映射 |
|------|--------|---------|---------|
| TE-1 | 集群完全不可用 | P0 | 集群可用性 < 100% |
| TE-2 | 应用服务不可用 | P0 | 服务可用性 SLO 违约 |
| TE-3 | Pod 启动失败 | P1 | 部署成功率 SLO 违约 |
| TE-4 | 网络通信异常 | P1 | 网络延迟/丢包 SLO 违约 |
| TE-5 | 存储访问失败 | P1 | 存储 IOPS/延迟 SLO 违约 |
| TE-6 | 资源调度异常 | P2 | 调度延迟 SLO 违约 |
| TE-7 | 安全认证失败 | P1 | 安全合规 SLO 违约 |
| TE-8 | 监控告警异常 | P2 | 可观测性 SLO 违约 |

**步骤 3：确定分析深度**

```
深度决策矩阵:

                        低可观测性          高可观测性
                   ┌──────────────────┬──────────────────┐
  高业务影响       │  深度分析(5层)    │  标准分析(4层)    │
  (P0/P1)         │  投入更多监控建设  │  充分利用现有监控  │
                   ├──────────────────┼──────────────────┤
  低业务影响       │  标准分析(3层)    │  轻量分析(3层)    │
  (P2/P3)         │  按需逐步深化     │  维持现状即可      │
                   └──────────────────┴──────────────────┘

停止分解的条件(满足任一即可):
  1. 该事件已有明确的监控指标和告警
  2. 该事件有成熟的修复 Runbook
  3. 该事件超出了团队的技术管控范围
  4. 进一步分解的收益不足以覆盖维护成本
```

## 5.3 阶段二：故障模式识别

**方法 1：FMEA 协同分析**

```
对每个 Kubernetes 组件执行 FMEA:

组件: etcd
┌──────┬──────────────┬──────────┬───────────────┬─────┬─────┬─────┬──────┐
│ 编号 │ 故障模式      │ 影响      │ 原因          │ S   │ O   │ D   │ RPN  │
├──────┼──────────────┼──────────┼───────────────┼─────┼─────┼─────┼──────┤
│ F-01 │ 磁盘空间耗尽  │ 数据无法写│ 日志/快照积累  │ 9   │ 5   │ 3   │ 135  │
│ F-02 │ 仲裁丢失      │ 集群只读  │ 网络分区       │ 10  │ 3   │ 4   │ 120  │
│ F-03 │ 数据损坏      │ 数据丢失  │ 磁盘问题       │ 10  │ 2   │ 6   │ 120  │
│ F-04 │ 响应延迟高    │ API Server│ 负载过高       │ 7   │ 6   │ 2   │  84  │
│ F-05 │ 证书过期      │ 连接拒绝  │ 证书管理疏忽   │ 9   │ 4   │ 2   │  72  │
│ F-06 │ 版本不兼容    │ 通信失败  │ 升级操作错误   │ 8   │ 2   │ 5   │  80  │
└──────┴──────────────┴──────────┴───────────────┴─────┴─────┴─────┴──────┘

S: 严重度(1-10), O: 发生频率(1-10), D: 可检测性(1-10)
RPN = S × O × D (>100 需重点关注)
```

**方法 2：历史问题数据挖掘**

```bash
# 从工单系统导出历史问题数据
# 按问题类别统计分布

Kubernetes 生产问题统计 (基于行业数据):
┌───────────────────────┬──────┬──────┬──────────────────────────┐
│ 问题类别               │ 占比  │ MTTR │ 典型根因                  │
├───────────────────────┼──────┼──────┼──────────────────────────┤
│ 应用配置错误           │ 35%  │ 45m  │ YAML错误、资源限制不当     │
│ 资源不足/耗尽          │ 22%  │ 30m  │ 内存泄漏、CPU突增、磁盘满  │
│ 网络问题              │ 18%  │ 60m  │ DNS问题、CNI异常、策略错误  │
│ 控制平面问题           │ 10%  │ 90m  │ etcd问题、API Server过载   │
│ 存储问题              │  8%  │ 75m  │ PVC绑定、CSI驱动问题       │
│ 安全/认证问题          │  5%  │ 40m  │ 证书过期、RBAC配置错误     │
│ 其他                  │  2%  │ 50m  │ 升级失败、硬件问题         │
└───────────────────────┴──────┴──────┴──────────────────────────┘

洞察:
  - 应用配置错误占比最高(35%)，应在 FTA 中重点展开
  - 控制平面问题虽然占比低(10%)，但 MTTR 最长(90m)，需要最深入的 FTA
  - 网络问题 MTTR 较长(60m)，说明诊断难度大，FTA 需要更多诊断分支
```

**方法 3：架构依赖分析**

```
Kubernetes 组件依赖图 → 问题传播路径:

用户请求 ──→ Ingress ──→ Service ──→ Pod
                │           │          │
                ▼           ▼          ▼
           Ingress      kube-proxy   Kubelet
           Controller   / iptables   / CRI
                │           │          │
                ▼           ▼          ▼
              CoreDNS      CNI      Container
                │           │       Runtime
                ▼           ▼          │
            API Server  ◄──────────────┘
                │
                ▼
              etcd

依赖分析规则:
  → 箭头方向 = 请求流向
  → 反向 = 问题传播方向
  
  etcd 问题 → 传播到 API Server → 传播到所有组件 → 集群不可用
  (单点问题扇出效应最大)
```

## 5.4 阶段三：故障树构建

**构建策略对比**：

| 策略 | 适用场景 | 优势 | 劣势 |
|------|---------|------|------|
| **按子系统分解** | 组件边界清晰的系统 | 与架构图对应，易理解 | 跨组件问题难以表达 |
| **按问题类型分解** | 运维团队按技能分工 | 与团队分工对应 | 可能有重叠 |
| **按影响范围分解** | SRE 关注 SLO | 与业务影响直接关联 | 同一根因可能出现多次 |

**推荐：混合策略**

```
第 1-2 层: 按影响范围分解 (对齐 SLO)
第 3 层: 按子系统分解 (对齐架构)
第 4 层: 按问题类型分解 (对齐运维团队)

示例:
  TE-2: 应用服务不可用          ← 影响范围(用户不可用)
  ├── IE-2.1 Pod运行异常         ← 子系统(工作负载)
  │   ├── BE-2.1 CrashLoopBackOff ← 问题类型(容器崩溃)
  │   ├── BE-2.2 ImagePullBackOff ← 问题类型(镜像拉取)
  │   ├── BE-2.3 OOMKilled        ← 问题类型(资源耗尽)
  │   └── BE-2.4 Evicted          ← 问题类型(节点驱逐)
  ├── IE-2.2 Service访问异常      ← 子系统(网络)
  │   └── ...
  └── IE-2.3 Ingress访问异常      ← 子系统(入口)
      └── ...
```

**底事件定义规范**：

每个底事件必须包含以下结构化信息（与 [kubernetes-fta-full-analysis.md](./kubernetes-fta-full-analysis.md) 格式一致）：

```yaml
bottom_event:
  id: "BE-2.3"
  name: "OOMKilled"
  description: "容器因内存使用超过 limits 被 Linux OOM Killer 终止"
  
  # 可观测性
  observable:
    metrics:
      - "container_memory_usage_bytes / container_spec_memory_limit_bytes > 0.95"
    logs:
      - "OOMKilled in container"
      - "Exit Code: 137"
    events:
      - "kubectl get events --field-selector reason=OOMKilling"
  
  # 可能的根因
  root_causes:
    - "应用内存泄漏"
    - "JVM 堆内存设置过大"
    - "资源 limits 设置过低"
    - "突发流量导致内存突增"
    - "sidecar 容器内存未计入"
  
  # 诊断命令
  diagnosis_commands:
    - "kubectl describe pod <pod> | grep -A5 'Last State'"
    - "kubectl top pod <pod> --containers"
    - "kubectl logs <pod> --previous"
    - "kubectl get events --field-selector involvedObject.name=<pod>"
  
  # 修复动作
  healing_actions:
    - id: "HA-2.3.1"
      description: "增加内存 limits"
      risk: "low"
      auto_healable: true
      command: |
        kubectl patch deployment <deploy> -p \
          '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","resources":{"limits":{"memory":"2Gi"}}}]}}}}'
    - id: "HA-2.3.2"
      description: "分析内存泄漏"
      risk: "none"
      auto_healable: false
      command: "需要开发团队介入，使用 pprof/heapdump 分析"
  
  # 概率数据
  probability:
    annual_rate: 0.05     # 年问题率 5%
    mttr_minutes: 15      # 平均修复时间
    auto_heal_rate: 0.70  # 自动修复成功率
```

## 5.5 阶段四：定性/定量分析

**定性分析流程**：

```
1. 提取最小割集 (使用 MOCUS 算法)
2. 识别所有 1 阶割集 (单点问题)
3. 标记共因问题 (Common Cause Failure)
4. 评审逻辑门类型是否正确
5. 输出: 最小割集列表 + 单点问题清单
```

**定量分析流程**：

```
1. 收集底事件概率数据
   来源: 厂商 MTBF 数据、历史问题统计、行业基准
   
2. 计算各层概率
   自底向上: 底事件 → 中间事件 → 顶事件
   
3. 计算重要度
   Fussell-Vesely + Birnbaum 双维度
   
4. 风险排序
   综合 RPN = 概率 × 影响 × 可检测性
   
5. 输出: 风险优先级矩阵 + 加固建议清单
```

## 5.6 阶段五：验证与优化

**三重验证法**：

| 验证方式 | 方法 | 目的 |
|---------|------|------|
| **专家评审** | 邀请 3-5 名领域专家交叉评审 | 检查逻辑正确性和完备性 |
| **问题回溯** | 用过去 12 个月的问题工单验证 FTA | 确认每个历史问题都能在 FTA 中找到路径 |
| **混沌注入** | 使用 Chaos Mesh / Litmus 注入问题 | 验证 FTA 预测是否与实际问题表现一致 |

**专家评审检查清单**：

```
□ 顶事件定义是否清晰、无歧义
□ 是否覆盖所有已知故障模式
□ 逻辑门类型选择是否正确 (OR vs AND)
□ 底事件是否满足可观测性原则
□ 底事件之间是否满足独立性原则
□ 底事件粒度是否适当 (不过粗、不过细)
□ 是否存在 1 阶最小割集 (单点问题)
□ 概率数据来源是否可靠
□ 修复动作是否可执行、可验证
□ 编号和命名是否符合规范
```

---

> **导航**: [<< 上一章 - FTA 方法论核心原则](./04-fta-core-principles.md) | [下一章 - FTA 验证与质量保证 >>](./06-fta-verification-and-quality.md)


<!-- risk-assessed -->
