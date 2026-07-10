---
title: Agent多租户架构
description: 'Agent系统租户隔离、Namespace模型、API Key管理、用量配额、审计日志与K8s NetworkPolicy'
summary: 'Agent系统租户隔离、Namespace模型、API Key管理、用量配额、审计日志与K8s NetworkPolicy'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- multi-tenancy
- isolation
- security
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- 安全工程师
estimated_read_time: 20min
intent_queries:
- Agent多租户架构 是什么
- 如何实现Agent租户隔离
- K8s Agent多租户
- Agent API Key管理
trigger_keywords:
- multi-tenancy
- tenant isolation
- namespace
- api key
- quota
- audit
- network policy
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

# Agent多租户架构

## 概述

当Agent平台面向多个团队或客户提供服务时，多租户架构成为核心需求。Agent多租户的隔离维度比传统SaaS更复杂：除了数据和网络隔离，还需要模型访问隔离、工具权限隔离、知识库隔离和独立计费。

本文覆盖四大隔离维度、Namespace-per-Tenant模型、API Key管理、用量配额、审计日志和K8s NetworkPolicy配置。

## 1. 租户隔离维度

### 1.1 隔离矩阵

```
┌──────────────┬──────────────────────────────────────────┐
│   隔离维度    │              隔离策略                     │
├──────────────┼──────────────────────────────────────────┤
│ 数据隔离      │ 租户独立数据库/Schema/行级过滤            │
│              │ 向量数据库租户分区                         │
│              │ 对象存储前缀隔离                           │
├──────────────┼──────────────────────────────────────────┤
│ 模型访问隔离  │ 租户独立模型白名单                        │
│              │ 独立Token配额                             │
│              │ 模型路由策略差异                           │
├──────────────┼──────────────────────────────────────────┤
│ 工具权限隔离  │ 租户工具白名单                            │
│              │ API Key绑定                               │
│              │ 沙箱执行环境                               │
├──────────────┼──────────────────────────────────────────┤
│ 计费隔离      │ 独立预算账户                              │
│              │ 用量计量与账单                             │
│              │ 预付/后付模式                              │
└──────────────┴──────────────────────────────────────────┘
```

### 1.2 隔离级别

```python
from enum import Enum

class IsolationLevel(Enum):
    """租户隔离级别"""
    SHARED = "shared"           # 共享资源，逻辑隔离
    NAMESPACE = "namespace"     # Namespace隔离
    DEDICATED = "dedicated"     # 独立集群/资源池

ISOLATION_MATRIX = {
    IsolationLevel.SHARED: {
        "compute": "共享Pod，请求级隔离",
        "data": "共享数据库，行级过滤",
        "model": "共享API Key，配额隔离",
        "network": "共享网络，应用层隔离",
        "cost": "最低",
        "isolation": "弱",
    },
    IsolationLevel.NAMESPACE: {
        "compute": "独立Namespace，独立Pod",
        "data": "独立数据库实例或Schema",
        "model": "独立API Key或Key Pool",
        "network": "NetworkPolicy隔离",
        "cost": "中等",
        "isolation": "中",
    },
    IsolationLevel.DEDICATED: {
        "compute": "独立节点池/集群",
        "data": "完全独立数据库集群",
        "model": "独立模型部署",
        "network": "VPC隔离",
        "cost": "高",
        "isolation": "强",
    },
}
```

## 2. Namespace-per-Tenant模型

### 2.1 架构设计

```
┌─────────────────────────────────────────────────────────┐
│                    K8s Cluster                           │
│                                                          │
│  ┌─────────────────────┐  ┌─────────────────────┐      │
│  │ ns: tenant-acme     │  │ ns: tenant-globex   │      │
│  │                     │  │                     │      │
│  │ ┌─────────────────┐ │  │ ┌─────────────────┐ │      │
│  │ │ agent-runtime   │ │  │ │ agent-runtime   │ │      │
│  │ │ (Deployment)    │ │  │ │ (Deployment)    │ │      │
│  │ └─────────────────┘ │  │ └─────────────────┘ │      │
│  │ ┌─────────────────┐ │  │ ┌─────────────────┐ │      │
│  │ │ knowledge-db    │ │  │ │ knowledge-db    │ │      │
│  │ │ (StatefulSet)   │ │  │ │ (StatefulSet)   │ │      │
│  │ └─────────────────┘ │  │ └─────────────────┘ │      │
│  │ ┌─────────────────┐ │  │ ┌─────────────────┐ │      │
│  │ │ cache           │ │  │ │ cache           │ │      │
│  │ │ (Redis)         │ │  │ │ (Redis)         │ │      │
│  │ └─────────────────┘ │  │ └─────────────────┘ │      │
│  │                     │  │                     │      │
│  │ NetworkPolicy:      │  │ NetworkPolicy:      │      │
│  │ deny-all + allow    │  │ deny-all + allow    │      │
│  └─────────────────────┘  └─────────────────────┘      │
│                                                          │
│  ┌──────────────────────────────────────────────────┐   │
│  │  ns: platform (共享服务)                           │   │
│  │  - API Gateway    - LLM Proxy    - Auth Service  │   │
│  │  - Billing        - Monitoring   - Audit Log     │   │
│  └──────────────────────────────────────────────────┘   │
└─────────────────────────────────────────────────────────┘
```

