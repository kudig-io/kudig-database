# Day 19: 故障排查方法论 (关键日)

> **学习时间**: 4-5 小时 | **主题**: FTA/FEBM 结构化故障排查

---

## 今日目标

- [ ] 理解 FTA 故障树分析方法
- [ ] 掌握 FEBM 取证循证方法论
- [ ] 建立结构化排障思维框架

---

## 理论学习 (2h) - 方法论精读

### 必读文档

1. **结构化故障排查框架**
   - 文件: `../../topic-structural-trouble-shooting/README.md`
   - 重点: 排障框架总览

2. **FTA 核心原理**
   - 文件: `../../topic-fta/04-fta-core-principles.md`
   - 重点: 故障树构建、根因分析

3. **FEBM 理论基础**
   - 文件: `../../topic-febm/01-febm-theory-foundations.md`
   - 重点: 取证循证方法论

---

## 实践任务 (2.5h)

### 任务 1: 理解 FTA 故障树 (45min)

FTA (Fault Tree Analysis) 核心概念:

```
顶事件 (Top Event): 要分析的故障现象
    ├── 中间事件 (Intermediate Event)
    │   ├── 基本事件 (Basic Event)
    │   └── 基本事件
    └── 中间事件
        ├── 基本事件
        └── 基本事件

逻辑门:
- AND 门: 所有子事件都发生，父事件才发生
- OR 门: 任一子事件发生，父事件就发生
```

示例: Pod 无法启动故障树

```
Pod 无法启动 (顶事件)
├─ [OR] 调度失败
│  ├─ 资源不足
│  ├─ nodeSelector 无匹配
│  └─ Taints 阻止
├─ [OR] 镜像问题
│  ├─ 镜像不存在
│  ├─ 拉取权限不足
│  └─ 网络不通
└─ [OR] 存储问题
   ├─ PVC 未绑定
   └─ 存储类不存在
```

### 任务 2: Pod Pending 完整排障 (45min)

参考 `../../domain-12-troubleshooting/05-pod-pending-diagnosis.md`

```bash
# 创建会 Pending 的 Pod
cat > pending-pod.yaml << 'EOF'
apiVersion: v1
kind: Pod
metadata:
  name: pending-test
spec:
  containers:
  - name: app
    image: nginx:alpine
    resources:
      requests:
        cpu: "100"       # 故意设置过大
        memory: "1000Gi"
EOF

kubectl apply -f pending-pod.yaml

# 排障步骤:
# 1. 查看 Pod 状态
kubectl get pod pending-test

# 2. 查看详细信息
kubectl describe pod pending-test

# 3. 查看事件
kubectl get events --field-selector involvedObject.name=pending-test

# 4. 分析原因 (资源不足)

# 5. 修复
kubectl delete pod pending-test
# 修改资源请求后重新创建
```

### 任务 3: Node NotReady 排障 (45min)

参考 `../../domain-12-troubleshooting/06-node-notready-diagnosis.md`

排障步骤清单:

```bash
# 1. 检查节点状态
kubectl get nodes
kubectl describe node <node-name>

# 2. 检查节点条件
kubectl get node <node-name> -o jsonpath='{.status.conditions}' | jq

# 3. 检查 kubelet 状态 (在节点上)
systemctl status kubelet
journalctl -u kubelet -n 100

# 4. 检查容器运行时
systemctl status containerd
crictl ps

# 5. 检查网络
ping <api-server-ip>
curl -k https://<api-server>:6443/healthz

# 6. 检查磁盘
df -h
du -sh /var/lib/kubelet

# 7. 检查内存
free -h
cat /proc/meminfo

# 8. 检查系统日志
dmesg | tail -50
```

### 任务 4: FEBM 实践 (30min)

FEBM (Forensic Evidence-Based Method) 核心流程:

```
1. 收集证据
   - 日志
   - 指标
   - 配置
   - 事件

2. 分析证据
   - 时间线重建
   - 因果关系分析
   - 排除法

3. 形成假设
   - 基于证据推理
   - 验证假设

4. 记录结论
   - 根因
   - 修复方案
   - 预防措施
```

练习: 记录一次排障过程

```markdown
## 故障报告

**时间**: YYYY-MM-DD HH:MM
**影响**: 描述影响范围

### 1. 现象
- 观察到的症状

### 2. 证据收集
- 日志片段
- 指标截图
- 配置检查

### 3. 分析过程
- 假设 1 -> 验证 -> 结果
- 假设 2 -> 验证 -> 结果

### 4. 根因
- 确定的根本原因

### 5. 修复方案
- 临时缓解
- 永久修复

### 6. 预防措施
- 监控告警
- 配置检查
```

---

## 费曼复述 (0.5h)

1. **FTA 故障树的 AND 门和 OR 门分别在什么场景使用？**
2. **FEBM 方法论的核心步骤是什么？**
3. **为什么结构化排障比"经验排障"更可靠？**

---

## 今日检验

- [ ] 能够画出简单故障的 FTA 故障树
- [ ] 能够按照结构化流程排查 Pod 问题
- [ ] 能够使用 FEBM 方法记录排障过程

---

## 核心排障命令速查

| 场景 | 命令 |
|------|------|
| Pod 状态 | `kubectl get pods -o wide` |
| Pod 详情 | `kubectl describe pod <name>` |
| Pod 日志 | `kubectl logs <name> --previous` |
| 事件 | `kubectl get events --sort-by='.lastTimestamp'` |
| 节点状态 | `kubectl describe node <name>` |
| API 健康 | `kubectl get --raw /healthz` |
