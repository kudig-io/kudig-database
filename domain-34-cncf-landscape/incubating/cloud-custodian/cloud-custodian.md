---
title: Cloud Custodian
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- rag
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- Cloud Custodian 是什么
- 如何 Cloud Custodian
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- Cloud
- Custodian
- cncf
- landscape
---


# Cloud Custodian

> **成熟度**: Incubating | **加入时间**: 2022-11 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://cloudcustodian.io |
| **GitHub** | https://github.com/cloud-custodian/cloud-custodian |
| **许可证** | Apache-2.0 |
| **主要语言** | Python |
| **CNCF 分类** | Provisioning & Cloud Management |

---

## 项目概述

Cloud Custodian 是云资源治理和管理的规则引擎，通过 YAML 策略实现云资源的合规性、成本优化和安全管理。它支持 AWS、Azure、GCP 等主流云平台和 Kubernetes。

## 核心特性

- **声明式策略**: YAML 定义资源筛选和操作规则
- **多云支持**: AWS、Azure、GCP、Kubernetes
- **实时监控**: 事件驱动的实时策略执行
- **成本优化**: 识别闲置资源、调整规格
- **安全合规**: 检测配置违规、自动修复
- **丰富过滤器**: 200+ 资源类型和过滤条件
- **灵活操作**: 标记、通知、停止、删除等

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                  Cloud Custodian Architecture                    │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                     Policy Definitions                     │ │
│  │                      (YAML Files)                          │ │
│  │  ┌─────────────┐ ┌─────────────┐ ┌─────────────────────┐  │ │
│  │  │  Security   │ │    Cost     │ │   Compliance        │  │ │
│  │  │  Policies   │ │  Policies   │ │   Policies          │  │ │
│  │  └─────────────┘ └─────────────┘ └─────────────────────┘  │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                   Custodian Engine                         │ │
│  │                                                            │ │
│  │  ┌─────────────┐  ┌─────────────┐  ┌─────────────────┐   │ │
│  │  │  Resource   │  │   Filter    │  │    Action       │   │ │
│  │  │  Discovery  │  │   Engine    │  │    Executor     │   │ │
│  │  └─────────────┘  └─────────────┘  └─────────────────┘   │ │
│  └────────────────────────────────────────────────────────────┘ │
│                              │                                   │
│              ┌───────────────┼───────────────────┐              │
│              │               │                   │              │
│              ▼               ▼                   ▼              │
│  ┌──────────────┐  ┌──────────────┐  ┌────────────────────┐   │
│  │     AWS      │  │    Azure     │  │       GCP          │   │
│  │  Resources   │  │  Resources   │  │    Resources       │   │
│  └──────────────┘  └──────────────┘  └────────────────────┘   │
│              │               │                   │              │
│              └───────────────┼───────────────────┘              │
│                              ▼                                   │
│  ┌────────────────────────────────────────────────────────────┐ │
│  │                    Outputs / Actions                       │ │
│  │  ┌─────────┐ ┌─────────┐ ┌─────────┐ ┌─────────────────┐ │ │
│  │  │  SNS    │ │  Email  │ │  Slack  │ │  S3/CloudWatch  │ │ │
│  │  │ Notify  │ │ Notify  │ │ Notify  │ │     Logs        │ │ │
│  │  └─────────┘ └─────────┘ └─────────┘ └─────────────────┘ │ │
│  └────────────────────────────────────────────────────────────┘ │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## 快速开始

### 安装

```bash
# 使用 pip 安装
pip install c7n

# 安装云平台特定包
pip install c7n-aws    # AWS
pip install c7n-azure  # Azure
pip install c7n-gcp    # GCP
pip install c7n-kube   # Kubernetes

# 验证安装
custodian version
```

### 第一个策略

```yaml
# policy.yaml
policies:
  - name: ec2-find-untagged
    resource: ec2
    filters:
      - "tag:Environment": absent
    actions:
      - type: tag
        key: Environment
        value: Unknown
```

```bash
# 试运行（不执行操作）
custodian run --dryrun -s output policy.yaml

# 执行策略
custodian run -s output policy.yaml
```

---

## 策略示例

### 安全策略 - 公开 S3 存储桶

```yaml
policies:
  - name: s3-public-access
    resource: s3
    filters:
      - type: global-grants
    actions:
      - type: notify
        template: default.html
        subject: "Public S3 Bucket Detected"
        to:
          - security@example.com
        transport:
          type: sns
          topic: arn:aws:sns:us-east-1:123456789:security-alerts
```

