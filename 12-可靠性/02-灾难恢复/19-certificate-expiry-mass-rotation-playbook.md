---
title: 集群证书批量过期紧急轮转
description: 'Kubernetes 集群证书批量过期检测、kubeadm 批量轮转、手动轮转及 kubeconfig 更新分发全流程'
summary: 'Kubernetes 集群证书批量过期检测、kubeadm 批量轮转、手动轮转及 kubeconfig 更新分发全流程'
category: reliability-engineering
tags:
- disaster-recovery
- k8s
- sre
- certificates
- kubeadm
- tls
- pki
tier: core
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
- 集群证书批量过期紧急轮转 是什么
- 如何批量轮转 Kubernetes 证书
- kubeadm certs renew 怎么用
trigger_keywords:
- certificates
- kubeadm
- tls
- pki
- cert-expiry
- kubeconfig
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


# 集群证书批量过期紧急轮转

## 概述

Kubernetes 使用大量 TLS 证书保障组件间安全通信。当集群运行超过一年（默认证书有效期），apiserver、etcd、kubelet、front-proxy 等证书可能批量过期，导致 API Server 拒绝连接、kubelet 无法注册、聚合 API 不可用等严重故障。本手册覆盖从证书过期检测到批量轮转、kubeconfig 更新分发的完整恢复流程。

---

## 1. 证书过期检测

### 1.1 kubeadm 证书检查（kubeadm 安装的集群）

```bash
# 在控制平面节点上执行
kubeadm certs check-expiration

# 输出示例：
# CERTIFICATE                EXPIRES                  RESIDUAL TIME   CERTIFICATE AUTHORITY   EXTERNALLY MANAGED
# admin.conf                 Jul 02, 2027 00:00 UTC   364d            ca                      no
# apiserver                  Jul 02, 2027 00:00 UTC   364d            ca                      no
# apiserver-etcd-client      Jul 02, 2027 00:00 UTC   364d            etcd-ca                 no
# apiserver-kubelet-client   Jul 02, 2027 00:00 UTC   364d            ca                      no
# controller-manager.conf    Jul 02, 2027 00:00 UTC   364d            ca                      no
# etcd-healthcheck-client    Jul 02, 2027 00:00 UTC   364d            etcd-ca                 no
# etcd-peer                  Jul 02, 2027 00:00 UTC   364d            etcd-ca                 no
# etcd-server                Jul 02, 2027 00:00 UTC   364d            etcd-ca                 no
# front-proxy-client         Jul 02, 2027 00:00 UTC   364d            front-proxy-ca          no
# scheduler.conf             Jul 02, 2027 00:00 UTC   364d            ca                      no
```

### 1.2 手动检查证书有效期

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 apiserver 证书
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates

# 检查 etcd 证书
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates

# 检查 front-proxy 证书
openssl x509 -in /etc/kubernetes/pki/front-proxy-client.crt -noout -dates

# 批量检查所有证书
for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
  echo "=== $cert ==="
  openssl x509 -in "$cert" -noout -subject -dates
done

# 检查 kubeconfig 中嵌入的证书
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | \
  base64 -d | openssl x509 -noout -dates
```
### 1.3 kubelet 证书检查

```bash
# kubelet 证书通常在 /var/lib/kubelet/pki/
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 如果使用 kubeadm，kubelet 证书由 kubelet 自动轮转
# 检查 kubelet 配置是否启用了自动轮转
grep rotateCertificates /var/lib/kubelet/config.yaml
# 应为: rotateCertificates: true
```

---

## 2. kubeadm certs renew 批量轮转

### 2.1 一键轮转所有证书

```bash
# 在控制平面节点上执行
# 确保 /etc/kubernetes/pki 目录可写
kubeadm certs renew all

# 输出将显示所有已轮转的证书名称
# 新证书有效期从当前时间开始，再续 1 年（默认）
```

### 2.2 轮转单个证书

```bash
# 仅轮转 apiserver 证书
kubeadm certs renew apiserver

# 仅轮转 etcd 相关证书
kubeadm certs renew etcd-server
kubeadm certs renew etcd-peer
kubeadm certs renew etcd-healthcheck-client
kubeadm certs renew apiserver-etcd-client

# 仅轮转 front-proxy 证书
kubeadm certs renew front-proxy-client

