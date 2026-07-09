---
title: 镜像拉取问题 — 远程顾问对话脚本
summary: 镜像拉取问题的远程顾问对话脚本，覆盖镜像不存在、认证失败、仓库不可达排查。
category: troubleshooting
tags:
- workloads
- remote-consultant
tier: supporting
created: '2026-05-23'
updated: '2026-05-23'
last_updated: 2026-05-23
dialogue_id: DIALOGUE-SKILL-IMG-001
skill_id: SKILL-IMG-001
version: 1.0.0
role: remote-consultant
language: zh
relationships:
- target: '[[skills/skill-k8s-node-notready-SKILL.md]]'
  type: uses
- target: '[[entities/deployment.md]]'
  type: uses
- target: '[[entities/kubelet.md]]'
  type: uses
- target: '[[系统基础/topic-dictionary/fundamentals/namespaces.md]]'
  type: uses
- target: '[[系统基础/topic-dictionary/networking/service.md]]'
  type: uses
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。



# K8s Image Pull Failure — 远程顾问对话脚本

> 顾问身份：部署在客户专有云之外的远程 SRE 专家，**无法直接连接集群**，只能通过对话指导现场工程师执行操作。

---

## 对话入口

### 入口 A：工程师明确报告镜像拉取失败

**工程师**：「Pod 状态是 ImagePullBackOff」/「镜像拉取失败了」/「ErrImagePull」

**顾问回应**：
> 收到，镜像拉取失败会直接导致 Pod 无法启动。作为远程顾问，我无法直连你的集群，请你配合执行检查命令。先回答三个问题（30 秒内）：
> 1. **影响范围**：多少 Pod / [[系统基础/topic-dictionary/fundamentals/namespaces.md|命名空间]] / 节点受影响？
> 2. **紧急程度**：业务是否已中断？是否有用户投诉？
> 3. **发生时间**：问题是突然发生还是逐渐恶化？最近是否有发布变更？

---

### 入口 B：工程师报告 Pod 无法启动（疑似镜像问题）

**工程师**：「Pod 一直在 Pending」/「[[entities/deployment.md|Deployment]] 滚动更新卡住了」/「新发布的版本起不来」

**顾问回应**：
> Pod 无法启动有多种可能，镜像拉取失败是最常见的根因之一。请先执行：
> ```bash
> kubectl get pods --all-namespaces | grep -E "ImagePullBackOff|ErrImagePull|Pending"
> ```
> **如果无法执行** → `kubectl get pods -n <namespace> | grep -E "ImagePullBackOff|ErrImagePull"`
> **如果连 namespace 权限也没有** → 请把 Dashboard 中看到的 Pod 状态截图发给我
> 请把输出贴给我。

---

### 入口 C：工程师报告镜像仓库或凭证相关告警

**工程师**：「镜像仓库认证失败」/「pull access denied」/「manifest unknown」/「registry 超时」

**顾问回应**：
> 这类告警通常指向私有仓库认证、镜像标签错误或仓库网络问题。请先执行：
> ```bash
> kubectl get events --all-namespaces --field-selector reason=Failed | grep -i "pull|image|registry" | tail -20
> ```
> **如果无法执行** → `kubectl get events -n <namespace> | grep -i "pull|image|registry" | tail -20`
> **如果 events 也查不了** → `kubectl describe pod <pod-name> -n <namespace> | grep -A 20 Events`
> 请把相关事件贴给我。

---

### 入口 D：工程师报告 Init 容器镜像拉取失败

**工程师**：「Init 容器起不来」/「Init:ImagePullBackOff」/「Pod 卡在 Init 阶段」

**顾问回应**：
> Init 容器镜像拉取失败会导致整个 Pod 初始化阻塞。请先确认 Init 容器状态：
> ```bash
> kubectl get pods --all-namespaces | grep "Init:"
> ```
> **如果无法执行** → `kubectl get pods -n <namespace> | grep "Init:"`
> **如果没有 grep** → `kubectl get pods -n <namespace>`，手动找出状态包含 Init 的 Pod
> 请告诉我受影响的 Pod 名称和当前状态。

---

## Round 1：快速定位问题层级

