---
title: 'Day 4: 调试工具全家桶安装'
description: '### 1.1 kubectl 基础配置'
summary: '### 1.1 kubectl 基础配置'
category: skills
tags:
- k8s
- learn
- quick-start
- cilium
- helm
- rbac
tier: core
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 'Day 4: 调试工具全家桶安装 是什么'
- '如何 Day 4: 调试工具全家桶安装'
trigger_keywords:
- Day
- '4:'
- 调试工具全家桶安装
prerequisites:
- kubectl-basics
- helm-basics
- cilium-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




trigger_keywords:
- Day
- '4:'
- 调试工具全家桶安装
- learn  role: contributor---

# Day 4: 调试工具全家桶安装

> **适用对象**: 新入职 SRE/Ops 工程师 | **版本**: K8s 1.28-1.33

---

## 1. kubectl 家族

### 1.1 kubectl 基础配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubectl 安装（Linux/macOS）
# Linux
curl -LO "https://dl.k8s.io/release/$(curl -L -s https://dl.k8s.io/release/stable.txt)/bin/linux/amd64/kubectl"
sudo install -o root -g root -m 0755 kubectl /usr/local/bin/kubectl

# macOS
brew install kubectl

# 验证
kubectl version --client

# kubectl 自动补全
# Bash
echo 'source <(kubectl completion bash)' >> ~/.bashrc
source ~/.bashrc

# Zsh
echo 'source <(kubectl completion zsh)' >> ~/.zshrc
source ~/.zshrc
```
### 1.2 kubectl 别名配置

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# 添加到 ~/.bashrc 或 ~/.zshrc
alias k='kubectl'
alias kg='kubectl get'
alias kd='kubectl describe'
alias ke='kubectl exec -it'
alias kl='kubectl logs'
alias klf='kubectl logs -f'
alias kga='kubectl get pods -A'
alias kgpw='kubectl get pods -o wide'
alias kdp='kubectl describe pod'
alias kds='kubectl describe service'
alias kdn='kubectl describe node'

# 常用 flag 别名
alias kgp='kubectl get pods'
alias kgs='kubectl get services'
alias kgd='kubectl get deployments'
alias kgn='kubectl get nodes'

# 命名空间快捷切换
alias kn='kubectl config set-context --current --namespace'
kn default  # 切换到 default namespace
```
### 1.3 kubectx / kubens（krew 插件）

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装 krew
(
  set -x
  cd "$(mktemp -d)"
  OS="$(uname | tr '[:upper:]' '[:lower:]')"
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/;s/aarch64/arm64/')"
  KREW="krew-${OS}_${ARCH}"
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz"
  tar zxf "${KREW}.tar.gz"
  ./"${KREW}" install krew
)
export PATH="${KREW_ROOT:-$HOME/.krew}/bin:$PATH"

# 安装 kubectx 和 kubens
kubectl krew install ctx
kubectl krew install ns

# 使用
kubectl ctx                           # 列出所有上下文
kubectl ctx production               # 切换到 production
kubectl ctx staging                  # 切换到 staging
kubectl ns                           # 列出所有 namespace
kubectl ns production               # 切换 namespace
```
---

## 2. k9s 终端 UI

### 2.1 安装 k9s

```bash
# Linux
curl -fsSL https://github.com/derailed/k9s/releases/latest/download/k9s_Linux_amd64.tar.gz | tar xz
sudo mv k9s /usr/local/bin/

# macOS
brew install derailed/k9s/k9s

# 验证
k9s version
```

### 2.2 k9s 常用快捷键

| 快捷键 | 功能 |
|--------|------|
| `?` | 帮助 |
| `q` | 退出 |
| `ctrl+d` | 删除资源 |
| `ctrl+e` | 编辑资源 |
| `l` | 查看日志 |
| `s` | 缩放（扩缩容） |
| `shift+\` | 切换命名空间 |
| `0-9` | 切换视图（Pod/Deployment/Service 等） |

---

## 3. stern 日志工具

### 3.1 安装 stern

```bash
# Linux
curl -fsSL https://github.com/stern/stern/releases/latest/download/stern_linux_amd64.tar.gz | tar xz
sudo mv stern /usr/local/bin/

# macOS
brew install stern

# 验证
stern --version
```

### 3.2 stern 使用

```bash
# 跟踪所有 Pod 日志
stern . -n production

# 跟踪特定 Deployment 日志
stern backend -n production

# 跟踪带特定关键词的日志
stern "error|timeout" -n production

# 限制时间范围
stern --since=5m backend -n production

# 只看最近 50 行
stern backend -n production --tail=50

# 输出带颜色
stern backend -n production --color=always
```

---

## 4. kubescape 安全工具

### 4.1 安装 kubescape

```bash
# Linux/macOS
curl -fsSL https://raw.githubusercontent.com/kubescape/kubescape/master/install.sh | bash

# macOS
brew install kubescape

# 验证
kubescape version
```

### 4.2 kubescape 使用

```bash
# 扫描集群安全配置
kubescape scan cluster

# 扫描命名空间
kubescape scan namespace production

# 查看 RBAC 配置
kubescape scan RBAC

# 生成安全报告
kubescape scan --format html --output report.html cluster
```

---

## 5. Popeye 集群健康检查

### 5.1 安装 Popeye

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `helm upgrade/install`：部署/升级 release

``` bash
# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权
# Helm 安装
helm repo add doktorlenz https://doktorlenz.github.io/charts
helm install popeye doktorlenz/popeye -n popeye --create-namespace

