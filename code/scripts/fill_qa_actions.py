#!/usr/bin/env python3
"""
为 QA 语料文件自动填充 action 字段。
根据 command 和 diagnosis 推断合理的修复/检查动作。
生成 *_with_actions.md 新文件，不覆盖原文件。
"""

import re
from pathlib import Path

import yaml


# 动作规则库：按 (command 关键词, diagnosis 关键词) → action 列表映射
ACTION_RULES = [
    # Pod 相关
    ("get pods", ["crashloop", "oom", "error", "restart", "failed", "panic"], [
        {"command": "kubectl logs <pod-name> -n <namespace> --previous", "description": "查看异常 Pod 的上一次容器日志", "risk_level": "low"},
        {"command": "kubectl describe pod <pod-name> -n <namespace>", "description": "查看 Pod Events 和 Conditions", "risk_level": "low"},
    ]),
    ("describe pod", ["oomkilled", "memory", "limits"], [
        {"command": "kubectl set resources deployment/<deployment-name> -n <namespace> --limits=memory=<new-limit> --requests=memory=<new-request>", "description": "调整容器内存限制", "risk_level": "medium"},
    ]),
    ("describe pod", ["imagepullbackoff", "errimagepull", "pull", "镜像"], [
        {"command": "kubectl create secret docker-registry acr-credential --docker-server=<registry> --docker-username=<username> --docker-password=<password> -n <namespace>", "description": "创建镜像拉取凭证", "risk_level": "low"},
        {"command": "kubectl patch serviceaccount default -n <namespace> -p '{\"imagePullSecrets\": [{\"name\": \"acr-credential\"}]}'", "description": "为 ServiceAccount 绑定镜像拉取凭证", "risk_level": "low"},
    ]),

    # Node 相关
    ("get nodes", ["notready", "unreachable", "diskpressure", "memorypressure", "pidpressure"], [
        {"command": "kubectl describe node <node-name>", "description": "查看节点 Conditions 和事件", "risk_level": "low"},
        {"command": "kubectl get events --field-selector involvedObject.name=<node-name> --sort-by='.lastTimestamp'", "description": "查看节点相关事件", "risk_level": "low"},
    ]),
    ("describe node", ["diskpressure", "磁盘"], [
        {"command": "kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name> --sort-by=memory", "description": "识别节点上资源消耗大户", "risk_level": "low"},
        {"command": "kubectl cordon <node-name> && kubectl drain <node-name> --ignore-daemonsets --delete-emptydir-data --force", "description": "封禁并排空节点以便清理或替换", "risk_level": "high"},
    ]),
    ("describe node", ["memorypressure", "内存"], [
        {"command": "kubectl top pods --all-namespaces --field-selector spec.nodeName=<node-name> --sort-by=memory", "description": "识别内存消耗大户", "risk_level": "low"},
        {"command": "kubectl evict-pod <pod-name> -n <namespace>", "description": "驱逐高内存消耗 Pod（谨慎操作）", "risk_level": "high"},
    ]),

    # 证书相关
    ("", ["cert", "certificate", "tls", "过期"], [
        {"command": "openssl x509 -in /var/lib/kubelet/pki/kubelet-client-current.pem -noout -dates", "description": "检查 kubelet 客户端证书有效期", "risk_level": "low"},
        {"command": "systemctl restart kubelet", "description": "重启 kubelet 触发证书自动轮转", "risk_level": "medium"},
    ]),

    # DNS 相关
    ("", ["dns", "coredns", "解析"], [
        {"command": "kubectl rollout restart deployment/coredns -n kube-system", "description": "重启 CoreDNS 使配置生效", "risk_level": "medium"},
        {"command": "kubectl get configmap coredns -n kube-system -o yaml | kubectl apply -f -", "description": "重新应用 CoreDNS 配置", "risk_level": "medium"},
    ]),

    # NetworkPolicy / 网络
    ("", ["networkpolicy", "网络策略", "503", "连接被拒绝"], [
        {"command": "kubectl get networkpolicy -n <namespace> -o yaml", "description": "查看当前 NetworkPolicy 规则", "risk_level": "low"},
        {"command": "kubectl delete networkpolicy <policy-name> -n <namespace>", "description": "临时删除问题 NetworkPolicy 恢复业务", "risk_level": "high"},
    ]),

    # PVC / 存储
    ("", ["pvc", "persistentvolumeclaim", "挂载", "failedmount", "存储"], [
        {"command": "kubectl get pvc <pvc-name> -n <namespace> -o yaml", "description": "查看 PVC 详细状态和事件", "risk_level": "low"},
        {"command": "kubectl get events -n <namespace> --field-selector reason=FailedMount", "description": "查看挂载失败事件", "risk_level": "low"},
        {"command": "kubectl get pv <pv-name> -o yaml", "description": "查看 PV 后端存储状态", "risk_level": "low"},
    ]),

    # SLB / LoadBalancer
    ("", ["slb", "loadbalancer", "负载均衡"], [
        {"command": "aliyun slb DescribeLoadBalancerAttribute --LoadBalancerId <slb-id> --RegionId <region-id>", "description": "查询 SLB 后端服务器组和健康状态", "risk_level": "low"},
        {"command": "aliyun slb DescribeVServerGroupAttribute --VServerGroupId <vserver-group-id> --RegionId <region-id>", "description": "查询 SLB 虚拟服务器组后端状态", "risk_level": "low"},
    ]),

    # etcd
    ("", ["etcd"], [
        {"command": "etcdctl endpoint status --cluster -w table", "description": "检查 etcd 集群健康状态", "risk_level": "low"},
        {"command": "etcdctl endpoint health --cluster", "description": "检查 etcd 端点健康", "risk_level": "low"},
        {"command": "etcdctl defrag --cluster", "description": "对 etcd 进行碎片整理", "risk_level": "high"},
    ]),

    # Deployment / 滚动更新
    ("", ["deployment", "滚动更新", "rollout", "replicas"], [
        {"command": "kubectl rollout status deployment/<deployment-name> -n <namespace>", "description": "查看 Deployment 滚动更新状态", "risk_level": "low"},
        {"command": "kubectl rollout undo deployment/<deployment-name> -n <namespace>", "description": "回滚 Deployment 到上一个版本", "risk_level": "medium"},
    ]),

    # HPA
    ("", ["hpa", "autoscale", "metrics"], [
        {"command": "kubectl get hpa <hpa-name> -n <namespace> -o yaml", "description": "查看 HPA 配置和当前指标", "risk_level": "low"},
        {"command": "kubectl rollout restart deployment/metrics-server -n kube-system", "description": "重启 metrics-server", "risk_level": "medium"},
    ]),

    # ServiceAccount / RBAC
    ("", ["rbac", "serviceaccount", "权限", "forbidden"], [
        {"command": "kubectl auth can-i <verb> <resource> --as=system:serviceaccount:<namespace>:<sa-name> -n <namespace>", "description": "验证 ServiceAccount 权限", "risk_level": "low"},
        {"command": "kubectl create rolebinding <name> --role=<role> --serviceaccount=<namespace>:<sa-name> -n <namespace>", "description": "为 ServiceAccount 绑定角色", "risk_level": "medium"},
    ]),

    # ConfigMap / Secret
    ("", ["configmap", "secret", "配置"], [
        {"command": "kubectl get configmap <configmap-name> -n <namespace> -o yaml", "description": "查看 ConfigMap 内容", "risk_level": "low"},
        {"command": "kubectl rollout restart deployment/<deployment-name> -n <namespace>", "description": "重启应用使配置生效", "risk_level": "medium"},
    ]),
]


