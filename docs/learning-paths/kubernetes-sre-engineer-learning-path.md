---
title: "K8s运维工程师学习路线"
category: "learning-path"
tags: ["reliability", "cluster"]
created: "2026-05-23"
updated: "2026-05-23"
---

# K8s运维工程师学习路线

> 本学习路线面向专有云环境中的Kubernetes运维工程师，从入门到专家，分阶段构建完整的知识体系。

---

## 阶段一：基础构建（第1-2周）

**目标**：理解K8s核心概念，能够执行基础运维操作

### 1.1 Kubernetes核心概念
- [ ] Pod、Deployment、Service、ConfigMap、Secret基本概念
- [ ] 控制器模式：ReplicaSet、StatefulSet、DaemonSet、Job/CronJob
- [ ] 网络模型：ClusterIP、NodePort、LoadBalancer、Ingress
- [ ] 存储模型：PV、PVC、StorageClass、CSI

**推荐学习**：
- [[Kubernetes Core Concepts]]
- `domain-01-cluster-fundamentals/01-architecture-overview/`
- `domain-01-cluster-fundamentals/05-kubectl/`

### 1.2 kubectl基础操作
- [ ] 资源查看：get、describe、logs、top
- [ ] 资源管理：create、apply、delete、patch、edit
- [ ] 调试命令：exec、port-forward、debug
- [ ] 输出格式化：jsonpath、json、yaml、custom-columns

**练习清单**：
```bash
# 完成以下操作至少3次
kubectl get pods --all-namespaces
kubectl describe node <node-name>
kubectl logs <pod> --previous
kubectl exec -it <pod> -- /bin/sh
kubectl top pod -n <namespace>
```

### 1.3 阿里云ACK基础
- [ ] ACK集群类型（专有版、托管版、Serverless）
- [ ] ACK控制台基础操作
- [ ] 阿里云镜像仓库ACR使用
- [ ] 阿里云SLB与Ingress集成

**推荐学习**：
- `domain-12-cloud-providers/01-alibaba-cloud/02-ACK集群运维.md`

### 阶段一评估标准
> 能够独立完成以下操作即达标：
> - 部署一个Nginx Deployment并暴露Service
> - 查看Pod日志并进入容器执行命令
> - 使用kubectl排查简单的Pod启动失败问题

---

## 阶段二：运维实战（第3-4周）

**目标**：掌握常见问题的诊断和修复能力

### 2.1 节点管理
- [ ] 节点生命周期：加入、维护、移除
- [ ] 节点问题排查：NotReady、DiskPressure、MemoryPressure
- [ ] 污点与容忍度配置
- [ ] 节点亲和性和反亲和性

**推荐学习**：
- [[video-scripts/node-notready.md]]
- `domain-01-cluster-fundamentals/07-performance-tuning/`

### 2.2 应用管理
- [ ] Deployment滚动更新策略
- [ ] 就绪探针和存活探针配置
- [ ] 资源限制（requests/limits）调优
- [ ] HPA和VPA配置

**推荐学习**：
- [[domain-10-troubleshooting-diagnostics/topic-skills/08-deployment-rollout-failure.md]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/02-pod-crashloop-oomkilled.md]]
- k8s-autoscaling

### 2.3 存储与配置
- [ ] PVC生命周期管理
- [ ] StorageClass选择策略
- [ ] ConfigMap和Secret的更新机制
- [ ] 有状态应用运维（StatefulSet）

**推荐学习**：
- k8s-pvc-storage
- k8s-config-secret
- [[domain-10-troubleshooting-diagnostics/topic-skills/21-statefulset-failure.md]]

### 2.4 网络排查
- [ ] Service连通性诊断
- [ ] DNS解析问题排查
- [ ] NetworkPolicy配置和验证
- [ ] Ingress和SLB问题诊断

