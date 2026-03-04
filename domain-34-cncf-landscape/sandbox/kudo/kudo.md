# KUDO (Kubernetes Universal Declarative Operator)

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kudo.dev/ |
| **GitHub** | https://github.com/kudobuilder/kudo |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

KUDO 是一个构建 Kubernetes Operator 的声明式工具包，允许开发者仅使用 YAML 定义复杂的有状态应用生命周期管理逻辑，无需编写 Go 代码。它将 Operator 的常见模式（安装、升级、备份、恢复、扩缩容等）抽象为声明式 Plan，每个 Plan 由有序的 Phase 和 Step 组成，并支持参数化配置和模板渲染。

### 核心特性

- **纯 YAML Operator**: 仅使用 YAML 定义 Operator，无需编写代码
- **Plan 编排**: 将运维操作定义为 Plan → Phase → Step 的层级结构
- **参数系统**: 支持用户自定义参数，安装时可传入配置
- **模板渲染**: 使用 Go Template 生成 Kubernetes 资源清单
- **版本管理**: 内置 Operator 版本升级和回滚支持
- **Operator 仓库**: 通过仓库分发和安装预构建的 Operator

---

## 架构设计

```
┌─────────────────────────────────────────┐
│          KUDO Controller Manager         │
│                                          │
│  ┌──────────────────────────────┐       │
│  │    Operator CRD Watcher      │       │
│  │  (监听 Instance 变更)         │       │
│  └──────────┬───────────────────┘       │
│             │                            │
│  ┌──────────▼───────────────────┐       │
│  │      Plan Executor           │       │
│  │  Plan → Phase → Step         │       │
│  │  (有序编排执行)               │       │
│  └──────────┬───────────────────┘       │
│             │                            │
│  ┌──────────▼───────────────────┐       │
│  │    Template Engine            │       │
│  │  (Go Template + 参数渲染)     │       │
│  └──────────┬───────────────────┘       │
│             │                            │
│  ┌──────────▼───────────────────┐       │
│  │  Kubernetes Resource Manager  │       │
│  │  (创建/更新/删除资源)          │       │
│  └──────────────────────────────┘       │
└──────────────────────────────────────────┘

Operator 包结构:
├── operator.yaml          # Operator 元信息
├── params.yaml            # 参数定义
└── templates/
    ├── deployment.yaml    # K8s 资源模板
    ├── service.yaml
    └── statefulset.yaml
```

---

## 快速开始

### 安装 KUDO CLI

```bash
# 安装 kubectl-kudo 插件
brew install kudo-dev/tap/kudo-cli

# 或通过 Go 安装
go install github.com/kudobuilder/kudo/cmd/kubectl-kudo@latest

# 在集群中安装 KUDO Controller
kubectl kudo init
```

### 安装 Operator

```bash
# 从官方仓库安装 Kafka Operator
kubectl kudo install kafka

# 指定参数安装
kubectl kudo install kafka \
  --instance=my-kafka \
  -p BROKER_COUNT=5 \
  -p DISK_SIZE=100Gi \
  -p BROKER_MEM=4096m

# 查看安装状态
kubectl kudo plan status --instance=my-kafka
```

### 创建自定义 Operator

```yaml
# operator.yaml
apiVersion: kudo.dev/v1beta1
name: my-database
operatorVersion: 0.1.0
appVersion: 8.0.0
kubernetesVersion: 1.25.0
maintainers:
  - name: Team
    email: team@example.com
plans:
  deploy:
    strategy: serial
    phases:
      - name: main
        strategy: parallel
        steps:
          - name: deploy-master
            tasks:
              - master
          - name: deploy-replicas
            tasks:
              - replicas
      - name: init
        strategy: serial
        steps:
          - name: initialize-db
            tasks:
              - init-job
  backup:
    strategy: serial
    phases:
      - name: backup-phase
        steps:
          - name: run-backup
            tasks:
              - backup-job
tasks:
  - name: master
    kind: Apply
    spec:
      resources:
        - master-statefulset.yaml
        - master-service.yaml
  - name: replicas
    kind: Apply
    spec:
      resources:
        - replica-statefulset.yaml
  - name: init-job
    kind: Apply
    spec:
      resources:
        - init-job.yaml
  - name: backup-job
    kind: Apply
    spec:
      resources:
        - backup-job.yaml
```

### 参数定义

```yaml
# params.yaml
apiVersion: kudo.dev/v1beta1
parameters:
  - name: REPLICAS
    description: "Number of database replicas"
    default: "3"
    displayName: "Replica Count"
  - name: MEMORY
    description: "Memory per instance"
    default: "2Gi"
  - name: STORAGE_SIZE
    description: "PVC size for data"
    default: "50Gi"
  - name: BACKUP_SCHEDULE
    description: "Cron schedule for backups"
    default: "0 2 * * *"
```

### 资源模板

```yaml
# templates/master-statefulset.yaml
apiVersion: apps/v1
kind: StatefulSet
metadata:
  name: {{ .Name }}-master
  namespace: {{ .Namespace }}
spec:
  replicas: 1
  selector:
    matchLabels:
      app: {{ .Name }}
      role: master
  template:
    spec:
      containers:
        - name: database
          image: mydb:{{ .Params.APP_VERSION }}
          resources:
            requests:
              memory: {{ .Params.MEMORY }}
          volumeMounts:
            - name: data
              mountPath: /var/lib/data
  volumeClaimTemplates:
    - metadata:
        name: data
      spec:
        resources:
          requests:
            storage: {{ .Params.STORAGE_SIZE }}
```

---

## Operator 生命周期

```bash
# 升级 Operator 版本
kubectl kudo upgrade my-database --operator-version=0.2.0

# 更新参数
kubectl kudo update --instance=my-db -p REPLICAS=5

# 触发备份 Plan
kubectl kudo plan trigger --name=backup --instance=my-db

# 查看 Plan 执行状态
kubectl kudo plan status --instance=my-db
```

---

## 与其他方案对比

| 特性 | KUDO | Operator SDK | Helm | Crossplane |
|:---|:---|:---|:---|:---|
| 编程语言 | YAML 仅 | Go/Ansible | YAML (模板) | Go + YAML |
| 学习曲线 | 低 | 高 | 低 | 中 |
| Day-2 运维 | Plan 编排 | 自定义控制器 | 有限 | 有限 |
| 版本升级 | 内置 | 自定义 | 内置 | 内置 |
| 有状态应用 | 优秀 | 优秀 | 一般 | 一般 |
| 社区生态 | 中等 | 丰富 | 非常丰富 | 丰富 |

---

## 最佳实践

1. **Plan 设计**: 为每个运维操作（备份、恢复、扩容）定义独立的 Plan
2. **参数化**: 将所有可变配置抽象为参数，提供合理默认值
3. **串行/并行**: 有依赖关系的步骤使用 serial，无依赖的使用 parallel 提速
4. **健康检查**: 在 Step 之间加入健康检查任务，确保前置条件满足
5. **版本策略**: 遵循语义版本控制，确保 Operator 升级向后兼容

---

## 参考资源

- [KUDO 官方文档](https://kudo.dev/docs/)
- [KUDO GitHub](https://github.com/kudobuilder/kudo)
- [KUDO Operators 仓库](https://github.com/kudobuilder/operators)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