# 仅轮转 kubeconfig 文件中的证书
kubeadm certs renew admin.conf
kubeadm certs renew controller-manager.conf
kubeadm certs renew scheduler.conf
```

### 2.3 重启控制平面组件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 证书轮转后必须重启控制平面组件才能加载新证书
# kubeadm 安装的集群通过 static pod 运行，移动 manifest 即可触发重启

# 方法一：移动 manifest 文件触发重启
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/ && sleep 20 && mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
mv /etc/kubernetes/manifests/kube-controller-manager.yaml /tmp/ && sleep 20 && mv /tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/
mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/ && sleep 20 && mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/

# etcd 也需要重启
mv /etc/kubernetes/manifests/etcd.yaml /tmp/ && sleep 20 && mv /tmp/etcd.yaml /etc/kubernetes/manifests/

# 验证组件恢复
kubectl -n kube-system get pod -l component=kube-apiserver
kubectl -n kube-system get pod -l component=kube-controller-manager
kubectl -n kube-system get pod -l component=kube-scheduler
kubectl -n kube-system get pod -l component=etcd
```
### 2.4 多控制平面节点轮转

```bash
# 对于 HA 集群（多个控制平面节点），需要逐个节点轮转
# 顺序：先 etcd 节点，再控制平面节点

# 在第一个控制平面节点上
kubeadm certs renew all
kubeadm certs renew admin.conf

# 复制更新后的证书到其他控制平面节点
for node in cp-2 cp-3; do
  scp /etc/kubernetes/pki/apiserver.crt root@${node}:/etc/kubernetes/pki/apiserver.crt
  scp /etc/kubernetes/pki/apiserver.key root@${node}:/etc/kubernetes/pki/apiserver.key
  scp /etc/kubernetes/pki/apiserver-kubelet-client.crt root@${node}:/etc/kubernetes/pki/apiserver-kubelet-client.crt
  scp /etc/kubernetes/pki/apiserver-kubelet-client.key root@${node}:/etc/kubernetes/pki/apiserver-kubelet-client.key
  scp /etc/kubernetes/pki/front-proxy-client.crt root@${node}:/etc/kubernetes/pki/front-proxy-client.crt
  scp /etc/kubernetes/pki/front-proxy-client.key root@${node}:/etc/kubernetes/pki/front-proxy-client.key
done

# 注意：etcd 证书需要在每个 etcd 节点上独立生成
# admin.conf / scheduler.conf / controller-manager.conf 需要在每个节点上分别 renew
```

---

## 3. 手动轮转证书

### 3.1 当 CA 证书本身过期时

```bash
# CA 过期是最严重的情况，kubeadm 无法自动处理
# 必须手动重新签发 CA 并重新签发所有子证书

# 备份现有 PKI
cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%Y%m%d)

# 重新生成 CA（如果 CA 确实过期）
# 注意：这会导致所有依赖旧 CA 的组件需要重新配置
openssl genrsa -out /etc/kubernetes/pki/ca.key 2048
openssl req -x509 -new -nodes -key /etc/kubernetes/pki/ca.key \
  -subj "/CN=kubernetes" -days 3650 \
  -out /etc/kubernetes/pki/ca.crt

# 重新生成 etcd CA
openssl genrsa -out /etc/kubernetes/pki/etcd/ca.key 2048
openssl req -x509 -new -nodes -key /etc/kubernetes/pki/etcd/ca.key \
  -subj "/CN=etcd-ca" -days 3650 \
  -out /etc/kubernetes/pki/etcd/ca.crt

# 重新生成 front-proxy CA
openssl genrsa -out /etc/kubernetes/pki/front-proxy-ca.key 2048
openssl req -x509 -new -nodes -key /etc/kubernetes/pki/front-proxy-ca.key \
  -subj "/CN=front-proxy-ca" -days 3650 \
  -out /etc/kubernetes/pki/front-proxy-ca.crt
```

### 3.2 手动生成 apiserver 证书

