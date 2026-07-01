---
title: 故障排查决策树 - Mermaid 可视化版 [troubleshooting]
description: 'title: 故障排查决策树 - Mermaid 可视化版'
category: learning
tags:
- k8s
- training
- hands-on
- apiserver
- kubelet
- cilium
- flannel
- calico
- coredns
- docker
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 15min
intent_queries:
- 故障排查决策树 - Mermaid 可视化版 是什么
- 如何 故障排查决策树 - Mermaid 可视化版
- Kubernetes 11 production operations 最佳实践
- 故障排查决策树 - Mermaid 可视化版 故障排查
- 故障排查决策树 - Mermaid 可视化版 排障步骤
trigger_keywords:
- 故障排查决策树
- Mermaid
- 可视化版
- production
- operations
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- cilium-basics
- cni-basics
created: "2026-05-23"
---

---
title: 故障排查决策树 - Mermaid 可视化版
description: '# 故障排查决策树 - Mermaid 可视化版'
category: learning
tags:
- tutorial
- k8s
- training
- lecturer
- apiserver
- [[kubelet|kubelet]]
- [[Cilium|cilium]]
- flannel
- calico
- [[CoreDNS|coredns]]
- docker
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- SRE 工程师
- 运维工程师
- 值班工程师
estimated_read_time: 5min
intent_queries:
- 故障排查决策树 - Mermaid 可视化版 是什么
- 如何 故障排查决策树 - Mermaid 可视化版
trigger_keywords:
- 故障排查决策树
- Mermaid
- 可视化版
- k8s
- learning
authors:
- name: KUDIG Team
  role: contributor
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'

tier: peripheral---

# 故障排查决策树 - Mermaid 可视化版

> **章节**: 工单场景 | **难度**: 入门/进阶 | **用途**: 快速诊断

---

## 使用说明

```
【Mermaid 决策树】

每个决策树都是 Mermaid flowchart 格式。
可以直接复制到以下工具查看：
• Mermaid Live Editor: https://mermaid.live
• VS Code Mermaid 插件（安装后按 Cmd+K 再按 V 预览）
• Notion、Miro 等支持 Mermaid 的工具

【复制方法】

选中 ```mermaid 和 ``` 之间的代码，粘贴到 Mermaid Live Editor。

【推荐工作流】

1. 用户描述问题 → 数字人识别问题类型
2. 数字人选择对应决策树
3. 按照决策树步骤排查
4. 每一步给出具体命令
```

---

## 1. Pod 处于 Pending

```mermaid
flowchart TD
    A["Pod 处于 Pending"] --> B["kubectl describe pod <name> -n <namespace>"]
    B --> C{查看 Events}
    C -->|"Insufficient cpu"| D["CPU 资源不足"]
    C -->|"Insufficient memory"| E["内存资源不足"]
    C -->|"node(s) had taint"| F["节点有污点"]
    C -->|"no nodes available"| G["无匹配节点"]
    C -->|"pvc not found"| H["PVC 未找到或 Pending"]
    C -->|"region constraint"| I["可用区约束"]
    D --> J["解决方案"]
    E --> J
    F --> J
    G --> J
    H --> J
    I --> J
    J --> K{选择方案}
    K -->|"减少资源请求"| L["kubectl edit pod 修改 resources.requests\n或 Deployment 的 spec.template.spec.containers[].resources"]
    K -->|"增加集群节点"| M["联系云厂商或运维扩容节点"]
    K -->|"清理不需要的 Pod"| N["kubectl delete pod <old-pod> --now"]
    K -->|"添加污点容忍"| O["在 Pod spec 添加:\ntolerations:\n- key: \"node.kubernetes.io/not-ready\"\n  operator: \"Exists\"\n  effect: \"NoSchedule\""]
    F --> O
    I --> P["检查节点的 label\nkubectl label nodes <node> topology.kubernetes.io/region=xxx\n或修改 Pod 的 nodeAffinity"]
    H --> Q["kubectl get pvc\nkubectl describe pvc <pvc-name>"]
    Q --> Q1{"PVC 状态"}
    Q1 -->|"Pending"| Q2["参考 PVC Pending 决策树"]
    Q1 -->|"Bound"| Q3["可能是 Attach 问题，检查 CSI"]
    Q2 --> R["还有问题吗?"]
    Q3 --> R
    L --> R
    M --> R
    N --> R
    O --> R
    P --> R
```

