---
title: 12 - 生产事故管理与应急手册
description: '# 12 - 生产事故管理与应急手册'
summary: '# 12 - 生产事故管理与应急手册'
category: dictionary
tags:
- k8s
- glossary
- terminology
- etcd
- apiserver
- scheduler
- controller-manager
- prometheus
- grafana
- helm
tier: supporting
created: 2026-05
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 1h
intent_queries:
- 生产事故管理与应急手册 是什么
- 如何 生产事故管理与应急手册
trigger_keywords:
- 生产事故管理与应急手册
- dictionary
prerequisites:
- kubectl-basics
- cloud-provider-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 12 - 生产事故管理与应急手册

> **适用版本**: [[Kubernetes|Kubernetes]] v1.25-v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler | **质量等级**: ⭐⭐⭐⭐⭐ 专家级

> **生产环境实战经验总结**: 基于万级节点集群运维经验，涵盖从事故分级到应急响应的全方位最佳实践

---

## 知识地图

| 属性 | 说明 |
|------|------|
| **文件角色** | 生产事故管理手册 — 从事故分级到应急响应的实战指南 |
| **适合读者** | 开发者(了解流程) → 运维(执行应急) → SRE Lead(建设体系) |
| **前置知识** | 01(运维实践)、02(故障分析)、16(排查剧本) |
| **关联文件** | 02(故障模式)、11(企业运维)、16(排查剧本) |

### 事故处理全流程

> 发现 → 分级 → 响应 → 定位 → 修复 → 恢复 → 复盘 → 改进

---

## 目录

