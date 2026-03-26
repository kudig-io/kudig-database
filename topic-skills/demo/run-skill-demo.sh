#!/usr/bin/env bash
# ============================================================================
# run-skill-demo.sh — Skills Demo 交互式运行器
# Interactive Skills Demo Runner
# ============================================================================
# 用法 / Usage:
#   bash run-skill-demo.sh              # 交互式菜单
#   bash run-skill-demo.sh 1            # 直接运行场景 1
#   bash run-skill-demo.sh all          # 顺序运行所有场景
# ============================================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# ---- 颜色 / Colors ----
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
MAGENTA='\033[0;35m'
BOLD='\033[1m'
DIM='\033[2m'
NC='\033[0m'

# ---- 前置检查 / Preflight ----
preflight_check() {
    echo -e "${BLUE}前置检查 / Preflight check...${NC}"
    local ok=true

    if ! command -v kubectl &>/dev/null; then
        echo -e "  ${RED}✗ kubectl 未安装${NC}"
        ok=false
    fi

    if ! kubectl cluster-info &>/dev/null 2>&1; then
        echo -e "  ${RED}✗ 无法连接 Kubernetes 集群${NC}"
        echo -e "  ${YELLOW}请先运行: bash setup-kind-cluster.sh${NC}"
        ok=false
    fi

    # 确保 skill-demo namespace 存在
    kubectl create namespace skill-demo --dry-run=client -o yaml | kubectl apply -f - &>/dev/null

    if [[ "${ok}" == "false" ]]; then
        exit 1
    fi
    echo -e "  ${GREEN}✓ 集群就绪${NC}"
    echo ""
}

# ---- 菜单 / Menu ----
show_menu() {
    echo -e "${CYAN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${CYAN}║       🎯 Skills Demo — 本地运行工单诊断技能                 ║${NC}"
    echo -e "${CYAN}║       Interactive Kubernetes Skill Execution Demo            ║${NC}"
    echo -e "${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}║  每个场景完整演示 Skill 执行闭环:                             ║${NC}"
    echo -e "${CYAN}║  故障注入 → 症状检测 → 快速分级 → 诊断 → 根因 → 修复 → 验证  ║${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}╠══════════════════════════════════════════════════════════════╣${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}║  ${BOLD}${GREEN}[1]${NC}${CYAN} 节点 Cordon NotReady    ${DIM}SKILL-NODE-001 / RC-012${NC}${CYAN}        ║${NC}"
    echo -e "${CYAN}║      ${DIM}cordon 节点 → 诊断 → uncordon → 验证${NC}${CYAN}                ║${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}║  ${BOLD}${GREEN}[2]${NC}${CYAN} Pod CrashLoopBackOff   ${DIM}SKILL-POD-001${NC}${CYAN}                   ║${NC}"
    echo -e "${CYAN}║      ${DIM}错误启动命令 → 日志分析 → 修正配置 → 验证${NC}${CYAN}            ║${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}║  ${BOLD}${GREEN}[3]${NC}${CYAN} Pod Pending            ${DIM}SKILL-POD-002${NC}${CYAN}                   ║${NC}"
    echo -e "${CYAN}║      ${DIM}资源超限 → 调度失败分析 → 调整资源 → 验证${NC}${CYAN}            ║${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}║  ${BOLD}${GREEN}[4]${NC}${CYAN} DNS 解析故障           ${DIM}SKILL-NET-001${NC}${CYAN}                   ║${NC}"
    echo -e "${CYAN}║      ${DIM}CoreDNS 缩容 → DNS 失败 → 恢复 → 验证${NC}${CYAN}              ║${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}║  ${BOLD}${GREEN}[5]${NC}${CYAN} Service 无 Endpoints   ${DIM}SKILL-NET-002${NC}${CYAN}                   ║${NC}"
    echo -e "${CYAN}║      ${DIM}selector 不匹配 → 排查 → 修正 → 验证${NC}${CYAN}               ║${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}║  ${BOLD}${YELLOW}[A]${NC}${CYAN} 顺序运行所有场景 / Run all scenarios${NC}${CYAN}              ║${NC}"
    echo -e "${CYAN}║  ${BOLD}${RED}[Q]${NC}${CYAN} 退出 / Quit${NC}${CYAN}                                         ║${NC}"
    echo -e "${CYAN}║                                                              ║${NC}"
    echo -e "${CYAN}╚══════════════════════════════════════════════════════════════╝${NC}"
}

run_scenario() {
    local num="$1"
    local script="${SCRIPT_DIR}/scenarios/0${num}-*.sh"
    local file
    file=$(ls ${script} 2>/dev/null | head -1)

    if [[ -z "${file}" || ! -f "${file}" ]]; then
        echo -e "${RED}✗ 场景 ${num} 脚本未找到${NC}"
        return 1
    fi

    echo -e "\n${MAGENTA}════════════════════════════════════════════════════════════════${NC}"
    echo -e "${MAGENTA}  运行场景 ${num}: $(basename "${file}")${NC}"
    echo -e "${MAGENTA}════════════════════════════════════════════════════════════════${NC}"

    bash "${file}"
}

run_all() {
    for i in 1 2 3 4 5; do
        run_scenario "${i}"
        if [[ "${i}" != "5" ]]; then
            echo -e "\n${YELLOW}━━━ 即将运行下一个场景 / Next scenario coming up ━━━${NC}"
            echo -e "${YELLOW}按 Enter 继续 / Press Enter to continue...${NC}"
            read -r
        fi
    done
    echo -e "\n${GREEN}╔══════════════════════════════════════════════════════════════╗${NC}"
    echo -e "${GREEN}║       🎉 全部场景完成 / All Scenarios Complete!              ║${NC}"
    echo -e "${GREEN}╚══════════════════════════════════════════════════════════════╝${NC}"
}

# ---- 主逻辑 / Main ----
main() {
    preflight_check

    # 支持命令行参数
    if [[ $# -gt 0 ]]; then
        case "$1" in
            [1-5])
                run_scenario "$1"
                exit 0
                ;;
            all|ALL|a|A)
                run_all
                exit 0
                ;;
            *)
                echo "用法 / Usage: $0 [1-5|all]"
                exit 1
                ;;
        esac
    fi

    # 交互式菜单
    while true; do
        show_menu
        echo ""
        read -rp "请选择 / Select [1-5/A/Q]: " choice
        case "${choice}" in
            1|2|3|4|5)
                run_scenario "${choice}"
                echo ""
                echo -e "${YELLOW}按 Enter 返回菜单 / Press Enter to return to menu...${NC}"
                read -r
                ;;
            a|A)
                run_all
                break
                ;;
            q|Q)
                echo -e "${GREEN}Bye! 👋${NC}"
                break
                ;;
            *)
                echo -e "${RED}无效选择 / Invalid choice${NC}"
                ;;
        esac
    done
}

main "$@"
