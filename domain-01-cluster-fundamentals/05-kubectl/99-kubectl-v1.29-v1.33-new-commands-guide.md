---
title: Kubectl v1.29 - v1.33 新命令与用法速查
description: '# 查询节点日志 (需 NodeLogQuery Feature Gate)'
summary: 'kubectl events --sort-by='.lastTimestamp''
category: architecture-fundamentals
tags:
- k8s
- architecture
- kubernetes
- kubelet
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 架构师
- SRE
- 平台工程师
estimated_read_time: 5min
intent_queries:
- Kubectl v1.29 - v1.33 新命令与用法速查 是什么
- 如何 Kubectl v1.29 - v1.33 新命令与用法速查
- Kubernetes 1 architecture fundamentals 最佳实践
trigger_keywords:
- Kubectl
- v1.29
- v1.33
- 新命令与用法速查
- architecture
- fundamentals
prerequisites:
- kubectl-basics
- kubernetes-concepts
- tls-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
cross_refs:
- type: domain
  path: ../domain-13-container-runtime/
  label: '相关知识域: domain-13-container-runtime'
- type: domain
  path: ../domain-01-cluster-fundamentals/
  label: '相关知识域: domain-01-cluster-fundamentals'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/k8s.md
  label: '速查卡: k8s'
- type: cheatsheet
  path: ../domain-17-system-foundation/topic-cheat-sheet/kubectl-scene-cheatsheet.md
  label: '速查卡: kubectl-scene-cheatsheet'
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# Kubectl v1.29 - v1.33 新命令与用法速查

> **适用版本**: kubectl v1.29 - v1.33  
> **最后更新**: 2026-04-24  
> **难度**: 初级 → 中级

---

<!-- chunk: 📋 目录 -->
## 📋 目录

