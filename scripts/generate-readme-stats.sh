#!/usr/bin/env bash
# ============================================================================
# generate-readme-stats.sh - KUDIG-DATABASE README 数字指标自动统计
# ============================================================================
# 功能: 统计 README.md 中引用的所有数字指标，支持 JSON/表格/徽章 三种输出格式
# 用途: 每次内容更新后运行，确保 README 中的数字与实际文件保持一致
# 用法:
#   ./scripts/generate-readme-stats.sh             # 默认表格输出
#   ./scripts/generate-readme-stats.sh --json      # JSON 格式（可供其他脚本消费）
#   ./scripts/generate-readme-stats.sh --badges    # 输出 README 徽章建议
#   ./scripts/generate-readme-stats.sh --diff      # 输出指标并与 README 当前数字比对
# 兼容: macOS bash 3.2+ / Linux bash 4+
# ============================================================================

set -euo pipefail

# 切换到项目根目录
cd "$(dirname "$0")/.."

# ───────────────────────────────────────────────────
# 颜色定义
# ───────────────────────────────────────────────────
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m'

# ───────────────────────────────────────────────────
# 辅助函数
# ───────────────────────────────────────────────────
format_number() {
    printf "%'d" "$1" 2>/dev/null || echo "$1"
}

count_md() {
    find "$1" -name "*.md" -type f 2>/dev/null | wc -l | tr -d ' '
}

count_md_content() {
    # 排除 README/SUMMARY/QUALITY/ENTERPRISE 辅助文件
    find "$1" -name "*.md" -type f \
        -not -name "README.md" \
        -not -name "SUMMARY.md" \
        -not -name "QUALITY*.md" \
        -not -name "ENTERPRISE*.md" \
        2>/dev/null | wc -l | tr -d ' '
}

# ═══════════════════════════════════════════════════
# 1. 整体规模指标
# ═══════════════════════════════════════════════════
TOTAL_FILES=$(find . -name "*.md" -type f | wc -l | tr -d ' ')
MD_DOCS=$(find domain-* topic-* -name "*.md" -type f 2>/dev/null | wc -l | tr -d ' ')
TOTAL_CHARS=$(LC_ALL=C find . -name "*.md" -type f -print0 | xargs -0 wc -m 2>/dev/null | tail -1 | awk '{print $1}')
DOMAIN_COUNT=$(ls -d domain-* topic-* 2>/dev/null | wc -l | tr -d ' ')
PRODUCT_COUNT=$(ls topic-fta/list/*-fta.md 2>/dev/null | sed 's/.*\///' | sed 's/-fta\.md$//' | sort -u | wc -l | tr -d ' ')

# ═══════════════════════════════════════════════════
# 2. AI 相关指标
# ═══════════════════════════════════════════════════
AI_AGENT_DOCS=$(count_md_content "topic-ai-agent")
FTA_TREES=$(ls topic-fta/list/*-fta.md 2>/dev/null | wc -l | tr -d ' ')
FTA_TOTAL_DOCS=$(count_md "topic-fta")
FEBM_DOCS=$(count_md_content "topic-febm")
LEARN_DOCS=$(count_md "topic-learn")
CNCF_PROJECTS=$(count_md_content "domain-34-cncf-landscape")

# ═══════════════════════════════════════════════════
# 3. 运维专题指标
# ═══════════════════════════════════════════════════
TS_D12=$(count_md "domain-12-troubleshooting")
TS_STS=$(count_md "topic-structural-trouble-shooting")
TROUBLESHOOT_DOCS=$((TS_D12 + TS_STS + FTA_TOTAL_DOCS))
SKILLS_COUNT=$(count_md_content "topic-skills")
CHEAT_SHEET_COUNT=$(count_md_content "topic-cheat-sheet")
PRESENTATION_COUNT=$(count_md_content "topic-presentations")
PAPERS_COUNT=$(count_md_content "domain-19-papers")

# ═══════════════════════════════════════════════════
# 4. 云厂商
# ═══════════════════════════════════════════════════
CLOUD_PROVIDERS=$(ls -d domain-17-cloud-provider/*/ 2>/dev/null | wc -l | tr -d ' ')

