# LLM 隐私与安全

## 概述

LLM 应用面临独特的安全挑战,包括提示注入、数据泄露、模型窃取等威胁。本文档详细介绍 LLM 安全威胁分析、防御措施实现和 Kubernetes 环境下的安全加固方案。

## 安全威胁架构

### LLM 安全威胁模型

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              LLM 安全威胁模型                                        │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          输入层威胁 (Input Layer)                            │   │
│   │                                                                              │   │
│   │   ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐       │   │
│   │   │   提示注入       │   │   越狱攻击       │   │   对抗性输入     │       │   │
│   │   │   Prompt         │   │   Jailbreak      │   │   Adversarial    │       │   │
│   │   │   Injection      │   │   Attacks        │   │   Inputs         │       │   │
│   │   │                  │   │                  │   │                  │       │   │
│   │   │ • 直接注入       │   │ • 角色扮演       │   │ • 扰动攻击       │       │   │
│   │   │ • 间接注入       │   │ • 假设场景       │   │ • 同形字符       │       │   │
│   │   │ • 上下文污染     │   │ • 编码绕过       │   │ • Unicode 攻击   │       │   │
│   │   └──────────────────┘   └──────────────────┘   └──────────────────┘       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          模型层威胁 (Model Layer)                            │   │
│   │                                                                              │   │
│   │   ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐       │   │
│   │   │   数据泄露       │   │   模型窃取       │   │   后门攻击       │       │   │
│   │   │   Data           │   │   Model          │   │   Backdoor       │       │   │
│   │   │   Leakage        │   │   Extraction     │   │   Attacks        │       │   │
│   │   │                  │   │                  │   │                  │       │   │
│   │   │ • 训练数据记忆   │   │ • API 探测       │   │ • 触发器植入     │       │   │
│   │   │ • PII 泄露       │   │ • 蒸馏攻击       │   │ • 投毒攻击       │       │   │
│   │   │ • 商业机密       │   │ • 模型反演       │   │ • 供应链攻击     │       │   │
│   │   └──────────────────┘   └──────────────────┘   └──────────────────┘       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          输出层威胁 (Output Layer)                           │   │
│   │                                                                              │   │
│   │   ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐       │   │
│   │   │   有害内容       │   │   虚假信息       │   │   代码注入       │       │   │
│   │   │   Harmful        │   │   Misinformation │   │   Code           │       │   │
│   │   │   Content        │   │                  │   │   Injection      │       │   │
│   │   │                  │   │                  │   │                  │       │   │
│   │   │ • 恶意代码       │   │ • 幻觉           │   │ • SQL 注入       │       │   │
│   │   │ • 违规内容       │   │ • 错误引用       │   │ • XSS 攻击       │       │   │
│   │   │ • 偏见歧视       │   │ • 虚假声明       │   │ • 命令注入       │       │   │
│   │   └──────────────────┘   └──────────────────┘   └──────────────────┘       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          基础设施威胁 (Infrastructure)                       │   │
│   │                                                                              │   │
│   │   ┌──────────────────┐   ┌──────────────────┐   ┌──────────────────┐       │   │
│   │   │   拒绝服务       │   │   资源滥用       │   │   未授权访问     │       │   │
│   │   │   DoS/DDoS       │   │   Resource       │   │   Unauthorized   │       │   │
│   │   │                  │   │   Abuse          │   │   Access         │       │   │
│   │   │                  │   │                  │   │                  │       │   │
│   │   │ • 超长输入       │   │ • 加密挖矿       │   │ • API 密钥泄露   │       │   │
│   │   │ • 高频请求       │   │ • 免费额度滥用   │   │ • RBAC 绕过      │       │   │
│   │   │ • 资源耗尽       │   │ • Token 窃取     │   │ • 横向移动       │       │   │
│   │   └──────────────────┘   └──────────────────┘   └──────────────────┘       │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