> 目标：判断问题发生在 **镜像配置 → 仓库认证 → 网络连通 → 节点磁盘** 的哪一层。

---

### Round 1 — 分支 A：Pod 状态为 ImagePullBackOff / ErrImagePull

**工程师反馈**：Pod 状态显示 ImagePullBackOff 或 ErrImagePull。

**顾问指令**：
> 请获取该 Pod 的详细事件和镜像信息。
> 1. 查看 Pod 事件：`kubectl describe pod <pod-name> -n <namespace> | grep -A 30 Events`
> **如果无法执行** → `kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name>`
> 2. 获取镜像地址：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].image}'`
> **如果无法执行 jsonpath** → `kubectl get pod <pod-name> -n <namespace> -o yaml | grep "image:"`
> 请把 Events 中的拉取错误信息和完整镜像地址贴给我。

**分支决策**：
- **A1**：Events 显示 `manifest unknown` / `not found` → Round 2 — 分支 A（镜像标签排查）
- **A2**：Events 显示 `unauthorized` / `pull access denied` → Round 2 — 分支 B（仓库认证排查）
- **A3**：Events 显示 `timeout` / `connection refused` → Round 2 — 分支 C（网络连通排查）
- **A4**：Events 显示 `no space left on device` / `disk full` → Round 2 — 分支 D（节点磁盘排查）

---

### Round 1 — 分支 B：多个 Pod 同时镜像拉取失败

**工程师反馈**：多个 Pod 或多个节点同时出现镜像拉取失败。

**顾问指令**：
> 多 Pod 同时失败通常指向集群级问题。先确认分布模式。
> 1. 统计节点分布：`kubectl get pods --all-namespaces -o wide | grep -E "ImagePullBackOff|ErrImagePull" | awk '{print $8}' | sort | uniq -c`
> **如果无法执行 awk** → 手动统计节点分布
> **如果无全集群权限** → `kubectl get pods -n <namespace> -o wide | grep -E "ImagePullBackOff|ErrImagePull"`
> 2. 检查镜像地址是否相同：`kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{" "}{.metadata.name}{" "}{.spec.containers[*].image}{"\n"}{end}' | grep -E "ImagePullBackOff|ErrImagePull"`
> **如果无法执行** → 请手动告诉我：失败的是同一个镜像还是不同镜像？

**分支决策**：
- **B1**：所有失败 Pod 使用同一个镜像/仓库 → Round 2 — 分支 E（仓库级问题排查）
- **B2**：失败 Pod 分布在特定节点上 → Round 2 — 分支 F（节点级问题排查）
- **B3**：失败 Pod 镜像和仓库各不相同 → Round 2 — 分支 G（全局配置排查）

---

### Round 1 — 分支 C：Init 容器镜像拉取失败

**工程师反馈**：Pod 状态为 Init:ImagePullBackOff 或 Init:ErrImagePull。

**顾问指令**：
> Init 容器镜像拉取失败的排查逻辑与应用容器相同，但需要检查 Init 容器的镜像配置。
> 1. 检查 Init 容器镜像：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.initContainers[*].image}'`
> **如果无法执行 jsonpath** → `kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 5 "initContainers"`
> 2. 查看 Init 容器事件：`kubectl describe pod <pod-name> -n <namespace> | grep -A 20 "Init Containers"`
> **如果无法执行** → `kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name> | grep -i init`
> 请把 Init 容器的镜像地址和相关错误信息贴给我。

**分支决策**：
- **C1**：Init 容器镜像与应用容器镜像来自同一仓库 → 按 Round 1 — 分支 A 的决策继续
- **C2**：Init 容器镜像是公共镜像（如 busybox、alpine）→ Round 2 — 分支 H（公共镜像排查）
- **C3**：Init 容器镜像是自定义工具镜像 → Round 2 — 分支 A（镜像标签排查）

---

## Round 2：分层深入诊断

> 目标：根据 Round 1 确定的层级，执行针对性的深度检查。

---

### Round 2 — 分支 A：镜像标签排查

