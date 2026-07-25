---
title: Kubernetes 证书与 PKI 生命周期运维 Runbook
description: 覆盖 kubeadm 内部 CA/证书轮换、cert-manager CA 轮换、Ingress/mTLS 证书过期监控、应急破窗轮换及告警规则的生产级运维手册
summary: 覆盖 kubeadm 内部 CA/证书轮换、cert-manager CA 轮换、Ingress/mTLS 证书过期监控、应急破窗轮换及告警规则的生产级运维手册
category: cluster
tags:
- production
- best-practices
- playbook
- security
- certificate
- pki
tier: core
created: '2026-07-01'
last_updated: '2026-07'
difficulty: advanced
reading_level: advanced
audience:
- SRE
- 运维工程师
- 平台工程师
estimated_read_time: 25min
intent_queries:
- Kubernetes 证书与 PKI 生命周期运维 Runbook 是什么
- 如何轮换 kubeadm 集群证书
- cert-manager CA 轮换步骤
- Ingress TLS 证书过期怎么处理
- Kubernetes mTLS 证书如何监控
- 证书过期应急破窗轮换
trigger_keywords:
- certificate
- pki
- kubeadm certs
- cert-manager
- tls rotation
- mtls
- cert-expiry
- ca rotation
- front-proxy
- kubelet client cert
prerequisites:
- kubectl-basics
- kubeadm-basics
- cert-manager-basics
- prometheus-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
- '1.33'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Kubernetes 证书与 PKI 生命周期运维 Runbook

> **适用范围**: Kubernetes v1.28–v1.33 | **最后更新**: 2026-07 | **文档类型**: 生产运维 Runbook

本 Runbook 聚焦 Kubernetes 集群 PKI 体系的完整生命周期管理，覆盖控制平面组件证书（apiserver、etcd、front-proxy、kubelet 客户端证书）、cert-manager 自签/中间 CA、Ingress 与 Service Mesh mTLS 证书的轮换、监控及应急破窗流程。证书过期是生产环境高频 P0/P1 诱因，必须在到期前 30 天进入主动轮换窗口，并保留可回滚的备份与验证清单。与单纯的命令速查不同，本手册强调“先评估影响范围、再执行轮换、最后验证信任链”的闭环思维，适用于需要保障高可用与合规性的生产 Kubernetes 平台。

Kubernetes 的 PKI 体系由多组相互独立的 CA 与 leaf 证书构成。控制平面默认包含四个根 CA：`ca` 用于签发 apiserver、kubelet 客户端、controller-manager、scheduler、admin 等证书；`etcd/ca` 用于 etcd 成员间及 apiserver-etcd-client 证书；`front-proxy-ca` 用于签发聚合层 front-proxy-client 证书；`sa` 用于 ServiceAccount token 签名。任何一张 CA 或 leaf 证书的过期都会引发链式故障，轻则监控失联，重则整个集群 API 不可达。因此，证书生命周期管理必须纳入平台的常规运维节奏，而不是临时应急操作。

---

## 1. 适用场景与范围

- **kubeadm 集群内部证书**：CA、apiserver、apiserver-kubelet-client、apiserver-etcd-client、etcd peer/server、front-proxy、scheduler/controller-manager kubeconfig、admin.conf。
- **kubelet 服务端/客户端证书**：由内置 CSR 审批或外部 CA 签发，需监控节点 `/var/lib/kubelet/pki` 目录。
- **cert-manager 证书体系**：ClusterIssuer / Issuer、自签 CA、中间 CA、Certificate 资源，以及 `ca.crt` 滚动更新后的工作负载热加载。
- **Ingress / Gateway TLS 与 mTLS**：NGINX Ingress、Gateway API、Istio/Linkerd SPIFFE 证书，包含公网域名与内部服务网格双向 TLS。
- **破窗场景**：证书已过期导致 API Server 拒绝连接、kubelet 无法注册、聚合层不可用时的紧急轮换。

不适用外部云厂商托管集群（如 EKS/GKE/ACK）默认托管 CA 的场景，但 Ingress / 工作负载证书部分仍然适用。

### 1.1 Kubernetes PKI 组件关系

在生产环境中，建议将以下证书纳入统一台账管理：

