#!/bin/bash
BASE="/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"
DOMAINS=("集群基础" "工作负载" "网络" "存储" "安全" "可观测性" "平台工程" "发布变更" "可靠性" "故障诊断" "生产运维" "云厂商" "容器运行时" "AI基础设施" "专项技术" "数据库中间件" "系统基础" "清单模式" "生态参考" "应用模式")

for d in "${DOMAINS[@]}"; do
  DIR="$BASE/$d"
  echo "===DOMAIN: $d ==="
  
  if [ ! -d "$DIR" ]; then
    echo "MISSING"
    continue
  fi
  
  # Total .md files
  total_md=$(find "$DIR" -name "*.md" | wc -l | tr -d ' ')
  echo "TOTAL_MD: $total_md"
  
  # Exclude index.md, MOC.md, README.md
  excluded=$(find "$DIR" \( -name "index.md" -o -name "MOC.md" -o -name "README.md" \) | wc -l | tr -d ' ')
  echo "EXCLUDED: $excluded"
  page_count=$((total_md - excluded))
  echo "PAGE_COUNT: $page_count"
  
  # List excluded files
  echo "EXCLUDED_FILES:"
  find "$DIR" \( -name "index.md" -o -name "MOC.md" -o -name "README.md" \) | sort
  
  # Subdirectory count (second-level dirs)
  subdir_list=$(find "$DIR" -mindepth 1 -maxdepth 1 -type d | sort)
  subdir_count=$(echo "$subdir_list" | grep -c . 2>/dev/null || echo 0)
  echo "SUBDIR_COUNT: $subdir_count"
  echo "SUBDIRS:"
  echo "$subdir_list" | while read sd; do basename "$sd"; done
  
  # Tier distribution
  tier_core=$(grep -rl "^tier: core" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  tier_supporting=$(grep -rl "^tier: supporting" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  tier_peripheral=$(grep -rl "^tier: peripheral" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  echo "TIER_CORE: $tier_core"
  echo "TIER_SUPPORTING: $tier_supporting"
  echo "TIER_PERIPHERAL: $tier_peripheral"
  
  # Difficulty distribution
  diff_beginner=$(grep -rl "^difficulty: beginner" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  diff_intermediate=$(grep -rl "^difficulty: intermediate" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  diff_advanced=$(grep -rl "^difficulty: advanced" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  diff_expert=$(grep -rl "^difficulty: expert" "$DIR" --include="*.md" 2>/dev/null | wc -l | tr -d ' ')
  echo "DIFF_BEGINNER: $diff_beginner"
  echo "DIFF_INTERMEDIATE: $diff_intermediate"
  echo "DIFF_ADVANCED: $diff_advanced"
  echo "DIFF_EXPERT: $diff_expert"
  
  # Frontmatter completeness
  fm_total=0
  fm_complete=0
  for f in $(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md"); do
    fm_total=$((fm_total + 1))
    has_title=$(grep -c "^title:" "$f" 2>/dev/null || echo 0)
    has_summary=$(grep -c "^summary:" "$f" 2>/dev/null || echo 0)
    has_tags=$(grep -c "^tags:" "$f" 2>/dev/null || echo 0)
    has_category=$(grep -c "^category:" "$f" 2>/dev/null || echo 0)
    if [ "$has_title" -gt 0 ] && [ "$has_summary" -gt 0 ] && [ "$has_tags" -gt 0 ] && [ "$has_category" -gt 0 ]; then
      fm_complete=$((fm_complete + 1))
    fi
  done
  echo "FM_TOTAL: $fm_total"
  echo "FM_COMPLETE: $fm_complete"
  
  # Line count and char count
  line_count=$(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md" -exec cat {} + 2>/dev/null | wc -l | tr -d ' ')
  char_count=$(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md" -exec cat {} + 2>/dev/null | wc -m | tr -d ' ')
  echo "LINE_COUNT: $line_count"
  echo "CHAR_COUNT: $char_count"
  
  # Stale pages: last_updated before 2025-12
  stale=0
  stale_files=""
  for f in $(find "$DIR" -name "*.md" ! -name "index.md" ! -name "MOC.md" ! -name "README.md"); do
    lu=$(grep "^last_updated:" "$f" 2>/dev/null | head -1 | sed 's/last_updated: *//' | tr -d '"' | tr -d "'")
    if [ -n "$lu" ]; then
      if [[ "$lu" < "2025-12" ]]; then
        stale=$((stale + 1))
        stale_files="$stale_files $(basename "$f")"
      fi
    fi
  done
  echo "STALE_COUNT: $stale"
  echo "STALE_FILES: $stale_files"
  
  echo "===END==="
  echo ""
done