**顾问指令**：
> 镜像标签不存在或拼写错误是最常见的根因。请按以下步骤排查。
> 1. 确认完整镜像地址：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.containers[*].image}'`
> **如果无法执行** → `kubectl get pod <pod-name> -n <namespace> -o yaml | grep "image:"`
> 2. 检查镜像标签在仓库中是否存在。请在能访问镜像仓库的机器上执行：`docker pull <full-image-address>`
> **如果无法执行 docker** → 直接访问镜像仓库 Web UI 确认
> 3. 检查是否使用了 `latest` 标签：`kubectl get pod <pod-name> -n <namespace> -o yaml | grep "image:" | grep ":latest"`
> **如果无法执行** → 请手动告诉我镜像地址中是否包含 `:latest`

**分支决策**：
- **A1**：标签确实不存在或拼写错误 → Round 3 — 分支 I（修正镜像标签）
- **A2**：标签存在但 `docker pull` 失败 → 返回 Round 1 — 分支 A，重新确认错误信息
- **A3**：使用了 `:latest` 标签，仓库中存在但 Pod 仍失败 → Round 3 — 分支 J（缓存/平台兼容排查）

---

### Round 2 — 分支 B：仓库认证排查

**顾问指令**：
> 认证失败通常由 imagePullSecret 缺失、过期或配置错误引起。
> 1. 检查 Pod 是否引用了 imagePullSecret：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.imagePullSecrets[*].name}'`
> **如果无法执行 jsonpath** → `kubectl get pod <pod-name> -n <namespace> -o yaml | grep -A 5 imagePullSecrets`
> 2. 检查 Secret 是否存在：`kubectl get secret -n <namespace> | grep dockerconfigjson`
> **如果没有 dockerconfigjson** → `kubectl get secret -n <namespace>`，找名称与 imagePullSecret 对应的 Secret
> 3. 验证 Secret 内容：`kubectl get secret <secret-name> -n <namespace> -o jsonpath='{.data.\.dockerconfigjson}' | base64 -d`
> **如果无法 base64 解码** → `kubectl get secret <secret-name> -n <namespace> -o yaml`
> **如果无权限查看 Secret** → 请联系集群管理员确认 Secret 是否过期

**分支决策**：
- **B1**：Pod 没有 imagePullSecret，但拉取私有镜像 → Round 3 — 分支 K（创建/绑定 imagePullSecret）
- **B2**：Secret 存在但内容过期或仓库地址错误 → Round 3 — 分支 K（更新 imagePullSecret）
- **B3**：Secret 存在且内容正确，但认证仍失败 → Round 3 — 分支 L（仓库权限排查）

---

### Round 2 — 分支 C：网络连通排查

**顾问指令**：
> 网络超时说明节点无法连接到镜像仓库。先确认网络路径。
> 1. 找到 Pod 所在节点：`kubectl get pod <pod-name> -n <namespace> -o wide`
> **如果无法执行** → `kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeName}'`
> 2. 在目标节点上测试网络连通：`ssh <node> "curl -I -m 10 https://<registry-host>"`
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=nicolaka/netshoot -- curl -I -m 10 https://<registry-host>`
> 3. 检查 DNS 解析：`kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup <registry-host>`
> **如果无法创建临时 Pod** → `kubectl exec -it <pod-name> -n <namespace> -- nslookup <registry-host>`

**分支决策**：
- **C1**：DNS 解析失败 → 升级至 [[skills/skill-k8s-node-notready-SKILL.md|SKILL]]-NET-001（DNS 问题诊断）
- **C2**：DNS 正常但 HTTP 超时，多节点受影响 → Round 3 — 分支 M（镜像仓库/代理排查）
- **C3**：DNS 正常但 HTTP 超时，仅单节点受影响 → Round 3 — 分支 N（单节点网络排查）

---

### Round 2 — 分支 D：节点磁盘排查

