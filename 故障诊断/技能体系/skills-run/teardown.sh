#!/usr/bin/env bash
# ============================================================================
# teardown.sh — 清理 Skills Demo 环境
# Clean up Skills Demo environment
# ============================================================================

set -euo pipefail

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
CYAN='\033[0;36m'
NC='\033[0m'

CLUSTER_NAME="${CLUSTER_NAME:-skill-demo}"

echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
echo -e "${CYAN}║      🧹 Skills Demo — Teardown                             ║${NC}"
echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
echo ""

# 检查 kind 是否安装
if ! command -v kind &>/dev/null; then
    echo -e "${RED}✗ kind 未安装 / kind is not installed${NC}"
    exit 1
fi

# 检查 kubectl 是否安装并获取当前 context
CURRENT_CTX="N/A"
if command -v kubectl &>/dev/null; then
    CURRENT_CTX=$(kubectl config current-context 2>/dev/null || echo "N/A")
fi

echo -e "  目标集群 / Target: ${CYAN}${CLUSTER_NAME}${NC}"
echo -e "  当前 context: ${CYAN}${CURRENT_CTX}${NC}"
echo ""

# 安全提示: 如果当前 context 不是 kind- 开头的，提醒用户
if [[ "${CURRENT_CTX}" != kind-* && "${CURRENT_CTX}" != "N/A" ]]; then
    echo -e "  ${YELLOW}⚠ 警告: 当前 context '${CURRENT_CTX}' 不是 Kind 集群${NC}"
    echo -e "  ${YELLOW}  teardown.sh 仅删除 Kind 集群，不会触碰生产集群${NC}"
    echo ""
fi

if kind get clusters 2>/dev/null | grep -q "^${CLUSTER_NAME}$"; then
    read -rp "Delete Kind cluster '${CLUSTER_NAME}'? [y/N] " answer
    if [[ "${answer,,}" == "y" ]]; then
        echo -e "${YELLOW}正在删除集群 / Deleting cluster: ${CLUSTER_NAME}${NC}"
        kind delete cluster --name "${CLUSTER_NAME}"
        echo -e "${GREEN}✓ 集群已删除 / Cluster deleted${NC}"
    else
        echo -e "${YELLOW}已取消删除 / Deletion cancelled${NC}"
        exit 0
    fi
else
    echo -e "${YELLOW}集群 '${CLUSTER_NAME}' 不存在 / Cluster does not exist${NC}"
fi

echo ""
echo -e "${GREEN}✅ 清理完成 / Cleanup complete${NC}"
