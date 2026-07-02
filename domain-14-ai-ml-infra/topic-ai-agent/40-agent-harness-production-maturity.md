---
title: Agent Harness 生产运维与成熟度模型 (domain-14-ai-ml-infra)
description: 'title: Agent Harness 生产运维与成熟度模型'
summary: 'title: Agent Harness 生产运维与成熟度模型'
category: general
tags:
- ai
- ai-agent
- production
- prometheus
- grafana
- helm
- redis
- postgresql
- gateway
- llm
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- Agent Harness 生产运维与成熟度模型 是什么
- 如何 Agent Harness 生产运维与成熟度模型
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- Harness
- 生产运维与成熟度模型
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- redis-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




title: Agent Harness 生产运维与成熟度模型
description: '# Agent Harness 生产运维与成熟度模型'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[Prometheus|prometheus]]
- grafana
- [[Helm|helm]]
- redis
- postgresql
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Agent Harness 生产运维与成熟度模型 是什么
- 如何 Agent Harness 生产运维与成熟度模型
trigger_keywords:
- Agent
- Harness
- 生产运维与成熟度模型
- ai
- agent
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

# Agent Harness 生产运维与成熟度模型

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: [[entities/k8s-production-operations.md|Production Operations]], 成熟度模型, 灰度发布, 容量规划, SLA, 故障恢复, 版本管理, 配置管理, 自进化, 运维自动化

---

<!-- chunk: 概述 -->## 概述

将 Agent Harness 从开发环境部署到生产环境，是一个跨越"能用"到"可靠可控"的质变过程。生产级 Harness 需要应对高可用、灰度发布、版本管理、故障恢复、容量规划等传统运维挑战，同时还需要处理 Agent 特有的非确定性行为管控。

本文系统阐述 Harness 的生产化路径、灰度发布策略、版本管理、配置热更新、故障恢复、SLA 设计，以及 Harness 成熟度五级模型的实施指南。

---

<!-- chunk: 1. 生产部署架构 -->## 1. 生产部署架构

## 1.1 部署拓扑

```
# 🟢 低风险：只读/信息收集，通常无副作用
Agent Harness 生产部署拓扑:

┌─────────────────────────────────────────────────────────┐
│                     入口层（Gateway）                     │
│  API Gateway │ 认证鉴权 │ 限流 │ 路由                    │
├─────────────────────────────────────────────────────────┤
│                   Harness 服务层                          │
│  ┌─────────────┐  ┌─────────────┐  ┌─────────────┐      │
│  │ Harness v2.1│  │ Harness v2.0│  │ Harness v1.9│      │
│  │ (Canary 5%) │  │ (Stable 95%)│  │ (Rollback)  │      │
│  └─────────────┘  └─────────────┘  └─────────────┘      │
├─────────────────────────────────────────────────────────┤
│                   LLM 提供商层                            │
│  OpenAI API │ Anthropic API │ Azure OpenAI │ 本地模型    │
├─────────────────────────────────────────────────────────┤
│                   工具执行层                              │
│  kubectl │ Prometheus │ Loki │ Helm │ 自定义工具          │
├─────────────────────────────────────────────────────────┤
│                   数据层                                  │
│  Redis（缓存）│ Milvus（向量）│ PostgreSQL（审计）        │
├─────────────────────────────────────────────────────────┤
│                   可观测性层                              │
│  OTel Collector │ Prometheus │ Grafana │ Langfuse        │
└─────────────────────────────────────────────────────────┘
```
## 1.2 K8S 部署清单

