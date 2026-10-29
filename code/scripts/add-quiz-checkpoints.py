#!/usr/bin/env python3
"""为培训课程批量追加自测题 checkpoint"""

import os
import re
import yaml
from pathlib import Path

BASE_DIR = Path("/Users/allengaller/Documents/GitHub/kudig-io/kudig-database")

# 每周主题的自测题模板
WEEK_CHECKPOINTS = {
    "week-1-foundation": [
        {"q": "Docker 容器与虚拟机的核心区别是什么?", "a": "容器共享宿主机内核, 通过 Namespace 隔离进程/网络/文件系统, 通过 Cgroup 限制资源; 虚拟机有独立内核, 通过 Hypervisor 虚拟化硬件。"},
        {"q": "Kubernetes 的四大核心组件是什么?", "a": "API Server (集群入口)、etcd (状态存储)、Scheduler (调度决策)、Controller Manager (状态 reconciliation)。"},
        {"q": "Linux 中如何查看端口占用?", "a": "ss -tlnp | grep <port> 或 netstat -tlnp | grep <port>"},
        {"q": "Dockerfile 中 COPY 和 ADD 的区别?", "a": "COPY 仅复制文件; ADD 额外支持 URL 下载和自动解压 tar 文件, 但行为不透明, 生产建议用 COPY。"},
        {"q": "如何查看 K8s 集群中所有节点的状态?", "a": "kubectl get nodes -o wide"},
    ],
    "week-2-core-tech": [
        {"q": "Pod 的三种 QoS 类分别是什么? 何时触发驱逐?", "a": "Guaranteed (requests=limits) > Burstable > BestEffort。资源不足时按 BestEffort→Burstable→Guaranteed 顺序驱逐。"},
        {"q": "Service 的三种主要类型有何区别?", "a": "ClusterIP (集群内部)、NodePort (节点端口暴露)、LoadBalancer (云 LB 暴露)。还有 Headless (无 ClusterIP, 用于 StatefulSet)。"},
        {"q": "PV 和 PVC 的关系是什么?", "a": "PV 是集群级存储资源, PVC 是命名空间级存储请求。PVC 通过 storageClassName 和 capacity 匹配 PV。"},
        {"q": "etcd 使用什么共识协议? 为什么需要奇数节点?", "a": "Raft 协议。奇数节点确保 majority (N/2+1), 如 3 节点容忍 1 故障, 5 节点容忍 2 故障。偶数节点不增加容错能力反而增加通信开销。"},
        {"q": "NetworkPolicy 的默认行为是什么?", "a": "如果命名空间没有 NetworkPolicy, 所有流量放行。一旦创建, 默认拒绝未匹配的流量 (ingress 和 egress 独立控制)。"},
    ],
    "week-3-operations": [
        {"q": "如何排查 CrashLoopBackOff?", "a": "1) kubectl logs <pod> --previous 查看上次崩溃日志; 2) kubectl describe pod 查看 Events; 3) 检查 livenessProbe 配置; 4) 检查资源 limits 是否 OOM。"},
        {"q": "Prometheus 的四种指标类型是什么?", "a": "Counter (只增计数器)、Gauge (可增可减仪表)、Histogram (分桶统计)、Summary (分位数统计)。"},
        {"q": "如何安全地升级 K8s 集群?", "a": "kubeadm upgrade plan → 升级控制面节点 (一次一个) → 升级 worker 节点 (逐个 drain/upgrade/uncordon)。先备份 etcd。"},
        {"q": "RBAC 中 Role 和 ClusterRole 的区别?", "a": "Role 是命名空间级, 仅授权特定 namespace 内资源; ClusterRole 是集群级, 可授权集群资源 (nodes, PV) 或所有 namespace 的资源。"},
        {"q": "Pod 处于 Pending 状态的常见原因?", "a": "资源不足 (Insufficient cpu/memory)、无匹配节点 (nodeSelector/affinity/taint)、PVC 未绑定、ResourceQuota 超限。"},
    ],
    "week-4-network-storage": [
        {"q": "Kubernetes Service 的 ClusterIP 是如何实现的?", "a": "kube-proxy 通过 iptables 或 IPVS 规则将 ClusterIP:Port 的流量 DNAT 到后端 Pod 的 PodIP:TargetPort。"},
        {"q": "Ingress 和 Gateway API 的区别?", "a": "Ingress 仅支持 HTTP/HTTPS, 功能有限 (需注解扩展); Gateway API 支持 HTTP/gRPC/TCP/TLS/UDP, 原生流量分割, 角色分离 (GatewayClass→Gateway→Route)。"},
        {"q": "StatefulSet 的 Pod 为什么有稳定的网络标识?", "a": "StatefulSet 创建的 Pod 名称格式为 <statefulset-name>-<ordinal>, 配合 Headless Service 创建 DNS 记录 <pod-name>.<service-name>.<namespace>.svc.cluster.local。"},
        {"q": "如何选择 CNI 插件?", "a": "Calico (通用, 支持 BGP/VXLAN, NetworkPolicy)、Cilium (eBPF, 高性能, 丰富 NetworkPolicy)、Flannel (简单, 仅 VXLAN, 无 NetworkPolicy)。生产推荐 Cilium 或 Calico。"},
        {"q": "PVC 的三种访问模式?", "a": "ReadWriteOnce (单节点读写)、ReadOnlyMany (多节点只读)、ReadWriteMany (多节点读写)。并非所有存储后端都支持全部模式。"},
    ],
}


def add_checkpoint_to_course(filepath: Path, week_key: str) -> bool:
    """为课程文件追加自测题"""
    if week_key not in WEEK_CHECKPOINTS:
        return False

    try:
        content = filepath.read_text(encoding='utf-8')
    except:
        return False

    # 如果已有自测题, 跳过
    if '自测题' in content or 'self-check' in content.lower():
        return False

    questions = WEEK_CHECKPOINTS[week_key]
    quiz_section = "\n\n---\n\n## 自测题 (Self-Check)\n\n"
    for i, qa in enumerate(questions, 1):
        quiz_section += f"### Q{i}. {qa['q']}\n\n"
        quiz_section += f"<details>\n<summary>查看答案</summary>\n\n{qa['a']}\n\n</details>\n\n"

    new_content = content.rstrip() + quiz_section
    filepath.write_text(new_content, encoding='utf-8')
    return True


def main():
    learn_dir = BASE_DIR / 'topic-learn'
    if not learn_dir.exists():
        print("topic-learn 目录不存在")
        return

    updated = 0

    # 处理 one-month 培训课程
    for week_key in WEEK_CHECKPOINTS:
        # 查找该周的 checkpoint.md 或 README.md
        for pattern in [
            f"*/{week_key}/checkpoint.md",
            f"*/{week_key}/README.md",
            f"**/{week_key}/checkpoint.md",
        ]:
            for fp in learn_dir.glob(pattern):
                if add_checkpoint_to_course(fp, week_key):
                    print(f"  已追加自测题: {fp.relative_to(BASE_DIR)}")
                    updated += 1

    # 处理 public-training 的周课程
    for week_key in WEEK_CHECKPOINTS:
        for fp in learn_dir.glob(f"**/{week_key}/checkpoint.md"):
            if add_checkpoint_to_course(fp, week_key):
                print(f"  已追加自测题: {fp.relative_to(BASE_DIR)}")
                updated += 1

    print(f"\n自测题追加完成: {updated} 个课程文件已更新")


if __name__ == '__main__':
    main()
