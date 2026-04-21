# 删除时的安全清理

## 概述

`cluster-create/16-security.md` 分析了集群创建过程中的安全机制（SA Token、Audit、加密存储、NodeRestriction）。集群删除时，这些安全资产同样需要完整清理，否则可能造成凭证泄露、权限残留或审计盲区。本文档分析删除过程中的安全清理要点。

---

## 1. 证书与密钥清理

### 1.1 kubeadm reset 自动清理的证书

`cleanup-node` 阶段通过 `CleanDir` 清理 `/etc/kubernetes/pki/` 目录**内容**：

```
/etc/kubernetes/pki/                    ← 目录保留，内容清除
├── ca.crt                              ✅ 已清理
├── ca.key                              ✅ 已清理
├── apiserver.crt                       ✅ 已清理
├── apiserver.key                       ✅ 已清理
├── apiserver-kubelet-client.crt        ✅ 已清理
├── apiserver-kubelet-client.key        ✅ 已清理
├── front-proxy-ca.crt                  ✅ 已清理
├── front-proxy-ca.key                  ✅ 已清理
├── front-proxy-client.crt              ✅ 已清理
├── front-proxy-client.key              ✅ 已清理
├── sa.pub                              ✅ 已清理
├── sa.key                              ✅ 已清理
└── etcd/
    ├── ca.crt                          ✅ 已清理
    ├── ca.key                          ✅ 已清理
    ├── server.crt                      ✅ 已清理
    ├── server.key                      ✅ 已清理
    ├── peer.crt                        ✅ 已清理
    ├── peer.key                        ✅ 已清理
    ├── healthcheck-client.crt          ✅ 已清理
    └── healthcheck-client.key          ✅ 已清理
```

### 1.2 不在 /etc/kubernetes/pki/ 中的密钥

| 密钥文件 | 路径 | 是否自动清理 |
|----------|------|-------------|
| kubelet 服务端证书 | `/var/lib/kubelet/pki/kubelet.crt` | ✅ CleanDir `/var/lib/kubelet` |
| kubelet 客户端证书 | `/var/lib/kubelet/pki/kubelet-client-*.pem` | ✅ CleanDir `/var/lib/kubelet` |
| etcd 数据（含 WAL） | `/var/lib/etcd/member/` | ✅ remove-etcd-member 阶段 |

### 1.3 安全隐患：非标准路径的密钥

```bash
# 检查是否有散落在非标准路径的证书/密钥
find / -name "*.key" -o -name "*.pem" 2>/dev/null | grep -i kube

# 常见遗漏位置
ls -la /etc/kubernetes/pki/          # 标准位置
ls -la /var/lib/kubelet/pki/         # kubelet 证书
ls -la /etc/etcd/pki/                # 外部 etcd 证书
ls -la /etc/kubernetes/encryption*   # 加密配置
```

---

## 2. kubeconfig 凭证清理

### 2.1 自动清理的 kubeconfig

`cleanup-node` 阶段通过 `os.RemoveAll` 删除以下文件：

```
/etc/kubernetes/admin.conf              ✅ 已清理
/etc/kubernetes/super-admin.conf        ✅ 已清理 (v1.29+)
/etc/kubernetes/kubelet.conf            ✅ 已清理
/etc/kubernetes/bootstrap-kubelet.conf  ✅ 已清理
/etc/kubernetes/controller-manager.conf ✅ 已清理
/etc/kubernetes/scheduler.conf          ✅ 已清理
```

### 2.2 需要手动清理的 kubeconfig

| 文件 | 路径 | 风险 |
|------|------|------|
| 用户 kubeconfig | `$HOME/.kube/config` | ⚠️ 包含集群管理员凭证 |
| 备份 kubeconfig | `$HOME/.kube/config.bak` | ⚠️ 同上 |
| CI/CD kubeconfig | GitLab/GitHub/Jenkins secret store | ⚠️ 泄露后可远程操作集群 |
| 跳板机 kubeconfig | `/home/<user>/.kube/config` | ⚠️ 多用户环境 |

```bash
# 手动清理
rm -rf $HOME/.kube/config
rm -rf $HOME/.kube/

# 如果 kubeconfig 被复制到了其他位置
find / -name "admin.conf" -o -name "kubeconfig" 2>/dev/null
```

### 2.3 super-admin.conf vs admin.conf

```
┌──────────────────────────────────────────────────────────────┐
│  admin.conf                                                  │
│  ├─ CN: kubernetes-admin                                     │
│  ├─ O: system:masters                                        │
│  └─ 权限: cluster-admin（通过 RBAC 绑定）                     │
│                                                                │
│  super-admin.conf (v1.29+)                                    │
│  ├─ CN: kubernetes-super-admin                                │
│  ├─ O: system:masters                                         │
│  └─ 权限: 绕过 RBAC（通过 --super-admin-group 标志）          │
│                                                                │
│  ⚠️ 两者都具有完全集群控制权，必须全部清理                     │
└──────────────────────────────────────────────────────────────┘
```

---

## 3. ServiceAccount Token 清理

### 3.1 静态 Token（K8s < 1.24 遗留）

```bash
# 查看是否有静态 SA Token Secret
kubectl get secrets -A -kubernetes.io/service-account-token

# 删除（在集群仍可用时）
kubectl delete secrets -A -l kubernetes.io/service-account-token
```

### 3.2 TokenRequest 签发的动态 Token

动态 Token 有过期时间（默认 3600s），删除集群后自动失效。但**已签发但未过期的 Token** 仍可用于访问 API Server（如果集群仍然存在）。

```bash
# 在删除集群前，撤销所有 ServiceAccount Token
kubectl delete secrets -A --all --field-selector type=kubernetes.io/service-account-token
```

---

## 4. etcd 数据安全

