---
title: "控制平面问题 — 远程顾问对话脚本"
category: "troubleshooting"
tags: ["cluster", "remote-consultant"]
created: "2026-05-23"
updated: "2026-05-23"
dialogue_id: "DIALOGUE-K8S_CONTROL_PLANE"
skill_id: "k8s-control-plane"
version: "1.0.0"
role: "remote-consultant"
language: "zh"
summary: "控制平面问题的远程顾问对话脚本，覆盖API Server、etcd、Scheduler排查。"
relationships:
  - target: "[[entities/etcd]]"
    type: uses
  - target: "[[entities/helm]]"
    type: uses
  - target: "[[entities/kubernetes]]"
    type: uses
  - target: "[[domain-17-system-foundation/topic-dictionary/networking/service]]"
    type: uses
---

# 控制平面组件问题 — 远程顾问对话脚本

> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**。

## 对话入口
### 入口 A
**工程师**：kubectl命令无响应，apiserver连接超时

**顾问回应**：先确认三个问题：
1. **影响范围**：哪些服务/应用受到影响？
2. **紧急程度**：是否影响生产流量？
3. **发生时间**：何时开始出现问题？

### 入口 B
**工程师**：调度器异常，Pod无法被调度到新节点

**顾问回应**：先确认三个问题：
1. **影响范围**：哪些服务/应用受到影响？
2. **紧急程度**：是否影响生产流量？
3. **发生时间**：何时开始出现问题？

### 入口 C
**工程师**：etcd集群成员掉线，quorum丢失

**顾问回应**：先确认三个问题：
1. **影响范围**：哪些服务/应用受到影响？
2. **紧急程度**：是否影响生产流量？
3. **发生时间**：何时开始出现问题？

## Round 1
### 分支 1：apiserver无响应
- `kubectl get --raw /healthz`
  - 如无法执行：请提供当前可执行的环境信息
- `curl -k https://<apiserver>:6443/healthz`
  - 如无法执行：请提供当前可执行的环境信息
- `systemctl status kube-apiserver (在master节点)`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 2：调度器异常
- `kubectl get pods -n kube-system -l component=kube-scheduler`
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl logs -n kube-system -l component=kube-scheduler --tail=50`
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl get events --field-selector reason=FailedScheduling`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 3：etcd异常
- `kubectl exec -it [[entities/etcd|etcd]]-<node> -n kube-system -- etcdctl endpoint health`
  > 💬 **顾问确认**：如输出与预期不符，请停止操作并立即反馈。
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl exec -it etcd-<node> -n kube-system -- etcdctl member list`
  - 如无法执行：请提供当前可执行的环境信息
- `df -h /var/lib/etcd (检查etcd磁盘)`
  - 如无法执行：请提供当前可执行的环境信息

## Round 2
### 分支 1：apiserver崩溃
- `journalctl -u kube-apiserver -n 100 --no-pager`
  - 如无法执行：请提供当前可执行的环境信息
- `检查证书有效期: openssl x509 -in /[[entities/kubernetes|kubernetes]]/pki/apiserver.crt -noout -dates`
  - 如无法执行：请提供当前可执行的环境信息
- `如证书过期: kubeadm certs renew all`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 2：调度器Pending
- `kubectl describe pod -n kube-system -l component=kube-scheduler`
  - 如无法执行：请提供当前可执行的环境信息
- `检查节点资源: kubectl describe node <node>`
  - 如无法执行：请提供当前可执行的环境信息
- `如调度器Pod异常: kubectl rollout restart deployment kube-scheduler -n kube-system`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 3：etcd修复
- `etcdctl snapshot save /tmp/etcd-backup.db`
  > 💬 **顾问确认**：请确认上述命令的输出，将结果贴回给我。
  - 如无法执行：请提供当前可执行的环境信息
- `如磁盘满: 清理日志和快照`
  - 如无法执行：请提供当前可执行的环境信息
- `如成员掉线: etcdctl member remove <member-id> && etcdctl member add <name> --peer-urls=<url>`
  - 如无法执行：请提供当前可执行的环境信息

## Round 3
### 分支 1：组件恢复验证
- `kubectl get nodes`
  - 如无法执行：请提供当前可执行的环境信息
- `kubectl get pods --all-namespaces`
  - 如无法执行：请提供当前可执行的环境信息
- `curl -k https://<apiserver>:6443/healthz`
  - 如无法执行：请提供当前可执行的环境信息

