# Confidential Containers

> **成熟度**: Sandbox | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官网** | https://confidentialcontainers.org/ |
| **GitHub** | https://github.com/confidential-containers |
| **许可证** | Apache-2.0 |
| **开发语言** | Rust, Go |
| **CNCF 状态** | Sandbox |

---

## 项目概述

Confidential Containers (CoCo) 是一个为 Kubernetes 提供机密计算能力的项目，使容器工作负载能够在硬件 TEE（可信执行环境）中运行。通过利用 AMD SEV、Intel TDX、IBM SE 等硬件机密计算技术，CoCo 保护运行中的数据免受云提供商、管理员和其他特权软件的访问。

### 核心特性

- **硬件级隔离**: 利用 CPU TEE 技术（AMD SEV-SNP, Intel TDX, IBM SE）保护工作负载
- **远程证明 (Remote Attestation)**: 在部署前验证 TEE 环境的完整性和真实性
- **镜像加密**: 支持容器镜像加密，确保镜像内容仅在 TEE 内解密
- **密钥管理集成**: 与 KBS (Key Broker Service) 集成实现安全的密钥分发
- **透明部署**: 对现有容器工作负载的修改最小，保持 Kubernetes 原生体验
- **多运行时支持**: 支持 Kata Containers、CoCo (peer-pods) 等运行时
- **机密 Pod**: 整个 Pod 在单个 TEE 中运行，保护 Pod 间通信

---

## 架构设计

```
┌────────────────────────────────────────────────────────┐
│                   Kubernetes Cluster                    │
│                                                         │
│  ┌─────────────────────────────────────────────────┐   │
│  │              CoCo Operator                       │   │
│  │   (RuntimeClass, Installation Management)        │   │
│  └──────────────────────┬──────────────────────────┘   │
│                         │                               │
│  ┌──────────────────────┴──────────────────────────┐   │
│  │              Kata Containers Runtime              │   │
│  │                                                   │   │
│  │  ┌─────────┐  ┌──────────┐  ┌───────────────┐  │   │
│  │  │ kata-    │  │ Guest    │  │ Attestation   │  │   │
│  │  │ runtime  │  │ Image    │  │ Agent (AA)    │  │   │
│  │  └────┬─────┘  └──────────┘  └───────┬───────┘  │   │
│  │       │                              │           │   │
│  │  ┌────┴──────────────────────────────┴───────┐  │   │
│  │  │        Confidential VM (TEE)               │  │   │
│  │  │  ┌──────────┐  ┌──────────┐               │  │   │
│  │  │  │Container │  │Container │  (encrypted)  │  │   │
│  │  │  │  App A   │  │  App B   │               │  │   │
│  │  │  └──────────┘  └──────────┘               │  │   │
│  │  │  AMD SEV-SNP / Intel TDX / IBM SE         │  │   │
│  │  └───────────────────────────────────────────┘  │   │
│  └─────────────────────────────────────────────────┘   │
│                                                         │
│  ┌─────────────────────────────────────────────────┐   │
│  │         Key Broker Service (KBS)                 │   │
│  │   ┌──────────┐  ┌──────────┐  ┌────────────┐   │   │
│  │   │Attestat- │  │ Policy   │  │ Key/Secret  │   │   │
│  │   │ion Svc   │  │ Engine   │  │ Provider    │   │   │
│  │   └──────────┘  └──────────┘  └────────────┘   │   │
│  └─────────────────────────────────────────────────┘   │
└────────────────────────────────────────────────────────┘
```

### 核心组件

| 组件 | 说明 |
|:---|:---|
| **CoCo Operator** | Kubernetes Operator，管理运行时安装和 RuntimeClass 配置 |
| **Kata Containers** | 基于轻量级 VM 的容器运行时，提供 TEE 隔离 |
| **Attestation Agent (AA)** | Guest 内运行的证明代理，处理远程证明流程 |
| **Key Broker Service (KBS)** | 密钥管理服务，验证证明并分发密钥和机密数据 |
| **Confidential Data Hub (CDH)** | Guest 内的数据中心，管理密钥和镜像解密 |
| **image-rs** | 安全的容器镜像管理，支持加密镜像的拉取和解密 |