---

## 2. Pod 处于 CrashLoopBackOff

```mermaid
flowchart TD
    A["Pod CrashLoopBackOff"] --> B["kubectl logs <pod> --previous -n <namespace>"]
    B --> C{日志显示}
    C -->|"OOMKilled"| D["内存超限"]
    C -->|"exit code 1"| E["应用启动失败"]
    C -->|"connection refused"| F["依赖服务不可用"]
    C -->|"command not found"| G["启动命令错误"]
    C -->|"file not found"| H["配置文件缺失"]
    C -->|"permission denied"| I["权限问题"]
    C -->|"No such file or directory"| J["文件/目录不存在"]
    D --> K["增加内存限制\nresources:\n  limits:\n    memory: 2Gi"]
    E --> L["检查应用启动配置\nkubectl describe pod | grep -A10 'Container Config'\n或 kubectl exec -it <pod> -- /bin/sh 手动测试"]
    F --> M["检查依赖服务\n1. 数据库是否运行\n2. API 是否可达\n3. 环境变量配置"]
    G --> N["检查 command 配置\ncommand 和 args 是否正确\n镜像的 Entrypoint 是否被覆盖"]
    H --> O["检查 ConfigMap/Secret 挂载\nkubectl describe pod | grep -A10 'Volumes'\n确认配置文件路径正确"]
    I --> P["检查文件权限\nkubectl exec -it <pod> -- ls -la /path/to/file\n检查 volume 挂载的权限"]
    J --> Q["检查挂载路径\nkubectl exec -it <pod> -- ls -la /mount/path\n确认目录存在"]
    K --> R["还有问题吗?"]
    L --> R
    M --> R
    N --> R
    O --> R
    P --> R
    Q --> R
    A2["如果没看到日志"] --> A3["kubectl describe pod 看 Events"]
    A3 --> A4{报错}
    A4 -->|"Error"| A5["应用内部错误，看 describe pod 的 Events"]
    A4 -->|"BackOff"| A6["健康检查失败，看 Readiness/Liveness Probe"]
    A5 --> A7["kubectl logs <pod> 查看应用日志"]
    A6 --> A8["增加 initialDelaySeconds\n或修复 Probe 配置"]
    A7 --> R
    A8 --> R
```

---

## 3. Service 无法访问

