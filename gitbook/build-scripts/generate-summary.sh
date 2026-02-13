#!/bin/bash
# generate-summary.sh
# 自动生成 mdBook SUMMARY.md 文件
# 扫描项目目录，生成包含所有 Markdown 文件的目录树

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
OUTPUT_FILE="$SCRIPT_DIR/src/SUMMARY.md"

# 提取 README.md 第一行标题
get_title() {
    local file="$1"
    if [[ -f "$file" ]]; then
        head -n 1 "$file" | sed 's/^#* *//'
    else
        basename "$(dirname "$file")"
    fi
}

# 从文件名生成显示标题
get_file_title() {
    local file="$1"
    local basename_no_ext=$(basename "$file" .md)
    # 尝试从文件第一行获取标题
    if [[ -f "$file" ]]; then
        local first_line=$(head -n 1 "$file")
        if [[ "$first_line" == "#"* ]]; then
            echo "$first_line" | sed 's/^#* *//'
            return
        fi
    fi
    # 否则使用文件名
    echo "$basename_no_ext"
}

# 生成相对路径（相对于 src 目录，通过符号链接访问）
get_relative_path() {
    local file="$1"
    # 从项目根目录的相对路径，用于 src 中的符号链接
    echo "${file#$PROJECT_ROOT/}"
}

# 处理单个目录（非递归），带额外缩进
process_directory() {
    local dir="$1"
    local indent="$2"
    local dir_name=$(basename "$dir")
    local readme="$dir/README.md"
    local relative_readme=$(get_relative_path "$readme")
    local entry_file=""
    
    # 获取目录标题
    local title
    if [[ -f "$readme" ]]; then
        title=$(get_title "$readme")
        echo "${indent}- [$title]($relative_readme)"
    else
        title="$dir_name"
        # 找到第一个 .md 文件作为入口
        local first_md=$(find "$dir" -maxdepth 1 -name "*.md" -type f | sort | head -n 1)
        if [[ -n "$first_md" ]]; then
            entry_file="$first_md"
            local relative_first=$(get_relative_path "$first_md")
            echo "${indent}- [$title]($relative_first)"
        else
            echo "${indent}- [$title]()"
        fi
    fi
    
    # 列出目录下所有 .md 文件（排除 README.md 和入口文件）
    while IFS= read -r file; do
        if [[ -n "$file" && "$file" != "$entry_file" ]]; then
            local file_title=$(get_file_title "$file")
            local relative_file=$(get_relative_path "$file")
            echo "${indent}  - [$file_title]($relative_file)"
        fi
    done < <(find "$dir" -maxdepth 1 -name "*.md" -type f ! -name "README.md" | sort)
}

# 处理带子目录的目录（递归）
process_directory_recursive() {
    local dir="$1"
    local indent="$2"
    local dir_name=$(basename "$dir")
    local readme="$dir/README.md"
    local relative_readme=$(get_relative_path "$readme")
    
    # 获取目录标题
    local title
    if [[ -f "$readme" ]]; then
        title=$(get_title "$readme")
        echo "${indent}- [$title]($relative_readme)"
    else
        title="$dir_name"
        local first_md=$(find "$dir" -maxdepth 1 -name "*.md" -type f | sort | head -n 1)
        if [[ -n "$first_md" ]]; then
            local relative_first=$(get_relative_path "$first_md")
            echo "${indent}- [$title]($relative_first)"
        else
            echo "${indent}- [$title]()"
        fi
    fi
    
    # 列出当前目录下的 .md 文件（排除 README.md）
    while IFS= read -r file; do
        if [[ -n "$file" ]]; then
            local file_title=$(get_file_title "$file")
            local relative_file=$(get_relative_path "$file")
            echo "${indent}  - [$file_title]($relative_file)"
        fi
    done < <(find "$dir" -maxdepth 1 -name "*.md" -type f ! -name "README.md" | sort)
    
    # 处理子目录
    while IFS= read -r subdir; do
        if [[ -n "$subdir" && -d "$subdir" ]]; then
            process_directory "$subdir" "${indent}  "
        fi
    done < <(find "$dir" -mindepth 1 -maxdepth 1 -type d | sort)
}

# 开始生成 SUMMARY.md
echo "正在生成 SUMMARY.md..."

cat > "$OUTPUT_FILE" << 'EOF'
# Summary

[首页](README.md)

---

EOF

# 核心知识域 (Domain 1-12) - 作为可折叠章节
echo "- [核心知识域 (Domain 1-12)]()" >> "$OUTPUT_FILE"

for i in 1 2 3 4 5 6 7 8 9 10 11 12; do
    dir=$(find "$PROJECT_ROOT" -maxdepth 1 -type d -name "domain-$i-*" | head -n 1)
    if [[ -d "$dir" ]]; then
        process_directory "$dir" "  " >> "$OUTPUT_FILE"
    fi
done

echo "" >> "$OUTPUT_FILE"

# 底层基础知识域 (Domain 13-17) - 作为可折叠章节
echo "- [底层基础知识域 (Domain 13-17)]()" >> "$OUTPUT_FILE"

for i in 13 14 15 16 17; do
    dir=$(find "$PROJECT_ROOT" -maxdepth 1 -type d -name "domain-$i-*" | head -n 1)
    if [[ -d "$dir" ]]; then
        # domain-17 有子目录，需要递归处理
        if [[ "$i" == "17" ]]; then
            process_directory_recursive "$dir" "  " >> "$OUTPUT_FILE"
        else
            process_directory "$dir" "  " >> "$OUTPUT_FILE"
        fi
    fi
done

echo "" >> "$OUTPUT_FILE"

# 企业级运维专题 (Domain 18-33) - 作为可折叠章节
echo "- [企业级运维专题 (Domain 18-33)]()" >> "$OUTPUT_FILE"

for i in 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33; do
    dir=$(find "$PROJECT_ROOT" -maxdepth 1 -type d -name "domain-$i-*" | head -n 1)
    if [[ -d "$dir" ]]; then
        process_directory "$dir" "  " >> "$OUTPUT_FILE"
    fi
done

echo "" >> "$OUTPUT_FILE"

# 专题资源 - 作为可折叠章节
echo "- [专题资源]()" >> "$OUTPUT_FILE"

# topic-cheat-sheet
dir="$PROJECT_ROOT/topic-cheat-sheet"
if [[ -d "$dir" ]]; then
    process_directory "$dir" "  " >> "$OUTPUT_FILE"
fi

# topic-dictionary
dir="$PROJECT_ROOT/topic-dictionary"
if [[ -d "$dir" ]]; then
    process_directory "$dir" "  " >> "$OUTPUT_FILE"
fi

# topic-presentations
dir="$PROJECT_ROOT/topic-presentations"
if [[ -d "$dir" ]]; then
    process_directory "$dir" "  " >> "$OUTPUT_FILE"
fi

# topic-structural-trouble-shooting（递归处理子目录）
dir="$PROJECT_ROOT/topic-structural-trouble-shooting"
if [[ -d "$dir" ]]; then
    process_directory_recursive "$dir" "  " >> "$OUTPUT_FILE"
fi

echo "" >> "$OUTPUT_FILE"

echo "SUMMARY.md 生成完成: $OUTPUT_FILE"
echo "文件数统计:"
grep -c "\.md)" "$OUTPUT_FILE" || echo "0"
