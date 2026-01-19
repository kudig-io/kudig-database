#!/bin/bash
# Kusheet 表格文件重新编号脚本
# 用途：按主题段落重新组织文件编号
# 使用前请先备份：cp -r tables tables_backup

set -e

TABLES_DIR="./tables"
TEMP_DIR="./tables_temp"

# 创建临时目录
mkdir -p "$TEMP_DIR"

echo "=== Kusheet 文件重组脚本 ==="
echo "源目录: $TABLES_DIR"
echo "临时目录: $TEMP_DIR"
echo ""

# 定义重命名映射 (旧编号 -> 新编号)
# 格式: OLD_NUM:NEW_NUM:filename

declare -a MAPPING=(
  # ========== 001-019: 基础架构与核心概念 ==========
  "01:001:kubernetes-architecture"
  "02:002:core-components"
  "03:003:features-and-api"
  "04:004:code-structure"
  "11:005:upgrade-paths"
  "29:006:admission-controllers"
  "30:007:etcd-operations"
  "31:008:crd-operator"
  "32:009:api-aggregation"
  "68:010:api-priority-fairness"
  "69:011:lease-leader-election"
  "93:012:apiserver-tuning"
  
  # ========== 020-039: 工作负载与容器 ==========
  "35:020:workload-controllers"
  "37:021:pod-lifecycle-events"
  "39:022:container-runtime"
  "51:023:container-images"
  "70:024:runtime-class"
  "74:025:container-lifecycle"
  "75:026:sidecar-containers"
  "135:027:advanced-pod-patterns"
  "54:028:windows-containers"
  
  # ========== 040-079: 网络与服务 ==========
  "12:040:network-components"
  "76:041:cni-plugins-comparison"
  "78:042:network-policy-advanced"
  "80:043:multi-cluster-networking"
  "83:044:network-encryption-mtls"
  "85:045:egress-traffic-management"
  "121:046:service-concepts-and-types"
  "77:047:service-implementation"
  "72:048:service-topology"
  "122:049:service-discovery-and-dns"
  "33:050:dns-service-discovery"
  "125:051:coredns-architecture-and-principles"
  "126:052:coredns-configuration-and-corefile"
  "82:053:dns-optimization"
  "123:054:kube-proxy-modes-and-performance"
  "124:055:service-advanced-features-and-use-cases"
  "38:056:ingress-api-gateway"
  "71:057:gateway-api"
  "79:058:ingress-controller-config"
  "127:059:ingress-fundamentals"
  "128:060:ingress-controller-deep-dive"
  "129:061:nginx-ingress-complete-guide"
  "130:062:ingress-tls-certificate-management"
  "131:063:ingress-advanced-routing"
  "132:064:ingress-security-hardening"
  "133:065:ingress-monitoring-troubleshooting"
  "134:066:ingress-production-best-practices"
  "20:067:service-mesh"
  "49:068:service-mesh-advanced"
  
  # ========== 080-099: 存储 ==========
  "13:080:storage"
  "89:081:pv-pvc-troubleshooting"
  "95:082:storage-performance-tuning"
  
  # ========== 100-119: 安全与合规 ==========
  "09:100:security-best-practices"
  "42:101:rbac-matrix"
  "43:102:image-security-scan"
  "50:103:policy-engines"
  "66:104:pod-security-standards"
  "67:105:certificate-management"
  "99:106:security-hardening"
  "23:107:compliance-certification"
  "100:108:compliance-audit"
  "102:109:secret-management-tools"
  "104:110:security-scanning-tools"
  "105:111:policy-validation-tools"
  
  # ========== 120-139: 可观测性与监控 ==========
  "07:120:monitoring-metrics"
  "17:121:logging-auditing"
  "40:122:custom-metrics"
  "73:123:events-audit-logs"
  "106:124:observability-tools"
  "107:125:log-aggregation-tools"
  
  # ========== 140-159: 调度与资源管理 ==========
  "27:140:node-management"
  "28:141:scheduler-config"
  "91:142:kubelet-configuration"
  "16:143:resource-management"
  "19:144:multi-tenancy"
  "06:145:configuration-parameters"
  "34:146:configmap-secret"
  "90:147:cluster-capacity-planning"
  "92:148:hpa-vpa-autoscaling"
  "10:149:scaling-performance"
  "22:150:cost-optimization"
  "53:151:cost-management"
  "45:152:green-computing"
  
  # ========== 160-179: 备份与容灾 ==========
  "18:160:backup-recovery"
  "96:161:backup-restore"
  "41:162:disaster-recovery"
  # 注意: 97-disaster-recovery 与 41 重复，合并或删除
  
  # ========== 180-199: 开发工具与扩展 ==========
  "05:180:kubectl-commands"
  "14:181:addons-extensions"
  "46:182:client-libraries"
  "47:183:helm-charts"
  "48:184:gitops-workflow"
  "21:185:cicd-pipelines"
  "101:186:package-management-tools"
  "103:187:image-build-tools"
  "110:188:cli-enhancement-tools"
  "52:189:chaos-engineering"
  
  # ========== 190-199: 多集群与高级特性 ==========
  "44:190:federated-cluster"
  "55:191:virtual-clusters"
  "98:192:multi-cluster-management"
  "24:193:edge-computing"
  "15:194:alibaba-cloud-integration"
  
  # ========== 200-249: AI/LLM 专题 ==========
  "25:200:ai-ml-workloads"
  "26:201:gpu-scheduling"
  "111:202:ai-infrastructure"
  "112:203:distributed-training-frameworks"
  "114:204:gpu-monitoring"
  "115:205:ai-data-pipeline"
  "117:206:ai-experiment-management"
  "118:207:automl-hyperparameter-tuning"
  "56:210:llm-data-pipeline"
  "57:211:llm-finetuning"
  "58:212:llm-inference-serving"
  "59:213:vector-database-rag"
  "60:214:llm-quantization"
  "61:215:multimodal-models"
  "113:216:model-registry"
  "116:217:llm-serving-architecture"
  "62:220:llm-privacy-security"
  "63:221:llm-cost-monitoring"
  "64:222:llm-model-versioning"
  "65:223:llm-observability"
  "119:224:ai-security-model-protection"
  "120:225:ai-cost-analysis-finops"
  
  # ========== 250-269: 故障排查 ==========
  "08:250:troubleshooting"
  "36:251:cluster-health-check"
  "81:252:network-troubleshooting"
  "86:253:pod-pending-diagnosis"
  "87:254:node-notready-diagnosis"
  "88:255:oom-memory-diagnosis"
  "108:256:troubleshooting-tools"
  "109:257:performance-profiling-tools"
  
  # ========== 270-289: 性能调优 ==========
  "84:270:network-performance-tuning"
  # 注意: 94-network-performance-tuning 与 84 重复，合并或删除
)

