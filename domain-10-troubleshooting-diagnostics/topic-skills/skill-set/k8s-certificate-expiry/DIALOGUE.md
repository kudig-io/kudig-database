---
dialogue_id: "DIALOGUE-SEC-001"
category: "troubleshooting"
tags: ["security", "remote-consultant"]
skill_ref: "SKILL-SEC-001"
title: "证书过期问题诊断与修复 — 对话脚本"
version: "1.0.0"
agent_role: "远程顾问"
execution_mode: "L1-advisory"
created: "2026-05-23"
updated: "2026-05-23"
summary: "证书过期问题的远程顾问对话脚本，覆盖TLS、CA、kubelet证书续期操作。"
relationships:
  - target: "[[skills/skill-k8s-node-notready-SKILL]]"
    type: uses
  - target: "[[entities/etcd]]"
    type: uses
  - target: "[[entities/kubelet]]"
    type: uses
  - target: "[[skills/kubelet-certificate-rotation]]"
    type: uses
---

# 证书过期问题诊断与修复 — 对话脚本

> **角色说明**：我是远程顾问，无法直接连接你的集群。我会通过对话指导你执行诊断和修复操作。

## 对话入口

### 入口 A：工程师直接提到证书/过期

**工程师**："集群证书好像过期了" / "看到 x509 certificate has expired"

**顾问回应**：
> 收到，证书过期是 **P0-P1 级别**问题。先快速确认：
> 1. 影响多少节点 / Pod？整个集群还是部分？
> 2. 业务是否已中断？有告警吗？
> 3. 最近 24 小时是否有变更？
>
> 请执行：
> ```bash
> kubectl version --short 2>&1 || echo "kubectl 执行失败"
> ```
> **如果 kubectl 无法执行**：请描述能看到的内容（监控截图、告警通知、错误日志）。

### 入口 B：工程师提到节点 NotReady

**工程师**："节点突然 NotReady" / "很多节点 NotReady"

**顾问回应**：
> 节点 NotReady 有多种原因，证书过期是其中之一（尤其是突然大规模发生时）。请执行：
> ```bash
> kubectl get nodes -o wide 2>&1
> ```
> **如果 kubectl 无法执行**：通过集群管理控制台查看节点状态，或描述节点数量和规律。
>
> 同时请回答：是**所有节点**同时 NotReady 还是**部分**？控制平面节点是否正常？

### 入口 C：工程师提到 API Server 连接失败

**工程师**："kubectl 命令都报错了" / "连不上 API Server"

**顾问回应**：
> 请执行以下命令，把**完整报错信息**贴给我：
> ```bash
> kubectl cluster-info 2>&1
> ```
> **如果 kubectl 完全无法使用**：
> - **替代方案 A**：SSH 到控制平面节点，执行 `crictl ps | grep apiserver` 查看 API Server 容器状态
> - **替代方案 B**：通过集群管理控制台（Rancher、OpenShift Console、云厂商控制台）查看控制平面状态
> - **替代方案 C**：请有权限的同事执行并转发结果
>
> 同时请确认：报错中是否包含 `x509`、`certificate`、`unauthorized`？集群是 kubeadm 部署还是托管集群？

### 入口 D：工程师提到 [[entities/kubelet|kubelet]] 报错

**工程师**："kubelet 日志里有证书错误" / "FailedToUpdateNodeStatus"

**顾问回应**：
> 请在**受影响节点**上执行：
> ```bash
> openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates 2>&1
> ```
> **如果无法执行 openssl**：
> - **替代方案 A**：`kubectl get csr` 查看是否有 Pending 的 CSR
> - **替代方案 B**：`journalctl -u kubelet -n 50 --no-pager` 查看日志，把含 `certificate`、`x509`、`CSR` 的行贴给我
> - **替代方案 C**：云厂商托管节点请在云控制台查看节点证书状态
>
> 请告诉我：影响单个节点还是多个节点？

## Round 1：快速确认与基础诊断

### Round 1 — 分支 A：kubectl 可用

**顾问**：kubectl 可正常连接。请执行：
> ```bash
> kubectl get nodes -o wide
> kubectl get pods -n kube-system
> ```
> 请贴出**完整输出**。如果 `get pods` 报错，请贴出报错并尝试 `kubectl get nodes`。