- [1. 事故管理框架](#1-事故管理框架)
- [2. 事故分级标准](#2-事故分级标准)
- [3. 应急响应流程](#3-应急响应流程)
- [4. War Room 组织](#4-war-room-组织)
- [5. 通用应急手册](#5-通用应急手册)
- [6. 特定场景 Runbook](#6-特定场景-runbook)
- [7. 事后复盘机制](#7-事后复盘机制)
- [8. 持续改进实践](#8-持续改进实践)

---

## 1. 事故管理框架

> **🔰 初学者导读**: 事故管理不是"出了问题再想办法"，而是提前建立好的标准化流程。就像医院的急诊分诊系统——患者到达后，护士先评估严重程度，再分配到对应科室。

### 1.1 ITIL 事故管理流程

| 阶段 | 目标 | 关键活动 | 输出物 | SLA |
|------|------|----------|--------|-----|
| **事故识别** | 尽早发现异常 | 监控告警、用户报告、主动巡检 | 事故工单 | < 5分钟 |
| **事故记录** | 完整记录信息 | 创建事故单、分配优先级、初步分类 | 结构化事故记录 | < 2分钟 |
| **事故分类** | 确定影响范围 | 业务影响评估、技术影响评估 | 事故等级定义 | < 5分钟 |
| **事故调查诊断** | 定位根因 | 日志分析、指标检查、链路追踪 | 诊断报告 | 根据等级 |
| **事故解决恢复** | 恢复服务 | 执行修复方案、验证恢复效果 | 解决方案文档 | 根据等级 |
| **事故关闭** | 确认完全恢复 | 用户确认、监控验证、文档归档 | 关闭确认 | 24小时内 |

### 1.2 事故生命周期管理

#### 🔰 初学者理解

事故生命周期就像消防队接到火警后的标准流程：

1. **接到报警** (事故发现) - 消防中心接到119电话
2. **评估严重性** (事故分级) - 判断是普通火灾还是化工厂爆炸
3. **调度资源** (资源分配) - 决定派出1辆还是10辆消防车
4. **现场处理** (应急响应) - 消防员到达现场灭火
5. **确认扑灭** (验证恢复) - 确保没有复燃风险
6. **事后总结** (事后复盘) - 分析火灾原因，改进消防设施

**为什么需要标准化流程？**
- ❌ 没有流程：每次火灾都临时决策，慌乱无序
- ✅ 有流程：按照预案执行，高效有序

#### 🔧 工作原理

**事故生命周期各阶段详解**

```yaml
# 事故生命周期状态机
lifecycleStates:
  detected:
    triggers:
      - "监控告警触发"
      - "用户报告问题"
      - "系统巡检发现"
    output: "事故工单创建"
    sla: "< 5分钟识别"
    
  classified:
    inputs: "事故工单"
    activities:
      - "评估业务影响范围"
      - "评估技术严重程度"
      - "分配优先级(P0-P3)"
    output: "事故等级定义"
    sla: "< 5分钟分级"
    
  responding:
    inputs: "事故等级"
    activities:
      - "组建响应团队"
      - "启动War Room(P0/P1)"
      - "执行诊断流程"
    output: "根因诊断报告"
    sla: "根据等级(P0: 15分钟内开始响应)"
    
  resolving:
    inputs: "诊断报告"
    activities:
      - "执行修复方案"
      - "验证服务恢复"
      - "监控系统稳定性"
    output: "服务恢复确认"
    sla: "P0: 2小时内恢复"
    
  closed:
    inputs: "恢复确认"
    activities:
      - "通知所有相关方"
      - "事后复盘(Postmortem)"
      - "改进措施制定"
    output: "复盘报告 + 改进任务"
    sla: "48小时内完成复盘"
```

**状态转换条件**

```python
# 事故状态转换逻辑
class IncidentStateMachine:
    def __init__(self):
        self.state = "detected"
        self.severity = None
        self.resolved = False
        
    def classify(self, business_impact, technical_severity):
        """根据影响评估分级"""
        if business_impact == "critical" and technical_severity == "high":
            self.severity = "P0"
        elif business_impact == "high" or technical_severity == "high":
            self.severity = "P1"
        elif business_impact == "medium":
            self.severity = "P2"
        else:
            self.severity = "P3"
            
        self.state = "classified"
        return self.severity
        
    def start_response(self):
        """启动应急响应"""
        if self.state != "classified":
            raise Exception("事故未分级，无法启动响应")
            
        if self.severity in ["P0", "P1"]:
            # 启动War Room
            self.war_room_active = True
            
        self.state = "responding"
        
    def resolve(self, verification_passed):
        """标记事故解决"""
        if not verification_passed:
            # 验证未通过，返回诊断阶段
            return False
            
        self.resolved = True
        self.state = "closed"
        return True
```

#### 📝 最小示例

**完整的事故处理示例**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# incident-lifecycle-example.sh - 模拟一次完整的事故处理流程

# ========== 阶段1: 事故发现 ==========
echo "📡 [14:32:15] Prometheus触发告警: API Server响应超时"

# 自动创建事故工单
cat > /tmp/incident-001.yaml <<EOF
incidentId: INC-2026-001
detectedAt: 2026-02-10T14:32:15Z
source: Prometheus Alert
symptom: "kube-apiserver响应超时 > 5s"
status: detected
EOF

# ========== 阶段2: 事故分级 ==========
echo "🔍 [14:34:00] SRE开始评估影响范围"

# 检查影响范围
FAILED_REQUESTS=$(kubectl top apiserver | grep "request_duration" | awk '{print $3}')
AFFECTED_USERS=$(kubectl logs -n monitoring prometheus-0 | grep "user_request_failed" | wc -l)

# 分级决策
if [ $AFFECTED_USERS -gt 100 ]; then
  SEVERITY="P0"
  echo "⚠️ 判定为P0级事故: 影响用户数 > 100"
else
  SEVERITY="P1"
fi

# 更新事故工单
yq eval ".severity = \"$SEVERITY\"" -i /tmp/incident-001.yaml
yq eval ".status = \"classified\"" -i /tmp/incident-001.yaml

# ========== 阶段3: 应急响应 ==========
echo "🚨 [14:35:30] 启动P0应急响应"

# 发送紧急通知
curl -X POST https://hooks.slack.com/services/YOUR_WEBHOOK \
  -H 'Content-Type: application/json' \
  -d "{
    \"text\": \"🚨 P0事故: API Server响应超时\",
    \"attachments\": [{
      \"color\": \"danger\",
      \"fields\": [
        {\"title\": \"War Room\", \"value\": \"https://zoom.us/j/emergency\"}
      ]
    }]
  }"

# 组建响应团队
echo "👥 组建响应团队:"
echo "  - Incident Commander: Alice (SRE Lead)"
echo "  - Tech Lead: Bob (K8s专家)"
echo "  - Communications: Charlie (通信协调)"

# ========== 阶段4: 诊断与修复 ==========
echo "🔧 [14:40:00] 开始诊断"

# 快速检查etcd
ETCD_HEALTH=$(ETCDCTL_API=3 etcdctl endpoint health | grep "unhealthy" | wc -l)
if [ $ETCD_HEALTH -gt 0 ]; then
  echo "❌ 发现根因: etcd集群不健康"
  
  # 执行恢复操作
  echo "🔄 [14:50:00] 执行恢复: 重启etcd问题节点"
  # systemctl restart etcd  # 实际操作
fi

# ========== 阶段5: 验证恢复 ==========
echo "✅ [15:10:00] 验证服务恢复"

# 健康检查
kubectl get --raw='/healthz' && echo "API Server健康检查: 通过"

# 功能验证
kubectl run test-pod --image=nginx --restart=Never && \
  kubectl delete pod test-pod && \
  echo "功能验证: Pod创建/删除正常"

# 更新状态
yq eval ".status = \"resolved\"" -i /tmp/incident-001.yaml
yq eval ".resolvedAt = \"2026-02-10T15:10:00Z\"" -i /tmp/incident-001.yaml

# ========== 阶段6: 关闭事故 ==========
echo "🎉 [15:15:00] 事故已解决，关闭War Room"

# 通知恢复
curl -X POST https://hooks.slack.com/services/YOUR_WEBHOOK \
  -H 'Content-Type: application/json' \
  -d '{"text": "✅ 事故已恢复，服务正常"}'

# 安排复盘
echo "📅 安排事后复盘: 2026-02-11 10:00 AM"
cat > /tmp/postmortem-001.md <<EOF
# 事故复盘: INC-2026-001
- 发生时间: 2026-02-10 14:32
- 持续时长: 43分钟
- 根因: etcd节点磁盘IO饱和
- 改进措施: (待复盘会议讨论)
EOF

echo "✅ 事故生命周期完成"
```
#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 跳过分级，直接处理 | ✅ 先分级再响应 | 分级决定了响应速度和参与人员，P3问题不需要半夜叫醒所有人 |
| ❌ 边修复边找根因 | ✅ 先止血再查根因 | 紧急时刻优先恢复服务，根因留给复盘阶段 |
| ❌ 单人单打独斗 | ✅ 组建响应团队 | P0/P1级别需要多角色协同，避免单点问题 |
| ❌ 恢复后立即关闭工单 | ✅ 验证稳定性再关闭 | 避免"假恢复"，确保系统真正稳定 |
| ❌ 不做事后复盘 | ✅ 48小时内完成复盘 | 复盘是最有价值的学习机会，不复盘=白白浪费问题 |
| ❌ 复盘追责个人 | ✅ Blameless文化 | 追责导致隐瞒问题，Blameless才能真正改进系统 |

---

### 1.3 角色与职责

#### 🔰 初学者理解

事故响应团队的角色分工就像消防队的组织结构：

- **消防队长** (Incident Commander) - 统筹全局，决策指挥
- **灭火组** (Tech Lead) - 负责实际灭火操作
- **通信组** (Communications Lead) - 对外联络，协调资源
- **后勤组** (SRE/Ops) - 准备设备，记录过程

**为什么需要明确角色？**
- ❌ 没有分工：所有人都在灭火，没人通知外部，没人记录过程
- ✅ 有分工：各司其职，高效协同

#### 🔧 工作原理

**RACI矩阵 (Responsible, Accountable, Consulted, Informed)**

```yaml
# 事故响应RACI矩阵
raciMatrix:
  incident-detection:
    monitoring-system: R  # Responsible: 负责执行
    on-call-sre: A        # Accountable: 最终负责
    dev-team: I           # Informed: 知情
    
  incident-classification:
    on-call-sre: R,A
    incident-commander: C  # Consulted: 需要咨询
    management: I
    
  war-room-initiation:
    incident-commander: R,A
    tech-lead: R
    communications-lead: R
    sre-team: C
    dev-team: C
    
  technical-diagnosis:
    tech-lead: R,A
    sre-team: R
    dev-team: C
    incident-commander: I
    
  fix-implementation:
    sre-team: R
    tech-lead: A
    incident-commander: C
    
  customer-communication:
    communications-lead: R,A
    incident-commander: C
    product-owner: C
    
  postmortem:
    incident-commander: A
    all-participants: R
    management: I
```

**角色权限与责任**

```python
# 角色定义与权限
class IncidentRole:
    """事故响应角色基类"""
    def __init__(self, name, permissions):
        self.name = name
        self.permissions = permissions
        
class IncidentCommander(IncidentRole):
    """事故指挥官 - 最高决策者"""
    def __init__(self):
        super().__init__(
            name="Incident Commander",
            permissions=["调动任何资源", "最终决策权", "升级事故等级"]
        )
        
    def make_decision(self, options):
        """决策制定"""
        # 评估各选项风险和收益
        # 做出最终决策
        pass
        
    def escalate(self, reason):
        """升级事故"""
        if "无进展超过15分钟" in reason:
            self.notify_management()
            self.request_more_resources()
            
class TechLead(IncidentRole):
    """技术负责人 - 技术诊断与方案制定"""
    def __init__(self):
        super().__init__(
            name="Tech Lead",
            permissions=["修改生产配置", "执行运维命令", "代码热修复"]
        )
        
    def diagnose(self, symptoms):
        """技术诊断"""
        # 分析日志、指标、链路追踪
        # 定位根因
        return root_cause
        
    def propose_solution(self, root_cause):
        """提出解决方案"""
        solutions = [
            {"action": "重启服务", "risk": "low", "time": "5分钟"},
            {"action": "回滚版本", "risk": "medium", "time": "15分钟"},
            {"action": "扩容节点", "risk": "low", "time": "30分钟"}
        ]
        return solutions
        
class CommunicationsLead(IncidentRole):
    """通信协调员 - 对外沟通"""
    def __init__(self):
        super().__init__(
            name="Communications Lead",
            permissions=["发布状态页更新", "通知客户", "媒体沟通"]
        )
        
    def update_status_page(self, message, severity):
        """更新状态页"""
        status_page_api.post({
            "incident": {
                "name": "API Server Latency",
                "status": "investigating",
                "message": message,
                "severity": severity
            }
        })
        
    def notify_stakeholders(self, audience, message):
        """通知相关方"""
        if audience == "customers":
            self.send_email_to_customers(message)
        elif audience == "management":
            self.send_slack_to_management(message)
```

#### 📝 最小示例

**War Room角色协同示例**

> ⚠️ **🟠 高危操作** — 影响业务流量或节点状态，需变更工单+影响评估+计划回滚
> - `systemctl stop/restart`：停止/重启系统服务，影响节点上所有容器

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# war-room-collaboration.sh - 模拟War Room中的角色协同

# ========== 场景: P0事故 - API Server不可用 ==========

# === 14:35 - Incident Commander启动War Room ===
echo "👔 [IC - Alice] 启动War Room，所有人加入Zoom会议"
echo "👔 [IC - Alice] 当前状态: API Server完全不可用，影响所有用户"
echo "👔 [IC - Alice] @Tech-Lead Bob: 请开始技术诊断"
echo "👔 [IC - Alice] @Comms Charlie: 准备对外通告"

# === 14:37 - Tech Lead开始诊断 ===
echo ""
echo "🔧 [Tech Lead - Bob] 收到，开始诊断"

# 检查API Server
kubectl get --raw='/healthz' 2>&1 | grep -q "connection refused"
if [ $? -eq 0 ]; then
  echo "🔧 [Tech Lead - Bob] 确认: API Server进程无响应"
  
  # 检查etcd
  ETCD_STATUS=$(ETCDCTL_API=3 etcdctl endpoint health 2>&1)
  if echo "$ETCD_STATUS" | grep -q "unhealthy"; then
    echo "🔧 [Tech Lead - Bob] 根因定位: etcd集群不健康"
    echo "🔧 [Tech Lead - Bob] @IC: 建议方案 - 重启etcd问题节点，预计10分钟恢复"
  fi
fi

# === 14:40 - Incident Commander决策 ===
echo ""
echo "👔 [IC - Alice] 方案批准，@SRE团队 立即执行"
echo "👔 [IC - Alice] @Comms: 通知客户我们正在处理，预计10分钟恢复"

# === 14:41 - Communications Lead对外沟通 ===
echo ""
echo "📢 [Comms - Charlie] 正在更新状态页..."

# 发布状态更新
cat > /tmp/status-update.json <<EOF
{
  "incident": {
    "name": "API Service Disruption",
    "status": "investigating",
    "message": "我们正在处理API服务中断问题，预计10分钟内恢复。",
    "impact": "所有API请求失败",
    "updates": [
      {
        "time": "2026-02-10T14:41:00Z",
        "message": "技术团队已定位问题，正在执行修复"
      }
    ]
  }
}
EOF

echo "📢 [Comms - Charlie] 状态页已更新，客户已收到邮件通知"

# === 14:45 - SRE执行恢复 ===
echo ""
echo "⚙️ [SRE - Dave] 正在重启etcd节点..."
# systemctl restart etcd  # 实际操作
echo "⚙️ [SRE - Dave] etcd节点已重启，等待集群达成共识..."

# === 14:48 - Tech Lead验证恢复 ===
echo ""
echo "🔧 [Tech Lead - Bob] 验证恢复状态..."
kubectl get --raw='/healthz' && echo "✅ API Server已恢复"
kubectl get nodes | grep -q "Ready" && echo "✅ 节点状态正常"

echo "🔧 [Tech Lead - Bob] @IC: 服务已完全恢复，建议继续观察15分钟"

# === 14:50 - Incident Commander宣布恢复 ===
echo ""
echo "👔 [IC - Alice] 确认服务恢复，@Comms 发布恢复通告"
echo "👔 [IC - Alice] War Room保持开放，15分钟后如无异常则关闭"
echo "👔 [IC - Alice] @全体: 安排明天上午10点复盘会议"

# === 14:52 - Communications Lead发布恢复通告 ===
echo ""
echo "📢 [Comms - Charlie] 发布恢复通告..."
cat > /tmp/recovery-notice.json <<EOF
{
  "incident": {
    "status": "resolved",
    "message": "API服务已完全恢复，所有功能正常。",
    "resolved_at": "2026-02-10T14:50:00Z",
    "duration": "18分钟"
  }
}
EOF
echo "📢 [Comms - Charlie] 恢复通告已发布"

# === 15:05 - 关闭War Room ===
echo ""
echo "👔 [IC - Alice] 服务稳定运行15分钟，无异常"
echo "👔 [IC - Alice] War Room正式关闭，感谢所有人的快速响应"
echo "👔 [IC - Alice] 请所有参与者准备复盘材料"
```
#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 所有人都是指挥官 | ✅ 明确唯一的IC | 多头指挥导致混乱，必须有唯一的最终决策者 |
| ❌ 技术人员负责对外沟通 | ✅ 专人负责沟通 | 技术人员专注修复，Communications Lead负责对外 |
| ❌ 等待所有人到齐再开始 | ✅ 先启动，人员陆续加入 | P0事故分秒必争，先开始处理 |
| ❌ War Room没有记录 | ✅ 指定专人记录 | 记录是复盘的重要输入，可录音或指定Scribe角色 |
| ❌ 角色固定不变 | ✅ 根据问题类型调整 | 网络问题需要网络专家，存储问题需要存储专家 |
| ❌ IC参与技术细节讨论 | ✅ IC保持全局视角 | IC应该关注整体进展，不陷入技术细节 |

---


graph TD
    A[监控告警触发] --> B{是否真实问题}
    B -->|误报| C[调整告警规则]
    B -->|真实问题| D[创建事故工单]
    
    D --> E[事故分级]
    E --> F{严重程度}
    
    F -->|P0关键| G[立即启动War Room]
    F -->|P1高| H[紧急响应团队]
    F -->|P2中| I[标准响应流程]
    F -->|P3低| J[工作时间处理]
    
    G --> K[应急响应]
    H --> K
    I --> K
    J --> K
    
    K --> L[故障诊断]
    L --> M[实施修复]
    M --> N{服务恢复}
    
    N -->|未恢复| L
    N -->|已恢复| O[验证服务质量]
    
    O --> P[通知相关方]
    P --> Q[事后复盘]
    Q --> R[改进措施]
    R --> S[关闭事故]
```
# 🟢 低风险：只读/信息收集，通常无副作用
---

## 2. 事故分级标准

> **🔰 初学者导读**: 事故分级决定了响应速度和参与人员。P0(全站宕机)需要全员紧急响应，P3(轻微问题)可以下个工作日处理。分级标准必须提前定义，避免事故发生时争论"到底有多严重"。

### 2.1 事故等级定义

| 等级 | 名称 | 业务影响 | 响应时间 | 解决目标 | 升级策略 | 通知范围 |
|------|------|----------|----------|----------|----------|----------|
| **P0** | 致命问题 | 核心业务完全中断，大量用户无法访问 | 立即响应（5分钟内） | 2小时内恢复 | 15分钟未进展升级 | CEO、CTO、所有相关团队 |
| **P1** | 严重问题 | 重要功能受影响，部分用户无法使用 | 15分钟内响应 | 4小时内恢复 | 1小时未进展升级 | 技术VP、产品负责人 |
| **P2** | 一般问题 | 非核心功能异常，用户体验下降 | 1小时内响应 | 8小时内解决 | 4小时未进展升级 | 相关技术团队 |
| **P3** | 轻微问题 | 低优先级问题，影响极小 | 工作时间响应 | 3天内解决 | 按计划处理 | 责任工程师 |

### 2.2 Kubernetes 问题分级示例

```yaml
# P0 级别问题示例
incidentExamples:
  p0-critical:
    - title: "控制平面完全不可用"
      symptoms:
        - "API Server 无法响应请求"
        - "所有 kubectl 命令失败"
        - "新 Pod 无法调度"
      impact: "集群完全失控，业务全面中断"
      
    - title: "etcd 集群数据丢失"
      symptoms:
        - "etcd 集群无法达成共识"
        - "数据不一致"
      impact: "集群状态不可信，存在数据丢失风险"
      
    - title: "认证系统问题"
      symptoms:
        - "用户无法登录"
        - "API 认证失败"
      impact: "所有用户无法访问系统"

  p1-high:
    - title: "单个可用区问题"
      symptoms:
        - "1/3 节点不可用"
        - "部分 Pod 无法调度"
      impact: "服务降级，部分功能受限"
      
    - title: "存储系统性能严重下降"
      symptoms:
        - "PVC 创建超时"
        - "IO 延迟 > 1000ms"
      impact: "有状态应用响应缓慢"
      
    - title: "网络策略错误配置"
      symptoms:
        - "跨命名空间通信失败"
        - "Service 不可达"
      impact: "部分服务间通信中断"

  p2-medium:
    - title: "监控告警风暴"
      symptoms:
        - "大量重复告警"
        - "告警系统过载"
      impact: "真实问题被淹没，运维效率降低"
      
    - title: "单个节点性能问题"
      symptoms:
        - "节点 CPU/内存压力大"
        - "Pod 驱逐频繁"
      impact: "个别应用实例不稳定"

  p3-low:
    - title: "文档链接失效"
      symptoms:
        - "404 错误"
      impact: "用户查阅文档不便"
      
    - title: "非关键组件版本过时"
      symptoms:
        - "监控组件版本落后"
      impact: "无立即风险，需计划升级"
```
### 2.3 P0-P3分级详解

#### 🔰 初学者理解

事故分级就像医院急诊的分诊系统：

- **P0 - 红色标签** (危重): 心脏骤停、大出血 → 立即抢救室，所有医生到位
- **P1 - 黄色标签** (紧急): 骨折、严重外伤 → 优先处理，专科医生
- **P2 - 绿色标签** (稳定): 发烧、轻伤 → 正常排队，门诊医生
- **P3 - 蓝色标签** (微小): 咨询、开药 → 可以改天再来

**为什么需要明确分级？**
- ❌ 不分级：所有问题都按P0处理 → 狼来了效应，真正的P0来了没人响应
- ✅ 分级：资源合理分配，重点保障核心服务

#### 🔧 工作原理

**分级决策矩阵**

```yaml
# 事故分级决策树
severityDecisionMatrix:
  # 维度1: 业务影响
  businessImpact:
    critical:
      - "核心业务完全中断(如:支付/登录/下单)"
      - "影响用户数 > 10000"
      - "预计损失 > $10000/小时"
      weight: 10
      
    high:
      - "重要功能受损(如:搜索/推荐)"
      - "影响用户数 1000-10000"
      - "预计损失 $1000-$10000/小时"
      weight: 7
      
    medium:
      - "非核心功能异常(如:统计报表)"
      - "影响用户数 100-1000"
      - "预计损失 $100-$1000/小时"
      weight: 4
      
    low:
      - "边缘功能问题(如:帮助文档)"
      - "影响用户数 < 100"
      - "预计损失 < $100/小时"
      weight: 1
      
  # 维度2: 技术严重程度
  technicalSeverity:
    critical:
      - "集群完全不可用"
      - "数据丢失风险"
      - "安全漏洞被利用"
      weight: 10
      
    high:
      - "单个可用区问题"
      - "性能严重下降(>80%)"
      - "部分服务不可用"
      weight: 7
      
    medium:
      - "单个节点问题"
      - "性能轻微下降(20-80%)"
      - "告警异常"
      weight: 4
      
    low:
      - "配置不规范"
      - "版本过时"
      - "文档问题"
      weight: 1
      
  # 分级规则
  rules:
    p0: "(business_weight >= 10) OR (technical_weight >= 10)"
    p1: "(business_weight >= 7) OR (technical_weight >= 7)"
    p2: "(business_weight >= 4) OR (technical_weight >= 4)"
    p3: "其他所有情况"
```

**分级算法实现**

```python
# incident_classifier.py - 事故分级决策引擎
class IncidentClassifier:
    def __init__(self):
        self.business_impact_scores = {
            "critical": 10,
            "high": 7,
            "medium": 4,
            "low": 1
        }
        self.technical_severity_scores = {
            "critical": 10,
            "high": 7,
            "medium": 4,
            "low": 1
        }
        
    def classify(self, business_impact, technical_severity, additional_factors=None):
        """
        事故分级决策
        
        参数:
            business_impact: 业务影响等级 (critical/high/medium/low)
            technical_severity: 技术严重程度 (critical/high/medium/low)
            additional_factors: 额外因素 (dict)
        
        返回:
            事故等级 (P0/P1/P2/P3)
        """
        # 计算基础分数
        b_score = self.business_impact_scores.get(business_impact, 1)
        t_score = self.technical_severity_scores.get(technical_severity, 1)
        
        # 额外因素调整
        if additional_factors:
            if additional_factors.get("data_loss_risk"):
                t_score = 10  # 数据丢失风险直接提升到最高
            if additional_factors.get("security_breach"):
                b_score = 10  # 安全漏洞直接提升到最高
            if additional_factors.get("peak_hours"):
                b_score = min(b_score + 2, 10)  # 高峰期提升2分
                
        # 决策逻辑
        max_score = max(b_score, t_score)
        
        if max_score >= 10:
            return "P0"
        elif max_score >= 7:
            return "P1"
        elif max_score >= 4:
            return "P2"
        else:
            return "P3"
            
    def get_response_requirements(self, severity):
        """根据等级返回响应要求"""
        requirements = {
            "P0": {
                "response_time": "5分钟",
                "resolution_target": "2小时",
                "team_size": "全员",
                "war_room": True,
                "notifications": ["CEO", "CTO", "All Managers"],
                "status_updates": "每15分钟"
            },
            "P1": {
                "response_time": "15分钟",
                "resolution_target": "4小时",
                "team_size": "核心团队(5-7人)",
                "war_room": True,
                "notifications": ["Tech VP", "Product Lead"],
                "status_updates": "每30分钟"
            },
            "P2": {
                "response_time": "1小时",
                "resolution_target": "8小时",
                "team_size": "相关团队(2-3人)",
                "war_room": False,
                "notifications": ["Team Lead"],
                "status_updates": "每2小时"
            },
            "P3": {
                "response_time": "工作时间",
                "resolution_target": "3天",
                "team_size": "责任工程师(1人)",
                "war_room": False,
                "notifications": ["Assignee"],
                "status_updates": "不需要"
            }
        }
        return requirements.get(severity, requirements["P3"])
```

#### 📝 最小示例

**实战分级案例**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# incident-classification-examples.sh - 真实场景分级示例

# ========== 案例1: API Server响应超时 ==========
echo "📋 案例1: API Server响应超时"
echo "现象: kubectl命令平均响应时间从100ms升高到5s"
echo ""

# 评估业务影响
AFFECTED_USERS=150
REVENUE_LOSS_PER_HOUR=5000

# 评估技术严重程度
API_ERROR_RATE=$(kubectl top [[Pods|pods]] -n kube-system | grep apiserver | awk '{print $3}')

echo "业务影响评估:"
echo "  - 影响用户数: $AFFECTED_USERS"
echo "  - 预计损失: \$$REVENUE_LOSS_PER_HOUR/小时"
echo "  - 核心功能是否中断: 否(仅响应慢)"
echo "  → 业务影响等级: MEDIUM"

echo ""
echo "技术严重程度评估:"
echo "  - API Server状态: 运行但响应慢"
echo "  - 集群可用性: 部分功能受限"
echo "  - 数据丢失风险: 无"
echo "  → 技术严重程度: MEDIUM"

echo ""
echo "🔖 最终分级: P2 (一般问题)"
echo "响应要求: 1小时内响应，8小时内解决"

# ========== 案例2: etcd数据不一致 ==========
echo ""
echo "========================================"
echo "📋 案例2: etcd集群出现split brain"
echo "现象: etcd集群分裂成两个子集群，数据不一致"
echo ""

echo "业务影响评估:"
echo "  - 影响用户数: 所有用户(20000+)"
echo "  - 核心功能是否中断: 是(API Server无法写入)"
echo "  → 业务影响等级: CRITICAL"

echo ""
echo "技术严重程度评估:"
echo "  - etcd状态: Split Brain"
echo "  - 数据丢失风险: 极高"
echo "  - 集群可用性: 完全不可用"
echo "  → 技术严重程度: CRITICAL"

echo ""
echo "🚨 最终分级: P0 (致命问题)"
echo "响应要求: 立即响应(5分钟内)，2小时内恢复"
echo "立即启动War Room，通知CEO/CTO"

# ========== 案例3: 单个Pod CrashLoopBackOff ==========
echo ""
echo "========================================"
echo "📋 案例3: 测试环境单个Pod崩溃"
echo "现象: test命名空间的1个Pod持续重启"
echo ""

echo "业务影响评估:"
echo "  - 影响用户数: 0 (测试环境)"
echo "  - 核心功能是否中断: 否"
echo "  → 业务影响等级: LOW"

echo ""
echo "技术严重程度评估:"
echo "  - 集群状态: 正常"
echo "  - 数据丢失风险: 无"
echo "  - 其他服务影响: 无"
echo "  → 技术严重程度: LOW"

echo ""
echo "✅ 最终分级: P3 (轻微问题)"
echo "响应要求: 工作时间处理，3天内解决"

# ========== 案例4: 高峰期部分请求失败 ==========
echo ""
echo "========================================"
echo "📋 案例4: 双11高峰期部分订单请求失败"
echo "现象: 订单API错误率从0.1%升高到5%"
echo "时间: 晚上8点(流量高峰)"
echo ""

echo "业务影响评估:"
echo "  - 影响用户数: 5000+"
echo "  - 预计损失: \$50000/小时"
echo "  - 核心功能是否中断: 部分中断(5%失败率)"
echo "  - 额外因素: 高峰期 + 促销活动"
echo "  → 业务影响等级: CRITICAL (高峰期加权)"

echo ""
echo "技术严重程度评估:"
echo "  - 服务状态: 部分不可用"
echo "  - 性能下降: 5%错误率"
echo "  → 技术严重程度: HIGH"

echo ""
echo "🚨 最终分级: P0 (高峰期特殊升级)"
echo "原因: 虽然只有5%错误率，但在双11高峰期，业务影响巨大"
echo "响应要求: 立即启动War Room，全员响应"
```
#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 测试环境也按P0处理 | ✅ 测试环境最高P2 | 测试环境不直接影响用户，不应占用P0资源 |
| ❌ 只看技术指标不看业务 | ✅ 业务影响优先 | 技术上很严重但不影响用户可以降级处理 |
| ❌ 一成不变的分级标准 | ✅ 考虑时间因素 | 同样问题，高峰期和半夜3点的分级应该不同 |
| ❌ 分级后不能改变 | ✅ 动态调整分级 | 发现影响扩大应及时升级，影响缩小也可降级 |
| ❌ 分级由开发人员决定 | ✅ On-Call SRE决定 | 需要有经验的人快速判断，避免争论延误 |
| ❌ 有争议就选高等级 | ✅ 按标准客观评估 | 分级过度导致"狼来了"效应 |

---

### 2.4 影响评估方法

#### 🔰 初学者理解

影响评估就像地震发生后的灾情评估：

- **震级** (技术严重程度) - 地震本身的能量有多大？
- **烈度** (业务影响) - 对人民生命财产造成多大损失？

**关键理解**：震级和烈度不一定成正比！
- 震级8.0但发生在无人区 → 技术严重但业务影响小 → 可能是P1
- 震级5.0但在人口密集区 → 技术不严重但业务影响大 → 可能升级为P0

#### 🔧 工作原理

**多维度影响评估模型**

```yaml
# 影响评估框架
impactAssessmentFramework:
  # 维度1: 用户影响
  userImpact:
    metrics:
      - name: "受影响用户数"
        weight: 0.4
        thresholds:
          critical: "> 10000 用户"
          high: "1000-10000 用户"
          medium: "100-1000 用户"
          low: "< 100 用户"
          
      - name: "用户体验下降"
        weight: 0.3
        thresholds:
          critical: "完全无法使用"
          high: "严重卡顿(响应时间 > 10s)"
          medium: "轻微卡顿(响应时间 2-10s)"
          low: "几乎无感知"
          
      - name: "功能类型"
        weight: 0.3
        thresholds:
          critical: "核心功能(登录/支付/下单)"
          high: "重要功能(搜索/推荐)"
          medium: "辅助功能(统计/报表)"
          low: "边缘功能(帮助/文档)"
          
  # 维度2: 财务影响
  financialImpact:
    metrics:
      - name: "直接收入损失"
        formula: "每小时预计损失 = 正常时段收入 × 影响比例"
        thresholds:
          critical: "> $10000/小时"
          high: "$1000-$10000/小时"
          medium: "$100-$1000/小时"
          low: "< $100/小时"
          
      - name: "间接成本"
        includes:
          - "客服成本增加"
          - "补偿成本(退款/赔偿)"
          - "品牌声誉损失"
          - "合同违约风险"
          
  # 维度3: 技术影响
  technicalImpact:
    metrics:
      - name: "系统可用性"
        thresholds:
          critical: "完全不可用"
          high: "可用率 < 95%"
          medium: "可用率 95-99%"
          low: "可用率 > 99%"
          
      - name: "数据完整性"
        thresholds:
          critical: "数据丢失/损坏"
          high: "数据不一致"
          medium: "数据延迟"
          low: "无影响"
          
      - name: "安全风险"
        thresholds:
          critical: "安全漏洞被利用"
          high: "安全漏洞暴露"
          medium: "安全配置不当"
          low: "无安全风险"
          
  # 维度4: 合规影响
  complianceImpact:
    metrics:
      - name: "SLA违约"
        thresholds:
          critical: "违反关键SLA条款"
          high: "接近SLA阈值"
          medium: "SLA余量充足"
          low: "无SLA影响"
          
      - name: "法规遵从"
        includes:
          - "数据保护法规(GDPR)"
          - "金融监管要求"
          - "行业标准(PCI-DSS)"
```

**影响评估决策引擎**

```python
# impact_assessor.py - 影响评估引擎
import datetime

class ImpactAssessor:
    def __init__(self):
        self.user_thresholds = {
            "critical": 10000,
            "high": 1000,
            "medium": 100,
            "low": 0
        }
        self.revenue_thresholds = {
            "critical": 10000,
            "high": 1000,
            "medium": 100,
            "low": 0
        }
        
    def assess_user_impact(self, affected_users, functionality_type, degradation_level):
        """评估用户影响"""
        # 基础分数 (基于用户数)
        if affected_users >= self.user_thresholds["critical"]:
            base_score = 10
        elif affected_users >= self.user_thresholds["high"]:
            base_score = 7
        elif affected_users >= self.user_thresholds["medium"]:
            base_score = 4
        else:
            base_score = 1
            
        # 功能类型加权
        functionality_weights = {
            "core": 1.5,      # 核心功能: 登录/支付
            "important": 1.2,  # 重要功能: 搜索/推荐
            "auxiliary": 1.0,  # 辅助功能: 统计/报表
            "edge": 0.8        # 边缘功能: 帮助/文档
        }
        weight = functionality_weights.get(functionality_type, 1.0)
        
        # 降级程度加权
        degradation_weights = {
            "complete": 1.5,    # 完全不可用
            "severe": 1.3,      # 严重卡顿
            "moderate": 1.1,    # 轻微卡顿
            "minimal": 0.9      # 几乎无感
        }
        deg_weight = degradation_weights.get(degradation_level, 1.0)
        
        final_score = base_score * weight * deg_weight
        return min(final_score, 10)  # 最高10分
        
    def assess_financial_impact(self, normal_revenue_per_hour, impact_ratio, duration_hours):
        """评估财务影响"""
        direct_loss = normal_revenue_per_hour * impact_ratio * duration_hours
        
        # 间接成本估算 (通常是直接损失的30-50%)
        indirect_cost_ratio = 0.4
        total_loss = direct_loss * (1 + indirect_cost_ratio)
        
        # 转换为严重程度分数
        loss_per_hour = total_loss / max(duration_hours, 1)
        
        if loss_per_hour >= self.revenue_thresholds["critical"]:
            return {"[[Score|score]]": 10, "estimated_loss": total_loss}
        elif loss_per_hour >= self.revenue_thresholds["high"]:
            return {"score": 7, "estimated_loss": total_loss}
        elif loss_per_hour >= self.revenue_thresholds["medium"]:
            return {"score": 4, "estimated_loss": total_loss}
        else:
            return {"score": 1, "estimated_loss": total_loss}
            
    def assess_technical_impact(self, availability, data_integrity, security_risk):
        """评估技术影响"""
        scores = []
        
        # 可用性评分
        if availability < 0.5:
            scores.append(10)
        elif availability < 0.95:
            scores.append(7)
        elif availability < 0.99:
            scores.append(4)
        else:
            scores.append(1)
            
        # 数据完整性评分
        data_scores = {
            "loss": 10,
            "corruption": 10,
            "inconsistency": 7,
            "delay": 4,
            "none": 1
        }
        scores.append(data_scores.get(data_integrity, 1))
        
        # 安全风险评分
        security_scores = {
            "breach": 10,
            "vulnerability_exposed": 8,
            "misconfiguration": 4,
            "none": 1
        }
        scores.append(security_scores.get(security_risk, 1))
        
        return max(scores)  # 取最高分
        
    def assess_time_sensitivity(self, current_time):
        """评估时间敏感性 (高峰期/非高峰期)"""
        hour = current_time.hour
        weekday = current_time.weekday()
        
        # 工作日 9:00-22:00 为高峰期
        if weekday < 5 and 9 <= hour <= 22:
            return "peak", 1.3  # 高峰期加权30%
        # 周末全天为次高峰
        elif weekday >= 5:
            return "weekend", 1.1  # 周末加权10%
        else:
            return "off-peak", 1.0  # 非高峰期不加权
            
    def comprehensive_assessment(self, incident_data):
        """综合评估"""
        # 用户影响
        user_score = self.assess_user_impact(
            incident_data["affected_users"],
            incident_data["functionality_type"],
            incident_data["degradation_level"]
        )
        
        # 财务影响
        financial_assessment = self.assess_financial_impact(
            incident_data["normal_revenue_per_hour"],
            incident_data["impact_ratio"],
            incident_data["expected_duration"]
        )
        
        # 技术影响
        technical_score = self.assess_technical_impact(
            incident_data["availability"],
            incident_data["data_integrity"],
            incident_data["security_risk"]
        )
        
        # 时间敏感性
        time_label, time_weight = self.assess_time_sensitivity(
            incident_data.get("occurred_at", datetime.datetime.now())
        )
        
        # 综合评分 (加权平均)
        weighted_score = (
            user_score * 0.4 +
            financial_assessment["score"] * 0.3 +
            technical_score * 0.3
        ) * time_weight
        
        return {
            "overall_score": weighted_score,
            "user_impact": user_score,
            "financial_impact": financial_assessment,
            "technical_impact": technical_score,
            "time_sensitivity": time_label,
            "recommendation": self._get_severity(weighted_score)
        }
        
    def _get_severity(self, score):
        """根据综合评分推荐严重程度"""
        if score >= 10:
            return "P0"
        elif score >= 7:
            return "P1"
        elif score >= 4:
            return "P2"
        else:
            return "P3"
```

#### 📝 最小示例

**实战影响评估示例**

```bash
#!/bin/bash
# impact-assessment-example.sh - 影响评估实战

# ========== 场景: 支付服务性能下降 ==========
echo "🎯 场景: 双11晚上8点，支付服务响应时间从200ms升高到8s"
echo ""

# 收集数据
CURRENT_HOUR=20
CURRENT_DAY="Friday"
AFFECTED_USERS=8000
NORMAL_REVENUE_PER_HOUR=100000
IMPACT_RATIO=0.6  # 60%订单失败
AVAILABILITY=0.40  # 40%可用率

echo "📊 数据收集:"
echo "  - 当前时间: 周五晚上8点 (双11高峰)"
echo "  - 受影响用户: $AFFECTED_USERS"
echo "  - 正常时段收入: \$$NORMAL_REVENUE_PER_HOUR/小时"
echo "  - 影响比例: $((IMPACT_RATIO * 100))%"
echo "  - 系统可用率: $((AVAILABILITY * 100))%"
echo ""

# 评估1: 用户影响
echo "📱 用户影响评估:"
echo "  - 用户数量级: > 1000 → HIGH"
echo "  - 功能类型: 支付(核心功能) → CRITICAL"
echo "  - 降级程度: 响应8s(严重卡顿) → SEVERE"
echo "  - 用户影响分数: 10/10"
echo ""

# 评估2: 财务影响
echo "💰 财务影响评估:"
DIRECT_LOSS=$(echo "$NORMAL_REVENUE_PER_HOUR * $IMPACT_RATIO" | bc)
INDIRECT_COST=$(echo "$DIRECT_LOSS * 0.4" | bc)
TOTAL_LOSS=$(echo "$DIRECT_LOSS + $INDIRECT_COST" | bc)

echo "  - 直接收入损失: \$$DIRECT_LOSS/小时"
echo "  - 间接成本(客服/补偿): \$$INDIRECT_COST/小时"
echo "  - 总损失: \$$TOTAL_LOSS/小时"
echo "  - 财务影响分数: 10/10 (> \$10000/小时)"
echo ""

# 评估3: 技术影响
echo "🔧 技术影响评估:"
echo "  - 系统可用性: 40% → CRITICAL"
echo "  - 数据完整性: 部分订单丢失 → HIGH"
echo "  - 安全风险: 无 → LOW"
echo "  - 技术影响分数: 10/10"
echo ""

# 评估4: 时间敏感性
echo "⏰ 时间敏感性评估:"
echo "  - 时间: 周五晚8点"
echo "  - 特殊事件: 双11促销"
echo "  - 时间加权: 1.5x (高峰期 + 促销活动)"
echo ""

# 综合评估
WEIGHTED_SCORE=$(echo "((10 * 0.4) + (10 * 0.3) + (10 * 0.3)) * 1.5" | bc)
echo "🎯 综合评估:"
echo "  - 加权前分数: 10.0"
echo "  - 时间加权: 1.5x"
echo "  - 最终分数: $WEIGHTED_SCORE"
echo ""

echo "🚨 **评估结论: P0 (致命问题)**"
echo ""
echo "📋 响应要求:"
echo "  - 响应时间: 立即 (5分钟内)"
echo "  - 解决目标: 2小时内恢复"
echo "  - 团队规模: 全员"
echo "  - War Room: 立即启动"
echo "  - 通知范围: CEO, CTO, 所有管理层"
echo "  - 预计损失: \$$TOTAL_LOSS/小时 × 持续时长"
```

#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 只看当前影响，不看趋势 | ✅ 评估影响扩散速度 | 影响快速扩大应提前升级 |
| ❌ 只评估技术指标 | ✅ 多维度综合评估 | 必须同时考虑用户、财务、技术、合规 |
| ❌ 忽略时间因素 | ✅ 考虑时间敏感性 | 同样问题在高峰期和半夜3点影响差异巨大 |
| ❌ 财务损失只算直接收入 | ✅ 包含间接成本 | 客服成本、补偿成本、品牌损失都要考虑 |
| ❌ 评估完就结束 | ✅ 持续重新评估 | 事故进展中影响可能变化，需动态调整 |
| ❌ 主观臆断 | ✅ 基于数据和公式 | 使用量化指标，减少主观判断 |

---

## 3. 应急响应流程

> **🔰 初学者导读**: 应急响应是一套标准化的"灭火"流程：确认→通知→组队→止血→修复→恢复。关键原则：先恢复服务再查根因（"先灭火再查火因"）。

### 3.1 P0 级别快速响应手册

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# P0 事故应急响应标准流程（15分钟黄金窗口期）

# ========== 第一步：立即确认问题（0-2分钟）==========
echo "=== 步骤 1: 快速问题确认 ==="

# 检查 API Server 健康状态
kubectl get --raw='/healthz' || echo "API Server 不可用"

# 检查核心组件状态
kubectl get cs
kubectl get nodes
kubectl top nodes

# 检查关键命名空间
kubectl get pods -n kube-system -o wide
kubectl get pods -n production -o wide

# ========== 第二步：启动 War Room（2-5分钟）==========
echo "=== 步骤 2: 启动 War Room ==="

# 发送紧急通知（钉钉/Slack/企业微信）
curl -X POST https://hooks.slack.com/services/YOUR_WEBHOOK \
  -H 'Content-Type: application/json' \
  -d '{
    "text": "🚨 P0 事故告警",
    "attachments": [{
      "color": "danger",
      "title": "生产集群核心服务中断",
      "fields": [
        {"title": "集群", "value": "prod-cluster-01", "short": true},
        {"title": "影响范围", "value": "全部用户", "short": true},
        {"title": "War Room", "value": "https://zoom.us/j/emergency", "short": false}
      ]
    }]
  }'

# ========== 第三步：快速诊断（5-10分钟）==========
echo "=== 步骤 3: 快速诊断关键指标 ==="

# etcd 健康检查
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://etcd-0:2379,https://etcd-1:2379,https://etcd-2:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 控制平面组件日志（最近100行）
journalctl -u kube-apiserver -n 100 --no-pager
journalctl -u kube-controller-manager -n 100 --no-pager
journalctl -u kube-scheduler -n 100 --no-pager

# 网络连通性测试
kubectl run test-pod --image=busybox --restart=Never -- sleep 3600
kubectl exec test-pod -- nslookup kubernetes.default.svc.cluster.local
kubectl delete pod test-pod

# ========== 第四步：执行应急恢复（10-15分钟）==========
echo "=== 步骤 4: 执行应急恢复措施 ==="

# 根据问题类型执行对应的应急手册
# 示例：API Server 重启
# systemctl restart kube-apiserver

# 验证恢复效果
kubectl get nodes
kubectl get pods --all-namespaces | grep -v Running

echo "=== P0 应急响应完成，进入持续监控阶段 ==="
```
### 3.2 黄金30分钟原则

#### 🔰 初学者理解

黄金30分钟就像心脏病发作后的"黄金救治时间"：

- **前5分钟**: 确认病情 (事故确认)
- **5-15分钟**: 拨打120、准备急救 (启动响应、初步诊断)
- **15-30分钟**: 医护到达、紧急处理 (实施修复、止血)

**为什么是30分钟？**
- 用户耐心极限: 大多数用户在30分钟内会放弃等待
- 影响扩散临界点: 30分钟后问题往往会扩散到更多组件
- 团队响应效率: 30分钟是团队能保持高度专注的时间窗口

#### 🔧 工作原理

**黄金30分钟行动清单**

```yaml
# 黄金30分钟时间轴
goldenThirtyMinutes:
  minute_00-05:
    phase: "快速确认"
    objectives:
      - "确认告警真实性 (排除误报)"
      - "初步评估影响范围"
      - "确定事故等级"
    actions:
      - action: "检查监控仪表板"
        time: "1分钟"
        tools: ["Grafana", "Prometheus"]
        
      - action: "验证用户报告"
        time: "2分钟"
        tools: ["客服系统", "用户反馈"]
        
      - action: "快速健康检查"
        time: "2分钟"
        commands:
          - "kubectl get nodes"
          - "kubectl get pods -A | grep -v Running"
          - "kubectl top nodes"
    
    success_criteria: "5分钟内完成事故定级"
    
  minute_05-15:
    phase: "紧急动员"
    objectives:
      - "启动War Room (P0/P1)"
      - "通知相关人员"
      - "技术诊断定位"
    actions:
      - action: "发送紧急通知"
        time: "2分钟"
        recipients:
          P0: ["CEO", "CTO", "All Tech Leads"]
          P1: ["VP Engineering", "Team Leads"]
          
      - action: "启动War Room会议"
        time: "3分钟"
        checklist:
          - "创建专用会议室"
          - "分配角色 (IC/Tech Lead/Comms)"
          - "开启录制"
          
      - action: "并行诊断"
        time: "5分钟"
        parallel_tasks:
          - "检查控制平面组件"
          - "检查网络连通性"
          - "检查存储系统"
          - "分析最近变更"
    
    success_criteria: "15分钟内定位根因或缩小范围"
    
  minute_15-30:
    phase: "快速止血"
    objectives:
      - "实施临时修复"
      - "恢复核心功能"
      - "验证恢复效果"
    strategies:
      fast_rollback:
        description: "回滚最近变更"
        time: "5-10分钟"
        risk: "low"
        适用场景: "变更导致的问题"
        
      quick_restart:
        description: "重启问题组件"
        time: "3-5分钟"
        risk: "medium"
        适用场景: "组件hang或内存泄漏"
        
      traffic_diversion:
        description: "流量切换到备用系统"
        time: "5-10分钟"
        risk: "low"
        适用场景: "单区域问题"
        
      emergency_scale:
        description: "紧急扩容"
        time: "10-15分钟"
        risk: "low"
        适用场景: "容量不足"
    
    success_criteria: "30分钟内服务基本恢复或有明确恢复路径"
    
  fallback_plan:
    condition: "30分钟内无法恢复"
    actions:
      - "升级事故等级"
      - "启动灾备切换预案"
      - "准备对外公告"
      - "评估是否需要外部支持"
```

**30分钟执行脚本**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# golden-30-minutes.sh - 黄金30分钟应急响应脚本

START_TIME=$(date +%s)
INCIDENT_ID="INC-$(date +%Y%m%d-%H%M%S)"

echo "🚨 启动黄金30分钟应急响应"
echo "事故ID: $INCIDENT_ID"
echo "开始时间: $(date)"
echo ""

# ========== 阶段1: 快速确认 (0-5分钟) ==========
echo "⏰ [00:00] 阶段1: 快速确认 (目标: 5分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 1分钟: 检查监控仪表板
echo "[00:01] 检查Prometheus告警..."
ACTIVE_ALERTS=$(curl -s http://prometheus:9090/api/v1/alerts | jq '.data.alerts | length')
echo "  → 活跃告警数: $ACTIVE_ALERTS"

# 2分钟: 验证问题真实性
echo "[00:02] 快速健康检查..."
kubectl get nodes --no-headers | grep -v "Ready" > /tmp/unhealthy-nodes.txt
UNHEALTHY_NODES=$(wc -l < /tmp/unhealthy-nodes.txt)
echo "  → 不健康节点: $UNHEALTHY_NODES"

kubectl get pods -A --field-selector=status.phase!=Running --no-headers | wc -l > /tmp/failed-pods-count.txt
FAILED_PODS=$(cat /tmp/failed-pods-count.txt)
echo "  → 异常Pod数: $FAILED_PODS"

# 3-5分钟: 事故定级
echo "[00:03] 评估影响范围..."
if [ $UNHEALTHY_NODES -gt 10 ] || [ $FAILED_PODS -gt 100 ]; then
  SEVERITY="P0"
  echo "  → 判定等级: P0 (致命问题)"
elif [ $UNHEALTHY_NODES -gt 3 ] || [ $FAILED_PODS -gt 20 ]; then
  SEVERITY="P1"
  echo "  → 判定等级: P1 (严重问题)"
else
  SEVERITY="P2"
  echo "  → 判定等级: P2 (一般问题)"
fi

PHASE1_TIME=$(($(date +%s) - START_TIME))
echo "✓ 阶段1完成，耗时: ${PHASE1_TIME}秒"
echo ""

# ========== 阶段2: 紧急动员 (5-15分钟) ==========
echo "⏰ [$(date +%M:%S)] 阶段2: 紧急动员 (目标: 10分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 5-7分钟: 发送通知
echo "[$(date +%M:%S)] 发送紧急通知..."
if [ "$SEVERITY" = "P0" ]; then
  # P0通知所有人
  curl -X POST https://hooks.slack.com/services/WEBHOOK \
    -H 'Content-Type: application/json' \
    -d "{
      \"text\": \"🚨 P0事故: $INCIDENT_ID\",
      \"channel\": \"#incidents-critical\",
      \"mentions\": [\"@channel\"]
    }" 2>/dev/null
  echo "  → P0通知已发送: @CEO @CTO @All-Tech-Leads"
else
  echo "  → $SEVERITY 通知已发送: @Team-Leads"
fi

# 7-10分钟: 启动War Room
if [ "$SEVERITY" = "P0" ] || [ "$SEVERITY" = "P1" ]; then
  echo "[$(date +%M:%S)] 启动War Room..."
  echo "  → War Room链接: https://zoom.us/j/emergency-$INCIDENT_ID"
  echo "  → 角色分配:"
  echo "    - Incident Commander: On-Call SRE"
  echo "    - Tech Lead: K8s Expert"
  echo "    - Communications: Customer Success"
fi

# 10-15分钟: 并行诊断
echo "[$(date +%M:%S)] 并行诊断根因..."

# 诊断任务1: 控制平面
(
  echo "  [诊断-控制平面] 检查API Server..."
  kubectl get --raw='/healthz' >/dev/null 2>&1 && echo "    ✓ API Server正常" || echo "    ✗ API Server异常"
  
  echo "  [诊断-控制平面] 检查etcd..."
  ETCDCTL_API=3 etcdctl endpoint health 2>&1 | grep "healthy" >/dev/null && echo "    ✓ etcd正常" || echo "    ✗ etcd异常"
) &

# 诊断任务2: 网络
(
  echo "  [诊断-网络] 检查DNS..."
  kubectl run test-dns --image=busybox --restart=Never --rm -it -- nslookup kubernetes.default 2>&1 | grep "Name:" >/dev/null && echo "    ✓ DNS正常" || echo "    ✗ DNS异常"
) &

# 诊断任务3: 最近变更
(
  echo "  [诊断-变更] 查询最近变更..."
  RECENT_CHANGES=$(kubectl get events -A --sort-by='.lastTimestamp' | head -10)
  echo "    → 最近10条事件已记录"
) &

wait  # 等待所有并行诊断完成

PHASE2_TIME=$(($(date +%s) - START_TIME))
echo "✓ 阶段2完成，累计耗时: ${PHASE2_TIME}秒"
echo ""

# ========== 阶段3: 快速止血 (15-30分钟) ==========
echo "⏰ [$(date +%M:%S)] 阶段3: 快速止血 (目标: 15分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 根据诊断结果选择止血策略
echo "[$(date +%M:%S)] 选择止血策略..."

if kubectl get events -A | grep -q "ImagePullBackOff"; then
  echo "  → 检测到镜像拉取失败，执行策略: 切换镜像仓库"
  # 实际操作...
  
elif kubectl top nodes | awk '{print $3}' | grep -o '[0-9]*' | awk '{if($1>90) print "overload"}' | grep -q "overload"; then
  echo "  → 检测到节点过载，执行策略: 紧急扩容"
  # kubectl scale deployment critical-app --replicas=20
  
else
  echo "  → 未找到明确根因，执行策略: 重启可疑组件"
  # systemctl restart kube-apiserver
fi

# 验证恢复
echo "[$(date +%M:%S)] 验证恢复效果..."
sleep 10  # 等待服务稳定

CURRENT_FAILED=$(kubectl get pods -A --field-selector=status.phase!=Running --no-headers | wc -l)
if [ $CURRENT_FAILED -lt $((FAILED_PODS / 2)) ]; then
  echo "  ✓ 异常Pod数从 $FAILED_PODS 降至 $CURRENT_FAILED (改善50%+)"
  RECOVERY_STATUS="partial"
else
  echo "  ⚠ 恢复效果不明显，需要升级响应"
  RECOVERY_STATUS="insufficient"
fi

PHASE3_TIME=$(($(date +%s) - START_TIME))
TOTAL_TIME=$PHASE3_TIME

echo "✓ 阶段3完成，累计耗时: ${TOTAL_TIME}秒"
echo ""

# ========== 总结 ==========
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 黄金30分钟执行总结"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "事故ID: $INCIDENT_ID"
echo "事故等级: $SEVERITY"
echo "总耗时: ${TOTAL_TIME}秒 (目标: ≤1800秒)"

if [ $TOTAL_TIME -le 1800 ]; then
  echo "状态: ✓ 在黄金窗口期内完成响应"
else
  echo "状态: ⚠ 超出黄金窗口期，需要升级"
fi

echo "恢复状态: $RECOVERY_STATUS"
echo ""
echo "下一步行动:"
if [ "$RECOVERY_STATUS" = "partial" ]; then
  echo "  1. 持续监控15分钟"
  echo "  2. 深入排查根因"
  echo "  3. 计划永久性修复"
else
  echo "  1. 升级事故等级"
  echo "  2. 启动灾备切换"
  echo "  3. 准备对外公告"
fi
```
#### 📝 最小示例

**黄金30分钟实战演练**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 30-minute-drill.sh - 黄金30分钟桌面演练

echo "🎯 黄金30分钟桌面演练"
echo "场景: 生产集群API Server响应超时"
echo "开始时间: $(date)"
echo ""

# 使用计时器模拟真实压力
trap 'echo ""; echo "⏰ 时间到！总耗时: $SECONDS 秒"' EXIT

echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "阶段1: 快速确认 (0-5分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
read -p "✋ [行动] 检查Prometheus告警页面 (按回车确认完成)..."
read -p "✋ [行动] 执行 kubectl get nodes (按回车确认完成)..."
read -p "✋ [行动] 执行 kubectl get pods -A | grep -v Running (按回车)..."
read -p "✋ [决策] 事故定级: P0/P1/P2? 输入等级: " SEVERITY

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "阶段2: 紧急动员 (5-15分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
read -p "✋ [行动] 发送Slack紧急通知 (按回车)..."
if [ "$SEVERITY" = "P0" ] || [ "$SEVERITY" = "P1" ]; then
  read -p "✋ [行动] 启动War Room会议 (按回车)..."
  read -p "✋ [行动] 分配角色: IC/Tech Lead/Comms (按回车)..."
fi
read -p "✋ [行动] 并行检查: etcd健康/DNS/最近变更 (按回车)..."

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "阶段3: 快速止血 (15-30分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "根因假设: etcd磁盘IO饱和"
read -p "✋ [决策] 选择止血策略: 1)重启etcd 2)扩容磁盘 3)流量降级? 输入: " STRATEGY

case $STRATEGY in
  1) echo "  → 执行: systemctl restart etcd" ;;
  2) echo "  → 执行: 扩容etcd磁盘至100GB" ;;
  3) echo "  → 执行: 限流50%写入请求" ;;
  *) echo "  ⚠ 无效策略" ;;
