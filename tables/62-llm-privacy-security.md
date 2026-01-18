# 62 - LLM隐私与安全

> **适用版本**: v1.25 - v1.32 | **参考**: [OWASP LLM Top 10](https://owasp.org/www-project-top-10-for-large-language-model-applications/)

## 一、LLM安全威胁

| 威胁类型 | 严重性 | 攻击示例 | 防御措施 |
|---------|--------|---------|---------|
| **提示注入** | 高 | "Ignore instructions" | 输入验证 |
| **数据泄露** | 极高 | 训练数据记忆化 | 差分隐私 |
| **模型窃取** | 高 | API探测 | 速率限制 |
| **拒绝服务** | 中 | 超长输入 | 长度限制 |
| **有害内容** | 高 | 生成恶意代码 | 内容过滤 |

## 二、提示注入防御

```python
import re
from typing import Tuple

class PromptInjectionDefense:
    DANGEROUS_PATTERNS = [
        r"ignore\s+(all\s+)?previous\s+instructions?",
        r"disregard\s+",
        r"forget\s+",
        r"system\s*:",
        r"<\|im_start\|>",
    ]
    
    def detect_injection(self, user_input: str) -> Tuple[bool, str]:
        for pattern in self.DANGEROUS_PATTERNS:
            if re.search(pattern, user_input, re.IGNORECASE):
                return True, f"Detected: {pattern}"
        return False, ""
    
    def sanitize_input(self, user_input: str) -> str:
        # 移除特殊标记
        sanitized = re.sub(r'<\|.*?\|>', '', user_input)
        # 长度限制
        sanitized = sanitized[:4096]
        return sanitized

# 使用
defense = PromptInjectionDefense()
is_injection, reason = defense.detect_injection(user_input)
if is_injection:
    return {"error": "Input rejected for security"}
```

## 三、访问控制

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: api-rate-limits
data:
  limits.yaml: |
    tiers:
      free:
        requests_per_hour: 100
        max_tokens: 1000
      pro:
        requests_per_hour: 10000
        max_tokens: 4000
      enterprise:
        requests_per_hour: 100000
        max_tokens: 8000
---
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: llm-inference-policy
spec:
  podSelector:
    matchLabels:
      app: llm-inference
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: api-gateway
```

## 四、差分隐私训练

```python
from opacus import PrivacyEngine
import torch

# 启用差分隐私
model = MyModel()
optimizer = torch.optim.Adam(model.parameters())

privacy_engine = PrivacyEngine()
model, optimizer, train_loader = privacy_engine.make_private(
    module=model,
    optimizer=optimizer,
    data_loader=train_loader,
    noise_multiplier=1.1,
    max_grad_norm=1.0
)

# 训练
for epoch in range(epochs):
    for batch in train_loader:
        optimizer.zero_grad()
        loss = model(batch)
        loss.backward()
        optimizer.step()
    
    # 隐私预算
    epsilon = privacy_engine.get_epsilon(delta=1e-5)
    print(f"ε = {epsilon:.2f}")
```

## 五、内容过滤

```python
class OutputFilter:
    HARMFUL_PATTERNS = [
        r"(password|api[_\s]?key)\s*[:=]",
        r"\b\d{16}\b",  # 信用卡
        r"\b\d{3}-\d{2}-\d{4}\b",  # SSN
    ]
    
    def filter_output(self, output: str) -> str:
        for pattern in self.HARMFUL_PATTERNS:
            output = re.sub(pattern, "[REDACTED]", output)
        return output

# 集成到推理
@app.post("/v1/chat/completions")
def chat(request: dict):
    # 输入检查
    defense = PromptInjectionDefense()
    if defense.detect_injection(request["messages"][-1]["content"])[0]:
        raise HTTPException(403, "Rejected")
    
    # 生成
    response = llm.generate(request["messages"])
    
    # 输出过滤
    filter = OutputFilter()
    response = filter.filter_output(response)
    
    return response
```

## 六、审计日志

```yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: audit-config
data:
  audit.yaml: |
    log_requests: true
    log_responses: false  # 隐私考虑
    retention_days: 90
    fields:
      - timestamp
      - user_id
      - model_name
      - input_tokens
      - output_tokens
      - latency
---
apiVersion: apps/v1
kind: Deployment
metadata:
  name: audit-logger
spec:
  template:
    spec:
      containers:
      - name: logger
        image: fluent/fluentd:v1.16
        volumeMounts:
        - name: logs
          mountPath: /var/log
```

## 七、监控告警

```yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: security-alerts
spec:
  groups:
  - name: llm_security
    rules:
    - alert: HighInjectionRate
      expr: rate(prompt_injection_detected_total[5m]) > 0.1
      annotations:
        summary: "提示注入检测率>10%"
    
    - alert: UnauthorizedAccess
      expr: rate(unauthorized_requests_total[5m]) > 1
      annotations:
        summary: "未授权访问尝试"
    
    - alert: AnomalousOutput
      expr: avg(output_token_count) > 2000
      annotations:
        summary: "异常长输出，疑似数据泄露"
```

## 八、合规Checklist

- [ ] **GDPR合规**
  - [ ] 用户数据删除机制
  - [ ] 数据导出功能
  - [ ] 隐私政策明示

- [ ] **访问控制**
  - [ ] API密钥认证
  - [ ] 速率限制
  - [ ] IP白名单

- [ ] **审计**
  - [ ] 请求日志记录
  - [ ] 90天日志保留
  - [ ] 安全事件告警

- [ ] **模型保护**
  - [ ] 模型水印
  - [ ] 输入/输出验证
  - [ ] 异常检测

---
**相关**: [119-AI安全](../119-ai-security-model-protection.md) | **版本**: K8s v1.27+