```mermaid
flowchart TD
    A["Service 无法访问"] --> B["Step 1: kubectl get svc -n <namespace>"]
    B --> C{Service 存在?}
    C -->|否| D["检查 Service 名称"]
    C -->|是| E["Step 2: kubectl get endpoints <svc> -n <namespace>"]
    E --> F{Endpoints 为空?}
    F -->|是| G["没有 Pod 匹配 selector"]
    F -->|否| H["Step 3: kubectl get pods -n <namespace>"]
    G --> I["Step 4: kubectl describe svc <name> | grep Selector"]
    I --> J{Pod labels 匹配?}
    J -->|否| K["方案一: 修改 Service selector\nkubectl edit svc <name>\n\n方案二: 给 Pod 添加 label\nkubectl label pods <pod-name> app=web --overwrite"]
    J -->|是| L["Pod 可能还在启动\n等待几秒后再检查"]
    H --> M{Pod Running?}
    M -->|否| N["参考 Pod Pending/CrashLoop 排查"]
    M -->|是| O["Step 5: kubectl describe pod | grep -A5 'Conditions'\nReady 是否为 True?"]
    O --> P{Ready?}
    P -->|False| Q["ReadinessProbe 失败\nkubectl logs <pod> --previous\n检查健康检查端点是否正常"]
    P -->|True| R["Step 6: 检查网络策略\nkubectl get networkpolicy -n <namespace>"]
    R --> S{有 NetworkPolicy?}
    S -->|是| T["检查是否允许入口流量\nkubectl describe networkpolicy <name>"]
    S -->|否| U["检查 Ingress 配置\n如果外部访问通过 Ingress"]
    T --> U
    U --> V["测试集群内部访问\nkubectl run -it --rm test --image=busybox -- wget -qO- <svc>:<port>"]
    K --> W["还有其他问题吗?"]
    L --> W
    N --> W
    Q --> W
    T --> X{"允许?"}
    X -->|否| Y["修改 NetworkPolicy 允许流量\n或临时删除测试"]
    X -->|是| V
    Y --> V
    V --> Z{"可以访问?"}
    Z -->|是| W
    Z -->|否| V2["检查 CNI/网络插件"]
    V2 --> V3["kubectl get pods -n kube-system | grep cni"]
    V3 --> V4{CNI Running?}
    V4 -->|否| V5["重启 CNI"]
    V4 -->|是| V6["检查节点网络配置"]
    V5 --> V6
    V6 --> W
    D --> W
```

---

## 4. DNS 解析失败

```mermaid
flowchart TD
    A["DNS 解析失败"] --> B["kubectl run -it --rm dnsutils --image=tutum/dnsutils -- nslookup kubernetes.default"]
    B --> C{测试成功?}
    C -->|是| D["应用侧问题，检查 /etc/resolv.conf"]
    C -->|否| E["集群 DNS 有问题"]
    D --> D1["kubectl exec -it <pod> -- cat /etc/resolv.conf"]
    D1 --> D2{nameserver 正确?}
    D2 -->|否| D3["检查 kubelet 的 --cluster-dns 参数\n应该指向 CoreDNS ClusterIP (通常是 10.96.0.10)"]
    D2 -->|是| D4["检查应用日志\n应用可能使用了错误的 DNS 解析库"]
    D3 --> D5["还有问题吗?"]
    D4 --> D5
    E --> F["Step 2: kubectl get pods -n kube-system -l k8s-app=kube-dns"]
    F --> G{CoreDNS Running?}
    G -->|否| H["重启 CoreDNS\nkubectl rollout restart deployment/coredns -n kube-system"]
    G -->|是| I["Step 3: kubectl logs -n kube-system -l k8s-app=kube-dns --tail=50"]
    I --> J{有错误?}
    J -->|是| K["查看详细错误信息\n可能是上游 DNS 配置问题或网络问题"]
    J -->|否| L["Step 4: kubectl exec -it <pod> -- cat /etc/resolv.conf"]
    L --> M{nameserver 指向 10.96.0.10?}
    M -->|否| N["检查 kubelet 配置\n--cluster-dns 参数应该设置为 CoreDNS ClusterIP"]
    M -->|是| O["Step 5: 检查网络连通性\nkubectl exec -it <pod> -- ping 10.96.0.10"]
    O --> P{能 ping 通?}
    P -->|否| Q["检查 kube-proxy\nkubectl get pods -n kube-system | grep kube-proxy"]
    P -->|是| R["检查 CNI\nkubectl get pods -n kube-system | grep -E 'calico|flannel|cilium'"]
    Q --> Q1{kube-proxy Running?}
    Q1 -->|否| Q2["重启 kube-proxy\nkubectl rollout restart daemonset/kube-proxy -n kube-system"]
    Q1 -->|是| Q3["查看 kube-proxy 日志"]
    R --> R1{CNI Running?}
    R1 -->|否| R2["重启 CNI"]
    R1 -->|是| R3["检查 iptables/ipvs 规则"]
    Q2 --> Q4["还有问题吗?"]
    Q3 --> Q4
    R2 --> Q4
    R3 --> Q4
    H --> Q4
    K --> Q4
    N --> Q4
```

