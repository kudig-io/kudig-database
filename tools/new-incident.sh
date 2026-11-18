#!/usr/bin/env bash
# new-incident.sh — 从快速记录模板创建新故障案例
#
# 用法:
#   ./tools/new-incident.sh <领域目录> <YYYY-MM-DD> <公司slug> <标题slug>
#
# 示例:
#   ./tools/new-incident.sh cloud-infrastructure 2026-07-01 example-cloud api-outage
#   生成: incidents/cloud-infrastructure/2026-07-01-example-cloud-api-outage.md
#
# 生成的文件为 draft 状态（快速占位模板），信息齐全后按
# templates/incident-report.md 的完整结构补全，并将 status 改为 published。

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
TEMPLATE="$ROOT/templates/quick-entry.md"

usage() {
  sed -n '2,12p' "$0" | sed 's/^# \{0,1\}//'
  exit "${1:-0}"
}

[[ $# -eq 4 ]] || usage 1

DOMAIN="$1" DATE="$2" COMPANY="$3" SLUG="$4"

# 校验领域目录（枚举权威来源：docs/severity-and-taxonomy.md）
VALID_DOMAINS="cloud-infrastructure cdn-edge networking-dns database-storage \
container-orchestration ai-ml-services security-services saas-platforms \
messaging-streaming observability identity-access"
if [[ " $VALID_DOMAINS " != *" $DOMAIN "* ]]; then
  echo "错误: 非法领域目录 '$DOMAIN'" >&2
  echo "合法取值: $VALID_DOMAINS" >&2
  exit 1
fi

# 校验日期格式与合法性
if ! [[ "$DATE" =~ ^[0-9]{4}-[0-9]{2}-[0-9]{2}$ ]]; then
  echo "错误: 日期格式必须为 YYYY-MM-DD，收到 '$DATE'" >&2
  exit 1
fi
if ! date -j -f '%Y-%m-%d' "$DATE" '+%Y-%m-%d' >/dev/null 2>&1 \
   && ! date -d "$DATE" '+%Y-%m-%d' >/dev/null 2>&1; then
  echo "错误: '$DATE' 不是合法日期" >&2
  exit 1
fi

# 校验 slug（全小写字母数字连字符）
for s in "$COMPANY" "$SLUG"; do
  if ! [[ "$s" =~ ^[a-z0-9]+(-[a-z0-9]+)*$ ]]; then
    echo "错误: slug 必须为全小写字母/数字/连字符，收到 '$s'" >&2
    exit 1
  fi
done

[[ -f "$TEMPLATE" ]] || { echo "错误: 模板不存在 $TEMPLATE" >&2; exit 1; }

DEST_DIR="$ROOT/incidents/$DOMAIN"
DEST="$DEST_DIR/$DATE-$COMPANY-$SLUG.md"
if [[ -e "$DEST" ]]; then
  echo "错误: 文件已存在，拒绝覆盖: ${DEST#"$ROOT"/}" >&2
  exit 1
fi
mkdir -p "$DEST_DIR"

INC_ID="INC-$(echo "$DATE" | tr -d '-')-$(echo "$COMPANY" | tr '[:lower:]' '[:upper:]')"
TODAY="$(date '+%Y-%m-%d')"

# 基于模板填充元数据骨架
sed \
  -e "s|^id: .*|id: $INC_ID|" \
  -e "s|^company: .*|company: $COMPANY|" \
  -e "s|^domain: .*|domain: $DOMAIN|" \
  -e "s|^date: .*|date: $DATE|" \
  -e "s|^last_updated: .*|last_updated: $TODAY|" \
  "$TEMPLATE" > "$DEST"

echo "已创建: ${DEST#"$ROOT"/}"
echo "  id: $INC_ID (status: draft)"
echo "下一步:"
echo "  1. 补全 Frontmatter（title/company_type/severity/root_cause_category/sources 等）"
echo "  2. 信息齐全后按 templates/incident-report.md 补全正文，status 改为 published"
echo "  3. 运行 python3 tools/build-index.py 校验并重建索引"