```yaml
# harness-deployment.yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: agent-harness
  namespace: agent-system
  labels:
    app: agent-harness
    version: v2.1
spec:
  replicas: 3
  strategy:
    type: RollingUpdate
    rollingUpdate:
      maxSurge: 1
      maxUnavailable: 0
  selector:
    matchLabels:
      app: agent-harness
  template:
    metadata:
      labels:
        app: agent-harness
        version: v2.1
      annotations:
        prometheus.io/scrape: "true"
        prometheus.io/port: "8080"
        prometheus.io/path: "/metrics"
    spec:
      containers:
      - name: harness
        image: agent-harness:v2.1
        ports:
        - containerPort: 8080
          name: http
        - containerPort: 8081
          name: metrics
        env:
        - name: HARNESS_VERSION
          value: "v2.1"
        - name: HARNESS_CONFIG_PATH
          value: "/config/harness-config.yaml"
        - name: OTEL_EXPORTER_OTLP_ENDPOINT
          value: "http://otel-collector:4317"
        - name: LANGFUSE_HOST
          value: "http://langfuse:3000"
        envFrom:
        - secretRef:
            name: llm-api-keys
        resources:
          requests:
            cpu: 500m
            memory: 1Gi
          limits:
            cpu: 2000m
            memory: 4Gi
        livenessProbe:
          httpGet:
            path: /healthz
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 30
        readinessProbe:
          httpGet:
            path: /readyz
            port: 8080
          initialDelaySeconds: 5
          periodSeconds: 10
        volumeMounts:
        - name: config
          mountPath: /config
        - name: soul-skill
          mountPath: /harness/prompts
      volumes:
      - name: config
        configMap:
          name: harness-config
      - name: soul-skill
        configMap:
          name: harness-prompts
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: harness-config
  namespace: agent-system
data:
  harness-config.yaml: |
    harness:
      version: v2.1
      loop:
        max_iterations: 15
        timeout_seconds: 300
      constraints:
        read_only: true
        max_tokens_per_task: 50000
        max_cost_per_task_usd: 2.0
        blocked_namespaces:
          - kube-system
          - kube-public
      verification:
        min_faithfulness: 0.85
        require_evidence: true
      model:
        default: gpt-4o
        fallback: gpt-4o-mini
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: harness-prompts
  namespace: agent-system
data:
  SOUL.md: |
    你是 K8S 运维诊断专家。
    你只能使用授权的工具进行信息收集。
    生产环境中禁止执行任何写操作。
    每个诊断结论必须引用具体的 Event 或日志证据。
  SKILL.md: |
    Pod Pending 诊断流程:
    1. kubectl describe pod → 检查 Events
    2. kubectl get events → 检查调度事件
    3. kubectl get nodes → 检查节点资源
    4. kubectl top nodes → 确认资源使用率
```

---

<!-- chunk: 2. 灰度发布策略 -->## 2. 灰度发布策略

## 2.1 四阶段灰度发布

```
Harness 灰度发布四阶段:

Stage 1: Shadow Mode（影子模式）
  时间: 24-48h
  策略: 新旧 Harness 并行运行，新版不生效
  监控: 对比两个版本的输出质量
  退出条件:
    - 新版质量 >= 旧版质量 - 2%
    - 无安全问题

Stage 2: Canary（金丝雀）
  时间: 48h
  策略: 5% 流量切到新 Harness
  监控:
    - 任务成功率
    - 验证通过率
    - 延迟 P95
    - Token 消耗
    - 成本
  退出条件:
    - 所有指标不低于旧版 5%
    - 无约束违反
    - 无安全事件

Stage 3: Progressive Rollout（渐进发布）
  时间: 每阶段 24h
  策略: 5% → 25% → 50% → 75%
  监控: 同 Canary，每阶段至少 24h
  回滚条件:
    - 任何阶段任一指标退化 > 5%
    - 检测到安全事件
    - 约束违反率 > 0

Stage 4: Full Rollout（全量发布）
  时间: 24h 观察期
  策略: 100% 流量切到新版
  监控: 持续 24h 全量监控
  确认: 保留旧版 72h 用于回滚
```

## 2.2 灰度控制器

