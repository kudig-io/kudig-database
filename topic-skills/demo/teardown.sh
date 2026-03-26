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

if kind get clusters 2>/dev/null | grep -q "^${CLUSTER_NAME}$"; then
    echo -e "${YELLOW}正在删除集群 / Deleting cluster: ${CLUSTER_NAME}${NC}"
    kind delete cluster --name "${CLUSTER_NAME}"
    echo -e "${GREEN}✓ 集群已删除 / Cluster deleted${NC}"
else
    echo -e "${YELLOW}集群 '${CLUSTER_NAME}' 不存在 / Cluster does not exist${NC}"
fi

echo ""
echo -e "${GREEN}✅ 清理完成 / Cleanup complete${NC}"
