---
title: Pod CrashLoopBackOff 远程顾问对话脚本
category: dialogue
tags: [dialogue, remote-advisor, pod-crashloop]
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
summary: "Pod反复重启问题的远程顾问对话脚本，覆盖OOMKilled、启动失败、探针配置排查。"
relationships:
  - target: "[[skills/skill-k8s-node-notready-SKILL.md]]"
    type: uses
  - target: "[[entities/cilium.md]]"
    type: uses
  - target: "[[entities/deployment.md]]"
    type: uses
---

# Pod CrashLoopBackOff 远程顾问对话脚本

> **角色设定**：你是部署在客户环境之外的远程顾问，无法直接连接集群。你只能通过对话指导现场工程师执行操作。
> **对话目标**：在 30 分钟内定位 Pod CrashLoopBackOff 的根因并给出修复方案。

---

## 对话入口

**顾问**：你好，我是远程 SRE 顾问。收到告警，你这边有 Pod 处于 CrashLoopBackOff 状态，对吗？请按以下步骤配合我排查。首先确认三个基础信息：

1. 这个 Pod 所在的 **命名空间** 和 **Pod 名称** 是什么？
2. 这是 **单个 Pod** 出问题，还是 **多个 Pod** 同时出问题？
3. 最近 **1 小时内** 是否有过部署更新、配置变更或扩缩容操作？

请尽可能准确地回答，不确定的就说"不确定"。

---

## Round 1：快速状态确认

**顾问**：请执行以下命令，获取 Pod 的基本状态信息：

```bash
kubectl get pod <pod-name> -n <namespace> -o wide
```

> **如果无法执行**：请告诉我你无法执行 kubectl 命令的原因（权限不足？没有 kubeconfig？），我会提供替代方案。如果你能登录到 Dashboard 或运维平台，请截图或复制 Pod 状态页面的文本信息给我。

### 分支 1-A：Pod 状态为 CrashLoopBackOff，Restart Count > 0

**顾问**：收到，Pod 确实在反复重启。请继续执行以下命令查看退出原因：

```bash
kubectl describe pod <pod-name> -n <namespace> | grep -A 5 "Last State"
```

> **如果无法执行 `kubectl describe`**：请尝试 `kubectl get pod <pod-name> -n <namespace> -o yaml`，然后把 `containerStatuses` 部分的内容复制给我。如果连 `-o yaml` 也无法执行，请登录 Dashboard 查看 Pod 详情页的"容器状态"部分。

**工程师回复选项**：
- **A1**：显示 `Reason: Error`，`Exit Code: 1`
- **A2**：显示 `Reason: OOMKilled`，`Exit Code: 137` 或 `143`
- **A3**：显示 `Reason: ContainerCannotRun` 或其他不常见原因

### 分支 1-B：Pod 状态为 Pending 或 ContainerCreating（非 CrashLoopBackOff）

**顾问**：当前 Pod 状态不是 CrashLoopBackOff，而是其他状态。这通常意味着问题出在调度或镜像拉取阶段。请执行：

```bash
kubectl describe pod <pod-name> -n <namespace> | tail -n 30
```

> **如果无法执行**：请查看 Pod 的 Events 信息（Dashboard 中通常在 Pod 详情页下方），把最后 10 条 Event 的文本发给我。

**工程师回复选项**：
- **B1**：Events 中有 `Failed to pull image` 或 `ImagePullBackOff`
- **B2**：Events 中有 `FailedScheduling` 或资源不足提示
- **B3**：Events 中有卷挂载失败（`MountVolume.SetUp failed`）

### 分支 1-C：多个 Pod 同时 CrashLoopBackOff

**顾问**：多个 Pod 同时问题，这提示可能是集群级或应用级的问题，而非单个 Pod 的偶发问题。请先确认范围：

```bash
kubectl get pods -n <namespace> -o wide | grep -E "CrashLoopBackOff|Error|OOMKilled"
```

> **如果无法执行**：请手动统计一下：有多少个 Pod 受影响？它们是否属于同一个 [[entities/deployment.md|Deployment]]/StatefulSet/DaemonSet？是否分布在不同的节点上？

**工程师回复选项**：
- **C1**：所有受影响 Pod 属于同一个 Deployment，分布在不同节点
- **C2**：受影响 Pod 属于不同的应用/Deployment
- **C3**：受影响 Pod 集中在同一个节点上

---

## Round 2：根因定位

### 2-A 分支：Exit Code 1（应用错误）

