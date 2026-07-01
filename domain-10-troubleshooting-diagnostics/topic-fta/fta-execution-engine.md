---
title: FTA 诊断执行引擎 (domain-10-troubleshooting-diagnostics)
description: 'description: ''**定位**: 将 FTA 理论转化为可执行代码的工程化指南'''
category: fta
tags:
- fta
- troubleshooting
- helm
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 25min
intent_queries:
- FTA 诊断执行引擎 是什么
- 如何 FTA 诊断执行引擎
- Kubernetes 10 troubleshooting diagnostics 最佳实践
- FTA 诊断执行引擎 故障排查
- FTA 诊断执行引擎 排障步骤
- FTA 诊断执行引擎 根因分析
trigger_keywords:
- FTA
- 诊断执行引擎
- troubleshooting
- diagnostics
- fta
prerequisites:
- kubectl-basics
- troubleshooting-methodology
- helm-basics
fta_id: FTA-FTA_EXECUTION_ENGINE-001
component: Fta Execution Engine
severity: critical
created: "2026-05-23"
---

title: FTA 诊断执行引擎
description: '**定位**: 将 FTA 理论转化为可执行代码的工程化指南'
category: fta
tags:
- k8s
- fault-tree
- root-cause
- troubleshooting
- [[Helm|helm]]
- agent
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 5min
intent_queries:
- FTA 诊断执行引擎 是什么
- 如何 FTA 诊断执行引擎
- FTA 诊断执行引擎 根因分析
- FTA 诊断执行引擎 故障树
trigger_keywords:
- FTA
- 诊断执行引擎
- fta
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
---

# FTA 诊断执行引擎

> **版本**: v1.0
> **定位**: 将 FTA 理论转化为可执行代码的工程化指南
> **更新日期**: 2026-05-18

---

<!-- chunk: 一、执行引擎架构 -->## 一、执行引擎架构

## 1.1 核心组件

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                         FTA 诊断执行引擎                                    │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐                  │
│  │  输入处理    │────►│  FTA 遍历   │────►│  证据收集    │                  │
│  │  Layer      │     │  Engine     │     │  Collector  │                  │
│  │             │     │             │     │             │                  │
│  │  症状解析    │     │  路径选择   │     │  多源验证    │                  │
│  │  上下文注入  │     │  剪枝策略   │     │  时序约束    │                  │
│  └─────────────┘     └─────────────┘     └─────────────┘                  │
│        │                   │                   │                           │
│        │                   │                   ▼                           │
│        │                   │         ┌─────────────────┐                  │
│        │                   │         │  置信度评估    │                  │
│        │                   │         │  Bayesian      │                  │
│        │                   │         └─────────────────┘                  │
│        │                   │                   │                           │
│        ▼                   ▼                   ▼                           │
│  ┌─────────────┐     ┌─────────────┐     ┌─────────────┐                  │
│  │  修复执行    │◄────│  决策输出    │◄────│  根因聚合    │                  │
│  │  Controller │     │  Generator  │     │  Engine     │                  │
│  │             │     │             │     │             │                  │
│  │  前置检查   │     │  概率排序   │     │  时序验证   │                  │
│  │  风险评估   │     │  阈值判断   │     │  证据链     │                  │
│  └─────────────┘     └─────────────┘     └─────────────┘                  │
│        │                                                            │
│        ▼                                                            │
│  ┌─────────────┐                                                    │
│  │  学习反馈    │                                                    │
│  │  Loop       │                                                    │
│  └─────────────┘                                                    │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 1.2 输入 Schema

```yaml
diagnosis_request:
  # 必填字段
  primary_symptom: string          # 主要症状
  timestamp: datetime              # 发生时间
  cluster_id: string               # 集群标识
  
  # 可选字段
  secondary_symptoms: [string]     # 伴随症状
  error_logs: [string]             # 错误日志
  exit_code: integer               # 退出码
  events: [object]                 # K8s Events
  metrics: object                  # 实时指标
  
  # 上下文
  context:
    namespace: string
    workload_type: string
    cloud_provider: string          # ACK/AWS/GCP
    environment: string             # prod/staging
```

## 1.3 输出 Schema

