---
title: Kubernetes Security Hardening Production Checklist
description: K8s 生产安全加固清单 — 控制平面加固、节点安全、Pod 安全、网络安全、供应链安全、运行时防护
summary: 面向生产环境的 Kubernetes 安全加固完整检查清单，涵盖 CIS Benchmark 全部关键项
category: practice
tags:
- security-hardening
- cis-benchmark
- pod-security
- supply-chain
- production
tier: core
created: '2026-07-21'
last_updated: '2026-07-21'
difficulty: advanced
domain: security
---
# Kubernetes 生产安全加固清单

> 基于 CIS Benchmark + NSA/CISA 指南的生产安全加固实践。

## 控制平面加固

### API Server

```yaml
# /etc/kubernetes/manifests/kube-apiserver.yaml 关键参数
spec:
  containers:
    - command:
        - kube-apiserver
        # 认证
        - --anonymous-auth=false
        - --token-auth-file=/etc/kubernetes/tokens.csv
        - --oidc-issuer-url=https://dex.example.com
        # 授权
        - --authorization-mode=Node,RBAC
        # 准入控制
        - --enable-admission-plugins=NodeRestriction,PodSecurity,ServiceAccount,DefaultStorageClass,ResourceQuota
        # 安全
        - --tls-cipher-suites=TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256,TLS_ECDHE_RSA_WITH_AES_256_GCM_SHA384
        - --tls-min-version=VersionTLS12
        - --encryption-provider-config=/etc/kubernetes/encryption-config.yaml
        # 审计
        - --audit-policy-file=/etc/kubernetes/audit-policy.yaml
        - --audit-log-path=/var/log/kubernetes/audit.log
        - --audit-log-maxage=90
        - --audit-log-maxbackup=10
        # 限制
        - --request-timeout=60s
        - --service-account-lookup=true
        - --profiling=false
```

### etcd 加固

```yaml
# etcd 关键配置
spec:
  containers:
    - command:
        - etcd
        - --peer-client-cert-auth=true
        - --client-cert-auth=true
        - --auto-tls=false
        - --peer-auto-tls=false
        - --cipher-suites=TLS_ECDHE_RSA_WITH_AES_128_GCM_SHA256
```

### 加密配置

```yaml
# /etc/kubernetes/encryption-config.yaml
apiVersion: apiserver.config.k8s.io/v1
kind: EncryptionConfiguration
resources:
  - resources:
      - secrets
    providers:
      - aescbc:
          keys:
            - name: key1
              secret: <base64-encoded-32-byte-key>
      - identity: {}
```

## 节点安全

```bash
# 节点加固脚本
#!/bin/bash

# 1. 禁用 swap
swapoff -a
sed -i '/swap/d' /etc/fstab

# 2. 内核参数
cat > /etc/sysctl.d/99-k8s-security.conf << EOF
kernel.kptr_restrict = 2
kernel.dmesg_restrict = 1
kernel.yama.ptrace_scope = 1
net.ipv4.conf.all.accept_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.all.accept_source_route = 0
net.ipv4.ip_forward = 1
vm.overcommit_memory = 1
vm.panic_on_oom = 0
EOF
sysctl --system

# 3. 禁用不需要的内核模块
cat > /etc/modprobe.d/k8s-blacklist.conf << EOF
blacklist sctp
blacklist dccp
blacklist rds
blacklist tipc
EOF

# 4. 文件权限
chmod 644 /etc/kubernetes/manifests/*.yaml
chmod 600 /etc/kubernetes/admin.conf
chmod 600 /var/lib/etcd/*
chmod 644 /etc/kubernetes/pki/*.crt
chmod 600 /etc/kubernetes/pki/*.key
```

## Pod 安全标准

### PSA 标签（命名空间级）

```yaml
apiVersion: v1
kind: Namespace
metadata:
  name: production
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/enforce-version: latest
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### Restricted Pod 模板

```yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-pod
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 1000
    fsGroup: 1000
    seccompProfile:
      type: RuntimeDefault
  containers:
    - name: app
      image: registry.example.com/app:v1.2.3@sha256:abc123...
      securityContext:
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        capabilities:
          drop: ["ALL"]
      resources:
        requests:
          cpu: 100m
          memory: 128Mi
        limits:
          cpu: "1"
          memory: 1Gi
      volumeMounts:
        - name: tmp
          mountPath: /tmp
  volumes:
    - name: tmp
      emptyDir: {}
  automountServiceAccountToken: false
