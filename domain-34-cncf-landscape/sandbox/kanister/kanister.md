# Kanister

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://kanister.io/ |
| **GitHub** | https://github.com/kanisterio/kanister |
| **许可证** | Apache-2.0 |
| **开发语言** | Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Kanister 是一个面向 Kubernetes 的应用级数据管理框架，专门用于有状态应用（数据库、消息队列等）的备份和恢复。它使用 Blueprint CRD 定义应用特定的备份/恢复操作流程，支持应用一致性的快照和备份。Kanister 可以与应用的数据保护 API（如 PostgreSQL pg_dump、MongoDB mongodump）深度集成。

### 核心特性

- **应用感知**: 针对不同数据库/应用定义专属的备份恢复流程
- **Blueprint**: 声明式定义备份、恢复、删除等数据管理操作
- **ActionSet**: 追踪每次备份/恢复操作的状态和输出
- **多存储后端**: 支持 S3、GCS、Azure Blob 等对象存储作为备份目标
- **Kanctl CLI**: 提供 CLI 工具简化操作管理
- **预置 Blueprint**: 内置 PostgreSQL、MySQL、MongoDB、Elasticsearch 等常用 Blueprint

---

## 架构设计

```
┌──────────────────────────────────────────────────┐
│            Kubernetes Cluster                      │
│                                                    │
│  ┌────────────────────────────────────────────┐   │
│  │        Kanister Controller                  │   │
│  │  (监听 ActionSet / 执行 Blueprint)          │   │
│  └──────────────────┬─────────────────────────┘   │
│                     │                              │
│  ┌──────────────────▼─────────────────────────┐   │
│  │  Blueprint (备份/恢复流程定义)               │   │
│  │  ┌──────────┐ ┌──────────┐ ┌───────────┐  │   │
│  │  │ backup   │ │ restore  │ │ delete     │  │   │
│  │  │ Action   │ │ Action   │ │ Action     │  │   │
│  │  └──────────┘ └──────────┘ └───────────┘  │   │
│  └────────────────────────────────────────────┘   │
│                     │                              │
│  ┌──────────────────▼─────────────────────────┐   │
│  │  ActionSet (操作实例/状态追踪)               │   │
│  └──────────────────┬─────────────────────────┘   │
│                     │                              │
│  ┌──────────────────▼─────────────────────────┐   │
│  │     Kanister Function Pod                   │   │
│  │  (执行 pg_dump / mongodump 等)              │   │
│  └──────────────────┬─────────────────────────┘   │
└─────────────────────┼──────────────────────────────┘
                      │
               ┌──────▼──────┐
               │ Object Store│
               │ S3/GCS/Azure│
               └─────────────┘
```

---

## 快速开始

### 安装

```bash
# 安装 Kanister Controller
helm repo add kanister https://charts.kanister.io/
helm install kanister kanister/kanister-operator \
  --namespace kanister \
  --create-namespace

# 安装 kanctl CLI
curl -LO https://github.com/kanisterio/kanister/releases/latest/download/kanctl_linux_amd64
chmod +x kanctl_linux_amd64
sudo mv kanctl_linux_amd64 /usr/local/bin/kanctl
```

### PostgreSQL Blueprint

