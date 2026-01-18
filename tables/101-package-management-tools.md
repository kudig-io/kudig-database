# 101 - 包管理与应用分发工具 (Package Management & Distribution)

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-01

## 核心工具对比矩阵

| 工具 (Tool) | 核心定位 (Position) | 生产优势 (Production Advantages) | ACK 集成 |
|------------|-------------------|--------------------------------|---------|
| **Helm** | 应用包管理器 | Chart 版本控制、回滚、依赖管理 | ✅ 原生支持 |
| **Kustomize** | 声明式配置管理 | 无模板、原生集成、多环境叠加 | ✅ kubectl 内置 |
| **Operator SDK** | 自动化运维框架 | 生命周期管理、自愈能力 | ✅ OLM 支持 |
| **ArgoCD** | GitOps 持续部署 | 声明式、自动同步、审计 | ✅ 可集成 |

## Helm 生产最佳实践

### 1. Chart 仓库管理
```bash
# 使用 OCI 仓库 (推荐)
helm registry login registry.cn-hangzhou.aliyuncs.com
helm push mychart-0.1.0.tgz oci://registry.cn-hangzhou.aliyuncs.com/mynamespace

# 传统 HTTP 仓库
helm repo add stable https://charts.helm.sh/stable
helm repo update
```

### 2. 值文件分层策略
```yaml
# values-prod.yaml (生产环境)
replicaCount: 3
resources:
  limits:
    cpu: 2
    memory: 4Gi
ingress:
  enabled: true
  className: nginx
  annotations:
    cert-manager.io/cluster-issuer: letsencrypt-prod
```

### 3. 版本管理与回滚
```bash
# 查看发布历史
helm history myapp -n production

# 回滚到指定版本
helm rollback myapp 3 -n production

# 原子性部署 (失败自动回滚)
helm upgrade --install myapp ./mychart --atomic --timeout 10m
```

## Kustomize 多环境管理

### 目录结构
```
├── base/
│   ├── deployment.yaml
│   ├── service.yaml
│   └── kustomization.yaml
├── overlays/
│   ├── dev/
│   │   └── kustomization.yaml
│   ├── staging/
│   │   └── kustomization.yaml
│   └── prod/
│       ├── kustomization.yaml
│       └── replicas.yaml
```

### 生产环境叠加
```yaml
# overlays/prod/kustomization.yaml
apiVersion: kustomize.config.k8s.io/v1beta1
kind: Kustomization
bases:
  - ../../base
replicas:
  - name: myapp
    count: 5
images:
  - name: myapp
    newTag: v1.2.3
patchesStrategicMerge:
  - replicas.yaml
```

## Operator 开发模式

| 模式 (Pattern) | 适用场景 (Use Case) | 复杂度 |
|---------------|-------------------|-------|
| **Helm Operator** | 简单应用打包 | 低 |
| **Ansible Operator** | 运维自动化 | 中 |
| **Go Operator** | 复杂状态管理 | 高 |

### 生产 Operator 示例 (MySQL Cluster)
- 自动主从切换
- 定时备份与恢复
- 资源自动扩缩容
- 监控指标暴露

## 故障排查

| 问题 (Issue) | 诊断 (Diagnosis) | 解决方案 (Solution) |
|-------------|-----------------|-------------------|
| Helm 安装失败 | `helm get manifest` | 检查 RBAC 权限 |
| Kustomize 构建错误 | `kustomize build --enable-alpha-plugins` | 验证 YAML 语法 |
| Operator 无法调谐 | 查看 Operator 日志 | 检查 CRD 版本兼容性 |


---

**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)