### 2.2 租户Namespace模板

```yaml
# tenant-namespace-template.yaml
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-${TENANT_ID}
  labels:
    tenant: ${TENANT_ID}
    isolation: namespace
    managed-by: agent-platform
  annotations:
    tenant.company.com/owner: "${TENANT_OWNER}"
    tenant.company.com/tier: "${TENANT_TIER}"
---
# ResourceQuota - 租户资源配额
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-quota
  namespace: tenant-${TENANT_ID}
spec:
  hard:
    requests.cpu: "8"
    requests.memory: "16Gi"
    limits.cpu: "16"
    limits.memory: "32Gi"
    pods: "20"
    services: "10"
    persistentvolumeclaims: "10"
---
# LimitRange - 默认资源限制
apiVersion: v1
kind: LimitRange
metadata:
  name: tenant-limits
  namespace: tenant-${TENANT_ID}
spec:
  limits:
  - type: Container
    default:
      cpu: "500m"
      memory: "512Mi"
    defaultRequest:
      cpu: "100m"
      memory: "128Mi"
    max:
      cpu: "4"
      memory: "8Gi"
---
# ServiceAccount - 租户服务账户
apiVersion: v1
kind: ServiceAccount
metadata:
  name: tenant-agent-sa
  namespace: tenant-${TENANT_ID}
---
# Role - 租户角色
apiVersion: rbac.authorization.k8s.io/v1
kind: Role
metadata:
  name: tenant-agent-role
  namespace: tenant-${TENANT_ID}
rules:
- apiGroups: [""]
  resources: ["pods", "services", "configmaps", "secrets"]
  verbs: ["get", "list", "watch", "create", "update", "delete"]
- apiGroups: ["apps"]
  resources: ["deployments", "statefulsets"]
  verbs: ["get", "list", "watch", "create", "update", "delete"]
---
# RoleBinding
apiVersion: rbac.authorization.k8s.io/v1
kind: RoleBinding
metadata:
  name: tenant-agent-binding
  namespace: tenant-${TENANT_ID}
subjects:
- kind: ServiceAccount
  name: tenant-agent-sa
  namespace: tenant-${TENANT_ID}
roleRef:
  kind: Role
  name: tenant-agent-role
  apiGroup: rbac.authorization.k8s.io
```

### 2.3 租户自动开通