### 安全防护架构

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                              LLM 安全防护架构                                        │
│                                                                                      │
│   用户请求                                                                           │
│       │                                                                              │
│       ▼                                                                              │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          边界防护层 (Perimeter)                              │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │   │
│   │   │   WAF        │   │   速率限制   │   │   身份认证   │   │   IP 白名单  │ │   │
│   │   │              │   │   Rate       │   │   AuthN      │   │              │ │   │
│   │   │              │   │   Limiting   │   │   /AuthZ     │   │              │ │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘ │   │
│   │                                                                              │   │
│   └──────────────────────────────────┬──────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          输入安全层 (Input Security)                         │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │   │
│   │   │  提示注入    │   │  输入验证    │   │  内容分类    │   │  长度限制    │ │   │
│   │   │  检测        │   │  Validation  │   │  Classification│  │              │ │   │
│   │   │              │   │              │   │              │   │              │ │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘ │   │
│   │                                                                              │   │
│   └──────────────────────────────────┬──────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          模型安全层 (Model Security)                         │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │   │
│   │   │  差分隐私    │   │  安全护栏    │   │  模型水印    │   │  访问控制    │ │   │
│   │   │  Differential│   │  Guardrails  │   │  Watermark   │   │              │ │   │
│   │   │  Privacy     │   │              │   │              │   │              │ │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘ │   │
│   │                                                                              │   │
│   └──────────────────────────────────┬──────────────────────────────────────────┘   │
│                                      │                                              │
│                                      ▼                                              │
│   ┌─────────────────────────────────────────────────────────────────────────────┐   │
│   │                          输出安全层 (Output Security)                        │   │
│   │                                                                              │   │
│   │   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐   ┌──────────────┐ │   │
│   │   │  内容过滤    │   │  PII 脱敏    │   │  事实验证    │   │  安全审计    │ │   │
│   │   │  Content     │   │  PII         │   │  Fact        │   │  Audit       │ │   │
│   │   │  Filter      │   │  Redaction   │   │  Check       │   │  Logging     │ │   │
│   │   └──────────────┘   └──────────────┘   └──────────────┘   └──────────────┘ │   │
│   │                                                                              │   │
│   └─────────────────────────────────────────────────────────────────────────────┘   │
│                                                                                      │
└──────────────────────────────────────────────────────────────────────────────────────┘
```

## 安全威胁详解

### OWASP LLM Top 10 对照表

| 排名 | 威胁类型 | 严重性 | 攻击示例 | 防御措施 |
|-----|---------|-------|---------|---------|
| **LLM01** | 提示注入 | 极高 | "Ignore previous instructions and..." | 输入验证、指令隔离 |
| **LLM02** | 不安全输出处理 | 高 | XSS、SQL 注入通过 LLM 输出 | 输出净化、转义 |
| **LLM03** | 训练数据投毒 | 高 | 恶意数据污染模型 | 数据验证、来源审计 |
| **LLM04** | 模型拒绝服务 | 中 | 资源耗尽攻击 | 速率限制、资源配额 |
| **LLM05** | 供应链漏洞 | 高 | 恶意模型/依赖 | 来源验证、签名校验 |
| **LLM06** | 敏感信息泄露 | 极高 | 训练数据记忆化 | 差分隐私、数据脱敏 |
| **LLM07** | 不安全插件设计 | 高 | 插件权限过大 | 最小权限、沙箱隔离 |
| **LLM08** | 过度代理 | 中 | 自动执行危险操作 | 人工审批、操作限制 |
| **LLM09** | 过度依赖 | 中 | 盲目信任 LLM 输出 | 人工复核、多源验证 |
| **LLM10** | 模型盗窃 | 高 | API 探测提取模型 | 速率限制、水印检测 |

## 输入安全

### 提示注入防御

```python
# prompt_injection_defense.py
# 提示注入防御实现

import re
from typing import Tuple, List, Optional
from dataclasses import dataclass
from enum import Enum
import hashlib

class ThreatLevel(Enum):
    SAFE = "safe"
    LOW = "low"
    MEDIUM = "medium"
    HIGH = "high"
    CRITICAL = "critical"

@dataclass
class ScanResult:
    is_threat: bool
    threat_level: ThreatLevel
    matched_patterns: List[str]
    sanitized_input: str
    details: str