```python
class GrayReleaseController:
    """灰度发布控制器"""

    def __init__(self, metrics_collector, alert_manager):
        self.metrics = metrics_collector
        self.alerts = alert_manager
        self.current_stage = "shadow"
        self.traffic_ratio = 0.0
        self.stage_start_time = None

    STAGES = {
        "shadow": {"traffic": 0.0, "duration_hours": 24, "next": "canary"},
        "canary": {"traffic": 0.05, "duration_hours": 48, "next": "progressive_25"},
        "progressive_25": {"traffic": 0.25, "duration_hours": 24, "next": "progressive_50"},
        "progressive_50": {"traffic": 0.50, "duration_hours": 24, "next": "progressive_75"},
        "progressive_75": {"traffic": 0.75, "duration_hours": 24, "next": "full"},
        "full": {"traffic": 1.0, "duration_hours": 24, "next": None},
    }

    def advance_stage(self) -> dict:
        """推进到下一阶段"""
        stage_config = self.STAGES[self.current_stage]

        # 检查安全条件
        safety_check = self._check_safety_conditions()
        if not safety_check["safe"]:
            return {
                "action": "hold",
                "reason": safety_check["reason"],
                "current_stage": self.current_stage,
            }

        # 检查质量条件
        quality_check = self._check_quality_conditions()
        if not quality_check["passed"]:
            return {
                "action": "rollback",
                "reason": quality_check["reason"],
                "current_stage": self.current_stage,
            }

        # 推进
        next_stage = stage_config["next"]
        if next_stage:
            self.current_stage = next_stage
            self.traffic_ratio = self.STAGES[next_stage]["traffic"]
            self.stage_start_time = time.time()
            return {
                "action": "advanced",
                "new_stage": next_stage,
                "traffic_ratio": self.traffic_ratio,
            }
        else:
            return {"action": "complete", "stage": "full"}

    def rollback(self, reason: str) -> dict:
        """回滚到稳定版"""
        self.current_stage = "shadow"
        self.traffic_ratio = 0.0
        self.alerts.send_alert(
            severity="critical",
            message=f"Harness 灰度回滚: {reason}",
        )
        return {"action": "rollback", "reason": reason}

    def _check_safety_conditions(self) -> dict:
        """安全条件检查"""
        violations = self.metrics.get_constraint_violations(
            window="1h"
        )
        if violations > 0:
            return {"safe": False, "reason": f"检测到 {violations} 次约束违反"}

        injection_attempts = self.metrics.get_injection_attempts(
            window="1h"
        )
        if injection_attempts > 5:
            return {"safe": False, "reason": "注入攻击频繁"}

        return {"safe": True}

    def _check_quality_conditions(self) -> dict:
        """质量条件检查"""
        new_metrics = self.metrics.get_version_metrics("new")
        old_metrics = self.metrics.get_version_metrics("old")

        if not new_metrics or not old_metrics:
            return {"passed": True}

        # 成功率不低于旧版 5%
        if new_metrics.get("success_rate", 0) < old_metrics.get("success_rate", 0) - 0.05:
            return {
                "passed": False,
                "reason": f"成功率退化: {new_metrics['success_rate']:.2%} < "
                         f"{old_metrics['success_rate']:.2%} - 5%",
            }

        # 验证通过率不低于旧版
        if new_metrics.get("verification_rate", 0) < old_metrics.get("verification_rate", 0) - 0.05:
            return {
                "passed": False,
                "reason": "验证通过率退化",
            }

        return {"passed": True}
```

---

<!-- chunk: 3. 配置管理与热更新 -->## 3. 配置管理与热更新

## 3.1 配置热更新机制

```python
import yaml
import hashlib
from watchdog.observers import Observer
from watchdog.events import FileSystemEventHandler

class HarnessConfigManager:
    """Harness 配置管理器：支持热更新"""

    def __init__(self, config_path: str):
        self.config_path = config_path
        self._config: dict = {}
        self._config_hash: str = ""
        self._callbacks: list = []
        self._load_config()

    def _load_config(self):
        """加载配置"""
        with open(self.config_path) as f:
            new_config = yaml.safe_load(f)
        new_hash = hashlib.md5(
            yaml.dump(new_config).encode()
        ).hexdigest()

        if new_hash != self._config_hash:
            old_config = self._config
            self._config = new_config
            self._config_hash = new_hash

            # 触发回调
            for callback in self._callbacks:
                callback(old_config, new_config)

    def get(self, key: str, default=None):
        """获取配置值（支持点号分隔的路径）"""
        keys = key.split(".")
        value = self._config
        for k in keys:
            if isinstance(value, dict) and k in value:
                value = value[k]
            else:
                return default
        return value

    def on_change(self, callback):
        """注册配置变更回调"""
        self._callbacks.append(callback)

    def start_watching(self):
        """启动文件监控（ConfigMap 挂载更新时触发）"""
        handler = ConfigFileHandler(self._load_config)
        observer = Observer()
        observer.schedule(handler, path=self.config_path, recursive=False)
        observer.start()


class ConfigFileHandler(FileSystemEventHandler):
    def __init__(self, reload_fn):
        self.reload_fn = reload_fn

    def on_modified(self, event):
        self.reload_fn()
```