# ═══════════════════════════════════════════════════
# 5. 各 domain / topic 分布 (macOS bash 3.2 兼容)
# ═══════════════════════════════════════════════════
DOMAIN_NAMES=""
DOMAIN_COUNTS=""
for d in domain-*/; do
    name=$(basename "$d")
    cnt=$(count_md_content "$d")
    DOMAIN_NAMES="${DOMAIN_NAMES}${name}|"
    DOMAIN_COUNTS="${DOMAIN_COUNTS}${cnt}|"
done

TOPIC_NAMES=""
TOPIC_COUNTS=""
for d in topic-*/; do
    name=$(basename "$d")
    cnt=$(count_md "$d")
    TOPIC_NAMES="${TOPIC_NAMES}${name}|"
    TOPIC_COUNTS="${TOPIC_COUNTS}${cnt}|"
done

# ═══════════════════════════════════════════════════
# 输出: 表格
# ═══════════════════════════════════════════════════
output_table() {
    local chars_wan=$((TOTAL_CHARS / 10000))
    echo ""
    echo -e "${BOLD}════════════════════════════════════════════════════${NC}"
    echo -e "${BOLD}  KUDIG-DATABASE 数字指标统计报告${NC}"
    echo -e "${BOLD}  统计时间: $(date '+%Y-%m-%d %H:%M')${NC}"
    echo -e "${BOLD}════════════════════════════════════════════════════${NC}"

    echo ""
    echo -e "${CYAN}📈 整体规模${NC}"
    echo "  ─────────────────────────────────────"
    printf "  %-20s %s\n" "文件总数" "$(format_number "$TOTAL_FILES")"
    printf "  %-20s %s\n" "Markdown 文档" "$(format_number "$MD_DOCS")"
    printf "  %-20s %s (约 %s 万)\n" "总字符数" "$(format_number "$TOTAL_CHARS")" "$chars_wan"
    printf "  %-20s %s\n" "知识领域" "$DOMAIN_COUNT"
    printf "  %-20s %s\n" "开源产品(FTA覆盖)" "$PRODUCT_COUNT"

    echo ""
    echo -e "${CYAN}🤖 AI 相关${NC}"
    echo "  ─────────────────────────────────────"
    printf "  %-20s %s 篇\n" "AI Agent 文档" "$AI_AGENT_DOCS"
    printf "  %-20s %s 个\n" "FTA 故障树" "$FTA_TREES"
    printf "  %-20s %s 篇\n" "FTA 总文档" "$FTA_TOTAL_DOCS"
    printf "  %-20s %s 篇\n" "FEBM 取证" "$FEBM_DOCS"
    printf "  %-20s %s 篇\n" "学习课程" "$LEARN_DOCS"
    printf "  %-20s %s 个\n" "CNCF 项目" "$CNCF_PROJECTS"

    echo ""
    echo -e "${CYAN}🔧 运维专题${NC}"
    echo "  ─────────────────────────────────────"
    printf "  %-20s %s 篇\n" "故障排查文档(合计)" "$TROUBLESHOOT_DOCS"
    printf "  %-20s %s 个\n" "技能库 Skills" "$SKILLS_COUNT"
    printf "  %-20s %s 张\n" "速查卡" "$CHEAT_SHEET_COUNT"
    printf "  %-20s %s 篇\n" "演示文档" "$PRESENTATION_COUNT"
    printf "  %-20s %s 篇\n" "技术白皮书/论文" "$PAPERS_COUNT"
    printf "  %-20s %s 家\n" "云厂商" "$CLOUD_PROVIDERS"

    echo ""
    echo -e "${CYAN}📂 各 Domain 文档数${NC}"
    echo "  ─────────────────────────────────────"
    IFS='|' read -ra _dn <<< "$DOMAIN_NAMES"
    IFS='|' read -ra _dc <<< "$DOMAIN_COUNTS"
    for i in "${!_dn[@]}"; do
        [ -z "${_dn[$i]}" ] && continue
        printf "  %-44s %s\n" "${_dn[$i]}" "${_dc[$i]}"
    done

    echo ""
    echo -e "${CYAN}📁 各 Topic 文档数${NC}"
    echo "  ─────────────────────────────────────"
    IFS='|' read -ra _tn <<< "$TOPIC_NAMES"
    IFS='|' read -ra _tc <<< "$TOPIC_COUNTS"
    for i in "${!_tn[@]}"; do
        [ -z "${_tn[$i]}" ] && continue
        printf "  %-44s %s\n" "${_tn[$i]}" "${_tc[$i]}"
    done

    echo ""
    echo -e "${BOLD}════════════════════════════════════════════════════${NC}"
}