---

## 快速开始

### 安装 CoCo Operator

```bash
# 部署 CoCo Operator
kubectl apply -k github.com/confidential-containers/operator/config/release?ref=v0.10.0

# 等待 Operator 就绪
kubectl wait --for=condition=Available deployment/cc-operator-controller-manager \
  -n confidential-containers-system --timeout=180s

# 创建 CoCo 自定义资源（安装运行时）
cat <<EOF | kubectl apply -f -
apiVersion: confidentialcontainers.org/v1beta1
kind: CcRuntime
metadata:
  name: ccruntime-sample
  namespace: confidential-containers-system
spec:
  ccNodeSelector:
    matchLabels:
      node-role.kubernetes.io/worker: ""
  runtimeName: kata-qemu-snp  # AMD SEV-SNP
  # runtimeName: kata-qemu-tdx  # Intel TDX
EOF

# 等待运行时安装完成
kubectl wait --for=condition=Ready ccruntime/ccruntime-sample \
  -n confidential-containers-system --timeout=360s
```

### 部署机密 Pod

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: confidential-workload
  annotations:
    io.katacontainers.config.hypervisor.default_memory: "2048"
spec:
  runtimeClassName: kata-qemu-snp  # 使用机密运行时
  containers:
    - name: app
      image: ghcr.io/confidential-containers/test-images/busybox:latest
      command: ["sh", "-c", "echo 'Running in TEE!' && sleep infinity"]
      resources:
        limits:
          memory: "512Mi"
          cpu: "1"
```

---

## 配置详解

### Key Broker Service (KBS) 部署

```yaml
apiVersion: apps/v1
kind: Deployment
metadata:
  name: kbs
  namespace: coco-tenant
spec:
  replicas: 1
  selector:
    matchLabels:
      app: kbs
  template:
    metadata:
      labels:
        app: kbs
    spec:
      containers:
        - name: kbs
          image: ghcr.io/confidential-containers/key-broker-service:latest
          ports:
            - containerPort: 8080
          env:
            - name: KBS_INSECURE_HTTP
              value: "true"  # 生产环境应使用 TLS
          volumeMounts:
            - name: kbs-config
              mountPath: /etc/kbs
            - name: kbs-repository
              mountPath: /opt/confidential-containers/kbs/repository
      volumes:
        - name: kbs-config
          configMap:
            name: kbs-config
        - name: kbs-repository
          persistentVolumeClaim:
            claimName: kbs-repository-pvc
---
apiVersion: v1
kind: ConfigMap
metadata:
  name: kbs-config
  namespace: coco-tenant
data:
  kbs-config.toml: |
    [http_server]
    sockets = ["0.0.0.0:8080"]
    
    [attestation_token]
    type = "CoCo"
    
    [attestation_service]
    type = "coco_as_builtin"
    
    [attestation_service.attestation_policy]
    type = "opa"
    policy_path = "/etc/kbs/policy.rego"
```

### 镜像加密

```bash
# 生成加密密钥
openssl rand -out image-key.bin 32

# 使用 skopeo 加密容器镜像
skopeo copy \
  docker://docker.io/library/nginx:latest \
  docker://registry.example.com/encrypted/nginx:latest \
  --encryption-key provider:attestation-agent:keyid=kbs:///default/key/nginx-key

# 将密钥注册到 KBS
curl -X POST http://kbs-host:8080/kbs/v0/resource/default/key/nginx-key \
  -H "Content-Type: application/octet-stream" \
  --data-binary @image-key.bin
```

### 证明策略 (OPA)

```rego
# policy.rego - 证明策略示例
package policy

default allow = false

# 验证 TEE 证据
allow {
    input.tee == "snp"
    input.tcb_status == "UpToDate"
    valid_measurement(input.measurement)
}

# 验证固件度量值
valid_measurement(m) {
    m == "expected_measurement_hash_value"
}
```

---

## 高级功能

### Peer Pods (远程证明 Pod)

```yaml
# 使用云端 CVM 作为 Pod 运行环境
apiVersion: v1
kind: Pod
metadata:
  name: peer-pod-workload