## 3.2 Prompt 版本管理

```python
class PromptVersionManager:
    """Prompt 版本管理器"""

    def __init__(self, storage_backend):
        self.storage = storage_backend

    def save_version(self, name: str, content: str,
                     metadata: dict = None) -> str:
        """保存 Prompt 新版本"""
        version_id = f"{name}_v{int(time.time())}"
        self.storage.save(version_id, {
            "name": name,
            "content": content,
            "metadata": metadata or {},
            "created_at": datetime.utcnow().isoformat(),
            "hash": hashlib.md5(content.encode()).hexdigest(),
        })
        return version_id

    def get_version(self, name: str, version: str = "latest") -> dict:
        """获取 Prompt 版本"""
        if version == "latest":
            return self.storage.get_latest(name)
        return self.storage.get(f"{name}_{version}")

    def rollback(self, name: str, target_version: str) -> dict:
        """回滚到指定版本"""
        target = self.get_version(name, target_version)
        if not target:
            return {"success": False, "error": "版本不存在"}
        self.save_version(
            name, target["content"],
            metadata={"rollback_from": "latest",
                       "rollback_to": target_version},
        )
        return {"success": True, "restored_version": target_version}

    def diff(self, name: str, v1: str, v2: str) -> dict:
        """对比两个版本"""
        version1 = self.get_version(name, v1)
        version2 = self.get_version(name, v2)
        import difflib
        diff = list(difflib.unified_diff(
            version1["content"].splitlines(),
            version2["content"].splitlines(),
            fromfile=f"{name}@{v1}",
            tofile=f"{name}@{v2}",
        ))
        return {"diff": "\n".join(diff), "v1": v1, "v2": v2}
```

---

<!-- chunk: 4. SLA 设计 -->## 4. SLA 设计

## 4.1 Agent Harness SLA 体系

```
Agent Harness SLA 指标:

可用性 SLA:
  目标: 99.9%（每月最多 43 分钟不可用）
  计算: 成功响应的请求 / 总请求
  排除: 计划内维护、LLM 提供商问题

质量 SLA:
  任务完成率: > 90%
  验证通过率: > 85%
  幻觉率: < 5%
  安全事件: 0 次 / 月

性能 SLA:
  P50 延迟: < 10 秒
  P95 延迟: < 30 秒
  P99 延迟: < 60 秒

成本 SLA:
  单任务平均成本: < $1.00
  日成本上限: 可配置
  Token 预算执行: 100% 生效
```

## 4.2 SLA 监控

```python
class SLAMonitor:
    """SLA 监控器"""

    def __init__(self, metrics_collector):
        self.metrics = metrics_collector
        self.sla_targets = {
            "availability": 0.999,
            "task_completion_rate": 0.90,
            "verification_pass_rate": 0.85,
            "hallucination_rate": 0.05,  # 上界
            "p50_latency_seconds": 10,
            "p95_latency_seconds": 30,
        }

    def check_sla(self, period: str = "24h") -> dict:
        """检查 SLA 合规性"""
        actuals = self.metrics.get_sla_metrics(period)
        results = []

        for metric, target in self.sla_targets.items():
            actual = actuals.get(metric, 0)
            is_upper_bound = metric in ("hallucination_rate",
                                         "p50_latency_seconds",
                                         "p95_latency_seconds")

            if is_upper_bound:
                met = actual <= target
            else:
                met = actual >= target

            results.append({
                "metric": metric,
                "target": target,
                "actual": actual,
                "met": met,
                "margin": abs(actual - target),
            })

        all_met = all(r["met"] for r in results)
        return {
            "period": period,
            "sla_met": all_met,
            "results": results,
            "violations": [r for r in results if not r["met"]],
        }
```