**工程师反馈路径**：
- 节点全部正常 → Round 2-A（检查证书即将过期）
- 部分节点 NotReady → Round 2-B（定位具体节点证书）
- 命令报错含 x509/certificate → Round 2-C（确认证书过期范围）

### Round 1 — 分支 B：kubectl 报错含 x509/certificate

**顾问**：确认是证书问题。请尝试：
> ```bash
> kubectl get nodes --insecure-skip-tls-verify 2>&1
> ```
> **注意**：`--insecure-skip-tls-verify` 仅用于诊断，不要用于日常操作。
>
> **如果 kubectl 完全无法使用**：
> - **替代方案 A**：SSH 到控制平面节点，执行 `kubeadm certs check-expiration 2>&1`
> - **替代方案 B**：查看集群备份或配置管理记录，确认近期是否有证书变更
> - **替代方案 C**：请有权限的同事执行并转发结果
>
> 请告诉我：完整报错信息是什么？集群是 kubeadm 部署还是托管集群？

**工程师反馈路径**：
- `--insecure-skip-tls-verify` 可获取节点列表 → 客户端证书有效，服务端证书问题 → Round 2-C
- SSH 到节点成功，kubeadm 可用 → Round 2-A
- 完全无法连接任何节点 → Round 1-分支 C（紧急升级）

### Round 1 — 分支 C：完全无法执行任何命令

**顾问**：当前信息有限。请尽可能收集：
> 1. **监控截图**：是否有 apiserver、[[entities/etcd|etcd]]、kubelet 告警？
> 2. **告警内容**：告警通知的完整内容
> 3. **应用表现**：业务是否还能访问？错误页面是什么？
> 4. **变更记录**：最近 24-48 小时是否有运维变更？
> 5. **集群信息**：K8s 版本、节点数、部署方式
>
> **如果以上信息都无法提供**：信息严重不足，建议置信度会降低。如业务已中断且影响严重，**建议立即升级**给有集群管理员权限的高级 SRE。

> ⚠️ **升级建议**：完全无法连接集群且业务已中断，属于**紧急事件**。请立即：
> 1. 联系有集群管理员权限的同事或云厂商技术支持
> 2. 继续收集能获取的任何信息，我协助准备交接材料
> 3. kubeadm 自建集群的管理员可能需要物理/堡垒机登录控制平面节点修复

## Round 2：深度诊断 — 定位过期证书

### Round 2 — 分支 A：kubeadm 可用

**顾问**：在**控制平面节点**上执行：
> ```bash
> kubeadm certs check-expiration
> ```
> 请贴出**完整输出**，重点关注 `EXPIRES` 和 `RESIDUAL TIME` 列。
>
> **如果无法执行 kubeadm 检查**：
> - **替代方案 A**：`for f in /etc/kubernetes/pki/*.crt; do echo "=== $f ==="; openssl x509 -in "$f" -noout -dates 2>/dev/null; done`
> - **替代方案 B**：`which kubeadm || find / -name kubeadm 2>/dev/null | head -5`
> - **替代方案 C**：非 kubeadm 部署请找证书路径（`/etc/kubernetes/pki/` 或 `/etc/kubernetes/ssl/`），用 openssl 逐个检查

**根据输出结果的路径**：

#### A1：有证书已过期

**顾问**：确认以下证书已过期：`[列出过期证书]`。
> **关键问题**：CA 证书（ca.crt）是否过期？
> - **CA 已过期** → **高风险操作，需要升级处理**（见 Round 3-分支 B）
> - **CA 未过期** → 可继续续期其他证书（见 Round 3-分支 A）

#### A2：证书未过期但即将过期（30 天内）

**顾问**：以下证书即将到期：`[列出即将过期证书]`。这是**预防性维护**场景。
> - 建议在维护窗口内执行证书续期
> - 如 kubelet 证书由外部 CA 签发，请检查外部 CA 过期时间
> - 建议设置证书过期监控告警
>
> 当前业务是否已受影响？如未受影响，可安排在维护窗口处理。

#### A3：证书检查正常，问题仍存在

**顾问**：证书未过期，问题可能不是证书引起。请执行：
> ```bash
> kubectl get events --sort-by='.lastTimestamp' -A | tail -30
> journalctl -u kubelet -n 100 --no-pager | grep -i "error\|fail\|cert\|x509"
> ```
> **如果无法查看 journalctl**：
> - **替代方案 A**：`kubectl logs -n kube-system <kubelet-pod>`
> - **替代方案 B**：查看 `/var/log/messages` 或 `/var/log/syslog`
> - **替代方案 C**：`kubectl debug node/<node> -it --image=busybox`
>
> 这可能需要引导至 [[skills/skill-k8s-node-notready-SKILL|SKILL]]-NODE-001（节点问题诊断）。

