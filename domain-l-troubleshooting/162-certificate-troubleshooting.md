# 162 - 证书故障排查 (Certificate Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 一、Kubernetes 证书体系 (Certificate Architecture)

### 1.1 证书组件总览

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                      Kubernetes 证书架构                                     │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                         Root CA                                      │  │
│   │                    /etc/kubernetes/pki/ca.crt                        │  │
│   └────────────────────────────┬────────────────────────────────────────┘  │
│                                │                                            │
│         ┌──────────────────────┼──────────────────────┐                    │
│         │                      │                      │                    │
│         ▼                      ▼                      ▼                    │
│   ┌───────────┐         ┌───────────┐         ┌───────────┐               │
│   │API Server │         │  etcd     │         │Front Proxy│               │
│   │Certificates│        │Certificates│        │ CA        │               │
│   └───────────┘         └───────────┘         └───────────┘               │
│         │                      │                      │                    │
│         ├─ apiserver.crt       ├─ etcd/server.crt    ├─ front-proxy-ca    │
│         ├─ apiserver-kubelet   ├─ etcd/peer.crt      └─ front-proxy-client│
│         ├─ apiserver-etcd      ├─ etcd/healthcheck                        │
│         └─ sa.pub/sa.key       └─ etcd CA                                 │
│                                                                             │
│   ┌─────────────────────────────────────────────────────────────────────┐  │
│   │                       Kubelet Certificates                           │  │
│   │                 /var/lib/kubelet/pki/                                │  │
│   │   kubelet-client-current.pem  kubelet.crt  kubelet.key              │  │
│   └─────────────────────────────────────────────────────────────────────┘  │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

### 1.2 证书文件位置

| 证书 | 路径 | 用途 |
|:---|:---|:---|
| **CA证书** | /etc/kubernetes/pki/ca.crt | 集群根CA |
| **API Server** | /etc/kubernetes/pki/apiserver.crt | API Server服务端证书 |
| **API Server-Kubelet** | /etc/kubernetes/pki/apiserver-kubelet-client.crt | API Server访问Kubelet |
| **API Server-etcd** | /etc/kubernetes/pki/apiserver-etcd-client.crt | API Server访问etcd |
| **etcd Server** | /etc/kubernetes/pki/etcd/server.crt | etcd服务端证书 |
| **etcd Peer** | /etc/kubernetes/pki/etcd/peer.crt | etcd集群通信 |
| **Front Proxy** | /etc/kubernetes/pki/front-proxy-ca.crt | API聚合代理CA |
| **Kubelet** | /var/lib/kubelet/pki/kubelet-client-current.pem | Kubelet客户端证书 |
| **SA密钥** | /etc/kubernetes/pki/sa.key | ServiceAccount签名密钥 |

---

## 二、证书过期排查 (Certificate Expiration)

### 2.1 检查证书过期时间

```bash
# === kubeadm方式检查所有证书 ===
kubeadm certs check-expiration

# 输出示例:
# CERTIFICATE                EXPIRES                  RESIDUAL TIME   EXTERNALLY MANAGED
# admin.conf                 Jan 15, 2027 10:00 UTC   364d            no
# apiserver                  Jan 15, 2027 10:00 UTC   364d            no
# apiserver-etcd-client      Jan 15, 2027 10:00 UTC   364d            no
# ...

# === 手动检查单个证书 ===
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -enddate

# === 批量检查所有证书 ===
for cert in /etc/kubernetes/pki/*.crt; do
  echo "=== $cert ===" 
  openssl x509 -in $cert -noout -enddate 2>/dev/null || echo "Not a certificate"
done

# === 检查etcd证书 ===
for cert in /etc/kubernetes/pki/etcd/*.crt; do
  echo "=== $cert ==="
  openssl x509 -in $cert -noout -enddate
done

# === 检查kubelet证书 ===
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
```

### 2.2 证书过期症状