```yaml
diagnosis_result:
  # 根因确定
  confirmed_root_cause:
    event_id: string               # BE-2.3
    name: string                   # OOMKilled
    probability: float             # 0.72
    confidence: float              # 0.85
    evidence_chain: [object]       # 证据链
    
  # 候选路径 (未确认时)
  candidate_paths: [object]
    - path_id: string
      fta_path: string              # TE-2 → IE-2.1 → BE-2.3
      probability: float
      required_evidence: [object]   # 还缺什么证据
      
  # 修复方案
  healing_plan:
    - action_id: string
      description: string
      risk_level: enum[low/medium/high/critical]
      preconditions: [object]
      auto_executable: boolean
      estimated_duration: string
      rollback_plan: string
        
  # 学习反馈
  learning_feedback:
    path_confidence_update: float
    new_pattern_flag: boolean
    recommended_actions: [string]
```

---

<!-- chunk: 二、FTA 遍历引擎实现 -->## 二、FTA 遍历引擎实现

## 2.1 路径选择算法

```python
class FTATraversalEngine:
    """FTA 遍历引擎"""
    
    def traverse(self, top_event, evidence_bundle, context):
        """
        遍历 FTA 树，选择最优诊断路径
        """
        
        # 1. 初始化：获取顶事件下的所有子路径
        candidate_paths = self.get_initial_paths(top_event)
        
        # 2. 动态概率调整
        for path in candidate_paths:
            path.dynamic_probability = self.calculate_dynamic_probability(
                path.terminal_events,
                context
            )
        
        # 3. 按概率排序
        candidate_paths.sort(key=lambda p: p.dynamic_probability, reverse=True)
        
        # 4. 智能剪枝
        pruned_paths = self.intelligent_prune(candidate_paths, context)
        
        # 5. 证据匹配验证
        verified_paths = []
        for path in pruned_paths:
            match_result = self.verify_path_with_evidence(path, evidence_bundle)
            if match_result.confidence > 0.5:
                verified_paths.append(match_result)
        
        return verified_paths
    
    def calculate_dynamic_probability(self, terminal_events, context):
        """
        计算动态概率 = 静态概率 × 时间因子 × 负载因子 × 趋势因子 × 季节因子
        """
        
        base_prob = terminal_events[0].static_probability
        
        # 时间因子
        time_factor = self.get_time_factor(context.timestamp)
        
        # 负载因子 (基于资源使用率动态调整)
        load_factor = self.get_load_factor(terminal_events, context)
        
        # 趋势因子 (基于历史问题频率)
        trend_factor = self.get_trend_factor(terminal_events, context)
        
        # 季节因子 (大促/节假日)
        season_factor = self.get_season_factor(context)
        
        return base_prob * time_factor * load_factor * trend_factor * season_factor
    
    def get_load_factor(self, terminal_events, context):
        """负载因子计算"""
        
        # OOM 相关：内存使用率越高，问题概率越高
        if any("oom" in e.id.lower() for e in terminal_events):
            memory_ratio = context.metrics.get("memory_usage_ratio", 0.5)
            # 基准线 70%，每超 10% 概率翻倍
            if memory_ratio > 0.7:
                return 1 + (memory_ratio - 0.7) * 5
            return 0.8  # 低于基准线略微降低
        
        # 网络相关：高并发时问题率上升
        if any("network" in e.id.lower() or "dns" in e.id.lower() for e in terminal_events):
            connection_count = context.metrics.get("active_connections", 0)
            threshold = context.metrics.get("connection_threshold", 10000)
            if connection_count > threshold:
                return 1.5
        
        return 1.0
    
    def intelligent_prune(self, paths, context):
        """
        智能剪枝策略
        """
        
        PRUNE_THRESHOLDS = {
            "min_confidence": 0.3,     # 置信度 < 30% 剪枝
            "max_cost_seconds": 300,   # 预计耗时 > 5 分钟剪枝
            "overlap_threshold": 0.8,  # 与已探索路径重叠 > 80% 剪枝
        }
        
        pruned = []
        
        # 已确认的高置信路径
        confirmed_paths = [p for p in context.explored_paths if p.confidence > 0.8]
        
        for path in paths:
            # 规则 1: 置信度过低
            if path.dynamic_probability < PRUNE_THRESHOLDS["min_confidence"]:
                path.prune_reason = "置信度过低"
                continue
                
            # 规则 2: 代价过高
            estimated_cost = self.estimate_path_cost(path)
            if estimated_cost > PRUNE_THRESHOLDS["max_cost_seconds"]:
                path.prune_reason = "预计耗时过长"
                continue
                
            # 规则 3: 与已确认路径重叠
            for confirmed in confirmed_paths:
                if self.calculate_overlap(path, confirmed) > PRUNE_THRESHOLDS["overlap_threshold"]:
                    path.prune_reason = f"与已确认路径重叠 {confirmed.path_id}"
                    continue
                    
            # 规则 4: 已有高置信路径时，低置信路径降级
            if confirmed_paths and path.dynamic_probability < 0.6:
                path.prune_reason = "已有高置信路径"
                continue
                
            pruned.append(path)
            
        return pruned
```