**顾问**：退出码 1 表示容器内的应用程序主动退出。请执行以下命令查看应用日志：

```bash
kubectl logs <pod-name> -n <namespace> --previous --tail=100
```

> **如果无法执行 `--previous`**：请尝试 `kubectl logs <pod-name> -n <namespace> --tail=100`。注意：如果 Pod 当前正在运行但即将崩溃，这个命令看到的是当前容器的日志，不是崩溃前的日志。如果日志已经输出到文件系统，请尝试进入节点查看 `/var/log/containers/` 目录下的日志文件。
> 
> **如果无法执行 `kubectl logs`**：请检查集群是否配置了日志采集系统（如 Loki、EFK、阿里云 SLS 等），尝试通过日志平台查询该 Pod 的日志。

**工程师回复选项**：
- **A1-1**：日志显示数据库/Redis/Kafka 连接失败（如 `Connection refused`、`Timeout`）
- **A1-2**：日志显示配置文件解析错误（如 `config error`、`yaml unmarshal error`）
- **A1-3**：日志显示端口冲突或权限错误（如 `permission denied`、`address already in use`）

### 2-B 分支：OOMKilled（Exit Code 137/143）

**顾问**：OOMKilled 表示容器使用的内存超过了设定的 limit。请执行以下命令确认内存限制和实际使用：

```bash
kubectl describe pod <pod-name> -n <namespace> | grep -A 2 "Limits"
kubectl top pod <pod-name> -n <namespace>
```

> **如果无法执行 `kubectl top`**：说明集群可能没有安装 metrics-server。请尝试 `kubectl describe node <node-name>`，查看该节点上 Allocated resources 的内存分配情况，并估算该 Pod 的内存 limit 是否合理。
> 
> **如果 describe node 也无法执行**：请告诉我节点的规格（CPU/内存总量），以及该 Pod 的 YAML 中 resources.limits.memory 设置的值。

**工程师回复选项**：
- **A2-1**：内存 limit 明显偏低（如 128Mi 但应用需要 1Gi）
- **A2-2**：内存 limit 看起来合理，但节点整体内存使用率很高（>90%）
- **A2-3**：内存 limit 合理，节点内存充足，但 Pod 内存使用持续增长（疑似内存泄漏）

### 2-C 分支：ImagePullBackOff / 镜像拉取失败

**顾问**：镜像拉取失败通常与镜像地址、凭证或网络有关。请执行：

```bash
kubectl describe pod <pod-name> -n <namespace> | grep -i "Failed to pull image\|pulling\|Back-off"
```

> **如果无法执行**：请告诉我 Deployment/StatefulSet 中 `image` 字段的完整值（包含 registry 地址、tag）。同时确认：你们使用的镜像仓库是公网还是私网？是否需要 imagePullSecret？

**工程师回复选项**：
- **B1-1**：镜像 tag 不存在或拼写错误（`manifest unknown`）
- **B1-2**：镜像仓库认证失败（`unauthorized` 或 `no basic auth credentials`）
- **B1-3**：网络超时，无法连接到镜像仓库

### 2-C-ACK 分支：阿里云ACR镜像拉取失败（ACK特有场景）

**顾问**：如果使用的是阿里云容器镜像服务（ACR），ACK 集群有一些特有诊断维度。请执行：

```bash
# 检查 ACR 免密插件状态（ACK默认安装 acr-credential-helper）
kubectl get pods -n kube-system | grep acr

# 检查 imagePullSecret 是否由 ack-acr-acceleration 自动生成
kubectl get secret -n <namespace> | grep acr

# 查看 Pod 事件中的 ACR 特有错误
kubectl describe pod <pod-name> -n <namespace> | grep -i "acr\|aliyun\|registry"
```

> **如果无法执行 kubectl**：请通过以下方式排查：
> 1. 登录 **阿里云控制台 > 容器镜像服务 ACR**，确认镜像仓库和 tag 是否存在
> 2. 在 ACK 控制台 **集群信息 > 集群资源** 中确认 **acr-credential-helper** 组件是否正常运行
> 3. 如果是 **ACR企业版**，请确认实例的 **访问控制** 是否允许该集群 VPC 访问

**阿里云ACR特有排查**：

