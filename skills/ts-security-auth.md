---
title: 安全认证故障排查
description: '# 安全认证故障排查'
category: skills
tags:
- k8s
- troubleshooting
- structural
- security-auth
- etcd
- apiserver
- rbac
- webhook
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 安全认证故障排查 是什么
- 如何 安全认证故障排查
trigger_keywords:
- 安全认证故障排查
prerequisites:
- kubectl-basics
- etcd-basics
- tls-basics
created: "2026-05-23"
---

# 安全认证故障排查

### 01 Rbac Troubleshootingompt 模板|Troubleshooting]]

#### 0. 10 分钟快速诊断

1. **确认身份**：`kubectl auth whoami` 与 `kubectl config current-context`，排除 kubeconfig 指向错误集群。
2. **快速权限判断**：`kubectl auth can-i <verb> <resource> -n <ns>`，必要时 `--as=system:serviceaccount:<ns>:<sa>`。
3. **事件与审计**：`kubectl get events -A --field-selector reason=Forbidden`，并在审计日志中检索 401/403。
4. **绑定链路检查**：`kubectl get rolebinding,clusterrolebinding -A | grep <user/sa>`，确认角色绑定存在。
5. **ServiceAccount Token**：Pod 内检查 `/var/run/secrets/kubernetes.io/serviceaccount/token` 是否可读取。
6. **快速缓解**：
   - 临时放行最小权限（最小 Role/RoleBinding）。
   - 对关键操作先使用 `kubectl auth can-i --list` 明确范围。
7. **证据留存**：保存 `can-i` 输出、角色绑定 YAML、审计日志片段。

---

#### 排查方法与步骤



#### 2.1 认证问题排查

```bash
# 步骤 1：确认当前身份
kubectl auth whoami
# 或者
kubectl config current-context

# 步骤 2：检查 kubeconfig
kubectl config view
cat ~/.kube/config | grep -A10 "current-context"

# 步骤 3：验证证书
# 检查客户端证书
openssl x509 -in ~/.kube/client.crt -noout -text

# 检查证书有效期
openssl x509 -in ~/.kube/client.crt -noout -dates

# 步骤 4：测试 Token（ServiceAccount）
TOKEN=$(kubectl get secret <sa-token-secret> -o jsonpath='{.data.token}' | base64 -d)
curl -k -H "Authorization: Bearer $TOKEN" https://<api-server>:6443/api/v1/namespaces

# 步骤 5：检查 API Server 认证配置
cat /etc/kubernetes/manifests/kube-apiserver.yaml | grep -E "authentication|authorization"
```

---

### 02 Certificate Troubleshooting

#### 0. 10 分钟快速诊断

1. **快速到期检查**：`kubeadm certs check-expiration`（kubeadm 集群）或 `openssl x509 -enddate` 批量扫描。
2. **API Server 可用性**：`curl -k https://<api-server>:6443/readyz?verbose`，确认是否卡在证书链/TLS。
3. **证书匹配性**：对 `apiserver.crt`/`apiserver.key` 运行 modulus 校验，排除密钥不匹配。
4. **SAN 核对**：`openssl x509 -ext subjectAltName`，检查访问域名/IP 是否在 SAN 列表。
5. **前置依赖**：确认 etcd 证书、front-proxy 证书、SA 密钥对未过期。
6. **快速缓解**：
   - 优先更新过期证书（kubeadm/cert-manager）。
   - 对访问异常先更新 kubeconfig 中的 CA/客户端证书。
7. **证据留存**：保存过期清单、关键证书解析输出、失败日志片段。

#### 排查方法与步骤



#### 2.1 排查决策树

```
证书问题
    │
    ├─── 组件无法启动？
    │         │
    │         ├─ 查看组件日志 ──→ 定位具体证书问题
    │         ├─ 检查证书文件存在性
    │         └─ 检查文件权限
    │
    ├─── TLS 握手失败？
    │         │
    │         ├─ 证书过期 ──→ 更新证书
    │         ├─ CA 不信任 ──→ 配置正确的 CA
    │         ├─ 主体不匹配 ──→ 重新签发证书
    │         └─ SAN 缺失 ──→ 添加 SAN 重签
    │
    ├─── kubectl 无法连接？
    │         │
    │         ├─ kubeconfig 证书过期 ──→ 更新 kubeconfig
    │         ├─ API Server 证书过期 ──→ 更新 API Server 证书
    │         └─ CA 变更 ──→ 更新 kubeconfig 的 CA
    │
    └─── ServiceAccount 认证失败？
              │
              ├─ Token 无效 ──→ 检查 SA 密钥对
              └─ Token 过期 ──→ 检查 TokenRequest API
```

