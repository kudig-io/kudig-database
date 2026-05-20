#!/bin/bash
# ============================================================
# Kudig-DB 智能体语料导出脚本
# 用法: ./export-corpus.sh [选项]
# ============================================================

set -e

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 默认配置
OUTPUT_DIR="kudig-corpus-export"
FORMAT="full"  # full | agent | lite
COMPRESS=true
INCLUDE_DOMAIN=true

# 帮助信息
show_help() {
    cat << EOF
${BLUE}Kudig-DB 智能体语料导出脚本${NC}

${YELLOW}用法:${NC}
    $0 [选项]

${YELLOW}选项:${NC}
    -o, --output DIR      输出目录 (默认: kudig-corpus-export)
    -f, --format FORMAT   导出格式: full | agent | lite (默认: full)
    -c, --compress        压缩导出文件
    -n, --no-domain      不包含 domain-* 目录
    -h, --help            显示帮助信息

${YELLOW}导出格式说明:${NC}
    full    - 完整语料 (FTA + FEBM + Skills + Structural + domain-*)
    agent   - Agent 核心语料 (FTA + FEBM + Skills)
    lite    - 轻量语料 (仅 FTA + Skills)

${YELLOW}示例:${NC}
    $0                           # 导出完整语料
    $0 -f agent -c               # 导出 Agent 核心语料并压缩
    $0 -o my-corpus -f lite      # 导出轻量语料到 my-corpus 目录

EOF
}

# 解析参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -o|--output)
            OUTPUT_DIR="$2"
            shift 2
            ;;
        -f|--format)
            FORMAT="$2"
            shift 2
            ;;
        -c|--compress)
            COMPRESS=true
            shift
            ;;
        -n|--no-domain)
            INCLUDE_DOMAIN=false
            shift
            ;;
        -h|--help)
            show_help
            exit 0
            ;;
        *)
            echo -e "${RED}未知参数: $1${NC}"
            show_help
            exit 1
            ;;
    esac
done

# 日志函数
log_info() { echo -e "${BLUE}[INFO]${NC} $1"; }
log_success() { echo -e "${GREEN}[SUCCESS]${NC} $1"; }
log_warn() { echo -e "${YELLOW}[WARN]${NC} $1"; }
log_error() { echo -e "${RED}[ERROR]${NC} $1"; }

# 检查环境
check_env() {
    log_info "检查运行环境..."

    if [[ ! -d ".git" ]]; then
        log_error "请在 Kudig-DB 仓库根目录运行此脚本"
        exit 1
    fi

    # 检查必要目录
    local required_dirs=("topic-fta" "topic-febm" "topic-skills")
    for dir in "${required_dirs[@]}"; do
        if [[ ! -d "$dir" ]]; then
            log_error "缺少必要目录: $dir"
            exit 1
        fi
    done

    log_success "环境检查通过"
}

# 创建输出目录
setup_output() {
    log_info "创建输出目录: $OUTPUT_DIR"
    rm -rf "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"
    mkdir -p "$OUTPUT_DIR"/{fta,febm,skills,structural,domain,metadata}
}

# 导出函数
export_dir() {
    local src=$1
    local dest=$2
    local desc=$3

    if [[ -d "$src" ]]; then
        log_info "导出 $desc..."
        rsync -a --quiet "$src/" "$dest/"
        local count=$(find "$dest" -name "*.md" 2>/dev/null | wc -l)
        log_success "导出 $count 个文件: $src → $dest"
    else
        log_warn "跳过 (目录不存在): $src"
    fi
}

# 生成元数据
generate_metadata() {
    log_info "生成语料元数据..."

    cat > "$OUTPUT_DIR/metadata/corpus-info.json" << EOF
{
    "export_date": "$(date -u +%Y-%m-%dT%H:%M:%SZ)",
    "format": "$FORMAT",
    "version": "1.0",
    "description": "Kudig-DB AI Agent Corpus",
    "statistics": {
        "fta_documents": $(find "$OUTPUT_DIR/fta" -name "*.md" 2>/dev/null | wc -l),
        "febm_documents": $(find "$OUTPUT_DIR/febm" -name "*.md" 2>/dev/null | wc -l),
        "skills_documents": $(find "$OUTPUT_DIR/skills" -name "*.md" 2>/dev/null | wc -l),
        "structural_documents": $(find "$OUTPUT_DIR/structural" -name "*.md" 2>/dev/null | wc -l),
        "domain_documents": $(find "$OUTPUT_DIR/domain" -name "*.md" 2>/dev/null | wc -l)
    }
}
EOF

    log_success "元数据已生成"
}

