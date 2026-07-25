#!/bin/bash
BASE="."
DOMAINS=("云厂商" "容器运行时" "AI基础设施" "专项技术" "数据库中间件" "系统基础" "清单模式" "生态参考" "应用模式")

for d in "${DOMAINS[@]}"; do
  DIR="$BASE/$d"
  echo "===DOMAIN: $d ==="
  if [ ! -d "$DIR" ]; then echo "MISSING"; continue; fi
  total_md=$(find "$DIR" -name "*.md" | wc -l | tr -d ' ')
  excluded=$(find "$DIR" \( -name "index.md" -o -name "MOC.md" -o -name "README.md" \) | wc -l | tr -d ' ')
  page_count=$((total_md - excluded))
  echo "TOTAL_MD: $total_md | EXCLUDED: $excluded | PAGE_COUNT: $page_count"
  find "$DIR" -mindepth 1 -maxdepth 1 -type d -exec basename {} \; | sort | tr '\n' ','
  echo ""
  tier_core=$(grep -rl "^tier: core" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  tier_supporting=$(grep -rl "^tier: supporting" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  tier_peripheral=$(grep -rl "^tier: peripheral" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  echo "TIER: core=$tier_core supporting=$tier_supporting peripheral=$tier_peripheral"
  db=$(grep -rl "^difficulty: beginner" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  di=$(grep -rl "^difficulty: intermediate" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  da=$(grep -rl "^difficulty: advanced" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  de=$(grep -rl "^difficulty: expert" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  echo "DIFF: beginner=$db intermediate=$di advanced=$da expert=$de"
  fm_total=0
  fm_complete=0
  while IFS= read -r f; do
    fm_total=$((fm_total + 1))
    ht=$(grep -c "^title:" "$f" 2>/dev/null)
    hs=$(grep -c "^summary:" "$f" 2>/dev/null)
    hg=$(grep -c "^tags:" "$f" 2>/dev/null)
    hc=$(grep -c "^category:" "$f" 2>/dev/null)
    if [ "$ht" -gt 0 ] 2>/dev/null && [ "$hs" -gt 0 ] 2>/dev/null && [ "$hg" -gt 0 ] 2>/dev/null && [ "$hc" -gt 0 ] 2>/dev/null; then
      fm_complete=$((fm_complete + 1))
    fi
  done < <(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md")
  echo "FM: $fm_complete/$fm_total"
  line_count=$(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md" -exec cat {} + 2>/dev/null | wc -l | tr -d ' ')
  char_count=$(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md" -exec cat {} + 2>/dev/null | wc -m | tr -d ' ')
  echo "LINES: $line_count | CHARS: $char_count"
  stale=0
  while IFS= read -r f; do
    lu=$(grep "^last_updated:" "$f" 2>/dev/null | head -1 | sed 's/last_updated: *//' | tr -d '"' | tr -d "'")
    if [ -n "$lu" ] && [[ "$lu" < "2025-12" ]]; then
      stale=$((stale + 1))
    fi
  done < <(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md")
  echo "STALE: $stale"
  echo "===END==="
done
