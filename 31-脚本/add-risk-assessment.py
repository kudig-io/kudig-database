#!/usr/bin/env python3
"""
为项目中的 Markdown 文档批量添加生产环境风险评估。

功能：
- 为包含运维命令的文档顶部插入统一的安全提示
- 为 bash/sh/kubectl/helm/terraform 等命令代码块标注风险等级：
  🔴 高风险：不可逆、数据丢失、服务中断、权限扩大
  🟡 中风险：会修改集群/资源状态，但通常可回滚
  🟢 低风险：只读/信息收集，无副作用
- 对高风险命令在代码块前追加显式警告块
- 通过 <!-- risk-assessed --> 标记实现幂等（已处理文件跳过）

用法：
  python3 scripts/add-risk-assessment.py [--dry-run]
"""
import re
import sys
from datetime import datetime
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
TODAY = datetime.now().strftime('%Y-%m-%d')

RISK_MARKER = '<!-- risk-assessed -->'

TOP_NOTICE = """> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。
"""

HIGH_RISK_PATTERNS = [
    re.compile(r'\bkubectl\s+delete\s+(?:namespace|node|pv|pvc|storageclass|clusterrole|clusterrolebinding)\b', re.I),
    re.compile(r'\bkubectl\s+delete\s+.*--all\b', re.I),
    re.compile(r'\bkubectl\s+delete\s+.*--force\b', re.I),
    re.compile(r'\bhelm\s+uninstall\b', re.I),
    re.compile(r'\bkubectl\s+drain\s+.*--force\b', re.I),
    re.compile(r'\bkubectl\s+drain\s+.*--delete-emptydir-data\b', re.I),
    re.compile(r'\bkubectl\s+cordon\b', re.I),
    re.compile(r'\bkubectl\s+taint\b', re.I),
    re.compile(r'\brm\s+-rf\b', re.I),
    re.compile(r'\betcdctl\s+(?:del|delete)\b', re.I),
    re.compile(r'\bkubectl\s+exec\s+.*rm\s+-rf\b', re.I),
    re.compile(r'\bkubectl\s+create\s+clusterrolebinding\s+.*cluster-admin\b', re.I),
    re.compile(r'\bkubectl\s+edit\b', re.I),
    re.compile(r'\bterraform\s+destroy\b', re.I),
    re.compile(r'\baws\s+.*\bdelete\b', re.I),
    re.compile(r'\bgcloud\s+.*\bdelete\b', re.I),
    re.compile(r'\baz\s+.*\bdelete\b', re.I),
    re.compile(r'\bdocker\s+(?:rm|kill|stop)\b', re.I),
    re.compile(r'\bsystemctl\s+(?:stop|restart)\s+(?:kubelet|containerd|docker|crio)\b', re.I),
    re.compile(r'\breboot\b', re.I),
    re.compile(r'\bshutdown\b', re.I),
    re.compile(r'\bmkfs\.\b', re.I),
    re.compile(r'\bfdisk\b', re.I),
    re.compile(r'\bparted\b', re.I),
]

MEDIUM_RISK_PATTERNS = [
    re.compile(r'\bkubectl\s+apply\b', re.I),
    re.compile(r'\bkubectl\s+patch\b', re.I),
    re.compile(r'\bkubectl\s+scale\b', re.I),
    re.compile(r'\bkubectl\s+rollout\s+restart\b', re.I),
    re.compile(r'\bkubectl\s+set\b', re.I),
    re.compile(r'\bkubectl\s+create\b', re.I),
    re.compile(r'\bkubectl\s+replace\b', re.I),
    re.compile(r'\bkubectl\s+run\b', re.I),
    re.compile(r'\bkubectl\s+exec\b', re.I),
    re.compile(r'\bkubectl\s+port-forward\b', re.I),
    re.compile(r'\bkubectl\s+drain\b', re.I),
    re.compile(r'\bkubectl\s+uncordon\b', re.I),
    re.compile(r'\bkubectl\s+label\b', re.I),
    re.compile(r'\bkubectl\s+annotate\b', re.I),
    re.compile(r'\bkubectl\s+cp\b', re.I),
    re.compile(r'\bhelm\s+upgrade\b', re.I),
    re.compile(r'\bhelm\s+install\b', re.I),
    re.compile(r'\bhelm\s+rollback\b', re.I),
    re.compile(r'\bterraform\s+apply\b', re.I),
    re.compile(r'\bdocker\s+(?:start|restart|pause|unpause)\b', re.I),
    re.compile(r'\bcrictl\s+(?:rm|stop|update)\b', re.I),
]

COMMAND_TOOLS = re.compile(
    r'\b(kubectl|helm|terraform|docker|aws|gcloud|az|etcdctl|istioctl|crictl|systemctl|reboot|shutdown)\b',
    re.I
)


