# 节点证书轮换

## 源码路径

`pkg/kubelet/certificate/`
`pkg/kubelet/kubelet.go`

---

## kubelet 证书轮换机制

```bash
# kubelet 使用两种 kubeconfig:
# 1. /etc/kubernetes/bootstrap-kubelet.conf (首次启动)
# 2. /etc/kubernetes/kubelet.conf (证书签发后)

# kubelet.conf 中的证书位于:
# /var/lib/kubelet/pki/kubelet-client-*.pem
```

---

## 自动证书轮换

```yaml
# /var/lib/kubelet/config.yaml
rotateCertificates: true  # 默认开启

# 当证书剩余有效期 < 80% 时，kubelet 自动发起 CSR 续期
# 无需手动干预
```

---

## 手动证书轮换

```bash
# 强制 kubelet 轮换证书
curl -X POST "https://localhost:10250/rotate certificates"

# 需要先创建 CSR 并 approve
kubectl get csr | grep kubelet
kubectl certificate approve <csr-name>
```

---

## Bootstrap Token 过期

```bash
# Token 默认 24 小时后过期
# 过期后 kubelet 无法续期证书

# 解决方案:
# 1. 创建新 Token
kubeadm token create

# 2. 更新 bootstrap-kubelet.conf
#    (kubelet 会自动使用新 Token 申请证书)

# 3. 或者禁用 Token 过期 (不推荐)
#    在 API Server 端设置较长的 BootstrapTokenTTL
```

---

## 证书查看

```bash
# 查看 kubelet 证书
openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates

# 对比 /etc/kubernetes/kubelet.conf
cat /etc/kubernetes/kubelet.conf | grep client-certificate-data | base64 -d | openssl x509 -noout -dates
```

---

## 常见问题

| 问题 | 原因 | 解决 |
|------|------|------|
| 证书过期后 kubelet 无法连接 | Token/证书过期 | 重建 bootstrap-kubelet.conf |
| CSR 一直是 Pending | csrapproving controller 问题 | 手动 approve |
| 证书与 API Server 不匹配 | API Server CA 变更 | 更新 /etc/kubernetes/pki/ca.crt |
