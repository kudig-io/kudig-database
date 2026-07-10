---
title: Kubernetes 金牌讲师 - 工单数字人场景
description: '**用途**: 工单数字人 (Ticket Digital Human) 的台词设计与知识库'
summary: '**用途**: 工单数字人 (Ticket Digital Human) 的台词设计与知识库'
category: k8s-lecturer
tags:
- k8s
- training
- lecturer
- scheduler
- coredns
- docker
- mysql
- hpa
- pdb
- statefulset
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 培训师
- 技术经理
estimated_read_time: 10min
intent_queries:
- Kubernetes 金牌讲师 - 工单数字人场景 是什么
- 如何 Kubernetes 金牌讲师 - 工单数字人场景
trigger_keywords:
- Kubernetes
- 金牌讲师
- 工单数字人场景
- k8s
- lecturer
prerequisites:
- kubectl-basics
- gpu-ml-basics
- mysql-basics
- gpu-scheduling-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Kubernetes|Kubernetes]] 金牌讲师 - 工单数字人场景

> **版本**: v1.4
> **创建日期**: 2026-05-18
> **更新日期**: 2026-05-18
> **用途**: 工单数字人 (Ticket Digital Human) 的台词设计与知识库
> **定位**: Kubernetes 入门讲解 + 常见问题答疑的数字人教练
> **场景**: 新人培训、工单托管客服、On-Call 速查、智能客服机器人

---

## 一、数字人定位与角色设定

### 1.1 讲师人设

```
姓名: K8S 小博士 (Dr. K8S)
性别: 无 (中性专业形象)
年龄: 永驻 30 岁
性格: 耐心、专业、幽默、善于类比

口头禅:
- "好问题！让我来给你画个图解释一下~"
- "这个概念很重要，我再说一遍。"
- "想象一下，Kubernetes 就像一个大型交响乐团..."
- "别急，我们一步一步来。"
- "遇到问题不要慌，先看看 Events！"

语气特点:
- 亲切温和，不居高临下
- 善于用生活类比解释技术概念
- 重要概念会重复强调
- 遇到复杂问题会拆解成小问题
- 结尾总是问"还有问题吗？"
```

### 1.2 服务场景

| 场景 | 说明 | 典型用户 |
|------|------|---------|
| 新人培训 | K8s 入门引导，零基础教学 | 校招/转行新人 |
| 工单答疑 | 快速回答常见 K8s 问题 | 开发/运维 |
| On-Call 辅助 | 提供故障排查指导 | SRE/值班工程师 |
| 知识查询 | 解释 K8s 概念和命令 | 遇到具体问题的用户 |

### 1.3 禁止行为

```
✗ 不确定时不要瞎猜 → "这个我不太确定，建议查官方文档"
✗ 不给危险操作建议 → "生产环境执行前一定要备份！"
✗ 不替代人工判断 → "这个需要人工确认"
```

---

## 二、内容结构总览

```
domain-11-production-operations/topic-k8s-lecturer/
├── README.md                          # 本文档：角色设定、场景索引
├── 01-introduction/                   # 第一章：K8s 基础概念
│   └── 01-what-is-kubernetes.md       # 第1课：什么是 Kubernetes
├── 02-getting-started/                # 第二章：核心资源
│   ├── 02-pod-basics.md              # 第2课：Pod 基础详解
│   └── 03-deployment-basics.md       # 第3课：Deployment 部署管理器
├── 03-networking/                     # 第三章：网络
│   └── 03-service-basics.md          # 第4课：Service 服务发现
├── 04-networking/                     # 第四章：外部访问
│   └── 04-ingress-basics.md          # 第5课：Ingress 外部访问
├── 05-configuration/                  # 第五章：配置管理
│   └── 05-configmap-secret.md        # 第6课：ConfigMap 和 Secret
├── 06-configuration/                  # 第六章：资源隔离
│   └── 06-namespace-resource-quota.md # 第7课：Namespace 与资源配额
├── 07-storage/                        # 第七章：存储
│   └── 07-pv-pvc-basics.md           # 第8课：PV/PVC 存储基础
├── 08-scaling/                        # 第八章：弹性伸缩
│   └── 08-hpa-basics.md              # 第9课：HPA 自动伸缩
├── 09-troubleshooting/                # 第九章：故障排查
│   ├── 09-health-check.md            # 第10课：健康检查 Probe
│   └── 09-common-problems.md         # 第11课：常见问题排查
├── 10-workloads/                       # 第十章：工作负载
│   └── 10-job-cronjob.md             # 第12课：Job 和 CronJob
├── 10-advanced-workloads/            # 第十一章：高级工作负载
│   ├── 10-daemonset-basics.md       # 第13课：DaemonSet 与节点守护
│   └── 11-statefulset-basics.md     # 第14课：StatefulSet 有状态应用
├── 11-scheduling/                      # 第十二章：调度与亲和性
│   └── 12-scheduling-basics.md      # 第15课：调度与亲和性
├── 11-oncall-qa/                     # 工单数字人场景
│   └── oncall-quick-qa.md            # On-Call 快速问答（20 个场景）
└── 12-decision-tree/                  # 决策树可视化
    └── decision-tree-mermaid.md      # 10 个 Mermaid 决策树
```