```bash
# 验证节点到 ACR 的网络连通性（在节点上执行）
ssh <node-ip> "curl -I https://registry.<region>.aliyuncs.com/v2/"

# 检查 ACR 免密配置是否生效（查看节点上的 credential helper）
ssh <node-ip> "cat /root/.docker/config.json | grep aliyuncs"

# 检查是否使用了 ACR 镜像加速（ACK Pro集群支持）
kubectl get configmap -n kube-system ack-acr-acceleration -o yaml 2>/dev/null || echo "未启用镜像加速"
```

> **如果无法 SSH**：请登录阿里云控制台，进入 **ACK 控制台 > 集群 > 运维管理 > 节点诊断**，对异常节点执行 **镜像拉取诊断**。

**阿里云ACR常见根因与修复**：

| ACR特有场景 | 诊断命令/方法 | 修复方案 |
|:---|:---|:---|
| ACR免密插件异常 | `kubectl logs -n kube-system -l app=acr-credential-helper` | 重启 acr-credential-helper Pod 或重新安装组件 |
| ACR企业版实例无权限 | ACR控制台检查实例授权 / RAM策略 | 在 **RAM > 角色** 中为集群 Worker RAM 角色添加 `AliyunContainerRegistryReadOnlyAccess` |
| 跨地域拉取镜像网络不通 | 节点上 `curl -v registry.cn-beijing.aliyuncs.com` | 使用 ACR 镜像加速器 / 配置专线 / 切换为同地域镜像 |
| 镜像同步延迟（海外镜像） | ACR控制台查看镜像同步任务状态 | 等待同步完成，或使用 ACR 镜像加速功能 |
| 专有云中 ACR 实例域名解析失败 | 检查专有云 DNS / PrivateZone 配置 | 配置正确的 ACR 内网域名解析 |
| ACR个人版/企业版实例被锁定 | ACR控制台查看实例状态 | 续费解锁实例，或切换至备用镜像仓库 |

**顾问**：如果确认是 ACR 问题，请告诉我：
1. 使用的是 **ACR个人版** 还是 **企业版**？
2. 镜像地址属于 **默认实例** 还是 **自定义实例**？
3. 如果是专有云环境，ACR 是 **阿里云公有云ACR** 还是 **专有云ACR（Apsara Stack）**？

> **远程顾问无法直连时的替代方案**：请工程师在阿里云控制台执行以下操作：
> 1. **ACK 控制台 > 集群 > 基本配置** 中查看 **容器镜像服务** 是否已关联
> 2. **ACR 控制台 > 镜像仓库** 中确认对应镜像 tag 存在且未过期
> 3. **云监控 > 事件中心** 中查看是否有 ACR 服务异常事件

### 2-D 分支：FailedScheduling / 资源不足

**顾问**：Pod 无法调度。请执行以下命令获取调度失败的详细原因：

```bash
kubectl describe pod <pod-name> -n <namespace> | grep -A 10 "Events"
```

> **如果无法执行**：请告诉我当前集群的节点数量、节点规格，以及该 Pod 的 resources.requests 设置。如果使用了节点亲和性或污点，也请一并说明。

**工程师回复选项**：
- **B2-1**：CPU/内存请求总量超过节点可分配资源
- **B2-2**：存在 Pod 亲和性/反亲和性规则导致无法调度
- **B2-3**：节点有污点（Taint），Pod 没有对应的容忍度（Toleration）

### 2-E 分支：多个 Pod 同时问题（应用级）

**顾问**：多个 Pod 同时 CrashLoopBackOff，且属于同一个应用。请先获取该应用的配置：

```bash
kubectl get deployment <deployment-name> -n <namespace> -o yaml | grep -A 20 "containers:"
```

> **如果无法执行**：请告诉我该 Deployment 最近是否有过版本更新？如果有，上一个正常运行的镜像版本号是什么？

**工程师回复选项**：
- **C1-1**：最近刚刚做过版本升级，回滚到旧版本后正常
- **C1-2**：没有版本变更，但 ConfigMap/Secret 最近有修改
- **C1-3**：应用依赖的外部服务（数据库/缓存/第三方 API）出现问题

### 2-F 分支：多个 Pod 集中在一个节点上（节点级）

**顾问**：Pod 集中在同一个节点问题，这提示可能是节点问题。请执行：

```bash
kubectl describe node <node-name>
kubectl get pods --all-namespaces --field-selector spec.nodeName=<node-name> | grep -E "CrashLoopBackOff|Error|OOMKilled"
```

> **如果无法执行 `kubectl describe node`**：请尝试登录该节点执行 `systemctl status kubelet` 和 `docker ps`（或 `crictl ps`），把输出发给我。如果无法登录节点，请检查监控系统中该节点的 CPU、内存、磁盘、网络指标是否有异常。