```yaml
apiVersion: cr.kanister.io/v1alpha1
kind: Blueprint
metadata:
  name: postgresql-blueprint
actions:
  backup:
    outputArtifacts:
      pgBackup:
        keyValue:
          s3path: "{{ .Phases.pgDump.Output.s3path }}"
    phases:
      - func: KubeTask
        name: pgDump
        objects:
          pgSecret:
            kind: Secret
            name: "{{ .StatefulSet.Name }}-postgresql"
            namespace: "{{ .StatefulSet.Namespace }}"
        args:
          image: ghcr.io/kanisterio/postgres-kanister-tools:0.100.0
          namespace: "{{ .StatefulSet.Namespace }}"
          command:
            - bash
            - -o
            - errexit
            - -c
            - |
              export PGHOST="{{ .StatefulSet.Name }}-postgresql.{{ .StatefulSet.Namespace }}.svc.cluster.local"
              export PGUSER="postgres"
              export PGPASSWORD="{{ index .Phases.pgDump.Secrets.pgSecret.Data "postgres-password" | toString }}"
              S3_PATH="s3://{{ .Profile.Location.Bucket }}/backups/{{ .StatefulSet.Namespace }}/{{ .StatefulSet.Name }}/{{ toDate "2006-01-02T15:04:05.999999999Z07:00" .Time | date "2006-01-02T15-04-05" }}"
              pg_dumpall --clean | kando location push --profile '{{ toJson .Profile }}' --path "${S3_PATH}/dump.sql" -
              kando output s3path "${S3_PATH}"

  restore:
    inputArtifactNames:
      - pgBackup
    phases:
      - func: KubeTask
        name: pgRestore
        objects:
          pgSecret:
            kind: Secret
            name: "{{ .StatefulSet.Name }}-postgresql"
            namespace: "{{ .StatefulSet.Namespace }}"
        args:
          image: ghcr.io/kanisterio/postgres-kanister-tools:0.100.0
          namespace: "{{ .StatefulSet.Namespace }}"
          command:
            - bash
            - -o
            - errexit
            - -c
            - |
              export PGHOST="{{ .StatefulSet.Name }}-postgresql.{{ .StatefulSet.Namespace }}.svc.cluster.local"
              export PGUSER="postgres"
              export PGPASSWORD="{{ index .Phases.pgRestore.Secrets.pgSecret.Data "postgres-password" | toString }}"
              kando location pull --profile '{{ toJson .Profile }}' --path "{{ .ArtifactsIn.pgBackup.KeyValue.s3path }}/dump.sql" - | psql -q

  delete:
    inputArtifactNames:
      - pgBackup
    phases:
      - func: KubeTask
        name: deleteBackup
        args:
          image: ghcr.io/kanisterio/postgres-kanister-tools:0.100.0
          namespace: "{{ .StatefulSet.Namespace }}"
          command:
            - bash
            - -c
            - |
              kando location delete --profile '{{ toJson .Profile }}' --path "{{ .ArtifactsIn.pgBackup.KeyValue.s3path }}"
```

### 执行备份

```bash
# 配置存储 Profile
kanctl create profile s3compliant \
  --access-key $AWS_ACCESS_KEY_ID \
  --secret-key $AWS_SECRET_ACCESS_KEY \
  --bucket my-backups \
  --region us-east-1 \
  --namespace kanister

# 执行备份
kanctl create actionset \
  --action backup \
  --namespace kanister \
  --blueprint postgresql-blueprint \
  --statefulset default/my-postgres \
  --profile kanister/s3-profile

# 查看备份状态
kubectl get actionsets -n kanister

# 执行恢复
kanctl create actionset \
  --action restore \
  --namespace kanister \
  --from <backup-actionset-name>
```

---

## 与其他方案对比

| 特性 | Kanister | Velero | K8up | 原生工具 |
|:---|:---|:---|:---|:---|
| 备份粒度 | 应用级 | 集群/命名空间 | 命名空间 | 应用级 |
| 应用一致性 | Blueprint 定义 | 通过 Hook | 通过 Hook | 手动 |
| 自定义流程 | Blueprint | 有限 | 有限 | 脚本 |
| PV 备份 | 通过工具 | CSI 快照 | Restic | 手动 |
| 适用场景 | 有状态应用 | 灾难恢复 | 通用备份 | 特定应用 |

---

## 最佳实践

1. **Blueprint 测试**: 在非生产环境充分测试 Blueprint 的备份和恢复流程
2. **定期备份**: 结合 CronJob 或外部调度器定期创建 ActionSet 执行备份
3. **恢复演练**: 定期执行恢复演练，确保备份数据可用
4. **清理策略**: 设置备份保留策略，定期清理过期的备份数据
5. **监控告警**: 监控 ActionSet 状态，对失败的备份/恢复操作设置告警

---

## 参考资源

- [Kanister 官方文档](https://docs.kanister.io/)
- [Kanister GitHub](https://github.com/kanisterio/kanister)
- [预置 Blueprint](https://github.com/kanisterio/kanister/tree/master/examples)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
