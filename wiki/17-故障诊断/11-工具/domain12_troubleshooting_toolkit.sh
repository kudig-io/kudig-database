#!/bin/bash
#
# Domain-12 故障排查工具套件
# Kubernetes生产环境运维专家级诊断工具集合
#

set -euo pipefail

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 日志函数
log_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

log_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

log_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

log_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 配置变量
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTPUT_DIR="/tmp/domain12_diagnostics_$(date +%Y%m%d_%H%M%S)"
mkdir -p "$OUTPUT_DIR"

# 检查依赖工具
check_dependencies() {
    log_info "检查必需工具..."
    
    local deps=("kubectl" "jq" "yq" "curl" "awk" "grep")
    local missing_deps=()
    
    for dep in "${deps[@]}"; do
        if ! command -v "$dep" &> /dev/null; then
            missing_deps+=("$dep")
        fi
    done
    
    if [ ${#missing_deps[@]} -ne 0 ]; then
        log_error "缺少必需工具: ${missing_deps[*]}"
        log_info "请安装缺失的工具后再运行此脚本"
        exit 1
    fi
    
    log_success "所有依赖工具检查通过"
}

# 集群健康状态检查
cluster_health_check() {
    local output_file="$OUTPUT_DIR/cluster_health.txt"
    log_info "执行集群健康状态检查..."
    
    {
        echo "=== 集群健康状态检查报告 ==="
        echo "检查时间: $(date)"
        echo ""
        
        echo "--- 节点状态 ---"
        kubectl get nodes -o wide
        
        echo ""
        echo "--- 控制平面组件状态 ---"
        kubectl get pods -n kube-system -o wide
        
        echo ""
        echo "--- API Server健康检查 ---"
        kubectl get --raw='/healthz?verbose'
        
        echo ""
        echo "--- etcd健康检查 ---"
        ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
            --cacert=/etc/kubernetes/pki/etcd/ca.crt \
            --cert=/etc/kubernetes/pki/etcd/server.crt \
            --key=/etc/kubernetes/pki/etcd/server.key \
            endpoint health 2>/dev/null || echo "etcd健康检查失败"
            
    } > "$output_file"
    
    log_success "集群健康检查完成: $output_file"
}

# 资源使用情况分析
resource_utilization_analysis() {
    local output_file="$OUTPUT_DIR/resource_utilization.txt"
    log_info "分析资源使用情况..."
    
    {
        echo "=== 资源使用情况分析报告 ==="
        echo "分析时间: $(date)"
        echo ""
        
        echo "--- 节点资源使用 ---"
        kubectl top nodes || echo "metrics-server不可用"
        
        echo ""
        echo "--- 命名空间资源消耗排名 ---"
        kubectl get namespaces -o jsonpath='{range .items[*]}{.metadata.name}{"\n"}{end}' | \
            xargs -I {} sh -c 'echo "Namespace: {}"; kubectl top pods -n {} 2>/dev/null | tail -n +2 | awk "{sum+=\$3} END {print \"Total Memory: \" sum \"Mi\"}"' | \
            grep -E "(Namespace|Total Memory)" | \
            paste - - | sort -k4 -hr
        
        echo ""
        echo "--- 资源配额使用情况 ---"
        kubectl get resourcequota --all-namespaces -o custom-columns=NAMESPACE:.metadata.namespace,NAME:.metadata.name,USED_CPU:.status.used.requests\.cpu,LIMIT_CPU:.status.hard.requests\.cpu,USED_MEM:.status.used.requests\.memory,LIMIT_MEM:.status.hard.requests\.memory --no-headers | \
            awk '{
                if($4 > 0) cpu_util = ($3/$4)*100; else cpu_util = 0
                if($6 > 0) mem_util = ($5/$6)*100; else mem_util = 0
                printf "%s/%s - CPU: %.1f%% (%s/%s), Memory: %.1f%% (%s/%s)\n", $1, $2, cpu_util, $3, $4, mem_util, $5, $6
            }' | sort -k7 -hr
        
    } > "$output_file"
    
    log_success "资源使用分析完成: $output_file"
}

# 故障Pod诊断
troubleshooting_pod_diagnostics() {
    local output_file="$OUTPUT_DIR/pod_diagnostics.txt"
    log_info "执行Pod故障诊断..."
    
    {
        echo "=== Pod故障诊断报告 ==="
        echo "诊断时间: $(date)"
        echo ""
        
        echo "--- Pending状态的Pod ---"
        kubectl get pods --all-namespaces --field-selector=status.phase=Pending -o wide
        
        echo ""
        echo "--- Running但NotReady的Pod ---"
        kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{" "}{.metadata.name}{" "}{range .status.conditions[*]}{.type}{"="}{.status}{" "}{end}{"\n"}{end}' | \
            grep "Ready=False"
        
        echo ""
        echo "--- CrashLoopBackOff的Pod ---"
        kubectl get pods --all-namespaces | grep "CrashLoopBackOff"
        
        echo ""
        echo "--- OOMKilled的Pod ---"
        kubectl get pods --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{" "}{.metadata.name}{" "}{range .status.containerStatuses[*]}{.lastState.terminated.reason}{" "}{end}{"\n"}{end}' | \
            grep "OOMKilled"
        
        echo ""
        echo "--- 最近的Events ---"
        kubectl get events --all-namespaces --sort-by='.lastTimestamp' | tail -20
        
    } > "$output_file"
    
    log_success "Pod故障诊断完成: $output_file"
}

# 网络连通性检查
network_connectivity_check() {
    local output_file="$OUTPUT_DIR/network_check.txt"
    log_info "执行网络连通性检查..."
    
    {
        echo "=== 网络连通性检查报告 ==="
        echo "检查时间: $(date)"
        echo ""
        
        echo "--- CoreDNS状态 ---"
        kubectl get pods -n kube-system -l k8s-app=kube-dns -o wide
        
        echo ""
        echo "--- Service状态 ---"
        kubectl get services --all-namespaces -o wide
        
        echo ""
        echo "--- NetworkPolicy状态 ---"
        kubectl get networkpolicies --all-namespaces
        
        echo ""
        echo "--- Ingress控制器状态 ---"
        kubectl get pods -n ingress-nginx 2>/dev/null || echo "未找到ingress-nginx命名空间"
        
    } > "$output_file"
    
    log_success "网络连通性检查完成: $output_file"
}

# 存储系统检查
storage_system_check() {
    local output_file="$OUTPUT_DIR/storage_check.txt"
    log_info "执行存储系统检查..."
    
    {
        echo "=== 存储系统检查报告 ==="
        echo "检查时间: $(date)"
        echo ""
        
        echo "--- PV状态 ---"
        kubectl get pv -o wide
        
        echo ""
        echo "--- PVC状态 ---"
        kubectl get pvc --all-namespaces -o wide
        
        echo ""
        echo "--- StorageClass状态 ---"
        kubectl get storageclass -o wide
        
        echo ""
        echo "--- 未绑定的PVC ---"
        kubectl get pvc --all-namespaces --no-headers | awk '$3=="Pending" {print}'
        
    } > "$output_file"
    
    log_success "存储系统检查完成: $output_file"
}

# 安全配置审计
security_configuration_audit() {
    local output_file="$OUTPUT_DIR/security_audit.txt"
    log_info "执行安全配置审计..."
    
    {
        echo "=== 安全配置审计报告 ==="
        echo "审计时间: $(date)"
        echo ""
        
        echo "--- ClusterRoleBindings ---"
        kubectl get clusterrolebindings -o wide
        
        echo ""
        echo "--- Roles with wildcard permissions ---"
        kubectl get roles --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{" "}{.metadata.name}{"\n"}{range .rules[*]}{.verbs}{" "}{.resources}{"\n"}{end}{end}' | \
            grep -E "(\*|star)"
        
        echo ""
        echo "--- Secrets访问权限检查 ---"
        kubectl get rolebindings --all-namespaces -o jsonpath='{range .items[*]}{.metadata.namespace}{" "}{.metadata.name}{" "}{.roleRef.name}{"\n"}{end}' | \
            grep -i secret
        
        echo ""
        echo "--- 默认服务账户检查 ---"
        kubectl get serviceaccounts --all-namespaces | grep "default "
        
    } > "$output_file"
    
    log_success "安全配置审计完成: $output_file"
}

# 性能瓶颈分析
performance_bottleneck_analysis() {
    local output_file="$OUTPUT_DIR/performance_analysis.txt"
    log_info "执行性能瓶颈分析..."
    
    {
        echo "=== 性能瓶颈分析报告 ==="
        echo "分析时间: $(date)"
        echo ""
        
        echo "--- 高CPU使用率Pod ---"
        kubectl top pods --all-namespaces 2>/dev/null | head -20
        
        echo ""
        echo "--- 高内存使用率Pod ---"
        kubectl top pods --all-namespaces --sort-by=memory 2>/dev/null | head -20
        
        echo ""
        echo "--- 节点资源压力 ---"
        kubectl describe nodes | grep -A 5 "Conditions:" | grep -E "(MemoryPressure|DiskPressure|PIDPressure)"
        
    } > "$output_file"
    
    log_success "性能瓶颈分析完成: $output_file"
}

# 生成综合报告
generate_comprehensive_report() {
    local report_file="$OUTPUT_DIR/comprehensive_report.md"
    log_info "生成综合诊断报告..."
    
    {
        echo "# Kubernetes集群综合诊断报告"
        echo ""
        echo "**生成时间**: $(date)"
        echo "**集群版本**: $(kubectl version --short 2>/dev/null | grep Server | awk '{print $3}')"
        echo ""
        
        echo "## 摘要"
        echo ""
        echo "| 检查项目 | 状态 | 发现问题数量 |"
        echo "|---------|------|-------------|"
        echo "| 集群健康 | $(if [ -s "$OUTPUT_DIR/cluster_health.txt" ]; then echo "✅ 正常"; else echo "❌ 异常"; fi) | $(grep -c "NotReady\|Unhealthy" "$OUTPUT_DIR/cluster_health.txt" 2>/dev/null || echo "0") |"
        echo "| 资源使用 | $(if [ -s "$OUTPUT_DIR/resource_utilization.txt" ]; then echo "✅ 正常"; else echo "❌ 异常"; fi) | $(grep -c "%" "$OUTPUT_DIR/resource_utilization.txt" 2>/dev/null | awk '{if($1>80) print "高使用率"; else print "正常"}' || echo "未知") |"
        echo "| Pod状态 | $(if [ -s "$OUTPUT_DIR/pod_diagnostics.txt" ]; then echo "✅ 正常"; else echo "❌ 异常"; fi) | $(grep -c "Pending\|CrashLoopBackOff\|OOMKilled" "$OUTPUT_DIR/pod_diagnostics.txt" 2>/dev/null || echo "0") |"
        echo "| 网络连通 | $(if [ -s "$OUTPUT_DIR/network_check.txt" ]; then echo "✅ 正常"; else echo "❌ 异常"; fi) | $(grep -c "Error\|Failed" "$OUTPUT_DIR/network_check.txt" 2>/dev/null || echo "0") |"
        
        echo ""
        echo "## 详细诊断结果"
        echo ""
        echo "### 1. 集群健康状态"
        echo "详见: [$OUTPUT_DIR/cluster_health.txt](file://$OUTPUT_DIR/cluster_health.txt)"
        echo ""
        echo "### 2. 资源使用情况"
        echo "详见: [$OUTPUT_DIR/resource_utilization.txt](file://$OUTPUT_DIR/resource_utilization.txt)"
        echo ""
        echo "### 3. Pod故障诊断"
        echo "详见: [$OUTPUT_DIR/pod_diagnostics.txt](file://$OUTPUT_DIR/pod_diagnostics.txt)"
        echo ""
        echo "### 4. 网络连通性"
        echo "详见: [$OUTPUT_DIR/network_check.txt](file://$OUTPUT_DIR/network_check.txt)"
        echo ""
        echo "### 5. 存储系统"
        echo "详见: [$OUTPUT_DIR/storage_check.txt](file://$OUTPUT_DIR/storage_check.txt)"
        echo ""
        echo "### 6. 安全配置"
        echo "详见: [$OUTPUT_DIR/security_audit.txt](file://$OUTPUT_DIR/security_audit.txt)"
        echo ""
        echo "### 7. 性能分析"
        echo "详见: [$OUTPUT_DIR/performance_analysis.txt](file://$OUTPUT_DIR/performance_analysis.txt)"
        
        echo ""
        echo "## 建议措施"
        echo ""
        echo "1. **立即处理**: 优先解决状态为'NotReady'的节点和'CrashLoopBackOff'的Pod"
        echo "2. **资源优化**: 对使用率超过80%的资源进行扩容或优化"
        echo "3. **安全加固**: 审查和收紧过度宽松的RBAC权限"
        echo "4. **性能调优**: 针对高资源消耗的应用进行性能分析和优化"
        echo "5. **定期巡检**: 建议每周执行一次完整的集群健康检查"
        
    } > "$report_file"
    
    log_success "综合报告生成完成: $report_file"
}

# 主菜单
show_menu() {
    echo ""
    echo "=========================================="
    echo "  Domain-12 Kubernetes 故障排查工具套件  "
    echo "=========================================="
    echo "1. 集群健康状态检查"
    echo "2. 资源使用情况分析"
    echo "3. Pod故障诊断"
    echo "4. 网络连通性检查"
    echo "5. 存储系统检查"
    echo "6. 安全配置审计"
    echo "7. 性能瓶颈分析"
    echo "8. 执行完整诊断套件"
    echo "9. 退出"
    echo "=========================================="
}

# 执行完整诊断套件
run_full_diagnostics() {
    log_info "开始执行完整诊断套件..."
    
    cluster_health_check
    resource_utilization_analysis
    troubleshooting_pod_diagnostics
    network_connectivity_check
    storage_system_check
    security_configuration_audit
    performance_bottleneck_analysis
    generate_comprehensive_report
    
    log_success "完整诊断套件执行完成!"
    log_info "诊断结果保存在: $OUTPUT_DIR"
    log_info "综合报告: $OUTPUT_DIR/comprehensive_report.md"
}

# 主程序
main() {
    check_dependencies
    
    if [ $# -eq 0 ]; then
        # 交互模式
        while true; do
            show_menu
            read -p "请选择操作 [1-9]: " choice
            
            case $choice in
                1) cluster_health_check ;;
                2) resource_utilization_analysis ;;
                3) troubleshooting_pod_diagnostics ;;
                4) network_connectivity_check ;;
                5) storage_system_check ;;
                6) security_configuration_audit ;;
                7) performance_bottleneck_analysis ;;
                8) run_full_diagnostics ;;
                9) 
                    log_info "感谢使用Domain-12故障排查工具套件!"
                    exit 0
                    ;;
                *)
                    log_warning "无效选择，请重新输入"
                    ;;
            esac
            
            echo ""
            read -p "按回车键继续..."
        done
    else
        # 命令行模式
        case $1 in
            health) cluster_health_check ;;
            resources) resource_utilization_analysis ;;
            pods) troubleshooting_pod_diagnostics ;;
            network) network_connectivity_check ;;
            storage) storage_system_check ;;
            security) security_configuration_audit ;;
            performance) performance_bottleneck_analysis ;;
            full) run_full_diagnostics ;;
            *)
                echo "用法: $0 [health|resources|pods|network|storage|security|performance|full]"
                exit 1
                ;;
        esac
    fi
}

# 脚本入口
main "$@"