---

### 03 Pod Security Troubleshooting

#### 0. 10 分钟快速诊断

1. **PSA 标签核对**：`kubectl get ns -L pod-security.kubernetes.io/enforce`，确认命名空间级别策略。
2. **拒绝原因**：`kubectl apply` 输出或 `kubectl get events --field-selector reason=FailedCreate` 查 PSA/准入拒绝。
3. **SecurityContext**：检查 Pod/容器的 `runAsNonRoot`、`allowPrivilegeEscalation`、`capabilities`。
4. **特权需求确认**：核对是否真的需要 `privileged`、`hostNetwork`、`hostPath`。
5. **文件权限**：容器内 `id`、`stat` 验证 UID/GID 与挂载卷权限。
6. **快速缓解**：
   - 临时切换 PSA 为 `baseline` 或设置 `warn/audit` 观察。
   - 为必要特权场景使用最小能力与命名空间豁免。
7. **证据留存**：保存 PSA 标签、拒绝事件、Pod YAML 与容器日志。

#### 排查方法与步骤



#### 2.1 排查决策树

```
Pod 安全问题
      │
      ├─── Pod 创建被拒绝？
      │         │
      │         ├─ PSA violation ──→ 检查命名空间 PSA 配置
      │         ├─ Webhook 拒绝 ──→ 检查准入控制 Webhook
      │         └─ 配置冲突 ──→ 检查 SecurityContext 配置
      │
      ├─── 容器运行时权限问题？
      │         │
      │         ├─ permission denied ──→ 检查 runAsUser/fsGroup
      │         ├─ operation not permitted ──→ 检查 capabilities/seccomp
      │         ├─ read-only file system ──→ 检查 readOnlyRootFilesystem
      │         └─ SELinux denial ──→ 检查 seLinuxOptions
      │
      └─── 需要特权操作？
                │
                ├─ 确认必要性 ──→ 评估安全风险
                ├─ 配置最小权限 ──→ 只授予必要 capabilities
                └─ 使用豁免 ──→ 配置 PSA exemptions
```

---

### 04 Audit Logging Troubleshooting

#### 0. 10 分钟快速诊断

1. **审计是否启用**：检查 `kube-apiserver` 启动参数 `--audit-policy-file/--audit-log-path`。
2. **日志是否生成**：确认 `/var/log/kubernetes/audit.log` 是否有输出与轮转配置。
3. **策略匹配**：检查审计策略规则顺序，避免首条规则覆盖导致不记录。
4. **Webhook 发送**：若使用 webhook，检查连接/TLS/超时与接收端健康。
5. **性能与磁盘**：监控审计日志大小、磁盘空间与 API 延迟。
6. **快速缓解**：
   - 临时下调审计级别或缩小范围。
   - 配置日志轮转与采集器，避免磁盘打满。
7. **证据留存**：保留审计策略、apiserver 参数、样例日志与失败日志。

---

#### 排查方法与步骤



#### 2.1 排查决策树

```
审计日志问题
        │
        ▼
┌───────────────────────┐
│  问题类型是什么？      │
└───────────────────────┘
        │
        ├── 审计日志不生成 ──────────────────────────────────┐
        │                                                     │
        │   ┌─────────────────────────────────────────┐      │
        │   │ 检查 API Server 启动参数                │      │
        │   │ 是否配置了审计                          │      │
        │   └─────────────────────────────────────────┘      │
        │                  │                                  │
        │                  ▼                                  │
        │   ┌─────────────────────────────────────────┐      │
        │   │ --audit-policy-file 是否配置?           │      │
        │   └─────────────────────────────────────────┘      │
        │          │                │                         │
        │         否               是                         │
        │          │                │                         │
        │          ▼                ▼                         │
        │   ┌────────────┐   ┌────────────────┐              │
        │   │ 需要配置   │   │ 检查策略文件   │              │
        │   │ 审计参数   │   │ 和后端配置     │              │
        │   └────────────┘   └────────────────┘              │
        │                                                     │
        ├── 日志内容不正确 ──────────────────────────────────┤
        │                                                     │
        │   ┌────────────────────────────────
...(截断)

## 相关链接

- [[skills/audit-rbac-configurations|RBAC 审计配置]]
- [[skills/ts-security-auth|安全认证排查]]

## Related

- [[skills/skill-reference-version-matrix|skill-reference-version-matrix]] — Version Matrix
- [[entities/kube-apiserver|kube-apiserver]] — kube-apiserver
- [[etcd]] — etcd
- [[cert-manager]] — cert-manager
- [[kubernetes]] — Kubernetes (CNCF Graduated)