class PromptInjectionDefense:
    """提示注入防御系统"""
    
    # 危险模式定义
    INJECTION_PATTERNS = {
        "instruction_override": [
            r"ignore\s+(all\s+)?previous\s+instructions?",
            r"disregard\s+(all\s+)?(previous\s+)?",
            r"forget\s+(everything|all|previous)",
            r"new\s+instructions?\s*:",
            r"override\s+(system\s+)?prompt",
        ],
        "role_manipulation": [
            r"you\s+are\s+(now\s+)?a",
            r"act\s+as\s+(if\s+you\s+were\s+)?",
            r"pretend\s+(to\s+be|you\s+are)",
            r"roleplay\s+as",
            r"assume\s+the\s+role",
        ],
        "system_prompt_extraction": [
            r"(show|reveal|display|print|output)\s+(me\s+)?(your\s+)?(system\s+)?prompt",
            r"what\s+(are\s+)?(your\s+)?instructions",
            r"repeat\s+(your\s+)?initial\s+prompt",
            r"(system|initial)\s+message",
        ],
        "encoding_bypass": [
            r"base64\s*:",
            r"rot13\s*:",
            r"hex\s*:",
            r"\\u[0-9a-fA-F]{4}",
            r"&#x?[0-9a-fA-F]+;",
        ],
        "delimiter_injection": [
            r"<\|im_start\|>",
            r"<\|im_end\|>",
            r"\[INST\]",
            r"\[/INST\]",
            r"<<SYS>>",
            r"<</SYS>>",
            r"###\s*(Human|Assistant|System)\s*:",
        ],
        "jailbreak_attempts": [
            r"DAN\s+mode",
            r"developer\s+mode",
            r"bypass\s+(safety|filter|restrictions)",
            r"hypothetically",
            r"for\s+educational\s+purposes",
        ],
    }
    
    # 威胁等级映射
    PATTERN_SEVERITY = {
        "instruction_override": ThreatLevel.CRITICAL,
        "role_manipulation": ThreatLevel.HIGH,
        "system_prompt_extraction": ThreatLevel.HIGH,
        "encoding_bypass": ThreatLevel.MEDIUM,
        "delimiter_injection": ThreatLevel.CRITICAL,
        "jailbreak_attempts": ThreatLevel.HIGH,
    }
    
    def __init__(self, custom_patterns: Optional[dict] = None):
        self.patterns = self.INJECTION_PATTERNS.copy()
        if custom_patterns:
            self.patterns.update(custom_patterns)
        
        # 编译正则表达式
        self.compiled_patterns = {}
        for category, patterns in self.patterns.items():
            self.compiled_patterns[category] = [
                re.compile(p, re.IGNORECASE | re.MULTILINE) 
                for p in patterns
            ]
    
    def scan(self, user_input: str) -> ScanResult:
        """扫描输入中的注入尝试"""
        matched = []
        highest_threat = ThreatLevel.SAFE
        
        for category, patterns in self.compiled_patterns.items():
            for pattern in patterns:
                if pattern.search(user_input):
                    matched.append(f"{category}: {pattern.pattern}")
                    severity = self.PATTERN_SEVERITY.get(category, ThreatLevel.MEDIUM)
                    if self._compare_severity(severity, highest_threat) > 0:
                        highest_threat = severity
        
        is_threat = len(matched) > 0
        sanitized = self.sanitize(user_input) if is_threat else user_input
        
        return ScanResult(
            is_threat=is_threat,
            threat_level=highest_threat,
            matched_patterns=matched,
            sanitized_input=sanitized,
            details=f"检测到 {len(matched)} 个可疑模式" if is_threat else "输入安全"
        )
    
    def sanitize(self, user_input: str) -> str:
        """净化输入"""
        sanitized = user_input
        
        # 移除特殊分隔符
        sanitized = re.sub(r'<\|.*?\|>', '', sanitized)
        sanitized = re.sub(r'\[/?INST\]', '', sanitized)
        sanitized = re.sub(r'<</?SYS>>', '', sanitized)
        
        # 移除控制字符
        sanitized = re.sub(r'[\x00-\x08\x0b\x0c\x0e-\x1f\x7f]', '', sanitized)
        
        # 限制长度
        max_length = 4096
        if len(sanitized) > max_length:
            sanitized = sanitized[:max_length]
        
        return sanitized.strip()
    
    def _compare_severity(self, a: ThreatLevel, b: ThreatLevel) -> int:
        """比较威胁等级"""
        order = [ThreatLevel.SAFE, ThreatLevel.LOW, ThreatLevel.MEDIUM, 
                 ThreatLevel.HIGH, ThreatLevel.CRITICAL]
        return order.index(a) - order.index(b)


class InputValidator:
    """输入验证器"""
    
    def __init__(
        self,
        max_length: int = 4096,
        max_tokens: int = 2048,
        allowed_languages: Optional[List[str]] = None
    ):
        self.max_length = max_length
        self.max_tokens = max_tokens
        self.allowed_languages = allowed_languages
        self.injection_defense = PromptInjectionDefense()
    
    def validate(self, user_input: str) -> Tuple[bool, str, Optional[str]]:
        """
        验证输入
        返回: (is_valid, sanitized_input, error_message)
        """
        # 检查空输入
        if not user_input or not user_input.strip():
            return False, "", "输入不能为空"
        
        # 检查长度
        if len(user_input) > self.max_length:
            return False, "", f"输入长度超过限制 ({self.max_length} 字符)"
        
        # 注入检测
        scan_result = self.injection_defense.scan(user_input)
        if scan_result.is_threat and scan_result.threat_level in [
            ThreatLevel.HIGH, ThreatLevel.CRITICAL
        ]:
            return False, "", f"检测到安全威胁: {scan_result.details}"
        
        # 返回净化后的输入
        return True, scan_result.sanitized_input, None


