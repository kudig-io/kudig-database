#!/bin/bash
# Wave 1 — P2 核心补齐批量生成脚本
# 用法: cd domain-10-troubleshooting-diagnostics/topic-skills && bash scripts/batch-generate-wave1.sh

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SKILL_DIR="$(dirname "$SCRIPT_DIR")"
PROJECT_ROOT="$(cd "$SKILL_DIR/../.." && pwd)"

echo "🚀 开始批量生成 Wave 1 P2 Skill..."
echo "   项目根目录: $PROJECT_ROOT"
echo "   Skill 目录: $SKILL_DIR"
echo ""

cd "$PROJECT_ROOT"

# Skill 26: Helm Chart 故障
echo "📦 生成 Skill 26: Helm Chart 部署与回滚故障..."
python3 "$SCRIPT_DIR/skill-generator.py" \
  --id SKILL-HELM-001 \
  --name "Helm Chart 部署与回滚故障诊断" \
  --number 26 \
  --category helm \
  --severity P2 \
  --mode L1 \
  --source-fta "skills/helm-fta.md" \
  --source-structural "domain-10-troubleshooting-diagnostics/36-helm-chart-troubleshooting.md" \
  --tags "helm,chart,deployment,rollback" \
  --keywords "helm,chart,release,rollback,upgrade failed,template error" \
  --output "$SKILL_DIR/26-helm-chart-failure.md"

# Skill 27: Service Mesh 故障
echo "🌐 生成 Skill 27: Istio/ASM Service Mesh 故障..."
python3 "$SCRIPT_DIR/skill-generator.py" \
  --id SKILL-MESH-001 \
  --name "Istio ASM Service Mesh 故障诊断" \
  --number 27 \
  --category service-mesh \
  --severity P2 \
  --mode L2 \
  --source-fta "skills/service-mesh-istio-fta.md" \
  --source-structural "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/05-workloads/05-service-mesh-istio-troubleshooting.md" \
  --tags "istio,asm,envoy,sidecar,mtls" \
  --keywords "istio,sidecar,envoy,xds,mtls,traffic routing,503" \
  --output "$SKILL_DIR/27-service-mesh-failure.md"

# Skill 28: Webhook Admission 故障
echo "🔒 生成 Skill 28: Admission Webhook 故障..."
python3 "$SCRIPT_DIR/skill-generator.py" \
  --id SKILL-WEBHOOK-001 \
  --name "Admission Webhook 超时与拒绝故障诊断" \
  --number 28 \
  --category webhook \
  --severity P2 \
  --mode L2 \
  --source-fta "skills/webhook-admission-fta.md" \
  --tags "webhook,admission,mutating,validating,timeout" \
  --keywords "webhook,timeout,Internal error, admission webhook,mutating,validating" \
  --output "$SKILL_DIR/28-webhook-admission-failure.md"

# Skill 29: CRD/Operator 故障
echo "⚙️  生成 Skill 29: CRD/Operator Reconcile 失败..."
python3 "$SCRIPT_DIR/skill-generator.py" \
  --id SKILL-OPERATOR-001 \
  --name "CRD Operator Reconcile 失败诊断" \
  --number 29 \
  --category operator \
  --severity P2 \
  --mode L1 \
  --source-fta "skills/crd-operator-fta.md" \
  --source-structural "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/08-cluster-operations/05-crd-operator-troubleshooting.md" \
  --tags "crd,operator,reconcile,controller,olm" \
  --keywords "operator,reconcile failed,crd,controller,finalizer,stuck" \
  --output "$SKILL_DIR/29-crd-operator-failure.md"

# Skill 30: Backup/Restore 故障
echo "💾 生成 Skill 30: Velero etcd 备份恢复故障..."
python3 "$SCRIPT_DIR/skill-generator.py" \
  --id SKILL-BACKUP-001 \
  --name "Velero etcd 备份恢复失败诊断" \
  --number 30 \
  --category backup \
  --severity P2 \
  --mode L3 \
  --source-fta "skills/backup-restore-fta.md" \
  --source-structural "domain-10-troubleshooting-diagnostics/31-backup-restore-troubleshooting.md" \
  --tags "velero,etcd,backup,restore,snapshot,dr" \
  --keywords "backup failed,restore failed,velero,snapshot,etcd,dr" \
  --output "$SKILL_DIR/30-backup-restore-failure.md"

# Skill 31: GPU/AI Workloads 故障
echo "🧠 生成 Skill 31: GPU AI Workloads 故障..."
python3 "$SCRIPT_DIR/skill-generator.py" \
  --id SKILL-GPU-001 \
  --name "GPU AI ML Workloads 调度与 OOM 诊断" \
  --number 31 \
  --category gpu \
  --severity P2 \
  --mode L1 \
  --source-fta "skills/gpu-fta.md" \
  --source-structural "domain-10-troubleshooting-diagnostics/topic-structural-trouble-shooting/10-ai-ml-workloads/01-ai-ml-workloads-troubleshooting.md" \
  --tags "gpu,nvidia,nccl,cuda,training,inference" \
  --keywords "gpu,oom,nvidia,nccl,cuda,out of memory,training failed" \
  --output "$SKILL_DIR/31-gpu-ai-workloads-failure.md"

echo ""
echo "✅ Wave 1 完成！已生成 6 个 Skill 文件:"
echo "   26-helm-chart-failure.md"
echo "   27-service-mesh-failure.md"
echo "   28-webhook-admission-failure.md"
echo "   29-crd-operator-failure.md"
echo "   30-backup-restore-failure.md"
echo "   31-gpu-ai-workloads-failure.md"
echo ""
echo "⚠️  下一步: 人工审核每个文件中的 [请补充] 标记，参考 01-node-notready.md 补齐内容"
echo "   完成后将状态从 'Beta' 更新为 'GA'"