```python
from dataclasses import dataclass
import yaml

@dataclass
class TenantConfig:
    tenant_id: str
    owner: str
    tier: str  # free/pro/enterprise
    max_agents: int
    max_tokens_per_day: int
    allowed_models: list[str]
    allowed_tools: list[str]

class TenantProvisioner:
    """租户自动开通"""

    TIER_DEFAULTS = {
        "free": {
            "max_agents": 3,
            "max_tokens_per_day": 100_000,
            "allowed_models": ["gpt-4o-mini"],
            "allowed_tools": ["search", "calculator"],
            "cpu": "2",
            "memory": "4Gi",
        },
        "pro": {
            "max_agents": 20,
            "max_tokens_per_day": 1_000_000,
            "allowed_models": ["gpt-4o-mini", "gpt-4o", "claude-sonnet"],
            "allowed_tools": ["search", "calculator", "code_interpreter", "web_browse"],
            "cpu": "8",
            "memory": "16Gi",
        },
        "enterprise": {
            "max_agents": 100,
            "max_tokens_per_day": 10_000_000,
            "allowed_models": ["*"],  # 所有模型
            "allowed_tools": ["*"],   # 所有工具
            "cpu": "32",
            "memory": "64Gi",
        },
    }

    def __init__(self, k8s_client, db_client):
        self.k8s = k8s_client
        self.db = db_client

    async def provision(self, tenant_id: str, owner: str, tier: str) -> dict:
        """开通租户"""
        defaults = self.TIER_DEFAULTS[tier]

        # 1. 创建Namespace
        ns_manifest = self._render_namespace(tenant_id, owner, tier, defaults)
        await self.k8s.apply(ns_manifest)

        # 2. 创建资源配额
        quota_manifest = self._render_quota(tenant_id, defaults)
        await self.k8s.apply(quota_manifest)

        # 3. 创建NetworkPolicy
        netpol_manifest = self._render_network_policy(tenant_id)
        await self.k8s.apply(netpol_manifest)

        # 4. 创建API Key
        api_key = await self._generate_api_key(tenant_id)

        # 5. 初始化数据库
        await self.db.create_tenant_schema(tenant_id)

        # 6. 注册到租户管理表
        await self.db.register_tenant(TenantConfig(
            tenant_id=tenant_id,
            owner=owner,
            tier=tier,
            **defaults,
        ))

        return {
            "tenant_id": tenant_id,
            "namespace": f"tenant-{tenant_id}",
            "api_key": api_key,
            "tier": tier,
            "limits": defaults,
        }

    def _render_namespace(self, tenant_id, owner, tier, defaults):
        return yaml.safe_load(f"""
apiVersion: v1
kind: Namespace
metadata:
  name: tenant-{tenant_id}
  labels:
    tenant: "{tenant_id}"
    tier: "{tier}"
  annotations:
    tenant.company.com/owner: "{owner}"
""")

    def _render_quota(self, tenant_id, defaults):
        return yaml.safe_load(f"""
apiVersion: v1
kind: ResourceQuota
metadata:
  name: tenant-quota
  namespace: tenant-{tenant_id}
spec:
  hard:
    requests.cpu: "{defaults['cpu']}"
    requests.memory: "{defaults['memory']}"
    pods: "20"
""")

    def _render_network_policy(self, tenant_id):
        return yaml.safe_load(f"""
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: tenant-isolation
  namespace: tenant-{tenant_id}
spec:
  podSelector: {{}}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: platform
    - podSelector: {{}}
  egress:
  - to:
    - namespaceSelector:
        matchLabels:
          name: platform
    - podSelector: {{}}
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
    ports:
    - protocol: TCP
      port: 443
""")

    async def _generate_api_key(self, tenant_id):
        import secrets
        key = f"sk-agent-{tenant_id}-{secrets.token_hex(24)}"
        # 存储到Secret
        await self.k8s.create_secret(
            namespace=f"tenant-{tenant_id}",
            name="api-keys",
            data={"api-key": key}
        )
        return key
```

## 3. API Key管理

### 3.1 多层API Key体系