```

## 供应链安全

```yaml
# 镜像签名验证（Kyverno）
apiVersion: kyverno.io/v1
kind: ClusterPolicy
metadata:
  name: verify-image-signatures
spec:
  validationFailureAction: Enforce
  background: false
  rules:
    - name: verify-signature
      match:
        resources:
          kinds: ["Pod"]
      verifyImages:
        - imageReferences:
            - "registry.example.com/*"
          attestors:
            - entries:
                - keys:
                    publicKeys: |-
                      -----BEGIN PUBLIC KEY-----
                      MFkwEwYHKoZIzj0CAQYIKoZIzj0DAQcDQgAE...
                      -----END PUBLIC KEY-----
    - name: check-sbom
      match:
        resources:
          kinds: ["Pod"]
      verifyImages:
        - imageReferences:
            - "registry.example.com/*"
          attestations:
            - type: https://spdx.dev/Document
              conditions:
                - all:
                    - key: "{{ creationInfo.created }}"
                      operator: NotEquals
                      value: ""
```

## 网络安全加固

```yaml
# 默认拒绝所有入站
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-ingress
  namespace: production
spec:
  podSelector: {}
  policyTypes: ["Ingress"]
---
# 默认拒绝所有出站（仅允许 DNS + 必要服务）
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: default-deny-egress
  namespace: production
spec:
  podSelector: {}
  policyTypes: ["Egress"]
  egress:
    - to:
        - namespaceSelector:
            matchLabels:
              kubernetes.io/metadata.name: kube-system
      ports:
        - protocol: UDP
          port: 53
        - protocol: TCP
          port: 53
```

## 审计策略

```yaml
# /etc/kubernetes/audit-policy.yaml
apiVersion: audit.k8s.io/v1
kind: Policy
rules:
  - level: None
    resources:
      - group: ""
        resources: ["endpoints", "pods/status"]
  - level: RequestResponse
    resources:
      - group: ""
        resources: ["secrets", "configmaps", "serviceaccounts"]
    verbs: ["create", "update", "patch", "delete"]
  - level: RequestResponse
    resources:
      - group: ""
        resources: ["pods/exec", "pods/attach"]
  - level: Metadata
    resources:
      - group: ""
        resources: ["pods", "services", "namespaces"]
  - level: Metadata
    omitStages: ["RequestReceived"]
```

## 安全检查清单总览

| 类别 | 检查项 | 优先级 |
|------|--------|--------|
| 控制平面 | API Server 禁用匿名认证 | P0 |
| 控制平面 | 启用 RBAC + Node 授权 | P0 |
| 控制平面 | etcd 加密 + TLS | P0 |
| 控制平面 | 审计日志启用 | P0 |
| 节点 | 禁用 SSH root 登录 | P0 |
| 节点 | kubelet 认证授权 | P0 |
| Pod | PSA restricted | P0 |
| Pod | 禁止特权容器 | P0 |
| Pod | 只读根文件系统 | P1 |
| Pod | 资源限制 | P1 |
| 网络 | 默认拒绝 NetworkPolicy | P0 |
| 网络 | 加密传输（mTLS/WireGuard） | P1 |
| 供应链 | 镜像签名验证 | P1 |
| 供应链 | 固定镜像 digest | P1 |
| 密钥 | 外部密钥管理（Vault/ESO） | P0 |
| 密钥 | etcd 静态加密 | P0 |
| 运行时 | Falco/Tetragon 威胁检测 | P2 |
| 合规 | CIS Benchmark 定期扫描 | P1 |

---

## 运行时安全防护

### Falco 部署与规则

```yaml
# Falco DaemonSet 关键配置
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: falco
  namespace: falco
