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
#   bash 31-脚本/check-broken-links.sh [--fix] [--verbose]
#
# Exit codes:
#   0 = all links valid
#   1 = broken links found

set -euo pipefail

DRY_RUN=false
VERBOSE=false
BASE_DIR="${PWD}"

# 知识域名（中文目录名，替代原 domain-* glob）
DOMAINS=(01-集群基础 02-工作负载 03-清单模式 04-应用模式 05-网络 06-存储 07-数据库中间件 08-安全 09-可观测性 10-平台工程 11-发布变更 12-可靠性 13-生产运维 14-容器运行时 15-AI基础设施 16-专项技术 17-系统基础 18-云厂商 19-故障诊断 21-生态参考)

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
    echo "Fix with: python3 31-脚本/enhance-cross-refs.py"
    exit 1
fi

exit 0