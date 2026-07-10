#!/bin/bash
# Agent Session Monitor - 演示脚本

set -e

SKILL_DIR="$(dirname "$(dirname "$(realpath "$0")")")"
EXAMPLE_DIR="$SKILL_DIR/example"
LOG_FILE="$EXAMPLE_DIR/test_access.log"
OUTPUT_DIR="$EXAMPLE_DIR/sessions"

echo "========================================"
echo "Agent Session Monitor - Demo"
echo "========================================"
echo ""

# 清理旧数据
if [ -d "$OUTPUT_DIR" ]; then
    echo "🧹 Cleaning up old session data..."
    rm -rf "$OUTPUT_DIR"
fi

echo "📂 Log file: $LOG_FILE"
echo "📁 Output dir: $OUTPUT_DIR"
echo ""

# 步骤1：解析日志文件（单次模式）
echo "========================================"
echo "步骤1：解析日志文件"
echo "========================================"
python3 "$SKILL_DIR/main.py" \
    --log-path "$LOG_FILE" \
    --output-dir "$OUTPUT_DIR"

echo ""
echo "✅ 日志解析完成！Session数据已保存到: $OUTPUT_DIR"
echo ""

# 步骤2：列出所有session
echo "========================================"
echo "步骤2：列出所有session"
echo "========================================"
python3 "$SKILL_DIR/scripts/cli.py" list \
    --data-dir "$OUTPUT_DIR" \
    --limit 10

# 步骤3：查看第一个session的详细信息
echo "========================================"
echo "步骤3：查看session详细信息"
echo "========================================"
FIRST_SESSION=$(ls -1 "$OUTPUT_DIR"/*.json | head -1 | xargs -I {} basename {} .json)
python3 "$SKILL_DIR/scripts/cli.py" show "$FIRST_SESSION" \
    --data-dir "$OUTPUT_DIR"

# 步骤4：按模型统计
echo "========================================"
echo "步骤4：按模型统计token开销"
echo "========================================"
python3 "$SKILL_DIR/scripts/cli.py" stats-model \
    --data-dir "$OUTPUT_DIR"

# 步骤5：按日期统计
echo "========================================"
echo "步骤5：按日期统计token开销"
echo "========================================"
python3 "$SKILL_DIR/scripts/cli.py" stats-date \
    --data-dir "$OUTPUT_DIR" \
    --days 7

# 步骤6：导出FinOps报表
echo "========================================"
echo "步骤6：导出FinOps报表"
echo "========================================"
python3 "$SKILL_DIR/scripts/cli.py" export "$EXAMPLE_DIR/finops-report.json" \
    --data-dir "$OUTPUT_DIR" \
    --format json

echo ""
echo "✅ 报表已导出到: $EXAMPLE_DIR/finops-report.json"
echo ""

# 显示报表内容
if [ -f "$EXAMPLE_DIR/finops-report.json" ]; then
    echo "📊 FinOps报表内容："
    echo "========================================"
    cat "$EXAMPLE_DIR/finops-report.json" | python3 -m json.tool | head -50
    echo "..."
fi

echo ""
echo "========================================"
echo "✅ Demo完成！"
echo "========================================"
echo ""
echo "💡 提示："
echo "  - Session数据保存在: $OUTPUT_DIR/"
echo "  - FinOps报表: $EXAMPLE_DIR/finops-report.json"
echo "  - 使用 'python3 脚本/cli.py --help' 查看更多命令"
echo ""
echo "🌐 启动Web界面查看："
echo "  python3 $SKILL_DIR/scripts/webserver.py --data-dir $OUTPUT_DIR --port 8888"
echo "  然后访问: http://localhost:8888"