---

## 5. 节点 NotReady

```mermaid
flowchart TD
    A["节点 NotReady"] --> B["kubectl get nodes\n查看状态是 NotReady 还是 Unknown"]
    B --> C{状态}
    C -->|"Unknown"| D["节点失联，网络或节点本身问题"]
    C -->|"NotReady"| E["describe node 查看 Conditions"]
    D --> D1["SSH 到节点检查\nsystemctl status kubelet"]
    D1 --> D2{kubelet 运行中?}
    D2 -->|否| D3["systemctl start kubelet\nsystemctl enable kubelet"]
    D2 -->|是| D4["journalctl -u kubelet --since '10 minutes ago'"]
    E --> E1{什么条件异常?}
    E1 -->|"MemoryPressure"| F["节点内存不足"]
    E1 -->|"DiskPressure"| G["节点磁盘不足"]
    E1 -->|"PIDPressure"| H["节点 PID 不足"]
    E1 -->|"NetworkUnavailable"| I["节点网络不可用"]
    E1 -->|"KubeletNotReady"| J["kubelet 和 API Server 通信问题"]
    F --> F1["kubectl describe node | grep -A5 'Allocated resources'"]
    F1 --> F2["找出内存使用高的 Pod\nkubectl top pods --all-namespaces | sort -k 2 -r | head"]
    G --> G1["docker system prune -a\nkubectl exec -it <node> -- docker system prune -a"]
    H --> H1["检查进程数\nps aux | wc -l"]
    I --> I1["检查网络配置\nip addr\nip route"]
    I1 --> I2["检查 CNI 状态"]
    J --> J1["检查证书\nopenssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates"]
    J1 --> J2{证书过期?}
    J2 -->|是| J3["kubeadm alpha certificates renew"]
    J2 -->|否| J4["检查 /var/log/kubelet.log"]
    D3 --> D5["重启后检查 kubectl get nodes"]
    D4 --> D5
    F2 --> F3["排查内存泄漏或增加节点"]
    G1 --> G2["清理后检查 kubectl get nodes"]
    H1 --> H2["限制节点进程数或增加 PID limit"]
    I2 --> I3["重启 CNI 或恢复网络"]
    J3 --> D5
    J4 --> J5["根据日志修复"]
    F3 --> D5
    G2 --> D5
    H2 --> D5
    I3 --> D5
    J5 --> D5
```

---

## 6. HPA 不触发扩容

```mermaid
flowchart TD
    A["HPA 不触发扩容"] --> B["kubectl get hpa -n <namespace>"]
    B --> C{"Metrics Available?"}
    C -->|Unknown/Unable| D["Metrics Server 问题"]
    C -->|Available| E["检查 target 是否达到"]
    D --> D1["kubectl get pods -n kube-system | grep metrics"]
    D1 --> D2{Metrics Server Running?}
    D2 -->|否| D3["重启 Metrics Server\nkubectl rollout restart deployment/metrics-server -n kube-system"]
    D2 -->|是| D4["查看 Metrics Server 日志\nkubectl logs -n kube-system -l k8s-app=k8s-dashboard-metrics-server --tail=50"]
    D4 --> D5["检查是否有错误，如 'context deadline exceeded'"]
    E --> E1{"当前 CPU > Target?"}
    E1 -->|否| E2["负载还没达到扩容阈值\n继续观察"]
    E1 -->|是| F["检查 Pod 资源请求"]
    F --> F1{kubectl describe pod <pod> | grep -A5 "Requests"}
    F1 -->|"没有 requests"| G["HPA 需要 Pod 设置 resources.requests 才能计算使用率"]
    F1 -->|"有 requests"| H["检查 HPA 配置"]
    G --> G1["kubectl edit deployment 添加:\nresources:\n  requests:\n    cpu: 100m\n    memory: 128Mi"]
    H --> H1["kubectl describe hpa <name> | grep -A20 'Conditions'"]
    H1 --> H2{AbleToScale?}
    H1 --> H3{ScalingActive?}
    H2 -->|"False"| H4["HPA 无法扩展，检查 Conditions"]
    H3 -->|"False"| H5["HPA 被禁用，可能在冷却期"]
    H4 --> H6["kubectl get events -n <namespace> --sort-by=.lastTimestamp | grep HPA"]
    H5 --> H7["等待冷却期结束或调整 behavior 配置"]
    H6 --> H8["解决阻止扩展的问题"]
    G1 --> H9["还有问题吗?"]
    H8 --> H9
    H7 --> H9
    E2 --> H9
    D3 --> D6["重启后检查 HPA"]
    D6 --> D7["还有问题吗?"]
    D5 --> D6
```

