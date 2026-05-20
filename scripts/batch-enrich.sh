#!/usr/bin/env bash
#
# batch-enrich.sh
# Batch enrich existing documents with missing front matter fields.
#
# Adds default values for:
#   - reading_level: "intermediate" (if missing)
#   - audience: ["SRE", "Ops Engineer"]
#   - estimated_read_time: auto-calculated from word count
#
# Usage:
#   bash scripts/batch-enrich.sh [--dry-run] [--fields field1,field2] [paths...]
#
# Exit codes:
#   0 = success
#   1 = errors

set -euo pipefail

DRY_RUN=false
FIELDS="reading_level,audience,estimated_read_time"
BASE_DIR="${PWD}"

while [[ $# -gt 0 ]]; do
    case $1 in
        --dry-run|-n) DRY_RUN=true ;;
        --fields|-f) FIELDS="$2"; shift ;;
        --help|-h)
            echo "Usage: $0 [--dry-run] [--fields f1,f2] [paths...]"
            echo "  --dry-run    Preview changes without applying"
            echo "  --fields     Comma-separated fields to add (default: reading_level,audience,estimated_read_time)"
            exit 0
            ;;
        *) break ;;
    esac
    shift
done

TARGETS="${*:-$BASE_DIR}"

echo "Batch enriching front matter in: $TARGETS"
[[ "$DRY_RUN" == true ]] && echo "[DRY-RUN MODE]"

enriched=0
skipped=0

for target in $TARGETS; do
    if [[ ! -e "$target" ]]; then
        echo "[ERROR] Not found: $target" >&2
        continue
    fi

    # Find markdown files
    while IFS= read -r -d '' f; do
        if [[ "/.venv/" == *"${f}"* ]] || [[ "/site/" == *"${f}"* ]]; then
            continue
        fi

        # Check if front matter exists
        if ! head -1 "$f" | grep -q '^---'; then
            skipped=$((skipped + 1))
            continue
        fi

        # Estimate reading time (200 wpm)
        words=$(awk '{w+=NF} END {print w}' "$f" 2>/dev/null || echo "0")
        minutes=$(( (words + 199) / 200 ))
        read_time="${minutes}min"

        # Add fields if missing
        changes=0

        if [[ "$FIELDS" == *"reading_level"* ]] && ! grep -q 'reading_level:' "$f"; then
            changes=$((changes + 1))
        fi

        if [[ "$FIELDS" == *"audience"* ]] && ! grep -q 'audience:' "$f"; then
            changes=$((changes + 1))
        fi

        if [[ "$FIELDS" == *"estimated_read_time"* ]] && ! grep -q 'estimated_read_time:' "$f"; then
            changes=$((changes + 1))
        fi

        if [[ $changes -gt 0 ]]; then
            echo "[ENRICH] $f (+$changes fields, ~$read_time)"
            enriched=$((enriched + 1))

            if [[ "$DRY_RUN" == false ]]; then
                # Use Python for reliable YAML insertion
                python3 - <<PYEOF
import re, sys
f = sys.argv[1]
read_time = sys.argv[2]
try:
    with open(f, 'r+') as fp:
        c = fp.read()
        # Find insertion point (after first ---)
        lines = c.split('\n')
        idx = 0
        for i, l in enumerate(lines[1:], 1):
            if l == '---':
                idx = i + 1
                break
        if idx == 0:
            sys.exit(0)

        insert = []
        if 'reading_level:' not in c:
            insert.append('reading_level: "intermediate"')
        if 'audience:' not in c:
            insert.append('audience: ["SRE", "Ops Engineer"]')
        if 'estimated_read_time:' not in c:
            insert.append(f'estimated_read_time: "{read_time}"')

        if insert:
            lines = lines[:idx] + insert + [''] + lines[idx:]
            fp.seek(0)
            fp.truncate()
            fp.write('\n'.join(lines))
except Exception as e:
    sys.exit(1)
PYEOF
                "$f" "$read_time"
            fi
        else
            skipped=$((skipped + 1))
        fi

    done < <(find "$target" -name '*.md' -print0 2>/dev/null)
done

echo ""
echo "--- Summary ---"
echo "Enriched: $enriched"
echo "Skipped: $skipped"
[[ "$DRY_RUN" == true ]] && echo "(dry-run - no changes applied)"

[[ $enriched -gt 0 && "$DRY_RUN" == false ]] && \
    echo "Run validate-frontmatter.py to verify"

exit 0