def infer_actions(command: str, diagnosis: list, scenario: str = "", tags: list = None) -> list:
    """根据 command、diagnosis、scenario、tags 推断 action。"""
    diag_text = " ".join(diagnosis).lower() if isinstance(diagnosis, list) else str(diagnosis).lower()
    cmd_lower = command.lower()
    scenario_lower = scenario.lower()
    tag_text = " ".join(tags).lower() if tags else ""
    combined = f"{cmd_lower} {diag_text} {scenario_lower} {tag_text}"
    actions = []
    seen_commands = set()

    for cmd_kw, diag_kws, rule_actions in ACTION_RULES:
        # command 关键词匹配（空字符串表示任意 command）
        cmd_match = (not cmd_kw) or (cmd_kw in cmd_lower)
        # diagnosis 关键词至少命中一个
        diag_match = any(kw in combined for kw in diag_kws)
        if cmd_match and diag_match:
            for action in rule_actions:
                if action["command"] not in seen_commands:
                    actions.append(action)
                    seen_commands.add(action["command"])

    # 兜底：如果没有推断出任何 action，给出一个通用查看命令
    if not actions:
        actions.append({
            "command": command,
            "description": "复现诊断命令并确认当前状态",
            "risk_level": "low"
        })

    return actions


def normalize_actions(actions):
    """将 action 统一转换为结构化对象列表。"""
    if not actions:
        return []
    if isinstance(actions, str):
        actions = [actions]
    result = []
    for act in actions:
        if isinstance(act, dict) and "command" in act:
            # 已经是结构化对象
            result.append(act)
        elif isinstance(act, str):
            # 字符串命令，转换为结构化对象
            cmd = act.strip()
            # 去除注释部分
            if "  # " in cmd:
                cmd_part, desc_part = cmd.split("  # ", 1)
                description = desc_part.strip()
            elif " # " in cmd:
                cmd_part, desc_part = cmd.split(" # ", 1)
                description = desc_part.strip()
            else:
                cmd_part = cmd
                description = "执行修复/检查命令"
            # 判断风险等级
            risk = "low"
            dangerous = ["delete", "drain", "cordon", "evict", "restart", "rollback", "force", "prune"]
            if any(d in cmd_part.lower() for d in ["delete", "drain", "cordon", "evict", "force"]):
                risk = "high"
            elif any(d in cmd_part.lower() for d in ["restart", "rollback", "prune", "apply", "patch"]):
                risk = "medium"
            result.append({
                "command": cmd_part.strip(),
                "description": description,
                "risk_level": risk
            })
    return result


