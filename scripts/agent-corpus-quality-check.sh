#!/bin/bash
# KUDIG-DATABASE Agent Corpus 质量持续监控脚本
# 运行于 CI 或手动执行，检查知识图谱和文档质量指标
#
# 用法: bash scripts/agent-corpus-quality-check.sh [--full]

set -euo pipefail

BASE_DIR="$(cd "$(dirname "$0")/.." && pwd)"
KG_FILE="$BASE_DIR/.understand-anything/knowledge-graph.json"
REPORT_DIR="$BASE_DIR/reports"
mkdir -p "$REPORT_DIR"

echo "============================================================"
echo "KUDIG-DATABASE Agent Corpus 质量检查"
echo "日期: $(date +%Y-%m-%d)"
echo "============================================================"

# === 1. 文档计数 ===
echo ""
echo "=== 文档统计 ==="
TOTAL_MD=$(find "$BASE_DIR" -name "*.md" -not -path "*/.git/*" -not -path "*/node_modules/*" | wc -l | tr -d ' ')
DOMAIN_COUNT=$(find "$BASE_DIR" -maxdepth 1 -type d -name "domain-*" | wc -l | tr -d ' ')
TOPIC_COUNT=$(find "$BASE_DIR" -maxdepth 1 -type d -name "topic-*" | wc -l | tr -d ' ')
MOC_COUNT=$(find "$BASE_DIR" -name "MOC.md" -not -path "*/.git/*" | wc -l | tr -d ' ')

echo "  Markdown 文档总数: $TOTAL_MD"
echo "  Domain 数量:       $DOMAIN_COUNT"
echo "  Topic 数量:        $TOPIC_COUNT"
echo "  MOC 导航页:        $MOC_COUNT"

# === 2. Frontmatter 检查 ===
echo ""
echo "=== Frontmatter 质量 ==="
NO_FM=0
MISSING_TAGS=0
MISSING_AUTHORS=0
MISSING_K8S=0

while IFS= read -r file; do
    fname=$(basename "$file")
    if [[ "$fname" == "README.md" ]] || [[ "$fname" == "MOC.md" ]]; then
        continue
    fi
    content=$(head -5 "$file" 2>/dev/null || true)
    if [[ ! "$content" =~ ^--- ]]; then
        NO_FM=$((NO_FM + 1))
        continue
    fi
    # Check for required fields in first 80 lines
    fm_block=$(head -80 "$file" | sed -n '/^---$/,/^---$/p' | head -60)
    if ! echo "$fm_block" | grep -q "^tags:"; then
        MISSING_TAGS=$((MISSING_TAGS + 1))
    fi
    if ! echo "$fm_block" | grep -q "^authors:"; then
        MISSING_AUTHORS=$((MISSING_AUTHORS + 1))
    fi
    if ! echo "$fm_block" | grep -q "^k8s_versions:"; then
        MISSING_K8S=$((MISSING_K8S + 1))
    fi
done < <(find "$BASE_DIR/domain-*" "$BASE_DIR/topic-*" -name "*.md" 2>/dev/null)

echo "  无 Frontmatter:     $NO_FM"
echo "  缺失 Tags:          $MISSING_TAGS"
echo "  缺失 Authors:       $MISSING_AUTHORS"
echo "  缺失 K8s_versions:  $MISSING_K8S"

# === 3. MOC 覆盖率 ===
echo ""
echo "=== MOC 覆盖率 ==="
TOTAL_DIRS=$(find "$BASE_DIR" -maxdepth 1 -type d \( -name "domain-*" -o -name "topic-*" \) | wc -l | tr -d ' ')
MOC_DIRS=$(find "$BASE_DIR" -maxdepth 1 -type d \( -name "domain-*" -o -name "topic-*" \) -exec test -f {}/MOC.md \; -print | wc -l | tr -d ' ')
echo "  应有 MOC 目录:      $TOTAL_DIRS"
echo "  已有 MOC 目录:      $MOC_DIRS"
if [ "$TOTAL_DIRS" -gt 0 ]; then
    COVERAGE=$((MOC_DIRS * 100 / TOTAL_DIRS))
    echo "  覆盖率:             ${COVERAGE}%"
