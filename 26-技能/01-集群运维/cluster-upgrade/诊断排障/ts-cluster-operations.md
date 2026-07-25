---
title: 集群运维故障排查
description: '# 集群运维故障排查'
summary: '1. **版本与倾斜**：`kubectl version --short` + `kubeadm upgrade plan`，确认版本跨度与倾斜策略。'
category: skills
tags:
- k8s
- troubleshooting
- structural
- cluster-operations
- etcd
- kubelet
- scheduler
- prometheus
- grafana
- helm
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 集群运维故障排查 是什么
- 如何 集群运维故障排查
trigger_keywords:
- 集群运维故障排查
prerequisites:
- kubectl-basics
- helm-basics
- prometheus-basics
- monitoring-basics
- etcd-basics
- logging-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# 集群运维故障排查

### 01 Cluster Maintenance Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **版本与倾斜**：`kubectl version --short` + `kubeadm upgrade plan`，确认版本跨度与倾斜策略。
2. **控制面健康**：`kubectl get --raw /readyz?verbose`，定位卡在 etcd/认证/聚合 API 的环节。
3. **etcd 状态**：`etcdctl endpoint health --cluster`，确认有 Leader 且无高延迟。
4. **节点维护阻塞**：`kubectl drain <node> --ignore-daemonsets --delete-emptydir-data` 看阻塞原因（PDB/本地存储）。
5. **证书到期**：`kubeadm certs check-expiration`，若临近到期先续期。
6. **快速缓解**：
   - 控制面异常先恢复 API 可用，再做升级。
   - 对升级失败节点回滚到稳定版本后再排查。
7. **证据留存**：保存 upgrade plan、/readyz 输出、etcd 状态与节点事件。

#### 排查方法与步骤



#### 排查决策树

```
集群运维问题
    │
    ├─► 升级问题
    │       │
    │       ├─► 升级前检查失败
    │       │       │
    │       │       ├─► 版本跨度过大 ──► 分步升级
    │       │       ├─► etcd 不健康 ──► 修复 etcd
    │       │       ├─► 证书即将过期 ──► 先续期证书
    │       │       └─► 废弃 API 使用 ──► 迁移 API
    │       │
    │       ├─► 控制平面升级失败
    │       │       │
    │       │       ├─► 组件启动失败 ──► 检查配置和日志
    │       │       ├─► 镜像拉取失败 ──► 检查镜像仓库
    │       │       └─► 配置不兼容 ──► 更新配置文件
    │       │
    │       └─► 节点升级失败
    │               │
    │               ├─► kubelet 启动失败 ──► 检查配置
    │               ├─► 版本不匹配 ──► 检查版本倾斜
    │               └─► 证书问题 ──► 重新生成证书
    │
    ├─► 节点管理问题
    │       │
    │       ├─► drain 卡住
    │       │       │
    │       │       ├─► PDB 阻止 ──► 检查/调整 PDB
    │       │       ├─► 本地存储 ──► 添加 --delete-emptydir-data
    │       │       ├─► DaemonSet Pod ──► 添加 --ignore-daemonsets
    │       │       └─► Finalizer 阻塞 ──► 检查 Finalizer
    │       │
    │       ├─► 节点加入失败
    │       │       │
    │       │       ├─► Token 过期 ──► 生成新 Token
    │       │       ├─► 网络不通 ──► 检查网络
    │       │       └─► 端口冲突 ──► 检查端口占用
    │       │
    │       └─► 节点 NotReady
    │               │
    │               ├─► kubelet 未运行 ──► 启动 kubelet
    │               ├─► 容器运行时问题 ──► 检查 containerd
    │               └─► 网络问题 ──► 检查节点网络
    │
    └─► 备份恢复问题
            │
            ├─► 备份失败 ──► 检查磁盘空间和权限
            │
            └─► 恢复失败
                    │
                    ├─► 数据目录存在 ──► 清理旧目录
    
...(截断)

---

### 02 Logging Monitoring Troubleshooting

#### 0. 10 分钟快速诊断

1. **采集链路**：确认采集器 DaemonSet/Agent Running（Fluent Bit/Promtail/Vector）。
2. **后端健康**：ES/Loki/Prometheus 的就绪与存储空间是否正常。
3. **目标抓取**：Prometheus Targets 中是否大量 `down`，检查 ServiceMonitor/Endpoint。
4. **告警链路**：AlertManager 是否正常接收与路由，Grafana 数据源可用。
5. **资源压力**：采集器/Prometheus OOM 或磁盘告警时优先扩容或限速。
6. **快速缓解**：
   - 降低日志采集粒度或采样。
   - 临时扩副本并提升资源请求。
7. **证据留存**：保存采集器日志、Targets 状态、后端健康与磁盘使用。

#### 排查方法与步骤



#### 排查决策树

```
日志/监控问题
      │
      ├─── 日志问题？
      │         │
      │         ├─ 日志丢失 ──→ 检查采集器状态/配置/后端
      │         ├─ 日志延迟 ──→ 检查队列/后端性能/网络
      │         ├─ 采集器崩溃 ──→ 检查资源/配置/权限
      │         └─ 格式错误 ──→ 检查解析器配置
      │
      ├─── 监控问题？
      │         │
      │         ├─ 指标缺失 ──→ 检查服务发现/抓取配置
      │         ├─ Prometheus 慢 ──→ 检查资源/查询/存储
      │         ├─ 告警问题 ──→ 检查规则/AlertManager
      │         └─ Grafana 无数据 ──→ 检查数据源/查询
      │
      └─── 存储问题？
                │
                ├─ ES/Loki 问题 ──→ 检查集群状态/存储
                └─ 磁盘空间不足 ──→ 清理/扩容
