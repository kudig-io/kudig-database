#!/usr/bin/env python3
"""repo-sync.py — 按清单检出 / 刷新 code/repo/ 下的上游源码树

清单文件（入库）：maintaining/02-元数据/repos.yaml
源码目录（.gitignore，可重建）：code/repo/<name>/

用法：
  python3 code/scripts/maintenance/repo-sync.py status
      查看各源码树是否已检出、ref 是否一致、体积与本地改动
  python3 code/scripts/maintenance/repo-sync.py sync
      克隆清单中缺失的源码树；已存在的只做校验，不改动
  python3 code/scripts/maintenance/repo-sync.py sync --update
      已存在的源码树按清单 ref 重新 fetch + checkout（detached）
  python3 code/scripts/maintenance/repo-sync.py sync --latest [--write]
      对 track: latest-release / latest-tag 的条目解析最新稳定版本
      （前者走 GitHub Releases，后者取远端最大稳定 semver tag）；
      --write 把解析结果与核对日期写回清单（保持注释与格式）
  python3 code/scripts/maintenance/repo-sync.py sync --proxy https://gh-proxy.com
      直连 github.com 不通时叠加镜像前缀（也可 export KUDIG_GIT_PROXY）

退出码：0 全部就绪；1 有条目失败（含清单非法）。
"""

from __future__ import annotations

import argparse
import datetime
import json
import os
import re
import shlex
import subprocess
import sys
import unicodedata
import urllib.error
import urllib.request
from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[3]
DEFAULT_MANIFEST = ROOT / "maintaining" / "02-元数据" / "repos.yaml"
VERSION_WATCH = ROOT / "maintaining" / "02-元数据" / "version-watch.json"
SHA_RE = re.compile(r"^[0-9a-f]{7,40}$")
SLUG_RE = re.compile(r"github\.com[/:]([^/]+)/([^/]+?)(?:\.git)?/?$")
TAG_VERSION_RE = re.compile(r"(\d+)\.(\d+)\.(\d+)$")
PRERELEASE_RE = re.compile(r"(alpha|beta|rc|pre|snapshot|dev|nightly)", re.IGNORECASE)


class GitError(RuntimeError):
    """git 命令返回非零退出码。"""


def git(args: list[str], cwd: Path | None = None, stream: bool = False) -> subprocess.CompletedProcess:
    cmd = ["git", *args]
    proc = subprocess.run(
        cmd,
        cwd=cwd,
        text=True,
        stdout=None if stream else subprocess.PIPE,
        stderr=None if stream else subprocess.PIPE,
    )
    if proc.returncode != 0:
        detail = "" if stream else (proc.stderr or proc.stdout or "").strip()
        raise GitError(f"git {' '.join(args)} 失败" + (f"：{detail}" if detail else ""))
    return proc


def load_manifest(path: Path) -> dict:
    if not path.is_file():
        raise SystemExit(f"清单不存在：{path}")
    data = yaml.safe_load(path.read_text(encoding="utf-8")) or {}
    repos = data.get("repos") or []
    if not repos:
        raise SystemExit(f"清单中没有 repos 条目：{path}")
    seen: set[str] = set()
    for repo in repos:
        for key in ("name", "url", "ref"):
            if not repo.get(key):
                raise SystemExit(f"清单条目缺少 {key} 字段：{repo}")
        if repo["name"] in seen:
            raise SystemExit(f"清单条目重名：{repo['name']}")
        seen.add(repo["name"])
    return data


def root_path(data: dict) -> Path:
    root = Path(data.get("root") or "code/repo")
    return root if root.is_absolute() else ROOT / root


def repo_depth(data: dict, repo: dict, override: int | None) -> int:
    if override is not None:
        return override
    return int(repo.get("depth", (data.get("defaults") or {}).get("depth", 1)))


def proxy_url(url: str, proxy: str | None) -> str:
    return f"{proxy.rstrip('/')}/{url}" if proxy else url


def github_slug(url: str) -> str | None:
    match = SLUG_RE.search(url.rstrip("/"))
    return f"{match.group(1)}/{match.group(2)}" if match else None


