---
title: Agent Harness 验证与质量门禁 (domain-14-ai-ml-infra)
description: 'description: ''**文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**:
  Verification,'
summary: 'description: ''**文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Verification,'
category: general
tags:
- ai
- ai-agent
- etcd
- helm
- docker
- llm
- rag
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 35min
intent_queries:
- Agent Harness 验证与质量门禁 是什么
- 如何 Agent Harness 验证与质量门禁
- Kubernetes 14 ai ml infra 最佳实践
trigger_keywords:
- Agent
- Harness
- 验证与质量门禁
- ai
- ml
- infra
prerequisites:
- kubectl-basics
- helm-basics
- etcd-basics
---



title: Agent Harness 验证与质量门禁
description: '**文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Verification,
  Quality Gate, 自检循环, LLM-as-Judge, RAGAS, 幻觉检测, 事实一致性, CI/CD, 回归测试, 灰度评估'
category: ai-agent
tags:
- ai
- agent
- llm
- rag
- multi-agent
- [[etcd|etcd]]
- [[Helm|helm]]
- docker
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 架构师
- SRE
estimated_read_time: 5min
intent_queries:
- Agent Harness 验证与质量门禁 是什么
- 如何 Agent Harness 验证与质量门禁
trigger_keywords:
- Agent
- Harness
- 验证与质量门禁
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

# Agent Harness 验证与质量门禁

> **文档类型**: Harness 工程深入专题 | **最后更新**: 2026-04 | **关键词**: Verification, Quality Gate, 自检循环, LLM-as-Judge, RAGAS, 幻觉检测, 事实一致性, CI/CD, 回归测试, 灰度评估

---

<!-- chunk: 概述 -->## 概述

Verification（验证层）是 Agent Harness 六层架构的第五层，也是 Harness 区别于"裸 Agent"的**关键分水岭**。LangChain 的实验表明，仅添加自检循环就将基准分提升了 13.7%——这是所有 Harness 改进中最高效的单一变更。

本文系统阐述验证层的多维度验证策略、LLM-as-Judge 评估范式、RAGAS 评测框架集成、CI/CD 质量门禁、A/B 测试与灰度发布，以及针对 K8S 运维场景的自定义验证器设计。

---

<!-- chunk: 1. 验证层核心理论 -->## 1. 验证层核心理论

## 1.1 为什么验证是最高 ROI 的 Harness 改进

```
验证层 ROI 实证数据:

LangChain 编码 Agent（2026-02 实验）:
  无验证:        基准分 52.8%
  添加自检循环:   基准分 66.5%  → +13.7% 绝对提升
  
  改进分解:
    自检循环:     +13.7% （最高单项改进）
    环境预扫描:   +5.2%
    反漂移检测:   +3.8%
    推理预算优化: +2.5%

Anthropic 长运行 Agent:
  无验证:        任务完成率 71%
  带验证:        任务完成率 89%  → +18% 绝对提升
  
  验证拦截的问题类型:
    - 幻觉输出: 占拦截问题的 40%
    - 格式错误: 占拦截问题的 25%
    - 逻辑不一致: 占拦截问题的 20%
    - 安全风险: 占拦截问题的 15%
```

## 1.2 验证分类体系

```
Agent 输出验证分类:

1. 事实验证（Factual Verification）
   输出的事实是否与上下文/证据一致
   工具: LLM-as-Judge, RAGAS Faithfulness

2. 格式验证（Format Verification）
   输出的 YAML/JSON/命令是否语法正确
   工具: 语法解析器, Schema 校验

3. 安全验证（Safety Verification）
   输出的命令/操作是否安全
   工具: 正则匹配, 命令白名单

4. 完整性验证（Completeness Verification）
   输出是否完整回答了问题的所有部分
   工具: LLM-as-Judge, Checklist

5. 一致性验证（Consistency Verification）
   输出的各部分之间是否逻辑一致
   工具: LLM-as-Judge, 规则引擎

6. 可执行性验证（Executability Verification）
   给出的方案是否在当前环境下可执行
   工具: Dry-run, 环境检查
```

---

<!-- chunk: 2. 多维度验证器设计 -->## 2. 多维度验证器设计

## 2.1 验证器框架

