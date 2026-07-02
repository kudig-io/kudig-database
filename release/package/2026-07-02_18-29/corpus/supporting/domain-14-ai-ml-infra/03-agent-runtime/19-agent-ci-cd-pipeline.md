---
title: Agent CI/CD流水线
description: 'Agent as Code、Prompt版本管理、自动化测试、渐进式部署与Rollback策略'
summary: 'Agent as Code、Prompt版本管理、自动化测试、渐进式部署与Rollback策略'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- ci-cd
- testing
- deployment
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- DevOps 工程师
- 平台工程师
estimated_read_time: 20min
intent_queries:
- Agent CI/CD流水线 是什么
- 如何实现Agent持续部署
- Prompt版本管理
- Agent自动化测试
trigger_keywords:
- agent ci cd
- prompt versioning
- agent testing
- canary deployment
- rollback
prerequisites:
- llm-basics
- kubernetes-basics
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


# Agent CI/CD流水线

## 概述

Agent的CI/CD不同于传统软件：Agent的行为由Prompt、工具配置、模型参数共同决定，这些"代码"的变更无法用传统编译器验证。Prompt的微小改动可能导致Agent行为剧变，而LLM的非确定性使得回归测试尤为关键。

本文覆盖Agent as Code理念、Prompt版本管理、三层自动化测试、渐进式部署和Rollback策略。

## 1. Agent as Code

### 1.1 Agent配置版本化

将Agent的全部配置——Prompt、工具定义、模型参数、知识库绑定——以代码形式管理：

```yaml
# agent-config.yaml - Agent完整配置
apiVersion: agent/v1
kind: AgentConfig
metadata:
  name: customer-service-agent
  version: "2.3.1"
  labels:
    team: platform
    env: production

spec:
  # 模型配置
  model:
    primary: gpt-4o
    fallback: claude-sonnet
    parameters:
      temperature: 0.7
      max_tokens: 4096
      top_p: 0.9

  # 系统提示词
  system_prompt: |
    你是{{company_name}}的客服助手。
    规则：
    1. 使用友好专业的语气回答问题
    2. 遇到无法回答的问题，转接人工客服
    3. 不透露内部系统信息
    4. 引用知识库内容时标注来源

  # 工具定义
  tools:
    - name: search_products
      description: 搜索产品目录
      schema:
        type: object
        properties:
          keyword:
            type: string
          category:
            type: string
      implementation:
        type: http
        endpoint: https://api.internal/products/search
        method: GET
        timeout: 5s

    - name: create_ticket
      description: 创建工单
      schema:
        type: object
        properties:
          title:
            type: string
          description:
            type: string
          priority:
            type: string
            enum: [low, medium, high]
      implementation:
        type: http
        endpoint: https://api.internal/tickets
        method: POST
        timeout: 10s

  # 知识库
  knowledge_bases:
    - id: product-docs
      retrieval:
        top_k: 5
        similarity_threshold: 0.7
        rerank: true

  # 安全策略
  safety:
    content_filter: true
    max_tool_calls_per_turn: 5
    allowed_domains:
      - "*.internal.com"
    blocked_patterns:
      - "密码|password|secret"

  # 部署策略
  deployment:
    strategy: canary
    canary_percentage: 10
    health_check:
      endpoint: /health
      interval: 30s
      timeout: 5s
```

### 1.2 GitOps工作流

```
┌─────────────────────────────────────────────────────────┐
│                 Agent GitOps Pipeline                    │
│                                                          │
│  ┌──────┐    ┌──────────┐    ┌──────────┐    ┌───────┐│
│  │ Git  │───→│  CI Build │───→│  Test    │───→│Deploy ││
│  │ Push │    │          │    │          │    │       ││
│  └──────┘    │ - Lint   │    │ - Unit   │    │ Canary││
│              │ - Valid  │    │ - Integ  │    │ Blue/ ││
│              │ - Diff   │    │ - E2E    │    │ Green ││
│              └──────────┘    └──────────┘    └───────┘│
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  ArgoCD / Flux → K8s Agent Runtime               │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

```yaml
# GitHub Actions: Agent CI/CD Pipeline
name: Agent CI/CD

on:
  push:
    paths:
      - 'agents/**'
      - 'prompts/**'
  pull_request:
    paths:
      - 'agents/**'