**顾问指令**：
> 节点磁盘空间不足会导致镜像拉取失败，因为镜像层需要本地存储空间。
> 1. 找到 Pod 所在节点：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.spec.nodeName}'`
> 2. 检查节点磁盘使用情况：`kubectl describe node <node-name> | grep -A 10 "Allocated resources"`
> **如果无法执行** → `kubectl get node <node-name> -o yaml | grep -A 20 "ephemeral-storage"`
> 3. 直接检查节点磁盘（如可 SSH）：`ssh <node-ip> "df -h /var/lib/containerd"`
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=busybox -- df -h`
> 4. 检查 Pod 事件中的磁盘相关错误：`kubectl describe pod <pod-name> -n <namespace> | grep -i "space|disk|full"`

**分支决策**：
- **D1**：节点磁盘使用率 >85%，有空间压力 → Round 3 — 分支 O（节点磁盘清理）
- **D2**：节点磁盘充足，但事件显示空间不足 → 检查 inode 使用率，Round 3 — 分支 P（inode 清理）
- **D3**：磁盘无异常，但容器运行时报告错误 → Round 3 — 分支 Q（容器运行时修复）

---

### Round 2 — 分支 E：仓库级问题排查

**顾问指令**：
> 多个 Pod 使用同一仓库同时失败，强烈提示仓库本身有问题。
> 1. 检查仓库健康状态：`curl -I -m 10 https://<registry-host>/v2/`
> **如果无法 curl** → 请确认镜像仓库是否有维护公告
> 2. 检查仓库认证服务：`curl -u <username>:<password> https://<registry-host>/v2/_catalog`
> **如果无法提供凭据** → 请确认仓库管理员是否修改了认证策略
> 3. 检查镜像仓库的 Rate Limit：`curl -v https://<registry-host>/v2/<repo>/manifests/<tag> 2>&1 | grep -i "rate|limit|429"`
> **如果无法执行** → 查看 Pod 事件中是否有 `TOOMANYREQUESTS` 或 `429`

**分支决策**：
- **E1**：仓库返回 429 / Rate Limit → Round 3 — 分支 R（限流处理）
- **E2**：仓库返回 503 / 502 / 完全不可达 → 升级决策点（联系镜像仓库管理团队）
- **E3**：仓库正常，但集群内无法访问 → Round 2 — 分支 C（网络连通排查）

---

### Round 2 — 分支 F：节点级问题排查

**顾问指令**：
> 失败集中在特定节点上，说明该节点的容器运行时或网络配置有问题。
> 1. 检查节点状态：`kubectl get node <node-name> -o wide`
> `kubectl describe node <node-name> | grep -A 15 Conditions`
> **如果无法 describe** → `kubectl get node <node-name> -o yaml | grep -A 20 conditions`
> 2. 检查该节点的容器运行时：`ssh <node-ip> "systemctl status containerd"`
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=busybox -- pgrep -a containerd`
> 3. 检查节点上镜像缓存：`ssh <node-ip> "crictl images | grep <image-name>"`
> **如果无法 SSH** → 检查节点事件

**分支决策**：
- **F1**：节点状态 NotReady 或有 DiskPressure/MemoryPressure → Round 3 — 分支 S（节点恢复）
- **F2**：容器运行时服务异常 → Round 3 — 分支 Q（容器运行时修复）
- **F3**：节点状态正常，仅镜像拉取异常 → Round 3 — 分支 N（单节点网络排查）

---

## Round 3：精确修复与验证

> 目标：执行最终修复动作，验证镜像拉取恢复正常，决定是否升级。

---

### Round 3 — 分支 I：修正镜像标签

**顾问指令**：
> 镜像标签不存在或拼写错误，需要修正为正确的标签。
> 1. 确认正确的镜像标签：`skopeo list-tags docker://<registry>/<repository>`
> **如果没有 skopeo** → 请人工确认正确的镜像 tag 版本
> 2. 修改 Deployment/StatefulSet 的镜像标签：`kubectl set image deployment/<deploy-name> <container>=<correct-image>:<correct-tag> -n <namespace>`
> **如果无法 set image** → `kubectl patch deployment/<deploy-name> -n <namespace> --type merge -p '{"spec":{"template":{"spec":{"containers":[{"name":"<container>","image":"<correct-image>:<correct-tag>"}]}}}}'`
> **如果 patch 也失败** → `kubectl edit deployment/<deploy-name> -n <namespace>`
> **如果无交互式终端** → 请准备修正后的 YAML 文件，执行 `kubectl apply -f fixed-deployment.yaml`
> 3. 验证新 Pod 状态：`kubectl get pods -n <namespace> -w`
> **如果无法 watch** → 间隔 30 秒执行 `kubectl get pods -n <namespace>`

