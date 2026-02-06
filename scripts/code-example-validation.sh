#!/bin/bash

echo "=== Kubernetes知识库代码示例质量检查 ==="
echo "检查时间: $(date)"

# 统计信息
TOTAL_YAML_BLOCKS=0
TOTAL_BASH_BLOCKS=0
INVALID_YAML=0
INVALID_BASH=0

echo -e "\n1. 检查YAML代码块语法..."

# 检查所有YAML代码块
find . -name "*.md" -not -path "./.git/*" | while read file; do
    # 提取YAML代码块
    yaml_blocks=$(grep -n "^\`\`\`yaml" "$file" | cut -d: -f1)
    
    if [ ! -z "$yaml_blocks" ]; then
        echo "检查文件: $file"
        while IFS= read -r line_num; do
            ((TOTAL_YAML_BLOCKS++))
            
            # 提取YAML块内容
            start_line=$((line_num + 1))
            end_line=$(sed -n "${start_line},\$p" "$file" | grep -n "^\`\`\`" | head -1 | cut -d: -f1)
            
            if [ ! -z "$end_line" ]; then
                end_line=$((start_line + end_line - 2))
                # 临时提取YAML内容进行验证
                temp_yaml="/tmp/temp_$$_$(basename "$file").yaml"
                sed -n "${start_line},${end_line}p" "$file" > "$temp_yaml"
                
                # 使用Python yaml验证
                if python3 -c "import yaml; yaml.safe_load(open('$temp_yaml'))" 2>/dev/null; then
                    echo "  ✓ YAML块 (行 $line_num) 语法正确"
                else
                    echo "  ✗ YAML块 (行 $line_num) 语法错误"
                    ((INVALID_YAML++))
                    echo "    错误详情:"
                    python3 -c "import yaml; yaml.safe_load(open('$temp_yaml'))" 2>&1 | sed 's/^/      /'
                fi
                rm -f "$temp_yaml"
            fi
        done <<< "$yaml_blocks"
    fi
done

echo -e "\n2. 检查Shell脚本语法..."

# 检查所有bash代码块
find . -name "*.md" -not -path "./.git/*" | while read file; do
    bash_blocks=$(grep -n "^\`\`\`bash" "$file" | cut -d: -f1)
    
    if [ ! -z "$bash_blocks" ]; then
        echo "检查文件: $file"
        while IFS= read -r line_num; do
            ((TOTAL_BASH_BLOCKS++))
            
            start_line=$((line_num + 1))
            end_line=$(sed -n "${start_line},\$p" "$file" | grep -n "^\`\`\`" | head -1 | cut -d: -f1)
            
            if [ ! -z "$end_line" ]; then
                end_line=$((start_line + end_line - 2))
                temp_sh="/tmp/temp_$$_$(basename "$file").sh"
                sed -n "${start_line},${end_line}p" "$file" > "$temp_sh"
                
                # 基本语法检查
                if bash -n "$temp_sh" 2>/dev/null; then
                    echo "  ✓ Bash块 (行 $line_num) 语法正确"
                else
                    echo "  ✗ Bash块 (行 $line_num) 语法错误"
                    ((INVALID_BASH++))
                    echo "    错误详情:"
                    bash -n "$temp_sh" 2>&1 | sed 's/^/      /'
                fi
                rm -f "$temp_sh"
            fi
        done <<< "$bash_blocks"
    fi
done

echo -e "\n=== 检查结果汇总 ==="
echo "YAML代码块总数: $TOTAL_YAML_BLOCKS"
echo "Bash代码块总数: $TOTAL_BASH_BLOCKS"
echo "YAML语法错误数: $INVALID_YAML"
echo "Bash语法错误数: $INVALID_BASH"

if [ $INVALID_YAML -eq 0 ] && [ $INVALID_BASH -eq 0 ]; then
    echo "🎉 所有代码示例语法正确！"
    exit 0
else
    echo "❌ 发现语法错误，请修复后再提交"
    exit 1
fi