| 症状 | 过期证书 | 影响范围 |
|:---|:---|:---|
| kubectl无法连接 | admin.conf | 管理员操作 |
| Node NotReady | kubelet证书 | 节点通信 |
| API Server无法启动 | apiserver.crt | 整个集群 |
| etcd集群故障 | etcd证书 | 数据存储 |
| Pod无法创建 | controller-manager证书 | 工作负载 |
| 调度失败 | scheduler证书 | Pod调度 |

### 2.3 证书续期

```bash
# === kubeadm续期所有证书 ===
kubeadm certs renew all

# === 续期单个证书 ===
kubeadm certs renew apiserver
kubeadm certs renew apiserver-kubelet-client
kubeadm certs renew apiserver-etcd-client
kubeadm certs renew front-proxy-client
kubeadm certs renew scheduler.conf
kubeadm certs renew controller-manager.conf
kubeadm certs renew admin.conf

# === 续期后重启组件 ===
# 如果使用静态Pod
mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/
sleep 5
mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/

# 或者重启kubelet
systemctl restart kubelet
```

---

## 三、kubelet证书问题 (Kubelet Certificate Issues)

### 3.1 kubelet证书排查

```bash
# === 检查kubelet证书 ===
ls -la /var/lib/kubelet/pki/
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -text | head -30

# === 检查kubelet日志 ===
journalctl -u kubelet | grep -i "certificate\|tls\|x509"

# === 常见错误 ===
# "certificate has expired"
# "x509: certificate signed by unknown authority"
# "Unable to connect to the server: x509: certificate has expired"
```

### 3.2 kubelet证书轮换

```bash
# === 检查自动轮换配置 ===
cat /var/lib/kubelet/config.yaml | grep -i rotate

# 配置示例:
# rotateCertificates: true
# serverTLSBootstrap: true

# === 手动触发轮换 ===
# 1. 删除旧证书
rm /var/lib/kubelet/pki/kubelet-client-current.pem
rm /var/lib/kubelet/pki/kubelet.crt
rm /var/lib/kubelet/pki/kubelet.key

# 2. 重启kubelet
systemctl restart kubelet

# 3. 批准CSR (如果需要)
kubectl get csr
kubectl certificate approve <csr-name>
```

### 3.3 CSR问题排查

```bash
# === 查看待处理的CSR ===
kubectl get csr
kubectl get csr -o wide

# === 查看CSR详情 ===
kubectl describe csr <csr-name>

# === 批准CSR ===
kubectl certificate approve <csr-name>

# === 拒绝CSR ===
kubectl certificate deny <csr-name>

# === 批量批准 ===
kubectl get csr | grep Pending | awk '{print $1}' | xargs kubectl certificate approve
```

---

## 四、API Server 证书问题 (API Server Certificate Issues)

### 4.1 常见错误

```bash
# 错误1: 证书过期
# "Unable to connect to the server: x509: certificate has expired or is not yet valid"

# 错误2: 证书不信任
# "x509: certificate signed by unknown authority"

# 错误3: 主机名不匹配
# "x509: certificate is valid for kubernetes, not api.example.com"
```

### 4.2 排查命令

```bash
# === 检查API Server证书 ===
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -text

# === 检查证书SAN ===
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -ext subjectAltName

# === 检查证书签发者 ===
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -issuer

# === 验证证书链 ===
openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt

# === 测试TLS连接 ===
openssl s_client -connect <api-server>:6443 -showcerts
curl -k -v https://<api-server>:6443/healthz
```

### 4.3 添加SAN到证书