## 2.2 证据收集器

```python
class EvidenceCollector:
    """证据收集器"""
    
    def collect(self, bottom_event, context):
        """
        收集指定底事件的证据
        """
        
        evidence_bundle = {
            "direct_evidence": [],    # 直接证据 (高确定性)
            "indirect_evidence": [],  # 间接证据 (中确定性)
            "circumstantial": [],      # 旁证 (低确定性)
            "contradicting": []        # 矛盾证据
        }
        
        # 1. 从 K8s Events 获取
        k8s_events = self.fetch_k8s_events(bottom_event, context)
        evidence_bundle["direct_evidence"].extend(k8s_events)
        
        # 2. 从 Metrics 获取
        metrics = self.fetch_metrics(bottom_event, context)
        evidence_bundle["direct_evidence"].extend(metrics)
        
        # 3. 从 Logs 获取
        logs = self.fetch_relevant_logs(bottom_event, context)
        evidence_bundle["indirect_evidence"].extend(logs)
        
        # 4. 时序验证
        temporal_valid = self.validate_temporal_constraints(bottom_event, evidence_bundle)
        if not temporal_valid:
            return EvidenceResult(bundle=evidence_bundle, confidence=0)
        
        # 5. 多源交叉验证
        cross_valid = self.cross_validate(evidence_bundle)
        
        return EvidenceResult(
            bundle=evidence_bundle,
            confidence=self.calculate_combined_confidence(evidence_bundle, cross_valid)
        )
    
    def validate_temporal_constraints(self, bottom_event, evidence_bundle):
        """验证时序约束"""
        
        if not hasattr(bottom_event, 'temporal_constraints'):
            return True
            
        for constraint in bottom_event.temporal_constraints:
            evidence_in_window = self.get_evidence_in_time_window(
                evidence_bundle,
                constraint.duration,
                constraint.condition
            )
            
            if not evidence_in_window:
                return False
                
        return True
```

---

<!-- chunk: 三、置信度评估引擎 -->## 三、置信度评估引擎

## 3.1 多维度置信度计算

```python
class ConfidenceEvaluator:
    """置信度评估引擎"""
    
    EVIDENCE_WEIGHTS = {
        # 直接证据 (高确定性)
        "k8s_event_oomkilled": 0.95,
        "k8s_event_evicted": 0.90,
        "metric_threshold_exceeded": 0.85,
        "exit_code_137": 0.90,  # OOM
        "exit_code_1": 0.60,   # 通用错误
        
        # 间接证据 (中确定性)
        "error_log_present": 0.70,
        "warning_log_pattern": 0.50,
        "timeout_detected": 0.65,
        
        # 旁证 (低确定性)
        "symptom_inference": 0.40,
        "historical_pattern": 0.30,
        "user_reported": 0.25
    }
    
    def evaluate(self, bottom_event, evidence_bundle):
        """
        综合评估证据Bundle的置信度
        """
        
        scores_by_category = {
            "direct": [],
            "indirect": [],
            "circumstantial": []
        }
        
        for evidence in evidence_bundle:
            weight = self.EVIDENCE_WEIGHTS.get(evidence.type, 0.5)
            
            # 多源验证加成
            if self.has_multiple_sources(evidence, evidence_bundle):
                weight *= 1.15
                
            # 时间一致性加成
            if self.is_temporally_consistent(evidence, evidence_bundle):
                weight *= 1.10
                
            # 分类
            category = self.classify_evidence(evidence)
            scores_by_category[category].append(weight)
        
        # 综合计算
        # 原则：取最高确定性证据为主，加权平均为辅
        direct_max = max(scores_by_category["direct"]) if scores_by_category["direct"] else 0
        indirect_avg = avg(scores_by_category["indirect"]) if scores_by_category["indirect"] else 0
        circumstantial_avg = avg(scores_by_category["circumstantial"]) if scores_by_category["circumstantial"] else 0
        
        # 综合 = 最大确定性 × 0.6 + 间接平均 × 0.3 + 旁证平均 × 0.1
        combined = direct_max * 0.6 + indirect_avg * 0.3 + circumstantial_avg * 0.1
        
        return combined
```

