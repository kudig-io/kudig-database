---
title: Helm Hooks 生命周期机制
description: 'pre/post-install/upgrade/rollback/delete hooks、helm test hook、hook 删除策略与权重排序'
summary: 'pre/post-install/upgrade/rollback/delete hooks、helm test hook、hook 删除策略与权重排序'
category: manifests-patterns
tags:
- helm
- hooks
- lifecycle
- test
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 15min
intent_queries:
- Helm Hooks 是什么
- 如何使用 Helm Hooks
- Helm hook 删除策略如何配置
trigger_keywords:
- helm
- hooks
- pre-install
- post-install
- lifecycle
- test
prerequisites:
- kubectl-basics
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

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Helm Hooks 生命周期机制

## 1. Hook 生命周期

Helm 在 Chart 生命周期的特定阶段执行 Hook：

```
安装(install)：
  pre-install → 安装资源 → post-install

升级(upgrade)：
  pre-upgrade → 升级资源 → post-upgrade

回滚(rollback)：
  pre-rollback → 回滚资源 → post-rollback

删除(delete)：
  pre-delete → 删除资源 → post-delete

测试(test)：
  test（多次执行，直到成功或超时）
```

## 2. Hook 类型详解

### 2.1 pre-install / post-install

```yaml
# pre-install: 数据库迁移
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-db-migrate
  annotations:
    "helm.sh/hook": pre-install
    "helm.sh/hook-weight": "-5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: db-migrate
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          command: ["python", "manage.py", "migrate"]
          env:
            - name: DATABASE_URL
              valueFrom:
                secretKeyRef:
                  name: {{ .Values.database.existingSecret }}
                  key: url
```

```yaml
# post-install: 初始化数据
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-seed-data
  annotations:
    "helm.sh/hook": post-install
    "helm.sh/hook-weight": "5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: seed-data
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          command: ["python", "manage.py", "seed"]
```

### 2.2 pre-upgrade / post-upgrade

```yaml
# pre-upgrade: 备份数据库
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-backup
  annotations:
    "helm.sh/hook": pre-upgrade
    "helm.sh/hook-weight": "-10"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: backup
          image: postgres:15
          command:
            - /bin/bash
            - -c
            - |
              pg_dump -h $DB_HOST -U $DB_USER $DB_NAME > /backup/dump.sql
          env:
            - name: DB_HOST
              value: {{ .Values.database.host }}
            - name: DB_USER
              valueFrom:
                secretKeyRef:
                  name: {{ .Values.database.existingSecret }}
                  key: username
            - name: DB_NAME
              value: {{ .Values.database.name }}
          volumeMounts:
            - name: backup-volume
              mountPath: /backup
      volumes:
        - name: backup-volume
          persistentVolumeClaim:
            claimName: {{ include "myapp.fullname" . }}-backup
```

```yaml
# post-upgrade: 清理缓存
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-cache-clear
  annotations:
    "helm.sh/hook": post-upgrade
    "helm.sh/hook-weight": "5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: cache-clear
          image: redis:7
          command: ["redis-cli", "-h", "{{ .Values.redis.host }}", "FLUSHDB"]
```

### 2.3 pre-rollback / post-rollback

```yaml
# pre-rollback: 验证回滚条件
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-pre-rollback
  annotations:
    "helm.sh/hook": pre-rollback
    "helm.sh/hook-weight": "-5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: pre-rollback
          image: bitnami/kubectl:latest
          command:
            - /bin/bash
            - -c
            - |
              echo "验证回滚条件..."
              # 检查是否有正在进行的迁移
              # 检查是否有活跃连接
              echo "回滚条件满足"
```

### 2.4 pre-delete

```yaml
# pre-delete: 清理资源
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-cleanup
  annotations:
    "helm.sh/hook": pre-delete
    "helm.sh/hook-weight": "-5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: cleanup
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          command: ["python", "manage.py", "cleanup"]
```

### 2.5 多 Hook 组合

```yaml
# 同一资源多个 Hook
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-migrate
  annotations:
    "helm.sh/hook": pre-install,pre-upgrade
    "helm.sh/hook-weight": "-5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: migrate
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          command: ["python", "manage.py", "migrate"]
```

## 3. Hook 删除策略

### 3.1 策略类型

```yaml
annotations:
  # 策略 1: Hook 成功后删除（默认）
  "helm.sh/hook-delete-policy": hook-succeeded

  # 策略 2: Hook 失败后删除
  "helm.sh/hook-delete-policy": hook-failed

  # 策略 3: 无论成功失败都删除
  "helm.sh/hook-delete-policy": before-hook-creation

  # 策略 4: Hook 完成后删除（成功或失败）
  "helm.sh/hook-delete-policy": hook-succeeded,hook-failed

  # 策略 5: 手动管理（不自动删除）
  "helm.sh/hook-delete-policy": ""
```