```
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
---

### 03 Helm Troubleshooting

#### 0. 10 分钟快速诊断

1. **Release 状态**：`helm list -A` 与 `helm status <release>`，确认是否 pending/failed。
2. **模板渲染**：`helm template <release> <chart> --debug` 复现渲染错误。
3. **资源冲突**：`kubectl get events -n <ns>`，查看是否已有资源冲突或 RBAC 失败。
4. **Hooks 卡住**：`helm get hooks <release>`，排查 pre/post hook 阻塞。
5. **超时与回滚**：必要时 `helm rollback <release> <rev>` 恢复服务。
6. **快速缓解**：
   - 增大 `--timeout`，或使用 `--atomic` 防止半成品。
   - 清理卡住的 pending release secret。
7. **证据留存**：保存渲染输出、release 详情与相关事件。

#### 排查方法与步骤



#### 排查决策树

```
Helm 部署问题
      │
      ├─── 安装/升级失败？
      │         │
      │         ├─ 模板错误 ──→ helm template 检查
      │         ├─ 资源错误 ──→ 检查 Kubernetes 资源状态
      │         ├─ 权限不足 ──→ 检查 RBAC
      │         └─ 超时 ──→ 增加超时/检查资源状态
      │
      ├─── Release 状态异常？
      │         │
      │         ├─ pending 状态 ──→ 检查后台操作/手动修复
      │         ├─ failed 状态 ──→ 分析失败原因/重新部署
      │         └─ 无法卸载 ──→ 检查 hooks/finalizers
      │
      └─── 配置未生效？
                │
                ├─ values 优先级 ──→ 检查 --set/-f 顺序
                ├─ 模板渲染 ──→ helm get manifest 检查
                └─ 缓存问题 ──→ helm repo update
```
# 🟢 低风险：只读/信息收集，通常无副作用
---

### 04 Ha Disaster Recovery Troubleshooting

#### 0. 10 分钟快速诊断

1. **API 可达性**：`kubectl get --raw /readyz?verbose` 与 LB 健康检查。
2. **etcd 健康**：`etcdctl endpoint status/health --cluster` 检查 Leader/延迟。
3. **控制面副本**：确认多个 API Server/Controller/Scheduler 均存活。
4. **备份可用性**：检查最近一次 etcd 快照是否成功、存储可读。
5. **证书有效性**：检查控制面/etcd 证书过期。
6. **快速缓解**：
   - 先恢复 LB 与 etcd 可用，再处理控制面。
   - 若多数 etcd 节点不可用，准备冷备恢复。
7. **证据留存**：保存 etcd 状态、LB 健康、快照列表与关键日志。

#### 排查方法与步骤



#### 排查决策树

```
高可用/灾备问题
      │
      ├─── API Server 不可用？
      │         │
      │         ├─ 检查负载均衡器 ──→ VIP/LB 健康检查
      │         ├─ 检查各 Master 节点 ──→ kubectl --server 直连
      │         └─ 检查证书 ──→ 证书过期/配置错误
      │
      ├─── etcd 集群问题？
      │         │
      │         ├─ 无 Leader ──→ 检查节点数/网络分区
      │         ├─ 数据不一致 ──→ 检查成员状态/日志
      │         └─ 性能问题 ──→ 检查磁盘/网络延迟
      │
      ├─── Controller/Scheduler 问题？
      │         │
      │         ├─ 选主失败 ──→ 检查 Lease 资源
      │         └─ 多个 Leader ──→ 检查时钟同步
      │
      └─── 备份/恢复问题？
                │
                ├─ 备份失败 ──→ 检查权限/存储空间
                └─ 恢复失败 ──→ 检查备份完整性/版本兼容
