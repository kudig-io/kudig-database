# KUDIG-DATABASE Manpages

> 核心开源产品和项目脚本的 manpage 参考文档

## 目录结构

```
man/
├── man1/           # 用户命令 (User Commands)
│   ├── kudig-stats.1
│   ├── kudig-quality.1
│   ├── kudig-validate.1
│   └── kudig-fta-viz.1
├── man8/           # 系统管理命令 (System Administration)
│   ├── kubernetes.8
│   ├── prometheus.8
│   ├── etcd.8
│   ├── containerd.8
│   ├── cilium.8
│   ├── helm.8
│   ├── argocd.8
│   ├── istio.8
│   ├── velero.8
│   └── cert-manager.8
└── README.md       # 本文件
```

## 使用方法

### 方式一：直接查看

```bash
# 查看项目脚本帮助
man ./man/man1/kudig-stats.1
man ./man/man1/kudig-quality.1

# 查看核心开源产品帮助
man ./man/man8/kubernetes.8
man ./man/man8/prometheus.8
```

### 方式二：安装到系统

#### Linux

```bash
# 复制到系统 man 目录
sudo cp -r man/man1/* /usr/local/share/man/man1/
sudo cp -r man/man8/* /usr/local/share/man/man8/

# 更新 man 数据库
sudo mandb

# 现在可以直接使用
man kudig-stats
man kubernetes
```

#### macOS

```bash
# 复制到系统 man 目录
sudo cp -r man/man1/* /usr/local/share/man/man1/
sudo cp -r man/man8/* /usr/local/share/man/man8/

# 使用
man kudig-stats
man kubernetes
```

### 方式三：添加到 MANPATH

```bash
# 临时添加（当前会话）
export MANPATH="$MANPATH:$(pwd)/man"
man kudig-stats

# 永久添加（添加到 ~/.bashrc 或 ~/.zshrc）
echo 'export MANPATH="$MANPATH:/path/to/kudig-database/man"' >> ~/.bashrc
```

## Manpage 清单

### Section 1 - 用户命令 (KUDIG 项目脚本)

| 命令 | 描述 | 对应脚本 |
|:---|:---|:---|
| `kudig-stats` | README 数字指标自动统计 | `scripts/generate-readme-stats.sh` |
| `kudig-quality` | 知识库全面质量检查 | `scripts/comprehensive-quality-check.sh` |
| `kudig-validate` | 代码示例语法校验 | `scripts/code-example-validation.sh` |
| `kudig-fta-viz` | FTA 故障树可视化 | `scripts/fta_tree_visualization.py` |

### Section 8 - 系统管理 (CNCF 核心开源产品)

| 产品 | 描述 | 文档位置 |
|:---|:---|:---|
| `kubernetes` | 容器编排平台 | `domain-1-architecture-fundamentals/` |
| `prometheus` | 监控和告警系统 | `domain-8-observability/` |
| `etcd` | 分布式键值存储 | `domain-3-control-plane/` |
| `containerd` | 容器运行时 | `domain-3-control-plane/` |
| `cilium` | eBPF 网络和安全 | `domain-5-networking/` |
| `helm` | Kubernetes 包管理器 | `domain-10-extensions/` |
| `argocd` | GitOps 持续交付 | `domain-9-platform-ops/` |
| `istio` | 服务网格平台 | `domain-26-service-mesh-microservices/` |
| `velero` | 备份和灾难恢复 | `domain-30-disaster-recovery-business-continuity/` |
| `cert-manager` | 证书管理自动化 | `domain-9-platform-ops/` |

## 文档标准

本项目的 manpage 遵循以下标准：

1. **格式标准**: 使用传统 Unix man 宏格式 (man 7 man)
2. **章节结构**:
   - NAME - 名称和简要描述
   - SYNOPSIS - 命令语法
   - DESCRIPTION - 详细描述
   - OPTIONS - 命令选项
   - EXAMPLES - 使用示例
   - SEE ALSO - 相关文档
   - AUTHOR - 作者信息
   - COPYRIGHT - 许可证信息

3. **交叉引用**: 每个 manpage 都链接到 KUDIG-DATABASE 的相关文档

## 更新和维护

当添加新的核心开源产品或项目脚本时，请同步创建对应的 manpage：

```bash
# 创建新的 manpage
touch man/man1/<command>.1   # 用户命令
touch man/man8/<product>.8   # 系统管理命令
```

使用现有的 manpage 作为模板，确保格式一致性。

## 故障排查

### man 命令找不到页面

```bash
# 检查文件是否存在
ls -la man/man1/kudig-stats.1

# 检查 MANPATH
man -w kudig-stats

# 手动指定路径
man ./man/man1/kudig-stats.1
```

### 格式显示问题

```bash
# 确保使用正确的编码
export LC_ALL=en_US.UTF-8
man ./man/man1/kudig-stats.1
```

## 相关资源

- [KUDIG-DATABASE 主文档](../README.md)
- [项目脚本](../scripts/README.md)
- [CNCF 项目库](../domain-34-cncf-landscape/)