spec:
  template:
    spec:
      containers:
        - name: falco
          image: falcosecurity/falco:latest
          args:
            - /usr/bin/falco
            - --modern-bpf        # 使用 eBPF
            - -o "json_output=true"
            - -o "json_include_output_property=true"
          securityContext:
            privileged: true  # Falco 需要特权访问内核
          volumeMounts:
            - name: proc
              mountPath: /host/proc
              readOnly: true
            - name: boot
              mountPath: /host/boot
              readOnly: true
      volumes:
        - name: proc
          hostPath:
            path: /proc
        - name: boot
          hostPath:
            path: /boot
---
# 自定义规则: 检测容器内异常行为
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-custom-rules
  namespace: falco
data:
  custom_rules.yaml: |
    - rule: Shell Spawned in Container
      desc: 检测容器内启动 shell
      condition: >
        spawned_process and container and
        proc.name in (bash, sh, zsh, dash)
      output: >
        Shell 在容器中启动 (user=%user.name container=%container.name
        shell=%proc.name parent=%proc.pname cmdline=%proc.cmdline)
      priority: WARNING

    - rule: Write Below /etc in Container
      desc: 检测容器内写入 /etc
      condition: >
        write and container and
        fd.directory = /etc
      output: >
        容器内写入 /etc (user=%user.name container=%container.name
        file=%fd.name)
      priority: ERROR

    - rule: Outbound Connection to Crypto Mining Pool
      desc: 检测挖矿连接
      condition: >
        outbound and container and
        fd.sip in (mining_pool_ips)
      output: >
        检测到挖矿连接 (container=%container.name ip=%fd.sip)
      priority: CRITICAL
```

### Tetragon (eBPF) 替代方案

```yaml
# Tetragon TracingPolicy: 监控敏感文件访问
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: monitor-sensitive-files
spec:
  kprobes:
    - call: "fd_install"
      syscall: false
      args:
        - index: 0
          type: "int"
        - index: 1
          type: "file"
      selectors:
        - matchArgs:
            - index: 1
              operator: "Equal"
              values:
                - "/etc/shadow"
                - "/etc/passwd"
                - "/root/.ssh"
---
# 监控网络连接
apiVersion: cilium.io/v1alpha1
kind: TracingPolicy
metadata:
  name: monitor-network
spec:
  kprobes:
    - call: "tcp_connect"
      syscall: false
      args:
        - index: 0
          type: "sock"
      selectors:
        - matchNamespaces:
            - production
```

---

## 安全加固自动化

### 自动化加固脚本

```bash
#!/bin/bash
# 🟡 security-hardening-audit.sh — 安全加固审计
set -euo pipefail

echo "══════════════════════════════════════════"
echo "  Kubernetes 安全加固审计 $(date)"
echo "══════════════════════════════════════════"

PASS=0; WARN=0; FAIL=0

check() {
  local desc="$1" cmd="$2" expected="$3"
  result=$(eval "$cmd" 2>/dev/null || echo "ERROR")
  if [[ "$result" == *"$expected"* ]]; then
    echo "✅ PASS: $desc"
    ((PASS++))
  else
    echo "❌ FAIL: $desc (got: $result)"
    ((FAIL++))
  fi
}

# 控制平面检查
echo -e "\n📌 控制平面"
check "匿名认证禁用" \
  "kubectl -n kube-system get pod -l component=kube-apiserver -o yaml | grep anonymous-auth" \
  "false"
check "RBAC 启用" \
  "kubectl -n kube-system get pod -l component=kube-apiserver -o yaml | grep authorization-mode" \
  "RBAC"
check "审计日志启用" \
  "kubectl -n kube-system get pod -l component=kube-apiserver -o yaml | grep audit-log-path" \
  "audit"

# Pod 安全检查
echo -e "\n📌 Pod 安全"
PRIVILEGED=$(kubectl get pods -A -o json | jq '[.items[].spec.containers[] | select(.securityContext.privileged == true)] | length')
if [ "$PRIVILEGED" -eq 0 ]; then
  echo "✅ PASS: 无特权容器"; ((PASS++))
else
  echo "❌ FAIL: 发现 $PRIVILEGED 个特权容器"; ((FAIL++))
