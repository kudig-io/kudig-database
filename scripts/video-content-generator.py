#!/usr/bin/env python3
"""
video-content-generator.py
基于 KUDIG 知识库内容（FTA/FEBM/Skills）生成数字人播报文案。

功能：
  - 从 FTA 故障树提取底事件，生成结构化解说词
  - 从 FEBM 取证文档提取步骤，生成循证播报
  - 从 Skills 文档提取诊断流程，生成技能演示解说
  - 生成带时间戳和镜头提示的完整视频脚本

Usage:
    python3 scripts/video-content-generator.py --type fta --topic pod-crashloop --output video-script.md
    python3 scripts/video-content-generator.py --type febm --topic etcd-election --output video-script.md
    python3 scripts/video-content-generator.py --type skill --topic node-notready --output video-script.md

Exit codes:
    0 = success
    1 = error
"""

import os
import sys
import re
import argparse
from pathlib import Path
from datetime import datetime

# 模板配置
TEMPLATE_FTA = """# {title} — 数字人播报脚本

> **生成时间**: {timestamp}
> **内容类型**: FTA 故障树分析
> **目标受众**: {audience}
> **预计时长**: {duration} 分钟

---

## 视频结构

| 段落 | 内容 | 时长 | 镜头 |
|:---|:---|:---:|:---|
| 开场 | 故障现象概述 | {intro_duration}s | 主播近景 |
| 故障树 | 顶事件 → 底事件路径 | {tree_duration}s | 动画演示 |
| 诊断 | 根因诊断步骤 | {diag_duration}s | 屏幕录制 |
| 修复 | 修复操作演示 | {fix_duration}s | 操作界面 |
| 总结 | 预防措施与要点 | {outro_duration}s | 主播近景 |

---

## 段落一：开场（{intro_duration}s）

**主播台词**：
> {intro_script}

**镜头提示**：主播直视镜头，语速适中，表情严肃专业

---

## 段落二：故障树分析（{tree_duration}s）

**动画脚本**：
```
{animation_script}
```

**主播台词**：
> {tree_script}

**镜头提示**：故障树动画 + 主播画外音

---

## 段落三：根因诊断（{diag_duration}s）

**诊断步骤**：

{diagnosis_steps}

**主播台词**：
> {diag_script}

**镜头提示**：终端操作界面录制 + 主播画外音

---

## 段落四：修复操作（{fix_duration}s）

**修复方案**：

{fix_steps}

**主播台词**：
> {fix_script}

**镜头提示**：操作界面逐步演示

---

## 段落五：总结（{outro_duration}s）

**主播台词**：
> {outro_script}

**镜头提示**：主播近景，强调预防措施

---

## 数字人参数配置

| 参数 | 值 |
|:---|:---|
| 形象 | 专业工程师（商务休闲） |
| 声音 | 中文男声/女声（根据 topic） |
| 语速 | 1.2x（诊断部分 1.0x） |
| 分辨率 | 1920x1080 |
| 输出格式 | MP4 |

---

## 背景素材建议

- 故障树动画（Mermaid → GIF）
- 终端操作界面截图
- K8s Dashboard 监控面板
- 网络拓扑图（如涉及网络故障）

---

## 关联知识库

- FTA 源文档：{fta_source}
- 相关 Skills：{related_skills}
- 深度文档：{related_docs}
"""

TEMPLATE_FEBM = """# {title} — 数字人播报脚本

> **生成时间**: {timestamp}
> **内容类型**: FEBM 取证循证分析
> **目标受众**: {audience}
> **预计时长**: {duration} 分钟

---

## 视频结构

| 段落 | 内容 | 时长 | 镜头 |
|:---|:---|:---:|:---|
| 开场 | 事件背景与影响范围 | {intro_duration}s | 主播近景 |
| 证据链 | 取证步骤与证据采集 | {evidence_duration}s | 屏幕录制 |
| 分析 | 根因推导逻辑 | {analysis_duration}s | 画布动画 |
| 结论 | 事实总结与复盘 | {conclusion_duration}s | 主播近景 |

---

## 段落一：开场（{intro_duration}s）

**主播台词**：
> {intro_script}

---

## 段落二：证据链（{evidence_duration}s）

{evidence_steps}

**主播台词**：
> {evidence_script}

---

## 段落三：分析（{analysis_duration}s）

**分析画布**：
```
{analysis_canvas}
```

**主播台词**：
> {analysis_script}

---

## 段落四：结论（{conclusion_duration}s）

**主播台词**：
> {conclusion_script}

---

## 数字人参数配置

| 参数 | 值 |
|:---|:---|
| 形象 | 技术调查员（严肃专业） |
| 声音 | 中文男声 |
| 语速 | 1.0x（全篇保持） |
| 分辨率 | 1920x1080 |

---

## 关联知识库

- FEBM 源文档：{febm_source}
- 相关 FTA：{related_fta}
"""