def has_command_content(text: str) -> bool:
    return bool(COMMAND_TOOLS.search(text))


def assess_risk(code: str) -> str:
    for pat in HIGH_RISK_PATTERNS:
        if pat.search(code):
            return 'high'
    for pat in MEDIUM_RISK_PATTERNS:
        if pat.search(code):
            return 'medium'
    return 'low'


def risk_comment(level: str) -> str:
    if level == 'high':
        return '# 🔴 高风险：可能造成数据丢失或服务中断，执行前需备份、变更审批与回滚方案'
    elif level == 'medium':
        return '# 🟡 中风险：会修改集群/资源状态，执行前请确认目标、影响范围与授权'
    else:
        return '# 🟢 低风险：只读/信息收集，通常无副作用'


def high_risk_warning() -> str:
    return """> **🔴 高风险操作警告**
>
> 下方命令属于不可逆或高影响操作，执行前请确认：
> - 已备份关键数据与配置
> - 处于批准的变更窗口期
> - 已获得相关责任人授权
> - 已准备回滚或恢复方案
> - 目标集群、Namespace、节点/资源名称正确无误

"""


def should_process_file(path: Path) -> bool:
    rel = path.relative_to(ROOT)
    for part in rel.parts:
        if part.startswith('.') or part in ('release', '.git', 'node_modules', '__pycache__'):
            return False
    return True


def add_top_notice(content: str) -> str:
    if content.startswith('---\n'):
        idx = content.find('\n---\n', 4)
        if idx != -1:
            fm_end = idx + 5
            return content[:fm_end] + '\n' + TOP_NOTICE + '\n' + content[fm_end:]
    m = re.search(r'^(# .+)$', content, re.M)
    if m:
        pos = m.start()
        return content[:pos] + TOP_NOTICE + '\n' + content[pos:]
    return TOP_NOTICE + '\n' + content


def process_file(path: Path, dry_run: bool = False) -> dict:
    try:
        content = path.read_text(encoding='utf-8')
    except Exception as e:
        return {'error': str(e)}

    if RISK_MARKER in content:
        return {'skipped': 'already_assessed'}

    if not has_command_content(content):
        return {'skipped': 'no_commands'}

    code_block_re = re.compile(r'^(```+)\s*(\w*)\s*\n(.*?)\n\1\s*$', re.MULTILINE | re.DOTALL)

    modified = False
    new_content = content
    offset = 0

    for match in code_block_re.finditer(content):
        lang = match.group(2).lower()
        code = match.group(3)

        if lang not in ('bash', 'sh', 'shell', 'zsh', 'kubectl', 'powershell', '') or not has_command_content(code):
            continue

        if re.search(r'#\s*[🔴🟡🟢]', code):
            continue

        level = assess_risk(code)
        comment = risk_comment(level)
        new_code = comment + '\n' + code

        warning = high_risk_warning() if level == 'high' else ''

        start = match.start() + offset
        end = match.end() + offset
        old_block = new_content[start:end]

        fence = match.group(1)
        lang_part = '' if not lang else ' ' + lang
        new_block = warning + fence + lang_part + '\n' + new_code + '\n' + fence

        new_content = new_content[:start] + new_block + new_content[end:]
        offset += len(new_block) - len(old_block)
        modified = True

    if modified or has_command_content(content):
        new_content = add_top_notice(new_content)
        new_content += '\n\n' + RISK_MARKER + '\n'
        if not dry_run:
            path.write_text(new_content, encoding='utf-8')
        return {'processed': True}
    else:
        return {'skipped': 'no_command_blocks'}


def main(dry_run: bool = False):
    md_files = [p for p in ROOT.rglob('*.md') if should_process_file(p)]
    stats = {
        'total': len(md_files),
        'processed': 0,
        'skipped_no_commands': 0,
        'skipped_already': 0,
        'errors': 0,
    }

    for p in md_files:
        result = process_file(p, dry_run=dry_run)
        if 'error' in result:
            stats['errors'] += 1
            print(f'[ERROR] {p.relative_to(ROOT)}: {result["error"]}')
        elif result.get('processed'):
            stats['processed'] += 1
            if not dry_run and stats['processed'] % 500 == 0:
                print(f'Progress: {stats["processed"]} processed...')
        elif result.get('skipped') == 'no_commands':
            stats['skipped_no_commands'] += 1
        elif result.get('skipped') == 'already_assessed':
            stats['skipped_already'] += 1

    print(f'Done. Total: {stats["total"]}, Processed: {stats["processed"]}, '
          f'No commands: {stats["skipped_no_commands"]}, Already assessed: {stats["skipped_already"]}, '
          f'Errors: {stats["errors"]}')


if __name__ == '__main__':
    dry = '--dry-run' in sys.argv
    main(dry_run=dry)
