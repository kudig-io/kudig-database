---
title: Litmus 混沌工程实践
description: '# Litmus 混沌工程实践'
summary: 'kubectl apply -f https://litmuschaos.github.io/litmus/3.12.0/litmus-3.12.0.yaml'
category: domain
tags:
- litmus
- chaos-engineering
- kubernetes
- ci-cd
- agent
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- Litmus 混沌工程实践 是什么
- 如何 Litmus 混沌工程实践
- Kubernetes 09 reliability engineering 最佳实践
trigger_keywords:
- Litmus
- 混沌工程实践
- reliability
- engineering
prerequisites:
- kubectl-basics
- sre-practices
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# [[Litmus|Litmus]] 混沌工程实践

## Litmus vs Chaos Mesh

| 特性 | Litmus | Chaos Mesh |
|------|--------|-----------|
| 项目归属 | CNCF 孵化项目 | PingCAP |
| 实验编排 | [[Argo|Argo]]go Workflows|Argo Workflows]] 原生 | 自定义 Workflow |
| 多集群 | 原生支持 | 需额外配置 |
| GitOps | 原生支持 | 有限 |
| 社区 | 活跃，企业采用多 | 活跃，中国社区大 |

## 核心概念

```
Litmus 架构:
├── ChaosExperiment: 定义实验（问题类型、参数）
├── ChaosEngine: 将实验绑定到应用
├── ChaosResult: 实验结果
└── ChaosCenter: 控制平面（Web UI + API）
```

## 安装

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安装 ChaosCenter
kubectl apply -f https://litmuschaos.github.io/litmus/3.12.0/litmus-3.12.0.yaml

# 安装 ChaosAgent（目标集群）
litmusctl agent connect \
  --agent-name="prod-cluster" \
  --project-id="$PROJECT_ID" \
  --installation-mode="namespace" \
  --namespace="litmus"
```
## 实验示例

```yaml
# ChaosEngine: 对 nginx 注入 Pod 删除
apiVersion: litmuschaos.io/v1alpha1
kind: ChaosEngine
metadata:
  name: nginx-chaos
  namespace: default
spec:
  appinfo:
    appns: 'default'
    applabel: 'app=nginx'
    appkind: 'deployment'
  annotationCheck: 'true'
  engineState: 'active'
  chaosServiceAccount: pod-delete-sa
  experiments:
    - name: pod-delete
      spec:
        components:
          env:
            - name: TOTAL_CHAOS_DURATION
              value: '30'
            - name: CHAOS_INTERVAL
              value: '10'
            - name: FORCE
              value: 'false'
          probe:
            - name: "check-nginx-access"
              type: "httpProbe"
              mode: "Continuous"
              runProperties:
                probeTimeout: "5s"
                retry: 2
                interval: "5s"
                probePollingInterval: "2s"
                initialDelay: "2s"
              httpProbe/inputs:
                url: "http://nginx.default.svc.cluster.local"
                insecureSkipVerify: false
                method:
                  get:
                    criteria: "=="
                    responseCode: "200"
```

## GitOps 集成

```yaml
# 与 Argo CD 集成，自动执行混沌实验
apiVersion: argoproj.io/v1alpha1
kind: Workflow
metadata:
  name: chaos-pipeline
spec:
  entrypoint: chaos-tests
  templates:
    - name: chaos-tests
      steps:
        - - name: deploy-app
            template: deploy
        - - name: run-chaos
            template: litmus-experiment
        - - name: verify-slo
            template: slo-check
```

## 相关

- [[32-发布/package/2026-07-02_18-40/corpus/core/domain-09-reliability-engineering/03-chaos-engineering/01-chaos-engineering-overview|01 chaos engineering overview]]
- [[32-发布/package/2026-07-02_18-40/corpus/supporting/domain-09-reliability-engineering/06-chaos-engineering/01-chaos-mesh-deployment|02 chaos mesh deployment]]

```

<!-- risk-assessed -->