---

<!-- chunk: 5. 故障恢复 -->## 5. 故障恢复

## 5.1 故障恢复策略

```python
class HarnessFailoverManager:
    """Harness 故障恢复管理器"""

    def __init__(self, primary_harness, fallback_harness,
                 health_checker):
        self.primary = primary_harness
        self.fallback = fallback_harness
        self.health = health_checker
        self.active = "primary"

    async def execute_with_failover(self, task: str, context: dict) -> dict:
        """带故障转移的执行"""
        if self.active == "primary":
            try:
                # 健康检查
                if not await self.health.check(self.primary):
                    self._switch_to_fallback("健康检查失败")
                    return await self._execute_fallback(task, context)

                result = await self.primary.async_run(task, context)

                # 结果质量检查
                if result.get("status") == "error":
                    self._switch_to_fallback("执行错误")
                    return await self._execute_fallback(task, context)

                return result

            except Exception as e:
                self._switch_to_fallback(str(e))
                return await self._execute_fallback(task, context)
        else:
            return await self._execute_fallback(task, context)

    async def _execute_fallback(self, task: str, context: dict) -> dict:
        """使用降级 Harness 执行"""
        result = await self.fallback.async_run(task, context)
        result["_failover"] = True
        result["_failover_reason"] = "primary harness unavailable"
        return result

    def _switch_to_fallback(self, reason: str):
        """切换到备用 Harness"""
        self.active = "fallback"
        logger.warning(f"切换到备用 Harness: {reason}")

    def recover_primary(self):
        """恢复主 Harness"""
        if self.health.check_sync(self.primary):
            self.active = "primary"
            logger.info("主 Harness 恢复")
```

## 5.2 LLM 提供商容灾

```python
class LLMProviderFailover:
    """LLM 提供商容灾"""

    def __init__(self, providers: list[dict]):
        self.providers = providers  # 按优先级排序
        self.current_index = 0
        self.failure_counts: dict[str, int] = {}

    async def invoke(self, prompt: str, **kwargs) -> dict:
        """带容灾的 LLM 调用"""
        for i, provider in enumerate(self.providers):
            name = provider["name"]
            try:
                result = await provider["client"].ainvoke(prompt, **kwargs)
                # 成功则重置失败计数
                self.failure_counts[name] = 0
                return {"result": result, "provider": name}
            except Exception as e:
                self.failure_counts[name] = self.failure_counts.get(name, 0) + 1
                logger.warning(f"LLM 提供商 {name} 失败 ({self.failure_counts[name]}次): {e}")

                if i < len(self.providers) - 1:
                    continue  # 尝试下一个
                else:
                    raise Exception(f"所有 LLM 提供商不可用: {[p['name'] for p in self.providers]}")

    def get_health_status(self) -> dict:
        """获取提供商健康状态"""
        return {
            p["name"]: {
                "recent_failures": self.failure_counts.get(p["name"], 0),
                "healthy": self.failure_counts.get(p["name"], 0) < 3,
            }
            for p in self.providers
        }
```

---

<!-- chunk: 6. 成熟度模型实施指南 -->## 6. 成熟度模型实施指南

## 6.1 五级成熟度详细定义