# 生成 chunk 元数据 (参考 corpus-config/rag-chunking-strategy.md)
generate_chunk_metadata() {
    log_info "生成分块元数据规范..."

    cat > "$OUTPUT_DIR/metadata/chunking-strategy.json" << 'EOF'
{
    "chunk_strategies": {
        "fta": {
            "method": "by_h3_header",
            "chunk_size": 1500,
            "overlap": 10,
            "description": "每个底事件 (BE-*) 独立分块"
        },
        "febm": {
            "method": "by_h2_header",
            "chunk_size": 2000,
            "overlap": 15,
            "description": "每个取证步骤独立分块"
        },
        "skills": {
            "method": "by_section",
            "chunk_size": 3000,
            "overlap": 5,
            "description": "每个技能动作独立分块"
        },
        "structural": {
            "method": "by_h2_header",
            "chunk_size": 768,
            "overlap": 20,
            "description": "按排查步骤分块"
        },
        "domain": {
            "method": "by_h2_header",
            "chunk_size": 2000,
            "overlap": 10,
            "description": "按章节分块，保持知识完整性"
        }
    },
    "embedding_models": [
        "text-embedding-3-large (3072维)",
        "bge-large-zh-v1.5 (1024维, 中文优先)"
    ]
}
EOF

    log_success "分块策略已生成"
}

# 生成 QA 对话模板
generate_qa_template() {
    log_info "生成 QA 对话模板..."

    cat > "$OUTPUT_DIR/metadata/qa-template.json" << 'EOF'
{
    "qa_conversation_template": {
        "system_prompt": "你是一个 Kubernetes 运维专家，擅长故障诊断和问题排查。",
        "user_template": "问题: {question}\n现象: {symptom}",
        "assistant_template": "分析路径: {fta_path}\n根因: {root_cause}\n修复步骤: {solution}",
        "example_pairs": [
            {
                "question": "Pod 反复重启，exit code 137，如何排查？",
                "symptom": "CrashLoopBackOff, OOMKilled, restart count > 5",
                "fta_path": "TE-2 → IE-2.1 → BE-2.3",
                "root_cause": "内存限制配置过小",
                "solution": "1. 检查 memory limit\n2. 调整 JVM heap 大小\n3. 验证修复"
            }
        ]
    }
}
EOF

    log_success "QA 模板已生成"
}

# 生成工具调用轨迹模板
generate_tool_template() {
    log_info "生成工具调用轨迹模板..."

    cat > "$OUTPUT_DIR/metadata/tool-trace-template.json" << 'EOF'
{
    "tool_call_template": {
        "trace_format": "jsonl",
        "fields": [
            "timestamp",
            "tool_name",
            "arguments",
            "result",
            "success"
        ],
        "example": {
            "timestamp": "2026-05-18T10:30:00Z",
            "tool_name": "kubectl_exec",
            "arguments": {
                "pod": "nginx-abcde",
                "command": "jcmd 1 GC.heap_info"
            },
            "result": "Heap: 1.2GB used, 2GB max",
            "success": true
        }
    }
}
EOF

    log_success "工具调用模板已生成"
}

# 生成 README
generate_readme() {
    log_info "生成导出说明文档..."

    cat > "$OUTPUT_DIR/README.md" << 'EOF'
# Kudig-DB 语料导出包

## 目录结构

```
kudig-corpus-export/
├── fta/           # FTA 故障树文档 (67篇)
├── febm/          # FEBM 取证文档 (10篇)
├── skills/        # Skills 自动化技能 (30篇)
├── structural/    # Structural 详细排查 (63篇)
├── domain/        # Domain 深度知识 (按需)
├── metadata/      # 元数据与配置
│   ├── corpus-info.json
│   ├── chunking-strategy.json
│   ├── qa-template.json
│   └── tool-trace-template.json
└── README.md
```

## 使用方式

### 1. RAG 检索
```python
from langchain_community.document_loaders import DirectoryLoader
loader = DirectoryLoader('./fta/', glob='**/*.md')
docs = loader.load()
```

### 2. Agent 微调
```python
# 使用 qa-template.json 构建 SFT 数据集
```

### 3. 向量化
```python
from langchain.embeddings import OpenAIEmbeddings
embeddings = OpenAIEmbeddings(model='text-embedding-3-large')
```

## 分块策略

详见 `metadata/chunking-strategy.json`

## 导出日期

详见 `metadata/corpus-info.json`
EOF

    log_success "README 已生成"
}