```python
from dataclasses import dataclass
from enum import Enum
from datetime import datetime

class KeyType(Enum):
    MASTER = "master"       # 主密钥，租户管理员
    AGENT = "agent"         # Agent密钥，绑定特定Agent
    SESSION = "session"     # 会话密钥，临时

@dataclass
class APIKey:
    key_id: str
    tenant_id: str
    key_type: KeyType
    key_hash: str           # 哈希存储，不存明文
    agent_id: str | None    # 绑定的Agent
    permissions: list[str]
    rate_limit: int         # RPM
    token_budget: int       # 日Token预算
    expires_at: datetime | None
    created_at: datetime
    last_used_at: datetime | None
    is_active: bool

class APIKeyManager:
    """API Key管理器"""

    def __init__(self, redis_client, db_client):
        self.redis = redis_client
        self.db = db_client

    def create_key(
        self,
        tenant_id: str,
        key_type: KeyType,
        agent_id: str | None = None,
        permissions: list[str] | None = None,
        rate_limit: int = 60,
        token_budget: int = 100_000,
        expires_in_days: int | None = None,
    ) -> tuple[str, APIKey]:
        """创建API Key"""
        import secrets
        import hashlib

        # 生成Key
        raw_key = f"sk-{key_type.value}-{secrets.token_hex(32)}"
        key_hash = hashlib.sha256(raw_key.encode()).hexdigest()
        key_id = f"key-{secrets.token_hex(8)}"

        expires_at = None
        if expires_in_days:
            from datetime import timedelta
            expires_at = datetime.utcnow() + timedelta(days=expires_in_days)

        api_key = APIKey(
            key_id=key_id,
            tenant_id=tenant_id,
            key_type=key_type,
            key_hash=key_hash,
            agent_id=agent_id,
            permissions=permissions or ["chat", "tools"],
            rate_limit=rate_limit,
            token_budget=token_budget,
            expires_at=expires_at,
            created_at=datetime.utcnow(),
            last_used_at=None,
            is_active=True,
        )

        # 存储到数据库
        self.db.save_api_key(api_key)

        # 缓存到Redis（快速校验）
        self.redis.hset(f"apikey:{key_hash}", mapping={
            "tenant_id": tenant_id,
            "key_type": key_type.value,
            "agent_id": agent_id or "",
            "rate_limit": str(rate_limit),
            "token_budget": str(token_budget),
        })

        return raw_key, api_key

    def validate_key(self, raw_key: str) -> APIKey | None:
        """校验API Key"""
        import hashlib
        key_hash = hashlib.sha256(raw_key.encode()).hexdigest()

        # 先查Redis缓存
        cached = self.redis.hgetall(f"apikey:{key_hash}")
        if not cached:
            # 回源数据库
            api_key = self.db.get_api_key_by_hash(key_hash)
            if not api_key or not api_key.is_active:
                return None
            return api_key

        # 检查过期
        # ... 省略过期检查

        return cached

    def revoke_key(self, key_id: str):
        """吊销API Key"""
        api_key = self.db.get_api_key(key_id)
        if api_key:
            api_key.is_active = False
            self.db.update_api_key(api_key)
            self.redis.delete(f"apikey:{api_key.key_hash}")
```

### 3.2 K8s Secret存储

```yaml
# 租户API Key Secret
apiVersion: v1
kind: Secret
metadata:
  name: tenant-api-keys
  namespace: tenant-${TENANT_ID}
type: Opaque
stringData:
  master-key: "sk-master-xxxxx"
  agent-key-default: "sk-agent-xxxxx"
  llm-proxy-key: "sk-proxy-xxxxx"
---
# External Secrets Operator - 从Vault同步
apiVersion: external-secrets.io/v1beta1
kind: ExternalSecret
metadata:
  name: tenant-api-keys
  namespace: tenant-${TENANT_ID}
spec:
  refreshInterval: 1h
  secretStoreRef:
    name: vault-backend
    kind: ClusterSecretStore
  target:
    name: tenant-api-keys
  data:
  - secretKey: master-key
    remoteRef:
      key: secret/data/tenants/${TENANT_ID}
      property: master-key
  - secretKey: llm-proxy-key
    remoteRef:
      key: secret/data/tenants/${TENANT_ID}
      property: llm-proxy-key
```

## 4. 用量配额

### 4.1 多维配额管理

```python
from dataclasses import dataclass
from datetime import date

@dataclass
class TenantQuota:
    tenant_id: str
    max_agents: int
    max_tokens_per_day: int
    max_tokens_per_month: int
    max_requests_per_minute: int
    max_tool_calls_per_day: int
    max_knowledge_base_size_gb: float

class QuotaManager:
    """租户配额管理"""

    def __init__(self, redis_client, db_client):
        self.redis = redis_client
        self.db = db_client

    def check_quota(
        self,
        tenant_id: str,
        resource: str,
        amount: int = 1
    ) -> tuple[bool, dict]:
        """检查配额"""
        quota = self.db.get_tenant_quota(tenant_id)
        if not quota:
            return False, {"error": "tenant_not_found"}

        today = date.today().isoformat()
        month = today[:7]

        checks = {
            "tokens_daily": {
                "used": self._get_usage(tenant_id, f"tokens:daily:{today}"),
                "limit": quota.max_tokens_per_day,
            },
            "tokens_monthly": {
                "used": self._get_usage(tenant_id, f"tokens:monthly:{month}"),
                "limit": quota.max_tokens_per_month,
            },
            "tool_calls_daily": {
                "used": self._get_usage(tenant_id, f"tool_calls:daily:{today}"),
                "limit": quota.max_tool_calls_per_day,
            },
        }

        if resource not in checks:
            return True, {}

        check = checks[resource]
        if check["used"] + amount > check["limit"]:
            return False, {
                "resource": resource,
                "used": check["used"],
                "limit": check["limit"],
                "requested": amount,
            }

        return True, checks

    def record_usage(self, tenant_id: str, resource: str, amount: int):
        """记录用量"""
        today = date.today().isoformat()
        month = today[:7]

        if resource == "tokens":
            self._incr_usage(tenant_id, f"tokens:daily:{today}", amount, 86400 * 2)
            self._incr_usage(tenant_id, f"tokens:monthly:{month}", amount, 86400 * 35)
        elif resource == "tool_calls":
            self._incr_usage(tenant_id, f"tool_calls:daily:{today}", amount, 86400 * 2)

    def _get_usage(self, tenant_id: str, key: str) -> int:
        value = self.redis.get(f"quota:{tenant_id}:{key}")
        return int(value) if value else 0

    def _incr_usage(self, tenant_id: str, key: str, amount: int, ttl: int):
        full_key = f"quota:{tenant_id}:{key}"
        self.redis.incrby(full_key, amount)
        self.redis.expire(full_key, ttl)
```