def fetch_latest_release(url: str) -> tuple[str | None, str]:
    """返回 (tag, 来源)。优先 GitHub Releases，失败回退 version-watch.json。"""
    slug = github_slug(url)
    if not slug:
        return None, "无法解析 owner/repo"
    request = urllib.request.Request(
        f"https://api.github.com/repos/{slug}/releases/latest",
        headers={"User-Agent": "kudig-repo-sync", "Accept": "application/vnd.github+json"},
    )
    try:
        with urllib.request.urlopen(request, timeout=10) as resp:
            tag = (json.load(resp) or {}).get("tag_name")
        if tag:
            return tag, "github-api"
    except (urllib.error.URLError, TimeoutError, ValueError) as exc:
        api_error = str(exc)
    else:
        api_error = "releases/latest 无 tag_name"

    if VERSION_WATCH.is_file():
        state = (json.loads(VERSION_WATCH.read_text(encoding="utf-8")) or {}).get("state") or {}
        if state.get(slug):
            return state[slug], "version-watch.json（离线回退）"
    return None, f"GitHub API 不可用：{api_error}"


def parse_tag_version(tag: str) -> tuple[int, int, int] | None:
    """从 tag 提取末尾 semver，兼容 v1.2.3 / alibaba-cloud-csi-driver-1.2.3 等前缀。"""
    match = TAG_VERSION_RE.search(tag)
    return tuple(int(part) for part in match.groups()) if match else None


def fetch_latest_tag(url: str) -> tuple[str | None, str]:
    """返回 (tag, 来源)。取远端最大稳定 semver tag，适用于只打 tag 不发 Release 的仓库。"""
    proc = subprocess.run(["git", "ls-remote", "--tags", "--refs", url], text=True, capture_output=True)
    if proc.returncode != 0:
        return None, f"git ls-remote 失败：{(proc.stderr or '').strip()[:120]}"
    tags = [line.split("refs/tags/", 1)[1] for line in proc.stdout.splitlines() if "refs/tags/" in line]
    candidates = [
        (version, tag)
        for tag in tags
        if (version := parse_tag_version(tag)) and not PRERELEASE_RE.search(tag)
    ]
    if not candidates:
        return None, "无稳定 semver tag"
    return max(candidates)[1], "git-tags"


def rev_parse(target: Path, rev: str) -> str | None:
    proc = subprocess.run(["git", "-C", str(target), "rev-parse", "--verify", "--quiet", rev],
                          text=True, capture_output=True)
    if proc.returncode != 0:
        return None
    return proc.stdout.strip() or None


def resolve_ref(target: Path, ref: str) -> str | None:
    candidates = (f"refs/tags/{ref}^{{commit}}", f"refs/remotes/origin/{ref}", f"refs/heads/{ref}", f"{ref}^{{commit}}")
    for candidate in candidates:
        commit = rev_parse(target, candidate)
        if commit:
            return commit
    return None


def is_synced(target: Path, ref: str) -> bool:
    head, want = rev_parse(target, "HEAD"), resolve_ref(target, ref)
    return bool(head and want and head == want)


def is_dirty(target: Path) -> bool:
    proc = subprocess.run(["git", "-C", str(target), "status", "--porcelain"], text=True, capture_output=True)
    return bool(proc.stdout.strip())


def dir_size(path: Path) -> str:
    proc = subprocess.run(["du", "-sk", str(path)], text=True, capture_output=True)
    if proc.returncode != 0:
        return "-"
    kb = int(proc.stdout.split()[0])
    if kb >= 1024 * 1024:
        return f"{kb / 1024 / 1024:.1f}GB"
    return f"{kb / 1024:.0f}MB" if kb >= 1024 else f"{kb}KB"


def clone_args(url: str, ref: str, depth: int, target: Path) -> list[str]:
    args = ["clone", "--progress"]
    if depth:
        args += ["--depth", str(depth), "--single-branch"]
    return [*args, "--branch", ref, url, str(target)]


def clone_by_sha(url: str, ref: str, depth: int, target: Path) -> None:
    target.mkdir(parents=True, exist_ok=True)
    git(["init", "-q"], cwd=target)
    git(["remote", "add", "origin", url], cwd=target)
    args = ["fetch", "--progress"] + (["--depth", str(depth)] if depth else [])
    git([*args, "origin", ref], cwd=target, stream=True)
    git(["checkout", "-q", "--detach", "FETCH_HEAD"], cwd=target)


def remote_ref_kind(url: str, ref: str) -> str:
    """判断 ref 在上游是 tag 还是分支；同名时以 tag 为准。判断不出时按 tag 处理。"""
    if SHA_RE.match(ref):
        return "commit"
    proc = subprocess.run(["git", "ls-remote", url, f"refs/tags/{ref}", f"refs/heads/{ref}"],
                          text=True, capture_output=True)
    if proc.returncode != 0:
        return "unknown"
    names = {line.split("\t", 1)[1] for line in proc.stdout.splitlines() if "\t" in line}
    if f"refs/heads/{ref}" in names and f"refs/tags/{ref}" not in names:
        return "branch"
    return "tag"