def process_file(input_path: Path, max_pairs: int = None) -> Path:
    output_path = input_path.with_suffix(".with_actions.md")
    content = input_path.read_text(encoding="utf-8")

    # 匹配 ```yaml ... ``` 块
    yaml_block_pattern = re.compile(r"(```yaml\n)(.*?)(\n```)", re.DOTALL)

    pair_count = 0
    modified_count = 0
    skipped_count = 0
    normalized_count = 0

    def replace_block(match):
        nonlocal pair_count, modified_count, skipped_count, normalized_count
        if max_pairs is not None and pair_count >= max_pairs:
            return match.group(0)
        pair_count += 1

        yaml_text = match.group(2)
        try:
            data = yaml.safe_load(yaml_text)
        except yaml.YAMLError as e:
            print(f"    YAML 解析失败: {e}")
            return match.group(0)

        if not isinstance(data, dict):
            return match.group(0)

        command = data.get("command", "")
        diagnosis = data.get("diagnosis", [])
        scenario = data.get("scenario", "")
        tags = data.get("tags", [])

        existing_action = data.get("action")

        # 情况 1：action 是字符串列表 → 结构化转换
        if existing_action and (isinstance(existing_action, str) or
                                (isinstance(existing_action, list) and existing_action and
                                 isinstance(existing_action[0], str))):
            data["action"] = normalize_actions(existing_action)
            normalized_count += 1
            modified_count += 1

        # 情况 2：action 已存在且为结构化对象列表且非空 → 跳过
        elif existing_action and isinstance(existing_action, list) and existing_action and isinstance(existing_action[0], dict):
            skipped_count += 1
            return match.group(0)

        # 情况 3：action 不存在 → 推断
        else:
            actions = infer_actions(command, diagnosis, scenario, tags)
            data["action"] = actions
            modified_count += 1

        # 重新序列化为 YAML
        new_yaml = yaml.safe_dump(data, allow_unicode=True, sort_keys=False, default_flow_style=False)
        return f"```yaml\n{new_yaml}```"

    new_content = yaml_block_pattern.sub(replace_block, content)
    output_path.write_text(new_content, encoding="utf-8")
    return output_path, modified_count, skipped_count, pair_count


def main():
    files = [
        # generated 批量 QA 文件
        Path("故障诊断/topic-qa-corpus/generated/command-output-diagnosis-p0.md"),
        Path("故障诊断/topic-qa-corpus/generated/command-output-diagnosis-p1.md"),
        Path("故障诊断/topic-qa-corpus/generated/command-output-diagnosis-p2.md"),
        # 核心命令输出解读语料
        Path("故障诊断/topic-qa-corpus/command-output-diagnosis.md"),
        # P0 核心场景手工种子
        Path("故障诊断/topic-qa-corpus/seed/p0-core-scenarios.md"),
    ]

    total_pairs = 0
    total_modified = 0
    total_skipped = 0

    for f in files:
        if not f.exists():
            print(f"跳过不存在的文件: {f}")
            continue
        print(f"处理: {f.name}")
        output, modified, skipped, total = process_file(f, max_pairs=None)
        print(f"  扫描 I-O 对: {total}")
        print(f"  已存在 action 跳过: {skipped}")
        print(f"  填充 action: {modified}")
        print(f"  输出: {output}")
        print()
        total_pairs += total
        total_modified += modified
        total_skipped += skipped

    print("=" * 50)
    print(f"总计扫描: {total_pairs} 个 I-O 对")
    print(f"已存在 action 跳过: {total_skipped}")
    print(f"新填充 action: {total_modified}")


if __name__ == "__main__":
    main()