## 5. 审计日志

### 5.1 审计事件结构

```python
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional

@dataclass
class AuditEvent:
    """审计事件"""
    event_id: str
    tenant_id: str
    user_id: str
    agent_id: str
    action: str                # chat/tool_call/admin/config_change
    resource: str              # 资源标识
    details: dict              # 事件详情
    ip_address: str
    user_agent: str
    timestamp: datetime
    status: str                # success/failure/denied
    risk_level: str            # low/medium/high

class AuditLogger:
    """审计日志记录器"""

    def __init__(self, kafka_producer, db_client):
        self.kafka = kafka_producer
        self.db = db_client

    def log(self, event: AuditEvent):
        """记录审计事件"""
        # 发送到Kafka（异步持久化）
        self.kafka.send(
            topic="agent-audit-log",
            key=event.tenant_id.encode(),
            value=self._serialize(event),
        )

        # 高风险事件同步写入数据库
        if event.risk_level == "high":
            self.db.insert_audit_event(event)

    def log_chat(self, tenant_id: str, user_id: str, agent_id: str, message: str, response: str):
        """记录对话事件"""
        self.log(AuditEvent(
            event_id=self._generate_id(),
            tenant_id=tenant_id,
            user_id=user_id,
            agent_id=agent_id,
            action="chat",
            resource=f"agent:{agent_id}",
            details={
                "input_length": len(message),
                "output_length": len(response),
                "input_preview": message[:200],
            },
            ip_address="",
            user_agent="",
            timestamp=datetime.utcnow(),
            status="success",
            risk_level="low",
        ))

    def log_tool_call(self, tenant_id: str, user_id: str, agent_id: str, tool_name: str, params: dict, result: str):
        """记录工具调用事件"""
        self.log(AuditEvent(
            event_id=self._generate_id(),
            tenant_id=tenant_id,
            user_id=user_id,
            agent_id=agent_id,
            action="tool_call",
            resource=f"tool:{tool_name}",
            details={
                "tool": tool_name,
                "params": params,
                "result_preview": result[:500],
            },
            ip_address="",
            user_agent="",
            timestamp=datetime.utcnow(),
            status="success",
            risk_level="medium",
        ))

    def _serialize(self, event: AuditEvent) -> bytes:
        import json
        return json.dumps({
            "event_id": event.event_id,
            "tenant_id": event.tenant_id,
            "user_id": event.user_id,
            "agent_id": event.agent_id,
            "action": event.action,
            "resource": event.resource,
            "details": event.details,
            "timestamp": event.timestamp.isoformat(),
            "status": event.status,
            "risk_level": event.risk_level,
        }).encode()

    def _generate_id(self):
        import secrets
        return f"audit-{secrets.token_hex(8)}"
```

### 5.2 审计日志查询

```yaml
# K8s部署审计日志服务
apiVersion: apps/v1
kind: Deployment
metadata:
  name: audit-log-service
  namespace: platform
spec:
  replicas: 2
  selector:
    matchLabels:
      app: audit-log
  template:
    spec:
      containers:
      - name: audit
        image: audit-log-service:latest
        env:
        - name: KAFKA_BROKERS
          value: "kafka.platform.svc.cluster.local:9092"
        - name: CLICKHOUSE_URL
          value: "http://clickhouse.platform.svc.cluster.local:8123"
        resources:
          requests:
            cpu: "250m"
            memory: "256Mi"
---
# ClickHouse审计表
# CREATE TABLE agent_audit_log (
#     event_id String,
#     tenant_id String,
#     user_id String,
#     agent_id String,
#     action String,
#     resource String,
#     details String,
#     timestamp DateTime,
#     status String,
#     risk_level String
# ) ENGINE = MergeTree()
# PARTITION BY toYYYYMM(timestamp)
# ORDER BY (tenant_id, timestamp)
# TTL timestamp + INTERVAL 365 DAY
```