**分支决策**：
- **I1**：Pod 变为 Running，镜像拉取成功 → 修复完成
- **I2**：Pod 仍为 ImagePullBackOff → 确认镜像地址是否仍错误，或返回 Round 1
- **I3**：无法修改 Deployment（权限不足）→ 升级决策点

---

### Round 3 — 分支 J：缓存/平台兼容排查

**顾问指令**：
> 镜像标签存在但拉取失败，可能是缓存或平台架构不兼容。
> 1. 检查镜像平台架构：`skopeo inspect --raw docker://<full-image-address> | grep -i "architecture|platform"`
> **如果没有 skopeo** → 请确认镜像是否为多架构镜像
> 2. 检查节点架构：`kubectl get node <node-name> -o jsonpath='{.status.nodeInfo.architecture}'`
> **如果无法执行 jsonpath** → `kubectl describe node <node-name> | grep "Architecture:"`
> 3. 清除可能的镜像缓存问题（单节点）：`ssh <node-ip> "crictl rmi <full-image-address>"`
> **如果无法 SSH** → `kubectl delete pod <pod-name> -n <namespace>`，让调度器重新调度到新节点
> **如果 Pod 使用 hostPath 或本地存储** → 请先确认删除是否安全

**分支决策**：
- **J1**：镜像架构与节点不匹配 → 重新构建多架构镜像或指定正确架构节点
- **J2**：清除缓存后恢复 → 修复完成
- **J3**：缓存和架构都正常，仍失败 → 升级决策点

---

### Round 3 — 分支 K：创建/更新 imagePullSecret

**顾问指令**：
> 需要创建或更新镜像仓库凭证，并绑定到 Pod/ServiceAccount。
> 1. 创建 docker-registry Secret：`kubectl create secret docker-registry <secret-name> --docker-server=<registry-host> --docker-username=<username> --docker-password=<password> --docker-email=<email> -n <namespace>`
> **如果无法交互式输入密码** → 准备 Secret YAML 文件后 `kubectl apply -f secret.yaml`
> 2. 将 Secret 绑定到 ServiceAccount：`kubectl patch serviceaccount default -n <namespace> -p '{"imagePullSecrets":[{"name":"<secret-name>"}]}'`
> **如果默认 SA 不存在** → `kubectl get serviceaccount -n <namespace>`，找到正确的 SA 名称后 patch
> **如果无法 patch SA** → 在 Pod spec 中直接添加 `imagePullSecrets` 字段
> 3. 触发 Pod 重建：`kubectl rollout restart deployment/<deploy-name> -n <namespace>`
> **如果无法 rollout restart** → `kubectl delete pod -l app=<app-label> -n <namespace>`
> **如果是裸 Pod** → 修改 YAML 后重新 apply

**分支决策**：
- **K1**：Pod 重建后镜像拉取成功 → 修复完成
- **K2**：Pod 重建后仍认证失败 → Round 3 — 分支 L（仓库权限排查）
- **K3**：无法创建 Secret（权限不足）→ 升级决策点

---

### Round 3 — 分支 L：仓库权限/服务账号排查

**顾问指令**：
> 凭证配置正确但认证仍失败，可能是仓库侧权限或集群 ServiceAccount 问题。
> 1. 确认仓库侧用户权限：登录镜像仓库管理后台，确认该用户/ServiceAccount 对目标仓库有 pull 权限；确认仓库是否启用了 IP 白名单，集群出口 IP 是否在白名单中
> 2. 检查集群内 ServiceAccount 状态：`kubectl get serviceaccount -n <namespace>`
> `kubectl describe serviceaccount <sa-name> -n <namespace>`
> **如果无法 describe** → `kubectl get serviceaccount <sa-name> -n <namespace> -o yaml`
> 3. 测试凭据是否有效（在外部机器）：`docker login <registry-host> -u <username> -p <password>`
> `docker pull <full-image-address>`
> **如果无法 docker** → `skopeo inspect --creds <username>:<password> docker://<full-image-address>`
> **如果外部测试成功但集群失败** → 可能是集群网络或代理问题