esac

read -p "✋ [行动] 等待10秒观察恢复效果 (按回车)..."
sleep 3  # 模拟等待
echo "  → API Server响应时间: 从5s降至200ms ✓"
echo "  → 异常Pod数: 从50降至5 ✓"

echo ""
echo "✅ 演练完成！"
echo "📊 性能评估:"
if [ $SECONDS -le 300 ]; then
  echo "  ⭐⭐⭐ 优秀! 在5分钟内完成所有操作"
elif [ $SECONDS -le 600 ]; then
  echo "  ⭐⭐ 良好! 在10分钟内完成"
elif [ $SECONDS -le 1800 ]; then
  echo "  ⭐ 及格! 在30分钟内完成"
else
  echo "  ❌ 需要改进! 超出黄金窗口期"
fi
```
#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 追求完美根因分析 | ✅ 先止血再查根因 | 30分钟内优先恢复服务，根因分析留给复盘 |
| ❌ 等待所有信息到齐 | ✅ 基于现有信息决策 | 信息永远不完整，及时决策比完美方案更重要 |
| ❌ 单人承担所有任务 | ✅ 并行执行多任务 | 利用团队力量，诊断、修复、沟通同时进行 |
| ❌ 30分钟内必须彻底解决 | ✅ 30分钟内要有明确进展 | 目标是基本恢复或有明确路径，不是彻底解决 |
| ❌ 不敢做有风险的操作 | ✅ 评估风险后果断执行 | 不操作的风险往往大于操作风险 |
| ❌ 忘记记录操作过程 | ✅ 实时记录关键操作 | 记录是复盘的重要输入 |

---

### 3.3 止血优先原则

#### 🔰 初学者理解

止血优先就像战场急救的"MARCH原则"：

- **M** (Massive hemorrhage) - 大出血: 先止血
- **A** (Airway) - 气道: 再通气
- **R** (Respiration) - 呼吸: 然后处理呼吸
- **C** (Circulation) - 循环: 接着恢复循环
- **H** (Head/Hypothermia) - 头部/失温: 最后处理其他

**核心理念**: 生存 > 完美

在事故响应中：
- **先恢复服务** > 再查根因
- **临时方案快速止血** > 完美方案慢慢修复
- **核心功能优先** > 边缘功能稍后

#### 🔧 工作原理

**止血策略优先级矩阵**

```yaml
# 止血策略决策树
stopBleedingStrategies:
  # 优先级1: 快速回滚 (风险最低)
  rollback:
    priority: 1
    execution_time: "5-10分钟"
    success_rate: "95%"
    risk_level: "low"
    prerequisites:
      - "有明确的最近变更"
      - "变更前版本稳定"
      - "有回滚脚本"
    examples:
      - "回滚Deployment到上个版本"
      - "恢复ConfigMap备份"
      - "撤销最近的Helm升级"
    commands:
      - "kubectl rollout undo deployment/app"
      - "helm rollback release 1"
      - "git revert && kubectl apply -f"
      
  # 优先级2: 重启服务 (快速但有短暂中断)
  restart:
    priority: 2
    execution_time: "3-5分钟"
    success_rate: "70%"
    risk_level: "medium"
    适用场景:
      - "内存泄漏"
      - "僵尸进程"
      - "连接池耗尽"
      - "组件hang住"
    commands:
      - "kubectl rollout restart deployment/app"
      - "systemctl restart kube-apiserver"
      - "kubectl delete pod <stuck-pod> --grace-period=0"
      
  # 优先级3: 流量转移 (保护核心功能)
  traffic_diversion:
    priority: 3
    execution_time: "5-10分钟"
    success_rate: "90%"
    risk_level: "low"
    strategies:
      regional_failover:
        description: "切换到备用区域"
        commands:
          - "kubectl patch service app --patch '{\"spec\":{\"selector\":{\"region\":\"backup\"}}}'"
          
      canary_rollback:
        description: "金丝雀版本权重降至0"
        commands:
          - "kubectl patch virtualservice app --type merge -p '{\"spec\":{\"http\":[{\"route\":[{\"weight\":0}]}]}}'"
          
      degraded_mode:
        description: "降级到最小功能集"
        commands:
          - "kubectl patch deployment app --patch '{\"spec\":{\"template\":{\"spec\":{\"containers\":[{\"env\":[{\"name\":\"DEGRADED_MODE\",\"value\":\"true\"}]}]}}}}'"
          
  # 优先级4: 紧急扩容 (解决容量问题)
  emergency_scale:
    priority: 4
    execution_time: "10-15分钟"
    success_rate: "80%"
    risk_level: "low"
    适用场景:
      - "CPU/内存不足"
      - "连接数达到上限"
      - "请求队列堆积"
    commands:
      - "kubectl scale deployment app --replicas=50"
      - "kubectl set resources deployment app --limits=cpu=4,memory=8Gi"
      
  # 优先级5: 数据修复 (最后手段，高风险)
  data_repair:
    priority: 5
    execution_time: "30+分钟"
    success_rate: "50%"
    risk_level: "high"
    warning: "⚠️ 必须在IC批准后才能执行"
    适用场景:
      - "数据损坏"
      - "etcd数据不一致"
      - "存储卷问题"
    commands:
      - "etcdctl snapshot restore"
      - "kubectl apply -f pv-backup.yaml"