## 6. K8s NetworkPolicy

### 6.1 租户网络隔离

```yaml
# 默认拒绝所有流量
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-all
  namespace: tenant-${TENANT_ID}
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
---
# 允许租户内部通信
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-tenant-internal
  namespace: tenant-${TENANT_ID}
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  - Egress
  ingress:
  - from:
    - podSelector: {}  # 同Namespace内Pod
  egress:
  - to:
    - podSelector: {}  # 同Namespace内Pod
---
# 允许访问平台共享服务
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-platform-services
  namespace: tenant-${TENANT_ID}
spec:
  podSelector:
    matchLabels:
      app: agent-runtime
  policyTypes:
  - Egress
  egress:
  # 允许访问LLM Proxy
  - to:
    - namespaceSelector:
        matchLabels:
          name: platform
      podSelector:
        matchLabels:
          app: llm-proxy
    ports:
    - protocol: TCP
      port: 8080
  # 允许访问Auth Service
  - to:
    - namespaceSelector:
        matchLabels:
          name: platform
      podSelector:
        matchLabels:
          app: auth-service
    ports:
    - protocol: TCP
      port: 8080
  # 允许访问外部HTTPS（LLM API等）
  - to:
    - ipBlock:
        cidr: 0.0.0.0/0
        except:
        - 10.0.0.0/8      # 排除内网
        - 172.16.0.0/12
        - 192.168.0.0/16
    ports:
    - protocol: TCP
      port: 443
---
# 允许平台访问租户（监控/管理）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: allow-platform-monitoring
  namespace: tenant-${TENANT_ID}
spec:
  podSelector: {}
  policyTypes:
  - Ingress
  ingress:
  - from:
    - namespaceSelector:
        matchLabels:
          name: platform
      podSelector:
        matchLabels:
          app: prometheus
    ports:
    - protocol: TCP
      port: 9090  # metrics
```

### 6.2 网络策略验证

```python
class NetworkPolicyValidator:
    """网络策略验证器"""

    def __init__(self, k8s_client):
        self.k8s = k8s_client

    async def validate_isolation(self, tenant_id: str) -> dict:
        """验证租户网络隔离"""
        namespace = f"tenant-{tenant_id}"
        results = {
            "namespace": namespace,
            "checks": [],
        }

        # 检查1: NetworkPolicy是否存在
        policies = await self.k8s.list_network_policies(namespace)
        results["checks"].append({
            "check": "network_policy_exists",
            "passed": len(policies) > 0,
            "details": f"Found {len(policies)} policies",
        })

        # 检查2: 默认拒绝策略
        has_deny_all = any(
            p.spec.pod_selector == {} and "Ingress" in p.spec.policy_types and "Egress" in p.spec.policy_types
            for p in policies
        )
        results["checks"].append({
            "check": "default_deny_all",
            "passed": has_deny_all,
        })

        # 检查3: 跨租户访问隔离
        cross_tenant_blocked = await self._test_cross_tenant_access(tenant_id)
        results["checks"].append({
            "check": "cross_tenant_isolation",
            "passed": cross_tenant_blocked,
        })

        results["all_passed"] = all(c["passed"] for c in results["checks"])
        return results

    async def _test_cross_tenant_access(self, tenant_id: str) -> bool:
        """测试跨租户访问是否被阻止"""
        # 实际执行网络连通性测试
        # 从tenant-A尝试访问tenant-B的Pod
        return True  # 简化实现
```

## 相关主题

- [[AI基础设施/Agent运行时/17-agent-rate-limiting-cost-control|Agent限流与成本控制]]
- [[AI基础设施/Agent运行时/19-agent-ci-cd-pipeline|Agent CI/CD流水线]]
- [[AI基础设施/Agent运行时/21-agent-runtime-architecture-overview|Agent Runtime架构总览]]

## 参考资料

- Kubernetes Multi-Tenancy
- Network Policy Recipes
- External Secrets Operator