### Round 2 — 分支 B：只能 SSH 到节点，手动检查

**顾问**：请在控制平面节点执行：
> **步骤 1**：
> ```bash
> ls -la /etc/kubernetes/pki/ 2>/dev/null || ls -la /etc/kubernetes/ssl/ 2>/dev/null || echo "未找到标准证书目录"
> ```
> **如果目录不存在**：
> - **替代方案 A**：`find /etc -name "*.crt" 2>/dev/null | grep -E "kubernetes|kube|pki" | head -20`
> - **替代方案 B**：`grep -r "cert" /etc/kubernetes/manifests/kube-apiserver.yaml 2>/dev/null`
> - **替代方案 C**：托管集群请登录云控制台查看证书状态
>
> **步骤 2**：检查关键证书过期时间
> ```bash
> for cert in /etc/kubernetes/pki/*.crt /etc/kubernetes/pki/etcd/*.crt; do
>   [ -f "$cert" ] && echo "=== $(basename $cert) ===" && openssl x509 -in "$cert" -noout -dates 2>/dev/null
> done
> ```
> **如果 openssl 不可用**：
> - **替代方案 A**：`which openssl || apt-get install openssl -y || yum install openssl -y`
> - **替代方案 B**：通过文件时间戳推测：`stat <证书文件>`
> - **替代方案 C**：托管集群通过云控制台查看证书有效期

**根据结果**：发现过期证书 → Round 3 对应分支；未发现 → 重新评估是否为证书问题

### Round 2 — 分支 C：通过日志/事件确认

**顾问**：请获取关键日志：
> **步骤 1**：kube-system 事件
> ```bash
> kubectl get events -n kube-system --sort-by='.lastTimestamp' | grep -i "cert\|x509\|unauthorized\|fail" | tail -20
> ```
> **如果无法执行 kubectl**：
> - **替代方案 A**：请有权限的同事执行并转发
> - **替代方案 B**：日志聚合系统（ELK/Loki）查询近 1 小时含 `x509`、`certificate has expired` 的日志
> - **替代方案 C**：应用自身错误日志搜索 `x509`、`certificate`、`TLS`
>
> **步骤 2**：API Server 日志
> ```bash
> kubectl logs -n kube-system kube-apiserver-<节点名> --tail=50 2>&1 | grep -i "cert\|x509"
> ```
> **如果无法指定 Pod 名称**：
> - **替代方案 A**：`kubectl get pods -n kube-system | grep apiserver` 获取正确名称
> - **替代方案 B**：`kubectl logs -n kube-system -l component=kube-apiserver --tail=50`
> - **替代方案 C**：SSH 到控制平面节点，`cat /var/log/containers/kube-apiserver-*` 或 `crictl logs <apiserver-container-id>`
>
> **步骤 3**：kubelet 日志
> ```bash
> kubectl logs -n kube-system <kubelet-pod> --tail=50 2>&1 | grep -i "cert\|x509"
> ```
> **如果 kubelet 不以 Pod 运行**：
> - **替代方案 A**：SSH 到节点执行 `journalctl -u kubelet -n 50 --no-pager`
> - **替代方案 B**：`kubectl get events --field-selector reason=FailedToUpdateNodeStatus -A`
> - **替代方案 C**：查看 `/var/log/pods/kube-system_kubelet*` 日志文件

**根据日志结果的路径**：
- 明确显示 `certificate has expired` → Round 3 对应修复分支
- 显示 `unauthorized` 但无过期信息 → 可能是证书吊销或 RBAC 问题
- 无证书相关错误 → 可能不是证书问题，重新评估

## Round 3：修复与验证

### Round 3 — 分支 A：非 CA 证书过期，kubeadm 续期

