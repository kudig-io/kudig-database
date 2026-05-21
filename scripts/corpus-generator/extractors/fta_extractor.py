#!/usr/bin/env python3
"""
FTA → I-O Pair 提取器
从 topic-fta/list/*.md 中提取故障树终端事件的命令输出模式
"""

import re
import yaml
from pathlib import Path
from typing import List, Dict, Any


class FTAExtractor:
    """从 FTA Markdown 中提取 I-O 对"""

    # FTA 中常见的命令输出示例模式
    OUTPUT_BLOCK_MARKERS = [
        r'(?:典型输出|输出示例|命令输出|错误输出|预期输出)[：:]?\s*\n```[^\n]*\n(.*?)```',
        r'(?:kubectl|etcdctl|systemctl|journalctl|curl).+?\n```[^\n]*\n(.*?)```',
    ]

    # 故障事件模式（FTA 叶节点）
    EVENT_MARKERS = [
        r'#{3,4}\s*(?:叶节点|终端事件|基础事件|BE)[：:]?\s*\n?(.*?)(?=\n#{3,4}|\Z)',
        r'(?:故障现象|错误信息|异常输出)[：:]\s*(.+)',
    ]

    def __init__(self, fta_dir: str):
        self.fta_dir = Path(fta_dir)
        self.io_pairs: List[Dict[str, Any]] = []
        self.sequence = {}

    def extract_all(self, priority_ftas: List[str] = None) -> List[Dict[str, Any]]:
        """提取所有 FTA 的 I-O 对"""
        fta_files = sorted(self.fta_dir.glob('*.md'))

        if priority_ftas:
            prioritized = [f for f in fta_files if any(p in f.name for p in priority_ftas)]
            others = [f for f in fta_files if f not in prioritized]
            fta_files = prioritized + others

        for fta_file in fta_files:
            pairs = self._extract_from_file(fta_file)
            self.io_pairs.extend(pairs)
            print(f"  [FTA] {fta_file.name}: {len(pairs)} I-O pairs")

        return self.io_pairs

    def _extract_from_file(self, fta_file: Path) -> List[Dict[str, Any]]:
        """从单个 FTA 文件提取"""
        content = fta_file.read_text(encoding='utf-8')
        pairs = []

        # 提取 frontmatter
        frontmatter = self._parse_frontmatter(content)
        fta_name = frontmatter.get('title', fta_file.stem)

        # 提取显式的命令输出块
        explicit_pairs = self._extract_explicit_blocks(content, fta_file.stem, fta_name)
        pairs.extend(explicit_pairs)

        # 从叶节点提取故障现象和检查命令
        leaf_pairs = self._extract_leaf_nodes(content, fta_file.stem, fta_name)
        pairs.extend(leaf_pairs)

        # 从 FTA 的"验证方法"段落提取
        verify_pairs = self._extract_verification_methods(content, fta_file.stem, fta_name)
        pairs.extend(verify_pairs)

        return pairs

    def _parse_frontmatter(self, content: str) -> Dict[str, Any]:
        if content.startswith('---'):
            parts = content.split('---', 2)
            if len(parts) >= 3:
                try:
                    return yaml.safe_load(parts[1]) or {}
                except yaml.YAMLError:
                    return {}
        return {}

    def _extract_explicit_blocks(self, content: str, fta_id: str, fta_name: str) -> List[Dict]:
        """提取显式的命令输出代码块"""
        pairs = []
        pattern = r'(?:输出示例|典型输出|命令输出)[：:]?\s*\n```(?:yaml|json|text|bash|shell)?\n(.*?)```'

        for match in re.finditer(pattern, content, re.DOTALL | re.IGNORECASE):
            block = match.group(1).strip()
            # 找前面的命令
            before = content[:match.start()]
            cmd_match = re.search(r'`{3}(?:bash|shell)?\n?(.*?)`{3}|`([^`]+)`', before[-500:])
            command = "<推断命令>"
            if cmd_match:
                command = (cmd_match.group(1) or cmd_match.group(2)).strip()

            domain = self._infer_domain(fta_id)
            seq = self._next_sequence(domain)

            # 从代码块推断诊断
            diagnosis = self._infer_diagnosis_from_block(block, fta_name)

            pairs.append({
                'io_pair_id': f"IODIAG-{domain}-{seq:04d}",
                'fta_ref': fta_id,
                'scenario': fta_name,
                'severity': 'high',
                'command': command,
                'output_pattern': block,
                'diagnosis': diagnosis,
                'action': [],
                'confidence': 0.88,
                'tags': [domain.lower(), 'fta'],
            })

        return pairs

    def _extract_leaf_nodes(self, content: str, fta_id: str, fta_name: str) -> List[Dict]:
        """从叶节点提取"""
        pairs = []
        # 匹配包含命令或输出的叶节点
        leaf_pattern = r'(?:#{3,4}\s*(?:叶节点|终端事件|基础事件)[：:]?\s*\n)?(.*?)(?=\n#{3,4}|\Z)'

        for match in re.finditer(leaf_pattern, content, re.DOTALL):
            block = match.group(1)
            # 检查是否包含 kubectl 命令
            cmd_matches = re.findall(r'`{3}(?:bash|shell)?\n?(kubectl [^`]+)`{3}|`(kubectl [^`]+)`', block)
            for cmd_tuple in cmd_matches:
                cmd = cmd_tuple[0] or cmd_tuple[1]
                # 找对应的输出描述
                output_desc = self._find_output_description(block, cmd)
                if output_desc:
                    domain = self._infer_domain(fta_id)
                    seq = self._next_sequence(domain)
                    pairs.append({
                        'io_pair_id': f"IODIAG-{domain}-{seq:04d}",
                        'fta_ref': fta_id,
                        'scenario': fta_name,
                        'severity': 'high',
                        'command': cmd.strip(),
                        'output_pattern': output_desc,
                        'diagnosis': [f"FTA 叶节点: {fta_name}"],
                        'action': [],
                        'confidence': 0.82,
                        'tags': [domain.lower(), 'fta', 'leaf-node'],
                    })

        return pairs

    def _extract_verification_methods(self, content: str, fta_id: str, fta_name: str) -> List[Dict]:
        """从验证方法段落提取"""
        pairs = []
        verify_pattern = r'(?:验证方法|排查步骤|检查命令)[：:]?\s*\n(.*?)(?=\n#{2,3}|\Z)'

        for match in re.finditer(verify_pattern, content, re.DOTALL | re.IGNORECASE):
            block = match.group(1)
            # 提取带编号的步骤
            steps = re.findall(r'\d+\.\s*`{3}(?:bash|shell)?\n?(.*?)`{3}', block, re.DOTALL)
            for i, step in enumerate(steps[:5]):
                lines = step.strip().split('\n')
                if lines:
                    command = lines[0].strip()
                    output = '\n'.join(lines[1:]) if len(lines) > 1 else "<verify output>"
                    domain = self._infer_domain(fta_id)
                    seq = self._next_sequence(domain)
                    pairs.append({
                        'io_pair_id': f"IODIAG-{domain}-{seq:04d}",
                        'fta_ref': fta_id,
                        'scenario': f"{fta_name} - 验证步骤 {i+1}",
                        'severity': 'medium',
                        'command': command,
                        'output_pattern': output,
                        'diagnosis': [f"FTA 验证: {fta_name}"],
                        'action': [],
                        'confidence': 0.85,
                        'tags': [domain.lower(), 'fta', 'verification'],
                    })

        return pairs

    def _find_output_description(self, block: str, command: str) -> str:
        """在文本块中找命令对应的输出描述"""
        # 找命令后面的代码块
        cmd_escaped = re.escape(command[:30])
        pattern = rf'{cmd_escaped}.*?\n```[^\n]*\n(.*?)```'
        match = re.search(pattern, block, re.DOTALL)
        if match:
            return match.group(1).strip()
        # 否则找 "输出"、"结果" 等关键字后面的内容
        out_match = re.search(r'(?:输出|结果)[：:]\s*(.+?)(?=\n\n|\n#{1,3}|\Z)', block, re.DOTALL)
        if out_match:
            return out_match.group(1).strip()
        return "<typical output for this fault>"

    def _infer_diagnosis_from_block(self, block: str, fta_name: str) -> List[str]:
        """从输出块推断诊断"""
        diagnosis = []
        # 检查常见错误模式
        if 'Error' in block or 'error' in block:
            diagnosis.append("命令执行返回错误")
        if 'NotReady' in block:
            diagnosis.append("资源状态异常")
        if 'unhealthy' in block.lower():
            diagnosis.append("服务健康检查失败")
        if 'failed' in block.lower() or 'Failed' in block:
            diagnosis.append("操作执行失败")
        if not diagnosis:
            diagnosis.append(f"FTA 场景: {fta_name}")
        return diagnosis

    def _infer_domain(self, filename: str) -> str:
        """推断 domain"""
        domain_map = {
            'node': 'NODE', 'pod': 'POD', 'dns': 'DNS',
            'service': 'NET', 'networkpolicy': 'NET', 'calico': 'NET',
            'cilium': 'NET', 'flannel': 'NET', 'terway': 'NET',
            'certificate': 'CERT', 'apiserver': 'CP', 'etcd': 'ETCD',
            'scheduler': 'CP', 'controller': 'CP', 'deployment': 'WORK',
            'daemonset': 'WORK', 'statefulset': 'WORK', 'job': 'WORK',
            'pvc': 'STORAGE', 'csi': 'STORAGE', 'storage': 'STORAGE',
            'ingress': 'INGRESS', 'gateway': 'INGRESS', 'higress': 'INGRESS',
            'nginx': 'INGRESS', 'rbac': 'SEC', 'quota': 'SEC',
            'psp': 'SEC', 'webhook': 'WEBHOOK', 'upgrade': 'UPGRADE',
            'monitoring': 'OBS', 'backup': 'DR', 'autoscaler': 'SCALE',
            'hpa': 'SCALE', 'vpa': 'SCALE', 'gpu': 'GPU', 'helm': 'HELM',
            'crd': 'CP', 'operator': 'CP', 'kubeadm': 'CP', 'openkruise': 'WORK',
        }
        name_lower = filename.lower()
        for key, domain in domain_map.items():
            if key in name_lower:
                return domain
        return 'FTA'

    def _next_sequence(self, domain: str) -> int:
        self.sequence[domain] = self.sequence.get(domain, 0) + 1
        return self.sequence[domain]


if __name__ == '__main__':
    import sys
    fta_dir = sys.argv[1] if len(sys.argv) > 1 else '../../domain-10-troubleshooting-diagnostics/topic-fta/list'
    extractor = FTAExtractor(fta_dir)
    pairs = extractor.extract_all()
    print(f"\n总计提取: {len(pairs)} I-O pairs")