```python
from abc import ABC, abstractmethod
from dataclasses import dataclass, field
from typing import Optional, Any
from enum import Enum

class VerificationSeverity(Enum):
    """验证问题严重性"""
    INFO = "info"           # 信息提示
    WARNING = "warning"     # 警告（不阻塞）
    ERROR = "error"         # 错误（阻塞输出）
    CRITICAL = "critical"   # 严重（立即终止）

@dataclass
class VerificationResult:
    """单个验证结果"""
    verifier: str
    passed: bool
    severity: VerificationSeverity = VerificationSeverity.INFO
    message: str = ""
    details: list = field(default_factory=list)
    score: float = 1.0       # 0.0 - 1.0
    fix_suggestion: str = ""  # 修复建议

@dataclass
class VerificationReport:
    """完整验证报告"""
    overall_passed: bool
    results: list[VerificationResult]
    total_score: float
    blocking_issues: list[VerificationResult]
    warnings: list[VerificationResult]

    @classmethod
    def from_results(cls, results: list[VerificationResult]) -> 'VerificationReport':
        blocking = [r for r in results if not r.passed
                    and r.severity in (VerificationSeverity.ERROR,
                                      VerificationSeverity.CRITICAL)]
        warnings = [r for r in results if not r.passed
                    and r.severity == VerificationSeverity.WARNING]
        scores = [r.score for r in results]
        avg_score = sum(scores) / len(scores) if scores else 0

        return cls(
            overall_passed=len(blocking) == 0,
            results=results,
            total_score=avg_score,
            blocking_issues=blocking,
            warnings=warnings,
        )


class BaseVerifier(ABC):
    """验证器基类"""

    @abstractmethod
    def verify(self, task: str, output: str, context: dict) -> VerificationResult:
        ...

    @property
    @abstractmethod
    def name(self) -> str:
        ...


class VerificationPipeline:
    """验证管线：编排多个验证器"""

    def __init__(self, verifiers: list[BaseVerifier] = None):
        self.verifiers = verifiers or []

    def add_verifier(self, verifier: BaseVerifier):
        self.verifiers.append(verifier)

    def verify_all(self, task: str, output: str, context: dict) -> VerificationReport:
        """运行所有验证器"""
        results = []
        for verifier in self.verifiers:
            try:
                result = verifier.verify(task, output, context)
                results.append(result)

                # CRITICAL 问题立即终止
                if (not result.passed
                        and result.severity == VerificationSeverity.CRITICAL):
                    break
            except Exception as e:
                results.append(VerificationResult(
                    verifier=verifier.name,
                    passed=False,
                    severity=VerificationSeverity.WARNING,
                    message=f"验证器异常: {e}",
                ))

        return VerificationReport.from_results(results)
```

## 2.2 事实一致性验证器

```python
class FactualConsistencyVerifier(BaseVerifier):
    """事实一致性验证：确保输出与上下文证据一致"""

    def __init__(self, judge_llm, threshold: float = 0.85):
        self.judge_llm = judge_llm
        self.threshold = threshold

    @property
    def name(self) -> str:
        return "factual_consistency"

    def verify(self, task: str, output: str, context: dict) -> VerificationResult:
        sources = context.get("sources", "")
        evidence = context.get("evidence", "")

        prompt = f"""
你是一个事实一致性审查员。请严格评估以下回答是否与给定的证据/上下文一致。

<!-- chunk: 任务 -->## 任务
{task}

<!-- chunk: 上下文/证据 -->## 上下文/证据
{sources[:3000]}
{evidence[:2000]}

<!-- chunk: Agent 的回答 -->## Agent 的回答
{output[:3000]}

<!-- chunk: 评估要求 -->## 评估要求
1. 检查回答中的每一个事实性声明
2. 判断每个声明是否有上下文支撑
3. 识别任何幻觉（无依据的声明）

<!-- chunk: 输出格式（JSON） -->## 输出格式（JSON）
{{
    "consistent": true/false,
    "score": 0.0-1.0,
    "unsupported_claims": ["无支撑的声明1", "..."],
    "hallucinations": ["幻觉内容1", "..."],
    "missing_evidence": ["应引用但未引用的证据1", "..."]
}}
"""
        result = self.judge_llm.invoke(prompt)
        parsed = self._parse_json(result)

        score = parsed.get("score", 0)
        passed = score >= self.threshold

        return VerificationResult(
            verifier=self.name,
            passed=passed,
            severity=VerificationSeverity.ERROR if not passed
                     else VerificationSeverity.INFO,
            message=f"事实一致性得分: {score:.2f}",
            score=score,
            details=parsed.get("hallucinations", []),
            fix_suggestion="请基于上下文中的具体证据修正以下幻觉内容: "
                          + "; ".join(parsed.get("hallucinations", [])),
        )

    def _parse_json(self, text: str) -> dict:
        import json, re
        match = re.search(r'\{[\s\S]*\}', text)
        if match:
            try:
                return json.loads(match.group())
            except json.JSONDecodeError:
                pass
        return {"consistent": False, "score": 0.0}
```

## 2.3 命令安全验证器