**推荐学习**：
- [[domain-10-troubleshooting-diagnostics/topic-skills/05-service-connectivity.md]]
- k8s-dns-failure
- [[domain-10-troubleshooting-diagnostics/topic-skills/13-ingress-gateway-failure.md]]

### 阶段二评估标准
> 能够独立完成以下操作即达标：
> - 诊断并修复节点NotReady问题
> - 处理Pod反复CrashLoopBackOff
> - 解决Service无法访问的问题
> - 配置HPA实现自动扩容

---

## 阶段三：深度诊断（第5-6周）

**目标**：掌握复杂问题的根因分析能力

### 3.1 控制平面运维
- [ ] apiserver/etcd/scheduler组件故障排查
- [ ] 证书生命周期管理
- [ ] 集群升级SOP
- [ ] etcd备份和恢复

**推荐学习**：
- [[domain-10-troubleshooting-diagnostics/topic-skills/11-control-plane-failure.md]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/06-certificate-expiry.md]]
- `domain-01-cluster-fundamentals/06-upgrade-paths/`

### 3.2 安全与合规
- [ ] RBAC权限模型和最小权限原则
- [ ] PodSecurityStandard配置
- [ ] Secret加密和密钥管理（KMS）
- [ ] 审计日志配置

**推荐学习**：
- [[domain-10-troubleshooting-diagnostics/topic-skills/09-rbac-quota-failure.md]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/18-security-incident-response.md]]
- `domain-05-security-compliance/`

### 3.3 可观测性体系
- [ ] Prometheus监控采集和告警规则
- [ ] Grafana仪表板配置
- [ ] 日志采集管道（Fluentd/Fluent Bit）
- [ ] 分布式链路追踪

**推荐学习**：
- [[domain-10-troubleshooting-diagnostics/topic-skills/15-monitoring-alerting-failure.md]]
- [[domain-10-troubleshooting-diagnostics/topic-skills/16-logging-pipeline-failure.md]]
- `domain-06-observability/`

### 3.4 性能优化
- [ ] CPU/内存瓶颈分析
- [ ] 网络性能调优
- [ ] 存储I/O优化
- [ ] 大规模集群性能考量

**推荐学习**：
- [[domain-10-troubleshooting-diagnostics/topic-skills/17-performance-bottleneck.md]]
- `domain-01-cluster-fundamentals/07-performance-tuning/`

### 阶段三评估标准
> 能够独立完成以下操作即达标：
> - 诊断控制平面组件故障并修复
> - 处理证书过期导致的集群不可用
> - 配置完整的监控告警体系
> - 进行性能瓶颈分析并提出优化方案

---

## 阶段四：专家进阶（第7-8周）

**目标**：具备设计、优化和故障预防能力

### 4.1 高可用设计
- [ ] 多可用区部署架构
- [ ] 控制平面高可用配置
- [ ] 有状态应用高可用（数据库、缓存）
- [ ] 灾难恢复方案设计

**推荐学习**：
- `domain-09-reliability-engineering/`
- `synthesis/高可用模式/`

### 4.2 多集群管理
- [ ] 多集群架构选型（联邦、GitOps、服务网格）
- [ ] 跨集群网络通信
- [ ] 多集群监控和告警
- [ ] 灾难恢复中的多集群切换

**推荐学习**：
- `domain-07-platform-engineering/`
- `synthesis/multi-cluster-observability-federation.md`

### 4.3 平台工程
- [ ] GitOps工作流设计（ArgoCD/Flux）
- [ ] 内部开发者平台（IDP）构建
- [ ] 成本优化和资源治理
- [ ] 标准化和模板化

**推荐学习**：
- `domain-07-platform-engineering/03-governance/`
- `domain-08-release-change-management/01-gitops/`

### 4.4 混沌工程
- [ ] 故障注入工具（Chaos Mesh、Litmus）
- [ ] 故障场景设计
- [ ] 韧性测试方法论
- [ ] 事后复盘（Post-mortem）

