#!/bin/bash
# 链接验证脚本 - 验证topic-dictionary相关链接

echo "=== Topic Dictionary 链接验证 ==="
echo "验证时间: $(date)"
echo ""

# 验证topic-dictionary目录下的文件
TOPIC_DIR="./topic-dictionary"
FILES=(
    "01-operations-best-practices.md"
    "02-failure-patterns-analysis.md" 
    "03-performance-tuning-expert.md"
    "04-sre-maturity-model.md"
    "05-concept-reference.md"
    "06-cli-commands.md"
    "07-tool-ecosystem.md"
    "validate-links.ps1"
)

echo "1. 验证topic-dictionary目录文件存在性:"
VALID_COUNT=0
TOTAL_COUNT=${#FILES[@]}

for file in "${FILES[@]}"; do
    if [ -f "${TOPIC_DIR}/${file}" ]; then
        size=$(stat -f%z "${TOPIC_DIR}/${file}" 2>/dev/null || stat -c%s "${TOPIC_DIR}/${file}" 2>/dev/null || echo "unknown")
        echo "✅ ${file} (大小: ${size} bytes)"
        ((VALID_COUNT++))
    else
        echo "❌ ${file} 不存在"
    fi
done

echo ""
echo "2. 验证README中topic-dictionary链接:"
# 检查README中是否有指向这些文件的链接
README_FILE="./README.md"
LINK_CHECKS=(
    "01-operations-best-practices.md"
    "02-failure-patterns-analysis.md"
    "03-performance-tuning-expert.md" 
    "04-sre-maturity-model.md"
    "05-concept-reference.md"
    "06-cli-commands.md"
    "07-tool-ecosystem.md"
)

LINK_VALID_COUNT=0
LINK_TOTAL_COUNT=${#LINK_CHECKS[@]}

for link in "${LINK_CHECKS[@]}"; do
    if grep -q "\./topic-dictionary/${link}" "${README_FILE}"; then
        echo "✅ README中包含 ${link} 的链接"
        ((LINK_VALID_COUNT++))
    else
        echo "❌ README中缺少 ${link} 的链接"
    fi
done

echo ""
echo "=== 验证结果汇总 ==="
echo "文件验证: ${VALID_COUNT}/${TOTAL_COUNT} 个文件存在"
echo "链接验证: ${LINK_VALID_COUNT}/${LINK_TOTAL_COUNT} 个链接正确"

if [ ${VALID_COUNT} -eq ${TOTAL_COUNT} ] && [ ${LINK_VALID_COUNT} -eq ${LINK_TOTAL_COUNT} ]; then
    echo "🎉 所有验证通过！topic-dictionary结构完整且链接有效。"
    exit 0
else
    echo "⚠️  存在验证问题，请检查上述错误信息。"
    exit 1
fi