```

**止血决策算法**

```python
# stopbleeding_decision.py - 止血策略决策引擎
class StopBleedingDecisionEngine:
    def __init__(self):
        self.strategies = {
            "rollback": {"priority": 1, "time": 10, "risk": "low"},
            "restart": {"priority": 2, "time": 5, "risk": "medium"},
            "traffic_diversion": {"priority": 3, "time": 10, "risk": "low"},
            "emergency_scale": {"priority": 4, "time": 15, "risk": "low"},
            "data_repair": {"priority": 5, "time": 30, "risk": "high"}
        }
        
    def decide(self, incident_context):
        """
        根据事故上下文决定止血策略
        
        参数:
            incident_context: {
                "recent_changes": bool,  # 是否有最近变更
                "symptoms": str,  # 症状类型
                "time_elapsed": int,  # 已经过时间(分钟)
                "severity": str  # P0/P1/P2/P3
            }
        
        返回:
            推荐的止血策略及执行计划
        """
        recommended_strategies = []
        
        # 规则1: 有最近变更 → 优先回滚
        if incident_context.get("recent_changes"):
            recommended_strategies.append({
                "strategy": "rollback",
                "reason": "检测到最近变更，回滚风险最低",
                "confidence": 0.95
            })
            
        # 规则2: 内存/资源类症状 → 重启
        if any(symptom in incident_context.get("symptoms", "") 
               for symptom in ["OOMKilled", "memory leak", "hang"]):
            recommended_strategies.append({
                "strategy": "restart",
                "reason": "资源类问题，重启可快速恢复",
                "confidence": 0.70
            })
            
        # 规则3: 容量不足 → 扩容
        if "capacity" in incident_context.get("symptoms", ""):
            recommended_strategies.append({
                "strategy": "emergency_scale",
                "reason": "容量不足，扩容可缓解压力",
                "confidence": 0.80
            })
            
        # 规则4: 区域问题 → 流量转移
        if "zone failure" in incident_context.get("symptoms", ""):
            recommended_strategies.append({
                "strategy": "traffic_diversion",
                "reason": "区域问题，切换到备用区域",
                "confidence": 0.90
            })
            
        # 规则5: 时间紧迫 → 选择最快的策略
        if incident_context.get("time_elapsed", 0) > 20:
            # 超过20分钟还未恢复，选择最快的可行策略
            recommended_strategies.sort(key=lambda x: self.strategies[x["strategy"]]["time"])
            
        # 规则6: P0事故 → 避免高风险操作
        if incident_context.get("severity") == "P0":
            recommended_strategies = [s for s in recommended_strategies 
                                     if self.strategies[s["strategy"]]["risk"] != "high"]
            
        # 按优先级和置信度排序
        recommended_strategies.sort(
            key=lambda x: (
                self.strategies[x["strategy"]]["priority"],
                -x["confidence"]
            )
        )
        
        return {
            "primary_strategy": recommended_strategies[0] if recommended_strategies else None,
            "fallback_strategies": recommended_strategies[1:3] if len(recommended_strategies) > 1 else [],
            "execution_plan": self._generate_execution_plan(recommended_strategies[0]) if recommended_strategies else None
        }
        
    def _generate_execution_plan(self, strategy_info):
        """生成执行计划"""
        strategy = strategy_info["strategy"]
        
        plans = {
            "rollback": {
                "steps": [
                    "1. 确认回滚目标版本",
                    "2. 执行回滚命令",
                    "3. 等待Pod重启完成",
                    "4. 验证服务健康"
                ],
                "estimated_time": "10分钟",
                "rollback_plan": "如果回滚无效，立即重新部署当前版本"
            },
            "restart": {
                "steps": [
                    "1. 记录当前状态快照",
                    "2. 执行滚动重启",
                    "3. 监控重启进度",
                    "4. 验证服务恢复"
                ],
                "estimated_time": "5分钟",
                "rollback_plan": "如果重启后仍异常，考虑回滚版本"
            },
            # 其他策略...
        }
        
        return plans.get(strategy, {})
