#!/bin/bash
# count-stats.sh - KUDIG-DATABASE 项目统计脚本
# 统计文件总数、总字数、产品总数、知识领域数、知识点总数

set -e

# 切换到脚本所在目录
cd "$(dirname "$0")"

# 格式化数字（添加千分位分隔符）
format_number() {
    printf "%'d" "$1"
}

# 1. 文件总数（所有 .md 文件）
file_count=$(find . -name "*.md" -type f | wc -l | tr -d ' ')

# 2. 总字数（所有 .md 文件的字符总数）
total_chars=$(find . -name "*.md" -type f -exec cat {} + 2>/dev/null | wc -m | tr -d ' ')

# 3. 产品总数（从 topic-fta/list/ 提取组件名）
if [ -d "topic-fta/list" ]; then
    product_count=$(ls topic-fta/list/*-fta.md 2>/dev/null | \
        sed 's/.*\///' | sed 's/-fta\.md$//' | sort -u | wc -l | tr -d ' ')
else
    product_count=0
fi

# 4. 知识领域数（domain-* + topic-* 目录）
domain_count=$(ls -d domain-* topic-* 2>/dev/null | wc -l | tr -d ' ')

# 5. 知识点总数（各知识领域下的 .md 文件）
knowledge_count=$(find domain-* topic-* -name "*.md" -type f 2>/dev/null | wc -l | tr -d ' ')

# 输出统计报告
echo "===================================="
echo "KUDIG-DATABASE 项目统计报告"
echo "===================================="
printf "文件总数:     %s 个\n" "$(format_number $file_count)"
printf "总字数:       %s 字\n" "$(format_number $total_chars)"
printf "产品总数:     %s 个\n" "$(format_number $product_count)"
printf "知识领域数:   %s 个\n" "$(format_number $domain_count)"
printf "知识点总数:   %s 个\n" "$(format_number $knowledge_count)"
echo "===================================="