**分支决策**：
- **L1**：仓库侧权限问题已修复 → 修复完成
- **L2**：外部测试成功但集群失败 → Round 2 — 分支 C（网络连通排查）
- **L3**：仓库管理员无法立即修复权限 → 升级决策点

---

### Round 3 — 分支 M：镜像仓库/代理排查

**顾问指令**：
> 多节点无法访问镜像仓库，可能存在网络代理、防火墙或 DNS 问题。
> 1. 检查集群是否配置了代理：`kubectl get configmap -n kube-system | grep -i proxy`
> **如果无法执行** → 检查节点环境：`ssh <node-ip> "env | grep -i proxy"`
> 2. 检查防火墙/安全组：确认集群节点到镜像仓库的出站 443 端口是否开放；确认是否有 NetworkPolicy 阻止出站流量
> 3. 检查集群 DNS 解析：`kubectl run dns-test --image=busybox:1.36 --rm -it --restart=Never -- nslookup <registry-host>`
> **如果无法创建临时 Pod** → `kubectl exec -it <coredns-pod> -n kube-system -- nslookup <registry-host>`

**分支决策**：
- **M1**：代理配置错误 → 修正代理配置（NO_PROXY 添加镜像仓库域名）
- **M2**：防火墙/安全组阻断 → 联系网络团队开放端口
- **M3**：DNS 解析失败 → 升级至 SKILL-NET-001（DNS 问题诊断）

---

### Round 3 — 分支 O：节点磁盘清理

**顾问指令**：
> 节点磁盘空间不足，需要清理以释放空间。
> 1. 查看镜像和容器占用：`ssh <node-ip> "crictl ps -a | wc -l && crictl images | wc -l"`
> **如果无法 SSH** → `kubectl debug node/<node-name> -it --image=busybox -- sh -c "df -h"`
> 2. 清理未使用的镜像（谨慎操作）：`ssh <node-ip> "crictl rmi --prune"`
> **如果 crictl 不支持 prune** → `ssh <node-ip> "docker image prune -a -f"`
> **如果无法 SSH** → 请节点管理员手动清理
> 3. 清理已退出的容器：`ssh <node-ip> "crictl rm \$(crictl ps -a -q)"`
> **如果无法执行** → 请节点管理员手动清理
> 4. 验证磁盘空间：`ssh <node-ip> "df -h /var/lib/containerd"`

**分支决策**：
- **O1**：清理后磁盘充足，Pod 恢复正常 → 修复完成
- **O2**：清理后仍不足 → 考虑节点扩容或迁移 Pod
- **O3**：无法清理（权限/策略限制）→ 升级决策点

---

## 验证修复

**顾问指令**：
> 修复已应用，验证镜像拉取是否恢复正常。
> 1. 验证 Pod 状态：`kubectl get pod <pod-name> -n <namespace> -w`
> **如果无法 watch** → 间隔 30 秒执行 `kubectl get pod <pod-name> -n <namespace>`
> 2. 确认 Events 无拉取错误：`kubectl describe pod <pod-name> -n <namespace> | grep -A 10 Events`
> **如果无法执行** → `kubectl get events -n <namespace> --field-selector involvedObject.name=<pod-name>`
> 3. 确认容器已运行：`kubectl get pod <pod-name> -n <namespace> -o jsonpath='{.status.containerStatuses[*].ready}'`
> **如果无法执行 jsonpath** → `kubectl get pod <pod-name> -n <namespace>`，确认 READY 列为 `1/1`
> 4. 验证日志输出正常：`kubectl logs <pod-name> -n <namespace> --tail=30`
> **如果无法执行** → 请确认应用已正常启动
> 请告诉我以上四个验证结果。如果全部通过，问题已修复。

---


### 分支 1.4：阿里云ACK/专有云镜像拉取排查

工程师："我们在阿里云ACK/专有云环境，镜像拉取失败"

顾问："阿里云环境镜像仓库有特殊性，请按以下顺序排查：

**步骤 1：阿里云ACR状态检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 确认镜像仓库类型
kubectl get pod <pod> -o yaml | grep -A 2 image

# 检查ACR实例状态
aliyun cr GET /repos/<namespace>/<repo>

