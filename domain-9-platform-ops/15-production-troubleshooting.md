# 生产环境故障诊断 (Production Troubleshooting)

> **适用版本**: Kubernetes v1.25 - v1.32 | **文档版本**: v1.0 | **最后更新**: 2026-02
> **目标读者**: SRE团队、故障处理工程师、运维人员

## 概述

生产环境故障诊断是保障业务连续性的关键能力。本文档提供系统性的故障诊断方法论、常见问题排查技巧和自动化诊断工具，帮助运维团队快速定位和解决生产环境问题。

## 故障诊断体系架构

```
┌─────────────────────────────────────────────────────────────────────────────┐
│                          生产环境故障诊断体系                                │
├─────────────────────────────────────────────────────────────────────────────┤
│                                                                             │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │
│  │   问题发现      │  │   信息收集      │  │   根因分析      │            │
│  │                 │  │                 │  │                 │            │
│  │ • 监控告警      │  │ • 日志收集      │  │ • 关联分析      │            │
│  │ • 用户反馈      │  │ • 指标分析      │  │ • 模式识别      │            │
│  │ • 自动检测      │  │ • 链路追踪      │  │ • 假设验证      │            │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘            │
│          │                     │                     │                     │
│          ▼                     ▼                     ▼                     │
│  ┌─────────────────┐  ┌─────────────────┐  ┌─────────────────┐            │
│  │   解决方案      │  │   验证修复      │  │   复盘总结      │            │
│  │                 │  │                 │  │                 │            │
│  │ • 临时缓解      │  │ • 效果确认      │  │ • 根因归档      │            │
│  │ • 根本解决      │  │ • 回归测试      │  │ • 预防措施      │            │
│  │ • 预防机制      │  │ • 监控加强      │  │ • 知识沉淀      │            │
│  └─────────────────┘  └─────────────────┘  └─────────────────┘            │
│                                                                             │
└─────────────────────────────────────────────────────────────────────────────┘
```

## 系统性故障诊断方法

### 1. 黄金信号诊断法
```yaml
golden_signals_troubleshooting:
  latency_issues:
    diagnosis_steps:
      - check_service_mesh_metrics: "检查服务网格延迟"
      - analyze_application_logs: "分析应用日志中的慢查询"
      - examine_network_latency: "检查网络延迟"
      - review_resource_limits: "检查CPU/内存限制"
      
  traffic_issues:
    diagnosis_steps:
      - verify_load_balancer_config: "验证负载均衡配置"
      - check_service_discovery: "检查服务发现问题"
      - analyze_ingress_controller: "分析Ingress控制器状态"
      - review_autoscaling_settings: "检查自动扩缩容配置"
      
  error_issues:
    diagnosis_steps:
      - examine_error_logs: "检查错误日志"
      - analyze_return_codes: "分析返回码分布"
      - check_dependency_services: "检查依赖服务状态"
      - review_security_policies: "检查安全策略影响"
      
  saturation_issues:
    diagnosis_steps:
      - monitor_resource_utilization: "监控资源使用率"
      - check_pending_workloads: "检查待处理工作负载"
      - analyze_queue_depth: "分析队列深度"
      - review_capacity_planning: "检查容量规划"
```

### 2. 分层诊断方法
```bash
#!/bin/bash
# 分层故障诊断脚本

layered_troubleshooting() {
    local service_name=$1
    local namespace=${2:-default}
    
    echo "=== 分层故障诊断: $service_name ==="
    
    # 第一层: 基础设施层检查
    echo "1. 基础设施层诊断:"
    echo "   □ 节点状态检查"
    kubectl get nodes | grep -v Ready
    echo "   □ 网络连通性检查"
    kubectl run debug-pod --image=busybox --rm -it -- ping -c 3 8.8.8.8
    
    # 第二层: Kubernetes组件层检查
    echo "2. Kubernetes组件层诊断:"
    echo "   □ API Server状态"
    kubectl get componentstatuses
    echo "   □ 控制器状态"
    kubectl get pods -n kube-system | grep -E "(controller|scheduler)"
    
    # 第三层: 应用层检查
    echo "3. 应用层诊断:"
    echo "   □ Pod状态检查"
    kubectl get pods -n $namespace | grep $service_name
    echo "   □ 服务状态检查"
    kubectl get svc -n $namespace | grep $service_name
    
    # 第四层: 业务逻辑层检查
    echo "4. 业务逻辑层诊断:"
    echo "   □ 应用日志分析"
    kubectl logs -n $namespace -l app=$service_name --tail=100
    echo "   □ 依赖服务检查"
    kubectl get endpoints -n $namespace
}

# 使用示例
layered_troubleshooting "user-service" "production"
```

## 常见故障场景及解决方案

### 1. Pod相关故障

#### Pod Pending状态
```bash
#!/bin/bash
# Pod Pending故障诊断脚本

diagnose_pending_pods() {
    local namespace=${1:-default}
    
    echo "=== Pod Pending状态诊断 ==="
    
    # 检查Pending的Pod
    pending_pods=$(kubectl get pods -n $namespace --field-selector=status.phase=Pending -o name)
    
    if [ -z "$pending_pods" ]; then
        echo "没有Pending状态的Pod"
        return
    fi
    
    echo "发现Pending Pods:"
    echo "$pending_pods"
    echo ""
    
    # 逐个分析Pending原因
    for pod in $pending_pods; do
        echo "分析Pod: $pod"
        
        # 检查事件
        echo "相关事件:"
        kubectl describe $pod -n $namespace | grep -A 10 "Events:"
        echo ""
        
        # 检查资源请求
        echo "资源请求:"
        kubectl get $pod -n $namespace -o jsonpath='{.spec.containers[*].resources}'
        echo ""
        
        # 检查节点选择器
        echo "节点选择器:"
        kubectl get $pod -n $namespace -o jsonpath='{.spec.nodeSelector}'
        echo ""
        
        echo "---"
    done
}

# 使用示例
diagnose_pending_pods "production"
```

通过系统性的故障诊断方法和工具，可以显著提升故障处理效率，减少业务中断时间，保障生产环境的稳定运行。