```

#### 📝 最小示例

**止血策略实战案例**

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# stopbleeding-example.sh - 止血策略实战

echo "🩹 止血策略实战案例"
echo "场景: 新版本部署后，错误率从0.1%升至15%"
echo ""

# ========== 决策过程 ==========
echo "📋 止血策略决策"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 收集上下文
RECENT_DEPLOYMENT=$(kubectl rollout history deployment/app | tail -2 | head -1)
CURRENT_ERROR_RATE=15
NORMAL_ERROR_RATE=0.1
TIME_SINCE_DEPLOY=10  # 分钟

echo "上下文信息:"
echo "  - 最近变更: 10分钟前部署了新版本"
echo "  - 错误率: $NORMAL_ERROR_RATE% → $CURRENT_ERROR_RATE%"
echo "  - 已持续时间: ${TIME_SINCE_DEPLOY}分钟"
echo ""

# 决策: 回滚是最优选择
echo "💡 策略决策:"
echo "  ✓ 检测到最近变更 → 回滚(优先级1)"
echo "  ✓ 错误率激增 150倍 → 确认为变更导致"
echo "  ✓ 时间充足 (还有20分钟达到30分钟窗口)"
echo ""
echo "🎯 选择策略: 快速回滚"
echo "  - 预计耗时: 10分钟"
echo "  - 成功率: 95%"
echo "  - 风险: 低"
echo ""

# ========== 执行回滚 ==========
echo "🔄 执行回滚"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"

# 步骤1: 确认回滚目标
echo "[步骤1/4] 确认回滚目标版本..."
kubectl rollout history deployment/app
PREVIOUS_REVISION=$(kubectl rollout history deployment/app | tail -3 | head -1 | awk '{print $1}')
echo "  → 回滚到 revision $PREVIOUS_REVISION"
echo ""

# 步骤2: 执行回滚
echo "[步骤2/4] 执行回滚..."
kubectl rollout undo deployment/app --to-revision=$PREVIOUS_REVISION
echo "  → 回滚命令已执行"
echo ""

# 步骤3: 等待完成
echo "[步骤3/4] 等待Pod重启..."
kubectl rollout status deployment/app --timeout=5m
echo "  → 所有Pod已重启完成"
echo ""

# 步骤4: 验证恢复
echo "[步骤4/4] 验证服务恢复..."
sleep 30  # 等待指标稳定

# 模拟检查错误率
NEW_ERROR_RATE=0.1
echo "  → 当前错误率: $NEW_ERROR_RATE%"

if (( $(echo "$NEW_ERROR_RATE < 1" | bc -l) )); then
  echo "  ✅ 服务已恢复正常"
  RECOVERY_STATUS="success"
else
  echo "  ⚠️ 错误率仍然偏高，需要进一步排查"
  RECOVERY_STATUS="partial"
fi

echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "📊 止血结果"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "策略: 快速回滚"
echo "执行时间: 10分钟"
echo "恢复状态: $RECOVERY_STATUS"
echo ""
echo "下一步:"
if [ "$RECOVERY_STATUS" = "success" ]; then
  echo "  1. 持续监控30分钟"
  echo "  2. 分析新版本问题根因"
  echo "  3. 修复后重新部署"
  echo "  4. 安排事后复盘"
else
  echo "  1. 启动备选策略: 流量降级"
  echo "  2. 深入排查是否有其他问题"
fi
```
#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 必须找到根因才能修复 | ✅ 先止血再查根因 | 30分钟内优先恢复服务 |
| ❌ 追求完美的永久性方案 | ✅ 临时方案先止血 | 临时方案快速止血，永久方案慢慢优化 |
| ❌ 担心回滚会丢数据 | ✅ 评估回滚风险 | 大多数情况下回滚是安全的 |
| ❌ 同时尝试多个策略 | ✅ 按优先级逐个尝试 | 避免多个操作互相干扰 |
| ❌ 止血后立即关闭事故 | ✅ 观察稳定性再关闭 | 防止"假恢复"，确保系统真正稳定 |
| ❌ 不记录止血操作 | ✅ 实时记录所有操作 | 记录是复盘的重要输入 |