```bash
# 生成 apiserver 证书签名请求
cat > /tmp/apiserver-csr.conf <<EOF
[req]
req_extensions = v3_req
distinguished_name = req_distinguished_name
[req_distinguished_name]
[v3_req]
basicConstraints = CA:FALSE
keyUsage = digitalSignature, keyEncipherment
subjectAltName = @alt_names
[alt_names]
DNS.1 = kubernetes
DNS.2 = kubernetes.default
DNS.3 = kubernetes.default.svc
DNS.4 = kubernetes.default.svc.cluster.local
IP.1 = 10.96.0.1
IP.2 = <apiserver-lb-ip>
IP.3 = <master-node-1-ip>
IP.4 = <master-node-2-ip>
IP.5 = <master-node-3-ip>
IP.6 = 127.0.0.1
EOF

openssl genrsa -out /etc/kubernetes/pki/apiserver.key 2048
openssl req -new -key /etc/kubernetes/pki/apiserver.key \
  -subj "/CN=kube-apiserver" \
  -config /tmp/apiserver-csr.conf -out /tmp/apiserver.csr
openssl x509 -req -in /tmp/apiserver.csr \
  -CA /etc/kubernetes/pki/ca.crt -CAkey /etc/kubernetes/pki/ca.key \
  -CAcreateserial -out /etc/kubernetes/pki/apiserver.crt -days 365 \
  -extensions v3_req -extfile /tmp/apiserver-csr.conf
```

### 3.3 手动生成 etcd 证书

```bash
# etcd server 证书
openssl genrsa -out /etc/kubernetes/pki/etcd/server.key 2048
openssl req -new -key /etc/kubernetes/pki/etcd/server.key \
  -subj "/CN=etcd-server" \
  -addext "subjectAltName=DNS:localhost,DNS:<hostname>,IP:127.0.0.1,IP:<node-ip>" \
  -out /tmp/etcd-server.csr
openssl x509 -req -in /tmp/etcd-server.csr \
  -CA /etc/kubernetes/pki/etcd/ca.crt -CAkey /etc/kubernetes/pki/etcd/ca.key \
  -CAcreateserial -out /etc/kubernetes/pki/etcd/server.crt -days 365

# etcd peer 证书
openssl genrsa -out /etc/kubernetes/pki/etcd/peer.key 2048
openssl req -new -key /etc/kubernetes/pki/etcd/peer.key \
  -subj "/CN=etcd-peer" \
  -addext "subjectAltName=DNS:<hostname>,IP:<node-ip>" \
  -out /tmp/etcd-peer.csr
openssl x509 -req -in /tmp/etcd-peer.csr \
  -CA /etc/kubernetes/pki/etcd/ca.crt -CAkey /etc/kubernetes/pki/etcd/ca.key \
  -CAcreateserial -out /etc/kubernetes/pki/etcd/peer.crt -days 365
```

### 3.4 手动生成 front-proxy 证书

```bash
openssl genrsa -out /etc/kubernetes/pki/front-proxy-client.key 2048
openssl req -new -key /etc/kubernetes/pki/front-proxy-client.key \
  -subj "/CN=front-proxy-client" -out /tmp/front-proxy-client.csr
openssl x509 -req -in /tmp/front-proxy-client.csr \
  -CA /etc/kubernetes/pki/front-proxy-ca.crt -CAkey /etc/kubernetes/pki/front-proxy-ca.key \
  -CAcreateserial -out /etc/kubernetes/pki/front-proxy-client.crt -days 365
```

---

## 4. kubeconfig 更新分发

### 4.1 更新管理员 kubeconfig

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubeadm 方式一键更新所有 kubeconfig
kubeadm certs renew admin.conf
kubeadm certs renew controller-manager.conf
kubeadm certs renew scheduler.conf

# 复制新的 admin.conf 到用户目录
cp /etc/kubernetes/admin.conf ~/.kube/config
chown $(id -u):$(id -g) ~/.kube/config