```
Agent Harness 成熟度五级:

L1 - 裸 Agent（Ad-hoc）
  特征:
    - 直接调用 LLM API
    - 无循环、无工具、无验证
    - 手动触发，手动查看结果
  风险: 幻觉率高、不可控、无审计
  适用: PoC 验证阶段
  
  升级到 L2 的关键动作:
    □ 实现基础 Agent Loop
    □ 接入至少 2 个工具
    □ 添加超时保护

L2 - 基础 Harness（Managed）
  特征:
    - 有 Agent Loop + 基本工具调用
    - 有超时和迭代限制
    - 但无验证、无约束、无持久化
  风险: 输出质量不稳定、无安全边界
  适用: 内部工具、非关键场景
  
  升级到 L3 的关键动作:
    □ 添加验证层（至少 3 个验证器）
    □ 实现约束层（只读+命令黑名单）
    □ 添加基本的 Prometheus 指标
    □ 建立 CI/CD 质量门禁

L3 - 生产就绪 Harness（Production-Ready）
  特征:
    - 六层架构完整
    - 有 CI/CD 质量门禁
    - 有基本监控和告警
    - 有基线对比和回归检测
  风险: 单点问题、扩展性有限
  适用: 生产级诊断 Agent、运维助手
  
  升级到 L4 的关键动作:
    □ 实现多 Agent 编排
    □ 部署灰度发布流程
    □ 完整 OTel + Langfuse 可观测性
    □ 实现 A/B 测试框架
    □ 部署红队测试

L4 - 企业级 Harness（Enterprise）
  特征:
    - 多 Agent 编排 + 分层 Harness
    - 灰度发布 + A/B 测试
    - 完整可观测性
    - 红队测试通过
    - LLM 提供商容灾
  风险: 运维复杂度高
  适用: 企业 AIOps 平台、核心业务系统
  
  升级到 L5 的关键动作:
    □ 实现 Meta-Agent 自动优化 Harness
    □ 自动调整工具集和上下文策略
    □ 失败模式自动学习和适应
    □ 跨任务知识迁移

L5 - 自进化 Harness（Self-Evolving）
  特征:
    - Harness 配置由 Meta-Agent 自动优化
    - 自动调整工具集、上下文策略、约束参数
    - 从失败中自动学习并改进
    - 跨集群/跨场景知识迁移
  风险: 控制复杂度（需要元约束层）
  适用: 下一代自适应 Agent 平台（前沿研究）
```

## 6.2 成熟度评估清单

```python
class MaturityAssessment:
    """Harness 成熟度评估"""

    CHECKLIST = {
        "L1": [
            ("有 LLM 调用", True),
        ],
        "L2": [
            ("有 Agent Loop", True),
            ("有工具调用（>=2 个工具）", True),
            ("有超时保护", True),
            ("有最大迭代限制", True),
        ],
        "L3": [
            ("有验证层（>=3 个验证器）", True),
            ("有约束层（只读+黑名单）", True),
            ("有上下文管理（分层构建）", True),
            ("有持久化（执行记录持久存储）", True),
            ("有 Prometheus 指标", True),
            ("有 CI/CD 质量门禁", True),
            ("有基线对比", True),
            ("有基本告警规则", True),
        ],
        "L4": [
            ("有多 Agent 编排", True),
            ("有灰度发布流程", True),
            ("有 A/B 测试", True),
            ("有 OTel 全链路追踪", True),
            ("有 Langfuse 集成", True),
            ("有红队测试", True),
            ("有 LLM 提供商容灾", True),
            ("有 SLA 监控", True),
            ("有配置热更新", True),
            ("有 Prompt 版本管理", True),
        ],
        "L5": [
            ("有 Meta-Agent 自优化", True),
            ("有自动工具集调整", True),
            ("有失败模式自动学习", True),
            ("有跨场景知识迁移", True),
        ],
    }

    def assess(self, harness_capabilities: dict) -> dict:
        """评估成熟度等级"""
        achieved_level = "L1"

        for level in ["L1", "L2", "L3", "L4", "L5"]:
            items = self.CHECKLIST[level]
            met = all(
                harness_capabilities.get(item[0], False)
                for item in items
            )
            if met:
                achieved_level = level
            else:
                break

        return {
            "current_level": achieved_level,
            "next_level": self._next_level(achieved_level),
            "gap_analysis": self._gap_analysis(achieved_level,
                                                harness_capabilities),
        }

    def _next_level(self, current: str) -> str:
        levels = ["L1", "L2", "L3", "L4", "L5"]
        idx = levels.index(current)
        return levels[idx + 1] if idx < len(levels) - 1 else "L5 (已达最高)"

    def _gap_analysis(self, current: str, capabilities: dict) -> list:
        """差距分析：列出升级到下一级需要的能力"""
        next_level = self._next_level(current)
        if next_level.startswith("L5 "):
            return []

        items = self.CHECKLIST[next_level]
        gaps = []
        for item_name, _ in items:
            if not capabilities.get(item_name, False):
                gaps.append(item_name)
        return gaps
```