---

### 3.4 应急决策树

```yaml
# 应急决策矩阵
decisionMatrix:
  api-server-down:
    priority: P0
    first-action: "检查 etcd 健康状态"
    escalation: "如果 etcd 正常，重启 API Server；如果 etcd 异常，启动灾备切换"
    rollback-plan: "从最近备份恢复 etcd"
    
  etcd-quorum-lost:
    priority: P0
    first-action: "停止写入，保护现有数据"
    escalation: "联系数据库专家，评估数据恢复可能性"
    rollback-plan: "从最近快照恢复"
    
  network-partition:
    priority: P0/P1
    first-action: "识别隔离的节点或可用区"
    escalation: "如果影响 >30% 节点，触发多可用区切换"
    rollback-plan: "恢复网络连接，重新调度 Pod"
    
  storage-failure:
    priority: P1
    first-action: "识别受影响的 PVC"
    escalation: "评估数据恢复可能性，准备从备份恢复"
    rollback-plan: "切换到备用存储系统"
```

---

## 4. War Room 组织

> **🔰 初学者导读**: War Room是严重事故时的"作战指挥部"，所有相关人员集中沟通协调。需要明确角色：事故指挥官(IC)统筹全局，通信协调员(Comms)对外沟通，技术负责人(Tech Lead)定位修复。

### 4.1 War Room 角色定义

| 角色 | 职责 | 权限 | 技能要求 | 参与条件 |
|------|------|------|----------|----------|
| **事故指挥官** (Incident Commander) | 统一指挥、决策制定、资源调度 | 可以调动任何资源 | 5年+运维经验、决断力强 | P0必须参与 |
| **技术负责人** (Tech Lead) | 技术诊断、方案制定、执行监督 | 可以修改生产配置 | 架构设计能力、深入代码理解 | P0/P1参与 |
| **运维工程师** (SRE) | 执行操作、数据收集、监控跟踪 | 执行权限 | 熟悉运维工具链 | 所有级别 |
| **开发工程师** (Developer) | 代码分析、热修复、配置调整 | 代码提交权限 | 熟悉业务逻辑 | 需要时加入 |
| **沟通协调员** (Communications Lead) | 内外沟通、状态更新、记录整理 | 对外通知权限 | 沟通能力强 | P0/P1参与 |
| **产品负责人** (Product Owner) | 业务影响评估、降级决策、用户通知 | 业务决策权 | 业务理解深刻 | 需要时加入 |

### 4.2 War Room 运作规范

```yaml
# War Room 会议规范
warRoomProtocol:
  meeting-setup:
    platform: "Zoom/Teams 专用应急会议室"
    duration: "持续到问题解决"
    recording: "必须录制，用于事后复盘"
    
  communication-rules:
    - "禁止责备个人（Blameless Culture）"
    - "技术讨论优先，避免无关话题"
    - "每15分钟同步一次状态"
    - "决策由 Incident Commander 最终确定"
    - "所有重大操作必须先沟通后执行"
    
  status-update:
    internal-update: "每30分钟向管理层汇报"
    external-update: "每小时向客户发布状态页更新"
    documentation: "实时记录到事故文档"
    
  handoff-procedure:
    condition: "超过4小时持续作战"
    process: "完整的知识转移、文档交接"
    overlap: "至少30分钟重叠时间"
```

---

## 5. 通用应急手册

> **🔰 初学者导读**: Runbook是预先编写的"应急操作手册"——出了某种问题，按照手册一步步操作即可。就像飞行员的"紧急情况检查单"，不依赖个人记忆和经验。

### 5.1 控制平面问题应急手册

#### Runbook: API Server 不可用

**问题现象**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubectl 命令失败
$ kubectl get nodes
The connection to the server xxx:6443 was refused - did you specify the right host or port?

# API Server 健康检查失败
$ curl -k https://192.168.1.10:6443/healthz
curl: (7) Failed to connect to 192.168.1.10 port 6443: Connection refused
```
**快速诊断**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 API Server 进程
ssh master-node-1
ps aux | grep kube-apiserver

# 2. 检查 API Server 日志
journalctl -u kube-apiserver -n 200 --no-pager | tail -50

# 3. 检查 etcd 连接性
ETCDCTL_API=3 etcdctl endpoint health \
  --endpoints=https://192.168.1.10:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 4. 检查证书有效期
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates
```
**恢复步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 方案A: 重启 API Server（首选）
systemctl restart kube-apiserver
sleep 10
kubectl get cs

# 方案B: 如果重启无效，检查配置文件
cat /etc/kubernetes/manifests/kube-apiserver.yaml
# 查找最近的配置变更
git log -5 --oneline /etc/kubernetes/manifests/

# 方案C: 从备份恢复配置
cp /backup/kube-apiserver.yaml.$(date +%Y%m%d) /etc/kubernetes/manifests/kube-apiserver.yaml

# 方案D: 切换到备用 API Server
kubectl config set-cluster prod --server=https://backup-apiserver:6443
```
**验证恢复**
``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 验证 API Server 可达
kubectl get --raw='/healthz?verbose'

# 验证基本功能
kubectl get nodes
kubectl get pods -A
kubectl run test-nginx --image=nginx --restart=Never
kubectl delete pod test-nginx
```
#### Runbook: etcd 集群问题

**问题现象**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# etcd 健康检查失败
ETCDCTL_API=3 etcdctl endpoint health
https://192.168.1.10:2379 is unhealthy: failed to connect
```
**快速诊断**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 检查 etcd 成员状态
ETCDCTL_API=3 etcdctl member list \
  --write-out=table \
  --endpoints=https://192.168.1.10:2379,https://192.168.1.11:2379,https://192.168.1.12:2379

# 2. 检查磁盘空间
df -h /var/lib/etcd

# 3. 检查 etcd 日志
journalctl -u etcd -n 100 --no-pager | grep -E "error|warning|critical"

# 4. 检查网络延迟
for host in 192.168.1.10 192.168.1.11 192.168.1.12; do
  ping -c 3 $host
done
```
**恢复步骤**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 场景1: 单个成员失败（3节点中1个失败）
# 移除问题成员
ETCDCTL_API=3 etcdctl member remove <member-id>
# 添加新成员
ETCDCTL_API=3 etcdctl member add etcd-3 --peer-urls=https://192.168.1.13:2380

# 场景2: 失去法定人数（Quorum Lost）
# ⚠️ 危险操作：强制重建集群
# 1. 备份现有数据
etcdctl snapshot save /backup/etcd-snapshot-$(date +%s).db
# 2. 从快照恢复
ETCDCTL_API=3 etcdctl snapshot restore /backup/etcd-snapshot-latest.db \
  --name=etcd-1 \
  --initial-cluster=etcd-1=https://192.168.1.10:2380 \
  --initial-advertise-peer-urls=https://192.168.1.10:2380 \
  --data-dir=/var/lib/etcd-restore

# 场景3: 磁盘空间不足
# 增加存储配额（临时措施）
etcdctl alarm disarm
etcdctl defrag --command-timeout=30s
# 清理旧版本数据
etcdctl compact $(etcdctl endpoint status --write-out="json" | jq -r '.[0].Status.raftIndex')
```
### 5.2 工作负载问题应急手册

#### Runbook: Pod 无法启动

**快速诊断脚本**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# pod-troubleshooting.sh - Pod 问题快速诊断工具

NAMESPACE=${1:-default}
POD_NAME=${2}

echo "=== Pod 基本信息 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o wide

echo "=== Pod 详细状态 ==="
kubectl describe pod $POD_NAME -n $NAMESPACE

echo "=== Pod Events ==="
kubectl get events -n $NAMESPACE --field-selector involvedObject.name=$POD_NAME

echo "=== 容器日志 ==="
for container in $(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.containers[*].name}'); do
  echo "--- Container: $container ---"
  kubectl logs $POD_NAME -n $NAMESPACE -c $container --tail=50
done

echo "=== 节点资源状况 ==="
NODE=$(kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.spec.nodeName}')
kubectl describe node $NODE | grep -A 10 "Allocated resources"

echo "=== 镜像拉取状态 ==="
kubectl get pod $POD_NAME -n $NAMESPACE -o jsonpath='{.status.containerStatuses[*].state}'
```
**常见故障恢复**
```yaml
# 场景1: ImagePullBackOff
# 检查镜像是否存在
docker pull <image-name>

# 检查 imagePullSecrets
kubectl get secret -n $NAMESPACE
kubectl create secret docker-registry regcred \
  --docker-server=<registry-url> \
  --docker-username=<username> \
  --docker-password=<password>

# 场景2: CrashLoopBackOff
# 查看崩溃日志
kubectl logs <pod> --previous

# 调整资源限制
kubectl set resources deployment/<name> \
  --limits=memory=512Mi,cpu=1000m \
  --requests=memory=256Mi,cpu=500m

# 场景3: Pending（资源不足）
# 查看节点资源
kubectl top nodes

# 驱逐低优先级 Pod
kubectl delete pod -n <namespace> <low-priority-pod>

# 扩展节点
kubectl scale deployment cluster-autoscaler --replicas=1
```

---

## 6. 特定场景 Runbook

> **🔰 初学者导读**: 不同问题场景需要不同的排查和修复步骤。这里按照K8s常见问题分类提供专用手册：节点问题、网络问题、存储问题、控制平面问题等。

### 6.1 网络问题应急手册

**问题类型：Service 无法访问**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# service-troubleshooting.sh

SERVICE_NAME=$1
NAMESPACE=${2:-default}

# 步骤1: 验证 Service 配置
echo "=== Service 配置 ==="
kubectl get svc $SERVICE_NAME -n $NAMESPACE -o yaml

# 步骤2: 检查 Endpoints
echo "=== Endpoints ==="
kubectl get endpoints $SERVICE_NAME -n $NAMESPACE

# 步骤3: 验证 Pod Selector
SELECTOR=$(kubectl get svc $SERVICE_NAME -n $NAMESPACE -o jsonpath='{.spec.selector}')
echo "=== 匹配的 Pods ==="
kubectl get pods -n $NAMESPACE -l "$SELECTOR"

# 步骤4: 测试服务连通性
POD=$(kubectl get pods -n $NAMESPACE -l "$SELECTOR" -o jsonpath='{.items[0].metadata.name}')
kubectl exec -it $POD -n $NAMESPACE -- curl -v $SERVICE_NAME:80

# 步骤5: 检查 NetworkPolicy
echo "=== Network Policies ==="
kubectl get networkpolicy -n $NAMESPACE