**工程师回复选项**：
- **C3-1**：节点状态为 `NotReady` 或有 `DiskPressure`/`MemoryPressure` 污点
- **C3-2**：节点状态正常，但容器运行时异常（docker/containerd 服务异常）
- **C3-3**：节点状态正常，容器运行时正常，只有特定 Pod 异常

---

## Round 3：修复方案与执行

### 3-A 分支：应用配置错误（ConfigMap/Secret/环境变量）

**顾问**：根因已定位到配置错误。请按以下步骤修复：

**步骤 1**：确认当前配置内容
```bash
kubectl get configmap <configmap-name> -n <namespace> -o yaml
```

> **如果无法执行**：如果你知道配置错误在哪里，可以直接告诉我需要修改的值，我会帮你生成正确的配置内容。

**步骤 2**：修复配置后重新加载

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl apply -f <fixed-config.yaml> -n <namespace>
```

> **如果无法执行 `kubectl apply`**：请使用 `kubectl edit configmap <configmap-name> -n <namespace>` 手动修改。如果 edit 也无法使用，请告诉我你平时如何修改配置（GitOps？Dashboard？），按你的流程操作即可，修改后告诉我已更新。

**步骤 3**：重启 Pod 使配置生效

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout restart deployment/<deployment-name> -n <namespace>
```

> **如果无法执行 rollout restart**：请使用 `kubectl delete pod <pod-name> -n <namespace>` 删除 Pod，让控制器自动重新创建。注意：对于 StatefulSet，建议逐个删除 Pod，等待前一个恢复后再删下一个。

### 3-B 分支：内存不足（OOMKilled）

**顾问**：根因是内存限制过低。请按以下步骤调整：

**步骤 1**：编辑 Deployment 增加内存 limit

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl edit deployment <deployment-name> -n <namespace>
```

> **如果无法执行 `kubectl edit`**：请准备一个新的 YAML 文件，将 `resources.limits.memory` 增加到合适的值（建议当前值的 1.5-2 倍，但不要超过节点可分配内存的 50%），然后执行 `kubectl apply -f <new-deployment.yaml>`。如果你不确定应该设置多少，请告诉我当前 limit 值和节点总内存，我来帮你计算。

**步骤 2**：观察滚动更新是否成功
```bash
kubectl rollout status deployment/<deployment-name> -n <namespace>
```

> **如果无法执行 `rollout status`**：请执行 `kubectl get pods -n <namespace> -w` 观察新 Pod 是否成功进入 Running 状态。如果新 Pod 也 OOMKilled，说明内存仍然不够，需要进一步增加。

**步骤 3**：如果节点整体内存不足
> 如果调整 limit 后仍然无法调度，说明节点内存已被占满。请考虑：1) 水平扩容节点（增加节点）；2) 清理非必要 Pod；3) 如果使用了 HPA，暂时降低副本数以释放内存。请告诉我你倾向哪种方案。

### 3-C 分支：镜像拉取失败

**顾问**：根因是镜像无法拉取。请按以下步骤排查：

**步骤 1**：确认镜像地址是否正确
```bash
# 在可以访问镜像仓库的机器上测试拉取
docker pull <full-image-url>
```

> **如果无法执行 `docker pull`**：请用 `crictl pull <full-image-url>` 测试。如果都没有权限执行，请手动在浏览器中访问镜像仓库的 Web UI，确认该 tag 是否存在。或者告诉我镜像地址，我帮你检查是否格式正确。

**步骤 2**：如果是认证问题，检查 imagePullSecret
```bash
kubectl get secret -n <namespace> | grep dockerconfigjson
```

> **如果无法执行**：请确认你的镜像仓库是否需要认证。如果需要，请检查 Pod 的 spec.imagePullSecrets 是否正确引用了 Secret。如果不知道 Secret 名称，请告诉我你们使用的镜像仓库类型（Harbor? Docker Hub? ECR? ACR?），我来提供对应的配置方式。

**步骤 3**：修复后重新创建 Pod

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl delete pod <pod-name> -n <namespace>
```

> **如果删除后新 Pod 仍然 ImagePullBackOff**：请检查新 Pod 的 `image` 字段是否已经更新为正确的地址。如果 Deployment 的 image 是旧的错误地址，需要先修改 Deployment：`kubectl set image deployment/<name> <container>=<correct-image> -n <namespace>`。