# ═══════════════════════════════════════════════════
# 输出: JSON
# ═══════════════════════════════════════════════════
output_json() {
    local chars_wan=$((TOTAL_CHARS / 10000))

    echo "{"
    echo "  \"generated_at\": \"$(date -u '+%Y-%m-%dT%H:%M:%SZ')\","

    # overall
    echo "  \"overall\": {"
    echo "    \"total_files\": $TOTAL_FILES,"
    echo "    \"markdown_docs\": $MD_DOCS,"
    echo "    \"total_chars\": $TOTAL_CHARS,"
    echo "    \"total_chars_wan\": $chars_wan,"
    echo "    \"domain_count\": $DOMAIN_COUNT,"
    echo "    \"product_count\": $PRODUCT_COUNT"
    echo "  },"

    # ai_related
    echo "  \"ai_related\": {"
    echo "    \"ai_agent_docs\": $AI_AGENT_DOCS,"
    echo "    \"fta_trees\": $FTA_TREES,"
    echo "    \"fta_total_docs\": $FTA_TOTAL_DOCS,"
    echo "    \"febm_docs\": $FEBM_DOCS,"
    echo "    \"learn_docs\": $LEARN_DOCS,"
    echo "    \"cncf_projects\": $CNCF_PROJECTS"
    echo "  },"

    # ops_topics
    echo "  \"ops_topics\": {"
    echo "    \"troubleshoot_docs\": $TROUBLESHOOT_DOCS,"
    echo "    \"skills_count\": $SKILLS_COUNT,"
    echo "    \"cheat_sheet_count\": $CHEAT_SHEET_COUNT,"
    echo "    \"presentation_count\": $PRESENTATION_COUNT,"
    echo "    \"papers_count\": $PAPERS_COUNT,"
    echo "    \"cloud_providers\": $CLOUD_PROVIDERS"
    echo "  },"

    # domains
    echo "  \"domains\": {"
    IFS='|' read -ra _dn <<< "$DOMAIN_NAMES"
    IFS='|' read -ra _dc <<< "$DOMAIN_COUNTS"
    local first=true
    for i in "${!_dn[@]}"; do
        [ -z "${_dn[$i]}" ] && continue
        $first && first=false || echo ","
        printf "    \"%s\": %s" "${_dn[$i]}" "${_dc[$i]}"
    done
    echo ""
    echo "  },"

    # topics
    echo "  \"topics\": {"
    IFS='|' read -ra _tn <<< "$TOPIC_NAMES"
    IFS='|' read -ra _tc <<< "$TOPIC_COUNTS"
    first=true
    for i in "${!_tn[@]}"; do
        [ -z "${_tn[$i]}" ] && continue
        $first && first=false || echo ","
        printf "    \"%s\": %s" "${_tn[$i]}" "${_tc[$i]}"
    done
    echo ""
    echo "  }"

    echo "}"
}