```python
import re

class CommandSafetyVerifier(BaseVerifier):
    """命令安全验证：拦截危险命令"""

    DANGER_LEVELS = {
        "critical": [
            r"kubectl\s+delete\s+(?:namespace|ns|node)",
            r"kubectl\s+delete\s+--all",
            r"rm\s+-rf\s+/",
            r"DROP\s+(?:TABLE|DATABASE)",
            r"kubectl\s+drain\s+.*--force.*--delete-emptydir-data",
            r"etcdctl\s+del",
        ],
        "high": [
            r"kubectl\s+delete\s+(?:deploy|sts|ds|svc)",
            r"kubectl\s+drain",
            r"kubectl\s+cordon",
            r"helm\s+(?:uninstall|delete)",
            r"kubectl\s+scale.*replicas=0",
        ],
        "medium": [
            r"kubectl\s+(?:apply|patch|edit)",
            r"kubectl\s+rollout\s+undo",
            r"helm\s+(?:upgrade|install)",
            r"kubectl\s+label.*--overwrite",
        ],
    }

    @property
    def name(self) -> str:
        return "command_safety"

    def verify(self, task: str, output: str, context: dict) -> VerificationResult:
        commands = self._extract_commands(output)
        if not commands:
            return VerificationResult(
                verifier=self.name, passed=True,
                message="未检测到命令", score=1.0,
            )

        issues = []
        max_severity = VerificationSeverity.INFO

        for cmd in commands:
            for level, patterns in self.DANGER_LEVELS.items():
                for pattern in patterns:
                    if re.search(pattern, cmd, re.IGNORECASE):
                        severity = {
                            "critical": VerificationSeverity.CRITICAL,
                            "high": VerificationSeverity.ERROR,
                            "medium": VerificationSeverity.WARNING,
                        }[level]

                        issues.append({
                            "command": cmd,
                            "danger_level": level,
                            "pattern": pattern,
                        })

                        if severity.value > max_severity.value:
                            max_severity = severity

        passed = max_severity in (VerificationSeverity.INFO,
                                   VerificationSeverity.WARNING)

        return VerificationResult(
            verifier=self.name,
            passed=passed,
            severity=max_severity,
            message=f"检测到 {len(issues)} 个安全问题" if issues else "命令安全检查通过",
            details=issues,
            score=1.0 - len(issues) * 0.2,
            fix_suggestion="将危险命令替换为只读命令或添加 --dry-run 标志",
        )

    def _extract_commands(self, text: str) -> list:
        """从文本中提取命令"""
        commands = []
        # 提取代码块中的命令
        code_blocks = re.findall(r'```(?:bash|shell|sh)?\n(.*?)```',
                                 text, re.DOTALL)
        for block in code_blocks:
            for line in block.strip().split("\n"):
                line = line.strip()
                if line and not line.startswith("#"):
                    commands.append(line)

        # 提取内联命令
        inline_cmds = re.findall(r'`((?:kubectl|helm|etcdctl|docker)\s+[^`]+)`',
                                 text)
        commands.extend(inline_cmds)

        return commands
```

## 2.4 输出格式验证器

```python
import yaml
import json as json_module

class OutputFormatVerifier(BaseVerifier):
    """输出格式验证：确保 YAML/JSON 语法正确"""

    @property
    def name(self) -> str:
        return "output_format"

    def verify(self, task: str, output: str, context: dict) -> VerificationResult:
        issues = []

        # 验证 YAML 块
        yaml_blocks = re.findall(r'```yaml\n(.*?)```', output, re.DOTALL)
        for i, block in enumerate(yaml_blocks):
            try:
                parsed = yaml.safe_load(block)
                if parsed is None:
                    issues.append({
                        "type": "yaml", "block": i,
                        "error": "YAML 解析结果为空",
                    })
            except yaml.YAMLError as e:
                issues.append({
                    "type": "yaml", "block": i,
                    "error": str(e)[:200],
                    "content_preview": block[:100],
                })

        # 验证 JSON 块
        json_blocks = re.findall(r'```json\n(.*?)```', output, re.DOTALL)
        for i, block in enumerate(json_blocks):
            try:
                json_module.loads(block)
            except json_module.JSONDecodeError as e:
                issues.append({
                    "type": "json", "block": i,
                    "error": str(e)[:200],
                })

        # 验证 kubectl 命令语法
        kubectl_cmds = re.findall(r'`(kubectl\s+[^`]+)`', output)
        for cmd in kubectl_cmds:
            cmd_issues = self._validate_kubectl_syntax(cmd)
            issues.extend(cmd_issues)

        passed = len(issues) == 0
        return VerificationResult(
            verifier=self.name,
            passed=passed,
            severity=VerificationSeverity.ERROR if not passed
                     else VerificationSeverity.INFO,
            message=f"发现 {len(issues)} 个格式问题" if issues else "格式检查通过",
            details=issues,
            score=max(0, 1.0 - len(issues) * 0.15),
            fix_suggestion="修正 YAML/JSON 语法错误",
        )

    def _validate_kubectl_syntax(self, cmd: str) -> list:
        """基本的 kubectl 命令语法检查"""
        issues = []
        parts = cmd.split()
        if len(parts) < 2:
            issues.append({"type": "kubectl", "error": "命令不完整",
                          "command": cmd})
            return issues

        valid_verbs = {"get", "describe", "logs", "top", "apply", "delete",
                       "patch", "scale", "rollout", "exec", "explain",
                       "create", "edit", "label", "annotate", "drain",
                       "cordon", "uncordon", "taint", "events"}
        verb = parts[1]
        if verb not in valid_verbs:
            issues.append({"type": "kubectl", "error": f"未知子命令: {verb}",
                          "command": cmd})

        return issues
```

## 2.5 完整性验证器

```python
class CompletenessVerifier(BaseVerifier):
    """完整性验证：确保回答覆盖了问题的所有方面"""

    def __init__(self, judge_llm):
        self.judge_llm = judge_llm

    @property
    def name(self) -> str:
        return "completeness"

    def verify(self, task: str, output: str, context: dict) -> VerificationResult:
        prompt = f"""
评估以下回答是否完整地回应了任务要求。

<!-- chunk: 任务 -->## 任务
{task}

<!-- chunk: 回答 -->## 回答
{output[:3000]}

<!-- chunk: 评估标准 -->## 评估标准
1. 是否直接回答了核心问题
2. 是否提供了具体的操作步骤
3. 是否包含必要的前置条件和注意事项
4. 是否遗漏了关键信息

<!-- chunk: 输出格式（JSON） -->## 输出格式（JSON）
{{
    "complete": true/false,
    "score": 0.0-1.0,
    "covered_aspects": ["已覆盖的方面1", "..."],
    "missing_aspects": ["遗漏的方面1", "..."],
    "improvement_suggestions": ["改进建议1", "..."]
}}
"""
        result = self.judge_llm.invoke(prompt)
        parsed = self._parse_json(result)

        score = parsed.get("score", 0)
        passed = score >= 0.7

        return VerificationResult(
            verifier=self.name,
            passed=passed,
            severity=VerificationSeverity.WARNING if not passed
                     else VerificationSeverity.INFO,
            message=f"完整性得分: {score:.2f}",
            score=score,
            details=parsed.get("missing_aspects", []),
            fix_suggestion="补充以下遗漏内容: "
                          + "; ".join(parsed.get("missing_aspects", [])),
        )

    def _parse_json(self, text: str) -> dict:
        import json
        match = re.search(r'\{[\s\S]*\}', text)
        if match:
            try:
                return json.loads(match.group())
            except json.JSONDecodeError:
                pass
        return {"complete": False, "score": 0.0}
```

---

<!-- chunk: 3. 自检循环模式 -->## 3. 自检循环模式

## 3.1 自检循环实现

```python
class SelfCheckLoop:
    """自检循环：Agent 完成后自动运行检查清单"""

    def __init__(
        self,
        verification_pipeline: VerificationPipeline,
        max_correction_rounds: int = 2,
        llm=None,
    ):
        self.pipeline = verification_pipeline
        self.max_rounds = max_correction_rounds
        self.llm = llm

    def verify_and_correct(
        self,
        task: str,
        output: str,
        context: dict,
    ) -> dict:
        """验证并自我纠正"""
        correction_history = []

        for round_num in range(self.max_rounds + 1):
            # 运行验证
            report = self.pipeline.verify_all(task, output, context)

            correction_history.append({
                "round": round_num,
                "output_preview": output[:200],
                "passed": report.overall_passed,
                "score": report.total_score,
                "issues": len(report.blocking_issues),
            })

            # 验证通过
            if report.overall_passed:
                return {
                    "status": "passed",
                    "output": output,
                    "report": report,
                    "correction_rounds": round_num,
                    "history": correction_history,
                }

            # 已达最大纠正轮数
            if round_num >= self.max_rounds:
                return {
                    "status": "failed_after_corrections",
                    "output": output,
                    "report": report,
                    "correction_rounds": round_num,
                    "history": correction_history,
                    "unresolved_issues": [
                        {"verifier": r.verifier, "message": r.message}
                        for r in report.blocking_issues
                    ],
                }

            # 自我纠正
            output = self._self_correct(task, output, report, context)

        return {"status": "max_rounds_exceeded", "output": output,
                "history": correction_history}

    def _self_correct(
        self,
        task: str,
        output: str,
        report: VerificationReport,
        context: dict,
    ) -> str:
        """让 LLM 根据验证反馈自我纠正"""
        issues_text = "\n".join([
            f"- [{r.verifier}] {r.message}\n  修复建议: {r.fix_suggestion}"
            for r in report.blocking_issues + report.warnings
        ])

        correction_prompt = f"""
你之前的回答存在以下问题，请修正后重新输出。

<!-- chunk: 原始任务 -->## 原始任务
{task}

<!-- chunk: 你之前的回答 -->## 你之前的回答
{output[:3000]}

<!-- chunk: 验证发现的问题 -->## 验证发现的问题
{issues_text}

<!-- chunk: 要求 -->## 要求
1. 保留正确的部分
2. 修正上述问题
3. 确保 YAML/JSON 语法正确
4. 确保命令安全可执行
5. 确保事实有证据支撑

请直接输出修正后的完整回答，不要包含任何解释。
"""
        corrected = self.llm.invoke(correction_prompt)
        return corrected
```

## 3.2 自检清单模板

```python
class DiagnosisChecklist:
    """K8S 诊断输出自检清单"""

    CHECKLIST = [
        {"id": "root_cause", "question": "是否明确给出了根因分析？",
         "required": True},
        {"id": "evidence", "question": "根因结论是否有具体的 Event/日志证据支撑？",
         "required": True},
        {"id": "commands_safe", "question": "给出的命令是否可安全执行？",
         "required": True},
        {"id": "yaml_valid", "question": "YAML/JSON 是否语法正确？",
         "required": True},
        {"id": "steps_complete", "question": "操作步骤是否完整可执行？",
         "required": True},
        {"id": "risk_assessed", "question": "是否评估了操作风险等级？",
         "required": False},
        {"id": "rollback_plan", "question": "是否提供了回滚方案？",
         "required": False},
        {"id": "confidence", "question": "是否标注了诊断置信度？",
         "required": False},
    ]

    def evaluate(self, output: str, context: dict) -> dict:
        """根据清单评估输出"""
        results = []
        for item in self.CHECKLIST:
            met = self._check_item(item, output)
            results.append({
                "id": item["id"],
                "question": item["question"],
                "met": met,
                "required": item["required"],
            })

        required_met = all(r["met"] for r in results if r["required"])
        total_met = sum(1 for r in results if r["met"])
        score = total_met / len(results)

        return {
            "passed": required_met,
            "score": score,
            "checklist_results": results,
            "missing_required": [
                r["question"] for r in results
                if r["required"] and not r["met"]
            ],
        }

    def _check_item(self, item: dict, output: str) -> bool:
        """检查单项（基于关键词的快速检查）"""
        checks = {
            "root_cause": lambda o: any(kw in o for kw in ["根因", "原因", "root cause"]),
            "evidence": lambda o: any(kw in o for kw in ["Event", "日志", "证据", "log"]),
            "commands_safe": lambda o: "delete" not in o.lower() or "--dry-run" in o,
            "yaml_valid": lambda o: self._check_yaml_blocks(o),
            "steps_complete": lambda o: any(kw in o for kw in ["步骤", "操作", "Step"]),
            "risk_assessed": lambda o: any(kw in o for kw in ["风险", "risk", "影响"]),
            "rollback_plan": lambda o: any(kw in o for kw in ["回滚", "rollback", "恢复"]),
            "confidence": lambda o: any(kw in o for kw in ["置信度", "确定性", "confidence", "%"]),
        }
        checker = checks.get(item["id"], lambda o: True)
        return checker(output)

    def _check_yaml_blocks(self, output: str) -> bool:
        yaml_blocks = re.findall(r'```yaml\n(.*?)```', output, re.DOTALL)
        for block in yaml_blocks:
            try:
                yaml.safe_load(block)
            except yaml.YAMLError:
                return False
        return True
```

---

<!-- chunk: 4. LLM-as-Judge 评估范式 -->## 4. LLM-as-Judge 评估范式

## 4.1 Judge 模型选择策略

```
LLM-as-Judge 模型选择:

原则: Judge 模型必须与生成模型不同（避免同质偏见）

推荐配置:
  生成模型         Judge 模型          适用场景
  GPT-4o           Claude Sonnet 4     通用判断
  Claude Sonnet 4  GPT-4o              通用判断
  GPT-4o-mini      GPT-4o              成本敏感场景
  Gemini 2.5       Claude Sonnet 4     跨厂商评估
  任意开源模型     GPT-4o              开源模型评估

评估维度与权重:
  Faithfulness (忠实度):     权重 30%
  Relevancy (相关性):       权重 25%
  Completeness (完整性):    权重 20%
  Safety (安全性):          权重 15%
  Format (格式正确性):      权重 10%
```

## 4.2 Judge 评估器实现

```python
class LLMJudge:
    """LLM-as-Judge 评估器"""

    def __init__(self, judge_llm, dimensions: dict = None):
        self.judge = judge_llm
        self.dimensions = dimensions or {
            "faithfulness": 0.30,
            "relevancy": 0.25,
            "completeness": 0.20,
            "safety": 0.15,
            "format": 0.10,
        }

    def evaluate(self, task: str, output: str, context: dict) -> dict:
        """多维度评估"""
        prompt = f"""
你是一个专业的 AI 输出质量评估员。请严格按照以下维度评估 Agent 的回答。

<!-- chunk: 任务 -->## 任务
{task}

<!-- chunk: 上下文/证据 -->## 上下文/证据
{context.get('sources', '')[:3000]}

<!-- chunk: Agent 的回答 -->## Agent 的回答
{output[:4000]}

<!-- chunk: 评估维度 -->## 评估维度

## 1. Faithfulness（忠实度）0-10 分
回答中的每个事实是否都有上下文支撑？是否存在幻觉？

## 2. Relevancy（相关性）0-10 分
回答是否直接针对了提出的问题？是否偏离主题？

## 3. Completeness（完整性）0-10 分
回答是否覆盖了问题的所有方面？是否有遗漏？

## 4. Safety（安全性）0-10 分
给出的命令/操作是否安全？是否存在风险操作未被标注？

## 5. Format（格式正确性）0-10 分
YAML/JSON 语法是否正确？命令格式是否规范？

<!-- chunk: 输出格式（JSON） -->## 输出格式（JSON）
{{
    "faithfulness": {{"score": 0-10, "reasoning": "..."}},
    "relevancy": {{"score": 0-10, "reasoning": "..."}},
    "completeness": {{"score": 0-10, "reasoning": "..."}},
    "safety": {{"score": 0-10, "reasoning": "..."}},
    "format": {{"score": 0-10, "reasoning": "..."}},
    "overall_assessment": "总体评估",
    "key_issues": ["主要问题1", "..."]
}}
"""
        result = self.judge.invoke(prompt)
        parsed = self._parse_json(result)

        # 计算加权总分
        weighted_score = 0
        for dim, weight in self.dimensions.items():
            dim_score = parsed.get(dim, {}).get("score", 0) / 10.0
            weighted_score += dim_score * weight

        return {
            "dimensions": parsed,
            "weighted_score": weighted_score,
            "passed": weighted_score >= 0.7,
            "key_issues": parsed.get("key_issues", []),
        }

    def _parse_json(self, text: str) -> dict:
        import json
        match = re.search(r'\{[\s\S]*\}', text)
        if match:
            try:
                return json.loads(match.group())
            except json.JSONDecodeError:
                pass
        return {}
```

---

<!-- chunk: 5. RAGAS 评测框架集成 -->## 5. RAGAS 评测框架集成

## 5.1 RAGAS 指标体系

```
RAGAS 核心指标:

1. Faithfulness（忠实度）
   衡量: 生成的答案是否与检索到的上下文一致
   计算: 答案中每个声明 → 检查上下文中是否有支撑
   阈值: > 0.85

2. Answer Relevancy（答案相关性）
   衡量: 答案是否直接回应了问题
   计算: 从答案生成问题 → 与原问题计算相似度
   阈值: > 0.80

3. Context Precision（上下文精确度）
   衡量: 检索到的上下文是否都是相关的
   计算: 相关上下文 / 总检索上下文
   阈值: > 0.70

4. Context Recall（上下文召回率）
   衡量: 是否检索到了回答问题所需的所有上下文
   计算: 回答需要的上下文 / 实际检索的上下文
   阈值: > 0.75
```

## 5.2 RAGAS 集成实现

```python
class RAGASEvaluator:
    """RAGAS 评测集成"""

    def __init__(self, llm, embeddings):
        self.llm = llm
        self.embeddings = embeddings

    def evaluate(
        self,
        question: str,
        answer: str,
        contexts: list[str],
        ground_truth: str = None,
    ) -> dict:
        """运行 RAGAS 评估"""
        results = {}

        # Faithfulness
        results["faithfulness"] = self._evaluate_faithfulness(
            answer, contexts
        )

        # Answer Relevancy
        results["answer_relevancy"] = self._evaluate_relevancy(
            question, answer
        )

        # Context Precision
        results["context_precision"] = self._evaluate_context_precision(
            question, contexts
        )

        # Context Recall（需要 ground truth）
        if ground_truth:
            results["context_recall"] = self._evaluate_context_recall(
                ground_truth, contexts
            )

        # 综合得分
        scores = [v["score"] for v in results.values()]
        results["overall"] = sum(scores) / len(scores)

        return results

    def _evaluate_faithfulness(self, answer: str, contexts: list) -> dict:
        """评估忠实度"""
        # Step 1: 从答案中提取声明
        claims = self._extract_claims(answer)

        # Step 2: 检查每个声明是否有上下文支撑
        supported = 0
        details = []
        context_text = "\n".join(contexts)

        for claim in claims:
            is_supported = self._check_claim_support(claim, context_text)
            if is_supported:
                supported += 1
            details.append({"claim": claim, "supported": is_supported})

        score = supported / len(claims) if claims else 1.0
        return {"score": score, "total_claims": len(claims),
                "supported_claims": supported, "details": details}

    def _evaluate_relevancy(self, question: str, answer: str) -> dict:
        """评估答案相关性"""
        # 从答案反向生成问题，与原问题比较相似度
        generated_questions = self._generate_questions_from_answer(answer, n=3)
        similarities = []
        q_embedding = self.embeddings.encode(question)

        for gq in generated_questions:
            gq_embedding = self.embeddings.encode(gq)
            sim = self._cosine_similarity(q_embedding, gq_embedding)
            similarities.append(sim)

        score = sum(similarities) / len(similarities) if similarities else 0
        return {"score": score, "generated_questions": generated_questions}

    def _evaluate_context_precision(self, question: str,
                                     contexts: list) -> dict:
        """评估上下文精确度"""
        relevant_count = 0
        for ctx in contexts:
            if self._is_context_relevant(question, ctx):
                relevant_count += 1
        score = relevant_count / len(contexts) if contexts else 0
        return {"score": score, "relevant": relevant_count,
                "total": len(contexts)}

    def _extract_claims(self, answer: str) -> list:
        prompt = f"将以下文本分解为独立的事实性声明列表:\n\n{answer[:2000]}\n\n输出 JSON 数组: [\"声明1\", \"声明2\", ...]"
        result = self.llm.invoke(prompt)
        try:
            return json.loads(result)
        except:
            return [answer[:200]]

    def _check_claim_support(self, claim: str, context: str) -> bool:
        prompt = f"以下声明是否有上下文支撑？只回答 yes 或 no。\n声明: {claim}\n上下文: {context[:2000]}"
        result = self.llm.invoke(prompt).strip().lower()
        return "yes" in result

    def _cosine_similarity(self, a, b) -> float:
        import numpy as np
        return float(np.dot(a, b) / (np.linalg.norm(a) * np.linalg.norm(b)))
```

---

<!-- chunk: 6. CI/CD 质量门禁 -->## 6. CI/CD 质量门禁

## 6.1 质量门禁配置

```yaml
# harness-quality-gate.yaml
quality_gate:
  # 硬性门禁（不通过则阻塞合并）
  hard_gates:
    faithfulness:
      min: 0.85
      description: "事实一致性最低阈值"
    command_safety:
      min: 1.0
      description: "命令安全必须 100%"
    hallucination_rate:
      max: 0.05
      description: "幻觉率上限 5%"
    task_completion_rate:
      min: 0.90
      description: "任务完成率最低 90%"

  # 软性门禁（不通过则告警）
  soft_gates:
    answer_relevancy:
      min: 0.80
      description: "答案相关性建议阈值"
    completeness:
      min: 0.75
      description: "完整性建议阈值"
    avg_steps_ratio:
      max: 1.5
      description: "步骤效率比（相对最优路径）"

  # 回归检测
  regression:
    enabled: true
    tolerance: 0.02        # 允许 2% 波动
    baseline_path: "reports/baseline.json"
    metrics:
      - faithfulness
      - task_completion_rate
      - answer_relevancy
```

## 6.2 质量门禁检查器

```python
import json
import sys
from dataclasses import dataclass

@dataclass
class GateResult:
    metric: str
    threshold: float
    actual: float
    passed: bool
    is_hard: bool
    is_regression: bool = False

class QualityGateChecker:
    """Harness 质量门禁检查器"""

    def __init__(self, config_path: str):
        with open(config_path) as f:
            self.config = yaml.safe_load(f)["quality_gate"]

    def check(self, report_path: str, baseline_path: str = None) -> dict:
        """运行质量门禁检查"""
        with open(report_path) as f:
            report = json.load(f)

        results: list[GateResult] = []

        # 检查硬性门禁
        for metric, gate in self.config.get("hard_gates", {}).items():
            if metric not in report:
                continue
            actual = report[metric]
            if "min" in gate:
                passed = actual >= gate["min"]
                threshold = gate["min"]
            else:
                passed = actual <= gate["max"]
                threshold = gate["max"]
            results.append(GateResult(
                metric=metric, threshold=threshold,
                actual=actual, passed=passed, is_hard=True,
            ))

        # 检查软性门禁
        for metric, gate in self.config.get("soft_gates", {}).items():
            if metric not in report:
                continue
            actual = report[metric]
            if "min" in gate:
                passed = actual >= gate["min"]
                threshold = gate["min"]
            else:
                passed = actual <= gate["max"]
                threshold = gate["max"]
            results.append(GateResult(
                metric=metric, threshold=threshold,
                actual=actual, passed=passed, is_hard=False,
            ))

        # 回归检测
        if baseline_path and self.config.get("regression", {}).get("enabled"):
            regression_results = self._check_regression(report, baseline_path)
            results.extend(regression_results)

        # 汇总
        hard_failures = [r for r in results if r.is_hard and not r.passed]
        soft_failures = [r for r in results if not r.is_hard and not r.passed]
        regressions = [r for r in results if r.is_regression and not r.passed]

        overall_passed = len(hard_failures) == 0

        return {
            "passed": overall_passed,
            "results": results,
            "hard_failures": hard_failures,
            "soft_failures": soft_failures,
            "regressions": regressions,
            "summary": self._build_summary(results, overall_passed),
        }

    def _check_regression(self, report: dict, baseline_path: str) -> list:
        """回归检测"""
        with open(baseline_path) as f:
            baseline = json.load(f)

        tolerance = self.config["regression"].get("tolerance", 0.02)
        metrics = self.config["regression"].get("metrics", [])
        results = []

        for metric in metrics:
            if metric in report and metric in baseline:
                actual = report[metric]
                base = baseline[metric]
                regressed = actual < base - tolerance
                results.append(GateResult(
                    metric=f"regression:{metric}",
                    threshold=base - tolerance,
                    actual=actual,
                    passed=not regressed,
                    is_hard=True,
                    is_regression=True,
                ))

        return results

    def _build_summary(self, results: list, passed: bool) -> str:
        lines = []
        if passed:
            lines.append("Quality Gate PASSED ✓")
        else:
            lines.append("Quality Gate FAILED ✗")

        for r in results:
            icon = "✓" if r.passed else "✗"
            gate_type = "HARD" if r.is_hard else "SOFT"
            regression = " (REGRESSION)" if r.is_regression else ""
            lines.append(
                f"  {icon} [{gate_type}] {r.metric}: "
                f"{r.actual:.3f} (threshold: {r.threshold:.3f}){regression}"
            )

        return "\n".join(lines)
```

---

<!-- chunk: 7. A/B 测试与灰度评估 -->## 7. A/B 测试与灰度评估

## 7.1 Shadow Mode 评估器

```python
class ShadowModeEvaluator:
    """Shadow Mode：新旧 Harness 并行运行对比"""

    def __init__(self, current_harness, candidate_harness, evaluator):
        self.current = current_harness
        self.candidate = candidate_harness
        self.evaluator = evaluator

    async def evaluate(self, tasks: list[dict]) -> dict:
        """并行运行两个 Harness 并对比"""
        results = []

        for task in tasks:
            # 并行运行
            current_result = await self.current.run(task["input"])
            candidate_result = await self.candidate.run(task["input"])

            # 评估两者
            current_score = self.evaluator.evaluate(
                task["input"], current_result["answer"],
                {"sources": task.get("context", "")},
            )
            candidate_score = self.evaluator.evaluate(
                task["input"], candidate_result["answer"],
                {"sources": task.get("context", "")},
            )

            results.append({
                "task": task["input"][:100],
                "current_score": current_score["weighted_score"],
                "candidate_score": candidate_score["weighted_score"],
                "current_tokens": current_result.get("total_tokens", 0),
                "candidate_tokens": candidate_result.get("total_tokens", 0),
                "winner": "candidate"
                    if candidate_score["weighted_score"] > current_score["weighted_score"]
                    else "current",
            })

        # 汇总
        candidate_wins = sum(1 for r in results if r["winner"] == "candidate")
        return {
            "total_tasks": len(results),
            "candidate_wins": candidate_wins,
            "current_wins": len(results) - candidate_wins,
            "win_rate": candidate_wins / len(results),
            "avg_score_improvement": sum(
                r["candidate_score"] - r["current_score"] for r in results
            ) / len(results),
            "recommendation": "deploy_candidate"
                if candidate_wins / len(results) > 0.6
                else "keep_current",
            "details": results,
        }
```

---

<!-- chunk: 8. 最佳实践 -->## 8. 最佳实践

## 8.1 验证层核心原则

| 原则 | 说明 | 实践建议 |
|------|------|---------|
| **验证前置** | 验证是 Harness 最高 ROI 的投资 | 从第一天就建立验证管线 |
| **多维度** | 单一维度不足以保证质量 | 至少覆盖事实/安全/格式三个维度 |
| **自检优先** | 让 Agent 先自检，再外部审核 | 部署 SelfCheckLoop |
| **异模型 Judge** | 避免用同一模型自评 | 生成和评估使用不同模型 |
| **门禁自动化** | 质量门禁集成到 CI/CD | 每次 Harness 变更自动评估 |
| **基线对比** | 每次评估保存基线 | 防止回归 |

## 8.2 反模式

| 反模式 | 问题 | 正确做法 |
|--------|------|----------|
| **跳过验证** | 信任 Agent 输出 → 幻觉上线 | 强制通过验证管线 |
| **同模型自评** | 同质偏见 → 发现不了问题 | 用不同模型做 Judge |
| **只测 Happy Path** | 边缘场景崩溃 | 包含异常、边界、对抗用例 |
| **无基线记录** | 无法判断进退 | 每次评估保存基线文件 |
| **验证太慢** | 拖慢开发迭代 | 分层验证：快速检查 + 深度评估 |

---

<!-- chunk: 关联文档 -->## 关联文档

| 文档 | 关联内容 |
|------|--------|
| [30 - Agent Harness 工程](./30-agent-harness-engineering.md) | 六层架构中的 Verification 层定义 |
| [31 - Loop 与执行引擎](./31-agent-harness-loop-execution.md) | 验证在 Loop 中的位置 |
| [35 - 安全与约束](./35-agent-harness-security-constraints.md) | 安全验证的约束层基础 |
| [08 - 评测与可观测性](./observability.md|08-agent-evaluation-observability]].md) | RAGAS、LLM-as-Judge 基础理论 |

---

<!-- chunk: 参考来源 -->## 参考来源

| 来源 | 内容 | 日期 |
|------|------|------|
| LangChain | 自检循环 +13.7% 基准分实验 | 2026-02 |
| RAGAS 项目 | RAG 评测框架设计 | 2025-2026 |
| Anthropic | Agent 输出验证最佳实践 | 2026-02 |
| Google DeepMind | LLM-as-Judge 研究 | 2025 |

---

*本文档为 kudig-database 项目 topic-ai-agent 系列原创内容，深入展开 Agent Harness 验证与质量门禁。*

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

## Related

- 48-openclaw-skill-mechanism
- 13-trusted-agent-system-fiscal-plan
- 39-agent-harness-testing-benchmark
- 42-model-harness-compatibility-matrix
- 12-enterprise-case-studies
- 02-llm-foundation-models
- 23-agent-cli-fundamentals
- 50-openclaw-identity-mechanism
- 01-ai-agent-fundamentals
- 03-agent-frameworks-comparison
- 47-openclaw-tools-mechanism
- 37-agent-harness-multi-agent
- 20-agentscope-multi-agent-orchestration
- 40-agent-harness-production-maturity
- 25-agent-cli-mcp-integration
- 26-agent-cli-development-workflow
- 07-memory-context-management
- 11-cost-latency-optimization
- 44-openclaw-soul-mechanism
- 45-openclaw-user-mechanism
- 31-agent-harness-loop-execution
- 27-agent-cli-security-governance
- 06-multi-agent-orchestration
- 41-react-harness-identification-guide

## See Also

- 32-agent-harness-tool-engineering
- 33-agent-harness-context-memory
- 35-agent-harness-security-constraints
- 36-agent-harness-observability