- [一、v1.29 新命令](#一v1.29-新命令)
- [二、v1.30 新命令](#二v1.30-新命令)
- [三、v1.31 新命令](#三v1.31-新命令)
- [四、v1.32 新命令](#四v1.32-新命令)
- [五、v1.33 新命令](#五v1.33-新命令)
- [六、命令增强与改进](#六命令增强与改进)
- [七、插件生态更新](#七插件生态更新)
- [八、快捷别名推荐](#八快捷别名推荐)

---

<!-- chunk: 一、v1.29 新命令 -->
## 一、v1.29 新命令

### kubectl events (替代 kubectl get events)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 更友好的事件查看
kubectl events

# 按类型筛选
kubectl events --types=Warning

# 按对象筛选
kubectl events --for=deployment/myapp

# 按时间范围
kubectl events --since=1h

# 排序输出
kubectl events --sort-by='.lastTimestamp'
```
### kubectl debug 改进

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用临时容器调试 (Ephemeral Containers GA)
kubectl debug mypod -it --image=busybox --target=myapp

# 复制 Pod 并调试
kubectl debug mypod -it --copy-to=mypod-debug --image=busybox

# 调试节点 (v1.29+ 改进)
kubectl debug node/mynode -it --image=ubuntu
```
---

<!-- chunk: 二、v1.30 新命令 -->
## 二、v1.30 新命令

### kubectl alpha node-logs (Alpha)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查询节点日志 (需 NodeLogQuery Feature Gate)
kubectl alpha node-logs mynode

# 查看特定服务日志
kubectl alpha node-logs mynode --service=kubelet

# 查看系统日志
kubectl alpha node-logs mynode --syslog

# 尾部跟踪
kubectl alpha node-logs mynode --tail=100 -f
```
### kubectl apply --prune-allowlist 改进

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 更安全的 prune，允许指定保留的资源类型
kubectl apply -k ./ --prune --prune-allowlist=core/v1/ConfigMap --prune-allowlist=core/v1/Secret
```
---

<!-- chunk: 三、v1.31 新命令 -->
## 三、v1.31 新命令

### kubectl rollout status 增强

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 支持自定义超时
kubectl rollout status deployment/myapp --timeout=5m

# 查看历史版本差异
kubectl rollout history deployment/myapp --revision=3
```
### kubectl wait 改进

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 等待删除完成
kubectl wait --for=delete pod/mypod --timeout=60s

# 等待多个资源
kubectl wait --for=condition=Ready pods -l app=myapp
```
---

<!-- chunk: 四、v1.32 新命令 -->
## 四、v1.32 新命令

### kubectl debug 增强

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 调试 Profile 支持
kubectl debug mypod --profile=netadmin     # 网络管理员权限
kubectl debug mypod --profile=sysadmin     # 系统管理员权限
kubectl debug mypod --profile=restricted   # 受限权限

# 自定义安全上下文
kubectl debug mypod --custom=securityContext.privileged=true
```
### kubectl create token 改进

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 创建短期 ServiceAccount Token
kubectl create token mysa --duration=10m

# 绑定特定 audience
kubectl create token mysa --audience=https://myapp.example.com
```
---

<!-- chunk: 五、v1.33 新命令 -->
## 五、v1.33 新命令

### kubectl get --show-labels 改进

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 支持选择特定标签显示
kubectl get pods --show-labels -L app,version
```
### kubectl delete --wait / --now

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 强制立即删除 (不等待优雅终止)
kubectl delete pod mypod --now

# 删除并等待完成
kubectl delete deployment myapp --wait --timeout=60s
```
### kubectl label/annotate --dry-run 改进

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl label/annotate`：改元数据可能影响选择器/控制器

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 预览标签变更
kubectl label pod mypod env=prod --dry-run=client -o yaml

# 预览注解变更
kubectl annotate pod mypod description="test" --dry-run=server
```
---

<!-- chunk: 六、命令增强与改进 -->
## 六、命令增强与改进

### kubectl explain 增强

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 查看字段的 CEL 表达式支持 (v1.30+ ValidatingAdmissionPolicy)
kubectl explain pod.spec --recursive | grep -A5 "CEL"

# 查看字段的默认值
kubectl explain deployment.spec.strategy.rollingUpdate.maxUnavailable

# 查看字段的枚举值
kubectl explain pod.spec.restartPolicy
```
### kubectl get 输出格式

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 自定义列输出 (所有版本)
kubectl get pods -o custom-columns='NAME:.metadata.name,STATUS:.status.phase,IP:.status.podIP'

# JSONPath 高级查询
kubectl get pods -o jsonpath='{range .items[*]}{.metadata.name}{"\t"}{.status.phase}{"\n"}{end}'

# 排序输出
kubectl get pods --sort-by='.status.startTime'
kubectl get pods --sort-by='.spec.nodeName'
```
### kubectl patch 增强

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 原地调整 Pod 资源 (v1.33 Alpha Feature Gate)
kubectl patch pod mypod --patch '
{
  "spec": {
    "containers": [{
      "name": "app",
      "resources": {
        "requests": {"cpu": "200m", "memory": "256Mi"},
        "limits": {"cpu": "400m", "memory": "512Mi"}
      }
    }]
  }
}'
```
---

<!-- chunk: 七、插件生态更新 -->
## 七、插件生态更新

### krew 插件 (v1.29-v1.33 兼容)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 krew
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower:]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/\(arm\)\(64\)\?.*/\1\2/' -e 's/aarch64$/arm64/')" &&
  KREW="krew-${OS}_${ARCH}" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz" &&
  tar zxvf "${KREW}.tar.gz" &&
  ./"${KREW}" install krew
)

# 必备插件
kubectl krew install ctx          # 快速切换上下文
kubectl krew install ns           # 快速切换命名空间
kubectl krew install neat         # 清理 YAML 输出
kubectl krew install tree         # 资源依赖树
kubectl krew install get-all      # 列出所有资源
kubectl krew install resource-capacity  # 资源容量查看
kubectl krew install view-allocations   # 资源分配查看
kubectl krew install cert-manager       # cert-manager 管理
kubectl krew install df-pv            # PV 磁盘使用
kubectl krew install node-shell       # 节点 shell
```
### kubectl 版本管理

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 使用 kubectl version 管理器
# 安装多个版本
brew install kubectl

# 查看可用版本
kubectl version --client

# 使用 asdf 管理版本
asdf plugin add kubectl
asdf install kubectl 1.33.0
asdf global kubectl 1.33.0
```
---

<!-- chunk: 八、快捷别名推荐 -->
## 八、快捷别名推荐

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）
> - `kubectl edit/patch`：修改运行中的资源
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

``` bash
# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案
# ~/.bashrc 或 ~/.zshrc

# 基础别名
alias k='kubectl'
alias kg='kubectl get'
alias kd='kubectl describe'
alias ke='kubectl edit'
alias kdel='kubectl delete'
alias ka='kubectl apply'
alias kaf='kubectl apply -f'
alias kdf='kubectl delete -f'
alias kex='kubectl exec -it'
alias kl='kubectl logs'
alias klf='kubectl logs -f'

# 资源类型快捷
alias kgp='kubectl get pods'
alias kgs='kubectl get svc'
alias kgd='kubectl get deploy'
alias kgn='kubectl get nodes'
alias kgns='kubectl get ns'
alias kgpvc='kubectl get pvc'
alias kgcm='kubectl get cm'
alias kgsec='kubectl get secret'

# 上下文和命名空间
alias kctx='kubectl ctx'
alias kns='kubectl ns'
alias kc='kubectl config current-context'
alias kgc='kubectl config get-contexts'

# 常用操作
alias kdp='kubectl describe pod'
alias kdd='kubectl describe deploy'
alias kdn='kubectl describe node'
alias ktp='kubectl top pod'
alias ktn='kubectl top node'
alias kwp='kubectl get pods -w'
alias kw='kubectl get pods -w'

# 清理
alias krmf='kubectl delete --all pods --grace-period=0 --force'

# 补全
source <(kubectl completion bash)  # Bash
source <(kubectl completion zsh)   # Zsh
```
---

<!-- chunk: 参考链接 -->
## 参考链接

- [Kubectl 参考](https://kubernetes.io/docs/reference/generated/kubectl/kubectl-commands/)
- [Kubectl 备忘单](https://kubernetes.io/docs/reference/kubectl/cheatsheet/)
- [Krew 插件索引](https://krew.sigs.k8s.io/plugins/)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- domain-01-cluster-fundamentals KUDIG Database — Global MOC
- [[domain-01-cluster-fundamentals/README.md|Domain-1: Kubernetes架构基础]]
- index.md|Domain-1 架构基础 — 开源项目索引]]
- [[entities/kubernetes.md|kubernetes]]
- [[entities/kubernetes.md|kubernetes]]
- 03 - 功能和API表
- 04 - Kubernetes 源码结构深度解析
- kubectl 命令完整参考
- 06 - 集群配置参数完全参考
- 07 - 升级路径与策略指南
- 08 - 多租户架构设计 (Multi-Tenancy Architecture)
- 09 - 边缘计算集成架构 (KubeEdge/OpenYurt)

## See Also

- 17-production-operations-best-practices
- 18-upgrade-migration-strategy
- 99-kubernetes-api-version-matrix
- 99-kubernetes-core-components-v1.29-v1.33-update

## Related

- [[domain-19-landscape-references/topic-index/gitops-cicd-index.md|GitOps / CI-CD 全局索引]]


<!-- risk-assessed -->