def update_repo(target: Path, url: str, ref: str, depth: int) -> None:
    """按 ref 类型 fetch 到对应的本地引用，再 detached checkout。"""
    base = ["fetch", "--force", "--progress"] + (["--depth", str(depth)] if depth else [])
    tag_spec, branch_spec = f"+refs/tags/{ref}:refs/tags/{ref}", f"+refs/heads/{ref}:refs/remotes/origin/{ref}"
    kind = remote_ref_kind(url, ref)
    if kind == "unknown":
        # 探测失败（镜像抖动等）时从轻到重试：先 tag，再分支
        try:
            git([*base, url, tag_spec], cwd=target)
        except GitError:
            git([*base, url, branch_spec], cwd=target, stream=True)
    else:
        refspec = {"commit": ref, "branch": branch_spec, "tag": tag_spec}[kind]
        git([*base, url, refspec], cwd=target, stream=True)
    git(["checkout", "-q", "--detach", "FETCH_HEAD"], cwd=target)


def sync_repo(data: dict, repo: dict, args: argparse.Namespace) -> tuple[str, bool]:
    """返回 (状态描述, 是否成功)。"""
    root = root_path(data)
    target = root / repo["name"]
    ref = repo["ref"]
    url = proxy_url(repo["url"], args.proxy)
    depth = repo_depth(data, repo, args.depth)

    if target.is_dir() and any(target.iterdir()):
        if not (target / ".git").exists():
            return f"目录已存在但不是 git 检出：{target}", False
        if not args.update:
            if is_synced(target, ref):
                return f"已存在，ref 一致（{ref}）", True
            head = (rev_parse(target, "HEAD") or "?")[:7]
            return f"已存在，ref 不一致（HEAD {head}），可加 --update", True
        if is_synced(target, ref):
            return f"已是最新 {ref}", True
        if args.dry_run:
            return f"[dry-run] 更新到 {ref}", True
        update_repo(target, url, ref, depth)
        if args.proxy:
            git(["remote", "set-url", "origin", repo["url"]], cwd=target)
        return f"已更新到 {ref}（{rev_parse(target, 'HEAD')[:7]})", True

    if args.dry_run:
        shown = f"checkout {ref}" if SHA_RE.match(ref) else shlex.join(clone_args(url, ref, depth, target))
        return f"[dry-run] {shown}", True

    root.mkdir(parents=True, exist_ok=True)
    try:
        if SHA_RE.match(ref):
            clone_by_sha(url, ref, depth, target)
        else:
            git(clone_args(url, ref, depth, target), stream=True)
    except GitError as exc:
        return f"{exc}（直连不通可加 --proxy https://gh-proxy.com）", False
    if args.proxy:
        git(["remote", "set-url", "origin", repo["url"]], cwd=target)
    return f"已克隆 {ref}（{rev_parse(target, 'HEAD')[:7]}）", True


def cmd_status(data: dict, args: argparse.Namespace) -> int:
    root = root_path(data)
    rows = [("名称", "清单 ref", "HEAD", "体积", "状态")]
    all_synced = True
    for repo in selected(data, args.only):
        target = root / repo["name"]
        if not (target / ".git").exists():
            rows.append((repo["name"], repo["ref"], "-", "-", "未检出"))
            all_synced = False
            continue
        head = (rev_parse(target, "HEAD") or "?")[:7]
        state = "已同步" if is_synced(target, repo["ref"]) else "ref 不一致（需 --update）"
        if is_dirty(target):
            state += "，有本地改动"
        rows.append((repo["name"], repo["ref"], head, dir_size(target), state))
    print_table(rows)
    print(f"\n清单：{args.manifest}\n根目录：{root}")
    if not all_synced:
        print("提示：make repo-sync 克隆缺失源码树")
    return 0


