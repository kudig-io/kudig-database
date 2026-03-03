# Day 26: FTA/FEBM 专题深化

> **学习时间**: 4-5 小时 | **主题**: 故障诊断方法论进阶

---

## 今日目标

- [ ] 深入学习 FTA 生产落地
- [ ] 掌握 K8s 完整故障树
- [ ] 了解 AI Agent 运维结合

---

## 理论学习 (2h)

### 必读文档

1. **FTA 生产快速落地**
   - 文件: `../../topic-fta/23_fta_production_quick_start.md`

2. **K8s 完整故障树分析**
   - 文件: `../../topic-fta/kubernetes_fta_full_analysis.md`

3. **FEBM 生产快速落地**
   - 文件: `../../topic-febm/8_febm_production_quick_start.md`

4. **AI Agent 运维模式**
   - 文件: `../../topic-fta/10_agent_orchestration_patterns.md`

---

## 实践任务 (2.5h)

### 任务 1: 构建 K8s 故障全景树 (1h)

```
K8s 应用故障 (顶事件)
├─ [OR] 应用层故障
│  ├─ 代码 Bug
│  ├─ 配置错误
│  └─ 依赖服务不可用
│
├─ [OR] 平台层故障
│  ├─ [OR] Pod 问题
│  │  ├─ Pending
│  │  ├─ CrashLoopBackOff
│  │  ├─ OOMKilled
│  │  └─ ImagePullBackOff
│  │
│  ├─ [OR] Service 问题
│  │  ├─ Endpoints 为空
│  │  ├─ Selector 不匹配
│  │  └─ Port 配置错误
│  │
│  ├─ [OR] Ingress 问题
│  │  ├─ 路由规则错误
│  │  ├─ TLS 证书问题
│  │  └─ Controller 故障
│  │
│  └─ [OR] 存储问题
│     ├─ PVC 未绑定
│     ├─ 存储满
│     └─ IO 性能问题
│
├─ [OR] 控制平面故障
│  ├─ etcd 不可用
│  ├─ API Server 过载
│  ├─ Scheduler 故障
│  └─ Controller 故障
│
└─ [OR] 基础设施故障
   ├─ 节点故障
   ├─ 网络故障
   └─ 存储后端故障
```

### 任务 2: FEBM 实战演练 (1h)

模拟一个复杂故障并使用 FEBM 方法排查:

```markdown
## 故障案例: 应用间歇性超时

### 1. 证据收集

**现象描述:**
- 用户报告: 页面加载偶尔超时
- 发生频率: 约 10% 的请求

**收集的证据:**
- Prometheus 指标: P99 延迟从 200ms 飙升到 5s
- Pod 日志: 大量 "connection timeout" 错误
- 节点状态: CPU/内存正常
- 网络抓包: 存在 TCP 重传

### 2. 假设列表

| 假设 | 验证方法 | 结果 |
|------|----------|------|
| DNS 解析慢 | 检查 CoreDNS 指标 | 排除 |
| Pod 资源不足 | 检查 limits 和实际使用 | 排除 |
| 网络策略阻断 | 检查 NetworkPolicy | 排除 |
| CNI 问题 | 检查 CNI 日志 | **确认** |

### 3. 根因确定

CNI 插件配置导致部分 Pod 网络不稳定

### 4. 修复方案

- 临时: 重启受影响的 Pod
- 永久: 更新 CNI 配置，增加超时参数

### 5. 预防措施

- 添加网络延迟监控告警
- CNI 配置纳入变更管理
```

### 任务 3: 为复杂故障构建完整 FTA + FEBM (30min)

选择一个你遇到过的故障，完整记录:

1. FTA 故障树
2. FEBM 证据收集表
3. 根因分析
4. 修复和预防措施

---

## 费曼复述 (0.5h)

1. **FTA 如何系统化地覆盖所有可能的故障原因？**
2. **FEBM 中"证据"和"假设"的关系是什么？**
3. **如何将 FTA/FEBM 融入日常运维流程？**

---

## 今日检验

- [ ] 能够构建复杂系统的 FTA 故障树
- [ ] 能够使用 FEBM 方法系统排查故障
- [ ] 理解故障诊断方法论的价值