**推荐学习**：
- `domain-09-reliability-engineering/04-chaos-engineering/`
- `synthesis/chaos-drill-integration.md`

### 阶段四评估标准
> 能够独立完成以下操作即达标：
> - 设计一个三可用区高可用K8s架构
> - 制定完整的灾难恢复方案
> - 设计并执行一次混沌工程演练
> - 建立团队内部的GitOps工作流

---

## 阶段五：远程顾问专项（持续）

**目标**：掌握远程顾问模式下的诊断和沟通技巧

### 5.1 远程诊断方法论
- [ ] 信息收集清单标准化
- [ ] 命令替代方案设计
- [ ] 受限环境应对策略
- [ ] 升级决策框架

**推荐学习**：
- `domain-10-troubleshooting-diagnostics/topic-skills/skill-set/*/SKILL.md`
- `domain-10-troubleshooting-diagnostics/topic-skills/skill-set/*/DIALOGUE.md`

### 5.2 工单处理流程
- [ ] 工单分级（P0/P1/P2）
- [ ] 沟通话术和确认技巧
- [ ] 多问题并发诊断
- [ ] 事后复盘和知识沉淀

**推荐学习**：
- `domain-10-troubleshooting-diagnostics/topic-multi-fault-scenarios/`
- `synthesis/case-studies/`

### 5.3 阿里云专有云专项
- [ ] 阿里云专有云架构理解
- [ ] ACK集群运维（专有版/托管版）
- [ ] Terway网络排查
- [ ] 阿里云存储和SLB集成
- [ ] 阿里云安全（RAM/KMS）

**推荐学习**：
- `domain-12-cloud-providers/01-alibaba-cloud/`

---

## 学习资源索引

| 类型 | 路径 | 数量 |
|:---|:---|---:|
| 概念文档 | `concepts/` | 62 |
| 最佳实践 | `best-practices/` | 14 |
| 诊断Skill | `domain-10-troubleshooting-diagnostics/topic-skills/skill-set/*/` | 17 |
| 对话脚本 | `domain-10-troubleshooting-diagnostics/topic-skills/skill-set/*/DIALOGUE.md` | 17 |
| 合成分析 | `synthesis/` | 100+ |
| 案例研究 | `synthesis/case-studies/` | 36 |
| 阿里云文档 | `domain-12-cloud-providers/01-alibaba-cloud/` | 6 |
| 多问题并发 | `domain-10-troubleshooting-diagnostics/topic-multi-fault-scenarios/` | 10 |

---

## 认证建议

完成本学习路线后，建议考取以下认证：

1. **CKA (Certified Kubernetes Administrator)** — 基础运维认证
2. **CKS (Certified Kubernetes Security Specialist)** — 安全专项认证
3. **阿里云ACP (容器服务Kubernetes版)** — 阿里云ACK专项认证
4. **阿里云ACE (云计算架构师)** — 架构设计认证

---

## 学习进度追踪

复制以下内容到个人笔记，标记完成状态：

```
## 我的学习进度

### 阶段一：基础构建
- [ ] 1.1 Kubernetes核心概念
- [ ] 1.2 kubectl基础操作
- [ ] 1.3 阿里云ACK基础

### 阶段二：运维实战
- [ ] 2.1 节点管理
- [ ] 2.2 应用管理
- [ ] 2.3 存储与配置
- [ ] 2.4 网络排查

### 阶段三：深度诊断
- [ ] 3.1 控制平面运维
- [ ] 3.2 安全与合规
- [ ] 3.3 可观测性体系
- [ ] 3.4 性能优化

### 阶段四：专家进阶
- [ ] 4.1 高可用设计
- [ ] 4.2 多集群管理
- [ ] 4.3 平台工程
- [ ] 4.4 混沌工程

### 阶段五：远程顾问专项
- [ ] 5.1 远程诊断方法论
- [ ] 5.2 工单处理流程
- [ ] 5.3 阿里云专有云专项
```
