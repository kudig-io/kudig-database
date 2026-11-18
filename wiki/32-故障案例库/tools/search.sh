#!/usr/bin/env bash
# search.sh — kuba-database 多维度故障案例检索
#
# 用法:
#   ./tools/search.sh "BGP"                        # 全文关键词检索
#   ./tools/search.sh --company cloudflare         # 按公司检索（不区分大小写）
#   ./tools/search.sh --domain database-storage    # 按技术领域检索
#   ./tools/search.sh --cause change-management    # 按根因分类检索
#   ./tools/search.sh --severity SEV-1             # 按严重等级检索
#   ./tools/search.sh --severity SEV-1 "DNS"       # 过滤器与关键词可任意组合
#
# 过滤器基于 YAML Frontmatter 精确匹配，关键词对全文做不区分大小写匹配。

set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
INCIDENTS_DIR="$ROOT/incidents"

usage() {
  sed -n '2,13p' "$0" | sed 's/^# \{0,1\}//'
  exit "${1:-0}"
}

COMPANY="" DOMAIN="" CAUSE="" SEVERITY="" KEYWORD=""

while [[ $# -gt 0 ]]; do
  case "$1" in
    --company)  COMPANY="${2:?--company 需要参数}";  shift 2 ;;
    --domain)   DOMAIN="${2:?--domain 需要参数}";    shift 2 ;;
    --cause)    CAUSE="${2:?--cause 需要参数}";      shift 2 ;;
    --severity) SEVERITY="${2:?--severity 需要参数}"; shift 2 ;;
    -h|--help)  usage ;;
    --*)        echo "未知选项: $1" >&2; usage 1 ;;
    *)          KEYWORD="$1"; shift ;;
  esac
done

if [[ -z "$COMPANY$DOMAIN$CAUSE$SEVERITY$KEYWORD" ]]; then
  usage 1
fi

# 提取 Frontmatter 中某字段的值（首个匹配）
fm_value() { # $1=file $2=key
  awk -F': ' -v key="$2" '
    NR==1 && $0!="---" { exit }
    NR>1 && $0=="---"  { exit }
    $1==key            { print $2; exit }
  ' "$1"
}

# 收集候选文件（--domain 直接缩小目录范围）
if [[ -n "$DOMAIN" ]]; then
  [[ -d "$INCIDENTS_DIR/$DOMAIN" ]] || { echo "领域目录不存在: incidents/$DOMAIN（见 docs/severity-and-taxonomy.md）" >&2; exit 1; }
  search_path="$INCIDENTS_DIR/$DOMAIN"
else
  search_path="$INCIDENTS_DIR"
fi

matches=0
while IFS= read -r file; do
  # Frontmatter 过滤
  if [[ -n "$COMPANY" ]]; then
    fm="$(fm_value "$file" company)"
    [[ "$(echo "$fm" | tr '[:upper:]' '[:lower:]')" == *"$(echo "$COMPANY" | tr '[:upper:]' '[:lower:]')"* ]] || continue
  fi
  if [[ -n "$CAUSE" ]]; then
    [[ "$(fm_value "$file" root_cause_category)" == "$CAUSE" ]] || continue
  fi
  if [[ -n "$SEVERITY" ]]; then
    [[ "$(fm_value "$file" severity)" == "$SEVERITY" ]] || continue
  fi
  # 关键词全文过滤
  if [[ -n "$KEYWORD" ]]; then
    grep -qi -- "$KEYWORD" "$file" || continue
  fi

  matches=$((matches + 1))
  rel="${file#"$ROOT"/}"
  printf '\033[1m%s\033[0m  %s  %s\n' "$(fm_value "$file" id)" "$(fm_value "$file" severity)" "$(fm_value "$file" date)"
  printf '  %s\n' "$(fm_value "$file" title)"
  printf '  \033[2m%s\033[0m\n' "$rel"
  if [[ -n "$KEYWORD" ]]; then
    # 展示前 3 处关键词命中上下文
    grep -in -- "$KEYWORD" "$file" | head -3 | sed 's/^/    L/'
  fi
  echo
done < <(find "$search_path" -name '*.md' ! -name 'README.md' | sort)

if [[ $matches -eq 0 ]]; then
  echo "无匹配案例。提示：--cause/--domain 取值见 docs/severity-and-taxonomy.md"
  exit 1
fi
echo "共 $matches 个匹配案例"