---

## 7. PVC Pending

```mermaid
flowchart TD
    A["PVC Pending"] --> B["kubectl describe pvc <name> -n <namespace>"]
    B --> C{查看 Events}
    C -->|"no persistent volumes available"| D["没有符合条件的 PV"]
    C -->|"StorageClass not found"| E["StorageClass 不存在"]
    C -->|"waiting for first consumer"| F["等待调度到 Pod"]
    C -->|"cannot find storage provisioner"| G["存储制备器问题"]
    C -->|"quota exceeded"| H["存储配额不足"]
    D --> D1["kubectl get storageclass\nkubectl get pv"]
    D1 --> D2{有可用 PV?}
    D2 -->|是| D3["检查 PV 的 accessModes 和 capacity 是否匹配 PVC"]
    D2 -->|否| D4["需要创建 PV 或使用 StorageClass 动态制备"]
    E --> E1["kubectl get storageclass"]
    E1 --> E2{StorageClass 存在?}
    E2 -->|否| E3["创建 StorageClass:\nkubectl create -f storageclass.yaml"]
    E2 -->|是| E4["PVC 的 storageClassName 是否正确"]
    F --> F1["这是正常的，PVC 等待 Pod 调度后才会制备"]
    F1 --> F2["等待 Pod 调度完成后 PVC 会自动绑定"]
    G --> G1["检查 CSI driver\nkubectl get pods -n kube-system | grep csi"]
    G1 --> G2{CSI Running?}
    G2 -->|否| G3["重启 CSI driver"]
    G2 -->|是| G4["查看 CSI driver 日志"]
    H --> H1["联系云厂商增加存储配额"]
    D3 --> D5["修改 PVC 或创建新的 PV"]
    D4 --> D6["方法一: 创建 PV\n方法二: 确保 StorageClass 可用"]
    E3 --> E5["创建后重新检查 PVC"]
    E4 --> E5
    F2 --> E5
    G3 --> G5["重启后检查 PVC"]
    G4 --> G5
    H1 --> H2["配额增加后 PVC 会自动绑定"]
    D5 --> D7["还有问题吗?"]
    D6 --> D7
    E5 --> D7
    G5 --> D7
    H2 --> D7
```

---

## 8. Deployment 滚动更新卡住