### 4.1 etcd 中存储的敏感数据

```
┌──────────────────────────────────────────────────────────────┐
│  etcd 中包含的敏感信息                                        │
├──────────────────────────────────────────────────────────────┤
│  Secret (Base64 编码)                                         │
│  ├─ TLS 证书和私钥                                            │
│  ├─ 数据库密码                                                │
│  ├─ API Token                                                 │
│  └─ SSH 私钥                                                  │
│                                                                │
│  ConfigMap                                                    │
│  ├─ kubeadm-config（含集群配置）                               │
│  └─ kube-proxy-config（含网络配置）                            │
│                                                                │
│  RBAC 对象                                                    │
│  ├─ ClusterRole / ClusterRoleBinding                          │
│  └─ 用户权限定义                                              │
│                                                                │
│  Audit 策略                                                   │
│  └─ 审计日志配置                                               │
└──────────────────────────────────────────────────────────────┘
```

### 4.2 安全删除 etcd 数据

```bash
# 普通删除（数据可能被恢复）
rm -rf /var/lib/etcd

# 安全擦除（推荐处理敏感数据）
shred -vfz -n 3 /var/lib/etcd/member/snap/*
shred -vfz -n 3 /var/lib/etcd/member/wal/*
rm -rf /var/lib/etcd

# 或使用 dd 覆写整个分区
dd if=/dev/urandom of=/dev/sdX bs=1M
```

### 4.3 etcd 快照备份清理

```bash
# 查找 etcd 快照文件
find / -name "snapshot*.db" -o -name "etcd-snapshot*" 2>/dev/null

# 安全删除快照
shred -vfz -n 3 /path/to/etcd-snapshot.db
rm /path/to/etcd-snapshot.db
```

---

## 5. 加密配置清理

### 5.1 EncryptionConfiguration

如果集群启用了静态加密（Encryption at Rest），加密密钥需要清理：

```bash
# 查找加密配置
find / -name "encryption-config.yaml" -o -name "encryption*.yaml" 2>/dev/null

# 安全删除
shred -vfz -n 3 /etc/kubernetes/encryption-config.yaml
rm /etc/kubernetes/encryption-config.yaml
```

### 5.2 audit 策略清理

```bash
# 查找审计配置
find / -name "audit-policy.yaml" -o -name "audit*.yaml" 2>/dev/null

# 审计日志包含敏感操作记录，需要清理
shred -vfz -n 3 /var/log/kubernetes/audit.log
rm /var/log/kubernetes/audit.log
```

---

## 6. RBAC 残留清理

### 6.1 集群级 RBAC 对象

Node 对象删除后，关联的 RBAC 绑定**不会自动清理**：

```bash
# 查看残留的 ClusterRoleBinding（在集群仍可用时）
kubectl get clusterrolebinding -o wide | grep <deleted-node>

# 清理与已删除节点相关的绑定
kubectl delete clusterrolebinding <binding-name>

# 清理 Bootstrap Token 相关的 RBAC
kubectl delete clusterrolebinding kubeadm:node-bootstrapper
kubectl delete clusterrolebinding kubeadm:bootstrap-signer
```

### 6.2 kubeadm 创建的 RBAC 资源

```
┌──────────────────────────────────────────────────────────────┐
│  kubeadm 创建的 RBAC 资源                                    │
├──────────────────────────────────────────────────────────────┤
│  ClusterRole:                                                 │
│  ├─ kubeadm:get-nodes                                         │
│  ├─ system:node-bootstrapper                                  │
│  └─ system:certificates.k8s.io:certificatesigningrequests    │
│                                                                │
│  ClusterRoleBinding:                                          │
│  ├─ kubeadm:node-bootstrapper                                 │
│  ├─ kubeadm:bootstrap-signer                                  │
│  └─ kubeadm:automatic-approve-all-csrs                       │
│                                                                │
│  Secret:                                                      │
│  ├─ bootstrap-token-<token-id> (kube-system)                  │
│  └─ kubeadm-certs (kube-system, HA 证书上传)                 │
└──────────────────────────────────────────────────────────────┘
```

---

## 7. systemd 服务清理

### 7.1 kubelet systemd drop-in

```bash
# kubeadm 创建的 systemd 配置
ls -la /etc/systemd/system/kubelet.service.d/10-kubeadm.conf

# 清理
rm -f /etc/systemd/system/kubelet.service.d/10-kubeadm.conf
systemctl daemon-reload
systemctl disable kubelet
```

### 7.2 完整 systemd 清理

```bash
systemctl stop kubelet 2>/dev/null || true
systemctl disable kubelet 2>/dev/null || true
rm -f /etc/systemd/system/kubelet.service
rm -rf /etc/systemd/system/kubelet.service.d/
systemctl daemon-reload
```

---

## 8. 安全清理检查清单

```
□ /etc/kubernetes/pki/ 内容已清除
□ /var/lib/kubelet/pki/ 已清除
□ /var/lib/etcd/ 已安全擦除
□ /etc/kubernetes/*.conf 已删除
□ $HOME/.kube/config 已删除
□ etcd 快照备份已安全擦除
□ 加密配置文件已安全擦除
□ 审计日志已安全擦除
□ systemd 服务配置已清理
□ CI/CD 中的 kubeconfig 已轮换/删除
□ Bootstrap Token 已过期/删除
□ RBAC 绑定已清理
□ cloud IAM Role/Policy 已分离（云环境）
```

---

## 参考

- [kubeadm reset 安全考量](https://kubernetes.io/docs/reference/setup-tools/kubeadm/kubeadm-reset/)
- [etcd 安全运维](https://etcd.io/docs/latest/op-guide/security/)
- [Kubernetes Secret 安全](https://kubernetes.io/docs/concepts/security/secrets-good-practices/)