fi

# === 4. 双向链接密度 ===
echo ""
echo "=== 双向链接密度 ==="
WIKILINK_FILES=0
TOTAL_WIKILINKS=0
while IFS= read -r file; do
    count=$(grep -c '\- \[\[' "$file" 2>/dev/null || true)
    if [ "$count" -gt 0 ]; then
        WIKILINK_FILES=$((WIKILINK_FILES + 1))
        TOTAL_WIKILINKS=$((TOTAL_WIKILINKS + count))
    fi
done < <(find "$BASE_DIR" -type f \( -path "*/domain-*/*.md" -o -path "*/topic-*/*.md" \) -not -name "MOC.md" -not -name "README.md" 2>/dev/null)

echo "  有 Wikilinks 文件:  $WIKILINK_FILES"
echo "  Wikilinks 总数:     $TOTAL_WIKILINKS"
if [ "$WIKILINK_FILES" -gt 0 ]; then
    AVG=$((TOTAL_WIKILINKS / WIKILINK_FILES))
    echo "  平均每文件:          $AVG"
fi

# === 5. 知识图谱检查 (如果存在) ===
if [ -f "$KG_FILE" ]; then
    echo ""
    echo "=== 知识图谱质量 ==="
    if command -v python3 &>/dev/null; then
        python3 -c "
import json, sys
with open('$KG_FILE') as f:
    g = json.load(f)
nodes = g.get('nodes', [])
edges = g.get('edges', [])
print(f'  节点总数:     {len(nodes)}')
print(f'  边总数:       {len(edges)}')

# Orphan rate
connected = set()
for e in edges:
    connected.add(e.get('source', ''))
    connected.add(e.get('target', ''))
connected = {c for c in connected if c}
orphans = len(nodes) - len(connected)
rate = (orphans / len(nodes) * 100) if nodes else 0
print(f'  孤立节点:     {orphans}')
print(f'  孤立率:       {rate:.1f}%')

# Edge type count
edge_types = set()
for e in edges:
    t = e.get('type', '')
    if t:
        edge_types.add(t)
types_list = ', '.join(sorted(edge_types))
print(f'  边类型数:     {len(edge_types)} ({types_list})')

# Layer coverage
layered = sum(1 for n in nodes if n.get('layer'))
print(f'  已分层节点:   {layered}/{len(nodes)}')
if nodes:
    print(f'  层覆盖率:     {layered/len(nodes)*100:.1f}%')
" 2>/dev/null || echo "  (图谱解析失败)"
    fi
fi

# === 6. 阈值检查 ===
echo ""
echo "=== 质量阈值 ==="
PASS=0
FAIL=0

check() {
    local name="$1" value="$2" threshold="$3" op="$4"
    if [ "$op" = "lt" ] && [ "$value" -lt "$threshold" ]; then
        echo "  [PASS] $name: $value < $threshold"
        PASS=$((PASS + 1))
    elif [ "$op" = "gt" ] && [ "$value" -gt "$threshold" ]; then
        echo "  [PASS] $name: $value > $threshold"
        PASS=$((PASS + 1))
    elif [ "$op" = "eq" ] && [ "$value" -eq "$threshold" ]; then
        echo "  [PASS] $name: $value == $threshold"
        PASS=$((PASS + 1))
    else
        echo "  [FAIL] $name: $value (threshold: $op $threshold)"
        FAIL=$((FAIL + 1))
    fi
}

if [ "$MOC_DIRS" -gt 0 ] && [ "$TOTAL_DIRS" -gt 0 ]; then
    check "MOC 覆盖率" "$MOC_DIRS" "$((TOTAL_DIRS - 1))" "gt"
fi
check "Frontmatter 缺失" "$NO_FM" "50" "lt"
check "Wikilinks 覆盖" "$WIKILINK_FILES" "500" "gt"

echo ""
echo "  通过: $PASS | 失败: $FAIL"

# === Summary ===
echo ""
echo "============================================================"
echo "质量检查完成"
echo "============================================================"
