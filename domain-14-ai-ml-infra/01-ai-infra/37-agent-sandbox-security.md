---
title: AI Agent 沙箱安全架构
description: AI Agent 代码执行沙箱、工具调用安全、数据隔离、K8s 上的 Agent 安全部署方案
category: ai-infra
tags:
- k8s
- ai
- agent
- sandbox
- security
- isolation
- gvisor
- wasm
- rbac
- prometheus
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 安全工程师
- 架构师
estimated_read_time: 40min
intent_queries:
- AI Agent 沙箱怎么实现
- Agent 代码执行安全隔离
- Agent 工具调用权限控制
- K8s 上部署安全的 AI Agent
- Agent 提示注入防护
trigger_keywords:
- Agent Sandbox
- AI Agent 安全
- 代码执行沙箱
- 工具调用安全
- Agent 隔离
prerequisites:
- kubectl-basics
- prometheus-basics
- gpu-scheduling-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-05-security-compliance/
  label: 云原生安全知识域
- type: domain
  path: ../domain-14-ai-ml-infra/
  label: AI 基础设施
created: "2026-05-23"
---

# AI Agent 沙箱安全架构

> **适用版本**: [[Kubernetes|Kubernetes]] v1.28 - v1.33 | **最后更新**: 2026-05

---

<!-- chunk: 一、概述 -->
## 一、概述

AI Agent 具备**自主执行代码、调用工具、访问外部系统**的能力, 这带来了传统应用不存在的安全风险:

| 风险类型 | 说明 | 严重程度 |
|----------|------|----------|
| 代码执行 | Agent 生成并执行任意代码 (Python/Bash) | 极高 |
| 工具滥用 | Agent 调用删除/修改等危险操作 | 高 |
| 数据泄露 | Agent 读取敏感数据并外传 | 高 |
| 提示注入 | 恶意输入劫持 Agent 行为 | 高 |
| 资源耗尽 | Agent 无限循环或分配过多资源 | 中 |

**沙箱的目标**: 在不影响 Agent 功能的前提下, 最小化每次操作的风险边界。

---

<!-- chunk: 二、沙箱架构模式 -->
## 二、沙箱架构模式

### 2.1 四种模式对比

| 模式 | 隔离强度 | 启动速度 | 资源开销 | 适用场景 |
|------|---------|---------|---------|---------|
| 容器级 (gVisor) | ★★★★ | ~100ms | ~15MB | 通用代码执行 |
| VM 级 (Firecracker) | ★★★★★ | ~125ms | ~5MB | 不可信代码 |
| 进程级 (nsjail) | ★★★ | ~10ms | ~1MB | 轻量快速执行 |
| Wasm 级 | ★★★ | ~5ms | ~2MB | 边缘/轻量任务 |

### 2.2 推荐选型

```
Agent 任务类型          推荐沙箱           理由
──────────────────────────────────────────────────
Python 脚本执行         gVisor 容器       兼容性好, 隔离强
Bash 命令执行           nsjail + gVisor   快速启动, 双重隔离
文件读写               gVisor + 只读挂载  防止文件系统破坏
网络请求               NetworkPolicy     限制出口域名/IP
数据库操作             RBAC + 审计日志    最小权限 + 可追溯
不可信代码             Firecracker VM    最强隔离
```

---

<!-- chunk: 三、K8s 上的 Agent 沙箱实现 -->
## 三、K8s 上的 Agent 沙箱实现

### 3.1 Agent Pod 安全模板

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: agent-sandbox
  namespace: agent-workspace
  labels:
    app: ai-agent
    security-level: sandbox