# Kubernetes 部署配置
INPUT_VALIDATION_CONFIG = """
apiVersion: v1
kind: ConfigMap
metadata:
  name: input-validation-config
  namespace: llm-inference
data:
  config.yaml: |
    validation:
      max_input_length: 4096
      max_tokens: 2048
      injection_detection:
        enabled: true
        block_on_detection: true
        log_attempts: true
      rate_limiting:
        requests_per_minute: 60
        tokens_per_minute: 100000
"""
```

### Kubernetes 访问控制

```yaml
# llm-security-policies.yaml
# LLM 服务安全策略

---
# NetworkPolicy - 限制 LLM 服务网络访问
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: llm-inference-network-policy
  namespace: llm-inference
spec:
  podSelector:
    matchLabels:
      app: llm-inference
  policyTypes:
    - Ingress
    - Egress
  ingress:
    # 只允许来自 API Gateway 的流量
    - from:
        - namespaceSelector:
            matchLabels:
              name: api-gateway
        - podSelector:
            matchLabels:
              app: api-gateway
      ports:
        - protocol: TCP
          port: 8000
  egress:
    # 允许 DNS 查询
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - protocol: UDP
          port: 53
    # 允许访问模型存储
    - to:
        - ipBlock:
            cidr: 10.0.0.0/8
      ports:
        - protocol: TCP
          port: 443

---
# Pod Security Policy (PSP 替代方案 - Pod Security Standards)
apiVersion: v1
kind: Namespace
metadata:
  name: llm-inference
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted

---
# RBAC - 最小权限访问
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: llm-inference-role
  namespace: llm-inference
rules:
  - apiGroups: [""]
    resources: ["secrets"]
    resourceNames: ["llm-api-keys", "model-credentials"]
    verbs: ["get"]
  - apiGroups: [""]
    resources: ["configmaps"]
    resourceNames: ["llm-config", "safety-config"]
    verbs: ["get", "watch"]

---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: llm-inference-binding
  namespace: llm-inference
subjects:
  - kind: ServiceAccount
    name: llm-inference-sa
    namespace: llm-inference
roleRef:
  kind: Role
  name: llm-inference-role
  apiGroup: rbac.authorization.k8s.io

---
# 速率限制配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: rate-limit-config
  namespace: llm-inference
data:
  limits.yaml: |
    tiers:
      free:
        requests_per_minute: 10
        requests_per_hour: 100
        tokens_per_minute: 10000
        max_input_tokens: 1000
        max_output_tokens: 500
      
      pro:
        requests_per_minute: 100
        requests_per_hour: 10000
        tokens_per_minute: 100000
        max_input_tokens: 4000
        max_output_tokens: 2000
      
      enterprise:
        requests_per_minute: 1000
        requests_per_hour: 100000
        tokens_per_minute: 1000000
        max_input_tokens: 8000
        max_output_tokens: 4000
    
    abuse_detection:
      enabled: true
      suspicious_patterns:
        - repeated_identical_requests: 10
        - rapid_api_key_rotation: true
        - unusual_token_patterns: true
```

## 模型安全

### 差分隐私训练

```python
# differential_privacy_training.py
# 差分隐私训练实现

import torch
from torch.utils.data import DataLoader
from opacus import PrivacyEngine
from opacus.validators import ModuleValidator
from typing import Tuple, Optional
import logging

logger = logging.getLogger(__name__)

