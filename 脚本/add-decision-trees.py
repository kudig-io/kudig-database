#!/usr/bin/env python3
"""
KUDIG-DATABASE FTA 决策树增强脚本
为 topic-fta/list/ 下的每篇 FTA 文档添加 Mermaid 决策树章节。

在文档末尾追加 "## 快速决策树" 章节，不修改已有内容。
"""

import re
from pathlib import Path
from collections import defaultdict

BASE_DIR = Path(__file__).parent.parent
FTA_DIR = BASE_DIR / "topic-fta" / "list"


def get_fta_info(filepath: Path) -> dict:
    """Extract key info from FTA document."""
    try:
        content = filepath.read_text(encoding="utf-8")
    except Exception:
        return {"title": filepath.stem, "component": "", "checks": [], "actions": []}

    # Check if already has decision tree
    if "## 快速决策树" in content:
        return {"has_tree": True}

    # Extract component from title or filename
    fm = {}
    content_stripped = content.lstrip()
    if content_stripped.startswith("---"):
        end = content_stripped.find("---", 3)
        if end != -1:
            import yaml
            try:
                fm = yaml.safe_load(content_stripped[3:end].strip()) or {}
            except Exception:
                pass

    title = fm.get("title", filepath.stem)
    component = fm.get("component", "")
    if not component:
        # Extract from filename
        component = filepath.stem.replace("-fta", "").replace("_", "-")

    # Extract top events from body
    top_events = re.findall(r'(?:顶事件|Top Event|故障现象)[：:.\s]+(.+?)(?:\n|$)', content)

    # Extract diagnosis sections
    diagnosis_sections = []
    for match in re.finditer(r'#{2,3}\s+(?:诊断|排查|检查|验证|确认)[^\n]*', content):
        diagnosis_sections.append(match.group(0).strip())

    # Extract commands
    commands = re.findall(r'```(?:bash|shell)?\n(kubectl[^\n]+)', content)

    return {
        "title": title,
        "component": component,
        "top_events": top_events[:3] if top_events else [f"{component} 故障"],
        "diagnosis_sections": diagnosis_sections[:3],
        "commands": commands[:5],
    }


def generate_decision_tree(fta_info: dict) -> str:
    """Generate a Mermaid decision tree for an FTA."""
    component = fta_info.get("component", "unknown")
    top_events = fta_info.get("top_events", [f"{component} 故障"])
    diagnosis_sections = fta_info.get("diagnosis_sections", [])
    commands = fta_info.get("commands", [])

    top_event = top_events[0] if top_events else f"{component} 故障"

    # Build decision tree nodes
    checks = []
    if diagnosis_sections:
        for i, sec in enumerate(diagnosis_sections[:3]):
            clean_sec = re.sub(r'^#+\s*', '', sec).strip()
            clean_sec = re.sub(r'[\U0001f300-\U0001f9ff]', '', clean_sec).strip()
            checks.append(clean_sec)

    if not checks:
        checks = [
            f"检查组件状态",
            f"检查日志和事件",
            f"检查资源配置",
        ]

    # Build mermaid
    nodes = []
    links = []

    # Top event
    nodes.append(f'    A["故障: {top_event[:50]}"]')

    # First check
    nodes.append(f'    B{{"{checks[0]}"}}')
    links.append('    A --> B')

    # Yes path → fix
    nodes.append(f'    C["修复: {component} 配置/重启"]')
    links.append('    B -->|"是"| C')

    # Second check
    if len(checks) > 1:
        nodes.append(f'    D{{"{checks[1]}"}}')
        links.append('    B -->|"否"| D')
        nodes.append(f'    E["修复: {component} 深度诊断"]')
        links.append('    D -->|"是"| E')
    else:
        links.append('    B -->|"否"| G')

    # Third check or escalation
    if len(checks) > 2:
        if len(checks) > 1:
            nodes.append(f'    F{{"{checks[2]}"}}')
            links.append('    D -->|"否"| F')
            nodes.append(f'    G["修复: {component} 专项处理"]')
            links.append('    F -->|"是"| G')
            links.append('    F -->|"否"| H')
        else:
            nodes.append(f'    F{{"{checks[1]}"}}')
            links.append('    B -->|"否"| F')
            nodes.append(f'    G["修复: {component} 深度处理"]')
            links.append('    F -->|"是"| G')
            links.append('    F -->|"否"| H')
    else:
        if len(checks) > 1:
            links.append('    D -->|"否"| H')
        else:
            links.append('    B -->|"否"| H')

    # Verification
    nodes.append('    I["验证修复"]')
    if len(checks) > 2:
        links.append('    C --> I')
        links.append('    E --> I')
        links.append('    G --> I')
    elif len(checks) > 1:
        links.append('    C --> I')
        links.append('    E --> I')
    else:
        links.append('    C --> I')
        links.append('    G --> I')

    # Final nodes
    nodes.append('    J["记录根因，关闭"]')
    nodes.append(f'    H["升级到专家"]')

    links.append('    I -->|"已修复"| J')
    links.append('    I -->|"未修复"| H')

    mermaid_body = "\n".join(nodes) + "\n\n" + "\n".join(links)

    # Build quick checks section
    checks_md = ""
    if commands:
        checks_md = "\n\n### 快速检查命令\n\n"
        for i, cmd in enumerate(commands[:3], 1):
            checks_md += f"**检查 {i}**:\n```bash\n{cmd}\n```\n\n"

    return f"""

---

## 快速决策树

> 基于 FTA 故障树自动生成的快速决策路径，3 步内定位问题。

```mermaid
graph TD
{mermaid_body}

    style A fill:#ef4444,stroke:#b91c1c,color:#fff
    style J fill:#22c55e,stroke:#166534,color:#fff
    style H fill:#f59e0b,stroke:#b45309,color:#fff
    style B fill:#3b82f6,stroke:#1d4ed8,color:#fff
{"""    style D fill:#3b82f6,stroke:#1d4ed8,color:#fff""" if len(checks) > 1 else ""}
{"""    style F fill:#3b82f6,stroke:#1d4ed8,color:#fff""" if len(checks) > 2 else ""}
```
{checks_md}
### 升级路径

| 条件 | 升级到 | 提供信息 |
|---|---|---|
| 决策树未定位 | SRE 专家 | 检查输出 + 日志 |
| 涉及数据风险 | DBA + 架构师 | 数据状态 |
| 生产服务中断 | On-call 负责人 | 影响范围 + 回滚方案 |
"""


def process_fta(filepath: Path) -> bool:
    """Process a single FTA file. Returns True if modified."""
    info = get_fta_info(filepath)
    if info.get("has_tree"):
        return False

    tree = generate_decision_tree(info)
    try:
        content = filepath.read_text(encoding="utf-8")
        content = content.rstrip() + tree
        filepath.write_text(content, encoding="utf-8")
        return True
    except Exception:
        return False


def main():
    if not FTA_DIR.exists():
        print("FTA directory not found")
        return

    fta_files = sorted(FTA_DIR.glob("*.md"))
    fta_files = [f for f in fta_files if f.name != "README.md"]

    print("=" * 60)
    print(f"FTA 决策树增强: {len(fta_files)} 文件")
    print("=" * 60)

    modified = 0
    skipped = 0
    for f in fta_files:
        if process_fta(f):
            modified += 1
        else:
            skipped += 1

    print(f"\n完成:")
    print(f"  修改: {modified} 文件")
    print(f"  跳过: {skipped} 文件 (已有决策树)")


if __name__ == "__main__":
    main()