- **控制平面 CA 与 leaf 证书**：位于 `/etc/kubernetes/pki`，由 kubeadm 初始化生成，有效期多为 1 年（CA 为 10 年）。
- **kubelet 服务端/客户端证书**：位于 `/var/lib/kubelet/pki`，客户端证书默认由 kube-controller-manager 自动 rotate。
- **cert-manager 签发的工作负载证书**：以 Secret 形式存储于业务命名空间，通常由 Certificate 资源声明式管理。
- **服务网格 mTLS 证书**：Istio 由 istiod/Citadel 签发 SPIFFE 证书；Linkerd 由 identity 组件签发，默认 24 小时滚动。
- **外部 Ingress TLS 证书**：由 cert-manager 对接 Let's Encrypt、内部 CA 或云厂商证书服务签发。

建立台账时应记录证书名称、路径、签发 CA、有效期、负责人、轮换窗口与关联组件。台账应每两周自动由脚本更新并推送至 Wiki 或 Git 仓库。

---

## 2. 前置条件与工具

### 2.1 权限与访问

- 对控制平面节点拥有 root 或具备 `sudo` 权限的 SSH 访问。
- 当前 kubeconfig 拥有 `cluster-admin` 角色。
- 若使用 cert-manager，需具备对 `cert-manager.io` 资源的读写权限。

### 2.2 必备工具

| 工具 | 用途 | 推荐版本 |
|------|------|----------|
| `kubeadm` | 集群内部证书检查与轮换 | 与集群控制平面版本一致 |
| `openssl` | 证书有效期、SAN、签发链验证 | 1.1.1+ / 3.x |
| `etcdctl` | etcd 成员证书健康检查 | v3.5+ |
| `kubectl` | 查看 CSR、Secret、Certificate 资源 | v1.28+ |
| `helm` / `cmctl` | cert-manager 与证书状态诊断 | cmctl v2+ |
| `velero` / `rsync` | 证书目录备份 | 最新稳定版 |

### 2.3 变更窗口与通报

- 常规轮换：建议安排在业务低峰期，预留 60 分钟回退时间。轮换前 48 小时在变更平台提交变更工单，并通知所有应用 Owner 与值班负责人。
- CA 根证书轮换：必须申请变更窗口，影响所有依赖该 CA 的组件，需按灰度分批执行。此类变更应纳入月度重大变更评审会议，明确回滚决策人与升级路径。
- 应急破窗：证书已过期导致服务不可用时，立即按 P0 事故响应流程执行，无需等待常规变更窗口，但事后必须补齐变更记录与复盘。

---

## 3. 标准操作流程

### 3.1 证书全景扫描与基线

在每次轮换前，必须先建立完整的证书基线清单。基线不仅是后续验证的对比依据，也是事故调查时的关键证据。建议在变更窗口开始前 48 小时完成扫描，并将结果保存到变更工单与对象存储中。

扫描范围应包括：控制平面节点 `/etc/kubernetes/pki` 下的所有证书与 CA、所有 kubeconfig 文件中的客户端证书、每个工作节点的 kubelet 客户端/服务端证书、cert-manager 管理的 Certificate 资源、Ingress/Gateway 使用的 TLS Secret、Istio/Linkerd 的网格根证书与 workload 证书。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. kubeadm 证书过期总览（控制平面节点执行）
kubeadm certs check-expiration

