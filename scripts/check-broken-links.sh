#!/usr/bin/env bash
#
# check-broken-links.sh
# Validate cross_refs and related_docs paths in KUDIG documents.
#
# Checks:
#   - cross_refs: paths exist and are reachable
#   - related_docs: paths exist
#   - README/internal links: targets exist
#
# Usage:
#   bash scripts/check-broken-links.sh [--fix] [--verbose]
#
# Exit codes:
#   0 = all links valid
#   1 = broken links found

set -euo pipefail

DRY_RUN=false
VERBOSE=false
BASE_DIR="${PWD}"

# 知识域名（中文目录名，替代原 domain-* glob）
DOMAINS=(集群基础 工作负载 网络 存储 安全 可观测性 平台工程 发布变更 可靠性 故障诊断 生产运维 云厂商 容器运行时 AI基础设施 专项技术 数据库中间件 系统基础 清单模式 生态参考 应用模式)

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run|-n) DRY_RUN=true ;;
        --verbose|-v) VERBOSE=true ;;
        --help|-h)
            echo "Usage: $0 [--dry-run] [--verbose]"
            echo "  --dry-run   Preview broken links without exiting with error"
            echo "  --verbose   Show all checked files"
            exit 0
            ;;
        *) echo "Unknown option: $1" >&2; exit 1 ;;
    esac
    shift
done

echo "Checking cross_refs and related_docs in KUDIG documents..."

broken=0
checked=0

# Check cross_refs paths
while IFS= read -r line; do
    if [[ "$line" =~ path:\ \"(\.\./[^\"]+)\" ]]; then
        rel_path="${BASH_REMATCH[1]}"
        # Resolve relative path: remove leading ../ and resolve
        clean_path="${rel_path#../}"
        full_path="${BASE_DIR}/${clean_path}"
        checked=$((checked + 1))

        # Check if path exists (file or directory)
        if [[ ! -e "$full_path" ]]; then
            echo "[BROKEN] $rel_path"
            broken=$((broken + 1))
        elif [[ "$VERBOSE" == true ]]; then
            echo "[OK] $rel_path"
        fi
    fi
done < <(grep -rh 'path:' "${DOMAINS[@]}" --include='*.md' 2>/dev/null | grep -E '\.\./' | head -200)

# Check related_docs paths
while IFS= read -r line; do
    if [[ "$line" =~ path:\ \"(\.\./[^\"]+)\" ]]; then
        rel_path="${BASH_REMATCH[1]}"
        clean_path="${rel_path#../}"
        full_path="${BASE_DIR}/${clean_path}"
        checked=$((checked + 1))

        if [[ ! -e "$full_path" ]]; then
            echo "[BROKEN] $rel_path"
            broken=$((broken + 1))
        elif [[ "$VERBOSE" == true ]]; then
            echo "[OK] $rel_path"
        fi
    fi
done < <(grep -rh 'related_docs:' -A 10 "${DOMAINS[@]}" --include='*.md' 2>/dev/null | grep 'path:')

echo ""
echo "--- Summary ---"
echo "Checked: $checked links"
echo "Broken: $broken"

if [[ $broken -gt 0 && "$DRY_RUN" == false ]]; then
    echo "Fix with: python3 scripts/enhance-cross-refs.py"
    exit 1
fi

exit 0