# 验证
kubectl get nodes
```
### 4.2 手动生成 kubeconfig

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 如果 kubeadm 不可用，手动创建 kubeconfig

# 设置集群
kubectl config set-cluster kubernetes \
  --certificate-authority=/etc/kubernetes/pki/ca.crt \
  --embed-certs=true \
  --server=https://<apiserver-ip>:6443 \
  --kubeconfig=/etc/kubernetes/admin.conf

# 设置用户凭证
kubectl config set-credentials kubernetes-admin \
  --client-certificate=/etc/kubernetes/pki/apiserver-kubelet-client.crt \
  --client-key=/etc/kubernetes/pki/apiserver-kubelet-client.key \
  --embed-certs=true \
  --kubeconfig=/etc/kubernetes/admin.conf

# 设置上下文
kubectl config set-context kubernetes-admin@kubernetes \
  --cluster=kubernetes \
  --user=kubernetes-admin \
  --kubeconfig=/etc/kubernetes/admin.conf

# 使用该上下文
kubectl config use-context kubernetes-admin@kubernetes \
  --kubeconfig=/etc/kubernetes/admin.conf
```
### 4.3 分发 kubeconfig 到运维团队

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 安全分发方式：通过 kubectl 创建 ServiceAccount 并生成 token
# 适用于无法直接分发 admin.conf 的场景

# 创建运维 ServiceAccount
kubectl create serviceaccount ops-admin -n kube-system

# 绑定 cluster-admin 权限
kubectl create clusterrolebinding ops-admin-binding \
  --clusterrole=cluster-admin \
  --serviceaccount=kube-system:ops-admin

# 获取 token（1.24+ 需要手动创建 Secret）
cat <<EOF | kubectl apply -f -
apiVersion: v1
kind: Secret
metadata:
  name: ops-admin-token
  namespace: kube-system
  annotations:
    kubernetes.io/service-account.name: ops-admin
type: kubernetes.io/service-account-token
EOF

# 获取 token 值
kubectl -n kube-system get secret ops-admin-token -o jsonpath='{.data.token}' | base64 -d
```
### 4.4 kubelet 自动证书轮转

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# 确保 kubelet 配置启用了证书自动轮转
# /var/lib/kubelet/config.yaml 中应包含：
#   rotateCertificates: true
#   serverTLSBootstrap: true

# 如果未启用，修改后重启 kubelet
systemctl restart kubelet

# kubelet 会自动向 apiserver 签发新证书
# 需要手动 approve CSR
kubectl get csr
kubectl certificate approve <csr-name>

# 批量 approve 所有 Pending CSR
kubectl get csr -o json | jq -r '.items[] | select(.status == {}) | .metadata.name' | \
  xargs kubectl certificate approve
```
---

## 5. 生产最佳实践

### 5.1 证书有效期规划

| 证书类型 | 默认有效期 | 建议轮转周期 | 说明 |
|----------|-----------|-------------|------|
| CA 证书 | 10 年 | 每 3-5 年 | 过期影响全局 |
| apiserver | 1 年 | 每 9 个月 | 核心组件 |
| etcd | 1 年 | 每 9 个月 | 数据层 |
| kubelet | 1 年 | 自动轮转 | 启用 rotateCertificates |
| front-proxy | 1 年 | 每 9 个月 | 聚合 API |
| admin.conf | 1 年 | 每 9 个月 | 管理员凭证 |

### 5.2 自动化轮转脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
#!/bin/bash
# 证书自动轮转脚本（建议通过 cron 定期执行）
set -euo pipefail

EXPIRY_THRESHOLD_DAYS=30
LOG_FILE="/var/log/cert-rotation.log"

log() {
  echo "[$(date '+%Y-%m-%d %H:%M:%S')] $*" | tee -a "$LOG_FILE"
}

check_and_rotate() {
  local needs_rotation=false

  # 检查所有证书剩余天数
  while IFS= read -r line; do
    cert_name=$(echo "$line" | awk '{print $1}')
    residual=$(echo "$line" | grep -oP '\d+d' | head -1 | tr -d 'd')

    if [[ -n "$residual" && "$residual" -lt "$EXPIRY_THRESHOLD_DAYS" ]]; then
      log "WARNING: $cert_name 将在 ${residual} 天后过期"
      needs_rotation=true
    fi
  done < <(kubeadm certs check-expiration 2>/dev/null | tail -n +2)

  if $needs_rotation; then
    log "开始执行证书轮转..."

    # 备份
    backup_dir="/etc/kubernetes/pki.bak.$(date +%Y%m%d%H%M%S)"
    cp -r /etc/kubernetes/pki "$backup_dir"
    log "PKI 已备份到 $backup_dir"

    # 轮转
    kubeadm certs renew all
    log "证书轮转完成"

    # 重启控制平面
    for manifest in kube-apiserver kube-controller-manager kube-scheduler etcd; do
      mv "/etc/kubernetes/manifests/${manifest}.yaml" /tmp/
      sleep 15
      mv "/tmp/${manifest}.yaml" "/etc/kubernetes/manifests/"
    done
    log "控制平面组件已重启"

    # 更新 admin kubeconfig
    cp /etc/kubernetes/admin.conf ~/.kube/config
    log "admin kubeconfig 已更新"

    # Approve pending CSRs
    kubectl get csr -o json | jq -r '.items[] | select(.status == {}) | .metadata.name' | \
      xargs -r kubectl certificate approve
    log "Pending CSR 已 approve"
  else
    log "所有证书有效期充足，无需轮转"
  fi
}