TEMPLATE_SKILL = """# {title} — 数字人播报脚本

> **生成时间**: {timestamp}
> **内容类型**: Skills 运维技能
> **目标受众**: {audience}
> **预计时长**: {duration} 分钟

---

## 视频结构

| 段落 | 内容 | 时长 | 镜头 |
|:---|:---|:---:|:---|
| 开场 | 症状识别与影响评估 | {intro_duration}s | 主播近景 |
| 诊断 | 分步诊断工作流 | {diag_duration}s | 终端+图示 |
| 修复 | 修复操作执行 | {fix_duration}s | 操作界面 |
| 验证 | 修复确认与监控 | {verify_duration}s | 监控面板 |
| 结尾 | 升级路径与要点 | {outro_duration}s | 主播近景 |

---

## 段落一：症状识别（{intro_duration}s）

**主播台词**：
> {intro_script}

**症状列表**：
{symptoms}

---

## 段落二：诊断工作流（{diag_duration}s）

**诊断步骤**：

{diagnosis_steps}

**主播台词**：
> {diag_script}

---

## 段落三：修复操作（{fix_duration}s）

{risk_level}

**修复命令**：

{fix_commands}

**主播台词**：
> {fix_script}

---

## 段落四：验证确认（{verify_duration}s）

**验证步骤**：

{verify_steps}

**主播台词**：
> {verify_script}

---

## 段落五：结尾（{outro_duration}s）

**主播台词**：
> {outro_script}

---

## 数字人参数配置

| 参数 | 值 |
|:---|:---|
| 形象 | SRE 工程师（实战派） |
| 声音 | 中文女声（清晰专业） |
| 语速 | 1.3x（修复部分 1.0x） |
| 分辨率 | 1920x1080 |

---

## 关联知识库

- Skill 源文档：{skill_source}
- 相关 FTA：{related_fta}
- 深度排查：{related_structural}
"""

def extract_fm(content):
    """提取 front matter"""
    if not content.startswith('---'):
        return {}
    lines = content.split('\n')[1:]
    fm = {}
    for line in lines:
        if line == '---':
            break
        match = re.match(r'^(\w+):\s*(.*)$', line)
        if match:
            fm[match.group(1)] = match.group(2).strip()
    return fm

def extract_title(content):
    """提取文档标题"""
    match = re.search(r'^#\s+(.+)$', content, re.MULTILINE)
    return match.group(1) if match else "未命名"

def extract_bes(content):
    """从 FTA 文档提取底事件"""
    bes = []
    # 匹配 BE-1, BE-2 等底事件标题
    matches = re.findall(r'^###\s+(BE-\d+[:：]\s*(.+))', content, re.MULTILINE)
    for match in matches:
        bes.append({'id': match[0], 'name': match[1]})
    return bes

def extract_diagnosis_steps(content):
    """提取诊断步骤"""
    steps = []
    # 匹配 "Step D1.1" 或 "步骤1" 等模式
    matches = re.findall(r'(?:Step|步骤)\s+D?\d+\.\d+[^`]*`{3}[^`]+`{3}', content, re.MULTILINE)
    for m in matches[:6]:  # 最多6步
        steps.append(m.strip())
    return steps

def extract_fix_commands(content):
    """提取修复命令"""
    cmds = []
    blocks = re.findall(r'```bash\n(.*?)```', content, re.DOTALL)
    for block in blocks[:4]:
        cmds.append(block.strip())
    return cmds

def generate_intro_script(title, content):
    """生成开场白"""
    fm = extract_fm(content)
    duration = fm.get('estimated_read_time', '15min').replace('min', '')
    return f"""大家好，今天我们来讲解 {title} 的完整故障排查流程。
这个故障在生产环境中很常见，如果不能及时处理，会对业务造成严重影响。
接下来的内容将帮助大家掌握从症状识别到根因定位的完整方法论。"""

