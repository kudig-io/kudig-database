#!/bin/bash
# 批量 zread 源码分析脚本 v2
# 对 code/ 下所有源码仓库运行 zread generate，并将结果复制到 源码分析/ 目录
# 改进：兼容 current 文件含 versions/ 前缀；已有生成结果时跳过生成直接复制

set -uo pipefail

PROJECT_ROOT="/Users/allengaller/Documents/GitHub/kudig-io/kudig-database"
CODE_DIR="${PROJECT_ROOT}/code"
OUTPUT_DIR="${PROJECT_ROOT}/源码分析"
LOG_FILE="${OUTPUT_DIR}/_batch.log"

mkdir -p "${OUTPUT_DIR}"

echo "========================================" | tee -a "${LOG_FILE}"
echo "批量源码分析开始 (v2): $(date '+%Y-%m-%d %H:%M:%S')" | tee -a "${LOG_FILE}"
echo "========================================" | tee -a "${LOG_FILE}"

TOTAL=0
SUCCESS=0
FAILED=0
SKIPPED=0

copy_wiki_output() {
    local repo_path="$1"
    local repo_name="$2"
    local current_file="${repo_path}/.zread/wiki/current"

    if [ ! -f "${current_file}" ]; then
        return 1
    fi

    local version_id
    version_id=$(cat "${current_file}")

    # current 文件可能包含 versions/ 前缀，兼容两种格式
    local version_dir
    if [[ "${version_id}" == versions/* ]]; then
        version_dir="${repo_path}/.zread/wiki/${version_id}"
    else
        version_dir="${repo_path}/.zread/wiki/versions/${version_id}"
    fi

    if [ -d "${version_dir}" ] && [ -f "${version_dir}/wiki.json" ]; then
        mkdir -p "${OUTPUT_DIR}/${repo_name}"
        cp -R "${version_dir}/"* "${OUTPUT_DIR}/${repo_name}/"
        return 0
    fi
    return 1
}

# 遍历 code/ 下所有目录
for repo_path in "${CODE_DIR}"/*/; do
    repo_name=$(basename "${repo_path}")
    TOTAL=$((TOTAL + 1))

    echo "" | tee -a "${LOG_FILE}"
    echo "[${TOTAL}] 处理: ${repo_name}" | tee -a "${LOG_FILE}"
    echo "    开始时间: $(date '+%H:%M:%S')" | tee -a "${LOG_FILE}"

    # 检查源码分析目录是否已有结果
    if [ -f "${OUTPUT_DIR}/${repo_name}/wiki.json" ]; then
        echo "    状态: 已存在于源码分析/，跳过" | tee -a "${LOG_FILE}"
        SKIPPED=$((SKIPPED + 1))
        continue
    fi

    # 检查仓库内是否已有生成结果（之前运行过但未复制）
    if [ -f "${repo_path}/.zread/wiki/current" ]; then
        echo "    发现已有生成结果，直接复制..." | tee -a "${LOG_FILE}"
        if copy_wiki_output "${repo_path}" "${repo_name}"; then
            echo "    状态: ✓ 复制成功" | tee -a "${LOG_FILE}"
            SUCCESS=$((SUCCESS + 1))
        else
            echo "    状态: ✗ 复制失败（版本目录异常）" | tee -a "${LOG_FILE}"
            FAILED=$((FAILED + 1))
        fi
        echo "    结束时间: $(date '+%H:%M:%S')" | tee -a "${LOG_FILE}"
        continue
    fi

    # 需要运行 zread generate
    cd "${repo_path}"
    echo "    运行 zread generate..." | tee -a "${LOG_FILE}"

    # 处理可能存在的 drafts
    if [ -d ".zread/wiki/drafts" ]; then
        echo "    发现未完成草稿，恢复生成" | tee -a "${LOG_FILE}"
        zread generate --draft resume -y --skip-failed 2>&1 | tail -3 | tee -a "${LOG_FILE}" || true
    else
        zread generate -y --skip-failed 2>&1 | tail -3 | tee -a "${LOG_FILE}" || true
    fi

    # 复制结果
    cd "${PROJECT_ROOT}"
    if copy_wiki_output "${repo_path}" "${repo_name}"; then
        echo "    状态: ✓ 生成并复制成功" | tee -a "${LOG_FILE}"
        SUCCESS=$((SUCCESS + 1))
    else
        echo "    状态: ✗ 生成失败" | tee -a "${LOG_FILE}"
        FAILED=$((FAILED + 1))
    fi

    echo "    结束时间: $(date '+%H:%M:%S')" | tee -a "${LOG_FILE}"
done

echo "" | tee -a "${LOG_FILE}"
echo "========================================" | tee -a "${LOG_FILE}"
echo "批量源码分析完成: $(date '+%Y-%m-%d %H:%M:%S')" | tee -a "${LOG_FILE}"
echo "总计: ${TOTAL} | 成功: ${SUCCESS} | 失败: ${FAILED} | 跳过: ${SKIPPED}" | tee -a "${LOG_FILE}"
echo "========================================" | tee -a "${LOG_FILE}"