def cmd_sync(data: dict, args: argparse.Namespace) -> int:
    failures = 0
    resolved: dict[str, str] = {}
    for repo in selected(data, args.only):
        if args.latest and repo.get("track") in {"latest-release", "latest-tag"}:
            fetch = fetch_latest_tag if repo["track"] == "latest-tag" else fetch_latest_release
            tag, source = fetch(repo["url"])
            if tag and tag != repo["ref"]:
                print(f"[{repo['name']}] 清单 {repo['ref']} → 最新 {tag}（{source}）")
                repo["ref"] = tag
                resolved[repo["name"]] = tag
            elif tag:
                print(f"[{repo['name']}] 已是最新发布 {tag}（{source}）")
            else:
                print(f"[{repo['name']}] 保留清单 ref {repo['ref']}：{source}", file=sys.stderr)
        message, ok = sync_repo(data, repo, args)
        print(f"[{repo['name']}] {message}")
        failures += 0 if ok else 1

    if resolved and args.write:
        if args.dry_run:
            print(f"[dry-run] 清单将更新：{args.manifest}")
        else:
            write_back(args.manifest, resolved)
            print(f"清单已更新：{args.manifest}")
    elif resolved:
        print("提示：加 --write 可把解析到的版本写回清单")
    return 0 if failures == 0 else 1


def selected(data: dict, only: list[str] | None) -> list[dict]:
    repos = data["repos"]
    if not only:
        return repos
    wanted = {part.strip() for item in only for part in item.split(",") if part.strip()}
    known = {repo["name"] for repo in repos}
    unknown = wanted - known
    if unknown:
        raise SystemExit(f"清单中没有这些条目：{', '.join(sorted(unknown))}（可选：{', '.join(sorted(known))}）")
    return [repo for repo in repos if repo["name"] in wanted]


def write_back(path: Path, refs: dict[str, str]) -> None:
    """按条目改写 ref / updated，保留清单中的注释与排版。"""
    lines = path.read_text(encoding="utf-8").splitlines()
    today = datetime.date.today().isoformat()
    current: str | None = None
    for index, line in enumerate(lines):
        name_match = re.match(r"^\s*-\s+name:\s*(\S+)\s*$", line)
        if name_match:
            current = name_match.group(1)
            continue
        if current not in refs:
            continue
        ref_match = re.match(r"^(\s*)ref:\s*\S+.*$", line)
        if ref_match:
            lines[index] = f"{ref_match.group(1)}ref: {refs[current]}"
            continue
        updated_match = re.match(r"^(\s*updated:\s*).*$", line)
        if updated_match:
            lines[index] = f"{updated_match.group(1)}{today}"
    path.write_text("\n".join(lines) + "\n", encoding="utf-8")


def display_width(text: str) -> int:
    return sum(2 if unicodedata.east_asian_width(char) in "WF" else 1 for char in text)


def print_table(rows: list[tuple[str, ...]]) -> None:
    widths = [max(display_width(row[i]) for row in rows) for i in range(len(rows[0]))]
    for row in rows:
        cells = [cell + " " * (widths[i] - display_width(cell)) for i, cell in enumerate(row)]
        print("  ".join(cells).rstrip())


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="按 maintaining/02-元数据/repos.yaml 重建 code/repo/ 下的上游源码树",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    parser.add_argument("command", nargs="?", choices=["sync", "status"], default="sync", help="sync（默认）或 status")
    parser.add_argument("--manifest", type=Path, default=DEFAULT_MANIFEST, help="清单路径")
    parser.add_argument("--only", nargs="+", metavar="NAME", help="只处理指定条目（空格或逗号分隔）")
    parser.add_argument("--proxy", default=os.environ.get("KUDIG_GIT_PROXY"),
                        help="git 镜像前缀，如 https://gh-proxy.com（默认取 KUDIG_GIT_PROXY）")
    parser.add_argument("--depth", type=int, help="覆盖清单里的浅克隆深度，0 为完整历史")

    sync = parser.add_argument_group("sync 选项")
    sync.add_argument("--update", action="store_true", help="已有源码树按清单 ref 重新 fetch + checkout")
    sync.add_argument("--latest", action="store_true", help="track: latest-release / latest-tag 条目解析最新稳定版")
    sync.add_argument("--write", action="store_true", help="把 --latest 解析到的版本写回清单")
    sync.add_argument("--dry-run", action="store_true", help="只打印将要执行的命令")
    return parser


def main() -> int:
    args = build_parser().parse_args()
    if args.write and not args.latest:
        raise SystemExit("--write 只能与 --latest 一起使用")
    data = load_manifest(args.manifest)
    args.manifest = args.manifest.resolve()
    return cmd_status(data, args) if args.command == "status" else cmd_sync(data, args)


if __name__ == "__main__":
    sys.exit(main())
