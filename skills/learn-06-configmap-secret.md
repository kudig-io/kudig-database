---
title: 第六课：ConfigMap 和 Secret - 配置管理
description: '# 第六课：ConfigMap 和 Secret - 配置管理'
category: skills
tags:
- k8s
- learn
- fundamentals
- docker
- opa
- mysql
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 第六课：ConfigMap 和 Secret - 配置管理 是什么
- 如何 第六课：ConfigMap 和 Secret - 配置管理
trigger_keywords:
- 第六课：ConfigMap
- Secret
- 配置管理
prerequisites:
- kubectl-basics
- mysql-basics
- policy-basics
created: "2026-05-23"
---

# 第六课：ConfigMap 和 Secret - 配置管理

> **章节**: 入门引导 | **难度**: 入门 | **时长**: 20 分钟

---

## 学习目标

1. 理解 ConfigMap 和 Secret 的作用
2. 掌握创建和使用方法
3. 了解环境变量和 volume 挂载方式
4. 学会管理应用配置

---

## 1. 问题引入

```
【场景】

你开发了一个应用，需要配置：
• 数据库连接地址 (mysql://localhost:3306)
• API 密钥 (your-secret-key-123)
• 日志级别 (info)
• 环境名称 (production)

问题：这些配置如何传递给应用？

【方案对比】

方案一：写死在代码里
❌ 不灵活，修改需要重新编译

方案二：环境变量
✅ 灵活，不同环境不同配置

方案三：配置文件
✅ 统一管理，方便修改

【K8s 解决方案】

ConfigMap：存储非敏感配置
Secret：存储敏感信息（密码、密钥、证书）

这样就可以把配置和代码分离！
```

---

## 2. ConfigMap

### 2.1 创建 ConfigMap

```
【方式一：从字面值创建】

kubectl create configmap app-config \
  --from-literal=database_url=localhost:3306 \
  --from-literal=log_level=info

【方式二：从文件创建】

kubectl create configmap app-config \
  --from-file=config.properties

【方式三：从 YAML 创建】

apiVersion: v1
kind: ConfigMap
metadata:
  name: app-config
data:
  database_url: "localhost:3306"
  log_level: "info"
```

### 2.2 在 Pod 中使用

```
【方式一：环境变量】

apiVersion: v1
kind: Pod
metadata:
  name: my-pod
spec:
  containers:
  - name: my-container
    image: my-app
    env:
    - name: DATABASE_URL
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: database_url
    - name: LOG_LEVEL
      valueFrom:
        configMapKeyRef:
          name: app-config
          key: log_level

【方式二：环境变量（全部）】

envFrom:
- configMapRef:
    name: app-config

这样会创建所有 key 作为环境变量。

【方式三：Volume 挂载】

volumes:
- name: config
  configMap:
    name: app-config
volumeMounts:
- name: config
  mountPath: /etc/config

文件会出现在：
/etc/config/database_url
/etc/config/log_level
```

---

## 3. Secret

### 3.1 为什么需要 Secret？

```
【ConfigMap 的问题】

ConfigMap 的值是明文存储的。
如果存密码、API 密钥等敏感信息，不安全！

【Secret 的特点】

1. 值是 base64 编码的（不是加密！）
2. 可以配合加密解决方案（如 Vault）
3. 支持 TLS 证书等二进制数据

【注意】

"Secret 只是编码，不是加密！
任何人可以 base64 解码看到原始值。
生产环境建议配合加密解决方案。"
```

### 3.2 创建 Secret

```
【方式一：从字面值创建】

kubectl create secret generic app-secret \
  --from-literal=username=admin \
  --from-literal=password=123456

【方式二：从文件创建】

kubectl create secret generic tls-cert \
  --from-file=tls.crt=path/to/cert.pem \
  --from-file=tls.key=path/to/key.pem

【方式三：从 YAML 创建】

apiVersion: v1
kind: Secret
metadata:
  name: app-secret
type: Opaque
data:
  username: YWRtaW4=    # base64 编码
  password: MTIzNDU2   # base64 编码

编码方法：
echo -n "admin" | base64
# 输出：YWRtaW4=
```