# 2. 所有 kubeconfig 引用的客户端证书
for f in /etc/kubernetes/*.conf; do
  echo "=== $f ==="
  kubectl --kubeconfig=$f config view --raw -o jsonpath='{$.users[0].user.client-certificate-data}' | base64 -d | openssl x509 -noout -dates -subject
  echo
done

# 3. 节点 kubelet 证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates -subject

# 4. cert-manager 证书资源
kubectl get certificate -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\t"}{.status.notAfter}{"\n"}{end}'

# 5. 检查 SAN 是否包含当前控制平面入口
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text | grep -A1 "Subject Alternative Name"
```
记录输出到变更工单，并同步到 `_meta/journal/` 的证书巡检记录。对于有效期小于 60 天的证书，应标记为黄色预警；小于 30 天应升级为红色预警并立即排期轮换。

### 3.2 kubeadm 内部证书轮换

kubeadm 默认签发的 leaf 证书有效期为 1 年，CA 有效期为 10 年。建议每 10 个月执行一次主动 leaf 证书轮换，避免春节假期或业务高峰前集中到期。轮换顺序应为：先第一个控制平面节点，再其余控制平面节点，最后工作节点 kubeconfig 与 kubelet 证书（后两者多为自动）。

#### 步骤 A：备份 /etc/kubernetes/pki

在执行任何写操作前，必须对 `/etc/kubernetes/pki` 与所有 kubeconfig 进行完整备份。备份文件应加密后上传至异地对象存储，并验证可解压。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
TS=$(date +%Y%m%d-%H%M%S)
tar czvf /root/pki-backup-${TS}.tar.gz /etc/kubernetes/pki /etc/kubernetes/*.conf
# 计算校验和并记录
sha256sum /root/pki-backup-${TS}.tar.gz > /root/pki-backup-${TS}.sha256
# 异地副本
aws s3 cp /root/pki-backup-${TS}.tar.gz s3://<cluster-backup-bucket>/pki/
aws s3 cp /root/pki-backup-${TS}.sha256 s3://<cluster-backup-bucket>/pki/
```
#### 步骤 B：执行非破坏性轮换（不轮换 CA）

`kubeadm certs renew all` 会一次性更新所有 leaf 证书，但不更新 CA。执行后必须重启加载证书的静态 Pod。

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
# 在第一个控制平面节点执行
kubeadm certs renew all

# 重启静态 Pod 以加载新证书
systemctl restart kubelet
# 或容器运行时重载（慎用，确保 Pod 能自动重建）
sudo crictl pods -n kube-system -q | xargs -I{} sudo crictl stopp {} && sudo crictl rmp {}
```
#### 步骤 C：更新 kubeconfig

leaf 证书更新后，所有 kubeconfig 文件中的客户端证书也必须同步更新，否则运维人员与 CI/CD 会因证书过期而无法连接集群。

```bash
kubeadm init phase kubeconfig all --kubeconfig-dir /etc/kubernetes
```

#### 步骤 D：对其他控制平面节点重复

逐个节点执行步骤 B/C，严禁并行操作。每完成一个节点需验证 API Server 与 etcd 健康：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get --raw=/healthz
etcdctl endpoint health --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
```
若某一节点轮换后 API Server 无法启动，应立即停止后续节点操作，先隔离并恢复该节点。

### 3.3 etcd 成员证书轮换

etcd 成员间通过 peer 证书通信，apiserver 通过 apiserver-etcd-client 证书访问 etcd。当这些证书接近过期时，建议单独轮换，以便更精细地控制影响范围。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 单独轮换 etcd 相关证书
kubeadm certs renew apiserver-etcd-client
kubeadm certs renew etcd-healthcheck-client
kubeadm certs renew etcd-peer
kubeadm certs renew etcd-server

# 验证 etcd 成员健康
ETCDCTL_API=3 etcdctl endpoint health --cluster \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 检查 peer 证书有效期
openssl x509 -in /etc/kubernetes/pki/etcd/peer.crt -noout -dates -subject
```
etcd 证书轮换应在低写入时段执行，并密切监控 `etcd_server_leader_changes_seen_total` 与 `etcd_disk_wal_fsync_duration_seconds`。若 leader 频繁切换，应暂停后续节点操作。

### 3.4 kubelet 客户端证书轮换

kubelet 客户端证书默认由 kube-controller-manager 自动续期，通常无需人工干预。但在以下场景需要主动检查或手动触发：

- 自定义 CSR 审批 controller 异常，导致证书 pending。
- 节点长时间 NotReady，无法自动续期。
- 使用外部 CA（如 Vault、cfssl）签发 kubelet 证书。

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
# 检查 kubelet 证书有效期
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 手动触发 CSR 生成（kubelet 会自动创建）
systemctl restart kubelet

# 批准 pending CSR
kubectl get csr | grep Pending | awk '{print $1}' | xargs kubectl certificate approve

# 若使用外部 CA（cfssl 示例）
cfssl gencert -ca=/etc/kubernetes/pki/ca.crt \
  -ca-key=/etc/kubernetes/pki/ca.key \
  -config=ca-config.json \
  -profile=client kubelet-csr.json | cfssljson -bare kubelet-client
```
轮换后需确认 `kubelet-client-current.pem` 指向新证书，且节点状态变为 Ready。

### 3.5 CA 根证书轮换（高危）

kubeadm 默认 CA 有效期为 10 年，若合规要求提前轮换，必须按 CA 替换流程执行。CA 轮换是所有证书操作中最危险的一类，因为它会同时影响 apiserver、kubelet、etcd、front-proxy、metrics-server、聚合 API 以及所有外部客户端的 kubeconfig。

推荐采用“双 CA 并行信任”的渐进式方案：

1. 使用新 CA 并行生成新证书链，保留旧 CA 作为信任锚。
2. 将新 CA 证书追加到所有客户端 `ca.crt` 信任 bundle（apiserver、kubelet、etcd、front-proxy-ca）。
3. 使用 `kubeadm alpha certs renew` 或手动 CSR 签发新 leaf 证书。
4. 在旧证书过期前完成全集群组件重启，再移除旧 CA。

> 注意：CA 轮换会导致所有 kubeconfig、kubelet、聚合层、metrics-server 短暂不信任，必须在维护窗口执行。执行前必须在至少一个非生产集群完成端到端演练，演练内容包括 CA 替换、leaf 重签、组件重启、kubeconfig 分发、聚合层验证与回滚。

对于大规模集群，建议分批次执行：先选择一个边缘测试命名空间所在节点，再逐步扩展到核心控制平面。整个过程应由 Incident Commander 统一协调，任何异常立即暂停并回滚。

### 3.6 cert-manager CA 轮换

#### 自签 CA 轮换

```yaml
# 1. 创建新的 CA Secret，标记为 is-ca: true
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: internal-ca-v2
  namespace: cert-manager
spec:
  isCA: true
  commonName: "KUDIG Internal CA V2"
  secretName: internal-ca-v2
  issuerRef:
    name: selfsigned-issuer
    kind: ClusterIssuer
  duration: 8760h
  renewBefore: 720h
---
# 2. 使用新 CA 的 ClusterIssuer
apiVersion: cert-manager.io/v1
kind: ClusterIssuer
metadata:
  name: internal-ca-v2-issuer
spec:
  ca:
    secretName: internal-ca-v2
```

切换工作负载 Certificate 的 `issuerRef` 到新 Issuer 后，cert-manager 会自动签发新 leaf 证书并更新 Secret。对于挂载了 TLS Secret 的 Pod，若应用不支持文件热加载，需滚动重启：

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
kubectl rollout restart deployment/<app> -n <ns>
```
对于 Java 等将证书加载到 JVM  truststore 的应用，除重启外还需更新 truststore 中的 CA 证书。建议在 ConfigMap 中维护 CA bundle，并通过 init container 在启动时重新生成 truststore。

#### 检查轮换状态

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl get certificate -A
kubectl describe certificate <name> -n <ns>
cmctl status certificate <name> -n <ns>

# 检查 Secret 中的证书链
kubectl get secret <secret-name> -n <ns> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl crl2pkcs7 -nocrl -certfile /dev/stdin | openssl pkcs7 -print_certs -noout
```
#### CA 轮换期间的双信任策略

在将旧 CA 完全替换为新 CA 前，必须确保所有客户端 truststore 同时包含新旧两个 CA 的公钥。这样可以避免在 leaf 证书由新 CA 签发、而客户端仍只信任旧 CA 时出现 TLS 握手失败。待所有工作负载完成重启并稳定运行一个证书有效期周期后，再移除旧 CA。

### 3.7 Ingress / mTLS 证书过期监控与轮换

Ingress 与 Service Mesh 的证书面向外部用户或东西向服务间通信，过期会直接导致业务中断或安全策略失效。建议将 Ingress TLS 证书统一接入 cert-manager，并通过 Prometheus 监控所有 Certificate 资源的 `notAfter` 与 `Ready` 状态。

#### Ingress TLS 轮换

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 通过 cert-manager 自动续期（推荐）
kubectl get ingress -A -o jsonpath='{range .items[*]}{.metadata.namespace}{"/"}{.metadata.name}{"\n"}{end}'
```
确保 Certificate 资源配置了正确的 DNS 名称与 Issuer。对于通配符域名或多 SAN 域名，应在 `dnsNames` 中完整列出，避免遗漏导致续期失败：

```yaml
apiVersion: cert-manager.io/v1
kind: Certificate
metadata:
  name: api-example-com
  namespace: production
spec:
  secretName: api-example-com-tls
  issuerRef:
    name: letsencrypt-prod
    kind: ClusterIssuer
  dnsNames:
  - api.example.com
  duration: 2160h
  renewBefore: 360h
```

#### mTLS 证书监控（Istio）

Istio 使用 istiod 作为证书签发机构，为每个 workload 签发短期 SPIFFE 证书（默认 24 小时）。虽然短期证书降低了过期风险，但根证书与中间 CA 的过期仍会导致整个网格信任链断裂。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查 Citadel/istiod 根证书有效期
kubectl get secret istio-ca-secret -n istio-system -o jsonpath='{.data.ca-cert\.pem}' | base64 -d | openssl x509 -noout -dates

# 工作负载证书有效期（进入 Pod）
openssl x509 -in /etc/certs/cert-chain.pem -noout -dates

# 检查 istiod 证书分发状态
istioctl proxy-status
```
若使用 SPIRE 替代 Citadel，应监控 SPIRE Server 的 CA 有效期与 SVID 签发成功率，并在 CA 过期前 30 天启动轮换。

### 3.8 破窗轮换（证书已过期）

当 `kubeadm certs check-expiration` 显示 EXPIRED，且 API Server 不可用时，集群已处于不可用状态，必须立即启动破窗恢复。此时无法通过常规 `kubectl` 操作，只能直接登录控制平面节点。

1. SSH 登录第一个控制平面节点。
2. 备份 PKI。
3. 设置系统时间回退（仅作为临时恢复手段，风险极高）：
   ```bash
   date -s "2025-06-01 00:00:00"
   ```
4. 执行 `kubeadm certs renew all` 并重启 kubelet。
5. 分发新的 admin.conf 到运维堡垒机与 CI/CD。
6. 恢复 NTP 同步，验证证书有效期已延长。

> 时间回退会触发审计异常与日志时间戳混乱，仅用于争取恢复窗口，事后必须出具专项复盘。破窗恢复完成后，应在 24 小时内对所有控制平面与工作节点执行一次完整的主动轮换，确保所有证书链一致且有效期充足。

破窗场景的预防胜于治疗。生产环境应至少配置两层防护：一是 Prometheus 证书过期告警，二是每季度一次的证书基线审计。将证书有效期纳入平台健康度看板，可显著降低证书过期事故的发生概率。

### 3.9 证书过期告警规则

除了人工巡检，还应在 Prometheus 中配置主动告警。以下规则覆盖 kubeadm 证书、cert-manager 证书与 Ingress TLS 证书。

```yaml
groups:
- name: certificate-expiry
  rules:
  - alert: KubeadmCertificateExpiringSoon
    expr: |
      (
        kubeadm_certificate_expiration_timestamp_seconds - time()
      ) / 86400 < 30
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Kubeadm 证书 {{ $labels.name }} 将在 30 天内过期"
      description: "证书剩余天数：{{ $value | humanize }}"
  - alert: CertManagerCertificateNotReady
    expr: |
      certmanager_certificate_ready_status{condition="False"} == 1
    for: 10m
    labels:
      severity: critical
    annotations:
      summary: "cert-manager 证书 {{ $labels.name }} 未就绪"
  - alert: IngressTLSCertificateExpiringSoon
    expr: |
      (
        certmanager_certificate_expiration_timestamp_seconds - time()
      ) / 86400 < 14
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Ingress TLS 证书 {{ $labels.name }} 将在 14 天内过期"
```

告警阈值应根据业务容忍度调整。对于生产核心集群，建议 60 天黄色预警、30 天红色预警、7 天 P0 应急；对于测试环境，可适当放宽至 14 天预警。

---

## 4. 关键检查点与验证命令

| 检查项 | 命令 | 合格标准 |
|--------|------|----------|
| kubeadm 证书有效期 | `kubeadm certs check-expiration` | 所有证书剩余 > 30 天 |
| API Server 响应 | `kubectl get --raw=/healthz` | 返回 `ok` |
| etcd 成员健康 | `etcdctl endpoint health --cluster` | 所有 endpoint healthy |
| kubelet 注册 | `kubectl get nodes` | 所有节点 Ready |
| CSR 审批状态 | `kubectl get csr` | 无 Pending 超过 5 分钟 |
| cert-manager 证书 | `kubectl get certificate -A` | Ready=True |
| Ingress TLS 有效期 | `kubectl get certificate -A` | notAfter > 30 天 |
| 聚合层可用 | `kubectl get --raw=/apis/metrics.k8s.io` | 返回 API 列表 |

---

## 5. 回滚/应急方案

- **单节点轮换失败**：从备份恢复 `/etc/kubernetes/pki` 与该节点 kubeconfig，重启 kubelet。
  ```bash
  tar xzvf /root/pki-backup-<TS>.tar.gz -C /
  systemctl restart kubelet
  ```
  恢复后需再次验证该节点 API Server 与 etcd 成员健康，确认无证书时间戳异常。
- **API Server 无法启动**：检查静态 Pod 日志 `/var/log/pods/kube-system_kube-apiserver-*` 中证书加载错误，确认 SAN 包含控制平面 VIP/LB 地址。常见错误是 SAN 遗漏了新的 LB IP 或域名，需要重新生成 apiserver 证书。
- **kubelet 证书未续期**：手动批准 CSR。
  ```bash
  kubectl get csr | grep Pending | awk '{print $1}' | xargs kubectl certificate approve
  ```
  若大量 CSR 同时出现 Pending，可能是 kube-controller-manager 的 CSR 审批 controller 异常，需检查 controller-manager 日志。
- **cert-manager 证书未 Ready**：检查 Challenge、Order、Issuer 状态，确认 DNS-01 / HTTP-01 可达性。Let's Encrypt 的失败次数存在速率限制，连续失败会导致域名被临时封禁。
- **全集群证书灾难**：参考 [[12-可靠性/02-灾难恢复/19-certificate-expiry-mass-rotation-playbook.md|集群证书批量过期紧急轮转]]。该场景下通常需要 SSH 登录控制平面节点，在时间回退或救援模式下完成证书续期。

---

## 6. 风险与注意事项

1. **CA 轮换是破坏性操作**：所有依赖旧 CA 的 kubeconfig、Webhook、metrics-server、aggregation layer 必须同步更新信任锚。建议在测试集群完整演练一次后再上生产。
2. **kubelet 证书默认自动轮换**：由 kube-controller-manager 自动批准并写入 `/var/lib/kubelet/pki/kubelet-client-current.pem`，勿手动删除该符号链接。若 CSR 审批被自定义 controller 接管，需确保审批逻辑不会因证书主题变更而拒绝。
3. **时间同步是证书信任根基**：所有节点必须配置 NTP/Chrony，时间漂移 > 5 分钟会导致 TLS 握手失败。建议在所有节点启用 Chrony 并配置多个上游时间源。
4. **cert-manager 证书 Secret 更新≠应用热加载**：大多数 Java/Go 应用需滚动重启或实现文件 watcher。对于无法重启的有状态服务，应在设计阶段引入证书重载机制。
5. **保留旧 CA 双信任**：CA 轮换期间务必将新旧 CA 同时加入 trust bundle，避免灰度期间服务中断。轮换完成后需在所有信任锚中彻底移除旧 CA，防止过期 CA 成为隐患。
6. **聚合层证书易被忽略**：metrics-server、metrics-adapter、自定义 API server 等聚合组件使用 front-proxy-client 证书，轮换后需重启这些组件。
7. **etcd 证书轮换需逐个成员进行**：etcd 集群对证书变更有严格的一致性要求，建议在低写入时段执行，并监控 leader 切换情况。

---

## 7. 相关 Runbook / 推荐阅读

- [[01-集群基础/00-总览/99-production-readiness-operations-guide.md|集群基础 生产就绪运维指南]]
- [[08-安全/00-总览/99-production-readiness-operations-guide.md|安全合规 生产就绪运维指南]]
- [[12-可靠性/00-总览/99-production-readiness-operations-guide.md|可靠性工程 生产就绪运维指南]]
- [[12-可靠性/02-灾难恢复/19-certificate-expiry-mass-rotation-playbook.md|集群证书批量过期紧急轮转]]
- [[01-集群基础/03-控制平面/32-kubeadm-cluster-lifecycle.md|kubeadm 集群生命周期管理]]
- [[01-集群基础/03-控制平面/19-etcd-operations.md|etcd 运维操作]]


<!-- risk-assessed -->