spec:
  runtimeClassName: kata-remote  # 远程 Pod 运行时
  containers:
    - name: app
      image: registry.example.com/confidential-app:latest
      env:
        - name: SECRET_KEY
          valueFrom:
            secretKeyRef:
              name: sealed-secret  # 密钥仅在 TEE 内可用
              key: api-key
---
# Peer Pod 配置
apiVersion: v1
kind: ConfigMap
metadata:
  name: peer-pods-cm
  namespace: confidential-containers-system
data:
  CLOUD_PROVIDER: "aws"  # 或 azure, libvirt
  VXLAN_PORT: "4789"
  PODVM_INSTANCE_TYPE: "m6a.large"  # SEV-SNP 实例
  PROXY_TIMEOUT: "5m"
```

### 多 TEE 平台支持

| 平台 | RuntimeClass | 硬件要求 |
|:---|:---|:---|
| AMD SEV-SNP | `kata-qemu-snp` | AMD EPYC (Milan/Genoa) |
| Intel TDX | `kata-qemu-tdx` | Intel Xeon (Sapphire Rapids+) |
| IBM Secure Execution | `kata-qemu-se` | IBM Z (z15+) |
| Cloud API (Peer Pods) | `kata-remote` | 云端 CVM 实例 |

### Sealed Secrets 集成

```yaml
# 机密数据仅在 TEE 验证通过后释放
apiVersion: v1
kind: Secret
metadata:
  name: confidential-secret
  annotations:
    confidentialcontainers.org/kbs-resource: "default/secrets/db-password"
type: Opaque
data:
  # 密钥由 KBS 在远程证明成功后注入
  password: ""  # 运行时从 KBS 获取
```

---

## 监控与运维

### 健康检查

```bash
# 检查 CoCo 运行时状态
kubectl get ccruntime -n confidential-containers-system

# 查看 RuntimeClass
kubectl get runtimeclass | grep kata

# 检查节点上的运行时安装
kubectl get daemonset -n confidential-containers-system

# 查看机密 Pod 状态
kubectl describe pod confidential-workload
```

### 证明日志

```bash
# 查看 Attestation Agent 日志
kubectl exec -it confidential-workload -- \
  journalctl -u attestation-agent

# 查看 KBS 证明日志
kubectl logs -n coco-tenant deployment/kbs -f
```

---

## 安全模型

```
┌─────────────┐    Attestation    ┌──────────────┐
│  TEE Guest  │ ──────────────► │     KBS       │
│  (CVM)      │    Evidence      │  (Verifier)   │
│             │ ◄────────────── │               │
│  Container  │  Keys/Secrets   │  Policy +     │
│  Workload   │                 │  Key Store    │
└─────────────┘                 └──────────────┘
     │                                │
     │  Hardware Root of Trust        │  Reference Values
     ▼                                ▼
┌─────────────┐               ┌──────────────┐
│ CPU TEE     │               │ RVPS         │
│ (SEV/TDX)   │               │ (Reference   │
│             │               │  Value Svc)  │
└─────────────┘               └──────────────┘
```

---

## 最佳实践

1. **硬件验证**: 部署前确认节点 CPU 支持所需的 TEE 技术并已在 BIOS 中启用
2. **镜像加密**: 生产环境始终使用加密容器镜像，密钥通过 KBS 管理
3. **证明策略**: 使用 OPA 策略精确定义可接受的 TEE 证据和固件版本
4. **密钥轮换**: 定期轮换 KBS 中的加密密钥，配合镜像重新加密
5. **网络隔离**: KBS 服务应部署在安全的网络区域，限制访问来源
6. **审计日志**: 启用 KBS 的证明审计日志，记录所有密钥分发事件

---

## 参考资源

- [Confidential Containers 官方文档](https://confidentialcontainers.org/docs/)
- [CoCo GitHub 组织](https://github.com/confidential-containers)
- [Kata Containers](https://katacontainers.io/)
- [CNCF Sandbox Projects](https://www.cncf.io/sandbox-projects/)

---

**维护者**: Kudig Team | **许可证**: MIT
