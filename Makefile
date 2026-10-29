
# =============================================================================
# Command-Output Diagnosis Corpus Targets
# =============================================================================

CORPUS_DIR := code/scripts/corpus-generator
CORPUS_OUTPUT := wiki/19-故障诊断/10-QA语料/generated
SKILLS_DIR := wiki/19-故障诊断/08-技能体系
FTA_DIR := wiki/19-故障诊断/06-FTA故障树/list

.PHONY: corpus-generate corpus-generate-all corpus-validate corpus-stats corpus-clean

corpus-generate-p0: ## 生成 P0 优先级 I-O 语料（核心故障场景）
	python3 $(CORPUS_DIR)/generate.py --priority P0 --output $(CORPUS_OUTPUT)/

corpus-generate-p1: ## 生成 P1 优先级 I-O 语料（扩展场景）
	python3 $(CORPUS_DIR)/generate.py --priority P1 --output $(CORPUS_OUTPUT)/

corpus-generate-p2: ## 生成 P2 优先级 I-O 语料（高级/边缘场景）
	python3 $(CORPUS_DIR)/generate.py --priority P2 --output $(CORPUS_OUTPUT)/

corpus-generate-all: ## 生成全量 I-O 语料（P0+P1+P2）
	python3 $(CORPUS_DIR)/generate.py --priority all --output $(CORPUS_OUTPUT)/

corpus-validate: ## 验证语料覆盖率
	python3 $(CORPUS_DIR)/validators/coverage_checker.py \
		--skills-dir $(SKILLS_DIR) \
		--fta-dir $(FTA_DIR) \
		--corpus-dir $(CORPUS_OUTPUT) \
		--output $(CORPUS_OUTPUT)/coverage-report-latest.json

corpus-stats: ## 统计语料规模
	@echo "=== Command-Output Corpus Statistics ==="
	@echo "I-O pairs (JSON):"
	@find $(CORPUS_OUTPUT) -name "*.json" -exec python3 -c "import json,sys; d=json.load(open(sys.argv[1])); print(f'  {sys.argv[1]}: {len(d)} pairs')" {} \;
	@echo ""
	@echo "I-O pairs (Markdown YAML blocks):"
	@find $(CORPUS_OUTPUT) -name "*.md" -exec grep -c "^io_pair_id:" {} \; | awk -F: '{s+=$$1} END {printf "  Total: %d pairs\n", s}'

corpus-clean: ## 清理生成的语料文件
	rm -f $(CORPUS_OUTPUT)/command-output-diagnosis-*.md
	rm -f $(CORPUS_OUTPUT)/command-output-diagnosis-*.json
	rm -f $(CORPUS_OUTPUT)/command-output-diagnosis-*.yaml
	rm -f $(CORPUS_OUTPUT)/coverage-report-*.json

# =============================================================================
# GTM Landing Page
# =============================================================================

GTM_DIR := code/GTM
GTM_SYNC := code/scripts/maintenance/gtm-sync-stats.py

.PHONY: gtm-sync gtm-check

gtm-sync: ## 按仓库真实计数刷新 GTM 页面并同步到 dist/
	python3 $(GTM_SYNC)
	mkdir -p $(GTM_DIR)/dist
	cp $(GTM_DIR)/index.html $(GTM_DIR)/dist/index.html
	cp $(GTM_DIR)/og.jpg $(GTM_DIR)/dist/og.jpg

gtm-check: ## 校验 GTM 页面数据是否为最新（CI/nightly 用）
	python3 $(GTM_SYNC) --check

# =============================================================================
# 上游源码检出（code/repo/ 不入库，按清单重建）
# 清单：maintaining/02-元数据/repos.yaml
# =============================================================================

REPO_SYNC := code/scripts/maintenance/repo-sync.py

.PHONY: repo-sync repo-status repo-update repo-update-latest

repo-sync: ## 克隆清单中缺失的上游源码树到 code/repo/
	python3 $(REPO_SYNC) sync

repo-status: ## 查看 code/repo/ 各源码树的 ref / HEAD / 体积 / 本地改动
	python3 $(REPO_SYNC) status

repo-update: ## 已有源码树按清单 ref 重新检出
	python3 $(REPO_SYNC) sync --update

repo-update-latest: ## 解析上游最新稳定版本（Release/tag）、写回清单并刷新源码树
	python3 $(REPO_SYNC) sync --latest --update --write