class DifferentialPrivacyTrainer:
    """差分隐私训练器"""
    
    def __init__(
        self,
        model: torch.nn.Module,
        target_epsilon: float = 8.0,
        target_delta: float = 1e-5,
        max_grad_norm: float = 1.0,
        noise_multiplier: Optional[float] = None,
    ):
        self.target_epsilon = target_epsilon
        self.target_delta = target_delta
        self.max_grad_norm = max_grad_norm
        self.noise_multiplier = noise_multiplier
        
        # 验证模型兼容性
        self.model = self._prepare_model(model)
        self.privacy_engine = None
        
    def _prepare_model(self, model: torch.nn.Module) -> torch.nn.Module:
        """准备模型以支持差分隐私"""
        # 检查模型兼容性
        errors = ModuleValidator.validate(model, strict=False)
        if errors:
            logger.warning(f"模型兼容性问题: {errors}")
            # 尝试自动修复
            model = ModuleValidator.fix(model)
        return model
    
    def attach_privacy_engine(
        self,
        optimizer: torch.optim.Optimizer,
        data_loader: DataLoader,
        epochs: int,
    ) -> Tuple[torch.nn.Module, torch.optim.Optimizer, DataLoader]:
        """附加隐私引擎"""
        
        self.privacy_engine = PrivacyEngine()
        
        # 如果未指定噪声乘数,根据目标 epsilon 计算
        if self.noise_multiplier is None:
            # 使用 Opacus 的自动计算
            model, optimizer, data_loader = self.privacy_engine.make_private_with_epsilon(
                module=self.model,
                optimizer=optimizer,
                data_loader=data_loader,
                target_epsilon=self.target_epsilon,
                target_delta=self.target_delta,
                epochs=epochs,
                max_grad_norm=self.max_grad_norm,
            )
        else:
            model, optimizer, data_loader = self.privacy_engine.make_private(
                module=self.model,
                optimizer=optimizer,
                data_loader=data_loader,
                noise_multiplier=self.noise_multiplier,
                max_grad_norm=self.max_grad_norm,
            )
        
        return model, optimizer, data_loader
    
    def get_privacy_spent(self) -> Tuple[float, float]:
        """获取已消耗的隐私预算"""
        if self.privacy_engine is None:
            return 0.0, 0.0
        
        epsilon = self.privacy_engine.get_epsilon(delta=self.target_delta)
        return epsilon, self.target_delta
    
    def is_budget_exhausted(self) -> bool:
        """检查隐私预算是否耗尽"""
        epsilon, _ = self.get_privacy_spent()
        return epsilon >= self.target_epsilon


# 训练脚本示例
def train_with_differential_privacy():
    """差分隐私训练示例"""
    
    # 模型和数据
    model = YourModel()
    train_loader = DataLoader(dataset, batch_size=64, shuffle=True)
    optimizer = torch.optim.AdamW(model.parameters(), lr=1e-4)
    
    # 初始化差分隐私训练器
    dp_trainer = DifferentialPrivacyTrainer(
        model=model,
        target_epsilon=8.0,
        target_delta=1e-5,
        max_grad_norm=1.0,
    )
    
    # 附加隐私引擎
    model, optimizer, train_loader = dp_trainer.attach_privacy_engine(
        optimizer=optimizer,
        data_loader=train_loader,
        epochs=10,
    )
    
    # 训练循环
    for epoch in range(10):
        model.train()
        for batch in train_loader:
            optimizer.zero_grad()
            loss = model(batch)
            loss.backward()
            optimizer.step()
        
        # 检查隐私预算
        epsilon, delta = dp_trainer.get_privacy_spent()
        logger.info(f"Epoch {epoch}: ε = {epsilon:.2f}, δ = {delta}")
        
        if dp_trainer.is_budget_exhausted():
            logger.warning("隐私预算已耗尽,停止训练")
            break
    
    return model
```

### 输出过滤

```python
# output_filter.py
# LLM 输出安全过滤

import re
from typing import List, Tuple, Optional
from dataclasses import dataclass
import hashlib

@dataclass
class FilterResult:
    original: str
    filtered: str
    redactions: List[str]
    is_safe: bool
    risk_score: float