### 分支 2：etcd集群恢复
- `etcdctl endpoint health --endpoints=<all-endpoints>`
  - 如无法执行：请提供当前可执行的环境信息
- `etcdctl endpoint status --endpoints=<all-endpoints>`
  - 如无法执行：请提供当前可执行的环境信息
- `验证quorum: etcdctl member list`
  - 如无法执行：请提供当前可执行的环境信息


### 分支 1.4：阿里云ACK/专有云控制平面排查

工程师："我们在阿里云ACK/专有云环境，控制平面异常"

顾问："阿里云环境控制平面有特殊架构，请按以下顺序排查：

**步骤 1：ACK托管版与专有版区分**
```bash
# 检查ACK类型
kubectl get nodes -o wide
# 托管版：Master节点由阿里云管理，不可见
# 专有版：Master节点可见，可SSH

# 检查ACK集群状态
aliyun cs GET /clusters/<cluster-id>
```

> **如果无法执行aliyun CLI**：请登录ACK控制台，告诉我：
> 1. 集群类型是托管版还是专有版？
> 2. 集群状态是否为运行中？

**步骤 2：托管版控制平面检查**
托管版Master由阿里云管理，排查重点：
```bash
# 检查APIServer可用性
kubectl cluster-info

# 检查APIServer负载
kubectl top pod -n kube-system | grep apiserver

# 检查ACK组件状态
kubectl get pods -n kube-system | grep ack-
```

> **如APIServer完全不可用**：立即联系阿里云技术支持（托管版Master不可直接修复）。

**步骤 3：专有版控制平面检查**
专有版Master可SSH访问：
```bash
# SSH到Master节点
ssh root@<master-ip>

# 检查Master节点资源
free -h && df -h

# 检查etcd状态（专有版可见）
etcdctl endpoint health --endpoints=https://127.0.0.1:2379

# 检查飞天组件状态
curl http://localhost:7070/api/v1/status
```

**步骤 4：专有云升级决策**

| 场景 | 处理方式 |
|:---|:---|
| 托管版APIServer不可用 | 联系阿里云技术支持 |
| 专有版etcd问题 | 参考etcd恢复文档 |
| 飞天组件异常 | 联系阿里云TAM |
| 天基控制台无法访问 | P0升级给驻场工程师 |


## 升级决策点
- **P0（立即升级）**：集群核心功能受损，多服务中断
- **P1（建议升级）**：单服务中断，有 workaround
- **P2（观察）**：非关键路径，可稍后处理

## 附录：常用命令速查
| 场景 | 命令 |
|:---|:---|
| 查看资源 | `kubectl get <resource> -n <ns>` |
| 查看详情 | `kubectl describe <resource> <name> -n <ns>` |
| 查看日志 | `kubectl logs <pod> -n <ns>` |
| 进入容器 | `kubectl exec -it <pod> -n <ns> -- /bin/sh` |

## Round 1 补充 — 控制器管理器状态

### 分支 4：controller-manager异常
- `kubectl get pods -n kube-system -l component=kube-controller-manager`
  > 💬 **顾问确认**：请确认上述命令的输出，将结果贴回给我。
  - 如无法执行：请描述控制器管理器状态
- `kubectl logs -n kube-system -l component=kube-controller-manager --tail=50`
  - 如无法执行：请提供控制器日志片段
- `kubectl get nodes -o wide | grep NotReady`
  - 如无法执行：请提供节点状态列表

### 分支 5：API Server证书检查
- `openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates`
  - 如无法执行：请提供证书有效期信息
- `openssl x509 -in /etc/kubernetes/pki/apiserver-etcd-client.crt -noout -dates`
  > 💬 **顾问确认**：请检查输出是否符合预期，确认无误后再继续下一步。
  - 如无法执行：请提供etcd客户端证书信息
- `kubeadm certs check-expiration`
  - 如无法执行：请提供证书过期检查结果

## Round 2 补充 — 高级诊断

### 分支 4：Webhook问题排查
- `kubectl get validatingwebhookconfiguration`
  - 如无法执行：请提供验证Webhook配置
- `kubectl get mutatingwebhookconfiguration`
  > 💬 **顾问确认**：如果命令执行失败，请提供错误信息，我会调整方案。
  - 如无法执行：请提供变更Webhook配置