### 3.2 策略对比

| 策略 | 成功时 | 失败时 | 适用场景 |
|------|--------|--------|----------|
| `hook-succeeded` | 删除 | 保留 | 数据库迁移、种子数据 |
| `hook-failed` | 保留 | 删除 | 调试、日志收集 |
| `before-hook-creation` | 删除 | 删除 | 临时任务 |
| `hook-succeeded,hook-failed` | 删除 | 删除 | 清理临时资源 |
| `""` | 保留 | 保留 | 需要手动管理 |

### 3.3 策略最佳实践

```yaml
# 推荐：数据库迁移使用 hook-succeeded
annotations:
  "helm.sh/hook": pre-install,pre-upgrade
  "helm.sh/hook-delete-policy": hook-succeeded

# 推荐：健康检查使用 hook-succeeded
annotations:
  "helm.sh/hook": post-install,post-upgrade
  "helm.sh/hook-delete-policy": hook-succeeded

# 推荐：调试/日志使用 hook-failed
annotations:
  "helm.sh/hook": test
  "helm.sh/hook-delete-policy": hook-failed
```

## 4. Hook 权重排序

### 4.1 权重机制

```yaml
# 权重控制执行顺序（数值越小越先执行）
annotations:
  "helm.sh/hook-weight": "-5"    # 先执行
  "helm.sh/hook-weight": "0"     # 默认
  "helm.sh/hook-weight": "5"     # 后执行
  "helm.sh/hook-weight": "10"    # 最后执行
```

### 4.2 权重使用场景

```yaml
# 场景：多个 Hook 按顺序执行
---
# Hook 1: 创建数据库
apiVersion: v1
kind: Job
metadata:
  name: create-db
  annotations:
    "helm.sh/hook": pre-install
    "helm.sh/hook-weight": "-10"
spec:
  template:
    spec:
      containers:
        - name: create-db
          image: postgres:15
          command: ["createdb", "-h", "postgres", "mydb"]
---
# Hook 2: 运行迁移
apiVersion: batch/v1
kind: Job
metadata:
  name: migrate-db
  annotations:
    "helm.sh/hook": pre-install
    "helm.sh/hook-weight": "-5"
spec:
  template:
    spec:
      containers:
        - name: migrate
          image: my-app:latest
          command: ["python", "manage.py", "migrate"]
---
# Hook 3: 种子数据
apiVersion: batch/v1
kind: Job
metadata:
  name: seed-data
  annotations:
    "helm.sh/hook": pre-install
    "helm.sh/hook-weight": "0"
spec:
  template:
    spec:
      containers:
        - name: seed
          image: my-app:latest
          command: ["python", "manage.py", "seed"]
```

### 4.3 权重最佳实践

```yaml
# 权重分配策略
-10 ~ -1:  前置准备（创建资源、初始化）
0:         主要操作（迁移、配置）
1 ~ 10:    后置操作（清理、验证）
10+:       最终步骤（通知、报告）
```

## 5. Helm Test Hook

### 5.1 基础测试

```yaml
# tests/test-connection.yaml
apiVersion: v1
kind: Pod
metadata:
  name: {{ include "myapp.fullname" . }}-test-connection
  annotations:
    "helm.sh/hook": test
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  restartPolicy: Never
  containers:
    - name: curl
      image: curlimages/curl:latest
      command:
        - /bin/sh
        - -c
        - |
          curl -f http://{{ include "myapp.fullname" . }}:{{ .Values.service.port }}/healthz
```

### 5.2 数据库连接测试

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: {{ include "myapp.fullname" . }}-test-db
  annotations:
    "helm.sh/hook": test
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  restartPolicy: Never
  containers:
    - name: db-test
      image: postgres:15
      command:
        - /bin/bash
        - -c
        - |
          PGPASSWORD=$DB_PASSWORD psql -h $DB_HOST -U $DB_USER -d $DB_NAME -c "SELECT 1;"
      env:
        - name: DB_HOST
          value: {{ .Values.database.host }}
        - name: DB_USER
          valueFrom:
            secretKeyRef:
              name: {{ .Values.database.existingSecret }}
              key: username
        - name: DB_PASSWORD
          valueFrom:
            secretKeyRef:
              name: {{ .Values.database.existingSecret }}
              key: password
        - name: DB_NAME
          value: {{ .Values.database.name }}