**顾问**：以下证书需要续期：`[列出过期证书]`。CA 证书未过期，可用 kubeadm 安全续期。
> ⚠️ **重要提醒**：续期会导致控制平面组件重启，可能产生 **30-60 秒** 短暂不可用。请确认：
> 1. 当前是否处于业务低峰期？
> 2. 是否有维护窗口？
> 3. 是否已备份 `/etc/kubernetes/pki`？
>
> **如果未备份，请先执行**：
> ```bash
> cp -r /etc/kubernetes/pki /etc/kubernetes/pki.bak.$(date +%Y%m%d-%H%M%S)
> ```
> **如果无法备份到本地**：
> - **替代方案 A**：`tar czf /tmp/pki-backup-$(date +%Y%m%d).tar.gz /etc/kubernetes/pki`
> - **替代方案 B**：磁盘空间不足时至少备份关键文件：`cp /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/ca.key /tmp/`
>
> **确认备份完成后执行**：
> ```bash
> kubeadm certs renew all
> ```
> **如果只需续期特定证书**：
> - **替代方案 A**：`kubeadm certs renew apiserver`
> - **替代方案 B**：`kubeadm certs renew apiserver-etcd-client`
> - **替代方案 C**：`kubeadm certs renew front-proxy-client`
>
> 请告诉我输出结果。

**工程师反馈路径**：

#### A1：续期成功

**顾问**：续期成功。需要重启控制平面组件使新证书生效。
> **静态 Pod 部署**（kubeadm 默认）：
> ```bash
> mv /etc/kubernetes/manifests/kube-apiserver.yaml /tmp/ && sleep 5 && mv /tmp/kube-apiserver.yaml /etc/kubernetes/manifests/
> mv /etc/kubernetes/manifests/kube-controller-manager.yaml /tmp/ && sleep 5 && mv /tmp/kube-controller-manager.yaml /etc/kubernetes/manifests/
> mv /etc/kubernetes/manifests/kube-scheduler.yaml /tmp/ && sleep 5 && mv /tmp/kube-scheduler.yaml /etc/kubernetes/manifests/
> ```
> **如果无法移动文件**：
> - **替代方案 A**：`sudo` 执行上述命令
> - **替代方案 B**：`systemctl restart kubelet`（自动重启静态 Pod）
> - **替代方案 C**：等待 kubelet 自动检测变更（可能需几分钟），或重启节点
>
> **systemd 部署**：
> ```bash
> systemctl restart kube-apiserver kube-controller-manager kube-scheduler
> ```
> **如果 systemctl 无法使用**：
> - **替代方案 A**：`service kube-apiserver restart`
> - **替代方案 B**：`kill -SIGHUP <apiserver-pid>`（如果支持）
> - **替代方案 C**：重启对应节点
>
> 重启后等待 1-2 分钟，然后执行验证。

#### A2：续期失败

**顾问**：请贴出完整错误输出。常见失败原因：
> - `CA certificate is expired` → CA 已过期，见 Round 3-分支 B（升级）
> - `permission denied` → 需要 root 或 sudo
> - 版本不兼容 → kubeadm 版本与集群版本不匹配
> - etcd 相关错误 → 检查 etcd 集群状态

#### A3：工程师不确定是否可执行高风险操作

**顾问**：可降低风险：
> 1. **先在测试环境验证**
> 2. **逐步续期**：`kubeadm certs renew apiserver`，验证正常后再续期下一个
> 3. **选择低峰期执行**
>
> 如果仍不确定，**建议升级**给有经验的高级 SRE，我会提供完整诊断记录用于交接。

### Round 3 — 分支 B：CA 证书过期 — 升级决策

**顾问**：⚠️ **关键发现**：**CA 根证书已过期**。
> 这意味着：
> 1. 无法使用 kubeadm 直接续期（所有证书依赖 CA 签发）
> 2. 需要重新生成 CA，重新签发**所有组件证书**
> 3. 操作复杂，失误可能导致集群**完全不可恢复**
>
> **升级决策点**：
> - ✅ **生产集群且业务已中断** → **强烈建议立即升级**给 K8s 架构专家或云厂商支持
> - ✅ **使用外部 CA**（Vault、云厂商 CA）→ 需按外部 CA 流程重新签发，需专家介入
> - ✅ **测试/开发集群且有完整备份** → 可尝试处理，但仍需谨慎
>
> **如需继续（仅限非生产/有备份场景）**：
> 1. 备份 etcd：`etcdctl snapshot save`
> 2. 备份 `/etc/kubernetes` 目录
> 3. 按官方文档重建 CA 并重新签发所有证书：https://kubernetes.io/docs/tasks/tls/manual-rotation-of-ca-certificates/
>
> 你是否有完整备份？集群是生产环境吗？