- `kubectl delete validatingwebhookconfiguration <name>` (如Webhook阻塞API)
  - 如无法执行：请确认是否可以临时禁用Webhook

### 分支 5：API Server性能分析
- `curl -k https://<apiserver>:6443/metrics | grep apiserver_request_duration_seconds`
  - 如无法执行：请提供API Server性能指标
- `kubectl top pod -n kube-system -l component=kube-apiserver`
  - 如无法执行：请描述API Server资源使用情况
- `检查API Server的--max-requests-inflight和--max-mutating-requests-inflight参数`
  > 💬 **顾问确认**：在执行危险操作前，请再次确认当前备份状态。
  - 如无法执行：请提供API Server启动参数

## Round 3 补充 — 高级修复

### 分支 3：控制平面节点迁移
- `kubeadm init phase upload-certs --upload-certs`
  - 如无法执行：请确认是否有新节点可加入
- `kubeadm token create --print-join-command`
  - 如无法执行：请提供加入命令
- `在新节点执行kubeadm join`
  > 💬 **顾问确认**：请确认上述命令的输出，将结果贴回给我。
  - 如无法执行：请描述新节点准备情况

### 分支 4：etcd数据恢复
- `etcdctl snapshot restore /tmp/etcd-backup.db --data-dir=/var/lib/etcd-new`
  - 如无法执行：请确认是否有备份文件
- `停止etcd，替换数据目录，重启`
  - 如无法执行：请描述当前etcd数据目录位置
- `验证集群状态: etcdctl endpoint health --endpoints=<all>`
  - 如无法执行：请提供各节点状态

## 升级决策点（补充）

- **P0-CRITICAL**：etcd quorum丢失或数据损坏，集群完全不可用
- **P0**：API Server证书全部过期，所有认证失败
- **P1**：单控制平面节点问题，多节点集群仍可工作
- **P2**：控制平面性能下降，暂无业务影响

## 附录：控制平面问题排查决策树

```
kubectl无法连接
    ├── 网络问题 → 检查防火墙/安全组/网络策略
    ├── 证书问题 → 检查证书有效期，必要时renew
    ├── API Server崩溃 → 检查日志，重启组件
    └── etcd问题
            ├── 磁盘满 → 清理空间/defrag
            ├── 成员掉线 → 移除/重新添加成员
            └── 数据损坏 → 从快照恢复
```

| 限制场景 | 替代方案 | 降级策略 |
|:---|:---|:---|
| 无法SSH到master节点 | 使用云提供商控制台或BMC | 通过工作节点代理执行 |
| 无法使用kubeadm | 手动操作证书和静态Pod | 使用备份恢复 |
| etcd无备份 | 尝试从其他成员同步 | 如quorum丢失，可能需要重建集群 |
| API Server完全不可用 | 通过直接访问etcd获取信息 | 使用静态Pod定义文件重启 |

## Round 1 扩展 — 新增分支

### 分支 6：控制平面网络连通性检查
- `kubectl get nodes -o wide`
  > 💬 **顾问确认**：请确认上述命令的输出，将结果贴回给我。如果节点状态全部为 NotReady，请优先排查网络。
  - 如无法执行：请提供集群节点列表截图或节点状态描述
- `ping -c 3 <apiserver-endpoint>`（从工作节点执行）
  - 如无法执行：请使用 `curl -v https://<apiserver>:6443/healthz` 替代
- `nc -zv <etcd-endpoint> 2379`（测试 etcd 端口连通性）
  - 如无法执行：请提供网络连通性测试结果或描述网络架构

### 分支 7：资源耗尽排查
- `kubectl top nodes`
  > 💬 **顾问确认**：如果命令执行失败，请提供错误信息，我会调整方案。
  - 如无法执行：请提供监控系统的节点资源截图
- `kubectl describe node <master-node> | grep -A 20 'Allocated resources'`
  - 如无法执行：请提供节点的资源分配情况描述
- `df -h /var/lib/kubelet /var/log /tmp`
  - 如无法执行：请提供磁盘使用率信息或截图

## Round 2 扩展 — 新增分支

