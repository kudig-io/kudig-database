
# =============================================================================
# Command-Output Diagnosis Corpus Targets
# =============================================================================

CORPUS_DIR := 31-脚本/corpus-generator
CORPUS_OUTPUT := 19-故障诊断/10-QA语料/generated
SKILLS_DIR := 19-故障诊断/08-技能体系
FTA_DIR := 19-故障诊断/06-FTA故障树/list

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