# 压缩输出
compress_output() {
    if [[ "$COMPRESS" == true ]]; then
        log_info "压缩导出文件..."

        cd ..
        tar -czvf "${OUTPUT_DIR}.tar.gz" "$OUTPUT_DIR"
        rm -rf "$OUTPUT_DIR"

        local size=$(du -h "${OUTPUT_DIR}.tar.gz" | cut -f1)
        log_success "压缩完成: ${OUTPUT_DIR}.tar.gz ($size)"

        OUTPUT_DIR="${OUTPUT_DIR}.tar.gz"
    fi
}

# 显示统计
show_stats() {
    echo ""
    echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
    echo -e "${GREEN}导出完成！${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
    echo ""
    echo -e "${YELLOW}输出位置:${NC} $OUTPUT_DIR"
    echo ""
    echo -e "${YELLOW}统计信息:${NC}"
    echo "  FTA 文档:     $(find "$OUTPUT_DIR/fta" -name "*.md" 2>/dev/null | wc -l) 篇"
    echo "  FEBM 文档:    $(find "$OUTPUT_DIR/febm" -name "*.md" 2>/dev/null | wc -l) 篇"
    echo "  Skills 文档:  $(find "$OUTPUT_DIR/skills" -name "*.md" 2>/dev/null | wc -l) 篇"
    echo "  Structural:   $(find "$OUTPUT_DIR/structural" -name "*.md" 2>/dev/null | wc -l) 篇"
    echo "  Domain:       $(find "$OUTPUT_DIR/domain" -name "*.md" 2>/dev/null | wc -l) 篇"
    echo ""
    echo -e "${YELLOW}使用方式:${NC}"
    echo "  1. RAG 检索: 使用 langchain/LlamaIndex 加载 .md 文件"
    echo "  2. 微调 SFT: 参考 metadata/qa-template.json 构建对话集"
    echo "  3. 向量化:   参考 metadata/chunking-strategy.json 分块"
    echo ""
}

# 主流程
main() {
    echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
    echo -e "${BLUE}  Kudig-DB 智能体语料导出工具${NC}"
    echo -e "${BLUE}═══════════════════════════════════════════════════${NC}"
    echo ""

    check_env
    setup_output

    case $FORMAT in
        full)
            log_info "导出模式: 完整语料"
            export_dir "topic-fta" "$OUTPUT_DIR/fta" "FTA 故障树"
            export_dir "topic-febm" "$OUTPUT_DIR/febm" "FEBM 取证"
            export_dir "topic-skills" "$OUTPUT_DIR/skills" "Skills 技能"
            export_dir "topic-structural-trouble-shooting" "$OUTPUT_DIR/structural" "Structural 排查"
            if [[ "$INCLUDE_DOMAIN" == true ]]; then
                export_dir "domain-12-troubleshooting" "$OUTPUT_DIR/domain" "Domain 知识"
            fi
            ;;
        agent)
            log_info "导出模式: Agent 核心语料"
            export_dir "topic-fta" "$OUTPUT_DIR/fta" "FTA 故障树"
            export_dir "topic-febm" "$OUTPUT_DIR/febm" "FEBM 取证"
            export_dir "topic-skills" "$OUTPUT_DIR/skills" "Skills 技能"
            ;;
        lite)
            log_info "导出模式: 轻量语料"
            export_dir "topic-fta" "$OUTPUT_DIR/fta" "FTA 故障树"
            export_dir "topic-skills" "$OUTPUT_DIR/skills" "Skills 技能"
            ;;
        *)
            log_error "未知格式: $FORMAT"
            exit 1
            ;;
    esac

    generate_metadata
    generate_chunk_metadata
    generate_qa_template
    generate_tool_template
    generate_readme
    compress_output
    show_stats
}

main "$@"