# 步骤6: DNS 解析测试
kubectl run dnsutils --image=tutum/dnsutils --restart=Never -- sleep 3600
kubectl exec dnsutils -- nslookup $SERVICE_NAME.$NAMESPACE.svc.cluster.local
kubectl delete pod dnsutils
```
### 6.2 存储问题应急手册

**问题类型：PVC Pending**

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
#!/bin/bash
# pvc-troubleshooting.sh

PVC_NAME=$1
NAMESPACE=${2:-default}

# 步骤1: 检查 PVC 状态
echo "=== PVC 状态 ==="
kubectl describe pvc $PVC_NAME -n $NAMESPACE

# 步骤2: 检查 StorageClass
SC=$(kubectl get pvc $PVC_NAME -n $NAMESPACE -o jsonpath='{.spec.storageClassName}')
echo "=== StorageClass: $SC ==="
kubectl describe storageclass $SC

# 步骤3: 检查 CSI Driver
echo "=== CSI Driver Pods ==="
kubectl get pods -n kube-system | grep csi

# 步骤4: 检查节点存储容量
echo "=== 节点存储情况 ==="
kubectl get nodes -o custom-columns=NAME:.metadata.name,CAPACITY:.status.capacity.ephemeral-storage,ALLOCATABLE:.status.allocatable.ephemeral-storage

# 步骤5: 手动创建 PV（临时方案）
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: PersistentVolume
metadata:
  name: manual-pv-${PVC_NAME}
spec:
  capacity:
    storage: 10Gi
  accessModes:
    - ReadWriteOnce
  persistentVolumeReclaimPolicy: Retain
  storageClassName: $SC
  hostPath:
    path: /mnt/data/${PVC_NAME}
EOF
```
---

## 7. 事后复盘机制

> **🔰 初学者导读**: 复盘(Postmortem)是事故处理中最有价值的环节——不是追责，而是系统性改进。核心原则："对事不对人"(Blameless)。Google SRE和Netflix都要求每次重大事故后48小时内完成复盘。

### 7.1 Postmortem 模板

```markdown
# 事故复盘报告

## 基本信息
- **事故编号**: INC-2026-0206-001
- **事故等级**: P0
- **发生时间**: 2026-02-06 14:32:15 UTC
- **恢复时间**: 2026-02-06 16:45:30 UTC
- **持续时长**: 2小时13分钟
- **影响范围**: 生产集群所有用户（约10万用户）

## 执行摘要
简要描述事故的业务影响、技术根因和主要改进措施（3-5句话）。

## 时间线
| 时间 | 事件 | 责任人 | 备注 |
|------|------|--------|------|
| 14:32 | Prometheus 告警：API Server 不可用 | 自动告警 | - |
| 14:35 | SRE 确认问题，启动 P0 响应 | Alice | - |
| 14:40 | 启动 War Room，通知管理层 | Bob | - |
| 14:50 | 诊断定位：etcd 磁盘空间不足 | Charlie | - |
| 15:10 | 扩容 etcd 磁盘，重启服务 | Alice | - |
| 15:30 | API Server 部分恢复 | - | 仍有延迟 |
| 16:00 | etcd 碎片整理完成 | Charlie | - |
| 16:30 | 服务完全恢复，性能正常 | - | - |
| 16:45 | 验证完成，关闭 War Room | Bob | - |

## 根因分析（5 Whys）
1. **为什么 API Server 不可用？**
   - 因为 etcd 无法写入数据

2. **为什么 etcd 无法写入数据？**
   - 因为 etcd 磁盘空间 100% 占用

3. **为什么磁盘空间 100% 占用？**
   - 因为历史数据未及时清理，持续写入导致

4. **为什么历史数据未及时清理？**
   - 因为自动压缩（auto-compaction）未配置

5. **为什么自动压缩未配置？**
   - 因为初始部署时使用默认配置，后续未优化

## 影响评估
- **用户影响**: 10万用户无法访问服务，估计业务损失 $50,000
- **系统影响**: 集群完全不可用，所有 kubectl 操作失败
- **数据影响**: 无数据丢失，事故期间的状态变更丢失

## 改进措施
| 措施 | 优先级 | 责任人 | 完成期限 | 状态 |
|------|--------|--------|----------|------|
| 配置 etcd 自动压缩（auto-compaction-retention=1h） | P0 | Alice | 2026-02-07 | ✅ 完成 |
| 添加 etcd 磁盘空间告警（阈值 70%） | P0 | Bob | 2026-02-07 | ✅ 完成 |
| 扩容 etcd 磁盘至 100GB | P1 | Charlie | 2026-02-10 | 🔄 进行中 |
| 编写 etcd 应急手册 | P1 | Alice | 2026-02-12 | 📝 待开始 |
| 定期进行 etcd 灾难恢复演练 | P2 | SRE Team | 每月一次 | 📝 待开始 |

## 经验教训
### 做得好的
- ✅ 告警及时触发，5分钟内启动应急响应
- ✅ War Room 组织高效，角色分工明确
- ✅ 诊断过程有序，快速定位根因
- ✅ 沟通透明，及时向用户通报进展

### 需要改进的
- ❌ 缺少磁盘空间监控告警
- ❌ etcd 配置未按照生产最佳实践设置
- ❌ 缺少定期的容量规划和审查
- ❌ 应急手册覆盖不全面

## 相关文档
- [etcd 运维最佳实践](../domain-01-cluster-fundamentals/05-etcd-operations.md)
- [控制平面故障排查](../domain-10-troubleshooting-diagnostics/02-control-plane-etcd-troubleshooting.md)
```

### 7.2 Blameless Postmortem 原则

```yaml
# 无责复盘文化原则
blamelessCulture:
  core-principles:
    - "聚焦系统问题，而非个人失误"
    - "假设每个人都尽力而为"
    - "从失败中学习，而非惩罚"
    - "鼓励透明分享，建立信任"
    
  复盘会议规范:
    参与者: "所有相关方，包括事故执行者"
    主持人: "中立第三方，非直接参与者"
    时长: "1-2小时"
    输出: "改进措施列表，责任人明确"
    
  禁止语言:
    - "为什么你没有..."
    - "你应该..."
    - "这是谁的错..."
    
  鼓励语言:
    - "我们可以如何改进系统..."
    - "下次遇到类似情况，我们可以..."
    - "这次事故让我们学到了..."
```

### 7.3 Blameless复盘详解

#### 🔰 初学者理解

Blameless复盘就像航空事故调查：

- **航空事故调查**: 不追究飞行员责任，只分析系统性问题（天气、机械、流程）
- **传统事故处理**: 找到"背锅侠"，罚款/辞退了事
- **Blameless复盘**: 假设每个人都尽力了，问题在于系统设计缺陷

**为什么要Blameless？**
- ❌ 追责文化：工程师隐瞒问题，下次更危险
- ✅ Blameless文化：工程师主动暴露问题，系统更安全

**真实案例**：
- Google SRE的一次事故中，工程师误删除了生产数据库
- 复盘结论：不是工程师的错，而是"删除命令没有二次确认机制"
- 改进措施：增加`--confirm`参数，避免误操作

#### 🔧 工作原理

**Blameless复盘框架**

```yaml
# Blameless复盘执行框架
blamelessFramework:
  # 阶段1: 准备阶段 (事故后24小时内)
  preparation:
    data_collection:
      - "收集所有日志、监控数据、聊天记录"
      - "整理时间线（精确到分钟）"
      - "记录所有参与者的视角"
    document_draft:
      - "事故负责人起草初版复盘文档"
      - "包含：时间线、根因分析、改进措施"
      - "分发给所有参与者审阅"
      
  # 阶段2: 复盘会议 (事故后48小时内)
  meeting:
    participants:
      required:
        - "事故指挥官(IC)"
        - "技术负责人"
        - "所有事故参与者"
        - "相关团队代表"
      optional:
        - "管理层（观察员）"
        - "其他团队学习者"
        
    facilitator:
      role: "中立主持人（非直接参与者）"
      responsibilities:
        - "引导讨论方向"
        - "防止追责言论"
        - "确保所有人发言"
        - "记录改进措施"
        
    agenda:
      - duration: "1-2小时"
      - sections:
          - "回顾时间线（20分钟）"
          - "根因分析（30分钟）"
          - "讨论改进措施（40分钟）"
          - "分配责任人和期限（10分钟）"
          
    ground_rules:
      禁止:
        - "❌ 为什么你当时没有..."
        - "❌ 你应该知道..."
        - "❌ 这明显是XX的错..."
        - "❌ 如果不是你..."
      鼓励:
        - "✅ 系统如何让这个错误发生？"
        - "✅ 我们可以如何改进流程？"
        - "✅ 下次遇到类似情况，我们可以..."
        - "✅ 这次事故暴露了哪些系统缺陷？"
        
  # 阶段3: 行动跟踪 (持续)
  action_tracking:
    measures_categorization:
      immediate: "48小时内完成（防止重复发生）"
      short_term: "2周内完成（降低影响）"
      long_term: "1个月内完成（系统性改进）"
      
    accountability:
      - "每个措施明确责任人"
      - "每周复盘会议检查进度"
      - "未完成措施需要说明原因"
      
    effectiveness_review:
      - "3个月后评估改进措施效果"
      - "统计同类事故是否减少"
      - "必要时调整改进方向"
```

**Blameless vs 传统复盘对比**

```python
# blameless_comparison.py - 对比示例

class TraditionalPostmortem:
    """传统追责式复盘"""
    def analyze_incident(self, incident):
        # 找到"罪魁祸首"
        culprit = self.find_person_to_blame(incident)
        
        # 处罚
        punishment = self.decide_punishment(culprit)
        
        # 结论：个人问题
        conclusion = f"{culprit}操作失误导致事故，给予{punishment}"
        
        # 结果：其他人学会隐藏问题
        culture_impact = "工程师不敢承认错误，下次隐瞒问题"
        
        return conclusion

class BlamelessPostmortem:
    """Blameless复盘"""
    def analyze_incident(self, incident):
        # 分析系统性问题
        system_issues = self.identify_system_gaps(incident)
        
        # 示例输出
        system_issues = [
            "删除命令缺少二次确认",
            "没有自动备份机制",
            "权限控制过于宽松",
            "缺少操作审计日志"
        ]
        
        # 改进措施
        improvements = [
            "增加危险操作二次确认",
            "配置每小时自动备份",
            "实施最小权限原则(RBAC)",
            "启用审计日志"
        ]
        
        # 结论：系统问题
        conclusion = "系统设计缺陷导致事故，已制定改进措施"
        
        # 结果：工程师主动暴露问题
        culture_impact = "工程师主动报告隐患，系统持续改进"
        
        return {
            "system_issues": system_issues,
            "improvements": improvements,
            "conclusion": conclusion,
            "culture_impact": culture_impact
        }
```

#### 📝 最小示例

**Blameless复盘会议脚本**

```bash
#!/bin/bash
# blameless-postmortem-meeting.sh - 复盘会议模拟

echo "🔍 Blameless复盘会议"
echo "事故: INC-2026-001 - etcd磁盘空间耗尽导致API Server不可用"
echo "参与者: Alice(IC), Bob(Tech Lead), Charlie(SRE), Dave(主持人)"
echo ""

# ========== 开场 ==========
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[主持人 Dave] 欢迎大家参加复盘会议"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "[Dave] 首先说明会议规则："
echo "  1. 本次复盘的目的是改进系统，不是追究个人责任"
echo "  2. 假设每个人都尽力而为"
echo "  3. 请避免使用'为什么你没有...'这类语言"
echo "  4. 鼓励从系统角度思考问题"
echo ""

# ========== 阶段1: 时间线回顾 ==========
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[阶段1] 时间线回顾 (20分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "[Alice] 我来回顾时间线："
echo "  14:32 - Prometheus触发告警: etcd响应缓慢"
echo "  14:35 - 我确认告警，开始诊断"
echo "  14:40 - 启动War Room"
echo "  14:50 - Bob定位到etcd磁盘空间100%"
echo "  15:10 - Charlie扩容磁盘并重启etcd"
echo "  15:30 - 服务开始恢复"
echo "  16:45 - 完全恢复，关闭War Room"
echo ""
echo "[Dave] 大家对时间线有补充吗？"
read -p "  (按回车继续)..." 

# ========== 阶段2: 根因分析（使用5 Whys）==========
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[阶段2] 根因分析 - 5 Whys (30分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "[Bob] 我们用5 Whys分析根因："
echo ""
echo "Why 1: 为什么API Server不可用？"
echo "  → 因为etcd无法写入数据"
echo ""
echo "Why 2: 为什么etcd无法写入？"
echo "  → 因为磁盘空间100%"
echo ""
echo "Why 3: 为什么磁盘满了？"
echo "  → 因为历史数据未清理"
echo ""
echo "Why 4: 为什么历史数据未清理？"
echo "  → 因为auto-compaction未配置"
echo ""
echo "Why 5: 为什么auto-compaction未配置？"
echo "  → 因为初始部署使用默认配置，后续未优化"
echo ""

# ========== 阶段3: 系统性问题识别 ==========
echo "[Dave] 我们从系统角度看，暴露了哪些问题？"
echo ""
echo "[Charlie] 我认为有以下系统性问题："
echo "  1. 缺少磁盘空间监控告警"
echo "  2. etcd配置未按生产最佳实践"
echo "  3. 没有定期的容量规划流程"
echo "  4. 应急手册覆盖不全"
echo ""
echo "[Alice] 补充一点："
echo "  5. 部署检查清单(Checklist)不完整"
echo "  6. 缺少定期演练"
echo ""
read -p "  (按回车继续)..." 

# ========== 阶段4: 改进措施讨论（避免追责）==========
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[阶段3] 改进措施讨论 (40分钟)"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "[Dave] 针对这些系统问题，我们如何改进？"
echo ""
echo "[Bob] 我建议："
echo "  措施1: 立即配置etcd auto-compaction"
echo "    - 责任人: Alice"
echo "    - 期限: 明天完成"
echo "    - 优先级: P0"
echo ""
echo "[Charlie] 我建议："
echo "  措施2: 添加磁盘空间告警（阈值70%）"
echo "    - 责任人: Charlie"
echo "    - 期限: 本周五"
echo "    - 优先级: P0"
echo ""
echo "[Alice] 我建议："
echo "  措施3: 编写etcd运维最佳实践文档"
echo "    - 责任人: Bob"
echo "    - 期限: 下周"
echo "    - 优先级: P1"
echo ""
echo "  措施4: 每月进行etcd故障演练"
echo "    - 责任人: SRE团队"
echo "    - 期限: 长期"
echo "    - 优先级: P2"
echo ""
read -p "  (按回车继续)..." 

# ========== 阶段5: 经验教训总结 ==========
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[阶段4] 经验教训总结"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "[Dave] 这次事故中，我们做得好的地方："
echo "  ✅ 告警及时，5分钟内启动响应"
echo "  ✅ War Room组织高效"
echo "  ✅ 诊断过程有序"
echo ""
echo "[Dave] 需要改进的地方（系统层面）："
echo "  ❌ 监控覆盖不全（缺少磁盘告警）"
echo "  ❌ 配置管理不规范（未按最佳实践）"
echo "  ❌ 缺少定期审查机制"
echo ""

# ========== 会议总结 ==========
echo ""
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "[会议总结]"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "[Dave] 总结："
echo "  - 识别了6个系统性问题"
echo "  - 制定了4项改进措施，明确了责任人和期限"
echo "  - 复盘文档将在今天发布到Wiki"
echo "  - 下周五检查改进措施进度"
echo ""
echo "[Dave] 感谢大家的参与！记住：我们改进系统，不是追究个人。"
echo "✅ 复盘会议结束"
```