```
# 🟢 低风险：只读/信息收集，通常无副作用
---

### 05 Crd Operator Troubleshooting

#### 0. 10 分钟快速诊断

1. **CRD 是否存在**：`kubectl get crd | grep <kind>`，确认版本与资源可用。
2. **Webhook 健康**：`kubectl get validatingwebhookconfigurations`，检查超时/证书问题。
3. **Operator 存活**：`kubectl get pods -n <operator-ns>`，查看重启与日志错误。
4. **Reconcile 失败**：`kubectl logs <operator-pod>`，检索 requeue/error。
5. **Finalizer 卡住**：资源 Terminating 时查看 `metadata.finalizers`。
6. **快速缓解**：
   - 回滚最近的 CR/CRD 变更。
   - 临时禁用 webhook（谨慎）以恢复核心操作。
7. **证据留存**：保存 CRD/CR 状态、Operator 日志与 webhook 配置。

#### 排查方法与步骤



#### 排查决策树

```
CRD/Operator 问题
        │
        ▼
┌───────────────────────┐
│  问题发生在哪个阶段？  │
└───────────────────────┘
        │
        ├── CR 创建/更新失败 ───────────────────────────────┐
        │                                                    │
        │   ┌─────────────────────────────────────────┐     │
        │   │ kubectl apply -f cr.yaml 报错?          │     │
        │   └─────────────────────────────────────────┘     │
        │                  │                                 │
        │                  ▼                                 │
        │   ┌─────────────────────────────────────────┐     │
        │   │ "no matches for kind"?                  │     │
        │   └─────────────────────────────────────────┘     │
        │          │                │                        │
        │         是               否                        │
        │          │                │                        │
        │          ▼                ▼                        │
        │   ┌────────────┐   ┌────────────────┐             │
        │   │ CRD 未安装 │   │ "admission     │             │
        │   │ 或版本错误 │   │ webhook denied"│             │
        │   └────────────┘   └────────────────┘             │
        │                           │                        │
        │                           ▼                        │
        │                    ┌────────────┐                 │
        │                    │ Webhook    │                 │
        │                 
...(截断)

---

### 06 Kustomize Troubleshooting

#### 0. 10 分钟快速诊断

1. **构建验证**：`kustomize build <path>`，先确认构建能通过。
2. **版本一致性**：`kustomize version` 与 `kubectl version --client` 的内置版本差异。
3. **路径与资源**：检查 `resources`/`bases`/`patches` 路径是否正确。
4. **Patch 命中**：遇到 `no resources matched` 优先检查 `target` 选择器。
5. **快速缓解**：
   - 先 `--dry-run=client` 验证输出。
   - 临时移除复杂 patch 逐步定位。
6. **证据留存**：保存构建输出与报错日志。

#### 排查方法与步骤



#### 排查决策树

```
Kustomize 问题
        │
        ▼
┌───────────────────────┐
│  问题发生在哪个阶段？  │
└───────────────────────┘
        │
        ├── kustomize build 失败 ────────────────────────────┐
        │                                                     │
        │   ┌─────────────────────────────────────────┐      │
        │   │ 错误类型是什么?                         │      │
        │   └─────────────────────────────────────────┘      │
        │          │                                          │
        │          ├── YAML 语法错误 ──► 检查 YAML 格式      │
        │          │                                          │
        │          ├── 文件未找到 ──► 检查路径和文件名        │
        │          │                                          │
        │          ├── Patch 目标未找到 ──► 检查 target       │
        │          │                                          │
        │          └── Generator 错误 ──► 检查生成器配置      │
        │                                                     │
        ├── build 成功但输出不符合预期 ──────────────────────┤
        │                                                     │
        │   ┌─────────────────────────────────────────┐      │
        │   │ kustomize build 检查输出                │      │
        │   └─────────────────────────────────────────┘      │
        │          │                                          │
        │          ├── Patch 未应用 ──► 检查 patch 匹配      │
        │          │                                          │
        │          ├── 字段被意外覆盖 ──► 检查合并策略       │
   
...(截断)

## 相关链接

- [[23-实体/15-参考与索引/k8s-knowledge-map.md|K8s 知识图谱]]
- [[26-技能/04-工作负载/pod/方法论/FTA Methodology and Core Principles.md|FTA 方法论]]

## Related

- [[containerd]] — containerd
- [[helm]] — Helm
- [[etcd]] — etcd
- [[prometheus]] — Prometheus
- [[kubernetes]] — Kubernetes (CNCF Graduated)


<!-- risk-assessed -->