```mermaid
flowchart TD
    A["Deployment 滚动更新卡住"] --> B["kubectl get deployment <name> -n <namespace>"]
    B --> C{"Available < Replicas?"}
    C -->|否| D["Deployment 已完成，检查其他问题"]
    C -->|是| E["新 Pod 没有 Ready"]
    E --> F["kubectl describe pod -n <namespace> | grep -A5 'Containers'"]
    F --> F1{新 Pod 状态}
    F1 -->|"Pending"| F2["调度失败，参考 Pod Pending 排查"]
    F1 -->|"ImagePullBackOff"| F3["镜像拉取失败"]
    F1 -->|"CrashLoopBackOff"| F4["应用启动失败"]
    F1 -->|"Running 但 NotReady"| F5["ReadinessProbe 失败"]
    F1 -->|"Terminating"| F6["Pod 删除卡住"]
    F2 --> F7["参考 Pod Pending 决策树"]
    F3 --> F8["检查镜像名称和凭证"]
    F4 --> F9["kubectl logs <pod> --previous"]
    F5 --> F10["检查 ReadinessProbe 配置\nkubectl describe pod | grep -A10 'Readiness'"]
    F6 --> F11["kubectl delete pod <pod> --grace-period=0 --force"]
    D --> D1["检查滚动更新策略配置\nkubectl describe deployment | grep -A5 'Strategy'"]
    D1 --> D2{maxUnavailable = 0?}
    D2 -->|是| D3["maxUnavailable=0 会限制更新速度\n如果有资源限制，可能卡住"]
    D2 -->|否| D4["检查 maxSurge 设置"]
    F7 --> R["还有问题吗?"]
    F8 --> R
    F9 --> R
    F10 --> R
    F11 --> R
    D3 --> R
    D4 --> D5["如果需要更快完成，增大 maxSurge"]
    D5 --> R
    A2["如果想快速恢复"] --> A3["kubectl rollout undo deployment/<name> -n <namespace>"]
    A3 --> A4["回滚到上一个版本"]
    A4 --> R
```

---

## 9. Ingress 404

```mermaid
flowchart TD
    A["Ingress 返回 404"] --> B["kubectl get ingress -n <namespace>"]
    B --> C{Ingress 存在?}
    C -->|否| D["检查 Ingress 名称"]
    C -->|是| E["kubectl describe ingress <name>"]
    E --> E1{检查 Rules}
    E1 -->|"没有 Rules"| E2["Ingress 没有配置规则"]
    E1 -->|"有 Rules"| E3["检查 Backend"]
    E2 --> E4["添加 Rules 或删除重建 Ingress"]
    E3 --> E5{"service.name 存在?"}
    E5 -->|否| E6["创建 Service 或修改 Ingress"]
    E5 -->|是| F["kubectl get endpoints <svc> -n <namespace>"]
    F --> F1{Endpoints 有值?}
    F1 -->|否| F2["Pod 未运行或 selector 不匹配"]
    F1 -->|是| G["检查 Ingress Class"]
    E6 --> H["还有问题吗?"]
    F2 --> I["参考 Service 无法访问排查"]
    G --> G1{kubectl get ingressclass\ningressClassName 配置正确?}
    G1 -->|否| G2["设置正确的 ingressClassName"]
    G1 -->|是| H["检查 Ingress Controller"]
    H --> H1["kubectl get pods -n ingress-nginx"]
    H1 --> H2{Ingress Controller Running?}
    H2 -->|否| H3["重启 Ingress Controller\nkubectl rollout restart deployment/ingress-nginx-controller -n ingress-nginx"]
    H2 -->|是| I["检查域名解析"]
    I --> I1["ping <ingress-host>"]
    I1 --> I2{"域名解析到正确的 IP?"}
    I2 -->|否| I3["修改 DNS 解析到 Ingress Controller 的 IP"]
    I2 -->|是| I4["curl -H 'Host: <host>' <Ingress-Controller-IP>"]
    I4 --> I5{"能访问?"}
    I5 -->|是| H["可能是用户侧问题"]
    I5 -->|否| I6["Ingress Controller 配置问题"]
    I6 --> I7["检查 Ingress Controller 配置和日志"]
    G2 --> H
    H3 --> H
    I3 --> H
    I7 --> H
```

---

## 10. RBAC Forbidden

