#!/usr/bin/env python3
"""
最佳实践内容质量检查工具

用法:
    python3 check_best_practices.py [目录路径]
    
功能:
    1. 检查最佳实践内容格式
    2. 验证链接有效性
    3. 检查代码示例语法
    4. 生成质量报告
"""

import os
import re
import sys
import json
from pathlib import Path
from typing import Dict, List, Any
from dataclasses import dataclass
from datetime import datetime

@dataclass
class QualityCheckResult:
    """质量检查结果"""
    file_path: str
    checks_passed: int
    checks_failed: int
    issues: List[str]
    score: float

class BestPracticeChecker:
    """最佳实践内容质量检查器"""
    
    def __init__(self, base_path: str):
        self.base_path = Path(base_path)
        self.results: List[QualityCheckResult] = []
        
    def check_all(self) -> Dict[str, Any]:
        """检查所有文件"""
        print(f"开始检查最佳实践内容质量...")
        print(f"基础路径: {self.base_path}")
        print("-" * 50)
        
        # 统计信息
        total_files = 0
        total_issues = 0
        passed_files = 0
        
        # 遍历所有markdown文件
        for md_file in self.base_path.rglob("*.md"):
            if md_file.name == "README.md":
                continue  # 跳过README文件
                
            total_files += 1
            result = self.check_file(md_file)
            self.results.append(result)
            
            if result.checks_failed == 0:
                passed_files += 1
            total_issues += result.checks_failed
            
            # 打印进度
            status = "✓" if result.checks_failed == 0 else "✗"
            print(f"{status} {md_file.relative_to(self.base_path)}: {result.checks_passed}/{result.checks_passed + result.checks_failed}")
        
        # 生成报告
        report = self.generate_report(total_files, passed_files, total_issues)
        
        print("-" * 50)
        print(f"检查完成: {passed_files}/{total_files} 文件通过")
        print(f"总问题数: {total_issues}")
        
        return report
    
    def check_file(self, file_path: Path) -> QualityCheckResult:
        """检查单个文件"""
        checks_passed = 0
        checks_failed = 0
        issues = []
        
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            # 检查1: 文件头信息
            if self.check_frontmatter(content):
                checks_passed += 1
            else:
                checks_failed += 1
                issues.append("缺少或格式错误的文件头信息")
            
            # 检查2: 标题结构
            if self.check_heading_structure(content):
                checks_passed += 1
            else:
                checks_failed += 1
                issues.append("标题结构不完整或不符合规范")
            
            # 检查3: 代码块语法
            if self.check_code_blocks(content):
                checks_passed += 1
            else:
                checks_failed += 1
                issues.append("代码块语法错误或缺少语言标识")
            
            # 检查4: 链接有效性
            if self.check_links(content, file_path):
                checks_passed += 1
            else:
                checks_failed += 1
                issues.append("包含无效链接")
            
            # 检查5: 最佳实践内容完整性
            if self.check_best_practice_content(content):
                checks_passed += 1
            else:
                checks_failed += 1
                issues.append("最佳实践内容不完整")
            
            # 计算得分
            total_checks = checks_passed + checks_failed
            score = (checks_passed / total_checks * 100) if total_checks > 0 else 0
            
            return QualityCheckResult(
                file_path=str(file_path.relative_to(self.base_path)),
                checks_passed=checks_passed,
                checks_failed=checks_failed,
                issues=issues,
                score=score
            )
            
        except Exception as e:
            return QualityCheckResult(
                file_path=str(file_path.relative_to(self.base_path)),
                checks_passed=0,
                checks_failed=1,
                issues=[f"读取文件失败: {str(e)}"],
                score=0
            )
    
    def check_frontmatter(self, content: str) -> bool:
        """检查文件头信息"""
        # 检查是否有YAML frontmatter
        if not content.startswith("---"):
            return False
        
        # 检查是否有title和description
        if "title:" not in content or "description:" not in content:
            return False
        
        return True
    
    def check_heading_structure(self, content: str) -> bool:
        """检查标题结构"""
        # 检查是否有H1标题
        if not re.search(r'^# ', content, re.MULTILINE):
            return False
        
        # 检查是否有H2标题
        if not re.search(r'^## ', content, re.MULTILINE):
            return False
        
        return True
    
    def check_code_blocks(self, content: str) -> bool:
        """检查代码块语法"""
        # 查找所有代码块
        code_blocks = re.findall(r'```(\w+)?\n[\s\S]*?```', content)
        
        # 检查是否有代码块
        if not code_blocks:
            return True  # 没有代码块不算错误
        
        # 检查代码块是否有语言标识
        for block in code_blocks:
            if not block:  # 空语言标识
                return False
        
        return True
    
    def check_links(self, content: str, file_path: Path) -> bool:
        """检查链接有效性"""
        # 查找所有相对链接
        relative_links = re.findall(r'\[([^\]]+)\]\(([^)]+)\)', content)
        
        for link_text, link_url in relative_links:
            if link_url.startswith(('http://', 'https://')):
                continue  # 跳过外部链接
            
            # 检查相对链接
            if link_url.startswith('#'):
                continue  # 跳过锚点链接
            
            # 解析相对路径
            link_path = file_path.parent / link_url
            
            # 检查文件是否存在
            if not link_path.exists():
                return False
        
        return True
    
    def check_best_practice_content(self, content: str) -> bool:
        """检查最佳实践内容完整性"""
        # 检查是否有最佳实践相关章节
        best_practice_sections = [
            "最佳实践",
            "Best Practice",
            "实施步骤",
            "验证方法",
            "常见陷阱"
        ]
        
        found_sections = 0
        for section in best_practice_sections:
            if section in content:
                found_sections += 1
        
        # 至少需要2个相关章节
        return found_sections >= 2
    
    def generate_report(self, total_files: int, passed_files: int, total_issues: int) -> Dict[str, Any]:
        """生成质量报告"""
        # 计算总体统计
        total_score = sum(r.score for r in self.results) / len(self.results) if self.results else 0
        
        # 按问题类型统计
        issue_counts = {}
        for result in self.results:
            for issue in result.issues:
                issue_counts[issue] = issue_counts.get(issue, 0) + 1
        
        # 按得分分类
        high_score = len([r for r in self.results if r.score >= 80])
        medium_score = len([r for r in self.results if 60 <= r.score < 80])
        low_score = len([r for r in self.results if r.score < 60])
        
        report = {
            "检查时间": datetime.now().isoformat(),
            "总文件数": total_files,
            "通过文件数": passed_files,
            "通过率": f"{(passed_files / total_files * 100):.1f}%" if total_files > 0 else "0%",
            "总问题数": total_issues,
            "平均得分": f"{total_score:.1f}%",
            "得分分布": {
                "高分 (≥80%)": high_score,
                "中分 (60-79%)": medium_score,
                "低分 (<60%)": low_score
            },
            "常见问题": dict(sorted(issue_counts.items(), key=lambda x: x[1], reverse=True)[:5]),
            "详细结果": [
                {
                    "文件": r.file_path,
                    "得分": f"{r.score:.1f}%",
                    "通过检查": r.checks_passed,
                    "失败检查": r.checks_failed,
                    "问题": r.issues
                }
                for r in sorted(self.results, key=lambda x: x.score)[:10]  # 显示得分最低的10个文件
            ]
        }
        
        return report
    
    def save_report(self, report: Dict[str, Any], output_path: str):
        """保存报告到文件"""
        with open(output_path, 'w', encoding='utf-8') as f:
            json.dump(report, f, ensure_ascii=False, indent=2)
        print(f"报告已保存到: {output_path}")

def main():
    """主函数"""
    if len(sys.argv) < 2:
        print("用法: python3 check_best_practices.py [目录路径]")
        print("示例: python3 check_best_practices.py /path/to/best-practices")
        sys.exit(1)
    
    base_path = sys.argv[1]
    
    if not os.path.exists(base_path):
        print(f"错误: 目录不存在 - {base_path}")
        sys.exit(1)
    
    # 创建检查器
    checker = BestPracticeChecker(base_path)
    
    # 执行检查
    report = checker.check_all()
    
    # 保存报告
    output_path = os.path.join(base_path, "best_practices_quality_report.json")
    checker.save_report(report, output_path)
    
    # 打印摘要
    print("\n=== 质量报告摘要 ===")
    print(f"总文件数: {report['总文件数']}")
    print(f"通过文件数: {report['通过文件数']}")
    print(f"通过率: {report['通过率']}")
    print(f"平均得分: {report['平均得分']}")
    print(f"总问题数: {report['总问题数']}")
    
    if report['常见问题']:
        print("\n常见问题:")
        for issue, count in report['常见问题'].items():
            print(f"  - {issue}: {count}次")

if __name__ == "__main__":
    main()