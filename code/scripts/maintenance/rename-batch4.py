#!/usr/bin/env python3
"""批量重命名二级子目录（git mv 保留历史）。"""
import subprocess
import sys

MAPPING = """
故障诊断/01-resource-troubleshooting|故障诊断/资源排障
故障诊断/03-advanced-troubleshooting|故障诊断/高级排障
故障诊断/tools|故障诊断/工具
故障诊断/topic-febm|故障诊断/FEBM方法论
故障诊断/topic-fta|故障诊断/FTA故障树
故障诊断/topic-skills|故障诊断/技能体系
故障诊断/topic-multi-fault-scenarios|故障诊断/多故障场景
故障诊断/topic-qa-corpus|故障诊断/QA语料

可观测性/01-overview|可观测性/总览
可观测性/02-metrics|可观测性/指标
可观测性/03-logging|可观测性/日志
可观测性/04-tracing|可观测性/链路追踪
可观测性/05-alerting|可观测性/告警
可观测性/06-slo-sli|可观测性/SLO-SLI
可观测性/07-tools|可观测性/工具

可靠性/01-backup-recovery|可靠性/备份恢复
可靠性/02-disaster-recovery|可靠性/灾难恢复
可靠性/03-capacity-planning|可靠性/容量规划
可靠性/05-chaos-engineering|可靠性/混沌工程
可靠性/06-postmortem|可靠性/事后复盘
可靠性/07-sre-practices|可靠性/SRE实践
可靠性/08-performance-testing|可靠性/性能测试

安全/01-identity-access|安全/身份与访问
安全/02-network-security|安全/网络安全
安全/03-runtime-security|安全/运行时安全
安全/04-policy-governance|安全/策略治理
安全/05-supply-chain|安全/供应链
安全/06-compliance|安全/合规审计

生产运维/01-finops|生产运维/成本治理
生产运维/02-governance|生产运维/集群治理
生产运维/03-incident-response|生产运维/事件响应
生产运维/04-green-computing|生产运维/绿色计算
生产运维/ticket-cases|生产运维/工单案例
生产运维/reply-templates|生产运维/回复话术

网络/00-core-k8s-networking|网络/K8s网络核心
网络/01-fundamentals|网络/网络基础
网络/02-service-mesh|网络/服务网格
网络/03-api-gateway|网络/API网关
网络/04-ebpf|网络/eBPF
网络/99-attachments|网络/附件
网络/topic-terway|网络/Terway

专项技术/01-edge-computing|专项技术/边缘计算
专项技术/02-webassembly|专项技术/WebAssembly
专项技术/03-extensions|专项技术/扩展机制
专项技术/04-serverless|专项技术/无服务器

集群基础/01-architecture-overview|集群基础/架构总览
集群基础/02-design-principles|集群基础/设计原则
集群基础/03-control-plane|集群基础/控制平面
集群基础/04-api-versions|集群基础/API版本
集群基础/06-upgrade-paths|集群基础/升级路径

AI基础设施/01-ai-infra|AI基础设施/基础设施
AI基础设施/02-ai-agents|AI基础设施/AI-Agents
AI基础设施/03-agent-runtime|AI基础设施/Agent运行时
AI基础设施/topic-ai-coding|AI基础设施/AI编码

存储/01-k8s-storage|存储/K8s存储
存储/02-storage-fundamentals|存储/存储基础
存储/03-distributed-storage|存储/分布式存储
存储/04-stateful-app-storage|存储/有状态应用存储

清单模式/01-yaml-reference|清单模式/YAML参考
清单模式/02-kustomize-patterns|清单模式/Kustomize模式
清单模式/03-helm-values-patterns|清单模式/Helm值模式

工作负载/00-core-workloads|工作负载/核心工作负载
工作负载/topic-java-kubernetes|工作负载/Java-on-K8s

系统基础/01-linux|系统基础/Linux
系统基础/02-hardware|系统基础/硬件
系统基础/03-kubernetes-events|系统基础/K8s事件
系统基础/topic-cheat-sheet|系统基础/速查卡
系统基础/topic-dictionary|系统基础/知识字典

平台工程/build|平台工程/构建
平台工程/operate|平台工程/运维
平台工程/governance|平台工程/治理
平台工程/developer-experience|平台工程/开发体验
平台工程/topic-code-analysis|平台工程/代码分析

数据库中间件/01-databases|数据库中间件/数据库
数据库中间件/02-cache|数据库中间件/缓存
数据库中间件/03-message-queues|数据库中间件/消息队列
数据库中间件/04-time-series-db|数据库中间件/时序数据库
数据库中间件/05-operator-management|数据库中间件/Operator管理
数据库中间件/06-data-streaming|数据库中间件/数据流

发布变更/01-gitops|发布变更/GitOps
发布变更/02-iac|发布变更/IaC
发布变更/03-change-management|发布变更/变更管理
发布变更/04-testing-quality|发布变更/测试质量
发布变更/topic-deployment|发布变更/部署方案
发布变更/topic-migration|发布变更/迁移方案

应用模式/sub-patterns|应用模式/子模式
应用模式/topic-application-architecture|应用模式/行业架构
应用模式/topic-production-patterns|应用模式/生产模式

生态参考/01-cncf-landscape|生态参考/CNCF全景
生态参考/02-papers|生态参考/论文
生态参考/topic-index|生态参考/领域索引

容器运行时/01-docker|容器运行时/Docker
容器运行时/02-image-management|容器运行时/镜像管理
容器运行时/03-containerd-cri-o|容器运行时/containerd-CRI-O
容器运行时/04-image-build|容器运行时/镜像构建
容器运行时/05-runtime-migration|容器运行时/运行时迁移
""".strip()

def main():
    ok, fail = 0, 0
    for line in MAPPING.splitlines():
        line = line.strip()
        if not line:
            continue
        src, dst = line.split('|')
        try:
            r = subprocess.run(['git', 'mv', src, dst], capture_output=True, text=True)
        except Exception as e:
            print(f"ERR  {src} -> {dst}: {e}")
            fail += 1
            continue
        if r.returncode != 0:
            print(f"FAIL {src} -> {dst}: {r.stderr.strip()}")
            fail += 1
        else:
            print(f"OK   {src} -> {dst}")
            ok += 1
    print(f"\n=== {ok} ok / {fail} fail ===")
    return 0 if fail == 0 else 1

if __name__ == '__main__':
    sys.exit(main())