spec:
  runtimeClassName: gvisor          # gVisor 沙箱
  serviceAccountName: agent-limited # 最小权限 SA
  securityContext:
    runAsNonRoot: true
    runAsUser: 65534
    fsGroup: 65534
    seccompProfile:
      type: RuntimeDefault
  containers:
    - name: agent-runtime
      image: agent-runtime:latest
      securityContext:
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        capabilities:
          drop: ["ALL"]
      resources:
        limits:
          cpu: "2"
          memory: "2Gi"
        requests:
          cpu: "500m"
          memory: "512Mi"
      volumeMounts:
        - name: tmp
          mountPath: /tmp
        - name: workspace
          mountPath: /workspace
          readOnly: false
      env:
        - name: AGENT_TIMEOUT
          value: "300"              # 5 分钟超时
        - name: AGENT_MAX_MEMORY
          value: "1Gi"
  volumes:
    - name: tmp
      emptyDir:
        sizeLimit: "500Mi"
    - name: workspace
      emptyDir:
        sizeLimit: "1Gi"
```

### 3.2 NetworkPolicy 限制

```yaml
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: agent-sandbox-netpol
  namespace: agent-workspace
spec:
  podSelector:
    matchLabels:
      app: ai-agent
  policyTypes:
    - Egress
  egress:
    # 允许 DNS
    - to: []
      ports:
        - port: 53
          protocol: UDP
    # 允许访问 LLM API
    - to:
        - ipBlock:
            cidr: 0.0.0.0/0
      ports:
        - port: 443
          protocol: TCP
    # 禁止访问内部服务 (默认拒绝)
    # 所有其他出站流量被阻断
```

### 3.3 RBAC 最小权限

```yaml
apiVersion: v1
kind: ServiceAccount
metadata:
  name: agent-limited
  namespace: agent-workspace
---
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: agent-readonly
  namespace: agent-workspace
rules:
  - apiGroups: [""]
    resources: ["pods", "services", "configmaps"]
    verbs: ["get", "list"]
  - apiGroups: ["apps"]
    resources: ["deployments"]
    verbs: ["get", "list"]
  # 不允许 create/update/delete
---
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: agent-readonly-binding
  namespace: agent-workspace
subjects:
  - kind: ServiceAccount
    name: agent-limited
roleRef:
  kind: Role
  name: agent-readonly
  apiGroup: rbac.authorization.k8s.io
```

---

<!-- chunk: 四、工具调用安全 -->
## 四、工具调用安全

### 4.1 分级审批模型

```
工具风险等级    示例                    Agent 行为
─────────────────────────────────────────────────
L0 无风险      查询状态/读取指标        自主执行
L1 低风险      读取日志/查看配置        自主执行 + 记录
L2 中风险      重启 Pod/修改 ConfigMap  执行前确认
L3 高风险      删除资源/修改 RBAC       强制人工审批
```

### 4.2 工具白名单配置

```yaml
# Agent 工具权限配置
agent_tools:
  allowed:
    - name: "kubectl_get"
      risk_level: "L0"
      params:
        resources: ["pods", "services", "nodes", "events"]
        verbs: ["get", "list"]
    
    - name: "kubectl_describe"
      risk_level: "L1"
      params:
        resources: ["pods", "services", "deployments"]
    
    - name: "kubectl_logs"
      risk_level: "L1"
      params:
        max_lines: 1000
  
  blocked:
    - "kubectl_delete"
    - "kubectl_exec"
    - "kubectl_cp"
  
  require_approval:
    - name: "kubectl_restart"
      risk_level: "L2"
      approvers: ["sre-team"]
    
    - name: "kubectl_scale"
      risk_level: "L2"
      approvers: ["sre-team"]
```

---

<!-- chunk: 五、代码执行沙箱 -->
## 五、代码执行沙箱

### 5.1 临时容器执行模式

```yaml
# Agent 代码执行 Pod (临时, 用完销毁)
apiVersion: v1
kind: Pod
metadata:
  name: agent-code-exec-{{execution_id}}
  namespace: agent-workspace
  annotations:
    agent.io/execution-id: "{{execution_id}}"
    agent.io/timeout: "300"