# 检查ACR网络访问
# 公网ACR：节点需有公网访问
# 专有云ACR：检查VPC连通性
```
> **如果无法执行aliyun CLI**：请登录ACR控制台，告诉我：
> 1. 镜像仓库和Tag是否存在？
> 2. 镜像构建状态是否成功？
> 3. 是否有访问控制限制？

**步骤 2：ACK镜像拉取Secret检查**
``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 检查默认Secret
kubectl get secret -n <ns> | grep docker

# 检查ACR免密插件
kubectl get pods -n kube-system | grep acr-credential

# 检查ServiceAccount绑定
kubectl get sa default -n <ns> -o yaml | grep imagePullSecrets
```
**步骤 3：专有云镜像特殊考虑**
- 专有云可能使用内部Harbor而非ACR
- 检查Harbor服务状态
- 确认镜像同步任务完成
- 检查专有云网络策略是否放行镜像仓库

**步骤 4：阿里云特定修复**

如ACR免密插件异常：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 重启ACR免密插件
kubectl delete pod -n kube-system -l app=acr-credential-helper

# 手动创建拉取Secret
kubectl create secret docker-registry acr-secret   --docker-server=registry.<region>.aliyuncs.com   --docker-username=<ram-user>   --docker-password=<password>
```
如专有云Harbor无法访问：
1. 检查Harbor Pod状态
2. 检查Harbor [[系统基础/topic-dictionary/networking/service.md|Service]]
3. 检查节点到Harbor网络连通性
4. 如Harbor异常，联系平台团队


## 升级决策点

| 条件 | 升级路径 | 说明 |
|------|---------|------|
| 镜像仓库完全不可用（503/502/维护中） | **镜像仓库管理团队** | 需要仓库侧恢复 |
| 涉及节点级容器运行时问题 | **SKILL-NODE-001** | 节点深度诊断 |
| 涉及 DNS 解析失败 | **SKILL-NET-001** | DNS 问题诊断 |
| 涉及网络策略/防火墙阻断出站 | **SKILL-NET-003** | 网络深度诊断 |
| 需要修改节点系统级配置 | **节点管理团队** | systemd、[[entities/kubelet.md|kubelet]] 等 |
| 镜像安全扫描/漏洞阻断 | **SKILL-SEC-003** | 安全策略相关 |
| 怀疑镜像被篡改或供应链攻击 | **安全团队** | 需紧急安全审查 |

**顾问升级话术**：
> 根据目前排查结果，这个问题超出了常规镜像拉取问题处理范围，可能涉及 **{具体原因}**。建议：
> 1. **立即止损**：如果可能，将已有镜像缓存的节点标记为可调度，或临时切换到备用镜像仓库
> 2. **升级诊断**：我会整理当前收集的所有信息，你可以提交给 **{升级目标团队}**
> 3. **持续监控**：继续观察镜像拉取事件，必要时在节点抓包分析
> 是否需要我帮你整理排查结果摘要？

---

## 附录：常用命令速查

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl rollout undo/restart`：触发滚动变更，影响副本

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 快速查看镜像拉取失败的 Pod
kubectl get pods --all-namespaces | grep -E "ImagePullBackOff|ErrImagePull"
# 查看 Pod 镜像地址
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.containers[*].image}'
# 查看 Pod 拉取事件
kubectl describe pod <pod> -n <ns> | grep -A 20 Events
# 检查 imagePullSecret
kubectl get pod <pod> -n <ns> -o jsonpath='{.spec.imagePullSecrets[*].name}'
# 修改镜像标签
kubectl set image deployment/<name> <container>=<image>:<tag> -n <ns>
# 创建 docker-registry Secret
kubectl create secret docker-registry <name> --docker-server=<registry> \
  --docker-username=<user> --docker-password=<pass> -n <ns>
# 绑定 Secret 到 ServiceAccount
kubectl patch serviceaccount default -n <ns> -p '{"imagePullSecrets":[{"name":"<secret>"}]}'
# 节点磁盘检查
kubectl describe node <node> | grep -A 10 "Allocated resources"
# 重启 Deployment
kubectl rollout restart deployment/<name> -n <ns>
```
---

*对话脚本版本: 1.0.0 | 技能: K8s Image Pull Failure 诊断与修复 | 模式: L2-semi-auto*


<!-- risk-assessed -->