check_and_rotate
```
### 5.3 监控告警

```yaml
groups:
- name: cert-alerts
  rules:
  - alert: KubernetesCertExpiringSoon
    expr: (kubernetes_cert_expiry_seconds - time()) / 86400 < 30
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "证书 {{ $labels.cn }} 将在 {{ $value }} 天后过期"

  - alert: KubernetesCertExpiryImminent
    expr: (kubernetes_cert_expiry_seconds - time()) / 86400 < 7
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "证书 {{ $labels.cn }} 将在 {{ $value }} 天后过期，需立即处理"

  - alert: KubernetesCSRApprovedRate
    expr: rate(kube_certificate_status_condition{condition="approved"}[5m]) == 0
    for: 1h
    labels:
      severity: warning
    annotations:
      summary: "过去 1 小时无新 CSR 被 approve，kubelet 证书轮转可能卡住"
```

---

## 6. 故障排查

### 6.1 常见问题对照表

| 症状 | 可能原因 | 处理方法 |
|------|---------|---------|
| `Unable to connect to the server: x509: certificate has expired` | apiserver 证书过期 | 在控制平面节点执行 `kubeadm certs renew apiserver` |
| `error: You must be logged in to the server (Unauthorized)` | admin.conf 中的证书过期 | `kubeadm certs renew admin.conf && cp /etc/kubernetes/admin.conf ~/.kube/config` |
| kubelet 无法注册到 apiserver | kubelet client cert 过期 | 检查 `/var/lib/kubelet/pki/` 并重启 kubelet |
| `x509: certificate signed by unknown authority` | CA 被替换或不匹配 | 检查 kubeconfig 中的 CA 是否与 apiserver 一致 |
| etcd 集群不健康 | etcd peer/server 证书过期 | 轮转 etcd 证书并重启 etcd |

### 6.2 证书链验证

```bash
# 验证 apiserver 证书链
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# 验证 etcd 证书链
openssl verify -CAfile /etc/kubernetes/pki/etcd/ca.crt /etc/kubernetes/pki/etcd/server.crt

# 验证 front-proxy 证书链
openssl verify -CAfile /etc/kubernetes/pki/front-proxy-ca.crt /etc/kubernetes/pki/front-proxy-client.crt

# 验证 apiserver kubelet client 证书
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver-kubelet-client.crt
```

### 6.3 etcd 证书问题专项

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 etcd 健康状态
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health

# 如果 etcd 因证书过期无法连接，直接查看 etcd 日志
journalctl -u etcd --since "10 minutes ago" --no-pager | grep -i "certificate\|tls\|x509"

# etcd 需要的证书：
#   --cert-file         → etcd server 证书
#   --key-file          → etcd server key
#   --peer-cert-file    → etcd peer 证书
#   --peer-key-file     → etcd peer key
#   --trusted-ca-file   → etcd CA
#   --peer-trusted-ca-file → etcd peer CA
#   --client-cert-auth  → 启用客户端证书认证
```
---

## 参考链接

- [kubeadm 证书管理](https://kubernetes.io/docs/tasks/administer-cluster/kubeadm/kubeadm-certs/)
- [Kubernetes PKI 证书和要求](https://kubernetes.io/docs/setup/best-practices/certificates/)
- [kubelet 证书轮转](https://kubernetes.io/docs/reference/access-authn-authz/kubelet-tls-bootstrapping/)
- [etcd 安全配置](https://etcd.io/docs/latest/op-guide/security/)


<!-- risk-assessed -->