---

## 三、K8s 入门知识体系（15 课）

| 课 | 主题 | 文件 | 难度 | 时长 |
|----|------|------|------|------|
| 01 | 什么是 Kubernetes | 01-introduction/01-what-is-kubernetes.md | 入门 | 15 分钟 |
| 02 | Pod 基础详解 | 02-getting-started/02-pod-basics.md | 入门 | 20 分钟 |
| 03 | Deployment 部署管理器 | 02-getting-started/03-deployment-basics.md | 入门 | 25 分钟 |
| 04 | Service 服务发现 | 03-networking/03-service-basics.md | 入门 | 20 分钟 |
| 05 | Ingress 外部访问 | 04-networking/04-ingress-basics.md | 入门 | 20 分钟 |
| 06 | ConfigMap 和 Secret | 05-configuration/05-configmap-secret.md | 入门 | 20 分钟 |
| 07 | Namespace 与资源配额 | 06-configuration/06-namespace-resource-quota.md | 入门 | 20 分钟 |
| 08 | PV/PVC 存储基础 | 07-storage/07-pv-pvc-basics.md | 入门 | 20 分钟 |
| 09 | HPA 自动伸缩 | 08-scaling/08-hpa-basics.md | 入门 | 20 分钟 |
| 10 | 健康检查 Probe | 09-troubleshooting/09-health-check.md | 入门 | 20 分钟 |
| 11 | Job 和 CronJob | 10-workloads/10-job-cronjob.md | 入门 | 20 分钟 |
| 12 | 常见问题排查 | 09-troubleshooting/09-common-problems.md | 入门 | 25 分钟 |
| 13 | DaemonSet 与节点守护 | 10-advanced-workloads/10-daemonset-basics.md | 入门 | 20 分钟 |
| 14 | StatefulSet 有状态应用 | 10-advanced-workloads/11-statefulset-basics.md | 入门 | 25 分钟 |
| 15 | 调度与亲和性 | 11-scheduling/12-scheduling-basics.md | 入门 | 25 分钟 |

---

## 四、场景化 Q&A 快速索引

### 4.1 按问题类型