# 或者 kubectl 插件
kubectl krew install popeye
```
### 5.2 Popeye 使用

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 扫描整个集群
kubectl popeye -A

# 扫描特定命名空间
kubectl popeye -n production

# 输出 JSON 格式
kubectl popeye -n production -o json

# 只扫描 Pod 配置（不扫描资源使用）
kubectl popeye -n production --scans pods
```
---

## 6. kubectl 插件集合（krew）

### 6.1 推荐插件

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装常用插件
kubectl krew install ctx          # 切换上下文
kubectl krew install ns           # 切换命名空间
kubectl krew install ktop         # top 命令类似 top pods/nodes
kubectl krew install node-shell   # SSH 到节点
kubectl krew install debug        # 调试 Pod
kubectl krew install neat          # 清理 kubectl 输出

# 查看所有插件
kubectl krew index list
kubectl krew search <keyword>
```
### 6.2 常用 kubectl 插件使用

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# node-shell: SSH 到节点
kubectl node-shell <node-name>

# debug: 调试 Pod
kubectl debug <pod-name> -it --image=busybox -- sh

# neat: 清理 YAML 输出
kubectl get pods -o yaml | kubectl neat

# ktop: 查看资源使用
kubectl ktop node
kubectl ktop pod -n production
```
---

## 7. 其他实用工具

### 7.1 yq (YAML 处理)

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# 安装
# Linux
sudo wget https://github.com/mikefarah/yq/releases/latest/download/yq_linux_amd64 -O /usr/local/bin/yq
sudo chmod +x /usr/local/bin/yq

# macOS
brew install yq

# 使用
kubectl get pod <pod-name> -o yaml | yq '.status.phase'
kubectl get pods -o json | yq '.items[0].metadata.name'
```
### 7.2 stern / kubetail (日志)

```bash
# kubetail（多个 Pod 日志）
# 安装
npm install -g kubetail

# 使用
kubetail <deployment-name> -n production
```

### 7.3 [[Cilium|cilium]] (CNI 调试)

```bash
# Cilium CLI
curl -fsSL https://github.com/cilium/cilium-cli/releases/latest/download/cilium-linux-amd64.tar.gz | tar xz
sudo mv cilium /usr/local/bin/

# 使用
cilium status
cilium connectivity test
cilium endpoint list
```

---

## 8. 工具安装验证

### 8.1 一键验证脚本

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
cat > verify-tools.sh <<'EOF'
#!/bin/bash

echo "=== 工具安装验证 ==="

# kubectl
echo "[1] kubectl"
kubectl version --client &>/dev/null && echo "  OK" || echo "  FAIL"

# k9s
echo "[2] k9s"
k9s version &>/dev/null && echo "  OK" || echo "  FAIL"

# stern
echo "[3] stern"
stern --version &>/dev/null && echo "  OK" || echo "  FAIL"

# kubescape
echo "[4] kubescape"
kubescape version &>/dev/null && echo "  OK" || echo "  FAIL"

# yq
echo "[5] yq"
yq --version &>/dev/null && echo "  OK" || echo "  FAIL"

# kubectx/kubens
echo "[6] kubectx"
kubectl ctx --help &>/dev/null && echo "  OK" || echo "  FAIL"

echo "=== 验证完成 ==="
EOF
chmod +x verify-tools.sh
./verify-tools.sh
```
---

## 9. 工具配置汇总

### 9.1 ~/.bashrc 或 ~/.zshrc 配置

``` bash
# 🟢 低风险：只读/信息收集，通常无副作用
# kubectl 配置
export PATH="${KREW_ROOT:-$HOME/.krew}/bin:$PATH"
source <(kubectl completion bash)

# kubectl 别名
alias k='kubectl'
alias kg='kubectl get'
alias kd='kubectl describe'
alias kga='kubectl get pods -A'
alias kdp='kubectl describe pod'
alias klf='kubectl logs -f'

# kubectx/kubens
source <(kubectl ctx completion bash)
source <(kubectl ns completion bash)
```
---

```yaml
---  - "调试工具怎么安装"
  - "kubectl插件有哪些"
  - "k9s怎么用"
  - "日志工具对比"
  - "stern安装配置"  - "kubectl插件"
  - "k9s安装"
  - "stern日志"
  - "kubescape安全"
  - "Popeye集群检查"
  - "krew插件管理"
  - "yq工具"
  - "终端UI"
  - "调试工具"  - sre工程师
  - ops工程师
  - 运维工程师
related_domains:
  - domain-01-cluster-fundamentals
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/quick-start/01-day-one-checklist
  - domain-11-production-operations/topic-learn/quick-start/02-first-ticket-guide
  - domain-07-platform-engineering/26-kubectl-plugin-ecosystem
id: QUICKSTART-DAY4
topic: onboarding
type: setup-guide
tags: [onboarding, tools, kubectl, k9s, stern, debugging, sre, ops-engineer, k8s-1.28-1.33]
---
```

## Related

- [[deployment]] — Deployment
- [[helm]] — Helm
- [[kubescape]] — Kubescape
- [[cni]] — CNI (Container Network Interface)
- [[kubernetes]] — Kubernetes (CNCF Graduated)

```

<!-- risk-assessed -->
