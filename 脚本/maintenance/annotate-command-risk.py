#!/usr/bin/env python3
"""
扫描所有 bash 代码块，按命令风险分级标注：
  - 代码块前插入风险横幅（blockquote），按最高等级
  - 🔴 灾难性命令行追加行内 `# ⚠️` 注释
  - 已有风险标注的代码块跳过（去重）

风险等级定义见 concepts/command-risk-assessment.md。
覆盖：🔴 灾难性 / 🟠 高危 / 🟡 中危写操作。只读命令(🟢)不标注。

Fence 处理采用 open/close 状态机，避免把 ```bash 误当前一个块的 close。
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EXCLUDE_DIRS = {
    "_archives", "_archived-release-notes", "node_modules", ".venv", ".git",
    ".ruff_cache", "__pycache__", "site", "web",
    ".codebuddy", ".comate", ".qoder", ".understand-anything", ".zread",
}

# 等级常量
RED, ORANGE, YELLOW = 1, 2, 3
LEVEL_META = {
    RED: ("🔴", "灾难性操作", "含不可逆命令，执行前必须满足变更窗口+双人复核+事前备份+回滚方案"),
    ORANGE: ("🟠", "高危操作", "影响业务流量或节点状态，需变更工单+影响评估+计划回滚"),
    YELLOW: ("🟡", "中危变更", "变更集群资源状态，建议先 --dry-run 或 diff 确认"),
}
# 标准页本身不标注（避免自引用）
SELF_SKIP = "concepts/command-risk-assessment.md"

# (regex, level, label, banner_summary, inline_note)
PATTERNS = [
    (re.compile(r"kubectl\s+delete\s+(?:ns|namespace)\s"), RED,
     "kubectl delete namespace", "永久删除命名空间及全部资源，不可恢复", "不可逆：永久删除命名空间及全部资源"),
    (re.compile(r"etcdctl\s+snapshot\s+restore"), RED,
     "etcdctl snapshot restore", "用快照覆盖 etcd 数据目录，集群状态强制回退", "覆盖 etcd 数据，集群状态回退"),
    (re.compile(r"etcdctl\s+member\s+remove"), RED,
     "etcdctl member remove", "移除 etcd 成员，误删多数派会致集群不可用/丢数据", "移除 etcd 成员，可能丢数据"),
    (re.compile(r"kubeadm\s+reset"), RED,
     "kubeadm reset", "清理节点所有 K8s 配置/证书/CNI，节点脱离集群", "清理节点所有 K8s 配置"),
    (re.compile(r"kubectl\s+delete\s+pod\b[^\n]*--force"), RED,
     "kubectl delete pod --force", "强制删除 Pod，跳过优雅终止与数据刷盘", "跳过优雅终止，可能丢数据"),
    (re.compile(r"kubectl\s+delete\s+\S+[^\n]*--all\b"), RED,
     "kubectl delete --all", "批量删除某类全部资源，波及面巨大", "批量删除，波及面大"),
    (re.compile(r"rm\s+-rf?\s+(?:/|/var|/etc|/root|/opt|/home|\$|[A-Z]:|[~])"), RED,
     "rm -rf (系统/数据路径)", "删除系统或数据文件，可能摧毁节点或丢失全部数据", "删除系统/数据文件"),
    (re.compile(r"docker\s+(?:system\s+prune|rm\s+-f|rmi\s+-f|volume\s+(?:prune|rm))"), RED,
     "docker prune/rm -f", "强制清理镜像/容器/卷，运行中容器会被杀", "强制清理，可能杀运行中容器"),
    (re.compile(r"helm\s+uninstall"), RED,
     "helm uninstall", "删除 release 及其释放的所有资源", "删除 release 及关联资源"),
    (re.compile(r"kubectl\s+delete\s+(?:pv|pvc)\b[^\n]*--all"), RED,
     "kubectl delete pv/pvc --all", "批量删除持久卷，可能永久丢失存储数据", "批量删卷，可能丢数据"),
    (re.compile(r"kubectl\s+drain\b"), ORANGE,
     "kubectl drain", "驱逐节点所有 Pod，业务流量受影响", ""),
    (re.compile(r"kubectl\s+cordon\b"), ORANGE,
     "kubectl cordon", "标记节点不可调度", ""),
    (re.compile(r"kubectl\s+taint\s+nodes"), ORANGE,
     "kubectl taint nodes", "变更污点影响 Pod 调度", ""),
    (re.compile(r"kubectl\s+scale\b[^\n]*replicas[=\s]+0\b"), ORANGE,
     "kubectl scale --replicas=0", "缩容到 0，立即停服", ""),
    (re.compile(r"sysctl\s+-w"), ORANGE,
     "sysctl -w", "实时修改内核参数，全局生效", ""),
    (re.compile(r"systemctl\s+(?:stop|restart|disable)\b"), ORANGE,
     "systemctl stop/restart", "停止/重启系统服务，影响节点上所有容器", ""),
    (re.compile(r"(?:chmod|chown)\s+-R\b"), ORANGE,
     "chmod/chown -R", "递归改权限，误操作破坏系统文件访问", ""),
    (re.compile(r"iptables\s+(?:-F\b|-X\b|-P\s+\S+\s+DROP)"), ORANGE,
     "iptables -F/-P DROP", "清空/改防火墙规则，可能立即断网(含SSH)", ""),
    (re.compile(r"kubectl\s+(?:apply|create|replace)\b"), YELLOW,
     "kubectl apply/create/replace", "创建/变更集群资源", ""),
    (re.compile(r"kubectl\s+delete\b"), YELLOW,
     "kubectl delete", "删除资源（可由声明式清单重建）", ""),
    (re.compile(r"kubectl\s+(?:edit|patch)\b"), YELLOW,
     "kubectl edit/patch", "修改运行中的资源", ""),
    (re.compile(r"kubectl\s+(?:label|annotate)\b"), YELLOW,
     "kubectl label/annotate", "改元数据可能影响选择器/控制器", ""),
    (re.compile(r"helm\s+(?:upgrade|install)\b"), YELLOW,
     "helm upgrade/install", "部署/升级 release", ""),
    (re.compile(r"kubectl\s+exec\b"), YELLOW,
     "kubectl exec", "进入容器执行命令，可能改变容器状态", ""),
    (re.compile(r"kubectl\s+rollout\s+(?:undo|restart)\b"), YELLOW,
     "kubectl rollout undo/restart", "触发滚动变更，影响副本", ""),
]

BASH_LANGS = ("", "bash", "sh", "shell", "console", "zsh", "bashwrap")
INLINE_COMMENT_RE = re.compile(r"#\s*⚠️")


def analyze_block(lines: list[str]) -> tuple[int | None, dict, list[int]]:
    """返回 (最高等级, {level: set(label)}, [需加行内注释的行索引])。"""
    top = None
    hit_labels: dict[int, set] = {RED: set(), ORANGE: set(), YELLOW: set()}
    inline_lines: list[int] = []
    for idx, line in enumerate(lines):
        for rx, level, label, _summary, note in PATTERNS:
            if rx.search(line):
                hit_labels.setdefault(level, set()).add(label)
                if top is None or level < top:
                    top = level
                if level == RED and note and not INLINE_COMMENT_RE.search(line) and not line.rstrip().endswith("\\"):
                    inline_lines.append(idx)
                break  # 一行只记最高等级（PATTERNS 按 🔴→🟠→🟡 排序，首个命中即最高）
    return top, hit_labels, inline_lines


def build_banner(top: int, hit_labels: dict) -> list[str]:
    emoji, name, desc = LEVEL_META[top]
    lines_out = [f"> ⚠️ **{emoji} {name}** — {desc}"]
    shown = 0
    for level in (RED, ORANGE, YELLOW):
        for label in sorted(hit_labels.get(level, set())):
            summary = next(s for rx, lv, lb, s, _ in PATTERNS if lb == label and lv == level)
            lines_out.append(f"> - `{label}`：{summary}")
            shown += 1
            if shown >= 4:
                break
        if shown >= 4:
            break
    return lines_out


def has_existing_banner(out_lines: list[str]) -> bool:
    """检查紧邻代码块前的 blockquote 块是否已是风险横幅（识别多行横幅）。"""
    in_quote = False
    for line in reversed(out_lines):
        s = line.strip()
        if s == "":
            if in_quote:
                break  # blockquote 块已结束
            continue  # 跳过末尾空行
        if s.startswith(">"):
            in_quote = True
            if s.startswith("> ⚠️") or "RISK:" in s:
                return True
            continue  # 继续往前扫 blockquote 块的其他行
        break  # 非空非 blockquote，停止
    return False


def add_inline_comment(line: str) -> str:
    if INLINE_COMMENT_RE.search(line) or line.rstrip().endswith("\\"):
        return line
    for rx, level, label, _summary, note in PATTERNS:
        if level == RED and note and rx.search(line):
            return f"{line.rstrip()}  # ⚠️ {note}"
    return line


def process_file(path: Path, write: bool, verbose: bool) -> int:
    """状态机版：正确配对 open/close fence，只分析 bash 类代码块。"""
    raw = path.read_text(encoding="utf-8").split("\n")
    out: list[str] = []
    blocks_marked = 0
    i = 0
    in_block = False
    block_lang = ""
    block_open_line = ""
    content: list[str] = []

    while i < len(raw):
        line = raw[i]
        is_fence = line.lstrip().startswith("```")

        if not in_block:
            if is_fence:
                # open
                in_block = True
                block_lang = line.lstrip()[3:].strip().lower()
                block_open_line = line
                content = []
            else:
                out.append(line)
            i += 1
            continue

        # in_block == True
        if is_fence:
            # close
            in_block = False
            if block_lang in BASH_LANGS:
                top, hit_labels, inline_lines = analyze_block(content)
                if top is not None and not has_existing_banner(out):
                    banner = build_banner(top, hit_labels)
                    if out and out[-1].strip() != "" and not out[-1].strip().startswith(">"):
                        out.append("")
                    out.extend(banner)
                    out.append("")
                    blocks_marked += 1
                    if verbose:
                        rel = path.relative_to(ROOT)
                        print(f"  {rel} L{i-len(content)} {LEVEL_META[top][0]} {sorted(hit_labels[top])}")
                out.append(block_open_line)
                inline_set = set(inline_lines)
                for ci, cline in enumerate(content):
                    out.append(add_inline_comment(cline) if ci in inline_set else cline)
            else:
                # 非 bash 块，原样输出
                out.append(block_open_line)
                out.extend(content)
            out.append(line)  # close fence
            i += 1
            continue

        content.append(line)
        i += 1

    # 未闭合块兜底
    if in_block:
        out.append(block_open_line)
        out.extend(content)

    new_text = "\n".join(out)
    if write and new_text != path.read_text(encoding="utf-8"):
        try:
            path.write_text(new_text, encoding="utf-8")
        except PermissionError:
            return 0
    return blocks_marked


def iter_md_files() -> list[Path]:
    out = []
    for p in ROOT.rglob("*.md"):
        if any(part in EXCLUDE_DIRS for part in p.parts):
            continue
        if str(p.relative_to(ROOT)) == SELF_SKIP:
            continue
        out.append(p)
    return sorted(out)


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--dry-run", action="store_true")
    ap.add_argument("--verbose", action="store_true")
    args = ap.parse_args()
    total = 0
    files = 0
    for p in iter_md_files():
        n = process_file(p, write=False, verbose=args.verbose)
        if n > 0:
            files += 1
            total += n
            if not args.dry_run:
                process_file(p, write=True, verbose=False)
    mode = "DRY-RUN" if args.dry_run else "EXECUTED"
    print(f"=== {mode}: {files} 文件, {total} 个代码块已标注风险横幅 ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