```bash
# === 如果需要添加新的SAN (主机名/IP) ===
# 1. 创建kubeadm配置
cat > kubeadm-config.yaml <<EOF
apiVersion: kubeadm.k8s.io/v1beta3
kind: ClusterConfiguration
apiServer:
  certSANs:
  - "kubernetes"
  - "kubernetes.default"
  - "kubernetes.default.svc"
  - "kubernetes.default.svc.cluster.local"
  - "10.96.0.1"           # ClusterIP
  - "192.168.1.100"       # Master IP
  - "api.example.com"     # 新增域名
  - "10.0.0.100"          # 新增IP
EOF

# 2. 备份旧证书
mv /etc/kubernetes/pki/apiserver.crt /etc/kubernetes/pki/apiserver.crt.bak
mv /etc/kubernetes/pki/apiserver.key /etc/kubernetes/pki/apiserver.key.bak

# 3. 生成新证书
kubeadm certs renew apiserver --config kubeadm-config.yaml

# 4. 重启API Server
systemctl restart kubelet
```

---

## 五、etcd 证书问题 (etcd Certificate Issues)

### 5.1 etcd证书检查

```bash
# === 检查etcd证书 ===
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -dates
openssl x509 -in /etc/kubernetes/pki/etcd/peer.crt -noout -dates
openssl x509 -in /etc/kubernetes/pki/etcd/healthcheck-client.crt -noout -dates

# === 检查etcd CA ===
openssl x509 -in /etc/kubernetes/pki/etcd/ca.crt -noout -text

# === 验证etcd证书 ===
openssl verify -CAfile /etc/kubernetes/pki/etcd/ca.crt /etc/kubernetes/pki/etcd/server.crt

# === 测试etcd连接 ===
ETCDCTL_API=3 etcdctl \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health
```

### 5.2 etcd证书续期

```bash
# === 续期etcd证书 ===
kubeadm certs renew etcd-server
kubeadm certs renew etcd-peer
kubeadm certs renew etcd-healthcheck-client

# === 重启etcd ===
# 静态Pod方式
mv /etc/kubernetes/manifests/etcd.yaml /tmp/
sleep 5
mv /tmp/etcd.yaml /etc/kubernetes/manifests/

# 或者systemd方式
systemctl restart etcd
```

---

## 六、kubeconfig 证书问题 (kubeconfig Issues)

### 6.1 检查kubeconfig证书

```bash
# === 提取kubeconfig中的证书 ===
kubectl config view --raw -o jsonpath='{.users[0].user.client-certificate-data}' | base64 -d > /tmp/client.crt
openssl x509 -in /tmp/client.crt -noout -dates

# === 检查admin.conf ===
cat /etc/kubernetes/admin.conf | grep client-certificate-data
openssl x509 -in <(cat /etc/kubernetes/admin.conf | grep client-certificate-data | awk '{print $2}' | base64 -d) -noout -dates

# === 常见kubeconfig位置 ===
# /etc/kubernetes/admin.conf
# /etc/kubernetes/scheduler.conf
# /etc/kubernetes/controller-manager.conf
# ~/.kube/config
```

### 6.2 更新kubeconfig

```bash
# === 重新生成admin.conf ===
kubeadm certs renew admin.conf

# === 更新用户kubeconfig ===
cp /etc/kubernetes/admin.conf ~/.kube/config
chown $(id -u):$(id -g) ~/.kube/config

# === 重新生成所有kubeconfig ===
kubeadm certs renew admin.conf
kubeadm certs renew scheduler.conf
kubeadm certs renew controller-manager.conf
```

---

## 七、证书问题诊断流程 (Diagnostic Flow)

### 7.1 快速诊断清单

```bash
#!/bin/bash
echo "=== 证书过期检查 ==="
kubeadm certs check-expiration 2>/dev/null || echo "kubeadm不可用,手动检查"

echo -e "\n=== API Server证书 ==="
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -enddate 2>/dev/null

echo -e "\n=== Kubelet证书 ==="
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -enddate 2>/dev/null

echo -e "\n=== etcd证书 ==="
openssl x509 -in /etc/kubernetes/pki/etcd/server.crt -noout -enddate 2>/dev/null

echo -e "\n=== 待处理CSR ==="
kubectl get csr 2>/dev/null | grep -v "Approved"

echo -e "\n=== 证书相关错误日志 ==="
journalctl -u kubelet --since "1 hour ago" | grep -i "certificate\|x509\|tls" | tail -10
```

