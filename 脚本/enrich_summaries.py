#!/usr/bin/env python3
"""
补全质量差的 summary（空 / 仅标题 / 仅符号）。

策略（保守，只改好不改坏）：
  对 frontmatter.summary 为空、或以 "## " 开头、或过短的页面，
  从正文提取首个「散文段落」作为 summary：
    - 跳过 frontmatter、标题、引用、列表项、表格、代码、树形图(├└│─)、分隔线
    - 要求：以中文或字母开头、长度 >=40、为完整句子而非碎片
  找不到合格段落则保持原样（不降质）。
  写回源文件 frontmatter，保留其它字段与正文不变。

用法:
  python scripts/enrich_summaries.py            # 预览(dry-run)
  python scripts/enrich_summaries.py --write    # 实际写回
"""
import argparse
import re
import sys
from pathlib import Path

_FM_RE = re.compile(r'^---\n(.*?)\n---\n', re.DOTALL)
_SUMMARY_RE = re.compile(r'^summary:\s*(.*)$', re.MULTILINE)


def is_bad_summary(s: str) -> bool:
    s = (s or '').strip().strip("'\"")
    if not s:
        return True
    if s.startswith('## ') or s.startswith('# '):
        return True
    if len(s) < 12:
        return True
    return False


def _is_prose(s: str) -> bool:
    """严格判定一行是否为可读散文（排除 yaml/模板/映射/树形/列表/代码）。"""
    if len(s) < 40:
        return False
    # yaml / 配置 / 模板特征
    if re.match(r'^[A-Za-z_][\w-]*\s*:\s', s):  # yaml key: value
        return False
    if any(tok in s for tok in (': [', ': ./', '{{', '}}', '${', '->', '=>', '│', '├', '└')):
        return False
    # 树形/框线/引用/列表/表格/标题/代码起始
    if s.startswith(('```', '|', '>', '-', '*', '#', '!', '<', '[')):
        return False
    # 多重枚举 "a / b / c"
    if s.count(' / ') >= 2:
        return False
    # 引号包裹的键值映射行
    if s.count('"') >= 4 and s.count(':') >= 1:
        return False
    # 特殊字符占比过高（代码/符号串）
    letters = len(re.findall(r'[\u4e00-\u9fa5A-Za-z]', s))
    if letters / max(len(s), 1) < 0.5:
        return False
    # 必须含足量中文或英文词
    if not (re.search(r'[\u4e00-\u9fa5]{6,}', s) or re.search(r'[A-Za-z]{10,}', s)):
        return False
    return True


def extract_prose_summary(body: str) -> str:
    """从正文提取首个合格散文段落，截断到 <=180 字符。"""
    for raw in body.splitlines():
        s = raw.strip()
        if not _is_prose(s):
            continue
        if len(s) > 180:
            cut = s[:180]
            # 尽量在标点处收束
            for pad, pat in [(cut, r'[。！？；，.!?;]')]:
                matches = list(re.finditer(pat, pad))
                if matches and matches[-1].start() > 40:
                    cut = cut[:matches[-1].start() + 1]
                    break
            s = cut
        return s
    return ''


def process(vault: Path, write: bool) -> tuple:
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    from export_corpus_for_nas import load_profile, should_include  # noqa: E402

    profile = load_profile(vault, 'rag-full-profile.yaml')

    scanned = candidates = improved = no_source = 0
    for p in vault.rglob('*.md'):
        rel = str(p.relative_to(vault))
        if not should_include(rel, profile):  # 仅处理会进入语料的页面
            continue
        scanned += 1
        try:
            text = p.read_text(encoding='utf-8')
        except Exception:
            continue
        m = _FM_RE.match(text)
        if not m:
            continue
        fm, body = m.group(1), text[m.end():]
        sm = _SUMMARY_RE.search(fm)
        cur = sm.group(1).strip().strip("'\"") if sm else ''
        if not is_bad_summary(cur):
            continue
        candidates += 1
        new = extract_prose_summary(body)
        if not new or is_bad_summary(new):
            if not new:
                no_source += 1
            continue
        if sm:
            new_fm = fm[:sm.start(1)] + f"'{new}'" + fm[sm.end(1):]
        else:
            new_fm = fm.rstrip() + f"\nsummary: '{new}'"
        new_text = f'---\n{new_fm}\n---\n' + body
        improved += 1
        if write:
            p.write_text(new_text, encoding='utf-8')
        elif improved <= 12:
            print(f"  [DRY] {rel}")
            print(f"      旧: {cur!r}")
            print(f"      新: {new!r}")
    return scanned, candidates, improved, no_source


def main():
    ap = argparse.ArgumentParser(description='补全质量差的 summary')
    ap.add_argument('--write', action='store_true', help='实际写回（默认 dry-run）')
    ap.add_argument('--vault', '-v', default=str(Path(__file__).resolve().parent.parent))
    args = ap.parse_args()
    vault = Path(args.vault)
    scanned, candidates, improved, no_source = process(vault, args.write)
    mode = 'WRITE' if args.write else 'DRY-RUN'
    print(f"\n[{mode}] 扫描 {scanned} | 候选(差summary) {candidates} | "
          f"成功补全 {improved} | 无可用正文 {no_source}")


if __name__ == '__main__':
    main()