# ═══════════════════════════════════════════════════
# 输出: README 徽章
# ═══════════════════════════════════════════════════
output_badges() {
    local chars_wan="$((TOTAL_CHARS / 10000))万"
    echo ""
    echo -e "${BOLD}README 徽章建议（基于实际统计）:${NC}"
    echo ""
    echo "<!-- Badges Row -->"
    echo "<p>"
    echo "  <img src=\"https://img.shields.io/badge/文档-${MD_DOCS}%2B-blue?style=flat-square&logo=readthedocs\" alt=\"文档数量\"/>"
    echo "  <img src=\"https://img.shields.io/badge/知识域-${DOMAIN_COUNT}%2B-green?style=flat-square&logo=bookstack\" alt=\"知识领域\"/>"
    echo "  <img src=\"https://img.shields.io/badge/总字数-${chars_wan}%2B-orange?style=flat-square&logo=markdown\" alt=\"总字数\"/>"
    echo "  <img src=\"https://img.shields.io/badge/CNCF项目-${CNCF_PROJECTS}-purple?style=flat-square&logo=cncf\" alt=\"CNCF项目\"/>"
    echo "  <img src=\"https://img.shields.io/badge/K8s版本-v1.25--v1.32-326ce5?style=flat-square&logo=kubernetes\" alt=\"K8s版本\"/>"
    echo "  <img src=\"https://img.shields.io/badge/最后更新-$(date '+%Y--%m')-brightgreen?style=flat-square\" alt=\"最后更新\"/>"
    echo "</p>"
    echo ""
    echo "<p>"
    echo "  <img src=\"https://img.shields.io/badge/AI%20Agent-${AI_AGENT_DOCS}篇-ff6b6b?style=flat-square&logo=openai\" alt=\"AI Agent\"/>"
    echo "  <img src=\"https://img.shields.io/badge/FTA故障树-${FTA_TREES}个-4ecdc4?style=flat-square\" alt=\"FTA\"/>"
    echo "  <img src=\"https://img.shields.io/badge/FEBM取证-${FEBM_DOCS}篇-45b7d1?style=flat-square\" alt=\"FEBM\"/>"
    echo "  <img src=\"https://img.shields.io/badge/学习计划-${LEARN_DOCS}篇-f9ca24?style=flat-square&logo=graduation-cap\" alt=\"学习计划\"/>"
    echo "</p>"
    echo ""
}

# ═══════════════════════════════════════════════════
# 输出: 差异比对
# ═══════════════════════════════════════════════════
output_diff() {
    local chars_wan=$((TOTAL_CHARS / 10000))
    output_table
    echo ""
    echo -e "${BOLD}与 README.md 当前数字的差异检查:${NC}"
    echo ""
    echo -e "${CYAN}  核心指标:${NC}"
    printf "  %-25s 实际值: %s\n" "Markdown 文档" "$MD_DOCS"
    printf "  %-25s 实际值: %s\n" "知识域" "$DOMAIN_COUNT"
    printf "  %-25s 实际值: %s 万\n" "总字符数" "$chars_wan"
    printf "  %-25s 实际值: %s\n" "CNCF 项目" "$CNCF_PROJECTS"
    printf "  %-25s 实际值: %s\n" "AI Agent 文档" "$AI_AGENT_DOCS"
    printf "  %-25s 实际值: %s\n" "FTA 故障树" "$FTA_TREES"
    printf "  %-25s 实际值: %s\n" "FEBM 取证" "$FEBM_DOCS"
    printf "  %-25s 实际值: %s\n" "学习课程" "$LEARN_DOCS"
    printf "  %-25s 实际值: %s\n" "文件总数" "$TOTAL_FILES"
    echo ""
    echo -e "  ${YELLOW}提示: 请手动核对上述实际值与 README.md 中的数字是否一致${NC}"
    echo -e "  ${YELLOW}      如有差异，可使用 --badges 模式生成最新徽章代码${NC}"
    echo ""
}

# ═══════════════════════════════════════════════════
# 主入口
# ═══════════════════════════════════════════════════
MODE="${1:-table}"

case "$MODE" in
    --json)   output_json ;;
    --badges) output_badges ;;
    --diff)   output_diff ;;
    --help|-h)
        echo "用法: $0 [选项]"
        echo ""
        echo "选项:"
        echo "  (无参数)    表格格式输出所有指标"
        echo "  --json      JSON 格式输出（可供其他脚本消费）"
        echo "  --badges    输出 README 徽章 HTML 建议"
        echo "  --diff      输出指标并与 README 当前数字比对"
        echo "  --help      显示帮助"
        ;;
    *)        output_table ;;
esac
