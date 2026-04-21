# Cluster Cert — Kubernetes 集群证书体系源码分析

本模块基于 Kubernetes 官方源码（`kubernetes/kubernetes`），系统梳理集群 PKI 证书的生成逻辑、证书链架构、轮换机制及安全设计。

---

## 文档索引

| 文档 | 内容 |
|------|------|
| [01-pki-architecture](01-pki-architecture.md) | PKI 架构总览：三组 CA、证书依赖关系、信任链 |
| [02-ca-generation](02-ca-generation.md) | CA 证书生成源码分析：kubeadm CA、etcd CA、Front Proxy CA |
| [03-apiserver-cert](03-apiserver-cert.md) | API Server 证书：SAN 生成逻辑、扩展属性、证书用途 |
| [04-etcd-cert](04-etcd-cert.md) | etcd 证书体系：Server/Peer/Client 证书及健康检查证书 |
| [05-kubelet-cert](05-kubelet-cert.md) | kubelet 证书：引导证书、CSR 机制、自动轮换源码 |
| [06-cert-rotation](06-cert-rotation.md) | 证书轮换机制：kubeadm renew、kubelet 自动轮换、Controller |
| [07-service-account-keys](07-service-account-keys.md) | ServiceAccount 密钥对：JWT 签名、Token 验证、密钥轮换 |
| [08-rbac-mapping](08-rbac-mapping.md) | 证书身份到 RBAC 的映射：CommonName/Organization、front-proxy |
| [09-join-cert-flow](09-join-cert-flow.md) | kubeadm join 证书分发：Bootstrap Token、CSR、HA 证书复制 |
| [10-front-proxy-workflow](10-front-proxy-workflow.md) | Front Proxy 聚合层完整工作流：APIService、metrics-server、安全边界 |
| [11-apiserver-cert-flags](11-apiserver-cert-flags.md) | API Server 证书启动参数汇总：全量标志、验证脚本、配置陷阱 |
| [12-kubeconfig-certs](12-kubeconfig-certs.md) | kubeconfig 证书嵌入逻辑：admin/controller-manager/scheduler、Base64 编码 |
| [13-cert-config](13-cert-config.md) | kubeadm 配置对证书的影响：certSANs、CertificatesDir、controlPlaneEndpoint |
| [14-admission-webhook-certs](14-admission-webhook-certs.md) | Webhook 证书体系：caBundle、cainjector、证书轮换、故障排查 |
| [15-cert-format-encoding](15-cert-format-encoding.md) | 证书格式与编码：PEM/DER/ASN.1、X.509v3 扩展字段 |
| [16-openssl-cookbook](16-openssl-cookbook.md) | OpenSSL 速查手册：查看/验证/生成/转换/调试 |
| [17-pki-security-best-practices](17-pki-security-best-practices.md) | PKI 安全最佳实践：私钥保护、监控告警、CIS 合规 |

---

## 源码参考

- kubeadm 证书阶段: `cmd/kubeadm/app/phases/certs/`
- PKI 工具库: `cmd/kubeadm/app/util/pkiutil/`
- 通用证书库: `staging/src/k8s.io/client-go/util/cert/`
- kubelet 证书管理: `pkg/kubelet/certificate/`
- CSR Controller: `pkg/controller/certificates/`
- ServiceAccount: `pkg/serviceaccount/`

---

## 版本说明

- 基于 Kubernetes v1.28 - v1.32 源码分析
- kubeadm 证书默认有效期：1 年（CA 10 年）
- kubelet 证书自动轮换：自 v1.19 起稳定