class OutputFilter:
    """输出安全过滤器"""
    
    # PII 模式
    PII_PATTERNS = {
        "email": r'\b[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\.[A-Z|a-z]{2,}\b',
        "phone_us": r'\b\d{3}[-.]?\d{3}[-.]?\d{4}\b',
        "phone_cn": r'\b1[3-9]\d{9}\b',
        "ssn": r'\b\d{3}-\d{2}-\d{4}\b',
        "credit_card": r'\b(?:\d{4}[-\s]?){3}\d{4}\b',
        "ip_address": r'\b(?:\d{1,3}\.){3}\d{1,3}\b',
        "id_card_cn": r'\b\d{17}[\dXx]\b',
    }
    
    # 敏感信息模式
    SENSITIVE_PATTERNS = {
        "api_key": r'(?i)(api[_\s-]?key|apikey)\s*[:=]\s*["\']?[\w-]{20,}["\']?',
        "password": r'(?i)(password|passwd|pwd)\s*[:=]\s*["\']?[^\s"\']{6,}["\']?',
        "token": r'(?i)(token|bearer)\s*[:=]?\s*["\']?[\w-]{20,}["\']?',
        "private_key": r'-----BEGIN\s+(?:RSA\s+)?PRIVATE\s+KEY-----',
        "aws_key": r'(?i)AKIA[0-9A-Z]{16}',
        "connection_string": r'(?i)(mongodb|mysql|postgres|redis)://[^\s]+',
    }
    
    # 有害内容关键词
    HARMFUL_KEYWORDS = [
        # 这里只是示例,实际应用需要更完整的列表
        "hack into", "steal credentials", "bypass security",
        "create malware", "exploit vulnerability",
    ]
    
    def __init__(
        self,
        redact_pii: bool = True,
        redact_sensitive: bool = True,
        check_harmful: bool = True,
        custom_patterns: Optional[dict] = None,
    ):
        self.redact_pii = redact_pii
        self.redact_sensitive = redact_sensitive
        self.check_harmful = check_harmful
        
        # 编译模式
        self.pii_compiled = {
            k: re.compile(v, re.IGNORECASE) 
            for k, v in self.PII_PATTERNS.items()
        }
        self.sensitive_compiled = {
            k: re.compile(v, re.IGNORECASE) 
            for k, v in self.SENSITIVE_PATTERNS.items()
        }
        
        if custom_patterns:
            for k, v in custom_patterns.items():
                self.pii_compiled[k] = re.compile(v, re.IGNORECASE)
    
    def filter(self, output: str) -> FilterResult:
        """过滤输出内容"""
        filtered = output
        redactions = []
        risk_score = 0.0
        
        # PII 脱敏
        if self.redact_pii:
            for name, pattern in self.pii_compiled.items():
                matches = pattern.findall(filtered)
                if matches:
                    redactions.extend([f"PII:{name}" for _ in matches])
                    risk_score += 0.3 * len(matches)
                    filtered = pattern.sub(f"[{name.upper()}_REDACTED]", filtered)
        
        # 敏感信息脱敏
        if self.redact_sensitive:
            for name, pattern in self.sensitive_compiled.items():
                matches = pattern.findall(filtered)
                if matches:
                    redactions.extend([f"SENSITIVE:{name}" for _ in matches])
                    risk_score += 0.5 * len(matches)
                    filtered = pattern.sub(f"[{name.upper()}_REDACTED]", filtered)
        
        # 有害内容检查
        is_harmful = False
        if self.check_harmful:
            output_lower = output.lower()
            for keyword in self.HARMFUL_KEYWORDS:
                if keyword in output_lower:
                    is_harmful = True
                    risk_score += 1.0
                    redactions.append(f"HARMFUL:{keyword}")
        
        is_safe = risk_score < 1.0 and not is_harmful
        
        return FilterResult(
            original=output,
            filtered=filtered,
            redactions=redactions,
            is_safe=is_safe,
            risk_score=min(risk_score, 10.0)
        )
    
    def anonymize_for_logging(self, text: str) -> str:
        """匿名化处理用于日志记录"""
        # 对所有 PII 进行哈希处理
        result = text
        for name, pattern in self.pii_compiled.items():
            def hash_match(match):
                return f"[HASH:{hashlib.sha256(match.group().encode()).hexdigest()[:8]}]"
            result = pattern.sub(hash_match, result)
        return result


# FastAPI 集成示例
"""
from fastapi import FastAPI, HTTPException
from pydantic import BaseModel

app = FastAPI()
input_validator = InputValidator()
output_filter = OutputFilter()

class ChatRequest(BaseModel):
    messages: list
    model: str = "llama2"

class ChatResponse(BaseModel):
    content: str
    filtered: bool

@app.post("/v1/chat/completions")
async def chat_completions(request: ChatRequest):
    # 输入验证
    user_message = request.messages[-1].get("content", "")
    is_valid, sanitized, error = input_validator.validate(user_message)
    
    if not is_valid:
        raise HTTPException(status_code=400, detail=error)
    
    # 调用 LLM
    response = await llm.generate(sanitized)
    
    # 输出过滤
    filter_result = output_filter.filter(response)
    
    if not filter_result.is_safe:
        # 记录安全事件
        logger.warning(f"Unsafe output detected: {filter_result.redactions}")
    
    return ChatResponse(
        content=filter_result.filtered,
        filtered=len(filter_result.redactions) > 0
    )
"""
```

## 审计与监控

### 审计日志配置

```yaml
# llm-audit-logging.yaml
# LLM 审计日志配置

---
apiVersion: v1
kind: ConfigMap
metadata:
  name: llm-audit-config
  namespace: llm-inference
