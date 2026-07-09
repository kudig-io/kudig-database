#!/usr/bin/env python3
"""
Skill → I-O Pair 提取器
从 topic-skills/*.md 中提取命令输出模式、诊断结论和后续操作
"""

import re
import yaml
from pathlib import Path
from typing import List, Dict, Any


class SkillExtractor:
    """从 Skill Markdown 中提取 I-O 对"""

    # 命令模式正则
    COMMAND_PATTERNS = [
        r'`{3}(?:bash|shell)?\n?(kubectl [^`]+)`{3}',
        r'`(kubectl [^`]+)`',
        r'`{3}(?:bash|shell)?\n?(etcdctl [^`]+)`{3}',
        r'`(etcdctl [^`]+)`',
        r'\*\*`?(kubectl [^*`]+)`?\*\*',
        r'##?\s+(?:检查|诊断|验证|排查)[：:]?\s*\n+`{3}[^`]*`{3}',
    ]

    # 诊断结论关键词模式
    DIAGNOSIS_MARKERS = [
        r'(?:原因|根因|诊断|结论)[：:]\s*(.+)',
        r'(?:说明|表示|意味着)[：:]\s*(.+)',
        r'(?:常见原因|可能原因)[：:]\s*(.+)',
        r'\*\s*(.+?(?:因为|由于|导致|引起).+?)',
    ]

    # 输出模式推断映射
    OUTPUT_INFERENCE = {
        'kubectl get nodes': """NAME       STATUS     ROLES           AGE   VERSION
<node-name>    NotReady   <roles>          <age>   <version>""",
        'kubectl get pods': """NAME                    READY   STATUS             RESTARTS      AGE
<pod-name>         0/1     <status>           <restarts>    <age>""",
        'kubectl describe node': """Conditions:
  Type             Status  Reason
  <condition>      True    <reason>""",
        'kubectl describe pod': """Events:
  Type     Reason        Age   From     Message
  Warning  <reason>      <age>  <from>   <message>""",
        'kubectl logs': """<timestamp>  <level>  <message>
...""",
        'etcdctl endpoint health': """https://<endpoint>:2379 is <health>: <message>""",
        'systemctl status kubelet': """● kubelet.service - Kubernetes Kubelet
   Loaded: loaded (/usr/lib/systemd/system/kubelet.service; enabled; vendor preset: disabled)
   Active: <status> (<since>)
   ...""",
    }

    def __init__(self, skills_dir: str):
        self.skills_dir = Path(skills_dir)
        self.io_pairs: List[Dict[str, Any]] = []
        self.sequence = {}

    def extract_all(self, priority_skills: List[str] = None) -> List[Dict[str, Any]]:
        """提取所有 Skill 的 I-O 对"""
        skill_files = sorted(self.skills_dir.glob('*.md'))

        if priority_skills:
            # 优先处理指定 Skill
            prioritized = [f for f in skill_files if any(p in f.name for p in priority_skills)]
            others = [f for f in skill_files if f not in prioritized]
            skill_files = prioritized + others

        for skill_file in skill_files:
            pairs = self._extract_from_file(skill_file)
            self.io_pairs.extend(pairs)
            print(f"  [Skill] {skill_file.name}: {len(pairs)} I-O pairs")

        return self.io_pairs

    def _extract_from_file(self, skill_file: Path) -> List[Dict[str, Any]]:
        """从单个 Skill 文件提取"""
        content = skill_file.read_text(encoding='utf-8')
        pairs = []

        # 提取 frontmatter
        frontmatter = self._parse_frontmatter(content)
        skill_id = frontmatter.get('skill_metadata', {}).get('skill_id', skill_file.stem)
        skill_name = frontmatter.get('title', skill_file.stem)
        severity = frontmatter.get('skill_metadata', {}).get('severity', 'medium')

        # 提取关键命令
        critical_commands = self._extract_critical_commands(content, frontmatter)

        # 提取诊断段落
        diagnosis_sections = self._extract_diagnosis_sections(content)

        # 提取 action/solution 段落
        action_sections = self._extract_action_sections(content)

        # 为每个关键命令构建 I-O 对
        for cmd_info in critical_commands:
            cmd = cmd_info['command']
            domain = self._infer_domain(skill_file.name)
            seq = self._next_sequence(domain)

            # 推断输出模式
            output_pattern = self._infer_output(cmd)

            # 匹配相关诊断
            diagnosis = self._match_diagnosis(cmd, diagnosis_sections)
            if not diagnosis:
                diagnosis = [f"执行 {cmd} 检查相关状态"]

            # 匹配相关操作
            actions = self._match_actions(cmd, action_sections)

            pair = {
                'io_pair_id': f"IODIAG-{domain}-{seq:04d}",
                'skill_ref': skill_id,
                'scenario': skill_name,
                'severity': self._map_severity(severity),
                'command': cmd,
                'output_pattern': output_pattern,
                'diagnosis': diagnosis,
                'action': actions,
                'confidence': cmd_info.get('confidence', 0.85),
                'tags': self._extract_tags(skill_file.name, cmd),
                'k8s_versions': frontmatter.get('k8s_versions', ['1.28', '1.29', '1.30', '1.31', '1.32']),
            }
            pairs.append(pair)

        # 从文中提取带输出示例的代码块
        explicit_pairs = self._extract_explicit_io_blocks(content, skill_id, skill_name, severity)
        pairs.extend(explicit_pairs)

        return pairs

    def _parse_frontmatter(self, content: str) -> Dict[str, Any]:
        """解析 YAML frontmatter"""
        if content.startswith('---'):
            parts = content.split('---', 2)
            if len(parts) >= 3:
                try:
                    return yaml.safe_load(parts[1]) or {}
                except yaml.YAMLError:
                    return {}
        return {}

    def _extract_critical_commands(self, content: str, frontmatter: Dict) -> List[Dict]:
        """提取关键命令列表"""
        commands = []

        # 从 frontmatter 提取
        agent_notes = frontmatter.get('agent_notes', {})
        critical = agent_notes.get('critical_commands', [])
        for cmd in critical:
            commands.append({'command': cmd, 'confidence': 0.95, 'source': 'frontmatter'})

        # 从正文提取所有 kubectl/etcdctl 命令
        seen = {c['command'] for c in commands}
        for pattern in self.COMMAND_PATTERNS:
            for match in re.finditer(pattern, content, re.MULTILINE | re.IGNORECASE):
                cmd = match.group(1).strip()
                if cmd and cmd not in seen and len(cmd) < 200:
                    commands.append({'command': cmd, 'confidence': 0.80, 'source': 'body'})
                    seen.add(cmd)

        return commands[:15]  # 限制每个 Skill 的命令数

    def _extract_diagnosis_sections(self, content: str) -> List[Dict]:
        """提取诊断段落"""
        sections = []
        # 匹配 "原因"、"诊断"、"根因" 等标题下的内容
        pattern = r'#{2,4}\s*(?:原因|诊断|根因|排查思路|分析)[：:]?\s*\n(.*?)(?=\n#{2,4}|\Z)'
        for match in re.finditer(pattern, content, re.DOTALL | re.IGNORECASE):
            text = match.group(1)
            # 提取列表项
            items = re.findall(r'[\*\-]\s*(.+)', text)
            sections.append({'type': 'diagnosis', 'items': items})
        return sections

    def _extract_action_sections(self, content: str) -> List[Dict]:
        """提取操作段落"""
        sections = []
        pattern = r'#{2,4}\s*(?:修复|解决方案|操作步骤|action|remediation)[：:]?\s*\n(.*?)(?=\n#{2,4}|\Z)'
        for match in re.finditer(pattern, content, re.DOTALL | re.IGNORECASE):
            text = match.group(1)
            items = re.findall(r'[\*\-]\s*(.+)', text)
            sections.append({'type': 'action', 'items': items})
        return sections

    def _extract_explicit_io_blocks(self, content: str, skill_id: str, skill_name: str, severity: str) -> List[Dict]:
        """提取文中显式的命令输出示例块"""
        pairs = []
        # 匹配 "输出示例"、"典型输出" 等标记的代码块
        pattern = r'(?:输出示例|典型输出|预期输出|错误输出)[：:]?\s*\n```[^\n]*\n(.*?)```'
        for match in re.finditer(pattern, content, re.DOTALL | re.IGNORECASE):
            output = match.group(1).strip()
            # 尝试找前面的命令
            before = content[:match.start()]
            cmd_match = re.search(r'`{3}[^\n]*\n?(kubectl [^`\n]+)`{3}|`(kubectl [^`\n]+)`', before)
            if cmd_match:
                cmd = cmd_match.group(1) or cmd_match.group(2)
                domain = self._infer_domain(skill_id)
                seq = self._next_sequence(domain)
                pairs.append({
                    'io_pair_id': f"IODIAG-{domain}-{seq:04d}",
                    'skill_ref': skill_id,
                    'scenario': skill_name,
                    'severity': self._map_severity(severity),
                    'command': cmd.strip(),
                    'output_pattern': output,
                    'diagnosis': [f"从 {skill_name} 的显式输出示例推断"],
                    'action': [],
                    'confidence': 0.90,
                    'tags': [domain],
                })
        return pairs

    def _infer_output(self, command: str) -> str:
        """根据命令推断典型输出"""
        for prefix, template in self.OUTPUT_INFERENCE.items():
            if command.startswith(prefix):
                return template
        # 通用模板
        return f"# 执行: {command}\n<typical output lines...>"

    def _match_diagnosis(self, command: str, sections: List[Dict]) -> List[str]:
        """匹配与命令相关的诊断"""
        diagnosis = []
        cmd_keywords = set(re.findall(r'[a-zA-Z]+', command.lower()))

        for section in sections:
            for item in section['items'][:5]:
                item_keywords = set(re.findall(r'[a-zA-Z]+', item.lower()))
                if cmd_keywords & item_keywords:
                    diagnosis.append(item.strip())

        return list(dict.fromkeys(diagnosis))[:5]  # 去重，最多5条

    def _match_actions(self, command: str, sections: List[Dict]) -> List[str]:
        """匹配与命令相关的操作"""
        actions = []
        cmd_keywords = set(re.findall(r'[a-zA-Z]+', command.lower()))

        for section in sections:
            for item in section['items'][:5]:
                item_keywords = set(re.findall(r'[a-zA-Z]+', item.lower()))
                if cmd_keywords & item_keywords or 'kubectl' in item or 'etcdctl' in item:
                    actions.append(item.strip())

        return list(dict.fromkeys(actions))[:5]

    def _infer_domain(self, filename: str) -> str:
        """从文件名推断 domain"""
        domain_map = {
            'node': 'NODE',
            'pod': 'POD',
            'dns': 'DNS',
            'service': 'NET',
            'network': 'NET',
            'certificate': 'CERT',
            'control-plane': 'CP',
            'apiserver': 'CP',
            'etcd': 'ETCD',
            'scheduler': 'CP',
            'deployment': 'WORK',
            'daemonset': 'WORK',
            'pvc': 'STORAGE',
            'storage': 'STORAGE',
            'ingress': 'INGRESS',
            'gateway': 'INGRESS',
            'rbac': 'SEC',
            'quota': 'SEC',
            'security': 'SEC',
            'upgrade': 'UPGRADE',
            'monitoring': 'OBS',
            'logging': 'OBS',
            'image': 'IMAGE',
            'gpu': 'GPU',
            'autoscal': 'SCALE',
            'configmap': 'CONFIG',
            'secret': 'CONFIG',
            'webhook': 'WEBHOOK',
            'job': 'WORK',
            'cronjob': 'WORK',
        }

        name_lower = filename.lower()
        for key, domain in domain_map.items():
            if key in name_lower:
                return domain
        return 'GENERAL'

    def _map_severity(self, sev: str) -> str:
        """映射严重度"""
        sev_lower = str(sev).lower()
        if 'p0' in sev_lower or 'critical' in sev_lower:
            return 'critical'
        if 'p1' in sev_lower or 'high' in sev_lower:
            return 'high'
        if 'p2' in sev_lower or 'medium' in sev_lower:
            return 'medium'
        return 'medium'

    def _extract_tags(self, filename: str, command: str) -> List[str]:
        """提取标签"""
        tags = []
        name_lower = filename.lower().replace('.md', '')
        tags.append(name_lower)

        if 'kubectl' in command:
            tags.append('kubectl')
        if 'etcdctl' in command:
            tags.append('etcdctl')
        if 'node' in command or 'node' in name_lower:
            tags.append('node')
        if 'pod' in command or 'pod' in name_lower:
            tags.append('pod')
        if 'get ' in command:
            tags.append('status')
        if 'describe' in command:
            tags.append('describe')
        if 'logs' in command:
            tags.append('logs')

        return list(dict.fromkeys(tags))

    def _next_sequence(self, domain: str) -> int:
        """获取下一个序列号"""
        self.sequence[domain] = self.sequence.get(domain, 0) + 1
        return self.sequence[domain]


if __name__ == '__main__':
    import sys
    skills_dir = sys.argv[1] if len(sys.argv) > 1 else '../../故障诊断/topic-skills'
    extractor = SkillExtractor(skills_dir)
    pairs = extractor.extract_all()
    print(f"\n总计提取: {len(pairs)} I-O pairs")