## 3.2 贝叶斯后验概率

```python
class BayesianReasoningEngine:
    """贝叶斯推理引擎"""
    
    def calculate_posterior(
        self,
        hypothesis_id,           # BE-2.3 OOMKilled
        prior_probability,       # FTA 中的先验概率
        evidence_bundle,          # 收集到的证据
        likelihoods               # P(证据|假设) 条件概率表
    ):
        """
        贝叶斯定理计算后验概率
        P(假设|证据) = P(证据|假设) × P(假设) / P(证据)
        """
        
        # 1. 计算似然度 (likelihood)
        likelihood = 1.0
        for evidence in evidence_bundle:
            p_evidence_given_hypothesis = likelihoods.get(
                (evidence.type, hypothesis_id),
                0.5  # 默认 50%
            )
            likelihood *= p_evidence_given_hypothesis
        
        # 2. 计算边际概率 P(证据)
        marginal = self.calculate_marginal_probability(
            evidence_bundle,
            hypothesis_id,
            prior_probability
        )
        
        # 3. 计算后验概率
        posterior = (likelihood * prior_probability) / marginal
        
        # 4. 计算置信区间
        confidence_interval = self.compute_confidence_interval(
            posterior,
            len(evidence_bundle)  # 证据越多，区间越窄
        )
        
        return PosteriorResult(
            probability=posterior,
            confidence_interval=confidence_interval,
            entropy=self.calculate_entropy(posterior)
        )
    
    def calculate_marginal_probability(self, evidence_bundle, hypothesis_id, prior):
        """
        计算边际概率 P(证据) = P(证据|假设)P(假设) + P(证据|非假设)P(非假设)
        """
        
        p_evidence_given_hypothesis = likelihoods.get((evidence.type, hypothesis_id), 0.5)
        p_evidence_given_not_hypothesis = 0.3  # 证据在非假设情况下出现的概率
        
        marginal = (
            p_evidence_given_hypothesis * prior +
            p_evidence_given_not_hypothesis * (1 - prior)
        )
        
        return marginal
```

---

<!-- chunk: 四、修复执行控制器 -->## 四、修复执行控制器

## 4.1 前置条件检查