### 7.2 常见问题解决方案

| 问题 | 症状 | 解决方案 |
|:---|:---|:---|
| **证书过期** | connection refused | `kubeadm certs renew all` |
| **CA不信任** | unknown authority | 检查CA证书路径 |
| **SAN不匹配** | hostname mismatch | 添加SAN重新签发 |
| **kubelet证书问题** | Node NotReady | 删除证书+重启kubelet |
| **CSR未批准** | Pending状态 | `kubectl certificate approve` |
| **kubeconfig过期** | kubectl报错 | 重新生成kubeconfig |

---

## 八、证书监控告警 (Certificate Monitoring)

### 8.1 Prometheus监控

```yaml
# === 使用kube-prometheus-stack ===
# 内置证书过期监控指标:
# apiserver_client_certificate_expiration_seconds_bucket
# apiserver_client_certificate_expiration_seconds_count

# 告警规则
groups:
- name: certificates
  rules:
  - alert: KubernetesClientCertificateExpiresSoon
    expr: |
      histogram_quantile(0.01, sum by (job, le) (rate(apiserver_client_certificate_expiration_seconds_bucket[5m]))) < 604800
    for: 5m
    labels:
      severity: warning
    annotations:
      summary: "Kubernetes client certificate expires in less than 7 days"
      
  - alert: KubernetesClientCertificateExpiresCritical
    expr: |
      histogram_quantile(0.01, sum by (job, le) (rate(apiserver_client_certificate_expiration_seconds_bucket[5m]))) < 86400
    for: 5m
    labels:
      severity: critical
    annotations:
      summary: "Kubernetes client certificate expires in less than 24 hours"
```

### 8.2 自动检查脚本

```bash
#!/bin/bash
# 证书过期检查脚本,可配置为cron任务

ALERT_DAYS=30
ALERT_SECONDS=$((ALERT_DAYS * 86400))

check_cert() {
    local cert_path=$1
    local cert_name=$2
    
    if [ -f "$cert_path" ]; then
        expiry_date=$(openssl x509 -in "$cert_path" -noout -enddate 2>/dev/null | cut -d= -f2)
        expiry_epoch=$(date -d "$expiry_date" +%s 2>/dev/null)
        current_epoch=$(date +%s)
        remaining=$((expiry_epoch - current_epoch))
        
        if [ $remaining -lt $ALERT_SECONDS ]; then
            echo "WARNING: $cert_name expires in $((remaining / 86400)) days"
        fi
    fi
}

# 检查关键证书
check_cert "/etc/kubernetes/pki/apiserver.crt" "API Server"
check_cert "/etc/kubernetes/pki/apiserver-kubelet-client.crt" "API Server Kubelet Client"
check_cert "/etc/kubernetes/pki/etcd/server.crt" "etcd Server"
check_cert "/var/lib/kubelet/pki/kubelet-client-current.pem" "Kubelet Client"
```

---

## 九、命令速查 (Quick Reference)

```bash
# === 证书检查 ===
kubeadm certs check-expiration
openssl x509 -in <cert> -noout -dates
openssl x509 -in <cert> -noout -text
openssl verify -CAfile <ca> <cert>

# === 证书续期 ===
kubeadm certs renew all
kubeadm certs renew <cert-name>

# === CSR管理 ===
kubectl get csr
kubectl certificate approve <csr>
kubectl certificate deny <csr>

# === TLS调试 ===
openssl s_client -connect <host>:6443 -showcerts
curl -k -v https://<host>:6443/healthz

# === kubelet证书 ===
ls -la /var/lib/kubelet/pki/
journalctl -u kubelet | grep -i cert

# === kubeconfig ===
kubectl config view --raw
kubeadm certs renew admin.conf
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