### 3.3 在 Pod 中使用

```
【环境变量方式】

env:
- name: DB_USERNAME
  valueFrom:
    secretKeyRef:
      name: app-secret
      key: username
- name: DB_PASSWORD
  valueFrom:
    secretKeyRef:
      name: app-secret
      key: password

【Volume 挂载方式】

volumes:
- name: secrets
  secret:
    secretName: app-secret
volumeMounts:
- name: secrets
  mountPath: /etc/secrets
  readOnly: true

文件内容（base64 解码后的值）：
/etc/secrets/username
/etc/secrets/password
```

---

## 4. 镜像拉取 Secret

### 4.1 私有仓库认证

```
【场景】

如果需要从私有镜像仓库拉取镜像，
需要创建 docker-registry secret。

【创建私有仓库 Secret】

kubectl create secret docker-registry my-registry-secret \
  --docker-server=registry.example.com \
  --docker-username=user \
  --docker-password=password \
  --docker-email=user@example.com

【在 Pod 中使用】

apiVersion: v1
kind: Pod
metadata:
  name: my-pod
spec:
  imagePullSecrets:
  - name: my-registry-secret
  containers:
  - name: my-container
    image: registry.example.com/my-app:latest
```

---

## 5. 常见问题

### 5.1 ConfigMap/Secret 更新后 Pod 不生效

```
【问题】

修改了 ConfigMap 或 Secret，但 Pod 没有更新。

【原因】

如果是环境变量，Pod 需要重启才能看到新值。
如果是 Volume 挂载，K8s 会自动更新（需要片刻）。

【解决方案】

方式一：重启 Pod
kubectl delete pod <pod-name> --now

方式二：如果使用 Deployment
kubectl rollout restart deployment <name>

方式三：如果是 Volume，可以配置 subPath 避免缓存问题
```

### 5.2 Secret 值查看

```
【查看 Secret 内容】

kubectl get secret app-secret -o jsonpath='{.data.password}' | base64 -d

【查看所有 Secret】

kubectl get secret
kubectl describe secret <name>
```

---

## 6. 总结

```
【命令速查】

ConfigMap 创建：
kubectl create configmap app-config --from-literal=key=value
kubectl create configmap app-config --from-file=config.properties

Secret 创建：
kubectl create secret generic app-secret --from-literal=key=value

查看：
kubectl get configmap
kubectl get secret
kubectl describe configmap <name>
kubectl describe secret <name>

【核心要点】

1. ConfigMap 存储非敏感配置
2. Secret 存储敏感信息（密码、密钥、证书）
3. 可以通过环境变量或 volume 挂载使用
4. Secret 只是 base64 编码，不是加密
5. 私有镜像需要 imagePullSecrets

【下节课预告】

下节课我们会学习 Namespace：
• 如何隔离资源
• 多团队多环境管理
• 资源配额和限制

有问题吗？"
```

---

**关联文档**:
- [../06-configuration/06-namespace-resource-quota.md](../06-configuration/06-namespace-resource-quota.md) — 命名空间与资源配额
- [../../domain-10-troubleshooting-diagnostics/topic-skills/14-configmap-secret-failure.md](../../domain-10-troubleshooting-diagnostics/topic-skills/14-configmap-secret-failure.md) — 配置管理问题 [[SKILL|Skill]]
- [../../domain-05-security-compliance/](../../domain-05-security-compliance/) — K8s 安全文档

## Related

- [[skills/learn-10-health-check|learn-10-health-check]] — 第八课：健康检查 - Probe 详解
- [[skills/skill-k8s-node-notready-SKILL|skill-k8s-node-notready-SKILL]] — Skill
- [[docker]] — Docker
- [[deployment]] — Deployment
- [[entities/vault|vault]] — HashiCorp Vault