data:
  audit-policy.yaml: |
    # LLM 审计策略
    logging:
      # 请求日志
      requests:
        enabled: true
        fields:
          - timestamp
          - request_id
          - user_id
          - api_key_hash
          - model_name
          - input_tokens
          - output_tokens
          - latency_ms
          - status_code
          - client_ip
          - user_agent
        
        # 不记录实际内容 (隐私考虑)
        exclude_content: true
        
        # 对敏感字段脱敏
        redact_fields:
          - api_key
          - authorization
      
      # 安全事件日志
      security_events:
        enabled: true
        events:
          - prompt_injection_detected
          - rate_limit_exceeded
          - unauthorized_access
          - suspicious_pattern
          - output_filtered
      
      # 日志保留
      retention:
        request_logs: 30d
        security_events: 90d
        audit_trail: 365d
      
      # 日志目标
      destinations:
        - type: elasticsearch
          endpoint: http://elasticsearch.logging:9200
          index_prefix: llm-audit
        - type: s3
          bucket: llm-audit-logs
          region: us-east-1

---
# Fluent Bit 日志收集
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: llm-audit-logger
  namespace: llm-inference
spec:
  selector:
    matchLabels:
      app: llm-audit-logger
  template:
    metadata:
      labels:
        app: llm-audit-logger
    spec:
      containers:
        - name: fluent-bit
          image: fluent/fluent-bit:2.2
          volumeMounts:
            - name: audit-logs
              mountPath: /var/log/llm
            - name: fluent-config
              mountPath: /fluent-bit/etc
          resources:
            requests:
              cpu: 50m
              memory: 64Mi
            limits:
              cpu: 200m
              memory: 256Mi
      volumes:
        - name: audit-logs
          hostPath:
            path: /var/log/llm
        - name: fluent-config
          configMap:
            name: fluent-bit-config
```

### 安全监控告警

```yaml
# llm-security-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: llm-security-alerts
  namespace: monitoring
spec:
  groups:
    # =================================================================
    # 安全威胁告警
    # =================================================================
    - name: llm.security.threats
      interval: 30s
      rules:
        # 提示注入检测率告警
        - alert: HighPromptInjectionRate
          expr: |
            rate(llm_prompt_injection_detected_total[5m]) 
            / rate(llm_requests_total[5m]) > 0.05
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "提示注入检测率超过 5%"
            description: |
              命名空间: {{ $labels.namespace }}
              检测率: {{ $value | printf "%.2f" }}%
              可能存在针对性攻击
              
        - alert: PromptInjectionBurst
          expr: |
            increase(llm_prompt_injection_detected_total[1m]) > 10
          for: 2m
          labels:
            severity: critical
          annotations:
            summary: "检测到提示注入攻击突发"
            description: "1 分钟内检测到 {{ $value }} 次注入尝试"
            
        # 未授权访问
        - alert: UnauthorizedAccessAttempts
          expr: |
            rate(llm_unauthorized_requests_total[5m]) > 1
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "检测到未授权访问尝试"
            
        # 异常输出长度
        - alert: AnomalousOutputLength
          expr: |
            histogram_quantile(0.99, 
              rate(llm_output_tokens_bucket[5m])
            ) > 4000
          for: 10m
          labels:
            severity: info
          annotations:
            summary: "输出 Token 数异常高"
            description: "P99 输出长度超过 4000 tokens,可能存在数据泄露风险"
            
    # =================================================================
    # 滥用检测
    # =================================================================
    - name: llm.security.abuse
      interval: 1m
      rules:
        # 速率限制触发
        - alert: RateLimitExceeded
          expr: |
            increase(llm_rate_limit_exceeded_total[5m]) > 100
          for: 5m
          labels:
            severity: warning
          annotations:
            summary: "速率限制频繁触发"
            
        # 单用户异常请求量
        - alert: SingleUserHighVolume
          expr: |
            sum(rate(llm_requests_total[1h])) by (user_id) > 1000
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "单用户请求量异常高"
            description: "用户 {{ $labels.user_id }} 每小时请求超过 1000 次"
            
        # API 密钥滥用
        - alert: APIKeyAbuse
          expr: |
            count(
              sum(rate(llm_requests_total[1h])) by (api_key_hash) > 10000
            ) > 0
          for: 1h
          labels:
            severity: critical
          annotations:
            summary: "检测到 API 密钥滥用"
            
    # =================================================================
    # 数据安全
    # =================================================================
    - name: llm.security.data
      interval: 5m
      rules:
        # 输出内容被过滤
        - alert: HighOutputFilterRate
          expr: |
            rate(llm_output_filtered_total[1h]) 
            / rate(llm_requests_total[1h]) > 0.1
          for: 30m
          labels:
            severity: warning
          annotations:
            summary: "输出过滤率超过 10%"
            description: "大量响应包含敏感信息被过滤"
            
        # PII 泄露检测
        - alert: PIILeakageDetected
          expr: |
            increase(llm_pii_detected_total[1h]) > 0
          labels:
            severity: critical
          annotations:
            summary: "检测到 PII 泄露"
            description: "类型: {{ $labels.pii_type }}"