```python
class HealingPreconditionChecker:
    """修复前置条件检查器"""
    
    def can_execute(self, healing_action, current_state):
        """
        检查修复动作是否可以执行
        """
        
        # 1. 风险等级检查
        if healing_action.risk_level == "critical":
            if not current_state.has_human_approval:
                return PreconditionResult(
                    can_execute=False,
                    reason="高风险操作需要人工审批",
                    blocking=True,
                    escalation_required=True
                )
        
        # 2. 前置条件检查
        for precondition in healing_action.preconditions:
            result = self.evaluate_precondition(precondition, current_state)
            if not result.met:
                return PreconditionResult(
                    can_execute=False,
                    reason=f"前置条件未满足: {precondition.description}",
                    blocking=precondition.blocking,
                    missing=result.detail
                )
        
        # 3. 依赖资源检查
        for resource in healing_action.required_resources:
            if not self.is_resource_available(resource, current_state):
                return PreconditionResult(
                    can_execute=False,
                    reason=f"依赖资源不可用: {resource}",
                    blocking=True
                )
        
        # 4. 冲突检测
        conflicting = self.check_conflicts(healing_action, current_state)
        if conflicting:
            return PreconditionResult(
                can_execute=False,
                reason=f"与正在执行的修复冲突: {conflicting}",
                blocking=True
            )
        
        return PreconditionResult(can_execute=True)
    
    def evaluate_precondition(self, precondition, current_state):
        """评估单个前置条件"""
        
        if precondition.type == "metric_threshold":
            current_value = current_state.get_metric(precondition.metric)
            return CheckResult(
                met=current_value >= precondition.threshold,
                detail=f"{precondition.metric}={current_value}, 需要>={precondition.threshold}"
            )
            
        elif precondition.type == "feature_flag":
            enabled = current_state.get_flag(precondition.flag_name)
            return CheckResult(met=enabled, detail=f"flag={enabled}")
            
        elif precondition.type == "maintenance_window":
            in_window = current_state.is_in_maintenance_window(
                precondition.allowed_windows
            )
            return CheckResult(met=in_window, detail=f"in_window={in_window}")
            
        elif precondition.type == "quota_available":
            remaining = current_state.get_quota_remaining(precondition.resource)
            return CheckResult(
                met=remaining >= precondition.required,
                detail=f"remaining={remaining}, required={precondition.required}"
            )
            
        return CheckResult(met=True)
```

## 4.2 修复执行流程

```python
class HealingExecutor:
    """修复执行器"""
    
    def execute(self, healing_plan, context):
        """
        执行修复计划
        """
        
        results = []
        
        for action in healing_plan:
            # 前置检查
            check_result = self.precondition_checker.can_execute(action, context.state)
            
            if not check_result.can_execute:
                if check_result.blocking:
                    # 阻塞性前置条件不满足，跳过此动作
                    results.append(HealingResult(
                        action_id=action.id,
                        status="skipped",
                        reason=check_result.reason
                    ))
                    continue
                else:
                    # 非阻塞性，可以继续但记录警告
                    results.append(HealingResult(
                        action_id=action.id,
                        status="warning",
                        reason=check_result.reason
                    ))
            
            # 执行修复
            if action.auto_executable or context.user_approved:
                exec_result = self.do_execute(action, context)
                results.append(exec_result)
                
                # 执行后验证
                if exec_result.success:
                    verify_result = self.verify_after_execution(action, context)
                    if not verify_result.success:
                        # 验证失败，触发回退
                        self.rollback(action, context)
            else:
                results.append(HealingResult(
                    action_id=action.id,
                    status="pending_approval",
                    reason="需要人工审批"
                ))
        
        return HealingPlanResult(actions=results)
    
    def do_execute(self, action, context):
        """执行修复动作"""
        
        # 1. 记录执行前状态
        snapshot = context.state.capture()
        
        try:
            if action.type == "kubectl_patch":
                return self.execute_kubectl_patch(action, context)
            elif action.type == "kubectl_scale":
                return self.execute_kubectl_scale(action, context)
            elif action.type == "kubectl_delete":
                return self.execute_kubectl_delete(action, context)
            elif action.type == "helm_upgrade":
                return self.execute_helm_upgrade(action, context)
            else:
                return HealingResult(
                    action_id=action.id,
                    status="error",
                    reason=f"未知修复类型: {action.type}"
                )
                
        except Exception as e:
            return HealingResult(
                action_id=action.id,
                status="error",
                reason=str(e),
                rollback_snapshot=snapshot
            )
```

---

<!-- chunk: 五、学习反馈闭环 -->## 五、学习反馈闭环

## 5.1 FTA 学习引擎