```

### 5.3 API 测试

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: {{ include "myapp.fullname" . }}-test-api
  annotations:
    "helm.sh/hook": test
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  restartPolicy: Never
  containers:
    - name: api-test
      image: curlimages/curl:latest
      command:
        - /bin/sh
        - -c
        - |
          # 测试健康检查
          curl -f http://{{ include "myapp.fullname" . }}:{{ .Values.service.port }}/healthz

          # 测试 API 端点
          curl -f http://{{ include "myapp.fullname" . }}:{{ .Values.service.port }}/api/v1/status

          # 测试认证
          curl -f -H "Authorization: Bearer $API_TOKEN" \
            http://{{ include "myapp.fullname" . }}:{{ .Values.service.port }}/api/v1/me
      env:
        - name: API_TOKEN
          valueFrom:
            secretKeyRef:
              name: {{ .Values.api.existingSecret }}
              key: token
```

### 5.4 集成测试

```yaml
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-test-integration
  annotations:
    "helm.sh/hook": test
    "helm.sh/hook-weight": "5"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  backoffLimit: 3
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: integration-test
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          command:
            - /bin/bash
            - -c
            - |
              # 运行集成测试套件
              python -m pytest tests/integration/ -v --tb=short
          env:
            - name: TEST_ENV
              value: "helm-test"
            - name: BASE_URL
              value: "http://{{ include "myapp.fullname" . }}:{{ .Values.service.port }}"
```

## 6. Hook 高级用法

### 6.1 等待资源就绪

```yaml
# post-install: 等待服务就绪后执行
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-wait-ready
  annotations:
    "helm.sh/hook": post-install
    "helm.sh/hook-weight": "0"
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: wait-ready
          image: bitnami/kubectl:latest
          command:
            - /bin/bash
            - -c
            - |
              echo "等待 Deployment 就绪..."
              kubectl rollout status deployment/{{ include "myapp.fullname" . }} \
                -n {{ .Release.Namespace }} \
                --timeout=300s

              echo "检查 Pod 状态..."
              kubectl get pods -n {{ .Release.Namespace }} \
                -l app.kubernetes.io/name={{ .Chart.Name }}
```

### 6.2 条件执行

```yaml
# 仅在特定条件下执行 Hook
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-conditional-hook
  annotations:
    "helm.sh/hook": pre-install
    "helm.sh/hook-delete-policy": hook-succeeded
spec:
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: conditional
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          command:
            - /bin/bash
            - -c
            - |
              {{- if .Values.database.migration.enabled }}
              echo "运行数据库迁移..."
              python manage.py migrate
              {{- else }}
              echo "数据库迁移已禁用，跳过"
              {{- end }}
```

### 6.3 Hook 超时控制

```yaml
# 设置 Hook 超时
apiVersion: batch/v1
kind: Job
metadata:
  name: {{ include "myapp.fullname" . }}-long-running
  annotations:
    "helm.sh/hook": pre-install
    "helm.sh/hook-delete-policy": hook-succeeded
    # Helm 3.7+ 支持
    "helm.sh/hook-timeout": "600"
spec:
  activeDeadlineSeconds: 600
  template:
    spec:
      restartPolicy: Never
      containers:
        - name: long-running
          image: "{{ .Values.image.repository }}:{{ .Values.image.tag }}"
          command: ["python", "long_running_task.py"]
```

## 7. Hook 监控与排障

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 查看 Hook 状态
helm history my-app

# 查看 Hook 日志
kubectl logs job/my-app-db-migrate -n production

# 查看 Hook 事件
kubectl get events -n production --field-selector reason=BackOff

# 手动重试失败的 Hook
kubectl delete job my-app-db-migrate -n production
helm upgrade my-app ./chart -f values/prod.yaml

# 强制清理 Hook 资源
kubectl get jobs -n production -l "helm.sh/hook"
kubectl delete jobs -n production -l "helm.sh/hook"
```
## 8. Hook 最佳实践

```yaml
# 最佳实践总结
annotations:
  # 1. 明确 Hook 类型
  "helm.sh/hook": pre-install,pre-upgrade

  # 2. 设置合理的权重
  "helm.sh/hook-weight": "-5"

  # 3. 选择合适的删除策略
  "helm.sh/hook-delete-policy": hook-succeeded

  # 4. 设置超时（Helm 3.7+）
  "helm.sh/hook-timeout": "300"

# Job 配置最佳实践
spec:
  # 5. 设置重试次数
  backoffLimit: 3

  # 6. 设置超时
  activeDeadlineSeconds: 300

  # 7. 不自动重启
  template:
    spec:
      restartPolicy: Never
```

---

## Related

- [[domain-18-manifests-patterns/03-helm-values-patterns/01-helm-values-best-practices|Helm Values 最佳实践]]
- [[domain-18-manifests-patterns/03-helm-values-patterns/03-helm-library-charts-reuse|Helm Library Chart 复用模式]]

## See Also

- [Helm Hooks 文档](https://helm.sh/docs/topics/charts_hooks/)
- [Helm Test](https://helm.sh/docs/topics/chart_tests/)


<!-- risk-assessed -->
