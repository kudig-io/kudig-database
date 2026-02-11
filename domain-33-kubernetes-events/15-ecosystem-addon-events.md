# 15 - 生态系统与插件事件

> **适用版本**: Kubernetes v1.25 - v1.32 | **最后更新**: 2026-02 | **作者**: Allen Galler

> **本文档记录 Kubernetes 生态系统中常见生产插件和扩展组件产生的事件,包括 Node Problem Detector、Ingress 控制器、cert-manager、Istio、ArgoCD、Knative、Prometheus Operator、Velero、External DNS 和 MetalLB。**

---

## 目录

- [一、事件总览](#一事件总览)
- [二、Node Problem Detector 事件](#二node-problem-detector-事件)
- [三、Ingress 控制器事件 (NGINX)](#三ingress-控制器事件-nginx)
- [四、cert-manager 证书管理事件](#四cert-manager-证书管理事件)
- [五、Istio Service Mesh 事件](#五istio-service-mesh-事件)
- [六、ArgoCD GitOps 事件](#六argocd-gitops-事件)
- [七、Knative Serverless 事件](#七knative-serverless-事件)
- [八、Prometheus Operator 事件](#八prometheus-operator-事件)
- [九、Velero 备份恢复事件](#九velero-备份恢复事件)
- [十、External DNS 事件](#十external-dns-事件)
- [十一、MetalLB 负载均衡事件](#十一metallb-负载均衡事件)
- [十二、生产环境最佳实践](#十二生产环境最佳实践)

---

## 一、事件总览

### 1.1 生态插件事件全景图

| 组件分类 | 组件名称 | 事件数量 | 主要关注场景 | 生产重要性 |
|:---|:---|:---:|:---|:---|
| **节点诊断** | Node Problem Detector | 10 | 节点内核、运行时、文件系统异常 | ⭐⭐⭐⭐⭐ |
| **流量网关** | NGINX Ingress Controller | 6 | Ingress 配置同步、规则更新 | ⭐⭐⭐⭐⭐ |
| **证书管理** | cert-manager | 8 | TLS 证书签发、续期、失败 | ⭐⭐⭐⭐⭐ |
| **服务网格** | Istio | 5 | Sidecar 注入、Envoy 配置 | ⭐⭐⭐⭐ |
| **GitOps** | ArgoCD | 7 | 应用同步、健康检查、漂移检测 | ⭐⭐⭐⭐⭐ |
| **Serverless** | Knative | 5 | Revision 部署、流量路由 | ⭐⭐⭐ |
| **监控管理** | Prometheus Operator | 4 | 规则同步、配置重载 | ⭐⭐⭐⭐ |
| **备份恢复** | Velero | 5 | 备份执行、恢复操作 | ⭐⭐⭐⭐⭐ |
| **DNS 集成** | External DNS | 4 | DNS 记录同步 | ⭐⭐⭐⭐ |
| **裸金属 LB** | MetalLB | 4 | IP 地址分配、BGP 宣告 | ⭐⭐⭐⭐ |

### 1.2 本文档覆盖的事件列表

| 事件原因 (Reason) | 类型 | 组件 | 适用版本 | 生产频率 | 简要说明 |
|:---|:---|:---|:---|:---|:---|
| **Node Problem Detector** | | | | | |
| `KernelOops` | Warning | node-problem-detector | v0.8+ | 罕见 | 内核 Oops 错误 |
| `DockerHung` / `ContainerdHung` | Warning | node-problem-detector | v0.8+ | 低频 | 容器运行时挂起 |
| `ReadonlyFilesystem` | Warning | node-problem-detector | v0.8+ | 低频 | 文件系统变为只读 |
| `CorruptDockerOverlay2` | Warning | node-problem-detector | v0.8+ | 罕见 | overlay2 文件系统损坏 |
| `TaskHung` | Warning | node-problem-detector | v0.8+ | 低频 | 任务挂起超时 |
| `UnregisterNetDevice` | Warning | node-problem-detector | v0.8+ | 低频 | 网络设备注销异常 |
| `KernelDeadlock` | Warning | node-problem-detector | v0.8+ | 罕见 | 内核死锁 |
| `OOMKilling` | Warning | node-problem-detector | v0.8+ | 中频 | 内核 OOM Killer 触发 |
| `FilesystemCorruption` | Warning | node-problem-detector | v0.8+ | 罕见 | 文件系统损坏 |
| **NGINX Ingress Controller** | | | | | |
| `Sync` | Normal | ingress-nginx | v0.20+ | 高频 | Ingress 配置同步成功 |
| `CREATE` / `UPDATE` / `DELETE` | Normal | ingress-nginx | v0.20+ | 中频 | Ingress 规则生命周期操作 |
| `Rejected` | Warning | ingress-nginx | v0.20+ | 低频 | Ingress 规则被拒绝 |
| `AddedOrUpdated` | Normal | ingress-nginx | v0.20+ | 高频 | Ingress 添加或更新 |
| **cert-manager** | | | | | |
| `Issuing` | Normal | cert-manager | v1.0+ | 中频 | 正在签发证书 |
| `Issued` | Normal | cert-manager | v1.0+ | 中频 | 证书签发成功 |
| `Ready` | Normal | cert-manager | v1.0+ | 中频 | 证书已就绪 |
| `NotReady` | Warning | cert-manager | v1.0+ | 低频 | 证书未就绪 |
| `IssuerNotReady` | Warning | cert-manager | v1.0+ | 低频 | 签发者未就绪 |
| `OrderCreated` | Normal | cert-manager | v1.0+ | 中频 | ACME Order 已创建 |
| `OrderComplete` | Normal | cert-manager | v1.0+ | 中频 | ACME Order 完成 |
| `RenewalScheduled` | Normal | cert-manager | v1.0+ | 低频 | 证书续期已调度 |
| **Istio** | | | | | |
| `InjectionSucceeded` | Normal | istio-sidecar-injector | v1.6+ | 高频 | Sidecar 注入成功 |
| `InjectionFailed` | Warning | istio-sidecar-injector | v1.6+ | 低频 | Sidecar 注入失败 |
| `ProxyConfigChanged` | Normal | istiod | v1.6+ | 中频 | Envoy 配置变更 |
| `EnvoyReady` | Normal | istio-proxy | v1.6+ | 高频 | Envoy 代理就绪 |
| `EnvoyNotReady` | Warning | istio-proxy | v1.6+ | 低频 | Envoy 代理未就绪 |
| **ArgoCD** | | | | | |
| `Synced` | Normal | argocd-application-controller | v2.0+ | 高频 | 应用同步成功 |
| `SyncFailed` | Warning | argocd-application-controller | v2.0+ | 中频 | 应用同步失败 |
| `Degraded` | Warning | argocd-application-controller | v2.0+ | 低频 | 应用处于降级状态 |
| `Healthy` | Normal | argocd-application-controller | v2.0+ | 高频 | 应用健康 |
| `OutOfSync` | Warning | argocd-application-controller | v2.0+ | 中频 | 应用漂移(与 Git 不一致) |
| `Progressing` | Normal | argocd-application-controller | v2.0+ | 中频 | 应用部署中 |
| `Pruned` | Normal | argocd-application-controller | v2.0+ | 低频 | 资源已清理 |
| **Knative** | | | | | |
| `RevisionReady` | Normal | knative-serving-controller | v0.20+ | 中频 | Revision 已就绪 |
| `ConfigurationReady` | Normal | knative-serving-controller | v0.20+ | 中频 | Configuration 已就绪 |
| `RouteReady` | Normal | knative-serving-controller | v0.20+ | 中频 | Route 已就绪 |
| `IngressNotReady` | Warning | knative-serving-controller | v0.20+ | 低频 | Ingress 未就绪 |
| `InternalError` | Warning | knative-serving-controller | v0.20+ | 低频 | 内部错误 |
| **Prometheus Operator** | | | | | |
| `PrometheusRuleCreated` | Normal | prometheus-operator | v0.40+ | 中频 | PrometheusRule 创建成功 |
| `AlertmanagerConfigSynced` | Normal | prometheus-operator | v0.40+ | 中频 | Alertmanager 配置同步 |
| `ConfigReloaded` | Normal | prometheus-operator | v0.40+ | 中频 | Prometheus 配置重载 |
| `SyncFailed` | Warning | prometheus-operator | v0.40+ | 低频 | 配置同步失败 |
| **Velero** | | | | | |
| `BackupCompleted` | Normal | velero | v1.5+ | 中频 | 备份完成 |
| `BackupFailed` | Warning | velero | v1.5+ | 低频 | 备份失败 |
| `RestoreCompleted` | Normal | velero | v1.5+ | 低频 | 恢复完成 |
| `RestoreFailed` | Warning | velero | v1.5+ | 低频 | 恢复失败 |
| `ScheduledBackupCreated` | Normal | velero | v1.5+ | 中频 | 定时备份创建 |
| **External DNS** | | | | | |
| `CreateRecord` | Normal | external-dns | v0.7+ | 中频 | DNS 记录创建 |
| `UpdateRecord` | Normal | external-dns | v0.7+ | 中频 | DNS 记录更新 |
| `DeleteRecord` | Normal | external-dns | v0.7+ | 低频 | DNS 记录删除 |
| `FailedToSyncRecords` | Warning | external-dns | v0.7+ | 低频 | DNS 记录同步失败 |
| **MetalLB** | | | | | |
| `IPAssigned` | Normal | metallb-controller | v0.10+ | 中频 | IP 地址分配成功 |
| `AllocationFailed` | Warning | metallb-controller | v0.10+ | 低频 | IP 地址分配失败 |
| `nodeAssigned` | Normal | metallb-speaker | v0.10+ | 中频 | 节点 BGP 宣告 |
| `IPNotAssigned` | Warning | metallb-controller | v0.10+ | 低频 | IP 地址未分配 |

---

## 二、Node Problem Detector 事件

**架构概述**: Node Problem Detector (NPD) 是 DaemonSet,在每个节点监控内核日志、系统日志和自定义健康检查,将节点问题报告为 Event 和 Node Condition。

---

### `KernelOops` - 内核 Oops 错误
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/kernel-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 罕见 |
#### 事件含义
内核检测到严重错误(Oops),通常由内核模块 bug、硬件故障或内核版本不兼容导致。Oops 不会导致立即崩溃,但系统不稳定。
#### 排查建议
```bash
dmesg -T | grep -i 'oops\|bug'
kubectl describe node <node-name> | grep -A 10 KernelOops
```
#### 解决建议
立即隔离节点 (`kubectl cordon`),升级内核或更换硬件。

---

### `DockerHung` / `ContainerdHung` - 容器运行时挂起
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/system-log-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 低频 |
#### 事件含义
容器运行时守护进程无响应,通常由磁盘 I/O 瓶颈、并发操作过多或内存耗尽导致。
#### 排查建议
```bash
systemctl status containerd
df -h /var/lib/containerd
crictl ps  # 测试是否挂起
```
#### 解决建议
重启运行时 `systemctl restart containerd`,清理磁盘空间,扩容或使用 SSD。

---

### `ReadonlyFilesystem` - 文件系统变为只读
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/system-log-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 低频 |
#### 事件含义
磁盘 I/O 错误导致 Linux 内核强制文件系统变为只读模式。原因包括物理磁盘损坏、网络存储中断、文件系统 bug。
#### 排查建议
```bash
mount | grep 'ro,'
dmesg | grep -i 'readonly\|i/o error'
smartctl -a /dev/sda
```
#### 解决建议
本地磁盘: 运行 `fsck` 或更换磁盘。网络存储: 检查存储后端健康。

---

### `CorruptDockerOverlay2` - overlay2 文件系统损坏
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/custom-plugin-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 罕见 |
#### 事件含义
Docker overlay2 存储驱动元数据损坏,通常由意外断电或磁盘空间耗尽触发。
#### 排查建议
```bash
docker info | grep "Storage Driver"
dmesg | grep overlay
```
#### 解决建议
停止 Docker,删除损坏目录 `rm -rf /var/lib/docker/overlay2/<id>`,重启 Docker。

---

### `TaskHung` - 任务挂起超时
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/kernel-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 低频 |
#### 事件含义
进程在不可中断睡眠状态 (D state) 超过 120 秒,通常等待磁盘/网络 I/O。
#### 排查建议
```bash
ps aux | grep ' D '
iotop -o -b -n 3
```
#### 解决建议
排查存储性能,调整 NFS/iSCSI 超时参数。

---

### `UnregisterNetDevice` - 网络设备注销异常
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/kernel-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 低频 |
#### 事件含义
网络设备注销时出现异常引用或资源泄漏,常见于 CNI 插件删除设备时出错。
#### 排查建议
```bash
dmesg | grep 'unregister_netdevice'
ip link show
```
#### 解决建议
升级 CNI 插件到最新版本,重启节点清理网络栈。

---

### `KernelDeadlock` - 内核死锁
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/kernel-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 罕见 |
#### 事件含义
内核死锁检测机制发现循环锁依赖,这是严重的内核 bug。
#### 排查建议
```bash
dmesg | grep -A 50 'possible deadlock'
```
#### 解决建议
立即隔离节点,升级内核,向社区报告 bug。

---

### `OOMKilling` - 内核 OOM Killer 触发
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/kernel-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 中频 |
#### 事件含义
系统内存耗尽,内核 OOM Killer 强制终止进程。与 Pod OOM 不同,这是系统级 OOM,影响节点稳定性。
#### 排查建议
```bash
dmesg | grep 'Out of memory'
free -h
```
#### 解决建议
调整 Kubelet 内存预留 `--system-reserved=memory=1Gi --eviction-hard=memory.available<500Mi`。

---

### `FilesystemCorruption` - 文件系统损坏
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | node-problem-detector/custom-plugin-monitor |
| **适用版本** | NPD v0.8+ |
| **生产频率** | 罕见 |
#### 事件含义
文件系统元数据损坏,可能导致文件丢失或读写失败。
#### 排查建议
```bash
dmesg | grep -i 'ext4-fs error\|corruption'
```
#### 解决建议
运行 `fsck -y /dev/sda1` (需卸载或单用户模式),备份数据,更换硬盘。

---

## 三、Ingress 控制器事件 (NGINX)

**架构概述**: NGINX Ingress Controller 监听 Kubernetes API 中的 Ingress 资源,动态生成 NGINX 配置文件并重载。包括 ingress-nginx-controller、admission webhook 和默认后端。

---

### `Sync` - Ingress 配置同步成功
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | ingress-nginx-controller |
| **适用版本** | v0.20+ |
| **生产频率** | 高频 |
#### 事件含义
控制器成功将 Ingress 规则转换为 NGINX 配置并重载。
#### 排查建议
```bash
kubectl describe ingress <ingress-name>
kubectl logs -n ingress-nginx deployment/ingress-nginx-controller
```
#### 解决建议
正常事件,如果频繁出现检查是否有自动化工具误操作。

---

### `CREATE` / `UPDATE` / `DELETE` - Ingress 规则生命周期
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | ingress-nginx-controller |
| **适用版本** | v0.20+ |
| **生产频率** | 中频 |
#### 事件含义
Ingress 控制器检测到资源创建、更新或删除。
#### 排查建议
```bash
kubectl get ingress -A
kubectl get ingress <name> -o yaml
```
#### 解决建议
正常生命周期事件。如果 DELETE 后流量仍存在,检查 NGINX 配置重载。

---

### `Rejected` - Ingress 规则被拒绝
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | ingress-nginx-admission-webhook |
| **适用版本** | v0.20+ |
| **生产频率** | 低频 |
#### 事件含义
Ingress 配置验证失败,常见原因: 无效注解语法、缺少 TLS 证书、与现有规则冲突。
#### 排查建议
```bash
kubectl describe ingress <ingress-name>
kubectl logs -n ingress-nginx deployment/ingress-nginx-admission
```
#### 解决建议
修复 YAML: 检查注解拼写、确保 Secret 存在、避免重复 host/path。

---

### `AddedOrUpdated` - Ingress 添加或更新
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | ingress-nginx-controller |
| **适用版本** | v0.20+ |
| **生产频率** | 高频 |
#### 事件含义
控制器成功处理 Ingress 新增或变更。
#### 排查建议
```bash
curl -H "Host: example.com" http://<ingress-ip>/path
```
#### 解决建议
正常事件。如果流量异常,检查 Service 和 Pod 是否就绪。

---

## 四、cert-manager 证书管理事件

**架构概述**: cert-manager 是 Kubernetes 原生 X.509 证书管理控制器,支持自动签发和续期 TLS 证书。核心 CRD: Issuer/ClusterIssuer、Certificate、CertificateRequest、Order/Challenge。

---

### `Issuing` - 正在签发证书
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | cert-manager |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |
#### 事件含义
cert-manager 开始向 Issuer 请求签发证书。
#### 排查建议
```bash
kubectl describe certificate <cert-name>
kubectl get certificaterequest
```
#### 解决建议
正常流程。如果长时间停留,检查 Issuer 是否 Ready、ACME Challenge 是否完成。

---

### `Issued` - 证书签发成功
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | cert-manager |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |
#### 事件含义
证书成功签发并存储到 Secret。
#### 排查建议
```bash
kubectl get secret <tls-secret> -o jsonpath='{.data.tls\.crt}' | base64 -d | openssl x509 -text -noout
```
#### 解决建议
正常事件。

---

### `Ready` - 证书已就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | cert-manager |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |
#### 事件含义
证书已签发且尚未过期,可正常使用。
#### 排查建议
```bash
kubectl get certificate <name> -o jsonpath='{.status.notAfter}'
```
#### 解决建议
正常状态,cert-manager 会在到期前自动续期。

---

### `NotReady` - 证书未就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | cert-manager |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |
#### 事件含义
证书签发失败、过期或被撤销。
#### 排查建议
```bash
kubectl describe certificate <name>
kubectl describe certificaterequest <request-name>
```
#### 解决建议
根据失败原因: 检查 Issuer 配置、确保 ACME Challenge 可访问、删除旧 Secret 触发重新签发。

---

### `IssuerNotReady` - 签发者未就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | cert-manager |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |
#### 事件含义
Issuer/ClusterIssuer 配置错误或依赖资源不可用。常见原因: ACME 服务器不可达、CA 证书 Secret 不存在、Vault 服务未认证。
#### 排查建议
```bash
kubectl describe issuer <issuer-name>
kubectl get secret <ca-secret>
```
#### 解决建议
ACME: 检查网络连通性,确认 ACME 账户 Secret。CA: 确保 CA 证书和私钥正确。

---

### `OrderCreated` - ACME Order 已创建
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | cert-manager/acme-controller |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |
#### 事件含义
使用 ACME 协议签发证书时,创建了 Order 资源,开始域名验证 Challenge。
#### 排查建议
```bash
kubectl get order
kubectl describe challenge <challenge-name>
```
#### 解决建议
等待 Challenge 完成。HTTP-01: 确保 Ingress 配置 `/.well-known/acme-challenge/`。DNS-01: 验证 DNS 提供商凭证。

---

### `OrderComplete` - ACME Order 完成
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | cert-manager/acme-controller |
| **适用版本** | v1.0+ |
| **生产频率** | 中频 |
#### 事件含义
ACME Order 的所有 Challenge 已完成,证书即将签发。
#### 排查建议
```bash
kubectl get certificate <name>
```
#### 解决建议
正常流程,通常几秒后收到 `Issued` 事件。

---

### `RenewalScheduled` - 证书续期已调度
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | cert-manager |
| **适用版本** | v1.0+ |
| **生产频率** | 低频 |
#### 事件含义
证书即将到期,cert-manager 已调度自动续期任务(默认在生命周期 2/3 时)。
#### 排查建议
```bash
kubectl get certificate <name> -o jsonpath='{.status.notAfter}'
```
#### 解决建议
正常自动续期。如果续期失败,会产生 `NotReady` 事件。

---

## 五、Istio Service Mesh 事件

**架构概述**: Istio 通过 Sidecar 代理 (Envoy) 拦截服务间流量,提供流量管理、安全、可观测性。核心组件: istiod (控制平面)、istio-sidecar-injector (Webhook)、istio-proxy (Envoy 数据平面)。

---

### `InjectionSucceeded` - Sidecar 注入成功
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | istio-sidecar-injector |
| **适用版本** | Istio v1.6+ |
| **生产频率** | 高频 |
#### 事件含义
Mutating Admission Webhook 成功向 Pod 注入 Envoy Sidecar 容器。
#### 排查建议
```bash
kubectl get pod <pod-name> -o jsonpath='{.spec.containers[*].name}'
kubectl get namespace <ns> -o jsonpath='{.metadata.labels.istio-injection}'
```
#### 解决建议
正常事件。如果 Pod 无 Sidecar,检查 Namespace 是否有 `istio-injection=enabled` 标签。

---

### `InjectionFailed` - Sidecar 注入失败
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | istio-sidecar-injector |
| **适用版本** | Istio v1.6+ |
| **生产频率** | 低频 |
#### 事件含义
Webhook 无法注入 Sidecar。常见原因: istio-sidecar-injector 服务不可达、Pod 定义不兼容 (如 hostNetwork)、资源限制。
#### 排查建议
```bash
kubectl get mutatingwebhookconfiguration istio-sidecar-injector -o yaml
kubectl logs -n istio-system deployment/istio-sidecar-injector
```
#### 解决建议
Webhook 不可用: 重启 istiod。资源限制: 增加 Namespace ResourceQuota。hostNetwork Pod: 添加 `sidecar.istio.io/inject=false`。

---

### `ProxyConfigChanged` - Envoy 配置变更
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | istiod |
| **适用版本** | Istio v1.6+ |
| **生产频率** | 中频 |
#### 事件含义
istiod 向 Envoy 代理下发了新的配置 (xDS)。
#### 排查建议
```bash
istioctl proxy-config cluster <pod-name>.<namespace>
istioctl proxy-status
```
#### 解决建议
正常事件。配置未生效可能需要等待几秒 (最终一致性)。

---

### `EnvoyReady` - Envoy 代理就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | istio-proxy |
| **适用版本** | Istio v1.6+ |
| **生产频率** | 高频 |
#### 事件含义
Envoy Sidecar 容器已启动并完成初始化。
#### 排查建议
```bash
kubectl logs <pod-name> -c istio-proxy
```
#### 解决建议
正常事件,标志 Sidecar 可以处理流量。

---

### `EnvoyNotReady` - Envoy 代理未就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | istio-proxy |
| **适用版本** | Istio v1.6+ |
| **生产频率** | 低频 |
#### 事件含义
Envoy 容器启动失败或无法与 istiod 建立连接。
#### 排查建议
```bash
kubectl logs <pod-name> -c istio-proxy
kubectl exec <pod-name> -c istio-proxy -- curl http://istiod.istio-system:15010/ready
```
#### 解决建议
无法连接 istiod: 检查网络策略或 DNS。配置错误: 查看 istiod 日志。资源不足: 增加 Sidecar 资源限制。

---

## 六、ArgoCD GitOps 事件

**架构概述**: ArgoCD 是声明式 GitOps 持续交付工具,将 Git 仓库作为配置唯一真实来源。核心组件: argocd-application-controller (监控 Application 执行同步)、argocd-repo-server (渲染模板)、argocd-server (API 和 UI)。

---

### `Synced` - 应用同步成功
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | argocd-application-controller |
| **适用版本** | ArgoCD v2.0+ |
| **生产频率** | 高频 |
#### 事件含义
Application 的期望状态 (Git) 与实际状态 (集群) 已同步。
#### 排查建议
```bash
argocd app get <app-name>
argocd app history <app-name>
```
#### 解决建议
正常事件。如果同步后应用仍异常,检查 Pod 是否就绪。

---

### `SyncFailed` - 应用同步失败
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | argocd-application-controller |
| **适用版本** | ArgoCD v2.0+ |
| **生产频率** | 中频 |
#### 事件含义
ArgoCD 无法将 Git 资源应用到集群。常见原因: YAML 语法错误、ServiceAccount 权限不足、资源配额耗尽。
#### 排查建议
```bash
argocd app get <app-name>
kubectl logs -n argocd deployment/argocd-application-controller
```
#### 解决建议
YAML 错误: 修复 Git 仓库配置。权限不足: 授予 RBAC 权限。资源配额: 扩容或清理。

---

### `Degraded` - 应用降级
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | argocd-application-controller |
| **适用版本** | ArgoCD v2.0+ |
| **生产频率** | 低频 |
#### 事件含义
应用健康检查失败,虽然同步成功但 Pod 不健康、Job 失败等。
#### 排查建议
```bash
argocd app get <app-name>
kubectl get all -l app.kubernetes.io/instance=<app-name>
```
#### 解决建议
根据资源类型检查 Pod 日志、Job 失败原因、Service Endpoint。

---

### `Healthy` - 应用健康
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | argocd-application-controller |
| **适用版本** | ArgoCD v2.0+ |
| **生产频率** | 高频 |
#### 事件含义
所有资源已同步且健康检查通过。
#### 排查建议
无需排查。
#### 解决建议
无需处理。

---

### `OutOfSync` - 应用漂移
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | argocd-application-controller |
| **适用版本** | ArgoCD v2.0+ |
| **生产频率** | 中频 |
#### 事件含义
集群资源被手动修改,与 Git 仓库不一致,ArgoCD 检测到漂移。
#### 排查建议
```bash
argocd app diff <app-name>
kubectl get events --field-selector involvedObject.name=<resource-name>
```
#### 解决建议
恢复同步: `argocd app sync <app-name>`。自动纠偏: 启用 `spec.syncPolicy.automated.selfHeal=true`。

---

### `Progressing` - 应用部署中
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | argocd-application-controller |
| **适用版本** | ArgoCD v2.0+ |
| **生产频率** | 中频 |
#### 事件含义
ArgoCD 正在应用资源变更,等待所有资源达到就绪状态。
#### 排查建议
```bash
argocd app get <app-name>
```
#### 解决建议
正常过程。长时间停留检查 Pod 是否卡在 ImagePullBackOff 或 Deployment 滚动更新阻塞。

---

### `Pruned` - 资源已清理
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | argocd-application-controller |
| **适用版本** | ArgoCD v2.0+ |
| **生产频率** | 低频 |
#### 事件含义
Git 中删除的资源已从集群清理 (需启用 `spec.syncOptions: [Prune=true]`)。
#### 排查建议
```bash
kubectl get application <app-name> -o jsonpath='{.spec.syncPolicy}'
```
#### 解决建议
正常清理。如果资源仍存在,确认启用 Prune 选项且资源无 `Prevent=true` 注解。

---

## 七、Knative Serverless 事件

**架构概述**: Knative 提供 Kubernetes 上的 Serverless 能力,包括 Knative Serving (自动扩缩容、流量路由) 和 Knative Eventing (事件驱动)。核心 CRD: Service、Revision、Route、Configuration。

---

### `RevisionReady` - Revision 已就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | knative-serving-controller |
| **适用版本** | Knative v0.20+ |
| **生产频率** | 中频 |
#### 事件含义
Knative Revision (不可变部署版本) 的所有 Pod 已就绪,可接收流量。
#### 排查建议
```bash
kubectl get revision
kubectl describe revision <revision-name>
```
#### 解决建议
正常事件。Revision 就绪后 Route 开始分配流量。

---

### `ConfigurationReady` - Configuration 已就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | knative-serving-controller |
| **适用版本** | Knative v0.20+ |
| **生产频率** | 中频 |
#### 事件含义
Configuration 的最新 Revision 已创建并就绪。
#### 排查建议
```bash
kubectl get configuration
```
#### 解决建议
正常事件,标志应用可处理请求。

---

### `RouteReady` - Route 已就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | knative-serving-controller |
| **适用版本** | Knative v0.20+ |
| **生产频率** | 中频 |
#### 事件含义
Route 已将流量规则应用到底层 Ingress/Gateway。
#### 排查建议
```bash
kubectl get route
kubectl get route <route-name> -o jsonpath='{.status.traffic}'
```
#### 解决建议
正常事件。可通过 Route 的 URL 访问服务。

---

### `IngressNotReady` - Ingress 未就绪
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | knative-serving-controller |
| **适用版本** | Knative v0.20+ |
| **生产频率** | 低频 |
#### 事件含义
Knative 无法创建或配置底层 Ingress 资源 (可能是 Istio Gateway 或 Contour HTTPProxy)。
#### 排查建议
```bash
kubectl get ingress -n <namespace>
kubectl get configmap config-network -n knative-serving -o yaml
```
#### 解决建议
Ingress Controller 未安装: 安装 Istio、Contour 或 Kourier。配置错误: 检查 `config-network` 中的 `ingress.class`。

---

### `InternalError` - 内部错误
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | knative-serving-controller |
| **适用版本** | Knative v0.20+ |
| **生产频率** | 低频 |
#### 事件含义
Knative 控制器遇到内部错误,无法协调资源。
#### 排查建议
```bash
kubectl logs -n knative-serving deployment/controller
```
#### 解决建议
临时错误可能自动恢复。持久错误查看日志向 Knative 社区报告 bug。

---

## 八、Prometheus Operator 事件

**架构概述**: Prometheus Operator 使用 CRD 简化 Prometheus 集群的部署和配置。核心 CRD: Prometheus (监控服务器)、ServiceMonitor (声明式服务发现)、PrometheusRule (告警规则)、Alertmanager (告警管理)。

---

### `PrometheusRuleCreated` - PrometheusRule 创建成功
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | prometheus-operator |
| **适用版本** | v0.40+ |
| **生产频率** | 中频 |
#### 事件含义
Operator 成功将 PrometheusRule 转换为 Prometheus 配置。
#### 排查建议
```bash
kubectl get prometheusrule
curl http://<prometheus-url>/api/v1/rules | jq .
```
#### 解决建议
正常事件。如果规则未生效,检查 PrometheusRule 的 label 是否匹配 Prometheus 的 `spec.ruleSelector`。

---

### `AlertmanagerConfigSynced` - Alertmanager 配置同步
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | prometheus-operator |
| **适用版本** | v0.40+ |
| **生产频率** | 中频 |
#### 事件含义
Alertmanager 配置 (告警路由、接收器) 已同步到 Alertmanager Pod。
#### 排查建议
```bash
kubectl get secret alertmanager-<name> -o jsonpath='{.data.alertmanager\.yaml}' | base64 -d
```
#### 解决建议
正常事件。配置更新后 Alertmanager 自动重载。

---

### `ConfigReloaded` - Prometheus 配置重载
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | prometheus-operator |
| **适用版本** | v0.40+ |
| **生产频率** | 中频 |
#### 事件含义
Prometheus 已重载配置文件,应用了新的抓取目标或规则。
#### 排查建议
```bash
curl http://<prometheus-url>/api/v1/status/config | jq .
```
#### 解决建议
正常事件。重载不会中断现有抓取任务。

---

### `SyncFailed` - 配置同步失败
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | prometheus-operator |
| **适用版本** | v0.40+ |
| **生产频率** | 低频 |
#### 事件含义
Operator 无法生成有效 Prometheus 配置。常见原因: PrometheusRule PromQL 语法错误、ServiceMonitor 选择器无法匹配、资源引用不存在。
#### 排查建议
```bash
kubectl logs -n monitoring deployment/prometheus-operator
promtool check rules <rule-file.yaml>
```
#### 解决建议
根据日志修复: 检查 PromQL 语法、确认 label selector、检查引用的 Secret/ConfigMap。

---

## 九、Velero 备份恢复事件

**架构概述**: Velero 是 Kubernetes 集群备份和迁移工具,支持集群资源备份 (Kubernetes API 对象)、持久卷备份 (快照或 Restic) 和定时备份 (CronJob 风格调度)。

---

### `BackupCompleted` - 备份完成
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | velero |
| **适用版本** | Velero v1.5+ |
| **生产频率** | 中频 |
#### 事件含义
备份任务成功完成,数据已存储到对象存储 (S3/GCS/Azure Blob)。
#### 排查建议
```bash
velero backup describe <backup-name>
velero backup logs <backup-name>
```
#### 解决建议
正常事件。定期验证备份可恢复性 (执行 `velero restore`)。

---

### `BackupFailed` - 备份失败
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | velero |
| **适用版本** | Velero v1.5+ |
| **生产频率** | 低频 |
#### 事件含义
备份任务失败。常见原因: 对象存储凭证过期或权限不足、网络连接中断、资源选择器未匹配对象。
#### 排查建议
```bash
velero backup describe <backup-name>
kubectl logs -n velero deployment/velero
```
#### 解决建议
存储凭证: 更新 S3/GCS 凭证 Secret。网络问题: 检查连通性。选择器: 调整 `--selector` 或 `--include-namespaces`。

---

### `RestoreCompleted` - 恢复完成
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | velero |
| **适用版本** | Velero v1.5+ |
| **生产频率** | 低频 |
#### 事件含义
从备份成功恢复资源到集群。
#### 排查建议
```bash
velero restore describe <restore-name>
kubectl get all -n <namespace>
```
#### 解决建议
正常事件。恢复后验证 Pod 是否正常运行、PV 数据是否完整。

---

### `RestoreFailed` - 恢复失败
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | velero |
| **适用版本** | Velero v1.5+ |
| **生产频率** | 低频 |
#### 事件含义
恢复任务失败。常见原因: 资源已存在 (名称冲突)、Kubernetes 版本不兼容 (API 废弃)、PV 快照无法挂载。
#### 排查建议
```bash
velero restore describe <restore-name>
velero restore logs <restore-name>
```
#### 解决建议
资源冲突: 删除冲突资源或使用 `--namespace-mappings`。版本不兼容: 升级集群或手动修改 YAML。PV 问题: 检查存储类。

---

### `ScheduledBackupCreated` - 定时备份创建
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | velero |
| **适用版本** | Velero v1.5+ |
| **生产频率** | 中频 |
#### 事件含义
定时备份任务 (Schedule) 触发并创建了新的 Backup 资源。
#### 排查建议
```bash
velero schedule get
velero backup get
```
#### 解决建议
正常事件。定期清理过期备份 (配置 `--ttl`)。

---

## 十、External DNS 事件

**架构概述**: External DNS 自动同步 Kubernetes 资源 (Service、Ingress) 到外部 DNS 提供商 (Route53、Cloudflare、Azure DNS 等)。

---

### `CreateRecord` - DNS 记录创建
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | external-dns |
| **适用版本** | v0.7+ |
| **生产频率** | 中频 |
#### 事件含义
External DNS 在外部 DNS 提供商创建了 A/CNAME 记录。
#### 排查建议
```bash
kubectl logs -n kube-system deployment/external-dns
dig <hostname>
```
#### 解决建议
正常事件。如果 DNS 未生效,检查 Service/Ingress 注解 `external-dns.alpha.kubernetes.io/hostname`、DNS 提供商凭证。

---

### `UpdateRecord` - DNS 记录更新
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | external-dns |
| **适用版本** | v0.7+ |
| **生产频率** | 中频 |
#### 事件含义
External DNS 更新了 DNS 记录 (如 LoadBalancer IP 变更)。
#### 排查建议
```bash
nslookup <hostname>
```
#### 解决建议
正常事件。注意 DNS TTL,记录变更生效有延迟。

---

### `DeleteRecord` - DNS 记录删除
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | external-dns |
| **适用版本** | v0.7+ |
| **生产频率** | 低频 |
#### 事件含义
External DNS 删除了不再需要的 DNS 记录。
#### 排查建议
```bash
dig <hostname>
```
#### 解决建议
正常事件。如果需要保留记录,确保资源仍存在或使用 `--policy=upsert-only`。

---

### `FailedToSyncRecords` - DNS 记录同步失败
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | external-dns |
| **适用版本** | v0.7+ |
| **生产频率** | 低频 |
#### 事件含义
External DNS 无法同步记录到 DNS 提供商。常见原因: API 凭证过期或权限不足、DNS Zone 不存在、达到 API 限流或配额。
#### 排查建议
```bash
kubectl logs -n kube-system deployment/external-dns
kubectl get secret external-dns -o yaml
```
#### 解决建议
权限问题: 更新 IAM 角色或 API Token。Zone 不存在: 在 DNS 提供商创建 Zone。限流: 降低同步频率 `--interval`。

---

## 十一、MetalLB 负载均衡事件

**架构概述**: MetalLB 为裸金属集群提供 LoadBalancer 服务支持,支持 Layer 2 模式 (使用 ARP/NDP 宣告 IP) 和 BGP 模式 (通过 BGP 协议与路由器集成)。核心组件: metallb-controller (监听 Service 分配 IP)、metallb-speaker (宣告 IP 或建立 BGP 会话)。

---

### `IPAssigned` - IP 地址分配成功
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | metallb-controller |
| **适用版本** | MetalLB v0.10+ |
| **生产频率** | 中频 |
#### 事件含义
MetalLB 从 IP 地址池中为 Service 成功分配了外部 IP。
#### 排查建议
```bash
kubectl get svc <svc-name>
kubectl get configmap -n metallb-system config -o yaml
```
#### 解决建议
正常事件。IP 会自动写入 `status.loadBalancer.ingress`。

---

### `AllocationFailed` - IP 地址分配失败
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | metallb-controller |
| **适用版本** | MetalLB v0.10+ |
| **生产频率** | 低频 |
#### 事件含义
MetalLB 无法分配 IP。常见原因: IP 地址池耗尽、Service 注解指定的池不存在、配置错误。
#### 排查建议
```bash
kubectl get ipaddresspool -n metallb-system
kubectl get svc --all-namespaces -o wide | grep LoadBalancer
```
#### 解决建议
扩展 IP 池: 在 MetalLB 配置中添加更多 IP 范围。释放 IP: 删除无用 LoadBalancer Service。检查配置: 确认 address-pool 名称正确。

---

### `nodeAssigned` - 节点 BGP 宣告
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Normal |
| **来源组件** | metallb-speaker |
| **适用版本** | MetalLB v0.10+ |
| **生产频率** | 中频 |
#### 事件含义
MetalLB speaker 成功在节点上建立 BGP 会话并宣告路由。
#### 排查建议
```bash
kubectl logs -n metallb-system daemonset/speaker | grep BGP
```
#### 解决建议
正常事件。如果路由未生效,检查 BGP peer 配置、路由器防火墙规则 (TCP 179)。

---

### `IPNotAssigned` - IP 地址未分配
| 属性 | 说明 |
|:---|:---|
| **事件类型** | Warning |
| **来源组件** | metallb-controller |
| **适用版本** | MetalLB v0.10+ |
| **生产频率** | 低频 |
#### 事件含义
MetalLB 控制器检测到 Service 应该有外部 IP,但未能分配。
#### 排查建议
```bash
kubectl describe svc <svc-name>
kubectl logs -n metallb-system deployment/controller
```
#### 解决建议
配置 IP 池: 创建 IPAddressPool 资源。检查 Service 类型: 确认 `type: LoadBalancer`。查看控制器日志排查错误。

---

## 十二、生产环境最佳实践

### 12.1 生态插件事件监控策略

| 监控维度 | 推荐工具 | 关键指标 | 告警阈值建议 |
|:---|:---|:---|:---|
| **事件采集** | Prometheus + kube-state-metrics | `kube_event_count` | Warning 事件 > 10/min |
| **事件持久化** | Event Exporter → Elasticsearch | 保留 30 天历史 | 磁盘使用率 > 80% |
| **插件健康** | Prometheus ServiceMonitor | 控制器 Pod 可用性 | Unavailable > 5min |
| **证书到期** | cert-manager Exporter | `certmanager_certificate_expiration_timestamp_seconds` | < 7 天 |
| **备份成功率** | Velero Prometheus Metrics | `velero_backup_success_total / velero_backup_attempt_total` | < 95% |

### 12.2 插件版本兼容性矩阵

| 插件 | k8s v1.25 | v1.26 | v1.27 | v1.28 | v1.29 | v1.30+ |
|:---|:---:|:---:|:---:|:---:|:---:|:---:|
| **NPD** | v0.8.10+ | v0.8.10+ | v0.8.12+ | v0.8.12+ | v0.8.14+ | v0.8.14+ |
| **NGINX Ingress** | v1.5+ | v1.6+ | v1.7+ | v1.8+ | v1.9+ | v1.10+ |
| **cert-manager** | v1.10+ | v1.11+ | v1.12+ | v1.13+ | v1.14+ | v1.15+ |
| **Istio** | v1.16+ | v1.17+ | v1.18+ | v1.19+ | v1.20+ | v1.21+ |
| **ArgoCD** | v2.5+ | v2.6+ | v2.7+ | v2.8+ | v2.9+ | v2.10+ |
| **Velero** | v1.10+ | v1.11+ | v1.11+ | v1.12+ | v1.13+ | v1.13+ |
| **MetalLB** | v0.13+ | v0.13+ | v0.13+ | v0.14+ | v0.14+ | v0.14+ |

### 12.3 插件部署优先级

**优先级 1 (必备):**
- ✅ Node Problem Detector: 节点健康监控
- ✅ cert-manager: TLS 证书自动化
- ✅ Velero: 灾难恢复能力

**优先级 2 (流量入口):**
- ✅ NGINX Ingress 或 Istio Gateway: 外部流量入口
- ✅ External DNS: DNS 自动化 (云环境)
- ✅ MetalLB: LoadBalancer 支持 (裸金属)

**优先级 3 (高级特性):**
- Istio: 服务网格 (微服务复杂场景)
- ArgoCD: GitOps 持续交付
- Knative: Serverless 工作负载

### 12.4 插件事件排查工作流

```
用户报告问题
    │
    ├─→ 1. 查看插件 Pod 状态
    │       kubectl get pods -n <plugin-namespace>
    │
    ├─→ 2. 查看插件事件
    │       kubectl get events -n <plugin-namespace> --sort-by='.lastTimestamp'
    │
    ├─→ 3. 查看控制器日志
    │       kubectl logs -n <plugin-namespace> deployment/<controller>
    │
    ├─→ 4. 检查 CRD 资源状态
    │       kubectl describe <crd-resource> <name>
    │
    ├─→ 5. 验证配置正确性
    │       kubectl get configmap/secret -n <plugin-namespace>
    │
    └─→ 6. 查阅插件文档和社区 Issue
            GitHub Issues / Slack / 官方文档
```

### 12.5 关键插件告警规则示例

```yaml
# Prometheus AlertManager 告警规则
groups:
  - name: addon-critical-events
    interval: 30s
    rules:
      # Node Problem Detector
      - alert: NodeKernelOops
        expr: increase(kube_event_count{reason="KernelOops"}[5m]) > 0
        severity: critical
      # cert-manager
      - alert: CertificateNotReady
        expr: kube_event_count{reason="NotReady",involved_object_kind="Certificate"} > 0
        for: 10m
        severity: warning
      # Velero
      - alert: BackupFailed
        expr: increase(velero_backup_failure_total[1h]) > 0
        severity: critical
      # MetalLB
      - alert: MetalLBIPPoolExhausted
        expr: increase(kube_event_count{reason="AllocationFailed"}[5m]) > 3
        for: 5m
        severity: warning
```

### 12.6 插件升级注意事项

**升级前检查清单:**
1. ✅ 阅读 Release Notes,确认无破坏性变更
2. ✅ 在测试环境验证升级流程
3. ✅ 备份关键配置 (CRD、ConfigMap、Secret)
4. ✅ 确认 Kubernetes 版本兼容性
5. ✅ 准备回滚计划

**高风险插件:**
- **Istio**: 控制平面和数据平面需协调升级 (Canary 方式)
- **cert-manager**: CRD 升级可能导致证书重新签发
- **ArgoCD**: 数据库迁移可能需要停机

### 12.7 插件安全加固

| 安全层面 | 加固措施 | 适用插件 |
|:---|:---|:---|
| **RBAC** | 最小权限原则,使用 RoleBinding 限制 ServiceAccount | 所有插件 |
| **网络策略** | 限制控制器只能访问必要的 API Server 端点 | 所有插件 |
| **Secret 管理** | 使用 External Secrets Operator 存储 API Token | External DNS, Velero |
| **镜像安全** | 定期扫描插件镜像漏洞 (Trivy/Clair) | 所有插件 |
| **审计日志** | 启用 Kubernetes Audit Log,记录插件 API 调用 | ArgoCD, Velero |

---

> **KUDIG-DATABASE** | Domain-33: Kubernetes Events 全域事件大全 | 文档 15/15