# 执行重命名
echo "开始重命名文件..."
for item in "${MAPPING[@]}"; do
  OLD_NUM=$(echo "$item" | cut -d: -f1)
  NEW_NUM=$(echo "$item" | cut -d: -f2)
  NAME=$(echo "$item" | cut -d: -f3)
  
  OLD_FILE="$TABLES_DIR/${OLD_NUM}-${NAME}.md"
  NEW_FILE="$TEMP_DIR/${NEW_NUM}-${NAME}.md"
  
  if [ -f "$OLD_FILE" ]; then
    cp "$OLD_FILE" "$NEW_FILE"
    echo "  $OLD_NUM -> $NEW_NUM: $NAME"
  else
    echo "  [SKIP] 文件不存在: $OLD_FILE"
  fi
done

echo ""
echo "=== 重命名完成 ==="
echo "新文件位于: $TEMP_DIR"
echo ""
echo "下一步操作:"
echo "1. 检查 $TEMP_DIR 中的文件"
echo "2. 确认无误后执行: rm -rf $TABLES_DIR && mv $TEMP_DIR $TABLES_DIR"
echo "3. 更新 README.md 中的所有链接"
echo ""
echo "需要删除的重复文件:"
echo "  - 94-network-performance-tuning.md (与84重复)"
echo "  - 97-disaster-recovery.md (与41重复)"