spec:
  runtimeClassName: gvisor
  restartPolicy: Never
  activeDeadlineSeconds: 300          # 强制 5 分钟超时
  securityContext:
    runAsNonRoot: true
    runAsUser: 65534
  containers:
    - name: executor
      image: python-sandbox:3.12      # 预装常用库的沙箱镜像
      command: ["python3", "-c", "{{agent_code}}"]
      securityContext:
        readOnlyRootFilesystem: true
        allowPrivilegeEscalation: false
      resources:
        limits:
          cpu: "1"
          memory: "512Mi"
      volumeMounts:
        - name: tmp
          mountPath: /tmp
  volumes:
    - name: tmp
      emptyDir:
        sizeLimit: "100Mi"
```

### 5.2 超时与资源控制

```python
# Agent 执行控制器
import kubernetes
import time

def execute_in_sandbox(code: str, timeout: int = 300):
    """在沙箱中执行 Agent 生成的代码"""
    pod_manifest = build_sandbox_pod(code, timeout)
    
    # 创建 Pod
    v1 = kubernetes.client.CoreV1Api()
    pod = v1.create_namespaced_pod("agent-workspace", pod_manifest)
    
    # 等待完成或超时
    start = time.time()
    while time.time() - start < timeout:
        status = v1.read_namespaced_pod_status(pod.metadata.name, "agent-workspace")
        if status.status.phase in ("Succeeded", "Failed"):
            break
        time.sleep(1)
    
    # 获取输出
    logs = v1.read_namespaced_pod_log(pod.metadata.name, "agent-workspace")
    
    # 清理 Pod
    v1.delete_namespaced_pod(pod.metadata.name, "agent-workspace")
    
    return logs
```

---

<!-- chunk: 六、监控与审计 -->
## 六、监控与审计

### 6.1 Agent 行为追踪

```yaml
# Prometheus 指标
agent_tool_calls_total{tool="kubectl_get", risk_level="L0"} 1523
agent_tool_calls_total{tool="kubectl_restart", risk_level="L2"} 12
agent_tool_calls_blocked{tool="kubectl_delete", reason="not_allowed"} 5
agent_code_executions_total{status="success"} 342
agent_code_executions_total{status="timeout"} 8
agent_code_executions_total{status="oom"} 3
agent_approval_pending{risk_level="L2"} 2
agent_approval_pending{risk_level="L3"} 0
```

### 6.2 审计日志格式

```json
{
  "timestamp": "2026-05-19T10:30:00Z",
  "agent_id": "agent-001",
  "execution_id": "exec-abc123",
  "action": "tool_call",
  "tool": "kubectl_restart",
  "risk_level": "L2",
  "params": {"namespace": "production", "deployment": "api-server"},
  "status": "approved",
  "approver": "sre-oncall",
  "latency_ms": 1234
}
```

---

<!-- chunk: 七、生产检查清单 -->
## 七、生产检查清单

- [ ] Agent Pod 使用 gVisor RuntimeClass
- [ ] readOnlyRootFilesystem 启用
- [ ] runAsNonRoot 强制
- [ ] NetworkPolicy 限制出站流量
- [ ] RBAC 最小权限 (只读)
- [ ] 代码执行超时 ≤ 5 分钟
- [ ] 内存限制 ≤ 2Gi
- [ ] 工具白名单配置
- [ ] L2/L3 操作人工审批
- [ ] 审计日志全量记录
- [ ] 异常行为告警 (调用频率/超时/OOM)
- [ ] 沙箱 Pod 自动清理 (TTL)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-11-ai-infra KUDIG Database — Global MOC
- [[domain-14-ai-ml-infra/README|Domain-11: AI基础设施]]
- index.md|Domain-11 AI 基础设施 — 开源项目索引]]
- AI 基础设施架构
- 132 - AI/ML工作负载运维 (AI/ML Workloads Operations)
- GPU 调度与管理
- GPU监控与可观测性
- 分布式训练框架
- AI数据处理Pipeline与特征工程
- AI实验管理与MLOps平台
- AutoML与超参数调优
- AI模型注册中心与版本管理

## See Also

- 35-model-drift-monitoring
- 36-ai-platform-observability-enhanced
- 99-kubeflow-ai-platform-guide
- 01-ai-infrastructure-overview
