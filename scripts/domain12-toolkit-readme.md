# Domain-12 故障排查工具套件使用说明

## 📋 工具概述

这是一个为Kubernetes生产环境设计的专业级故障排查工具套件，包含完整的诊断、分析和报告功能，帮助运维专家快速定位和解决集群问题。

## 🚀 快速开始

### 安装依赖
```bash
# 确保以下工具已安装
kubectl
jq
yq
curl
awk
grep
```

### 运行工具
```bash
# 交互式运行
./scripts/domain12_troubleshooting_toolkit.sh

# 命令行运行特定功能
./scripts/domain12_troubleshooting_toolkit.sh full      # 完整诊断
./scripts/domain12_troubleshooting_toolkit.sh health   # 集群健康检查
./scripts/domain12_troubleshooting_toolkit.sh pods     # Pod故障诊断
```

## 🛠️ 功能模块详解

### 1. 集群健康状态检查
**功能**: 全面检查集群控制平面和节点健康状态
**输出文件**: `cluster_health.txt`
**检查内容**:
- 节点状态和基本信息
- 控制平面组件运行状态
- API Server健康检查
- etcd集群健康状态

### 2. 资源使用情况分析
**功能**: 分析集群和命名空间级别的资源使用情况
**输出文件**: `resource_utilization.txt`
**分析维度**:
- 节点CPU/Memory使用率
- 命名空间资源消耗排名
- ResourceQuota使用情况
- 资源瓶颈识别

### 3. Pod故障诊断
**功能**: 识别和分类各种Pod异常状态
**输出文件**: `pod_diagnostics.txt`
**诊断类型**:
- Pending状态Pod分析
- Running但NotReady的Pod
- CrashLoopBackOff的Pod
- OOMKilled的Pod
- 最近事件分析

### 4. 网络连通性检查
**功能**: 检查集群网络组件和服务连通性
**输出文件**: `network_check.txt`
**检查项目**:
- CoreDNS运行状态
- Service和Endpoint状态
- NetworkPolicy配置
- Ingress控制器状态

### 5. 存储系统检查
**功能**: 分析持久化存储系统的健康状况
**输出文件**: `storage_check.txt`
**检查内容**:
- PV/PVC绑定状态
- StorageClass配置
- 未绑定PVC识别
- 存储后端连接状态

### 6. 安全配置审计
**功能**: 审计RBAC权限和安全配置
**输出文件**: `security_audit.txt`
**审计范围**:
- ClusterRoleBindings权限
- 过度宽松的权限配置
- Secret访问权限检查
- 默认服务账户配置

### 7. 性能瓶颈分析
**功能**: 识别系统性能瓶颈和热点
**输出文件**: `performance_analysis.txt`
**分析指标**:
- 高CPU使用率Pod
- 高内存使用率Pod
- 节点资源压力状态
- 性能优化建议

## 📊 输出结果说明

### 诊断报告结构
所有诊断结果保存在 `/tmp/domain12_diagnostics_<timestamp>/` 目录下：
```
/tmp/domain12_diagnostics_20260205_143022/
├── cluster_health.txt          # 集群健康状态
├── resource_utilization.txt    # 资源使用分析
├── pod_diagnostics.txt         # Pod故障诊断
├── network_check.txt           # 网络连通性
├── storage_check.txt           # 存储系统检查
├── security_audit.txt          # 安全配置审计
├── performance_analysis.txt    # 性能瓶颈分析
└── comprehensive_report.md     # 综合诊断报告
```

### 综合报告内容
`comprehensive_report.md` 包含：
- 集群健康摘要
- 各项检查的状态和问题数量
- 详细的诊断结果链接
- 针对性的问题解决建议

## 🎯 使用场景

### 日常巡检
```bash
# 每日例行检查
./scripts/domain12_troubleshooting_toolkit.sh full
```

### 故障应急
```bash
# 快速定位问题
./scripts/domain12_troubleshooting_toolkit.sh pods
./scripts/domain12_troubleshooting_toolkit.sh health
```

### 性能优化
```bash
# 性能瓶颈分析
./scripts/domain12_troubleshooting_toolkit.sh performance
./scripts/domain12_troubleshooting_toolkit.sh resources
```

### 安全审计
```bash
# 安全配置检查
./scripts/domain12_troubleshooting_toolkit.sh security
```

## ⚙️ 高级配置

### 环境变量配置
```bash
# 自定义输出目录
export DOMAIN12_OUTPUT_DIR="/var/log/diagnostics"

# 设置Kubernetes配置文件路径
export KUBECONFIG="/path/to/kubeconfig"

# 调整资源阈值
export CPU_THRESHOLD="80"
export MEMORY_THRESHOLD="85"
```

### 集成到CI/CD
```yaml
# 在流水线中使用
- name: kubernetes-health-check
  run: |
    ./scripts/domain12_troubleshooting_toolkit.sh full
    # 检查是否有严重问题
    if grep -q "❌" /tmp/domain12_diagnostics_*/comprehensive_report.md; then
      echo "发现严重问题，停止部署"
      exit 1
    fi
```

## 🔧 故障排除

### 常见问题

**1. 权限不足**
```bash
# 确保有足够的RBAC权限
kubectl auth can-i get nodes
kubectl auth can-i get pods --all-namespaces
```

**2. 工具依赖缺失**
```bash
# Ubuntu/Debian
apt-get install jq curl yq

# CentOS/RHEL
yum install jq curl yq
```

**3. metrics-server不可用**
```bash
# 检查metrics-server状态
kubectl get pods -n kube-system | grep metrics-server
# 如果不存在，需要部署metrics-server
```

## 📈 最佳实践

### 1. 定期执行
- 建议每天执行一次完整诊断
- 在重大变更前后执行健康检查
- 定期进行安全配置审计

### 2. 结果归档
```bash
# 自动归档诊断结果
find /tmp/domain12_diagnostics_* -mtime +7 -exec rm -rf {} \;
```

### 3. 告警集成
```bash
# 集成到监控告警系统
if grep -q "Critical" $OUTPUT_DIR/comprehensive_report.md; then
    # 发送告警通知
    curl -X POST "https://alert-system/api/alert" \
         -d "message=Kubernetes集群发现严重问题"
fi
```

## 🆘 技术支持

如遇到问题，请提供以下信息：
1. 工具版本和执行命令
2. 错误输出信息
3. 集群版本信息 (`kubectl version`)
4. 相关的诊断输出文件

---
**工具版本**: v1.0.0 | **最后更新**: 2026-02-05 | **适用环境**: Kubernetes v1.25+