### 3-D 分支：应用依赖未就绪（数据库连接失败等）

**顾问**：根因是应用启动依赖的外部服务不可用。请按以下步骤处理：

**步骤 1**：检查依赖服务状态
```bash
# 如果依赖服务也在集群内
kubectl get svc -n <dependency-namespace> | grep <dependency-name>
kubectl get endpoints <dependency-svc> -n <dependency-namespace>
```

> **如果无法执行**：请确认依赖服务（数据库/Redis/Kafka 等）是否部署在同一个集群。如果是外部服务，请从应用节点尝试 `telnet <host> <port>` 或 `nc -vz <host> <port>` 测试连通性。如果连 nc 也没有，请尝试 `curl -v telnet://<host>:<port>`。

**步骤 2**：如果是网络策略或防火墙问题
```bash
kubectl get networkpolicy -n <namespace>
```

> **如果无法执行**：请确认你们是否使用了 [[entities/cilium.md|Cilium]] 等网络策略。如果使用了，请检查是否有规则阻止了 Pod 访问依赖服务。如果无法确认，可以暂时将网络策略全部删除测试（仅限测试环境）。

**步骤 3**：临时缓解方案
> 如果依赖服务确实不可用且短时间内无法恢复，建议：1) 如果应用支持启动重试，可以增加 `initialDelaySeconds` 和 `failureThreshold` 让 Pod 有更多时间等待依赖恢复；2) 如果业务可以降级，可以临时将该 Deployment 的副本数缩为 0，等依赖恢复后再扩容。请告诉我你的选择。

### 3-E 分支：版本回滚（新版本引入 Bug）

**顾问**：根因是新版本引入问题。建议立即回滚到上一个稳定版本。

**步骤 1**：查看历史版本
```bash
kubectl rollout history deployment/<deployment-name> -n <namespace>
```

> **如果无法执行**：请告诉我你记得的上一版本的镜像 tag，或者你们是否使用 GitOps（如 ArgoCD/Flux），可以通过 GitOps 回滚。

**步骤 2**：执行回滚

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

```bash
kubectl rollout undo deployment/<deployment-name> -n <namespace>
# 或者回滚到指定版本
kubectl rollout undo deployment/<deployment-name> -n <namespace> --to-revision=<revision-number>
```

> **如果无法执行 rollout undo**：请手动修改 Deployment 的 image 字段为上一个稳定版本的镜像地址，然后 `kubectl apply`。如果是 StatefulSet，回滚流程会更复杂，请先告诉我，我提供 StatefulSet 回滚的详细步骤。

**步骤 3**：确认回滚成功
```bash
kubectl get pods -n <namespace>
kubectl rollout status deployment/<deployment-name> -n <namespace>
```

> **回滚后如果仍然 CrashLoopBackOff**：说明问题可能不是版本引起的，请重新检查日志，看是否回滚过程中又触发了其他变更（如 ConfigMap 也被修改了）。

---

## Round 4：验证修复与升级决策

**顾问**：修复已执行。现在请验证修复是否生效。请执行以下检查：

### 检查清单

```bash
# 1. 确认 Pod 状态正常
kubectl get pod <pod-name> -n <namespace>
```

> **如果无法执行**：请通过 Dashboard 或其他运维平台确认 Pod 状态。

```bash
# 2. 确认容器 Ready
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].ready}'
```

> **如果无法执行 jsonpath**：请告诉我 `kubectl get pod` 输出中 READY 列的值（如 `1/1` 还是 `0/1`）。

```bash
# 3. 确认重启次数不再增加
kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[0].restartCount}'
```

> **如果无法执行**：请等待 2 分钟后再次查看 Pod 状态，确认 Restart Count 是否增加。

```bash
# 4. 查看最新日志无错误
kubectl logs <pod-name> -n <namespace> --tail=50
```

> **如果无法执行**：如果配置了日志平台，请通过日志平台查看最新 50 条日志。

---

### 升级决策点

**顾问**：根据修复结果，请告诉我当前状态，我将决定是否升级或结束：

#### 决策 4-A：修复成功，所有检查通过

**顾问**：很好，修复已验证成功。请继续保持观察 **5 分钟**，如果 Restart Count 不再增加，可以认为问题已解决。

> **后续建议**：
> 1. 如果是内存 limit 调整，建议后续配置 VPA 自动调整资源
> 2. 如果是配置错误，建议将配置纳入 Git 版本控制，避免手动修改
> 3. 如果是版本问题，建议在新版本修复 Bug 后重新验证再上线