jobs:
  validate:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4

      - name: Lint Agent Config
        run: |
          # YAML Schema验证
          ajv validate -s agent-schema.json -d agents/*.yaml
          # Prompt质量检查
          python scripts/lint_prompts.py agents/

      - name: Diff Analysis
        run: |
          # 分析变更影响
          python scripts/diff_analysis.py \
            --base main \
            --head ${{ github.sha }} \
            --output impact-report.json

  test:
    needs: validate
    runs-on: ubuntu-latest
    strategy:
      matrix:
        test-suite: [unit, integration, regression]
    steps:
      - name: Run ${{ matrix.test-suite }} Tests
        run: |
          python -m pytest tests/${{ matrix.test-suite }}/ \
            --junitxml=results/${{ matrix.test-suite }}.xml

  deploy:
    needs: test
    if: github.ref == 'refs/heads/main'
    runs-on: ubuntu-latest
    steps:
      - name: Deploy Canary
        run: |
          kubectl apply -f agents/canary-deployment.yaml

      - name: Monitor Canary
        run: |
          python scripts/canary_monitor.py \
            --duration 300 \
            --success-threshold 0.95

      - name: Promote or Rollback
        run: |
          python scripts/promote_or_rollback.py
```

## 2. Prompt版本管理

### 2.1 Prompt版本化存储

```python
from datetime import datetime
from dataclasses import dataclass, field
from typing import Optional
import hashlib

@dataclass
class PromptVersion:
    """Prompt版本"""
    version_id: str
    content: str
    hash: str
    created_at: datetime
    author: str
    message: str
    tags: list[str] = field(default_factory=list)
    metadata: dict = field(default_factory=dict)

    @staticmethod
    def compute_hash(content: str) -> str:
        return hashlib.sha256(content.encode()).hexdigest()[:12]


class PromptVersionManager:
    """Prompt版本管理器"""

    def __init__(self, storage_backend):
        self.storage = storage_backend

    def commit(
        self,
        prompt_name: str,
        content: str,
        author: str,
        message: str,
        tags: Optional[list[str]] = None
    ) -> PromptVersion:
        """提交新版本"""
        version_id = self._next_version(prompt_name)
        version = PromptVersion(
            version_id=version_id,
            content=content,
            hash=PromptVersion.compute_hash(content),
            created_at=datetime.utcnow(),
            author=author,
            message=message,
            tags=tags or [],
        )

        self.storage.save(prompt_name, version)
        return version

    def get(self, prompt_name: str, version: str) -> PromptVersion:
        """获取指定版本"""
        return self.storage.load(prompt_name, version)

    def get_latest(self, prompt_name: str) -> PromptVersion:
        """获取最新版本"""
        return self.storage.load_latest(prompt_name)

    def list_versions(self, prompt_name: str) -> list[PromptVersion]:
        """列出所有版本"""
        return self.storage.list_all(prompt_name)

    def diff(self, prompt_name: str, v1: str, v2: str) -> str:
        """对比两个版本差异"""
        p1 = self.get(prompt_name, v1)
        p2 = self.get(prompt_name, v2)

        import difflib
        diff = difflib.unified_diff(
            p1.content.splitlines(keepends=True),
            p2.content.splitlines(keepends=True),
            fromfile=f"{prompt_name}@{v1}",
            tofile=f"{prompt_name}@{v2}",
        )
        return ''.join(diff)

    def tag(self, prompt_name: str, version: str, tag: str):
        """为版本打标签"""
        v = self.get(prompt_name, version)
        if tag not in v.tags:
            v.tags.append(tag)
            self.storage.save(prompt_name, v)

    def _next_version(self, prompt_name: str) -> str:
        versions = self.list_versions(prompt_name)
        if not versions:
            return "v1.0.0"
        latest = versions[-1].version_id
        # 语义化版本
        major, minor, patch = latest.lstrip('v').split('.')
        return f"v{major}.{minor}.{int(patch) + 1}"
```

### 2.2 Prompt评估与测试

```python
@dataclass
class EvalCase:
    """评估用例"""
    input: str
    expected_output: str
    expected_contains: list[str] = field(default_factory=list)
    expected_tools: list[str] = field(default_factory=list)
    tags: list[str] = field(default_factory=list)

@dataclass
class EvalResult:
    """评估结果"""
    case_id: str
    passed: bool
    score: float
    actual_output: str
    metrics: dict

class PromptEvaluator:
    """Prompt评估器"""

    def __init__(self, agent_factory, eval_cases: list[EvalCase]):
        self.agent_factory = agent_factory
        self.cases = eval_cases

    async def evaluate(self, prompt_version: str) -> dict:
        """评估指定版本的Prompt"""
        agent = self.agent_factory(prompt_version=prompt_version)
        results: list[EvalResult] = []

        for i, case in enumerate(self.cases):
            result = await self._run_case(agent, case, i)
            results.append(result)

        # 汇总统计
        total = len(results)
        passed = sum(1 for r in results if r.passed)
        avg_score = sum(r.score for r in results) / total

        return {
            "prompt_version": prompt_version,
            "total_cases": total,
            "passed": passed,
            "failed": total - passed,
            "pass_rate": passed / total,
            "avg_score": avg_score,
            "details": results,
        }

    async def _run_case(self, agent, case: EvalCase, case_id: int) -> EvalResult:
        """运行单个评估用例"""
        actual_output = await agent.execute(case.input)

        # 内容匹配
        contains_pass = all(
            keyword in actual_output for keyword in case.expected_contains
        )

        # 语义相似度（使用LLM评判）
        semantic_score = await self._semantic_similarity(
            case.expected_output, actual_output
        )

        passed = contains_pass and semantic_score > 0.7
        score = semantic_score

        return EvalResult(
            case_id=f"case_{case_id}",
            passed=passed,
            score=score,
            actual_output=actual_output,
            metrics={
                "contains_pass": contains_pass,
                "semantic_score": semantic_score,
            }
        )

    async def _semantic_similarity(self, expected: str, actual: str) -> float:
        """语义相似度评分"""
        # 使用LLM评判
        judge_prompt = f"""
        评分标准：回答与期望的语义相似度（0-1分）
        期望回答: {expected}
        实际回答: {actual}
        只输出分数（0-1之间的数字）。
        """
        # 简化实现
        return 0.85
```

## 3. 自动化测试

### 3.1 三层测试体系

```
┌─────────────────────────────────────────────────────┐
│              Agent 测试金字塔                         │
│                                                      │
│                    ┌──────┐                          │
│                    │ E2E  │  真实API + Golden Dataset │
│                    │      │  少量、高成本、高置信      │
│                   ─┴──────┴─                         │
│                  ┌──────────┐                        │
│                  │集成测试    │  真实API + Mock工具     │
│                  │           │  中量、中成本           │
│                 ─┴──────────┴─                       │
│                ┌──────────────┐                      │
│                │  单元测试      │  Mock LLM + Mock工具 │
│                │               │  大量、低成本、快速    │
│               ─┴──────────────┴─                     │
└─────────────────────────────────────────────────────┘
```

### 3.2 单元测试（Mock LLM）

```python
import pytest
from unittest.mock import AsyncMock, MagicMock

class MockLLMClient:
    """Mock LLM客户端"""

    def __init__(self, responses: list[dict]):
        self.responses = responses
        self.call_count = 0
        self.call_history: list[dict] = []

    async def chat(self, messages, tools=None, **kwargs):
        response = self.responses[self.call_count % len(self.responses)]
        self.call_count += 1
        self.call_history.append({
            "messages": messages,
            "tools": tools,
            "kwargs": kwargs,
        })
        return MagicMock(**response)


class TestAgentUnit:
    """Agent单元测试"""

    @pytest.fixture
    def mock_llm(self):
        return MockLLMClient(responses=[
            {"content": "您好！请问有什么可以帮助您的？", "tool_calls": []},
        ])

    @pytest.fixture
    def agent(self, mock_llm):
        return Agent(
            llm=mock_llm,
            system_prompt="你是客服助手",
            tools=[],
        )

    @pytest.mark.asyncio
    async def test_basic_response(self, agent):
        """测试基本响应"""
        result = await agent.execute("你好")
        assert "您好" in result or "你好" in result

    @pytest.mark.asyncio
    async def test_tool_calling(self, mock_llm):
        """测试工具调用"""
        mock_llm.responses = [
            {"content": "", "tool_calls": [
                MagicMock(name="search", arguments='{"keyword": "iPhone"}')
            ]},
            {"content": "iPhone 15 价格为 $999", "tool_calls": []},
        ]

        mock_tool = AsyncMock(return_value='{"products": [{"name": "iPhone 15", "price": 999}]}')

        agent = Agent(
            llm=mock_llm,
            tools=[{"name": "search", "func": mock_tool}],
        )

        result = await agent.execute("搜索iPhone价格")
        mock_tool.assert_called_once()
        assert "999" in result

    @pytest.mark.asyncio
    async def test_max_tool_calls(self, mock_llm):
        """测试工具调用次数限制"""
        # 始终返回工具调用
        mock_llm.responses = [
            {"content": "", "tool_calls": [
                MagicMock(name="search", arguments='{}')
            ]},
        ] * 20

        agent = Agent(
            llm=mock_llm,
            tools=[{"name": "search", "func": AsyncMock(return_value="{}")}],
            max_tool_calls=5,
        )

        result = await agent.execute("搜索")
        assert mock_llm.call_count <= 6  # 5次工具调用 + 1次最终回答
```

### 3.3 集成测试（真实API + Mock工具）

```python
class TestAgentIntegration:
    """Agent集成测试（使用真实LLM API）"""

    @pytest.fixture
    def agent(self):
        return Agent(
            llm=RealLLMClient(model="gpt-4o-mini"),  # 使用小模型降低成本
            system_prompt="你是测试助手",
            tools=[
                {
                    "name": "get_time",
                    "description": "获取当前时间",
                    "func": lambda: datetime.now().isoformat(),
                }
            ],
        )

    @pytest.mark.asyncio
    async def test_tool_selection(self, agent):
        """测试工具选择准确性"""
        result = await agent.execute("现在几点了？")
        # 验证工具被调用
        assert any(t["name"] == "get_time" for t in agent.last_tool_calls)

    @pytest.mark.asyncio
    async def test_no_unnecessary_tool_call(self, agent):
        """测试不调用不必要的工具"""
        result = await agent.execute("1+1等于几？")
        # 验证没有调用工具
        assert len(agent.last_tool_calls) == 0

    @pytest.mark.asyncio
    async def test_error_handling(self, agent):
        """测试错误处理"""
        agent.tools[0]["func"] = lambda: (_ for _ in ()).throw(Exception("API错误"))
        result = await agent.execute("现在几点了？")
        # 验证错误被优雅处理
        assert "错误" in result or "抱歉" in result or "问题" in result
```

### 3.4 回归测试（Golden Dataset）

```python
@dataclass
class GoldenCase:
    """Golden Dataset用例"""
    id: str
    category: str
    input: str
    expected_behavior: str
    expected_contains: list[str]
    expected_tools: list[str]
    max_latency_ms: int
    max_cost_usd: float

class GoldenDatasetRegression:
    """Golden Dataset回归测试"""

    def __init__(self, golden_cases: list[GoldenCase]):
        self.cases = golden_cases
        self.results: list[dict] = []

    async def run_regression(
        self,
        agent_factory,
        model: str = "gpt-4o-mini"
    ) -> dict:
        """运行完整回归测试"""
        agent = agent_factory(model=model)

        for case in self.cases:
            start_time = time.time()
            result = await agent.execute(case.input)
            latency_ms = (time.time() - start_time) * 1000

            # 验证
            contains_pass = all(kw in result for kw in case.expected_contains)
            tools_pass = self._check_tools(agent.last_tool_calls, case.expected_tools)
            latency_pass = latency_ms <= case.max_latency_ms

            self.results.append({
                "case_id": case.id,
                "category": case.category,
                "contains_pass": contains_pass,
                "tools_pass": tools_pass,
                "latency_pass": latency_pass,
                "latency_ms": latency_ms,
                "passed": contains_pass and tools_pass and latency_pass,
            })

        return self._generate_report()

    def _check_tools(self, actual_calls: list, expected_tools: list[str]) -> bool:
        if not expected_tools:
            return len(actual_calls) == 0
        actual_names = [c["name"] for c in actual_calls]
        return all(t in actual_names for t in expected_tools)

    def _generate_report(self) -> dict:
        total = len(self.results)
        passed = sum(1 for r in self.results if r["passed"])

        by_category = {}
        for r in self.results:
            cat = r["category"]
            if cat not in by_category:
                by_category[cat] = {"total": 0, "passed": 0}
            by_category[cat]["total"] += 1
            if r["passed"]:
                by_category[cat]["passed"] += 1

        return {
            "total": total,
            "passed": passed,
            "pass_rate": passed / total,
            "by_category": by_category,
            "failures": [r for r in self.results if not r["passed"]],
        }
```

## 4. 渐进式部署

### 4.1 Canary Agent部署

```yaml
# Canary Agent Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: customer-agent-canary
  labels:
    app: customer-agent
    version: canary
spec:
  replicas: 1  # Canary仅1个副本
  selector:
    matchLabels:
      app: customer-agent
      version: canary
  template:
    metadata:
      labels:
        app: customer-agent
        version: canary
    spec:
      containers:
      - name: agent
        image: agent-runtime:v2.3.1-canary
        env:
        - name: AGENT_VERSION
          value: "v2.3.1"
        - name: CANARY_WEIGHT
          value: "10"  # 10%流量
        resources:
          requests:
            cpu: "500m"
            memory: "512Mi"
          limits:
            cpu: "2"
            memory: "2Gi"
        readinessProbe:
          httpGet:
            path: /health
            port: 8080
          initialDelaySeconds: 10
          periodSeconds: 5
---
# 稳定版Deployment
apiVersion: apps/v1
kind: Deployment
metadata:
  name: customer-agent-stable
  labels:
    app: customer-agent
    version: stable
spec:
  replicas: 9  # 稳定版9个副本
  selector:
    matchLabels:
      app: customer-agent
      version: stable
  template:
    metadata:
      labels:
        app: customer-agent
        version: stable
    spec:
      containers:
      - name: agent
        image: agent-runtime:v2.3.0
        env:
        - name: AGENT_VERSION
          value: "v2.3.0"
        - name: CANARY_WEIGHT
          value: "90"  # 90%流量
---
# Istio流量分割
apiVersion: networking.istio.io/v1beta1
kind: VirtualService
metadata:
  name: customer-agent-vs
spec:
  hosts:
  - customer-agent
  http:
  - route:
    - destination:
        host: customer-agent
        subset: stable
      weight: 90
    - destination:
        host: customer-agent
        subset: canary
      weight: 10
```

### 4.2 Canary监控与自动决策

```python
class CanaryMonitor:
    """Canary部署监控"""

    def __init__(
        self,
        duration_seconds: int = 300,
        success_threshold: float = 0.95,
        latency_threshold_ms: float = 2000,
    ):
        self.duration = duration_seconds
        self.success_threshold = success_threshold
        self.latency_threshold = latency_threshold_ms
        self.metrics: list[dict] = []

    async def monitor(self, canary_endpoint: str, stable_endpoint: str) -> dict:
        """监控Canary与Stable的对比"""
        start_time = time.time()

        while time.time() - start_time < self.duration:
            # 采集Canary指标
            canary_metrics = await self._collect_metrics(canary_endpoint)
            stable_metrics = await self._collect_metrics(stable_endpoint)

            self.metrics.append({
                "timestamp": time.time(),
                "canary": canary_metrics,
                "stable": stable_metrics,
            })

            # 实时检查是否需要回滚
            if self._should_rollback(canary_metrics, stable_metrics):
                return {
                    "decision": "rollback",
                    "reason": "Canary指标显著差于Stable",
                    "metrics": self.metrics[-1],
                }

            await asyncio.sleep(10)

        # 分析结果
        return self._analyze_results()

    def _should_rollback(self, canary: dict, stable: dict) -> bool:
        """判断是否需要回滚"""
        # 成功率下降超过5%
        if canary["success_rate"] < stable["success_rate"] - 0.05:
            return True
        # 延迟增加超过50%
        if canary["p99_latency"] > stable["p99_latency"] * 1.5:
            return True
        # 错误率超过阈值
        if canary["error_rate"] > 0.1:
            return True
        return False

    def _analyze_results(self) -> dict:
        """分析Canary结果"""
        canary_avg_success = sum(m["canary"]["success_rate"] for m in self.metrics) / len(self.metrics)
        canary_avg_latency = sum(m["canary"]["p99_latency"] for m in self.metrics) / len(self.metrics)

        if canary_avg_success >= self.success_threshold and canary_avg_latency <= self.latency_threshold:
            decision = "promote"
        else:
            decision = "rollback"

        return {
            "decision": decision,
            "avg_success_rate": canary_avg_success,
            "avg_p99_latency": canary_avg_latency,
            "sample_count": len(self.metrics),
        }
```

## 5. Rollback策略

### 5.1 多级回滚

```yaml
回滚策略:

Level 1: 流量回滚（秒级）
  方法: Istio VirtualService权重调整
  操作: canary weight=0, stable weight=100
  影响: 无中断，平滑切换
  适用: Canary指标异常

Level 2: 版本回滚（分钟级）
  方法: Deployment回滚
  操作: kubectl rollout undo deployment/agent
  影响: 短暂中断（滚动更新）
  适用: Level 1无法解决

Level 3: 配置回滚（分钟级）
  方法: Git revert + ArgoCD同步
  操作: git revert <commit> && argocd app sync
  影响: 配置完全回退
  适用: Prompt/配置变更导致问题

Level 4: 数据回滚（小时级）
  方法: 知识库/向量数据库恢复
  操作: 从备份恢复知识库
  影响: 数据回退，可能丢失新数据
  适用: 知识库更新导致幻觉增加
```

### 5.2 自动回滚脚本

```python
class AgentRollbackManager:
    """Agent回滚管理器"""

    def __init__(self, k8s_client, argocd_client):
        self.k8s = k8s_client
        self.argocd = argocd_client

    async def auto_rollback(self, deployment: str, namespace: str, level: int = 1):
        """自动回滚"""
        if level == 1:
            await self._traffic_rollback(deployment, namespace)
        elif level == 2:
            await self._version_rollback(deployment, namespace)
        elif level == 3:
            await self._config_rollback(deployment, namespace)

    async def _traffic_rollback(self, deployment: str, namespace: str):
        """Level 1: 流量回滚"""
        # Istio VirtualService权重调整
        vs_patch = {
            "spec": {
                "http": [{
                    "route": [
                        {"destination": {"host": f"{deployment}", "subset": "stable"}, "weight": 100},
                        {"destination": {"host": f"{deployment}", "subset": "canary"}, "weight": 0},
                    ]
                }]
            }
        }
        await self.k8s.patch_virtual_service(deployment, namespace, vs_patch)

    async def _version_rollback(self, deployment: str, namespace: str):
        """Level 2: 版本回滚"""
        await self.k8s.rollback_deployment(deployment, namespace)

    async def _config_rollback(self, deployment: str, namespace: str):
        """Level 3: 配置回滚"""
        await self.argocd.rollback(deployment)
```

### 5.3 K8s部署配置

```yaml
# Agent CI/CD组件
apiVersion: apps/v1
kind: Deployment
metadata:
  name: agent-cicd-controller
  namespace: agent-system
spec:
  replicas: 1
  selector:
    matchLabels:
      app: agent-cicd
  template:
    spec:
      containers:
      - name: controller
        image: agent-cicd-controller:latest
        env:
        - name: ARGOCD_SERVER
          value: "argocd.argocd.svc.cluster.local"
        - name: PROMETHEUS_URL
          value: "http://prometheus.monitoring.svc.cluster.local:9090"
        - name: CANARY_DURATION
          value: "300"
        - name: SUCCESS_THRESHOLD
          value: "0.95"
        resources:
          requests:
            cpu: "250m"
            memory: "256Mi"
```

## 相关主题

- [[domain-14-ai-ml-infra/03-agent-runtime/18-agent-retry-resilience|Agent弹性设计]]
- [[domain-14-ai-ml-infra/03-agent-runtime/20-agent-multi-tenancy|Agent多租户架构]]
- [[domain-14-ai-ml-infra/03-agent-runtime/21-agent-runtime-architecture-overview|Agent Runtime架构总览]]

## 参考资料

- Prompt Engineering Guide
- Promptfoo Evaluation Framework
- ArgoCD GitOps
- Istio Traffic Management


<!-- risk-assessed -->