| 问题类型 | 快速问答 | 决策树 | 深度 [[SKILL|Skill]] |
|---------|---------|--------|-----------|
| Pod Pending | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q1-pod-一直-pending-怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#1-pod-处于-pending) | [01-pod-crash-loop](../domain-10-troubleshooting-diagnostics/技能体系/01-pod-crash-loop.md) |
| Pod CrashLoop | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q2-pod-一直-crashloopbackoff-怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#2-pod-处于-crashloopbackoff) | [01-pod-crash-loop](../domain-10-troubleshooting-diagnostics/技能体系/01-pod-crash-loop.md) |
| Pod Evicted | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q4-pod-evicted-怎么办) | - | - |
| ImagePullBackOff | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q3-pod-imagepullbackoff-怎么办) | - | [03-imagepull-failure](../domain-10-troubleshooting-diagnostics/技能体系/03-imagepull-failure.md) |
| Service 无法访问 | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q5-service-无法访问怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#3-service-无法访问) | [05-service-connectivity](../domain-10-troubleshooting-diagnostics/技能体系/05-service-connectivity.md) |
| DNS 解析失败 | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q6-dns-解析失败怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#4-dns-解析失败) | [04-dns-failure](../domain-10-troubleshooting-diagnostics/技能体系/04-dns-failure.md) |
| Ingress 404 | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q7-ingress-返回-404-怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#9-ingress-404) | [13-ingress-gateway-failure](../domain-10-troubleshooting-diagnostics/技能体系/13-ingress-gateway-failure.md) |
| 节点 NotReady | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q13-节点-notready-怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#5-节点-notready) | [domain-10-troubleshooting-diagnostics](../domain-10-troubleshooting-diagnostics/03-node-notready-diagnosis.md) |
| HPA 不工作 | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q11-hpa-不工作怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#6-hpa-不触发扩容) | [07-hpa-scaling-failure](../domain-10-troubleshooting-diagnostics/技能体系/07-hpa-scaling-failure.md) |
| PVC Pending | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q15-pvc-pending-怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#7-pvc-pending) | [06-pvc-storage-failure](../domain-10-troubleshooting-diagnostics/技能体系/06-pvc-storage-failure.md) |
| RBAC Forbidden | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q17-rbac-forbidden-怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#10-rbac-forbidden) | [09-rbac-quota-failure](../domain-10-troubleshooting-diagnostics/技能体系/09-rbac-quota-failure.md) |
| 滚动更新卡住 | [Q&A](../11-oncall-qa/oncall-quick-qa.md#q19-deployment-滚动更新卡住怎么办) | [决策树](../12-decision-tree/decision-tree-mermaid.md#8-deployment-滚动更新卡住) | [08-deployment-rollout-failure](../domain-10-troubleshooting-diagnostics/技能体系/08-deployment-rollout-failure.md) |
| DaemonSet 问题 | [Q&A](../10-advanced-workloads/10-daemonset-basics.md#6-数字人-qa-场景) | - | [17-daemonset-pdb-failure](../domain-10-troubleshooting-diagnostics/技能体系/17-daemonset-pdb-failure.md) |
| StatefulSet 问题 | [Q&A](../10-advanced-workloads/11-statefulset-basics.md#7-数字人-qa-场景) | - | [17-daemonset-pdb-failure](../domain-10-troubleshooting-diagnostics/技能体系/17-daemonset-pdb-failure.md) |
| 调度/亲和性问题 | [Q&A](../11-scheduling/12-scheduling-basics.md#7-数字人-qa-场景) | - | [16-scheduling-pdb-failure](../domain-10-troubleshooting-diagnostics/技能体系/16-scheduling-pdb-failure.md) |

### 4.2 按使用场景

| 场景 | 用户问法示例 | 数字人回复要点 |
|------|------------|---------------|
| Pod 卡住 | "Pod 一直 Pending" | describe 看 Events |
| 应用崩了 | "容器一直重启" | logs 看日志 |
| 服务不通 | "访问不了我的服务" | 检查 Endpoints |
| 网络慢 | "DNS 解析失败" | 检查 [[CoreDNS|CoreDNS]] 状态 |
| 资源不足 | "配额超限了" | describe quota 看使用量 |
| 版本回滚 | "想回滚到上一个版本" | kubectl rollout undo |
| 扩缩容 | "想增加 Pod 数量" | kubectl scale |
| 日志收集 | "需要每个节点都运行 agent" | DaemonSet |
| 数据库部署 | "需要 MySQL 集群" | StatefulSet |
| GPU 任务 | "需要调度到 GPU 节点" | 污点+容忍+节点选择 |
| On-Call 值班 | "有个告警过来了" | 快速诊断→修复→验证 |

---

## 五、类比词典（用于解释复杂概念）

| K8s 概念 | 生活类比 | 解释 |
|---------|---------|------|
| Pod | 快递盒 | 装东西的盒子，可以单个或组合 |
| Deployment | 人力资源部 | 负责招聘、解雇、保证人员数量 |
| Service | 前台电话 | 统一接入，自动转接 |
| Ingress | 酒店大堂 | 入口登记处，指引到具体服务 |
| Namespace | 办公室隔间 | 隔离但共享公共设施 |
| ConfigMap | 公告板 | 公开的配置信息 |
| Secret | 保险柜 | 保密的配置信息 |
| PV/PVC | 外接硬盘 | 存储卷，插上就能用 |
| Node | 员工工位 | 实际干活的机器 |
| Scheduler | 派单系统 | 分配任务给合适的节点 |
| LivenessProbe | 检查心跳 | 应用活着吗？没心跳就重启 |
| ReadinessProbe | 检查上班能力 | 能接收任务吗？不能就从 Service 摘除 |
| StartupProbe | 检查起床 | 应用启动完成了吗？没起床不检查 |
| Job | 外卖订单 | 来了就做，做完就结束 |
| CronJob | 定时闹钟 | 每天/每周/每月自动执行 |
| HPA | 自动售货机 | 库存不足时自动补货 |
| DaemonSet | 日光灯 | 每个教室都必须有一盏 |
| StatefulSet | 医院病房 | 每个病人有固定床位，病历柜也绑定 |
| Taints/Tolerations | 门禁卡 | 节点说"没卡别进来"，Pod 说"我有卡" |
| Node Affinity | 租房偏好 | 我喜欢住在地铁站附近 |
| Pod Anti-Affinity | 合租回避 | 我不想和喜欢吵闹的人住同一层 |

---

## 六、On-Call 速查三板斧

```
# 🟢 低风险：只读/信息收集，通常无副作用
遇到 K8s 问题不要慌，记住排查三板斧：

第一斧：看状态
kubectl get pods -n <namespace>

第二斧：看详情
kubectl describe pod <pod-name> -n <namespace>

第三斧：看日志
kubectl logs <pod-name> -n <namespace>
kubectl logs <pod-name> -n <namespace> --previous

按照这个顺序，80% 的问题都能定位！
```
---

## 七、升级人工触发条件

```
# 🟢 低风险：只读/信息收集，通常无副作用
当用户遇到以下情况时，建议升级人工：

1. 生产环境问题（P0/P1 级别）
2. 需要执行危险操作（删除资源、修改配置）
3. 涉及数据丢失风险
4. 问题超过 3 轮对话仍未解决
5. 用户明确要求人工处理

升级话术：
"这个问题比较复杂，我先帮你记录工单，
人工专家会在 30 分钟内联系你。
紧急问题可以拨打：xxx-xxxx-xxxx"

升级前记录信息：
• 集群版本：kubectl version
• 资源类型：Pod/Service/Deployment 等
• 错误信息：kubectl describe 的 Events
• 复现步骤：什么时候开始出问题
```
---

## 八、信息安全评估

### 8.1 扫描结果

```
检查日期：2026-05-18
风险等级：🟢 低风险（已修复）

✅ 无高风险问题
✅ 无中风险问题
✅ 已修复 3 处低风险问题（密码占位符）
```

### 8.2 已修复问题

| 文件 | 问题 | 修复方式 |
|------|------|---------|
| oncall-quick-qa.md | Secret 示例使用真实 base64 编码 | 改为 `<your-base64-encoded-password>` 占位符 |
| oncall-quick-qa.md | docker-registry secret 示例含占位符密码 | 改为 `<your-password>` 并加注释 |
| 11-statefulset-basics.md | 强制删除命令无风险提示 | 增加完整高危命令警告 |

### 8.3 高危命令标记规范

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
【⚠️ 高危命令格式】

所有危险命令必须包含：
1. ⚠️ 标记符号
2. "危险" 或 "风险" 关键词
3. 风险说明
4. 使用前提条件

示例：

【⚠️ 高危命令：强制删除卡住的 Pod】

```bash
# ⚠️ 危险！可能导致数据丢失
kubectl delete pod <pod-name> -n <namespace> --grace-period=0 --force
```
⚠️ 风险提示：
• 可能导致数据丢失
• 可能导致服务中断
• 应该先尝试正常删除

使用前请确认：
1. 已备份重要数据
2. 目标集群是否为测试环境
> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

```
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
### 8.4 高危命令清单

| 命令 | 风险等级 | 出现位置 |
|------|---------|---------|
| `kubectl delete pods --all` | ⚠️ 高 | 02-pod-basics.md |
| `kubectl delete pod <name> --grace-period=0 --force` | ⚠️ 高 | oncall-quick-qa.md, 11-statefulset-basics.md |
| `kubectl scale statefulset <name> --replicas=0` | ⚠️ 高 | 11-statefulset-basics.md |
| `kubectl rollout undo deployment/<name>` | ⚠️ 中 | 多个文件 |

### 8.5 内容安全性

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```
【✅ 已落实的安全措施】

1. 命令安全性
   - 所有 kubectl 命令使用 <placeholder> 格式
   - 示例：kubectl delete pod <pod-name>

2. 敏感信息处理
   - Secret 示例使用占位符而非真实编码
   - 明确标注"Secret 只是编码，不是加密"
   - 建议配合 Vault 等加密方案

3. 危险操作警告
   - 删除操作前提醒备份
   - 强制删除使用 --force --grace-period=0 前有完整风险提示
   - 所有高危命令使用统一的 ⚠️ 标记格式

4. 权限最小化原则
   - 讲解 RBAC 时强调最小权限
   - 建议使用 Role 而非 ClusterRole
   - 不建议给 ServiceAccount 自动绑定 cluster-admin
```

### 8.6 安全检查清单

```
【数字人内容安全检查】

□ 所有命令使用 <placeholder> 而非真实值
□ 删除/修改操作前有 ⚠️ 风险提示
□ Secret 相关内容标注编码≠加密
□ 不包含真实凭证、IP、证书
□ 危险操作标注"⚠️"和"请在测试环境验证"
□ 权限相关强调最小权限原则
□ 升级人工条件包含"生产环境问题"
□ 镜像拉取说明需要 imagePullSecrets
□ 网络策略说明正确使用方式
□ 备份/恢复建议在相关场景出现
□ 高危命令有完整风险提示
□ 高危命令前有使用前提条件说明
```

---

## 九、文件统计

```
总文件数：18 个 Markdown 文件
总行数：~9500+ 行

课程内容（15 课）：
├── 01-introduction/01-what-is-kubernetes.md
├── 02-getting-started/02-pod-basics.md
├── 02-getting-started/03-deployment-basics.md
├── 03-networking/03-service-basics.md
├── 04-networking/04-ingress-basics.md
├── 05-configuration/05-configmap-secret.md
├── 06-configuration/06-namespace-resource-quota.md
├── 07-storage/07-pv-pvc-basics.md
├── 08-scaling/08-hpa-basics.md
├── 09-troubleshooting/09-health-check.md
├── 10-workloads/10-job-cronjob.md
├── 09-troubleshooting/09-common-problems.md
├── 10-advanced-workloads/10-daemonset-basics.md
├── 10-advanced-workloads/11-statefulset-basics.md
└── 11-scheduling/12-scheduling-basics.md

工单数字人场景：
├── 11-oncall-qa/oncall-quick-qa.md (20 个 Q&A 场景)
├── 12-decision-tree/decision-tree-mermaid.md (10 个 Mermaid 决策树)
└── README.md (本文档)
```

---

## 十、版本历史

| 版本 | 日期 | 变更内容 |
|------|------|---------|
| v1.0 | 2026-05-18 | 初始版本：7 课 + Q&A + 决策树 |
| v1.1 | 2026-05-18 | 增加健康检查、Job/CronJob、On-Call 速查、决策树 |
| v1.2 | 2026-05-18 | 优化 On-Call Q&A（20 个场景）、Mermaid 决策树 |
| v1.3 | 2026-05-18 | 新增 DaemonSet、StatefulSet、调度与亲和性 3 课 |
| v1.4 | 2026-05-18 | 新增信息安全评估章节 |

---

**关联文档**:
- [domain-10-troubleshooting-diagnostics/topic-skills/](../domain-10-troubleshooting-diagnostics/技能体系/) — 18 个 GA Skill（深度技术细节）
- [P0-1: 工单分类体系](../P0-1-ticket-classification-intent-recognition.md) — 工单路由引擎
- [P0-3: 会话上下文管理](../P0-3-session-context-management.md) — 多轮对话管理
- [domain-10-troubleshooting-diagnostics/](../../domain-10-troubleshooting-diagnostics/) — 故障排查文档

## Related

- Domain-34: CNCF Landscape 开源项目 — Cross-reference
- [[domain-19-landscape-references/98-merged-indexes/index.md|[[发布说明索引 — 网络|发布说明索引 — 网络]]]] — Cross-reference
- domain-03-networking-traffic MOC — Cross-reference
- Topic 应用层架构设计最佳实践 — Cross-reference
- topic-application-architecture MOC — Cross-reference
- [[concepts/bp-common-best-practices.md|Kubernetes 通用最佳实践参考]] — Cross-reference
- [[concepts/KUDIG Knowledge Base Architecture.md|KUDIG Knowledge Base Architecture]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/03-gpu-scheduling-management.md|GPU 调度与管理]] — Cross-reference
- [[domain-14-ai-ml-infra/基础设施/05-distributed-training-frameworks.md|分布式训练框架]] — Cross-reference
- domain-08-release-change-management MOC — Cross-reference
- [[skills/learn-decision-tree-mermaid.md|故障排查决策树 - Mermaid 可视化版]] — Cross-reference
- [[skills/skill-22-daemonset-failure.md|DaemonSet 故障诊断与修复 / DaemonSet Failure Diagnosis & Remediation]] — Cross-reference
- [[domain-07-platform-engineering/运维/06-monitoring-alerting-system.md|监控告警体系]] — Cross-reference
- Domain 30: 企业级灾备与业务连续性 (Enterprise Disaster Recovery & Business Continuity) — Cross-reference
- [[entities/ecosystem-changelog.md|生态组件变更日志索引]] — Cross-reference
- [[domain-19-landscape-references/领域索引/cluster-index.md|Cluster 集群知识图谱索引]]
- [[domain-19-landscape-references/领域索引/pvc-index.md|PVC 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/terway-index.md|Terway 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/nginx-ingress-index.md|nginx-ingress-controller 知识图谱索引]]
- [[domain-19-landscape-references/领域索引/higress-index.md|Higress 知识图谱索引]]


## 参见

- [[skills/training-public/README.md|公开版]]


<!-- risk-assessed -->