---

<!-- chunk: 7. 最佳实践 -->## 7. 最佳实践

## 7.1 生产运维核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **灰度优先** | 新 Harness 必须经过灰度验证 | 四阶段灰度发布 |
| **可回滚** | 任何变更都能快速回滚 | 保留 N-2 版本 |
| **配置分离** | Prompt 和代码分开管理 | ConfigMap + 热更新 |
| **容灾设计** | LLM 提供商不可用时有降级方案 | 多提供商容灾 |
| **SLA 驱动** | 有明确的质量和性能目标 | SLA 监控 + 告警 |
| **渐进成熟** | 按成熟度模型逐步提升 | L1→L2→L3 渐进升级 |

## 7.2 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **直接全量发布** | 新 Harness 质量未验证 | 四阶段灰度 |
| **Prompt 硬编码** | 修改需重新部署 | ConfigMap + 热更新 |
| **单 LLM 提供商** | 提供商问题 = 全面瘫痪 | 多提供商容灾 |
| **无版本管理** | Prompt 变更无法追溯 | 版本管理 + diff |
| **跳级成熟度** | 基础不牢，高级功能不可靠 | 按 L1→L5 逐步建设 |

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | 成熟度模型概述 |
| [34 - 验证与质量门禁](./34-agent-harness-verification-quality.md) | CI/CD 质量门禁和灰度评估 |
| [36 - 可观测性](./observability.md|36-agent-harness-observability]].md) | 生产监控和告警体系 |
| [09 - 生产部署指南](./09-production-deployment-guide.md) | K8S 部署基础设施 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| Anthropic | Agent 生产部署最佳实践 | 2026-02 |
| Martin Fowler / Birgitta Böckeler | Harness Engineering 生产化指南 | 2026-02 |
| Google SRE | SLA/SLO/SLI 体系 | 持续更新 |
| LangChain | Agent 灰度发布实践 | 2026-02 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，深入展开 Agent Harness 生产运维与成熟度模型。*

---

<!-- chunk: Obsidian 相关文档 -->## Obsidian 相关文档

- topic-ai-agent MOC
- [[domain-14-ai-ml-infra/topic-ai-agent/README.md|AI Agent 工程专题]]
- [[domain-14-ai-ml-infra/topic-ai-agent/01-ai-agent-fundamentals.md|AI Agent 基础与核心架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/02-llm-foundation-models.md|LLM 基座模型选型与评估]]
- [[domain-14-ai-ml-infra/topic-ai-agent/03-agent-frameworks-comparison.md|主流 Agent 框架深度对比]]
- [[domain-14-ai-ml-infra/topic-ai-agent/04-rag-knowledge-retrieval.md|RAG 检索增强生成深度指南]]
- [[domain-14-ai-ml-infra/topic-ai-agent/05-tool-use-function-calling.md|Tool Use & Function Calling 设计规范]]
- [[domain-14-ai-ml-infra/topic-ai-agent/06-multi-agent-orchestration.md|多 Agent 编排与协作架构]]
- [[domain-14-ai-ml-infra/topic-ai-agent/07-memory-context-management.md|记忆管理与上下文窗口工程]]
- [[domain-14-ai-ml-infra/topic-ai-agent/08-agent-evaluation-observability.md|Agent 评测体系与可观测性]]
- [[domain-14-ai-ml-infra/topic-ai-agent/09-production-deployment-guide.md|生产部署指南：K8s 上运行 Agent 服务]]
- [[domain-14-ai-ml-infra/topic-ai-agent/10-security-guardrails.md|安全护栏、提示注入防护与合规]]

## See Also

- 38-agent-harness-performance-cost
- 39-agent-harness-testing-benchmark
- 41-react-harness-identification-guide
- 42-model-harness-compatibility-matrix


<!-- risk-assessed -->