```mermaid
flowchart TD
    A["RBAC Forbidden 403"] --> B["kubectl auth whoami"]
    B --> C["确认当前用户/ServiceAccount"]
    C --> D["kubectl auth can-i --list"]
    D --> E["查看当前所有权限"]
    E --> F["测试具体操作\nkubectl auth can-i <verb> <resource> -n <namespace>"]
    F --> G{权限足够?}
    G -->|是| H["可能需要加 --subresource"]
    G -->|否| I["缺少权限"]
    H --> H1["kubectl auth can-i <verb> <resource>/<subresource> -n <namespace>"]
    H1 --> H2{权限足够?}
    H2 -->|是| H3["操作是允许的，检查其他原因"]
    H2 -->|否| H4["确认是权限问题，继续排查"]
    I --> J["创建 Role"]
    J --> J1["kubectl create role <name> --verb=get,list --resource=pods -n <namespace>"]
    J1 --> J2["绑定到用户/组/SA"]
    J2 --> J3["kubectl create rolebinding <name> --role=<role-name> --user=<user> -n <namespace>"]
    J3 --> J4["测试权限\nkubectl auth can-i get pods -n <namespace>"]
    J4 --> J5{权限正常?}
    J5 -->|是| J6["完成"]
    J5 -->|否| J7["检查 RoleBinding 是否正确绑定"]
    J7 --> J8["kubectl get rolebinding -n <namespace>"]
    J8 --> J9["确认 Role 和 Subject 正确"]
    H4 --> K["如果是集群级别权限"]
    K --> K1["kubectl create clusterrole <name> --verb=get,list --resource=pods"]
    K1 --> K2["kubectl create clusterrolebinding <name> --clusterrole=<role-name> --user=<user>"]
    K2 --> J4
    H3 --> R["还有其他问题吗?"]
    J6 --> R
    J9 --> R
    A2["如果不知道当前用户是谁"] --> A3["检查 kubeconfig\nkubectl config current-context"]
    A3 --> A4["kubectl config view --minify"]
    A4 --> A5["确认用户和上下文配置正确"]
    A5 --> R
```

---

## 快速复制区

```markdown
## Pod Pending 决策树

```mermaid
flowchart TD
    A["Pod 处于 Pending"] --> B{describe pod 看 Events}
    B --> C{常见报错}
    C -->|"Insufficient cpu/memory"| D["资源不足"]
    C -->|"node(s) had taint"| E["节点有污点"]
    D --> F["减少请求/增加节点/清理 Pod"]
    E --> G["添加污点容忍"]
```

## Service 无法访问决策树

```mermaid
flowchart TD
    A["Service 无法访问"] --> B["Step 1: 检查 Service 存在"]
    B --> C{Service 存在?}
    C -->|否| D["检查名称"]
    C -->|是| E["Step 2: 检查 Endpoints"]
    E --> F{Endpoints 为空?}
    F -->|是| G["没有 Pod 匹配 selector"]
```
```

---

## 一页速查表

| 问题 | 第一步命令 | 关键检查点 |
|------|-----------|-----------|
| Pod Pending | `describe pod` | Events 的报错信息 |
| CrashLoopBackOff | `logs --previous` | 上一容器日志 |
| Service 不通 | `get endpoints` | Endpoints 是否为空 |
| DNS 失败 | `nslookup kubernetes.default` | CoreDNS 是否 Running |
| 节点 NotReady | `describe node` | Conditions 部分 |
| HPA 不工作 | `get hpa` | Metrics Server 是否运行 |
| PVC Pending | `describe pvc` | Events 报错 |
| Ingress 404 | `describe ingress` | Rules 和 Backend |
| RBAC Forbidden | `auth can-i --list` | 当前用户权限 |

---

**关联文档**:
- [../README.md](../README.md) — 讲师完整台词设计
- [../oncall-qa/oncall-quick-qa.md](../oncall-qa/oncall-quick-qa.md) — On-Call 快速问答
- [../../P1-4-decision-tree-mermaid-visualization.md](../../P1-4-decision-tree-mermaid-visualization.md) — 完整决策树库
- [../../domain-10-troubleshooting-diagnostics/](../../domain-10-troubleshooting-diagnostics/) — 故障排查文档

## See Also

- kubernetes-workload-presentation
- presentation-template
- 01-what-is-kubernetes
- 02-pod-basics


## 参见

- [[skills/training-lecturer/12-decision-tree/decision-tree-mermaid.md|讲师版]]