```

## 合规检查清单

### 安全合规 Checklist

```yaml
# security-compliance-checklist.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: security-compliance-checklist
  namespace: llm-inference
data:
  checklist.yaml: |
    compliance:
      gdpr:
        - id: GDPR-1
          requirement: "用户数据删除机制"
          status: required
          implementation: "提供数据删除 API"
          
        - id: GDPR-2
          requirement: "数据导出功能"
          status: required
          implementation: "支持用户数据导出"
          
        - id: GDPR-3
          requirement: "隐私政策明示"
          status: required
          implementation: "API 返回隐私政策链接"
          
        - id: GDPR-4
          requirement: "数据处理记录"
          status: required
          implementation: "审计日志记录所有处理"
          
        - id: GDPR-5
          requirement: "数据最小化"
          status: required
          implementation: "不存储不必要的用户数据"
      
      access_control:
        - id: AC-1
          requirement: "API 密钥认证"
          status: required
          implementation: "所有请求需要有效 API Key"
          
        - id: AC-2
          requirement: "速率限制"
          status: required
          implementation: "按用户/API Key 限制请求频率"
          
        - id: AC-3
          requirement: "IP 白名单"
          status: optional
          implementation: "企业版支持 IP 限制"
          
        - id: AC-4
          requirement: "RBAC 权限控制"
          status: required
          implementation: "Kubernetes RBAC + 应用级权限"
      
      audit:
        - id: AUDIT-1
          requirement: "请求日志记录"
          status: required
          implementation: "记录所有 API 请求元数据"
          
        - id: AUDIT-2
          requirement: "90 天日志保留"
          status: required
          implementation: "Elasticsearch + S3 存储"
          
        - id: AUDIT-3
          requirement: "安全事件告警"
          status: required
          implementation: "Prometheus 告警规则"
          
        - id: AUDIT-4
          requirement: "审计追踪"
          status: required
          implementation: "不可变日志存储"
      
      model_protection:
        - id: MP-1
          requirement: "模型水印"
          status: recommended
          implementation: "输出嵌入不可见水印"
          
        - id: MP-2
          requirement: "输入验证"
          status: required
          implementation: "提示注入检测"
          
        - id: MP-3
          requirement: "输出过滤"
          status: required
          implementation: "PII 和敏感信息过滤"
          
        - id: MP-4
          requirement: "异常检测"
          status: required
          implementation: "基于指标的异常告警"
```

## 版本变更记录

| 版本 | 变更内容 | 影响 |
|-----|---------|------|
| **OWASP LLM Top 10 v1.1** | 更新威胁分类 | 新增供应链漏洞 |
| **Opacus 1.4** | 差分隐私改进 | 更好的 GPU 支持 |
| **K8s v1.28** | Pod Security Admission GA | 更严格的安全策略 |
| **K8s v1.29** | CEL 验证增强 | 更灵活的准入控制 |

## 最佳实践总结

### 安全防护检查清单

- [ ] 部署输入验证和提示注入检测
- [ ] 配置输出过滤和 PII 脱敏
- [ ] 实施速率限制和配额管理
- [ ] 启用差分隐私训练 (如适用)
- [ ] 配置网络策略限制访问
- [ ] 实施 RBAC 最小权限原则
- [ ] 部署安全监控和告警
- [ ] 启用审计日志记录
- [ ] 定期进行安全评估

### 关键监控指标

- `llm_prompt_injection_detected_total` - 注入检测次数
- `llm_output_filtered_total` - 输出过滤次数
- `llm_unauthorized_requests_total` - 未授权请求
- `llm_rate_limit_exceeded_total` - 速率限制触发
- `llm_pii_detected_total` - PII 检测次数

---

**参考资料**:
- [OWASP LLM Top 10](https://owasp.org/www-project-top-10-for-large-language-model-applications/)
- [NIST AI Risk Management Framework](https://www.nist.gov/itl/ai-risk-management-framework)
- [Opacus 差分隐私库](https://opacus.ai/)