```python
class FTALearningEngine:
    """FTA 学习引擎"""
    
    def learn_from_incident(self, incident_record):
        """
        从实际问题中学习，更新 FTA
        """
        
        # 1. 验证 FTA 路径准确性
        predicted = incident_record.fta_predicted_path
        actual = incident_record.actual_root_cause
        
        if self.path_matches(predicted, actual):
            # 预测正确：增强置信度
            self.increment_confidence(predicted)
            feedback = "预测正确"
        else:
            # 预测错误：减少置信度，补充新路径
            self.decrement_confidence(predicted)
            self.propose_new_path(actual, incident_record)
            feedback = "预测错误，已记录新路径"
        
        # 2. 更新概率数据
        self.update_probability_stats(
            incident_record.root_cause_id,
            incident_record.mttr_minutes,
            incident_record.auto_heal_success
        )
        
        # 3. 检测新故障模式
        if not incident_record.path_existed_in_fta:
            self.flag_new_fault_pattern(incident_record)
        
        # 4. 记录学习结果
        self.record_learning(
            incident_id=incident_record.id,
            feedback=feedback,
            path_confidence_change=self.calculate_confidence_delta(predicted),
            new_pattern=incident_record.path_existed_in_fta == False
        )
    
    def increment_confidence(self, path):
        """增加路径置信度 (上限 1.0)"""
        for node_id in path.node_ids:
            node = self.fta.get_node(node_id)
            node.confidence_score = min(1.0, node.confidence_score * 1.05)
    
    def decrement_confidence(self, path):
        """减少路径置信度 (下限 0.1)"""
        for node_id in path.node_ids:
            node = self.fta.get_node(node_id)
            node.confidence_score = max(0.1, node.confidence_score * 0.9)
    
    def propose_new_path(self, actual_root_cause, incident_record):
        """提议新路径 (PROPOSED 状态，待评审)"""
        
        new_node = {
            "id": f"PROPOSED-{incident_record.id}",
            "name": actual_root_cause.name,
            "type": "bottom_event",
            "status": "proposed",  # 待评审状态
            "evidence": incident_record.evidence_chain,
            "proposed_by": "learning_engine",
            "proposed_at": incident_record.resolved_at
        }
        
        self.fta.add_node(new_node)
        
        # 生成评审任务
        self.create_review_task(new_node)
```

## 5.2 学习触发机制

```yaml
learning_triggers:
  # 触发条件 → 执行动作
  
  - trigger: "诊断准确完成"
    condition: "confirmed_root_cause.confidence >= 0.85"
    action: 
      - name: "更新路径置信度"
        function: "increment_confidence(path)"
      - name: "记录成功案例"
        function: "record_success_case(incident)"
        
  - trigger: "诊断失败"
    condition: "diagnostic_time > threshold AND not confirmed"
    action:
      - name: "降低路径置信度"
        function: "decrement_confidence(path)"
      - name: "触发 FEBM 复盘"
        function: "initiate_febm_review(incident)"
        
  - trigger: "自动修复失败"
    condition: "healing_result.status == 'failed'"
    action:
      - name: "记录修复失败"
        function: "record_healing_failure(action, error)"
      - name: "更新修复成功率"
        function: "update_heal_rate(path, success=false)"
        
  - trigger: "新问题路径发现"
    condition: "actual_root_cause not in FTA"
    action:
      - name: "创建提案节点"
        function: "propose_new_path(root_cause, incident)"
      - name: "通知评审委员会"
        function: "notify_review_board(proposal)"
        
  - trigger: "相同根因重复发生"
    condition: "count(root_cause) >= 3 in last 30d"
    action:
      - name: "提升路径优先级"
        function: "increase_path_priority(root_cause)"
      - name: "调整静态概率"
        function: "adjust_base_probability(root_cause, factor=1.5)"
```

---

<!-- chunk: 六、执行引擎配置 -->## 六、执行引擎配置

## 6.1 全局配置

```yaml
fta_execution_engine:
  # 遍历配置
  traversal:
    max_parallel_paths: 5           # 最多并行 5 条路径
    min_confidence_threshold: 0.3   # 置信度 < 30% 剪枝
    max_diagnosis_time_seconds: 600 # 诊断超时 10 分钟
    prune_early_on_confirmed: true # 确认后立即剪枝其他路径
    
  # 证据收集配置
  evidence:
    sources:
      - k8s_events
      - metrics
      - logs
      - node_exporter
      - custom_exporter
    time_window_seconds: 300       # 收集最近 5 分钟证据
    cross_validation_required: true # 需要多源验证
    
  # 置信度配置
  confidence:
    threshold_confirmed: 0.85       # >= 85% 确认根因
    threshold_candidate: 0.5        # >= 50% 作为候选
    bayesian_enabled: true          # 启用贝叶斯推理
    
  # 修复执行配置
  healing:
    auto_execute_low_risk: true    # 低风险自动执行
    auto_execute_medium_risk: false # 中风险需审批
    require_approval_high_risk: true # 高风险必须审批
    rollback_on_failure: true       # 失败自动回退
    max_retry_attempts: 3           # 最多重试 3 次
    
  # 学习配置
  learning:
    enabled: true
    trigger_on_success: true
    trigger_on_failure: true
    trigger_on_new_pattern: true
    auto_update_probability: true   # 自动更新概率
    auto_update_confidence: true    # 自动更新置信度
    proposal_review_required: true  # 新路径需评审
```