**对话结束** ✅

#### 决策 4-B：修复后 Pod 仍然 CrashLoopBackOff

**顾问**：修复未生效，Pod 仍在反复重启。这说明根因可能比我初步判断的更复杂。接下来有以下选择：

> **升级路径选择**：
> 1. **继续深入排查** → 我需要你执行更详细的诊断命令，包括节点级别检查
> 2. **升级至存储专家** → 如果涉及数据库/消息队列等有状态服务，我将转接至 [[skills/skill-k8s-node-notready-SKILL.md|SKILL]]-STORE-001
> 3. **升级至节点专家** → 如果怀疑是节点/内核级别问题，我将转接至 SKILL-NODE-001
> 4. **升级至工作负载专家** → 如果涉及复杂的 Deployment/StatefulSet 编排问题，我将转接至 SKILL-WORK-001

请告诉我你的选择，或者告诉我当前 Pod 的最新状态和日志，我继续帮你排查。

#### 决策 4-C：修复后 Pod 状态反复波动（时好时坏）

**顾问**：Pod 状态不稳定，时好时坏。这通常暗示以下问题之一：

> 1. **资源竞争**：节点资源不足，Pod 被频繁驱逐或 OOMKill → 需要检查节点资源并考虑扩容
> 2. **依赖服务不稳定**：下游服务间歇性不可用 → 需要检查依赖服务健康状态
> 3. **探针配置过于敏感**：存活探针阈值设置过严 → 需要放宽探针参数

请告诉我：Pod 状态波动的时间间隔大概是多少？是否有规律？这有助于判断根因。

#### 决策 4-D：修复后出现新的错误

**顾问**：修复后出现新的错误，这可能是修复操作带来的副作用。请执行：

```bash
kubectl describe pod <pod-name> -n <namespace> | grep -i "warning\|error\|fail"
kubectl logs <pod-name> -n <namespace> --tail=100
```

> **如果无法执行**：请把你能看到的最新错误信息发给我。

**顾问判断**：
- 如果是资源 limit 调高后导致节点无法调度 → 需要回调 limit 并考虑节点扩容
- 如果是配置修改后格式错误 → 需要检查 YAML 语法并修复
- 如果是回滚后版本不兼容 → 需要检查依赖服务版本匹配性

---

## 附录：常用命令速查

> 以下命令供现场工程师快速复制使用，顾问可根据实际情况选择性提供。

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
# 快速查看 Pod 状态
kubectl get pods -n <namespace> -o wide

# 查看 Pod 详情和事件
kubectl describe pod <pod-name> -n <namespace>

# 查看当前容器日志
kubectl logs <pod-name> -n <namespace> --tail=100

# 查看上一个崩溃容器的日志
kubectl logs <pod-name> -n <namespace> --previous --tail=100

# 查看 Pod 资源使用
kubectl top pod <pod-name> -n <namespace>

# 查看节点资源
kubectl top nodes

# 查看 Deployment 历史
kubectl rollout history deployment/<name> -n <namespace>

# 查看 ReplicaSet
kubectl get rs -n <namespace> | grep <deployment-name>

# 进入运行中的容器（调试用）
kubectl exec -it <pod-name> -n <namespace> -- /bin/sh
# 如果容器没有 /bin/sh，尝试：
kubectl exec -it <pod-name> -n <namespace> -- /bin/bash
```

---

## 对话结束语

**顾问**：感谢你的配合。如果问题已解决，请记录本次问题的根因和修复方案，便于后续复盘。如果问题仍未解决，请告诉我当前状态，我们继续排查。

> **重要提醒**：本对话脚本仅覆盖常见 CrashLoopBackOff 场景。对于涉及数据一致性、有状态服务、集群级问题等复杂情况，请随时要求升级至更专业的 Skill 处理。

## 相关案例

- [[concepts/case-studies/2026-03-15-oomkilled-java-restart.md|2026-03-15-oomkilled-java-restart]]
- [[concepts/case-studies/2026-08-10-容器内存限制过严导致java应用频繁oom.md|2026-08-10-容器内存限制过严导致java应用频繁oom]]
## Related

- [[scripts/video-scripts/pod-crashloop.md|Pod CrashLoopBackOff & OOMKilled 诊断与修复 — 数字人播报脚本 (video-scripts)]]
- [[domain-17-system-foundation/topic-dictionary/configuration/secrets.md|Secrets]]