### 成本优化 - 停止空闲 EC2

```yaml
policies:
  - name: ec2-stop-idle
    resource: ec2
    filters:
      - type: instance-age
        days: 30
      - type: metrics
        name: CPUUtilization
        statistics: Average
        days: 7
        value: 5
        op: less-than
    actions:
      - type: stop
      - type: notify
        template: default.html
        subject: "Idle EC2 Instance Stopped"
        to:
          - owner
        transport:
          type: sns
          topic: arn:aws:sns:us-east-1:123456789:cost-alerts
```

### 合规策略 - EBS 加密

```yaml
policies:
  - name: ebs-unencrypted
    resource: ebs
    filters:
      - Encrypted: false
    actions:
      - type: tag
        key: compliance-status
        value: unencrypted
      - type: notify
        template: default.html
        subject: "Unencrypted EBS Volume Found"
        to:
          - compliance@example.com
```

### 资源清理 - 删除旧快照

```yaml
policies:
  - name: ebs-old-snapshots
    resource: ebs-snapshot
    filters:
      - type: age
        days: 90
        op: greater-than
      - "tag:Permanent": absent
    actions:
      - delete
```

### Kubernetes 策略 - Pod 安全

```yaml
policies:
  - name: privileged-pods
    resource: k8s.pod
    filters:
      - type: value
        key: spec.containers[*].securityContext.privileged
        value: true
        op: contains
    actions:
      - type: event
        msg: "Privileged pod detected: {resource.metadata.name}"
```

---

## Azure 策略

```yaml
policies:
  - name: azure-vm-no-tags
    resource: azure.vm
    filters:
      - "tag:Owner": absent
    actions:
      - type: tag
        tag: Owner
        value: Unknown
```

## GCP 策略

```yaml
policies:
  - name: gcp-compute-old
    resource: gcp.instance
    filters:
      - type: age
        days: 365
        op: greater-than
    actions:
      - type: notify
        template: default.html
        to:
          - admin@example.com
```

---

## 执行模式

### 定时执行 (CloudWatch Events)

```yaml
policies:
  - name: ec2-tag-compliance
    resource: ec2
    mode:
      type: periodic
      schedule: "rate(1 day)"
      role: arn:aws:iam::123456789:role/CustodianRole
    filters:
      - "tag:Environment": absent
    actions:
      - type: mark-for-op
        op: stop
        days: 7
```

### 事件驱动 (CloudTrail)

```yaml
policies:
  - name: ec2-auto-tag-creator
    resource: ec2
    mode:
      type: cloudtrail
      events:
        - RunInstances
      role: arn:aws:iam::123456789:role/CustodianRole
    actions:
      - type: auto-tag-user
        tag: CreatorName
```

---

## 过滤器类型

| 过滤器 | 说明 |
|--------|------|
| value | 属性值匹配 |
| tag | 标签存在/值匹配 |
| age | 资源年龄 |
| metrics | CloudWatch 指标 |
| marked-for-op | 已标记操作 |
| cross-account | 跨账户关联 |
| event | 事件属性 |

---

## 输出配置

```bash
# 输出到 S3
custodian run -s s3://my-bucket/custodian-output policy.yaml

# 输出到本地
custodian run -s ./output policy.yaml
```

### 通知模板

```html
<!-- templates/default.html.j2 -->
<h2>Cloud Custodian Notification</h2>
<p>Policy: {{ policy.name }}</p>
<p>Account: {{ account_id }}</p>
<p>Region: {{ region }}</p>
<p>Resources affected: {{ resources | length }}</p>
<ul>
{% for resource in resources %}
  <li>{{ resource.id }} - {{ resource.Name }}</li>
{% endfor %}
</ul>
```

---

## 最佳实践

1. **先试运行**: 始终使用 `--dryrun` 验证策略
2. **渐进执行**: 先标记 (mark-for-op)，后执行操作
3. **细粒度权限**: 为 Custodian 创建最小权限 IAM 角色
4. **版本控制**: 策略文件纳入 Git 管理
5. **监控告警**: 配置策略执行结果通知

---

## 参考资源

- [官方文档](https://cloudcustodian.io/docs)
- [GitHub Repo](https://github.com/cloud-custodian/cloud-custodian)
- [策略示例](https://github.com/cloud-custodian/cloud-custodian/tree/main/docs/source/azure/examples)
- [过滤器参考](https://cloudcustodian.io/docs/filters.html)

---

**维护者**: Kudig Team | **许可证**: MIT
