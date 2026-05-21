#!/bin/bash
# KUDIG-DATABASE MOC 自动更新脚本
# 重新生成所有 Domain/Topic MOC 和 Global MOC
#
# 用法: bash scripts/update-mocs.sh
# 通常在 git hook 或 CI 中调用

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
BASE_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"

echo "============================================================"
echo "MOC 自动更新"
echo "日期: $(date +%Y-%m-%d)"
echo "============================================================"

# Regenerate all MOCs
python3 "$SCRIPT_DIR/generate-mocs.py"

echo ""
echo "MOC 更新完成"
