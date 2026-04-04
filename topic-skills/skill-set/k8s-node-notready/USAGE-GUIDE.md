# Skills + FTA 使用指南 — k8s-node-notready & node-fta

> **范围**：`topic-skills/skill-set/k8s-node-notready/` ↔ `topic-fta/list/node-fta.md`  
> **受众**：AI Agent 运行时开发者、集成诊断系统的 SRE 团队，以及扩展知识库的贡献者  
> **版本**：1.0 — 2026-03

---

## 目录

1. [系统架构概述](#1-系统架构概述)
2. [文件清单与职责](#2-文件清单与职责)
3. [Skill 元系统 — 工作原理](#3-skill-元系统--工作原理)
4. [FTA 系统 — 工作原理](#4-fta-系统--工作原理)
5. [双向集成设计](#5-双向集成设计)
6. [Agent 执行流程详解](#6-agent-执行流程详解)
7. [关键数据契约](#7-关键数据契约)
8. [常见使用模式](#8-常见使用模式)
9. [扩展系统](#9-扩展系统)
10. [ID 参考表](#10-id-参考表)

---

## 1. 系统架构概述

诊断系统分为四层。Skill 与 FTA 位于第 3、4 层，二者深度集成：

```
┌────────────────────────────────────────────────────────────────────┐
│  第 4 层: topic-skills/              (做什么 — Agent 执行层)        │
│  自包含 Runbook: 触发 → 诊断 → 修复 → 验证                          │
├────────────────────────────────────────────────────────────────────┤
│  第 3 层: topic-fta/list/            (为什么 — 故障分析模型)         │
│  FTA 故障树: 概率模型、因果链、底事件                                 │
├────────────────────────────────────────────────────────────────────┤
│  第 2 层: topic-structural-trouble-shooting/  (如何做 — 深度参考)   │
├────────────────────────────────────────────────────────────────────┤
│  第 1 层: domain-*/                  (背景知识 — 理论/架构)          │
└────────────────────────────────────────────────────────────────────┘
```

**Skill 系统回答**：_应该采取什么操作？按照什么顺序？风险门控条件是什么？_  
**FTA 系统回答**：_节点为什么故障？通往根因的因果链是什么？_

两者双向关联：
- **Skill → FTA**：通过 `root-cause-map.yaml` 中的 `fta_mapping.step_ids` 和 `skill-metadata.yaml` 中的 `rc_to_fta_steps`
- **FTA → Skill**：通过 `node-fta.md` 每个底事件中的 `metadata.skill_ref`

---

## 2. 文件清单与职责

### 2.1 Skill 系统文件

```
topic-skills/skill-set/k8s-node-notready/
├── SKILL.md                          # 人机共用的主 Skill 文档
├── assets/
│   ├── skill-metadata.yaml           # 机器可解析的路由与 FTA 索引
│   ├── root-cause-map.yaml           # 含 FTA 映射的 RC 决策图
│   ├── symptom-patterns.yaml         # 症状识别的 NLP/模式匹配
│   └── escalation-template.md        # 升级消息模板
├── reference/
│   ├── root-cause-catalog.md         # 人类可读的 RC 参考文档
│   ├── remediation-playbook.md       # 详细修复流程（REM-*）
│   ├── diagnostic-workflow.md        # 扩展诊断步骤参考
│   └── version-matrix.md             # K8s v1.28–v1.32 兼容性矩阵
└── scripts/
    ├── diagnose-quick.sh             # 快速分诊诊断
    ├── diagnose-deep.sh              # 全面深度诊断
    ├── check-resources.sh            # 资源压力分析
    ├── cleanup-disk.sh               # 磁盘清理操作
    └── remediate.sh                  # 引导式修复运行器
```

### 2.2 FTA 系统文件

```
topic-fta/list/node-fta.md            # FTA 故障树主文档
  ├── Mermaid diagram                 # 可视化故障树（供人阅读）
  ├── Diagnostic command tables       # 按分类的快速命令参考
  └── JSON flow_steps array           # 机器可执行的诊断流程图
      ├── gate_root_or                # 入口门（跨所有分类的 OR 门）
      ├── cat_* steps                 # 分类门（kubelet、runtime、resource 等）
      ├── evt_* steps（底事件）        # 含 skill_ref 的叶级诊断步骤
      └── AND gates                   # 多条件事件
```

---

## 3. Skill 元系统 — 工作原理

### 3.1 skill-metadata.yaml — Agent 入口点

这是 Agent 调度器**首先读取的主路由配置**。关键部分如下：

**路由/触发块** — Agent 如何检测何时激活此 Skill：
```yaml
routing:
  trigger_keywords:
    cn: ["NotReady", "节点不可用", "节点异常", "节点不可达"]
    en: ["NotReady", "NodeNotReady", "kubelet stopped", "node unreachable"]
  trigger_events:
    - NodeNotReady
    - NodeStatusUnknown
    - NodeHasDiskPressure
    - NodeHasMemoryPressure
    - NodeHasPIDPressure
  trigger_metrics:
    - 'kube_node_status_condition{condition="Ready",status="false"}'
    - 'kube_node_status_condition{condition="Ready",status="unknown"}'
```

**FTA 引用块** — 从 RC ID 到 FTA 步骤 ID 的完整索引：
```yaml
references:
  fta:
    - file: "topic-fta/list/node-fta.md"
      fta_entry_step: "gate_root_or"       # Start here when running the FTA
      skill_ref_field: "metadata.skill_ref" # Field name for reverse link in FTA
      rc_to_fta_steps:
        RC-001: ["evt_kubelet_down", "evt_heartbeat_fail"]
        RC-002: ["evt_rt_down", "evt_cri_sock", "evt_rt_hang"]
        RC-003: ["evt_disk_pressure", "evt_image_gc_fail"]
        RC-004: ["evt_mem_pressure", "evt_and_mem_low", "evt_and_mem_nolimit"]
        RC-005: ["evt_pid_exhaust"]
        RC-006: ["evt_api_unreachable", "evt_policy_block", "evt_route_fail"]
        RC-007: ["evt_kubelet_cert", "evt_node_cert_expire"]
        RC-008: ["evt_pleg", "evt_and_pleg_timeout", "evt_and_pleg_overload"]
        RC-009: ["evt_kernel_panic", "evt_driver_issue"]
        RC-010: ["evt_time_skew_tls"]
        RC-011: ["evt_cni_fail"]
        RC-012: ["evt_cordon"]
```

### 3.2 root-cause-map.yaml — RC 决策图

每个 RC 条目具有标准结构。`fta_mapping` 字段将 RC 关联到对应的 FTA 底事件：

```yaml
- id: RC-001
  name:
    cn: "kubelet 进程崩溃或未运行"
    en: "kubelet process crashed or not running"
  probability: high
  diagnostic_evidence:
    primary: [D2.1, D2.2]       # References to diagnostic-workflow.md step IDs
    secondary: [D1.5]
  diagnostic_rules:
    - step: D2.1
      condition: "kubelet service not active/running"
      confidence: 0.9
  remediation:
    primary: REM-003             # Primary remediation in remediation-playbook.md
    alternatives: [REM-006]
  fta_mapping:                   # Bidirectional link to FTA
    file: "topic-fta/list/node-fta.md"
    step_ids: ["evt_kubelet_down", "evt_heartbeat_fail"]
  related_causes: [RC-008]       # Causes that may co-occur
```

### 3.3 SKILL.md — 人机共用文档

`SKILL.md` 是主 Skill 文件，分为 10 个章节：

| 章节 | 用途 | Agent 关键点 |
|------|------|-------------|
| YAML 前置元数据 | `skill_id`、`trigger_*`、`fta_refs` | 路由激活 |
| 第 2 节 | 症状模式表（S1–S8） | 基于置信度的匹配 |
| 第 3 节 | 快速分诊（T1–T3 步骤） | 影响范围评估 |
| 第 4 节 | 诊断工作流（D1.x–D3.x） | 逐步诊断 |
| 第 5 节 | 根因表（RC-001–RC-012） | 根因确认 |
| 第 6 节 | 修复操作（REM-001–REM-010） | 风险门控操作 |
| 第 7 节 | 验证序列（V1–V6） | 修复确认 |
| 第 8 节 | 升级协议 | 何时移交 |
| 第 9 节 | K8s 版本矩阵 | v1.28–v1.32 差异 |
| 第 10 节 | 知识演进 | 误诊模式 |

---

## 4. FTA 系统 — 工作原理

### 4.1 node-fta.md 的结构

该文件包含三个部分：

1. **Mermaid 图** — 人类可读的可视化故障树。入口：`TE[顶事件: Node异常]` → `OR0{{OR}}` → 9 个分类分支
2. **诊断命令表** — 按分类（kubelet、runtime、resource、network、storage、kernel、cert/time、control plane）归组的快速命令参考
3. **JSON `flow_steps` 数组** — 机器可执行的诊断流程图

### 4.2 JSON flow_steps — 步骤类型

`flow_steps` 数组中有四种步骤类型：

| 动作类型 | 示例 `step` ID | 用途 |
|---------|---------------|------|
| `gate_or` | `gate_root_or`、`cat_kubelet` | OR 门 — 任一子条件成立则向前路由 |
| `gate_and` | `evt_and_pleg_timeout`、`evt_and_mem_low` | AND 门 — 所有条件必须同时成立 |
| `bottom_event` | `evt_kubelet_down`、`evt_cni_fail` | 叶节点 — 实际诊断检查 |
| `category_gate` | `cat_nstat`、`cat_resource` | 顶级分类路由器 |

### 4.3 底事件结构

每个底事件是故障树中的叶节点，具有以下 JSON 结构：

```json
{
  "name": "底事件: kubelet 服务异常",
  "action": "bottom_event",
  "step": "evt_kubelet_down",
  "description": "kubelet 进程崩溃、无法启动或 OOM",
  "cmd": {
    "type": "sequence",
    "commands": [
      {
        "id": "check_kubelet_service",
        "description": "检查 kubelet 服务状态",
        "exec": "ssh ${NODE_NAME} 'systemctl status kubelet --no-pager -l | tail -20'",
        "timeout": "10s"
      }
    ]
  },
  "match": {
    "positive": ["inactive", "failed", "activating"],
    "negative": ["active (running)"]
  },
  "metadata": {
    "skill_ref": {
      "skill_id": "SKILL-NODE-001",
      "rc_id": "RC-001",
      "remediation_ids": ["REM-003"],
      "script": "scripts/diagnose-deep.sh"
    },
    "remediation": "...",
    "references": ["..."]
  }
}
```

### 4.4 skill_ref — 反向链接

`metadata.skill_ref` 字段是 **FTA → Skill 的反向链接**。当 Agent 确认某个底事件时，会读取 `skill_ref` 直接导航到正确的 RC 和修复步骤：

| 字段 | 类型 | 用途 |
|------|------|------|
| `skill_id` | string | 要激活的 Skill（`SKILL-NODE-001`） |
| `rc_id` | string | 已确认的根因 ID（`RC-001`–`RC-012`） |
| `remediation_ids` | array | 要执行的适用 REM ID |
| `script` | string | 推荐的诊断/修复脚本 |
| `cross_skill` | string | （可选）需要协同激活的次级 Skill |
| `is_fault` | boolean | （可选）`false` = 计划操作，非故障 |
| `note` | string | （可选）需要人工操作 |

---

## 5. 双向集成设计

### 5.1 正向方向：Skill → FTA

当 Agent 已识别出可能的根因（如 RC-001）并需要 FTA 诊断命令时：

```
skill-metadata.yaml
  └── rc_to_fta_steps["RC-001"] → ["evt_kubelet_down", "evt_heartbeat_fail"]
        ↓
root-cause-map.yaml RC-001
  └── fta_mapping.step_ids → ["evt_kubelet_down", "evt_heartbeat_fail"]
        ↓
node-fta.md flow_steps
  └── step "evt_kubelet_down" → cmd.commands → execute diagnostics
```

### 5.2 反向方向：FTA → Skill

当 FTA 诊断确认某个底事件，Agent 需要导航到修复步骤时：

```
node-fta.md flow_steps
  └── step "evt_kubelet_down"
        └── metadata.skill_ref.rc_id → "RC-001"
        └── metadata.skill_ref.remediation_ids → ["REM-003"]
              ↓
SKILL.md Section 6 / reference/remediation-playbook.md
  └── REM-003: Restart kubelet service (high risk, requires approval)
```

### 5.3 集成映射图 — 全部 12 个根因

| RC ID | 根因 | FTA 步骤 ID | 主修复项 |
|-------|-----------|--------------|-------------|
| RC-001 | kubelet 进程崩溃 | `evt_kubelet_down`, `evt_heartbeat_fail` | REM-003 |
| RC-002 | 容器运行时异常 | `evt_rt_down`, `evt_cri_sock`, `evt_rt_hang` | REM-004 |
| RC-003 | 磁盘压力/镜像GC失败 | `evt_disk_pressure`, `evt_image_gc_fail` | REM-002, REM-005 |
| RC-004 | 内存压力/驱逐 | `evt_mem_pressure`, `evt_and_mem_low`, `evt_and_mem_nolimit` | REM-005, REM-006 |
| RC-005 | PID 耗尽 | `evt_pid_exhaust` | REM-005, REM-006 |
| RC-006 | 节点与 API Server 不通 | `evt_api_unreachable`, `evt_policy_block`, `evt_route_fail` | manual |
| RC-007 | kubelet 证书过期 | `evt_kubelet_cert`, `evt_node_cert_expire` | REM-008 + SKILL-SEC-001 |
| RC-008 | PLEG 不健康 | `evt_pleg`, `evt_and_pleg_timeout`, `evt_and_pleg_overload` | REM-003, REM-004 |
| RC-009 | 内核崩溃/驱动异常 | `evt_kernel_panic`, `evt_driver_issue` | REM-006, REM-009, REM-010 |
| RC-010 | 时间同步失败/TLS | `evt_time_skew_tls` | manual NTP fix |
| RC-011 | CNI 组件异常 | `evt_cni_fail` | manual CNI redeploy |
| RC-012 | 节点被 cordon（非故障） | `evt_cordon` | REM-001 (is_fault: false) |

---

## 6. Agent 执行流程详解

### 6.1 完整执行流程

```
1. 触发（TRIGGER）
   告警: kube_node_status_condition{condition="Ready",status="false"} > 0
   或
   事件: 集群接收到 NodeNotReady
         ↓
2. 路由（ROUTING）
   读取 skill-metadata.yaml → 匹配 trigger_keywords / trigger_events / trigger_metrics
   → 激活 SKILL-NODE-001（k8s-node-notready）
         ↓
3. 分诊（TRIAGE）（SKILL.md 第 3 节）
   执行 T1–T3 只读 kubectl 命令
   → 确定受影响节点数、Pod 驱逐数、严重程度 P0/P1/P2/P3
   → 检查第 3.3 节：立即升级条件（若触发则跳到第 7 步）
         ↓
4. 诊断（DIAGNOSIS）（SKILL.md 第 4 节 / diagnostic-workflow.md）
   阶段 1：快速检查 — kubectl、节点条件、Events（D1.1–D1.6）
   阶段 2：深度检查 — SSH 到节点、systemd、日志、资源统计（D2.1–D2.8）
   阶段 3：主动探测 — 网络连通性、存储健康状态（D3.1–D3.3）
   → 将证据与 root-cause-map.yaml 的 diagnostic_rules 比对
   → 识别最可能的 RC（如 RC-001，置信度 0.9）
         ↓
5. FTA 验证（可选深度确认）
   读取 root-cause-map.yaml RC-001 → fta_mapping.step_ids → ["evt_kubelet_down", ...]
   或使用 skill-metadata.yaml rc_to_fta_steps["RC-001"] → 相同列表
   执行 node-fta.md flow_steps 中的 "evt_kubelet_down"：
     → 运行 cmd.commands
     → 检查 match.positive / match.negative 模式
     → 读取 metadata.skill_ref → 确认 RC-001，remediation_ids: ["REM-003"]
         ↓
6. 修复（REMEDIATION）（SKILL.md 第 6 节 / remediation-playbook.md）
   查找 REM-003：重启 kubelet
   → 风险等级：🔴 高 → 在 L1-advisory 模式下请求人工审批
   → 审批通过后：运行 scripts/diagnose-deep.sh 然后执行修复
   → 运行修复后验证（第 7 节中的 V1–V6）
         ↓
7. 升级（ESCALATION）（如适用）
   使用 escalation-template.md 生成结构化移交消息
   包含：已完成的诊断步骤、已确认发现、YAML 快照、事件时间线
         ↓
8. 关闭/验证（CLOSE / VERIFY）
   确认节点 Ready 状态（V1）
   按第 7.2 节监控 5–15 分钟
   按第 7.4 节检查回归检测项
```

### 6.2 短路：直接进入 FTA

当 Agent 有强先验证据时（如告警标签 = `disk_pressure`），可跳过完整诊断，直接进入 FTA：

```python
# 伪代码
fta_steps = skill_metadata["rc_to_fta_steps"]["RC-003"]  # ["evt_disk_pressure", "evt_image_gc_fail"]
for step_id in fta_steps:
    step = fta_flow_steps[step_id]
    results = execute(step["cmd"]["commands"])
    if matches(results, step["match"]["positive"]):
        skill_ref = step["metadata"]["skill_ref"]
        activate_remediation(skill_ref["skill_id"], skill_ref["rc_id"], skill_ref["remediation_ids"])
        run_script(skill_ref["script"])
        break
```

### 6.3 跨 Skill 导航

RC-007（证书过期）和 RC-010（时间偏差/TLS）链接到 `SKILL-SEC-001`。`skill_ref` 中的 `cross_skill` 字段标记此情况：

```json
"skill_ref": {
  "skill_id": "SKILL-NODE-001",
  "rc_id": "RC-007",
  "remediation_ids": ["REM-008"],
  "cross_skill": "SKILL-SEC-001",
  "script": "scripts/diagnose-deep.sh"
}
```

当 `cross_skill` 存在时，Agent 应并行协同激活 `SKILL-SEC-001`（certificate-expiry）。

---

## 7. 关键数据契约

### 7.1 FTA 步骤 ID 格式

所有 FTA 步骤标识符遵循以下命名规范：

| 前缀 | 类型 | 示例 |
|------|------|------|
| `gate_root_` | 顶层 OR 门 | `gate_root_or` |
| `cat_` | 分类门 | `cat_kubelet`、`cat_runtime`、`cat_resource` |
| `evt_` | 底事件（叶节点） | `evt_kubelet_down`、`evt_cni_fail` |
| `evt_and_` | AND 条件节点 | `evt_and_pleg_timeout`、`evt_and_mem_low` |

### 7.2 ID 命名空间边界

| ID 类型 | 命名空间 | 格式 | 唯一范围 |
|---------|---------|------|----------|
| Skill ID | 全局 | `SKILL-{CAT}-{SEQ}` | 跨项目 |
| RC ID | 单个 Skill | `RC-{3digits}` | 在同一 Skill 文件内 |
| REM ID | 单个 Skill | `REM-{3digits}` | 在同一 Skill 文件内 |
| 诊断步骤 | 单个 Skill | `D{phase}.{seq}` | 在同一 Skill 文件内 |
| FTA 步骤 | 单个 FTA 文件 | `evt_*`、`cat_*`、`gate_*` | 在同一 FTA 文件内 |

### 7.3 fta_mapping Schema（root-cause-map.yaml）

```yaml
fta_mapping:
  file: "topic-fta/list/node-fta.md"   # 必填：相对于仓库根目录的路径
  step_ids: ["evt_*", ...]              # 必填：真实 FTA 步骤 ID 数组
  note: "..."                            # 可选：人工注释
```

### 7.4 skill_ref Schema（node-fta.md metadata）

```json
{
  "skill_id": "SKILL-NODE-001",          // 必填
  "rc_id": "RC-001",                     // 必填
  "remediation_ids": ["REM-003"],        // 必填（可为空数组）
  "script": "scripts/diagnose-deep.sh", // 必填
  "cross_skill": "SKILL-SEC-001",       // 可选
  "is_fault": false,                     // 可选，默认 true
  "note": "manual action required"       // 可选
}
```

---

## 8. 常见使用模式

### 模式 A：已知症状，查找对应的 FTA 步骤

```
1. 在 symptom-patterns.yaml 中搜索匹配的 NLP 模式
   → 返回带置信度评分的 RC 候选列表
2. 读取 root-cause-map.yaml 中排名最高的 RC 候选
   → rc.fta_mapping.step_ids 给出精确的 FTA 步骤 ID
3. 在 node-fta.md flow_steps 中执行这些步骤
```

### 模式 B：已有 Prometheus 告警，从顶部开始 FTA

```
1. 从步骤 "gate_root_or" 进入 node-fta.md
2. 根据告警标签跟随分类门（如 DiskPressure → cat_storage）
3. 执行底事件的 cmd.commands
4. 匹配成功后：读取 skill_ref → 导航至修复步骤
```

### 模式 C：已确认 RC，查找所有相关 FTA 证据

```
1. 读取 skill-metadata.yaml rc_to_fta_steps["RC-XXX"]
2. 数组中每个 step_id 代表该 RC 的一条 FTA 子路径
3. 与 root-cause-catalog.md 交叉对照获取人类可读解释
```

### 模式 D：人工理解某个 FTA 底事件

```
1. 在 node-fta.md 中找到对应的 evt_* 步骤
2. 读取 metadata.skill_ref.rc_id
3. 打开 reference/root-cause-catalog.md 中 RC-XXX 章节获取完整说明
4. 打开 reference/remediation-playbook.md 中 REM-XXX 获取详细修复步骤
```

---

## 9. 扩展系统

### 9.1 向现有 Skill 添加新根因

1. 在 `root-cause-map.yaml` 中添加新的 RC 条目，`fta_mapping.step_ids` 指向真实的 FTA `evt_*` 步骤 ID
2. 在 `root-cause-catalog.md` 汇总表中添加该 RC 行，并创建其详细章节
3. 在 `SKILL.md` 第 5 节的汇总表中添加该 RC 列
4. 若 FTA 步骤已存在：在 `node-fta.md` 对应步骤的 `metadata` 中添加 `skill_ref`
5. 若 FTA 步骤不存在：在 `node-fta.md` flow_steps 中添加新的 `evt_*` 步骤（含 `skill_ref`）
6. 更新 `skill-metadata.yaml` 的 `rc_to_fta_steps` 映射，加入新 RC

### 9.2 添加新的 FTA 底事件

1. 在 `node-fta.md` 的 `flow_steps` 中添加新步骤对象，`action: "bottom_event"`
2. 按 `evt_*` 命名规范为其分配唯一的 `step` ID
3. 包含 `cmd.commands`、`match.positive`、`match.negative` 和 `metadata.skill_ref`
4. 将其接入父分类门的 `next_steps` 数组
5. 在 `root-cause-map.yaml` 中将新 `evt_*` ID 添加至对应 RC 的 `fta_mapping.step_ids`
6. 更新 `skill-metadata.yaml` 中该 RC 的 `rc_to_fta_steps`

### 9.3 将此模式应用到新的 Skill/FTA 对

这里建立的模式可复制到任意 Skill + FTA 对：

1. 在 FTA 文件中：确保所有底事件使用 `evt_*` ID（而非描述性字符串）
2. 在 `root-cause-map.yaml` 中：将基于文本的 `fta_mapping` 替换为结构化的 `{file, step_ids}` 格式
3. 在 `skill-metadata.yaml` 中：在 `references.fta` 下添加 `rc_to_fta_steps`
4. 在 FTA 文件中：向每个底事件的 `metadata` 注入 `skill_ref`
5. 验证：每个 RC 至少有 `≥1` 个 FTA 步骤，每个被引用的 `evt_*` 必须在 FTA 中真实存在

---

## 10. ID 参考表

### 10.1 根因 ID（RC-001 – RC-012）

| RC ID | 名称 | 概率 | 主要 FTA 步骤 |
|-------|------|------|---------------|
| RC-001 | kubelet 进程崩溃 | 高 | `evt_kubelet_down`、`evt_heartbeat_fail` |
| RC-002 | 容器运行时异常 | 高 | `evt_rt_down`、`evt_cri_sock`、`evt_rt_hang` |
| RC-003 | 磁盘压力/镜像 GC 失败 | 中 | `evt_disk_pressure`、`evt_image_gc_fail` |
| RC-004 | 内存压力/驱逐 | 中 | `evt_mem_pressure`、`evt_and_mem_low`、`evt_and_mem_nolimit` |
| RC-005 | PID 耗尽 | 低 | `evt_pid_exhaust` |
| RC-006 | 节点无法访问 apiserver | 中 | `evt_api_unreachable`、`evt_policy_block`、`evt_route_fail` |
| RC-007 | kubelet 证书过期 | 低 | `evt_kubelet_cert`、`evt_node_cert_expire` |
| RC-008 | PLEG 不健康 | 中 | `evt_pleg`、`evt_and_pleg_timeout`、`evt_and_pleg_overload` |
| RC-009 | 内核崩溃/驱动异常 | 低 | `evt_kernel_panic`、`evt_driver_issue` |
| RC-010 | 时间偏差/TLS 失败 | 低 | `evt_time_skew_tls` |
| RC-011 | CNI 组件异常 | 低 | `evt_cni_fail` |
| RC-012 | 节点被 cordon（非故障） | — | `evt_cordon` |

### 10.2 修复 ID（REM-001 – REM-010）

| REM ID | 操作 | 风险 | 适用 RC |
|--------|------|------|----------|
| REM-001 | 解除节点 cordon | 低（绿色） | RC-012 |
| REM-002 | 磁盘清理（日志轮转、镜像清除） | 中（黄色） | RC-003 |
| REM-003 | 重启 kubelet 服务 | 高（红色） | RC-001、RC-008 |
| REM-004 | 重启容器运行时 | 高（红色） | RC-002、RC-008 |
| REM-005 | 资源限额调整/Pod 驱逐 | 中（黄色） | RC-003、RC-004、RC-005 |
| REM-006 | 节点排空并重新加入 | 高（红色） | RC-001、RC-002、RC-004、RC-005、RC-009 |
| REM-007 | 网络规则修复（iptables/ipvs 刷新） | 高（红色） | RC-006 |
| REM-008 | 证书续期（kubelet cert rotate） | 严重（黑色） | RC-007 → SKILL-SEC-001 |
| REM-009 | 节点重启 | 严重（黑色） | RC-009 |
| REM-010 | 操作系统重装/硬件替换 | 严重（黑色） | RC-009 |

### 10.3 FTA 底事件 ID

| 步骤 ID | 描述 | RC | 含 skill_ref |
|---------|------|-----|-------------|
| `evt_kubelet_down` | kubelet 服务崩溃/未运行 | RC-001 | ✓ |
| `evt_heartbeat_fail` | kubelet 心跳未到达 apiserver | RC-001 | ✓ |
| `evt_rt_down` | containerd/dockerd 服务停止 | RC-002 | ✓ |
| `evt_cri_sock` | CRI socket 不可用 | RC-002 | ✓ |
| `evt_rt_hang` | 运行时挂起/无响应 | RC-002 | ✓ |
| `evt_disk_pressure` | 磁盘使用率 > 驱逐阈值 | RC-003 | ✓ |
| `evt_image_gc_fail` | 镜像 GC 失败 | RC-003 | ✓ |
| `evt_mem_pressure` | 内存低于驱逐阈值 | RC-004 | ✓ |
| `evt_and_mem_low` | AND：内存不足（条件 1/2） | RC-004 | — |
| `evt_and_mem_nolimit` | AND：Pod 无内存限制（条件 2/2） | RC-004 | — |
| `evt_pid_exhaust` | PID 耗尽 | RC-005 | ✓ |
| `evt_api_unreachable` | 节点无法访问 kube-apiserver | RC-006 | ✓ |
| `evt_policy_block` | NetworkPolicy 阻断节点流量 | RC-006 | — |
| `evt_route_fail` | 路由表/iptables 故障 | RC-006 | — |
| `evt_kubelet_cert` | kubelet 客户端证书过期/无效 | RC-007 | ✓ |
| `evt_node_cert_expire` | 节点服务证书过期 | RC-007 | ✓ |
| `evt_pleg` | PLEG relist 错误 | RC-008 | ✓ |
| `evt_and_pleg_timeout` | AND：PLEG relist 超时（条件 1/2） | RC-008 | — |
| `evt_and_pleg_overload` | AND：容器过多/运行时过慢（条件 2/2） | RC-008 | — |
| `evt_kernel_panic` | 内核 panic/oops | RC-009 | ✓ |
| `evt_driver_issue` | 驱动或内核模块故障 | RC-009 | ✓ |
| `evt_time_skew_tls` | NTP 不同步导致 TLS 验证失败 | RC-010 | ✓ |
| `evt_cni_fail` | CNI DaemonSet 停止或配置错误 | RC-011 | ✓ |
| `evt_cordon` | 节点被有意 cordon | RC-012 | ✓（is_fault: false） |

### 10.4 诊断步骤 ID（Skill 第 4 节）

| 阶段 | 步骤 ID | 描述 |
|------|---------|------|
| 阶段 1（快速，只读） | D1.1–D1.6 | kubectl 检查：节点条件、Events、Lease、Pod 影响 |
| 阶段 2（深度，SSH） | D2.1–D2.8 | kubelet 服务、日志、运行时、磁盘、内存、PID、网络 |
| 阶段 3（主动探测） | D3.1–D3.3 | 网络连通性测试、证书有效性、NTP 检查 |

### 10.5 症状 ID 与验证 ID

| 症状 ID | S1–S8 | SKILL.md 第 2 节中的症状模式表 |
|---------|-------|----------------------------|
| 分诊步骤 | T1–T3 | 第 3 节中的快速影响评估 |
| 验证步骤 | V1–V6 | 第 7 节中的修复后验证序列 |
