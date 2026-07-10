---
title: kubectl 插件实战指南
description: 面向阿里云/专有云 K8s 运维的 kubectl 插件指南，涵盖 Krew 管理、ktop、kubectl-trace、kubectl-node-shell
  等常用插件。
summary: 面向阿里云/专有云 K8s 运维的 kubectl 插件指南，涵盖 Krew 管理、ktop、kubectl-trace、kubectl-node-shell
  等常用插件。
category: troubleshooting
tags:
- k8s
- kubectl
- krew
- plugins
- ktop
- kubectl-trace
- diagnostics
tier: supporting
created: '2026-06-29'
updated: '2026-06-29'
last_updated: 2026-06
difficulty: intermediate
reading_level: intermediate
audience:
- SRE
- 运维工程师
- 技术支持
estimated_read_time: 20min
intent_queries:
- kubectl 插件推荐
- Krew 安装使用
- ktop kubectl-trace 使用
trigger_keywords:
- kubectl
- krew
- ktop
- kubectl-trace
- 插件
prerequisites:
- kubectl-basics
- linux-basics
- shell-basics
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




# kubectl 插件实战指南

> **适用版本**: Kubernetes v1.28 - v1.32 | **最后更新**: 2026-06
> **文档定位**: 面向阿里云/专有云 K8s 运维，系统介绍常用 kubectl 插件的安装、使用与故障诊断场景。

## 目录