### Round 3 — 分支 C：kubelet 证书问题 — CSR 处理

**顾问**：确认是 kubelet 证书问题。请先确认集群的 [[skills/kubelet-certificate-rotation|kubelet 证书轮换机制]]（自动/手动）。
> **自动轮换场景**，请执行：
> ```bash
> kubectl get csr
> ```
> **如果 kubectl 无法使用**：
> - **替代方案 A**：请有权限的同事执行并转发
> - **替代方案 B**：SSH 到控制平面节点，`ls -la /var/lib/kubelet/pki/`

#### C1：有 Pending 的 CSR

**顾问**：kubelet 已申请新证书，只需批准：
> ```bash
> kubectl get csr | grep Pending
> kubectl certificate approve <csr-name>
> ```
> **如果 CSR 太多**：
> - **替代方案 A**：`kubectl get csr | grep Pending | awk '{print $1}' | xargs kubectl certificate approve`
> - **替代方案 B**：先检查 CSR 详情：`kubectl describe csr <csr-name>`，确认 `Requesting User` 是 `system:node:<节点名>`
>
> 批准后等待 1-2 分钟，执行 `kubectl get nodes` 检查节点是否恢复 Ready。

#### C2：没有 Pending 的 CSR

**顾问**：kubelet 未发起证书申请。可能原因：自动轮换被禁用、当前证书已过期无法连接 API Server、kubelet 服务异常。
> 请检查：
> ```bash
> cat /var/lib/kubelet/config.yaml | grep -A5 rotate
> ```
> **如果无法 SSH**：
> - **替代方案 A**：`kubectl debug node/<node-name> -it --image=busybox`
> - **替代方案 B**：`ps aux | grep kubelet | grep rotate`
>
> 查看 `rotateCertificates` 是否为 `true`。如为 `false`，需修改配置并重启 kubelet：
> ```bash
> systemctl restart kubelet
> ```
> **如果无法重启**：
> - **替代方案 A**：`service kubelet restart`
> - **替代方案 B**：`kill -SIGHUP <kubelet-pid>`
> - **替代方案 C**：重启节点
>
> 重启后再次检查 `kubectl get csr`，看是否有新 Pending CSR。

#### C3：CSR 批准了但节点仍 NotReady

**顾问**：CSR 已批准但节点未恢复。请检查 kubelet 日志：
> ```bash
> journalctl -u kubelet -n 50 --no-pager
> ```
> **如果无法查看 journalctl**：
> - **替代方案 A**：`kubectl logs -n kube-system <kubelet-pod>`
> - **替代方案 B**：`cat /var/log/syslog | grep kubelet | tail -50`
>
> 检查是否有以下错误：
> - `certificate signed by unknown authority` → CA 证书不匹配
> - `connection refused` → API Server 不可达
> - `node "xxx" not found` → 节点注册信息丢失
>
> 如果无法解决，**建议升级**至 SKILL-NODE-001。

## 验证修复

**顾问**：修复完成。请执行以下验证：
> **步骤 1**：验证证书已续期
> ```bash
> kubeadm certs check-expiration
> ```
> **如果 kubeadm 不可用**：
> - **替代方案 A**：`openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates`
> - **替代方案 B**：`for f in /etc/kubernetes/pki/*.crt; do openssl x509 -in "$f" -noout -dates 2>/dev/null | grep notAfter; done`
>
> **步骤 2**：验证节点状态
> ```bash
> kubectl get nodes -o wide
> ```
> **所有节点应显示 Ready**。
>
> **步骤 3**：验证 Pod 状态
> ```bash
> kubectl get pods -n kube-system
> ```
> **所有应处于 Running 或 Completed**。
>
> **步骤 4**：验证 API Server 响应
> ```bash
> kubectl get pods -A | head -10
> ```
>
> **步骤 5**：验证 kubelet 证书（如修复过）
> ```bash
> openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates
> ```
> **如果路径不同**：
> - **替代方案 A**：`find /var/lib/kubelet/pki -name "*.pem" | head -5`
> - **替代方案 B**：`cat /var/lib/kubelet/kubelet.conf | grep client-certificate`
>
> 请贴出验证结果。

### 验证结果处理

#### 验证通过