---

<!-- chunk: 七、完整诊断流程示例 -->## 七、完整诊断流程示例

```
场景: Pod CrashLoopBackOff + OOMKilled + Exit Code 137

Step 1: 输入处理
  输入: {
    primary_symptom: "Pod CrashLoopBackOff",
    secondary_symptoms: ["OOMKilled", "Exit Code 137"],
    namespace: "default",
    workload_type: "Deployment"
  }
  
Step 2: FTA 遍历
  候选路径:
    - TE-2 → IE-2.1 → BE-2.3 (OOMKilled): 动态概率 0.72
    - TE-2 → IE-2.1 → BE-2.1 (CrashLoop): 动态概率 0.45
    - TE-3 → IE-3.1 → BE-3.1 (调度失败): 动态概率 0.12
  
  剪枝结果:
    - BE-2.3 (0.72): 保留 ✅
    - BE-2.1 (0.45): 保留 (接近阈值)
    - BE-3.1 (0.12): 剪枝 ❌
  
Step 3: 证据收集
  BE-2.3 证据Bundle:
    - K8s Event "OOMKilled": 置信度 0.95 ✅
    - Exit Code 137: 置信度 0.90 ✅
    - Memory usage 92%: 置信度 0.70 ✅
    
  综合置信度: 0.95 * 0.6 + 0.80 * 0.3 + 0.40 * 0.1 = 0.83
  
Step 4: 决策输出
  confirmed_root_cause: BE-2.3 (OOMKilled)
  confidence: 0.83 (> 0.85 阈值，但接近)
  
  (如果 < 0.85，需要进一步验证)
  
Step 5: 修复执行
  方案: HA-2.3.1 (增加内存 limit)
  前置检查: ✅ 通过
  自动执行: ✅ 成功
  
Step 6: 学习反馈
  结果: 诊断成功 + 修复成功
  动作: 置信度 +5%
```

---

> **版本**: v1.0
> **维护团队**: Platform Team / SRE
> **下一步**: 集成到 K8sOpsAgent 实现

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- [[domain-10-troubleshooting-diagnostics/topic-fta/MOC.md|topic-fta [[KUDIG Database — Global MOC|MOC]]]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/README.md|topic-fta: 故障树分析（FTA）方法论与 AI Agent 智能运维实践]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/01-fta-origin-and-evolution.md|第一章：FTA 起源与发展史]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/02-fta-mathematical-foundations.md|第二章：FTA 数学基础与理论模型]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/03-fta-symbol-system-and-standards.md|第三章：FTA 符号体系与标准规范]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/04-fta-core-principles.md|第四章：FTA 方法论核心原则]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/05-fta-construction-process.md|第五章：FTA 构建完整流程]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/06-fta-verification-and-quality.md|第六章：FTA 验证与质量保证]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/07-fta-maintenance-and-evolution.md|第七章：FTA 维护与演进策略]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/08-ai-agent-ops-revolution.md|第八章：AI Agent 时代的运维范式革命]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/09-fta-as-agent-knowledge-skeleton.md|第九章：FTA 作为 AI Agent 的知识骨架]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/10-agent-orchestration-patterns.md|第十章：Agent 编排模式与 FTA 逻辑门映射]]

## See Also

- [[domain-10-troubleshooting-diagnostics/topic-fta/appendix-d-templates.md|appendix-d-templates]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-diagnosis-improvement.md|fta-diagnosis-improvement]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-index.md|fta-index]]
- [[domain-10-troubleshooting-diagnostics/topic-fta/fta-methodology-and-agentic-practices.md|fta-methodology-and-agentic-practices]]