### 分支 6：etcd 性能深度分析
- `ETCDCTL_API=3 etcdctl endpoint status --write-out=table`
  > 💬 **顾问确认**：请重点关注 `DB SIZE` 和 `RAFT TERM` 列。如有异常请立即说明。
  - 如无法执行：请提供 etcd 集群各节点的状态描述
- `ETCDCTL_API=3 etcdctl check perf`
  - 如无法执行：请描述 etcd 响应速度是否变慢
- `iostat -x 1 5`（在 etcd 节点执行）
  > 💬 **顾问确认**：请检查 `%util` 列是否持续接近 100%，这表示磁盘 IO 瓶颈。
  - 如无法执行：请提供磁盘性能监控截图

### 分支 7：控制平面证书链完整性检查
- `openssl crl2pkcs7 -nocrl -certfile /etc/kubernetes/pki/ca.crt | openssl pkcs7 -print_certs -noout | grep 'subject'`
  - 如无法执行：请提供 CA 证书信息
- `openssl verify -CAfile /etc/kubernetes/pki/ca.crt /etc/kubernetes/pki/apiserver.crt`
  > 💬 **顾问确认**：请确认输出是否为 `OK`。如显示错误，证书链可能已损坏。
  - 如无法执行：请提供证书验证结果描述
- `for cert in /etc/kubernetes/pki/*.crt; do echo "$cert: $(openssl x509 -in $cert -noout -enddate)"; done`
  - 如无法执行：请提供所有证书的有效期列表

## Round 3 扩展 — 新增分支

### 分支 5：控制平面组件资源调整
- `kubectl set resources deployment kube-apiserver -n kube-system --limits=memory=8Gi,cpu=4000m`
  > 💬 **顾问确认**：这是一个变更操作，请再次确认当前是否处于维护窗口，以及是否已备份 etcd。
  - 如无法执行：请确认是否可以通过编辑静态 Pod manifest 调整资源限制
- `kubectl rollout status deployment kube-apiserver -n kube-system`
  - 如无法执行：请手动检查 apiserver Pod 是否已重启并 Running
- `kubectl top pod -n kube-system -l component=kube-apiserver`
  - 如无法执行：请提供资源调整后的 Pod 状态描述

### 分支 6：集群级修复验证
- `kubectl get cs`（检查组件状态，兼容旧版本）
  - 如无法执行：请使用 `kubectl get pods -n kube-system` 替代
- `kubectl auth can-i '*' '*'`（验证 API Server 认证授权正常）
  > 💬 **顾问确认**：请确认输出为 `yes`。如显示 `no`，RBAC 可能存在问题。
  - 如无法执行：请尝试执行 `kubectl get ns` 并反馈结果
- `kubectl run nginx-test --image=nginx --restart=Never --rm -it -- echo "API Server test passed"`
  - 如无法执行：请手动创建一个测试 Pod 验证调度链路

## 顾问确认语气短语大全

> 在对话中，顾问应根据场景使用以下确认短语，确保工程师理解指令并在执行前进行确认。

### 执行前确认
- "请确认您已理解上述步骤，确认无误后再继续执行。"
- "在执行此操作前，请再次确认当前是否处于维护窗口。"
- "这是一个高危操作，请确认您已备份相关数据。"
- "请确认您当前操作的节点和集群名称，避免误操作。"
- "操作前请确认您的 kubectl context 指向正确的集群：`kubectl config current-context`"

### 执行中确认
- "命令已开始执行，请稍等片刻后贴出完整输出。"
- "如果命令执行时间较长（超过 30 秒），请告诉我当前的输出状态。"
- "如输出中有任何 `Error` 或 `Warning` 字样，请立即停止并反馈。"
- "请确认命令是否成功返回，如有异常请优先贴出错误信息。"
- "如果命令卡住不动，请使用 Ctrl+C 中断后告诉我现象。"

### 结果确认
- "请确认输出是否符合预期。如果与预期不符，请告诉我差异点。"
- "结果看起来正常，请再次确认是否还有其他异常现象。"
- "修复步骤已完成，请确认业务是否恢复正常。"
- "请确认问题是否已完全解决，还是仅部分缓解。"
- "如修复后问题反复出现，请记录复现步骤并反馈。"

### 升级/交接确认
- "当前情况建议升级处理。请确认是否已联系高级 SRE。"
- "我已准备好问题摘要，请确认是否需要我同步给其他团队成员。"
- "请确认是否需要在维护窗口后安排事后复盘会议。"