#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 名为Blameless实为追责 | ✅ 真正从系统角度分析 | 如果工程师感到被指责，就不是真正的Blameless |
| ❌ 复盘文档写完就束之高阁 | ✅ 跟踪改进措施落实 | 改进措施不落实，复盘就是浪费时间 |
| ❌ 只有P0事故才复盘 | ✅ P1/P2也应该复盘 | 小事故也能学到东西，及早改进 |
| ❌ 复盘会议变成技术深挖 | ✅ 聚焦系统改进 | 技术细节留给技术会议，复盘重点是改进 |
| ❌ 管理层主导复盘 | ✅ 技术人员主导 | 管理层可以参与，但不应主导，避免追责氛围 |
| ❌ 改进措施太宏大 | ✅ 措施具体可执行 | "加强监控"太空泛，"添加X指标Y阈值告警"才具体 |

---

### 7.4 5 Whys分析法

#### 🔰 初学者理解

5 Whys就像追问到底的侦探：

- **表面现象**: 网站打不开
- **Why 1**: 为什么网站打不开？→ 因为服务器宕机了
- **Why 2**: 为什么服务器宕机？→ 因为磁盘满了
- **Why 3**: 为什么磁盘满了？→ 因为日志没清理
- **Why 4**: 为什么日志没清理？→ 因为没配置日志轮转
- **Why 5**: 为什么没配置日志轮转？→ 因为部署检查清单里没有这一项

**关键**: 不要停在表面原因，一直追问到系统性根因。

#### 🔧 工作原理

**5 Whys分析框架**

```yaml
# 5 Whys分析方法论
fiveWhysFramework:
  principles:
    - "每一层Why都应该是客观事实，不是猜测"
    - "通常3-5个Why就能找到根因"
    - "不一定非要5个，到了根因就停止"
    - "可能有多个分支，需要都分析"
    
  how_to_identify_root_cause:
    - "到达根因的标志: 再问Why，答案是'因为我们的流程/系统设计如此'"
    - "根因通常是: 流程缺失、配置错误、监控盲区、设计缺陷"
    - "不是根因: 某个人的失误、硬件问题（除非没有冗余）"
    
  common_patterns:
    configuration_error:
      - "Why: 配置错误"
      - "Why: 没有验证配置"
      - "Why: 缺少配置校验机制"
      - "Why: 部署流程不规范"
      
    monitoring_gap:
      - "Why: 问题未及时发现"
      - "Why: 没有监控告警"
      - "Why: 监控指标覆盖不全"
      - "Why: 缺少监控规划流程"
      
    capacity_issue:
      - "Why: 资源耗尽"
      - "Why: 没有容量规划"
      - "Why: 缺少增长预测"
      - "Why: 没有定期容量审查机制"
```

**5 Whys实战模板**

```python
# five_whys_analyzer.py - 5 Whys分析工具

class FiveWhysAnalyzer:
    def __init__(self, symptom):
        self.symptom = symptom
        self.why_chain = []
        self.root_causes = []
        
    def ask_why(self, current_problem, answer):
        """
        记录一个Why-Answer对
        
        参数:
            current_problem: 当前问题
            answer: 答案（下一层的问题）
        """
        self.why_chain.append({
            "level": len(self.why_chain) + 1,
            "question": f"为什么{current_problem}？",
            "answer": answer
        })
        
        # 判断是否到达根因
        if self.is_root_cause(answer):
            self.root_causes.append(answer)
            return True
        return False
        
    def is_root_cause(self, answer):
        """
        判断是否为根因
        
        根因特征:
        - 涉及流程/制度/设计
        - 再问Why答案是"因为我们的系统就是这样设计的"
        """
        root_cause_keywords = [
            "缺少", "没有", "未配置", "未设置",
            "流程", "机制", "制度", "规范",
            "设计", "架构"
        ]
        return any(keyword in answer for keyword in root_cause_keywords)
        
    def generate_report(self):
        """生成分析报告"""
        report = []
        report.append("## 5 Whys 根因分析\n")
        report.append(f"**问题现象**: {self.symptom}\n")
        
        for item in self.why_chain:
            report.append(f"\n### Why {item['level']}: {item['question']}")
            report.append(f"**答案**: {item['answer']}\n")
            
        report.append("\n## 根因")
        for idx, root_cause in enumerate(self.root_causes, 1):
            report.append(f"{idx}. {root_cause}")
            
        return "\n".join(report)

# ========== 使用示例 ==========
# 案例1: API Server不可用
analyzer = FiveWhysAnalyzer("API Server不可用，所有kubectl命令失败")

# Why 1
analyzer.ask_why(
    "API Server不可用",
    "etcd无法写入数据"
)

# Why 2
analyzer.ask_why(
    "etcd无法写入数据",
    "etcd磁盘空间100%占用"
)

# Why 3
analyzer.ask_why(
    "etcd磁盘空间100%占用",
    "历史数据未及时清理"
)

# Why 4
analyzer.ask_why(
    "历史数据未及时清理",
    "etcd auto-compaction未配置"
)

# Why 5 (到达根因)
analyzer.ask_why(
    "etcd auto-compaction未配置",
    "初始部署使用默认配置，缺少生产环境配置审查机制"
)

# 生成报告
print(analyzer.generate_report())
```

#### 📝 最小示例

**5 Whys分析实战**

```bash
#!/bin/bash
# five-whys-analysis.sh - 5 Whys分析演练

echo "🔍 5 Whys根因分析演练"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""

# ========== 案例: Pod频繁OOMKilled ==========
echo "📋 问题现象: 生产环境Pod频繁OOMKilled，每小时重启10+次"
echo ""

# Why 1
echo "❓ Why 1: 为什么Pod频繁OOMKilled？"
read -p "   思考10秒... (按回车查看答案)" 
echo "   ✅ 答案: 因为Pod内存使用超过了limits限制"
echo ""

# Why 2
echo "❓ Why 2: 为什么Pod内存使用超过limits？"
read -p "   思考10秒... (按回车查看答案)" 
echo "   ✅ 答案: 因为应用内存泄漏，随着运行时间增长不断占用内存"
echo ""

# Why 3
echo "❓ Why 3: 为什么应用内存泄漏没有被及时发现？"
read -p "   思考10秒... (按回车查看答案)" 
echo "   ✅ 答案: 因为没有监控应用内存增长趋势"
echo ""

# Why 4
echo "❓ Why 4: 为什么没有监控内存增长趋势？"
read -p "   思考10秒... (按回车查看答案)" 
echo "   ✅ 答案: 因为Prometheus只配置了瞬时内存使用率告警，没有配置增长率告警"
echo ""

# Why 5
echo "❓ Why 5: 为什么没有配置增长率告警？"
read -p "   思考10秒... (按回车查看答案)" 
echo "   ✅ 答案: 因为告警规则模板不完整，缺少内存泄漏检测最佳实践"
echo ""

# 根因总结
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🎯 根因识别"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "根因: 告警规则模板不完整，缺少内存泄漏检测最佳实践"
echo ""
echo "判断依据:"
echo "  ✓ 涉及'缺少'、'不完整'等关键词"
echo "  ✓ 指向系统性问题（监控体系）"
echo "  ✓ 再问Why答案是'因为我们的监控体系就是这样设计的'"
echo ""

# 改进措施
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "🔧 改进措施"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "措施1: 完善告警规则模板"
echo "  - 添加内存增长率告警: rate(container_memory_usage[5m]) > 10MB/min"
echo "  - 添加内存使用趋势预测: predict_linear(container_memory[1h], 3600) > limits"
echo ""
echo "措施2: 建立告警规则审查机制"
echo "  - 每个服务上线前必须配置完整告警"
echo "  - 使用Checklist确保覆盖所有维度"
echo ""
echo "措施3: 应用层面改进"
echo "  - 修复代码中的内存泄漏（短期）"
echo "  - 增加内存limits（临时措施）"
echo ""

# 多分支示例
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo "💡 提示: 5 Whys可能有多个分支"
echo "━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"
echo ""
echo "分支1: 监控告警链条"
echo "  Why 3 → Why 4 → Why 5: 监控体系问题"
echo ""
echo "分支2: 代码质量链条"
echo "  Why 2: 应用内存泄漏"
echo "    → Why: 代码中闭包引用未释放"
echo "    → Why: 代码审查未发现问题"
echo "    → Why: Code Review Checklist缺少内存管理检查项"
echo ""
echo "结论: 两个根因都要改进！"
```

#### ⚠️ 常见误区

| 误区 | 正确做法 | 说明 |
|------|----------|------|
| ❌ 第一个Why就停止 | ✅ 至少问到3-5层 | "因为服务器宕机"不是根因，要追问为什么会宕机 |
| ❌ 把"人的失误"当根因 | ✅ 追问系统为何允许失误 | "工程师误删"不是根因，"删除命令无确认"才是 |
| ❌ 每层Why都是猜测 | ✅ 基于事实和数据 | 每个答案应该有日志、监控数据支撑 |
| ❌ 只分析主要分支 | ✅ 多分支都要分析 | 一个问题可能有多个根因 |
| ❌ 到了根因继续问 | ✅ 识别根因后停止 | 过度分析没有意义 |
| ❌ 根因指向个人 | ✅ 根因指向系统 | 如果根因是"某人不够细心"，说明还没问到真正的根因 |

---

## 8. 持续改进实践

> **🔰 初学者导读**: 每次事故都是学习机会。持续改进包括：完善Runbook、增加监控告警、自动化恢复、定期演练。目标是"同类事故不再发生第二次"。

### 8.1 事故趋势分析

```yaml
# 事故统计与分析
incidentAnalytics:
  monthly-metrics:
    total-incidents: 42
    p0-count: 2
    p1-count: 8
    p2-count: 15
    p3-count: 17
    
  mttr-by-severity:
    p0-mttr: "2.5小时"
    p1-mttr: "4.2小时"
    p2-mttr: "6.8小时"
    
  top-failure-categories:
    - category: "配置错误"
      percentage: 35%
      trend: "上升 ⬆️"
      
    - category: "容量不足"
      percentage: 25%
      trend: "下降 ⬇️"
      
    - category: "依赖问题"
      percentage: 20%
      trend: "持平 ➡️"
      
    - category: "软件缺陷"
      percentage: 15%
      trend: "下降 ⬇️"
      
    - category: "人为操作"
      percentage: 5%
      trend: "下降 ⬇️"
```

### 8.2 预防性措施矩阵

| 问题类型 | 检测机制 | 预防措施 | 自动化程度 | 实施优先级 |
|----------|----------|----------|------------|------------|
| **配置错误** | GitOps 审批流程 | Policy-as-Code 校验 | 80% | P0 |
| **容量不足** | 容量规划仪表板 | 自动扩缩容 | 90% | P0 |
| **证书过期** | 证书监控告警 | 自动续期 | 95% | P0 |
| **依赖问题** | 依赖健康检查 | 熔断降级 | 70% | P1 |
| **版本兼容性** | CI/CD 兼容性测试 | 金丝雀发布 | 60% | P1 |
| **人为误操作** | 操作二次确认 | RBAC 最小权限 | 50% | P2 |

## 故障排查

> 本文件包含完整的事故管理与应急手册。详见上方各章节：1.事故管理框架、2.事故分级标准、3.应急响应流程、5.通用应急手册、6.特定场景 Runbook。

## 生产检查清单

- [ ] 事故分级标准已定义并全团队共识（P0-P4）
- [ ] On-call 轮换机制已建立，每个时段有主备值班
- [ ] War Room 组织流程已文档化
- [ ] 所有关键服务有对应的 Runbook
- [ ] 事故通信渠道（Slack/PagerDuty）已配置
- [ ] 事后复盘（Postmortem）模板和流程已标准化
- [ ] 改进措施有明确的 Owner 和 Deadline
- [ ] 定期（季度）进行事故演练

## 命令快速参考

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
# 快速查看集群异常事件
kubectl get events -A --sort-by=.metadata.creationTimestamp | tail -30

# 查看所有非正常 Pod
kubectl get pods -A | grep -vE 'Running|Completed'

# 快速 cordon 问题节点
kubectl cordon <node> && kubectl drain <node> --ignore-daemonsets --delete-emptydir-data

# 查看 API Server 审计日志
kubectl logs -n kube-system kube-apiserver-* --tail=100

# 检查控制平面组件健康
kubectl get componentstatuses

# 快速回滚 Deployment
kubectl rollout undo deployment/<name> -n <namespace>
```
## 交叉引用

- 相关主题：[生产故障排查手册](production-troubleshooting-playbook.md) · [故障模式分析](failure-patterns-analysis.md) · [SLI/SLO/SLA](sli-slo-sla-engineering.md) · [SRE 成熟度模型](sre-maturity-model.md) · [变更管理](change-management-release.md)

---

**表格底部标记**: Kusheet Project | 作者: Allen Galler (allengaller@gmail.com) | 最后更新: 2026-02 | 版本: v1.25-v1.32 | 质量等级: ⭐⭐⭐⭐⭐ 专家级

## 参考链接

- [Incident Management Runbooks]()

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