fi

HOST_NET=$(kubectl get pods -A -o json | jq '[.items[] | select(.spec.hostNetwork == true)] | length')
if [ "$HOST_NET" -eq 0 ]; then
  echo "✅ PASS: 无 hostNetwork Pod"; ((PASS++))
else
  echo "⚠️ WARN: 发现 $HOST_NET 个 hostNetwork Pod"; ((WARN++))
fi

# 网络策略检查
echo -e "\n📌 网络安全"
for ns in production staging; do
  NP_COUNT=$(kubectl get networkpolicy -n $ns --no-headers 2>/dev/null | wc -l)
  if [ "$NP_COUNT" -gt 0 ]; then
    echo "✅ PASS: $ns 有 $NP_COUNT 个 NetworkPolicy"; ((PASS++))
  else
    echo "❌ FAIL: $ns 无 NetworkPolicy"; ((FAIL++))
  fi
done

# 密钥管理检查
echo -e "\n📌 密钥管理"
ESO=$(kubectl get externalsecrets -A --no-headers 2>/dev/null | wc -l)
if [ "$ESO" -gt 0 ]; then
  echo "✅ PASS: 使用 External Secrets ($ESO 个)"; ((PASS++))
else
  echo "⚠️ WARN: 未使用 External Secrets"; ((WARN++))
fi

echo -e "\n══════════════════════════════════════════"
echo "  结果: ✅ $PASS | ⚠️ $WARN | ❌ $FAIL"
echo "══════════════════════════════════════════"
```

### 安全加固成熟度

| 级别 | 名称 | 特征 | 建议时间 |
|------|------|------|----------|
| L1 | 基础 | 默认配置，无加固 | - |
| L2 | 强化 | PSA + RBAC + 审计日志 | 2 周 |
| L3 | 防护 | NetworkPolicy + 镜像扫描 + 密钥管理 | 1 月 |
| L4 | 检测 | Falco/Tetragon + 异常检测 | 3 月 |
| L5 | 零信任 | mTLS + 工作负载身份 + 微分段 | 6 月 |
| L6 | 自适应 | AI 威胁检测 + 自动响应 | 12 月 |

---

## 安全加固验证命令集

```bash
# 🟢 快速安全检查命令集

# 1. 检查特权容器
kubectl get pods -A -o json | jq '.items[] | select(.spec.containers[].securityContext.privileged == true) | .metadata.name'

# 2. 检查 hostNetwork/hostPID/hostIPC
kubectl get pods -A -o json | jq '.items[] | select(.spec.hostNetwork == true or .spec.hostPID == true or .spec.hostIPC == true) | .metadata.name'

# 3. 检查无资源限制的容器
kubectl get pods -A -o json | jq '.items[].spec.containers[] | select(.resources.limits == null) | .name' | head -20

# 4. 检查 default ServiceAccount 使用
kubectl get pods -A -o json | jq '.items[] | select(.spec.serviceAccountName == "default") | "\(.metadata.namespace)/\(.metadata.name)"' | head -20

# 5. 检查未加密的 Secret
kubectl get secrets -A -o json | jq '.items[] | select(.type == "Opaque") | .metadata.name' | wc -l

# 6. 检查 cluster-admin 绑定
kubectl get clusterrolebindings -o json | jq '.items[] | select(.roleRef.name == "cluster-admin") | .metadata.name'

# 7. 检查可写 rootFilesystem
kubectl get pods -A -o json | jq '.items[].spec.containers[] | select(.securityContext.readOnlyRootFilesystem != true) | .name' | wc -l

# 8. CIS Benchmark 快速扫描
kubectl apply -f https://raw.githubusercontent.com/aquasecurity/kube-bench/main/job.yaml
kubectl wait --for=condition=complete job/kube-bench --timeout=300s
kubectl logs job/kube-bench | grep -c '\[FAIL\]'
kubectl delete job kube-bench
```

## Related

- [[08-安全/06-合规审计/index.md|合规审计]]
- [[08-安全/04-策略治理/index.md|策略治理]]
- [[08-安全/03-运行时安全/index.md|运行时安全]]
