# 13 - 证书故障排查 (Certificate Troubleshooting)

> **适用版本**: Kubernetes v1.25-v1.32 | **最后更新**: 2026-01

---

## 1. Kubernetes 证书体系 (Certificate Architecture)

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

## 2. 证书过期排查 (Certificate Expiration)

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

## 3. kubelet证书问题 (Kubelet Certificate Issues)

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

## 4. API Server 证书问题 (API Server Certificate Issues)

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

## 5. etcd 证书问题 (etcd Certificate Issues)

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

## 6. kubeconfig 证书问题 (kubeconfig Issues)

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

## 7. 证书问题诊断流程 (Diagnostic Flow)

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

## 8. 证书监控告警 (Certificate Monitoring)

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

## 9. 命令速查 (Quick Reference)

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

## 4. 证书问题解决方案 (Certificate Solutions)

### 4.1 证书过期解决方案

#### 立即处理步骤：

1. **备份当前证书**
   ```bash
   mkdir -p /etc/kubernetes/pki.backup.$(date +%Y%m%d)
   cp -r /etc/kubernetes/pki/* /etc/kubernetes/pki.backup.$(date +%Y%m%d)/
   ```

2. **使用 kubeadm 重新签发证书**
   ```bash
   # 检查即将过期的证书
   kubeadm certs check-expiration
   
   # 重新签发所有证书
   kubeadm certs renew all
   
   # 或者只签发特定证书
   kubeadm certs renew apiserver
   kubeadm certs renew apiserver-kubelet-client
   ```

3. **重启相关组件**
   ```bash
   # 重启控制平面组件
   crictl ps | grep kube-apiserver | awk '{print $1}' | xargs crictl stop
   systemctl restart kubelet
   
   # 重启 kubelet
   systemctl restart kubelet
   ```

4. **验证证书更新**
   ```bash
   kubeadm certs check-expiration
   openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates
   ```

#### 预防措施：

1. **设置证书监控告警**
   ```bash
   # 创建证书检查脚本
   cat > /usr/local/bin/check-certificates.sh << 'EOF'
   #!/bin/bash
   THRESHOLD_DAYS=30
   
   check_cert() {
     local cert=$1
     local days=$(openssl x509 -in $cert -noout -days-left 2>/dev/null)
     if [ $? -eq 0 ] && [ $days -lt $THRESHOLD_DAYS ]; then
       echo "WARNING: Certificate $cert expires in $days days"
     fi
   }
   
   find /etc/kubernetes/pki -name "*.crt" -exec check_cert {} \;
   EOF
   
   chmod +x /usr/local/bin/check-certificates.sh
   ```

2. **配置自动续期**
   ```bash
   # 添加到 crontab
   echo "0 2 * * * /usr/local/bin/check-certificates.sh" | crontab -
   ```

### 4.2 证书验证失败解决方案

#### 常见问题处理：

1. **证书链不完整**
   ```bash
   # 检查证书链
   openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt
   
   # 修复证书链
   cat /etc/kubernetes/pki/apiserver.crt /etc/kubernetes/pki/ca.crt > /tmp/fullchain.crt
   mv /tmp/fullchain.crt /etc/kubernetes/pki/apiserver.crt
   ```

2. **证书主题不匹配**
   ```bash
   # 检查证书主题
   openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -subject
   
   # 重新生成带有正确SAN的证书
   kubeadm init phase certs apiserver --cert-dir=/etc/kubernetes/pki
   ```

3. **私钥不匹配**
   ```bash
   # 验证私钥匹配
   openssl x509 -noout -modulus -in /etc/kubernetes/pki/apiserver.crt | openssl md5
   openssl rsa -noout -modulus -in /etc/kubernetes/pki/apiserver.key | openssl md5
   
   # 如果不匹配，需要重新生成证书对
   rm /etc/kubernetes/pki/apiserver.crt /etc/kubernetes/pki/apiserver.key
   kubeadm certs renew apiserver
   ```

### 4.3 自动化证书管理脚本

#### 证书健康检查脚本：

```bash
#!/bin/bash
# certificate_health_check.sh

echo "=== Kubernetes Certificate Health Check ==="

# 检查控制平面证书
echo -e "\n--- Control Plane Certificates ---"
kubeadm certs check-expiration

# 检查 kubelet 证书
echo -e "\n--- Kubelet Certificate ---"
if [ -f /var/lib/kubelet/pki/kubelet-client-current.pem ]; then
  openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
else
  echo "Kubelet client certificate not found"
fi

# 检查 API Server 连接
echo -e "\n--- API Server Connectivity ---"
kubectl get --raw='/healthz?verbose' >/dev/null 2>&1
if [ $? -eq 0 ]; then
  echo "✓ API Server is responding"
else
  echo "✗ API Server connectivity issue"
fi

# 检查 etcd 证书
echo -e "\n--- etcd Certificates ---"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/server.crt \
  --key=/etc/kubernetes/pki/etcd/server.key \
  endpoint health
```

#### 证书自动续期脚本：

```bash
#!/bin/bash
# auto_renew_certificates.sh

LOG_FILE="/var/log/cert-renewal.log"
BACKUP_DIR="/etc/kubernetes/pki.backup.$(date +%Y%m%d_%H%M%S)"

# 日志函数
log() {
  echo "$(date '+%Y-%m-%d %H:%M:%S') - $1" | tee -a $LOG_FILE
}

# 备份证书
backup_certs() {
  log "Backing up certificates to $BACKUP_DIR"
  mkdir -p $BACKUP_DIR
  cp -r /etc/kubernetes/pki/* $BACKUP_DIR/
}

# 检查过期证书
check_expiring_certs() {
  log "Checking for expiring certificates"
  local expiring_certs=$(kubeadm certs check-expiration | grep -E "(expires in [0-9]+ days|EXPIRED)")
  if [ -n "$expiring_certs" ]; then
    log "Found expiring certificates:"
    echo "$expiring_certs" | tee -a $LOG_FILE
    return 0
  else
    log "No certificates expiring soon"
    return 1
  fi
}

# 续期证书
renew_certificates() {
  log "Renewing certificates"
  backup_certs
  
  # 续期所有证书
  kubeadm certs renew all
  
  if [ $? -eq 0 ]; then
    log "Certificate renewal completed successfully"
    return 0
  else
    log "Certificate renewal failed"
    return 1
  fi
}

# 重启服务
restart_services() {
  log "Restarting services"
  
  # 重启 kubelet
  systemctl restart kubelet
  
  # 重启 API Server 容器
  crictl ps | grep kube-apiserver | awk '{print $1}' | xargs crictl stop
  
  log "Services restarted"
}

# 主函数
main() {
  log "Starting certificate renewal process"
  
  if check_expiring_certs; then
    if renew_certificates; then
      restart_services
      log "Certificate renewal process completed"
    else
      log "Certificate renewal failed, restoring backup"
      cp -r $BACKUP_DIR/* /etc/kubernetes/pki/
      systemctl restart kubelet
    fi
  else
    log "No certificate renewal needed"
  fi
}

# 执行主函数
main
```

---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)
