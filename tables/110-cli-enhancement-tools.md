# CLI增强工具

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01 | **参考**: [k9s](https://k9scli.io/) | [kubectx](https://github.com/ahmetb/kubectx)

## 工具对比

| 工具 | 类型 | 功能 | 交互方式 | 学习曲线 | 效率提升 | 生产推荐 |
|------|------|------|---------|---------|---------|---------|
| **k9s** | 终端UI | 全功能管理 | 键盘交互 | ⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **kubectx/kubens** | 上下文切换 | 集群/命名空间切换 | 命令行 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐⭐ | 强烈推荐 |
| **stern** | 日志聚合 | 多Pod日志 | 命令行 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 推荐 |
| **Lens** | 桌面应用 | 图形化管理 | GUI | ⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 本地开发 |
| **kube-ps1** | Shell提示符 | 显示上下文 | 被动显示 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐ | 辅助工具 |
| **kubectl-aliases** | 命令别名 | 快捷命令 | 命令行 | ⭐⭐⭐⭐⭐ | ⭐⭐⭐⭐ | 效率提升 |

---

## k9s - 终端UI管理神器

### 核心特性

```
k9s功能
├── 资源浏览(Pod/Deployment/Service等)
├── 实时日志查看
├── 资源编辑(YAML)
├── Shell访问(kubectl exec)
├── 端口转发
├── 资源监控(CPU/Memory)
├── 插件系统
└── 多集群管理
```

### 安装

```bash
# macOS
brew install k9s

# Linux
curl -sS https://webinstall.dev/k9s | bash

# Windows
scoop install k9s

# 或下载二进制
wget https://github.com/derailed/k9s/releases/download/v0.31.0/k9s_Linux_amd64.tar.gz
tar -xzf k9s_Linux_amd64.tar.gz
sudo mv k9s /usr/local/bin/
```

### 常用快捷键

| 快捷键 | 功能 | 说明 |
|-------|------|------|
| `:pod` | 查看Pod | 输入资源类型 |
| `/` | 过滤资源 | 支持正则 |
| `l` | 查看日志 | 实时日志 |
| `d` | 描述资源 | kubectl describe |
| `e` | 编辑资源 | YAML编辑 |
| `s` | Shell访问 | kubectl exec |
| `Ctrl-a` | 附加容器 | kubectl attach |
| `pf` | 端口转发 | Port forward |
| `y` | 查看YAML | 完整定义 |
| `Ctrl-d` | 删除资源 | 确认删除 |
| `Ctrl-k` | 删除Pod | 强制删除 |
| `?` | 帮助 | 快捷键列表 |

### 配置文件 ($HOME/.config/k9s/config.yml)

```yaml
k9s:
  # 日志刷新率
  refreshRate: 2
  
  # 最大日志行数
  maxConnRetry: 5
  
  # 启用鼠标
  enableMouse: true
  
  # 日志配置
  logger:
    tail: 200
    buffer: 5000
    sinceSeconds: 60
  
  # 皮肤主题
  skin: dracula
  
  # 资源限制
  thresholds:
    cpu:
      critical: 90
      warn: 70
    memory:
      critical: 90
      warn: 70
```

### 自定义别名 ($HOME/.config/k9s/alias.yml)

```yaml
alias:
  # 快捷访问
  pp: v1/pods
  dep: apps/v1/deployments
  svc: v1/services
  ing: networking.k8s.io/v1/ingresses
  
  # 自定义查询
  failing: v1/pods?labelSelector=status!=Running
  nginx: v1/pods?labelSelector=app=nginx
```

### 插件系统 ($HOME/.config/k9s/plugin.yml)

```yaml
plugin:
  # 查看Pod镜像漏洞
  trivy:
    shortCut: Shift-T
    confirm: false
    description: Trivy scan
    scopes:
      - pods
    command: kubectl
    background: false
    args:
      - exec
      - -it
      - -n
      - $NAMESPACE
      - $NAME
      - --
      - trivy
      - image
      - $COL-IMAGE
  
  # 查看Pod网络连接
  netstat:
    shortCut: Shift-N
    confirm: false
    description: Network connections
    scopes:
      - pods
    command: kubectl
    background: false
    args:
      - exec
      - -n
      - $NAMESPACE
      - $NAME
      - --
      - netstat
      - -tunlp
```

---

## kubectx + kubens

### kubectx - 集群上下文切换

```bash
# 安装
brew install kubectx

# 列出所有上下文
kubectx

# 切换上下文
kubectx prod-cluster

# 切换到上一个上下文
kubectx -

# 重命名上下文
kubectx new-name=old-name

# 删除上下文
kubectx -d context-name

# 显示当前上下文
kubectx -c
```

### kubens - 命名空间切换

```bash
# 列出所有命名空间
kubens

# 切换命名空间
kubens production

# 切换到上一个命名空间
kubens -

# 显示当前命名空间
kubens -c
```

### 交互式选择(fzf集成)

```bash
# 安装fzf
brew install fzf

# kubectx自动启用fzf
kubectx  # 显示交互式选择器
kubens   # 显示交互式选择器
```

---

## stern - 多Pod日志聚合

### 安装与使用

```bash
# 安装
brew install stern

# 查看所有包含"myapp"的Pod日志
stern myapp -n production

# 正则匹配
stern "^myapp-" -n production

# 多命名空间
stern myapp --all-namespaces

# 指定容器
stern myapp -c nginx -n production

# 仅显示特定时间范围
stern myapp --since 1h -n production

# 输出格式
stern myapp --template '{{.PodName}} | {{.Message}}' -n production

# 彩色输出(高亮错误)
stern myapp --color always -n production

# 排除某些Pod
stern myapp --exclude "test" -n production

# 实时关注新Pod
stern myapp --tail 0 -n production
```

### 高级过滤

```bash
# 按标签过滤
stern -l app=myapp,env=prod -n production

# 按字段选择器
stern --field-selector status.phase=Running -n production

# 组合条件
stern myapp -l tier=backend --field-selector status.phase=Running
```

### 输出到文件

```bash
# 保存日志
stern myapp -n production > myapp.log

# JSON格式输出
stern myapp -n production --output json > myapp.json

# 自定义模板
stern myapp --template '{{printf "%s | %s | %s\n" .PodName .ContainerName .Message}}' -n production
```

---

## Lens - Kubernetes IDE

### 核心功能

```
Lens Desktop功能
├── 多集群管理
├── 图形化资源查看
├── 实时监控(Prometheus集成)
├── 终端内嵌(kubectl/Shell)
├── Helm Chart管理
├── YAML编辑器(语法高亮)
├── 扩展插件系统
└── 资源拓扑图
```

### 安装

```bash
# macOS
brew install --cask lens

# Windows
winget install Lens

# Linux
# 从官网下载 .deb/.rpm
wget https://api.k8slens.dev/binaries/Lens-2023.12.0-latest.amd64.deb
sudo dpkg -i Lens-2023.12.0-latest.amd64.deb
```

### Lens扩展推荐

| 扩展名 | 功能 | 用途 |
|-------|------|------|
| **@alebcay/openlens-node-pod-menu** | Pod节点菜单 | 快速访问 |
| **@nevalla/kube-resource-explorer** | 资源浏览器 | 深度查看 |
| **@lens-extension/metrics** | 指标增强 | 监控面板 |
| **@lens-extension/resource-map** | 资源拓扑图 | 依赖可视化 |

---

## kube-ps1 - Shell提示符增强

### 安装配置

```bash
# 安装
brew install kube-ps1

# Bash配置 (~/.bashrc)
source "/usr/local/opt/kube-ps1/share/kube-ps1.sh"
PS1='$(kube_ps1)'$PS1

# Zsh配置 (~/.zshrc)
source "/usr/local/opt/kube-ps1/share/kube-ps1.sh"
PROMPT='$(kube_ps1) '$PROMPT

# 自定义显示
KUBE_PS1_PREFIX="["
KUBE_PS1_SUFFIX="]"
KUBE_PS1_SYMBOL_ENABLE=true
KUBE_PS1_SYMBOL_DEFAULT="☸️ "
KUBE_PS1_CTX_COLOR="cyan"
KUBE_PS1_NS_COLOR="green"
```

### 效果示例

```bash
# 默认显示
(⎈|prod-cluster:production) user@host:~$

# 自定义显示
[☸️ prod-cluster:production] user@host:~$
```

---

## kubectl-aliases - 命令别名

### 安装配置

```bash
# 下载别名文件
wget https://raw.githubusercontent.com/ahmetb/kubectl-aliases/master/.kubectl_aliases

# 添加到 ~/.bashrc 或 ~/.zshrc
[ -f ~/.kubectl_aliases ] && source ~/.kubectl_aliases

# 启用自动补全(可选)
function kubectl() { command kubectl "$@"; }
complete -F __start_kubectl kubectl
```

### 常用别名示例

| 别名 | 完整命令 | 说明 |
|-----|---------|------|
| `k` | `kubectl` | 基础 |
| `kg` | `kubectl get` | 获取资源 |
| `kgpo` | `kubectl get pods` | 获取Pod |
| `kdp` | `kubectl describe pod` | 描述Pod |
| `klo` | `kubectl logs` | 查看日志 |
| `kex` | `kubectl exec -it` | 执行命令 |
| `kaf` | `kubectl apply -f` | 应用YAML |
| `kdf` | `kubectl delete -f` | 删除资源 |
| `kgpoyaml` | `kubectl get pod -o yaml` | YAML输出 |
| `kgpow` | `kubectl get pod -o wide` | 详细输出 |

### 自定义别名

```bash
# 添加到 ~/.bashrc 或 ~/.zshrc
alias kgpa='kubectl get pods --all-namespaces'
alias kgpn='kubectl get pods -n'
alias klf='kubectl logs -f'
alias kpf='kubectl port-forward'
alias kgd='kubectl get deployments'
alias kgs='kubectl get services'
alias kgi='kubectl get ingress'

# 复杂别名
alias kgpoerror='kubectl get pods --all-namespaces --field-selector=status.phase!=Running'
alias kgponr='kubectl get pods --all-namespaces --field-selector=status.phase!=Running,status.phase!=Succeeded'
```

---

## kubectl插件管理器 - krew

### 安装krew

```bash
# macOS/Linux
(
  set -x; cd "$(mktemp -d)" &&
  OS="$(uname | tr '[:upper:]' '[:lower:]')" &&
  ARCH="$(uname -m | sed -e 's/x86_64/amd64/' -e 's/\(arm\)\(64\)\?.*/\1\2/' -e 's/aarch64$/arm64/')" &&
  KREW="krew-${OS}_${ARCH}" &&
  curl -fsSLO "https://github.com/kubernetes-sigs/krew/releases/latest/download/${KREW}.tar.gz" &&
  tar zxvf "${KREW}.tar.gz" &&
  ./"${KREW}" install krew
)

# 添加到PATH (~/.bashrc 或 ~/.zshrc)
export PATH="${KREW_ROOT:-$HOME/.krew}/bin:$PATH"
```

### 推荐插件

```bash
# 查看可用插件
kubectl krew search

# 安装推荐插件
kubectl krew install \
  ctx \          # kubectx
  ns \           # kubens
  tail \         # stern
  debug \        # kubectl-debug
  tree \         # 资源树形结构
  access-matrix \# RBAC权限矩阵
  neat \         # 清理YAML输出
  resource-capacity \  # 资源容量
  view-secret    # 查看Secret内容

# 更新插件
kubectl krew upgrade
```

---

## 终端效率配置

### Zsh + Oh My Zsh + 插件

```bash
# 安装Oh My Zsh
sh -c "$(curl -fsSL https://raw.githubusercontent.com/ohmyzsh/ohmyzsh/master/tools/install.sh)"

# 安装插件
git clone https://github.com/zsh-users/zsh-autosuggestions ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-autosuggestions
git clone https://github.com/zsh-users/zsh-syntax-highlighting ${ZSH_CUSTOM:-~/.oh-my-zsh/custom}/plugins/zsh-syntax-highlighting

# 配置 ~/.zshrc
plugins=(
  git
  kubectl
  docker
  zsh-autosuggestions
  zsh-syntax-highlighting
)

# 启用kube-ps1
source "/usr/local/opt/kube-ps1/share/kube-ps1.sh"
PROMPT='$(kube_ps1) '$PROMPT
```

---

## 工具组合推荐

### 日常运维

```bash
# 终端UI
k9s

# 快速切换
kubectx prod-cluster
kubens production

# 日志查看
stern myapp -n production

# 命令行快捷
k get pods -w  # kubectl别名
```

### 开发调试

```bash
# 图形化IDE
lens

# 命令行调试
kubectl debug myapp-pod-xxxx --image=nicolaka/netshoot

# 端口转发
kubectl port-forward service/myapp 8080:80
```

---

## 最佳实践

```yaml
# 1. 工具安装清单
install:
  - k9s: 终端UI
  - kubectx/kubens: 上下文切换
  - stern: 日志聚合
  - krew: 插件管理
  - kube-ps1: 提示符

# 2. Shell配置
shell:
  - kubectl别名
  - 自动补全
  - kube-ps1提示符

# 3. 效率组合
workflow:
  - k9s: 资源浏览
  - stern: 日志追踪
  - kubectx: 快速切换
  - kubectl: 精确操作
```

---

## 常见问题

**Q: k9s vs Lens如何选择?**  
A: k9s适合终端环境、SSH远程；Lens适合本地开发、可视化需求。

**Q: 如何自动切换命名空间?**  
A: 使用kubens或配置kubectl context的默认namespace。

**Q: stern日志太多如何过滤?**  
A: 使用`--exclude`排除、`-l`标签过滤、管道到`grep`。