1. [Krew 插件管理器](#krew-插件管理器)
2. [ktop：集群资源拓扑](#ktop集群资源拓扑)
3. [kubectl-trace：系统调用追踪](#kubectl-trace系统调用追踪)
4. [kubectl-node-shell：节点 Shell](#kubectl-node-shell节点-shell)
5. [kubectl-view-allocations：资源分配](#kubectl-view-allocations资源分配)
6. [kubectl-neat：YAML 清理](#kubectl-neatyaml-清理)
7. [实用插件组合](#实用插件组合)
8. [最佳实践检查清单](#最佳实践检查清单)

---

## 1. Krew 插件管理器

### 1.1 安装 Krew

```bash
# macOS / Linux
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower:]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/\(arm\)\(64\)\?.*/\1\2/' -e 's/aarch64$/arm64/')" &&
  KREW="krew-${OS}_${ARCH}" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz" &&
  tar zxvf "${KREW}.tar.gz" &&
  ./"${KREW}" install krew
)
```

### 1.2 Krew 常用命令

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 更新索引
kubectl krew update

# 搜索插件
kubectl krew search top

# 安装插件
kubectl krew install ktop

# 列出已安装插件
kubectl krew list

# 升级插件
kubectl krew upgrade ktop

# 卸载插件
kubectl krew uninstall ktop
```
---

## 2. ktop：集群资源拓扑

### 2.1 安装与使用

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install ktop
kubectl ktop
```
### 2.2 常用视图

| 按键 | 视图 |
|:---|:---|
| `1` | Node 视图 |
| `2` | Namespace 视图 |
| `3` | Pod 视图 |
| `4` | Container 视图 |
| `/` | 搜索 |
| `q` | 退出 |

### 2.3 使用场景

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 快速定位高 CPU Pod
kubectl ktop --namespace production
```
---

## 3. kubectl-trace：系统调用追踪

### 3.1 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install trace
```
### 3.2 追踪 Pod 系统调用

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 bpftrace 脚本追踪 Pod 的文件打开操作
kubectl trace run <pod-name> -n <namespace> \
  -e 'kprobe:do_sys_open { printf("%s: %s\n", comm, str(arg1)) }'
```
### 3.3 追踪节点内核事件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 追踪节点上的 TCP 重传
kubectl trace run node/<node-name> \
  -e 'kprobe:tcp_retransmit { printf("retransmit: %s -> %s\n", saddr, daddr) }'
```
---

## 4. kubectl-node-shell：节点 Shell

### 4.1 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install node-shell
```
### 4.2 进入节点调试

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 直接进入节点 Shell
kubectl node-shell <node-name>

# 在节点上执行一次性命令
kubectl node-shell <node-name> -- crictl ps
```
### 4.3 使用场景

- 节点磁盘、网络、kubelet 问题排查
- 直接访问 containerd/docker 命令
- 查看节点日志与系统状态

---

## 5. kubectl-view-allocations：资源分配

### 5.1 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install view-allocations
```
### 5.2 查看资源分配

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 按节点查看 CPU/Memory 分配
kubectl view-allocations --utilization

# 按命名空间汇总
kubectl view-allocations --namespace --group-by namespace
```
---

## 6. kubectl-neat：YAML 清理

### 6.1 安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install neat
```
### 6.2 清理冗余字段

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出干净的资源 YAML
kubectl get deployment order-service -n production -o yaml | kubectl neat

# 直接获取干净 YAML
kubectl neat get deployment order-service -n production -o yaml
```
---

## 7. 实用插件组合

### 7.1 推荐插件列表

| 插件 | 用途 |
|:---|:---|
| ktop | 资源监控 |
| trace | bpftrace 追踪 |
| node-shell | 节点调试 |
| view-allocations | 资源分配 |
| neat | YAML 清理 |
| ctx / ns | 快速切换 context/namespace |
| cert-manager | 证书管理 |
| access-matrix | RBAC 权限检查 |
| who-can | 查询谁有权限 |
| sniff | 抓包 |

### 7.2 批量安装

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
cat > /tmp/krew-plugins.txt <<EOF
ktop
trace
node-shell
view-allocations
neat
ectx
ns
cert-manager
access-matrix
who-can
sniff
EOF

for p in $(cat /tmp/krew-plugins.txt); do
  kubectl krew install $p
done
```
---

## 8. 最佳实践检查清单

| 检查项 | 要求 | 验证方式 |
|:---|:---|:---|
| Krew 已安装 | 所有运维节点 | `kubectl krew version` |
| 常用插件安装 | ktop/trace/node-shell | `kubectl krew list` |
| 插件版本更新 | 月度更新 | `kubectl krew upgrade` |
| 权限最小化 | 插件仅用于诊断 | RBAC |
| 使用文档 | 团队共享 | Wiki |

---

## 插件权限与审计

kubectl 插件执行时继承当前 kubeconfig 权限。生产环境应遵循最小权限原则，并为不同角色配置只读或读写权限。

| 角色 | 推荐插件权限 |
|:---|:---|
| 值班工程师 | 只读插件（ktop、stern、cost、ctx、ns） |
| SRE | 读写插件 + trace（受审批） |
| 安全团队 | trace、inspektor-gadget、sniff（受审计） |

### 离线环境部署

专有云或隔离网络无法访问 GitHub 时，可提前下载插件并托管到内部镜像仓库或文件服务器。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 在有外网的环境下载插件包
kubectl krew download <plugin>

# 2. 将 tar 包上传到内部仓库
ossutil cp <plugin>.tar.gz oss://internal-tools/krew-plugins/

# 3. 在离线环境安装
kubectl krew install --manifest=<plugin>.yaml --archive=<plugin>.tar.gz
```
### 插件使用审计

建议记录关键插件的使用人、时间与目标资源，便于安全审计与故障追溯。

## 插件使用场景速查

| 场景 | 推荐插件 | 命令示例 |
|:---|:---|:---|
| 查看集群整体负载 | ktop | `kubectl ktop` |
| 追踪多 Pod 日志 | stern | `kubectl stern -n prod -l app=api` |
| 调试内核级问题 | kubectl-trace | `kubectl trace run node-1 -e '...'` |
| 切换上下文/命名空间 | ctx / ns | `kubectl ctx prod` |
| 查看资源成本 | cost | `kubectl cost namespace` |
| 查看资源精简配置 | neat | `kubectl neat get pod ...` |

### 插件版本管理

建议将插件版本与集群版本对应，避免不兼容：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出当前插件列表
kubectl krew list > krew-plugins.txt

# 在另一环境批量安装
xargs -n1 kubectl krew install < krew-plugins.txt
```
## 典型工单场景与处理

**场景**：用户报告某服务 Pod 日志分散，难以定位错误。

处理步骤：
1. 使用 stern 按标签聚合日志。
2. 结合 grep 过滤 ERROR 或关键字。
3. 如日志不足，使用 `kubectl logs --previous` 查看崩溃前日志。
4. 对高频问题配置日志告警。

## 离线环境与私有仓库

在专有云或隔离网络中，插件安装需要提前准备离线包。

### 离线安装步骤

1. 在外网环境下载 Krew 与所需插件。
2. 将插件包上传到内部文件服务器或 OSS。
3. 在目标环境安装 Krew 并配置本地索引。
4. 使用 `--manifest` 与 `--archive` 参数安装插件。

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 导出已安装插件列表
kubectl krew list > krew-plugins.txt

# 在外网打包插件目录
tar czvf krew-offline.tar.gz ~/.krew

# 在目标环境解压并配置 PATH
export PATH="$HOME/.krew/bin:$PATH"
```
### 插件权限最小化

- 只读插件可配置给一线值班。
- 写操作插件需经过审批与审计。
- trace、sniff 等高权限插件仅授权给安全团队。

## 常用插件组合

| 角色 | 推荐插件组合 |
|:---|:---|
| 一线值班 | ctx, ns, ktop, stern, cost |
| SRE 排障 | 上述 + trace, sniff, neat |
| 安全团队 | trace, sniff, gadget, cost |
| 平台工程师 | krew, ktop, stern, cost, trace |

### kubectl 插件排障

如果插件命令失败，可检查以下项：

1. 插件是否正确安装：`kubectl krew list`
2. kubeconfig 是否有足够权限
3. 插件版本是否与 K8s 版本兼容
4. 是否缺少依赖（如 trace 需要节点上有 bcc 工具）

## 插件组合实战：Pod 重启排查

以排查 Pod 反复重启为例，演示插件组合使用：

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 1. 切换上下文与命名空间
kubectl ctx prod
kubectl ns production

# 2. 查看 Pod 状态与资源使用
kubectl ktop

# 3. 查看多副本日志
kubectl stern -l app=order-service | grep ERROR

# 4. 使用 neat 查看精简配置
kubectl neat get pod order-service-xxx -o yaml

# 5. 如需内核级跟踪，使用 kubectl-trace
kubectl trace run node-1 -e 'kprobe:do_exit { printf("pid=%d comm=%s
", pid, comm); }'
```
### 插件维护建议

- 每季度 review 一次插件版本，删除不再使用的插件。
- 将插件安装与更新纳入运维镜像构建流程。
- 对高权限插件实施使用审批与审计。

## 插件安全与审计

在生产环境使用 kubectl 插件时，需关注权限与审计：

1. 只读插件可广泛使用，写操作插件需审批。
2. 高权限插件（如 sniff、trace）使用前应获得授权。
3. 记录插件使用日志，便于事后审计。
4. 定期审查插件来源，避免使用未经验证的第三方插件。

### 推荐插件安装清单

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
kubectl krew install ctx ns ktop stern cost neat trace sniff gadget
```
## 插件学习资源

| 插件 | 官方文档 |
|:---|:---|
| Krew | https://krew.sigs.k8s.io/ |
| ktop | https://github.com/yaacine/ktop |
| stern | https://github.com/stern/stern |
| kubectl-trace | https://github.com/iovisor/kubectl-trace |
| ctx / ns | https://github.com/ahmetb/kubectx |
| kubectl-cost | https://github.com/kubecost/kubectl-cost |

### 插件使用建议

1. 优先掌握 ctx、ns、ktop、stern 四个插件，可覆盖 80% 日常排障场景。
2. trace、sniff 等内核级插件应在测试环境熟悉后再上生产。
3. 将常用插件命令整理为团队 wiki 或脚本，降低使用门槛。
4. 定期检查插件更新，及时获取安全修复与新功能。

## Related

- [[故障诊断/工具/README.md|Domain-12 故障排查工具套件使用说明]]
- [[故障诊断/核心排障/00-open-source-projects-index-from-domain-12.md|故障排查开源项目索引]]

## See Also

- [[故障诊断/工具/02-network-diagnostic-tools.md|网络诊断工具]]
- [[故障诊断/工具/03-ebpf-diagnostic-tools.md|eBPF 诊断工具]]


<!-- risk-assessed -->