**顾问**：✅ 验证通过！证书已续期，集群恢复正常。
> **后续建议**：
> 1. **设置证书过期监控**：配置告警，到期前 30 天/7 天触发（指标：`apiserver_client_certificate_expiration_seconds_count`）
> 2. **记录本次事件**：运维日志中记录问题、修复时间、影响范围
> 3. **审查证书管理流程**：如重复发生，建立证书生命周期管理
> 4. **清理备份**：确认修复成功后，保留备份一段时间再删除

#### 验证部分失败

**顾问**：部分验证未通过。请告诉我：
> 1. 哪些步骤失败？
> 2. 失败的具体输出？
> 3. 失败的是控制平面还是工作节点？
>
> 根据失败部分继续排查：
> - 部分节点仍 NotReady → Round 2-分支 C（kubelet 深入排查）
> - API Server 仍有问题 → 检查 apiserver 日志和证书配置
> - 证书已续期但组件仍报错 → 可能是组件缓存旧证书，需重启对应组件
>
> 如果反复排查无法解决，**建议升级**给高级 SRE。


### 分支 1.4：阿里云ACK/专有云证书特定排查

工程师："我们在阿里云ACK/专有云环境，证书即将过期"

顾问："阿里云环境有额外的证书管理维度，请按以下顺序排查：

**步骤 1：阿里云SSL证书服务检查**
请登录阿里云控制台或使用CLI检查：

```bash
# 检查阿里云SSL证书列表和到期时间
aliyun cas DescribeUserCertificateList --RegionId cn-hangzhou

# 检查ACK Ingress使用的证书
kubectl get ingress -A -o yaml | grep -A 5 secretName

# 检查SLB绑定的证书
aliyun slb DescribeServerCertificates --RegionId <region>
```

> **如果无法执行aliyun CLI**：请登录阿里云控制台，进入SSL证书控制台，告诉我：
> 1. 即将过期的证书是否已在阿里云SSL证书服务中？
> 2. 是否有已购买的续期证书？

**步骤 2：ACK托管证书检查**
```bash
# 检查ACK是否使用托管证书
kubectl get secret -n kube-system | grep cert

# 检查cert-manager状态（如果使用）
kubectl get pods -n cert-manager

# 检查证书自动续期配置
kubectl get clusterissuer,certificate -A
```

**步骤 3：专有云证书特殊考虑**
- 专有云环境可能使用自签名证书或内部CA
- 检查飞天/天基组件证书：`find /etc/aso -name "*.crt" -o -name "*.pem"`
- 确认专有云版本对应的证书管理策略

> **如果无法SSH到节点**：请通过天基控制台检查节点证书状态。

**步骤 4：阿里云证书续期操作**

如使用阿里云托管证书：
1. 在阿里云SSL证书控制台购买/续期证书
2. 下载新证书并更新K8s Secret
3. 如使用ALB Ingress，在ALB控制台更新证书绑定
4. 验证：curl -v https://<domain> 确认新证书生效

如使用cert-manager自动续期：
```bash
# 强制触发续期
kubectl cert-manager renew --all-namespaces

# 检查续期状态
kubectl get certificate -A -w
```


## 升级条件汇总

| 场景 | 升级目标 | 原因 |
|------|---------|------|
| CA 证书已过期 | 专家级事件响应 / 云厂商支持 | 需集群级证书重建，风险极高 |
| 完全无法连接集群 | 现场高级 SRE / 云厂商支持 | 需物理/堡垒机登录修复 |
| 证书续期后节点仍 NotReady | SKILL-NODE-001 | 可能涉及非证书问题 |
| 续期失败且原因不明 | 专家级事件响应 | 可能涉及版本兼容、配置损坏 |
| 非 kubeadm 部署且无文档 | 专家级事件响应 | 二进制部署证书管理路径各异 |
| 生产集群且无操作权限 | 集群管理员 | 权限限制 |

## 对话结束

**顾问**：本次证书过期问题诊断对话已结束。
> **总结**：
> - 诊断结果：`[填入结论]`
> - 执行的操作：`[填入已执行操作]`
> - 最终状态：`[已解决 / 已升级 / 需后续跟进]`
> - 后续措施：`[监控/流程改进建议]`
>
> 如有新的证书相关问题，可引用本次对话或重新发起。还有其他我可以帮助的吗？
## Related

- [[domain-17-system-foundation/topic-dictionary/networking/ingress|Ingress]]