## "如果无法执行" 替代方案扩展

> 以下替代方案覆盖更多受限场景，顾问应根据工程师反馈的实际限制灵活切换。

### 场景：无 kubectl 权限
| 原命令 | 替代方案 A | 替代方案 B | 替代方案 C |
|:---|:---|:---|:---|
| `kubectl get pods` | 通过 Kubernetes Dashboard 查看 | 请集群管理员提供只读 kubeconfig | 使用云厂商控制台（EKS/GKE/ACK） |
| `kubectl logs` | 查看应用日志文件 `/var/log/containers/` | 通过日志系统（Loki/ELK）查询 | 使用 `crictl logs`（如有节点权限） |
| `kubectl describe node` | 查看云厂商监控面板 | 使用 `kubectl get node -o yaml` | 请管理员导出节点详情 |

### 场景：无法访问 etcd 节点
| 原命令 | 替代方案 A | 替代方案 B | 替代方案 C |
|:---|:---|:---|:---|
| `etcdctl endpoint health` | 通过 apiserver 间接检查：`kubectl get --raw /healthz/etcd` | 查看 etcd Pod 日志 | 检查 etcd [[domain-17-system-foundation/topic-dictionary/networking/service|Service]] Endpoint |
| `etcdctl member list` | 查看 etcd Pod 状态推断成员数 | 使用 `kubectl get endpoints -n kube-system etcd` | 请管理员协助执行 |
| `etcdctl snapshot save` | 检查是否有自动化备份（Velero/etcd-backup-operator） | 使用云厂商 etcd 备份功能 | 联系 DBA 或存储团队协助 |

### 场景：网络隔离无法直连
| 原命令 | 替代方案 A | 替代方案 B | 替代方案 C |
|:---|:---|:---|:---|
| `curl https://apiserver:6443` | 通过跳板机/堡垒机执行 | 在工作节点上执行 `curl -k https://<apiserver-ip>:6443` | 使用 VPN 连接后重试 |
| `ssh master-node` | 使用云厂商 Serial Console | 使用 `kubectl debug node/<node>` 进入节点 | 通过运维平台（如 Ansible Tower）执行 |
| 下载诊断工具 | 使用容器镜像内置工具 | 使用 busybox 镜像：`kubectl run debug --rm -it --image=busybox -- /bin/sh` | 手动上传工具到节点 |

### 场景：安全策略禁止执行命令
| 原命令 | 替代方案 A | 替代方案 B | 替代方案 C |
|:---|:---|:---|:---|
| 任何 kubectl 命令 | 提供截图和描述 | 审查现有配置文件（[[entities/helm|Helm]]） | 转为文档指导和流程确认 |
| 修改静态 Pod manifest | 通过 GitOps/Config Management 下发 | 联系有权限的安全管理员 | 使用声明式 API 而非直接修改节点 |
| etcd 数据操作 | 完全依赖自动化备份恢复流程 | 联系平台团队执行 | 准备重建集群的方案 |

### 通用降级沟通策略

当工程师表示完全无法执行任何命令时，按以下优先级收集信息：

1. **视觉信息**（最高优先级）
   - 监控系统截图（Grafana/Datadog/云厂商控制台）
   - 告警通知原文（请直接复制粘贴）
   - 错误页面截图或文本

2. **描述性信息**
   - 问题现象的详细文字描述
   - 问题发生前后的操作记录
   - 业务层面的异常表现（哪些服务不可用）

3. **变更信息**
   - 最近 24 小时内的所有变更列表
   - 是否有新版本发布、配置变更、扩缩容
   - 是否有安全补丁或系统更新

4. **环境信息**
   - 集群规模（节点数、Pod 数）
   - 使用的发行版（EKS/GKE/ACK/自建）
   - 网络架构（VPC、子网、防火墙规则）

> 💬 **顾问确认**：如果您当前完全无法执行命令，请从以上 4 类信息中提供您能获得的内容。即使只有部分信息，我也可以给出初步判断。

## 相关案例

- [[synthesis/case-studies/2026-07-15--admission-webhook超时导致所有api操作失败.md|2026-07-15--admission-webhook超时导致所有api操作失败]]
## Related

- [[domain-17-system-foundation/03-kubernetes-events/02-pod-container-lifecycle-events|02 - Pod 与容器生命周期事件]]