def generate_tree_script(bes):
    """生成故障树解说"""
    if not bes:
        return "首先我们来分析故障树的顶层事件，然后逐步深入到底事件。"
    first = bes[0]['name'] if bes else '关键路径'
    return f"""让我们从顶事件开始分析。故障树显示，主要的底事件包括：
{'；'.join([b['name'] for b in bes[:5]])}。
其中最常见的根因是 {first}，我们需要重点关注。"""

def generate_diag_script(steps):
    """生成诊断解说"""
    if not steps:
        return "接下来我们按照诊断工作流逐步排查。"
    return f"""现在我们进入诊断阶段。根据结构化排查方法，
需要依次执行以下 {len(steps)} 个诊断步骤。每个步骤都有明确的判定条件。"""

def generate_fix_script(commands):
    """生成修复解说"""
    if not commands:
        return "诊断确认后，我们可以开始修复操作。"
    return f"""根据诊断结果，建议执行以下修复命令。
注意：高风险操作需要人工审批后再执行。"""

def generate_outro_script(title):
    """生成结尾解说"""
    return f"""以上就是 {title} 的完整故障排查流程。
最后提醒大家：预防胜于治疗。建议建立完善的监控告警体系，
定期进行故障演练，确保团队具备快速响应能力。"""

def generate_fta_script(topic_path, output_path):
    """生成 FTA 视频脚本"""
    try:
        with open(topic_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        return False, f"无法读取文件: {e}"

    title = extract_title(content)
    fm = extract_fm(content)
    bes = extract_bes(content)
    steps = extract_diagnosis_steps(content)
    cmds = extract_fix_commands(content)

    duration = fm.get('estimated_read_time', '15min').replace('min', '')
    audience = fm.get('audience', 'SRE, Ops Engineer')

    script = TEMPLATE_FTA.format(
        title=title,
        timestamp=datetime.now().strftime('%Y-%m-%d %H:%M'),
        audience=audience,
        duration=duration,
        intro_duration=15,
        tree_duration=45,
        diag_duration=60,
        fix_duration=30,
        outro_duration=20,
        intro_script=generate_intro_script(title, content),
        animation_script='\n'.join([f'{i+1}. {b["id"]} → {b["name"]}' for i, b in enumerate(bes[:6])]),
        tree_script=generate_tree_script(bes),
        diagnosis_steps='\n'.join([f'{i+1}. {s}' for i, s in enumerate(steps)]),
        diag_script=generate_diag_script(steps),
        fix_steps='\n'.join([f'{i+1}. ```bash\n{cmds[i]}\n```' for i in range(len(cmds))]) if cmds else '（无命令）',
        fix_script=generate_fix_script(cmds),
        outro_script=generate_outro_script(title),
        fta_source=topic_path,
        related_skills='参考 topic-skills/',
        related_docs='参考 domain-12-troubleshooting/'
    )

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(script)
    return True, output_path

def generate_febm_script(topic_path, output_path):
    """生成 FEBM 视频脚本"""
    try:
        with open(topic_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        return False, f"无法读取文件: {e}"

    title = extract_title(content)
    fm = extract_fm(content)
    duration = fm.get('estimated_read_time', '10min').replace('min', '')

    script = TEMPLATE_FEBM.format(
        title=title,
        timestamp=datetime.now().strftime('%Y-%m-%d %H:%M'),
        audience=fm.get('audience', 'SRE'),
        duration=duration,
        intro_duration=15,
        evidence_duration=50,
        analysis_duration=45,
        conclusion_duration=20,
        intro_script=f"""今天我们通过一个实际案例，进行取证循证分析。
这个案例展示了如何在海量日志中抽丝剥茧，找到真正的根因。""",
        evidence_steps='\n'.join([f'{i+1}. 采集证据...' for i in range(5)]),
        evidence_script="首先，我们按照证据易失性优先级采集运行时数据。",
        analysis_canvas='证据A → 证据B → 逻辑推导 → 根因结论',
        analysis_script="基于采集到的证据，我们进行逻辑推导。",
        conclusion_script="最终确认根因为某某组件故障。",
        febm_source=topic_path,
        related_fta='参考 topic-fta/'
    )

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(script)
    return True, output_path

def generate_skill_script(topic_path, output_path):
    """生成 Skills 视频脚本"""
    try:
        with open(topic_path, 'r', encoding='utf-8') as f:
            content = f.read()
    except Exception as e:
        return False, f"无法读取文件: {e}"

    title = extract_title(content)
    fm = extract_fm(content)
    steps = extract_diagnosis_steps(content)
    cmds = extract_fix_commands(content)
    duration = fm.get('estimated_read_time', '10min').replace('min', '')

    script = TEMPLATE_SKILL.format(
        title=title,
        timestamp=datetime.now().strftime('%Y-%m-%d %H:%M'),
        audience=fm.get('audience', 'SRE, Ops Engineer'),
        duration=duration,
        intro_duration=12,
        diag_duration=50,
        fix_duration=35,
        verify_duration=20,
        outro_duration=15,
        intro_script=f"""大家好，今天我们来讲解 {title} 的完整处理流程。
作为 SRE 工程师，掌握这个技能可以快速定位和解决常见故障。""",
        symptoms='\n'.join(['- 症状1', '- 症状2']),
        diagnosis_steps='\n'.join([f'{i+1}. {s}' for i, s in enumerate(steps)]),
        diag_script="接下来我们按照诊断工作流逐步排查。",
        risk_level='**🟡 中风险**：建议人工审批后执行',
        fix_commands='\n'.join([f'```bash\n{c}\n```' for c in cmds]) if cmds else '（无命令）',
        fix_script="修复操作需要谨慎，请确保已备份配置。",
        verify_steps='\n'.join([f'{i+1}. 验证步骤...' for i in range(3)]),
        verify_script="修复后需要验证确认，确保服务恢复正常。",
        outro_script="以上就是完整的处理流程。遇到无法解决的问题，请及时升级。",
        skill_source=topic_path,
        related_fta='参考 topic-fta/',
        related_structural='参考 topic-structural-trouble-shooting/'
    )

    with open(output_path, 'w', encoding='utf-8') as f:
        f.write(script)
    return True, output_path

def find_content(topic, content_type):
    """查找对应的知识库内容"""
    base = Path('.')

    if content_type == 'fta':
        search_dirs = ['topic-fta']
    elif content_type == 'febm':
        search_dirs = ['topic-febm']
    else:
        search_dirs = ['topic-skills']

    for d in search_dirs:
        for md in Path(d).rglob('*.md'):
            if topic.lower() in md.stem.lower() or topic.lower() in md.name.lower():
                return str(md)
    return None

def main():
    parser = argparse.ArgumentParser(
        description='生成数字人播报视频脚本',
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
示例:
  python3 scripts/video-content-generator.py --type fta --topic pod-crashloop --output my-script.md
  python3 scripts/video-content-generator.py --type skill --topic node-notready --output my-script.md
  python3 scripts/video-content-generator.py --list               # 列出可用 topic
        """
    )
    parser.add_argument('--type', choices=['fta', 'febm', 'skill'], required=True,
                        help='内容类型')
    parser.add_argument('--topic', default='',
                        help='Topic 名称或文件路径（使用 --list 时可省略）')
    parser.add_argument('--output', '-o',
                        help='输出脚本路径（使用 --list 时可省略）')
    parser.add_argument('--list', action='store_true',
                        help='列出可用 topic')
    parser.add_argument('--verbose', '-v', action='store_true')

    args = parser.parse_args()

    if args.list:
        content_path = find_content('', args.type)
        print(f"\n=== 可用的 {args.type.upper()} Topics ===\n")
        if content_path:
            base = Path(content_path).parent
            for md in base.rglob('*.md'):
                print(f"  {md.stem}")
        return 0

    # 查找内容文件
    content_path = find_content(args.topic, args.type)
    if not content_path:
        print(f"[ERROR] 未找到内容: {args.topic}", file=sys.stderr)
        print(f"  使用 --list 查看可用 topic", file=sys.stderr)
        return 1

    print(f"[INFO] 生成视频脚本: {args.topic}")
    print(f"       内容来源: {content_path}")

    # 生成脚本
    if args.type == 'fta':
        ok, msg = generate_fta_script(content_path, args.output)
    elif args.type == 'febm':
        ok, msg = generate_febm_script(content_path, args.output)
    else:
        ok, msg = generate_skill_script(content_path, args.output)

    if ok:
        print(f"[SUCCESS] 脚本已生成: {msg}")
        return 0
    else:
        print(f"[ERROR] {msg}", file=sys.stderr)
        return 1

if __name__ == '__main__':
    sys.exit(main() or 0)