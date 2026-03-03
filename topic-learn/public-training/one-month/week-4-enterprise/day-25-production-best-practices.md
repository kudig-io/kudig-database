# Day 25: 生产运维最佳实践

> **学习时间**: 4-5 小时 | **主题**: 变更管理与事故响应

---

## 今日目标

- [ ] 理解生产架构设计原则
- [ ] 掌握变更管理流程
- [ ] 建立事故响应机制

---

## 理论学习 (2h) - 知识整合日

### 必读文档

1. **生产架构设计原则**
   - 文件: `../../domain-18-production-operations/01-production-architecture-design-principles.md`

2. **变更管理流程**
   - 文件: `../../domain-18-production-operations/22-change-management-process.md`

3. **事故响应处理**
   - 文件: `../../domain-18-production-operations/23-incident-response-handling.md`

4. **容量规划预测**
   - 文件: `../../domain-18-production-operations/24-capacity-planning-forecasting.md`

---

## 实践任务 (2.5h)

### 任务 1: 变更管理 SOP (1h)

创建 `~/change-management-sop.md`:

```markdown
# 变更管理标准操作流程

## 1. 变更分类

### 标准变更
- 已知低风险变更
- 无需审批
- 示例: 配置参数微调

### 正常变更
- 需要审批的常规变更
- 需要 CAB 评审
- 示例: 应用版本升级

### 紧急变更
- 紧急修复
- 事后补审批
- 示例: 安全漏洞修复

## 2. 变更流程

### 提交阶段
- [ ] 变更描述
- [ ] 影响范围
- [ ] 回滚方案
- [ ] 测试验证

### 审批阶段
- [ ] 技术评审
- [ ] 业务评审
- [ ] 安全评审

### 执行阶段
- [ ] 变更窗口确认
- [ ] 执行变更
- [ ] 验证结果
- [ ] 通知相关方

### 复盘阶段
- [ ] 记录结果
- [ ] 更新文档
- [ ] 经验总结

## 3. 回滚触发条件

- 核心指标异常
- 错误率显著上升
- 用户投诉增加
- 执行超时
```

### 任务 2: 事故响应 Runbook (1h)

创建 `~/incident-response-runbook.md`:

```markdown
# 事故响应 Runbook

## 严重级别定义

| 级别 | 描述 | 响应时间 | 示例 |
|------|------|----------|------|
| P1 | 核心业务完全不可用 | 5 分钟 | 全站宕机 |
| P2 | 核心功能受影响 | 15 分钟 | 支付失败 |
| P3 | 非核心功能受影响 | 1 小时 | 报表延迟 |
| P4 | 轻微问题 | 4 小时 | UI 显示异常 |

## 响应流程

### 1. 发现阶段 (0-5min)
- 确认告警真实性
- 评估影响范围
- 确定严重级别
- 通知相关人员

### 2. 响应阶段 (5-30min)
- 组建响应团队
- 初步定位问题
- 执行临时缓解
- 持续沟通状态

### 3. 恢复阶段 (30min-N)
- 根因分析
- 执行修复
- 验证恢复
- 恢复业务

### 4. 复盘阶段 (事后)
- 时间线整理
- 根因分析
- 改进措施
- 文档更新

## 常见问题快速响应

### Pod 大面积 Pending
1. `kubectl get nodes` 检查节点状态
2. `kubectl describe node` 检查资源
3. 紧急扩容或释放资源

### Service 不可用
1. `kubectl get endpoints` 检查后端
2. `kubectl get pods -l <selector>` 检查 Pod
3. 滚动重启或回滚

### 数据库连接问题
1. 检查 Secret 配置
2. 检查网络策略
3. 检查数据库状态
```

### 任务 3: 容量规划 (30min)

```bash
# 收集历史指标
# CPU 使用趋势
# sum(rate(container_cpu_usage_seconds_total[1h])) by (namespace)

# 内存使用趋势
# sum(container_memory_usage_bytes) by (namespace)

# Pod 数量趋势
# count(kube_pod_info) by (namespace)

# 规划公式
# 预留容量 = 当前使用 * (1 + 增长率) * 冗余系数
```

---

## 费曼复述 (0.5h)

1. **变更管理的核心目标是什么？**
2. **事故响应的 MTTD 和 MTTR 是什么？如何改进？**
3. **如何做好容量规划？**

---

## 今日检验

- [ ] 能够制定变更管理 SOP
- [ ] 能够编写事故响应 Runbook
- [ ] 理解容量规划方法
