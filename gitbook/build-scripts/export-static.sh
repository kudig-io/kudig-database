#!/bin/bash
# export-static.sh - 导出可本地直接打开的静态 gitbook
# 用法:
#   ./export-static.sh           # 导出到 gitbook/dist/ 目录
#   ./export-static.sh --zip     # 导出并打包为 zip 文件
#
# 导出的文件可以直接用浏览器打开 dist/index.html

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
DIST_DIR="$SCRIPT_DIR/dist"
ZIP_FLAG="${1:-}"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

log_info()  { echo -e "${GREEN}[INFO]${NC} $*"; }
log_warn()  { echo -e "${YELLOW}[WARN]${NC} $*"; }
log_error() { echo -e "${RED}[ERROR]${NC} $*"; }

# 检查 mdbook 是否安装
if ! command -v mdbook &>/dev/null; then
    log_error "mdbook 未安装，请先安装: cargo install mdbook"
    exit 1
fi

START_TIME=$(date +%s)

# 1. 更新符号链接
log_info "更新符号链接..."
src_dir="$SCRIPT_DIR/src"
find "$src_dir" -maxdepth 1 -type l ! -exec test -e {} \; -delete 2>/dev/null || true
for dir in "$PROJECT_ROOT"/domain-* "$PROJECT_ROOT"/topic-* "$PROJECT_ROOT"/tables; do
    if [[ -d "$dir" ]]; then
        name=$(basename "$dir")
        link="$src_dir/$name"
        if [[ ! -L "$link" ]]; then
            ln -sf "../../$name" "$link"
        fi
    fi
done

# 2. 生成 SUMMARY.md
log_info "生成 SUMMARY.md..."
bash "$SCRIPT_DIR/generate-summary.sh"

# 3. 创建临时 book.toml（去掉 site-url，确保本地 file:// 可访问）
log_info "准备静态导出配置..."
ORIG_TOML="$SCRIPT_DIR/book.toml"
BACKUP_TOML="$SCRIPT_DIR/book.toml.bak"
cp "$ORIG_TOML" "$BACKUP_TOML"

# 修改 book.toml 用于静态导出
# - 去掉 site-url（使路径完全相对化，兼容 file:// 协议）
# - 修改 build-dir 为 dist
sed -i '' '/^site-url/d' "$ORIG_TOML"
sed -i '' 's|^build-dir = "book"|build-dir = "dist"|' "$ORIG_TOML"

# 4. 构建静态版本
log_info "构建静态版本..."
cd "$SCRIPT_DIR"
mdbook build 2>&1 | grep -v "^$"

# 5. 恢复原始 book.toml
mv "$BACKUP_TOML" "$ORIG_TOML"
log_info "已恢复原始 book.toml"

# 6. 统计结果
PAGE_COUNT=$(find "$DIST_DIR" -name "*.html" -not -name "print.html" | wc -l | tr -d ' ')
TOTAL_SIZE=$(du -sh "$DIST_DIR" 2>/dev/null | cut -f1)

log_info "静态导出完成:"
log_info "  页面数量: $PAGE_COUNT"
log_info "  总大小: $TOTAL_SIZE"
log_info "  输出目录: $DIST_DIR"

# 7. 可选打包为 zip
if [[ "$ZIP_FLAG" == "--zip" ]]; then
    TIMESTAMP=$(date +%Y%m%d-%H%M%S)
    ZIP_NAME="kudig-database-gitbook-${TIMESTAMP}.zip"
    ZIP_PATH="$SCRIPT_DIR/$ZIP_NAME"
    
    log_info "打包为 zip..."
    cd "$DIST_DIR"
    zip -r -q "$ZIP_PATH" . -x "*.DS_Store"
    ZIP_SIZE=$(du -sh "$ZIP_PATH" 2>/dev/null | cut -f1)
    log_info "zip 文件: $ZIP_PATH ($ZIP_SIZE)"
fi

END_TIME=$(date +%s)
ELAPSED=$((END_TIME - START_TIME))
log_info "总耗时: ${ELAPSED}s"

echo ""
echo -e "${GREEN}使用方法:${NC}"
echo "  直接打开: open $DIST_DIR/index.html"
if [[ "$ZIP_FLAG" == "--zip" ]]; then
    echo "  分享 zip: $ZIP_PATH"
fi
