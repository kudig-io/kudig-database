---
title: "[2026-09-01] [P1] GPU 显存泄漏导致训练任务失败"
category: case-study
tags: [production, incident, ai-ml, gpu, cuda, memory-leak]
date: "2026-09-01"
severity: P1
mttr: "42min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-09-01] PyTorch 显存泄漏导致 GPU 训练节点全部 OOM，任务队列积压

## 工单信息
- **工单编号**: INC-2026-0901-019
- **发现时间**: 2026-09-01 02:00 UTC
- **恢复时间**: 2026-09-01 02:42 UTC
- **影响范围**: `ml-training` namespace，4 个 GPU 节点，12 个训练 Job
- **业务影响**: 推荐模型训练中断 42 分钟，当日模型更新延迟

## 问题现象
02:00，ML 平台告警 GPU 训练任务失败率 > 80%。训练 Pod 状态显示 `Error` 或 `OOMKilled`：
```bash
kubectl get pods -n ml-training
# NAME              READY   STATUS      RESTARTS
# train-resnet-0    0/1     OOMKilled   0
# train-bert-1      0/1     Error       0
# ...
```

## 诊断过程

**02:05** — 查看 GPU 节点状态：
```bash
kubectl get nodes -l nvidia.com/gpu.present=true
# NAME                         STATUS   GPU
# ip-10-0-10-10.ec2.internal   Ready    4/4
# ip-10-0-10-11.ec2.internal   Ready    0/4
# ip-10-0-10-12.ec2.internal   Ready    0/4
# ip-10-0-10-13.ec2.internal   Ready    0/4
```

3 个节点的 GPU 全部不可用。

**02:07** — 检查 GPU 显存使用：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n ml-training nvidia-device-plugin-xxx -- \
  nvidia-smi
# +---------------------------------------------------------------------------------------+
# | NVIDIA-SMI 535.104.05             Driver Version: 535.104.05   CUDA Version: 12.2     |
# |-----------------------------------------+----------------------+----------------------+
# | GPU  Name                 Persistence-M | Bus-Id        Disp.A | Volatile Uncorr. ECC |
# | Fan  Temp   Perf          Pwr:Usage/Cap |         Memory-Usage | GPU-Util  Compute M. |
# |                                         |                      |               MIG M. |
# |=========================================+======================+======================|
# |   0  NVIDIA A100-SXM4-40GB            Off| 00000000:00:04.0 Off |                    0 |
# | N/A   35C    P0              65W / 400W |  39990MiB / 40960MiB |      0%      Default |
# +-----------------------------------------+----------------------+----------------------+
# |   1  NVIDIA A100-SXM4-40GB            Off| 00000000:00:05.0 Off |                    0 |
# | N/A   36C    P0              62W / 400W |  39985MiB / 40960MiB |      0%      Default |
# ...
```

显存几乎 100% 占用，但 GPU 利用率为 0%（无活跃进程）。

**02:10** — 检查僵尸进程：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n ml-training nvidia-device-plugin-xxx -- \
  nvidia-smi pids -i 0
# No running processes found
```

显存被占用但没有运行进程，典型的显存泄漏。

**02:12** — 查看之前完成的训练 Job：
```bash
kubectl get jobs -n ml-training
# NAME              COMPLETIONS   DURATION   AGE
# train-resnet-0    1/1           45m        2h
# train-bert-1      1/1           120m       3h
# ...
```

已完成的 Job 的 Pod 仍在节点上：
```bash
kubectl get pods -n ml-training -o wide | grep Completed
# train-resnet-0-xxx   0/1   Completed   ip-10-0-10-11.ec2.internal
```

**02:14** — 检查训练代码：
```python
# 训练脚本片段（来自 Git）
for epoch in range(num_epochs):
    for batch in dataloader:
        optimizer.zero_grad()
        loss = model(batch)
        loss.backward()
        optimizer.step()
    # 缺少 torch.cuda.empty_cache()
```

训练脚本未在 epoch 结束时释放显存，且 Pod 的 `restartPolicy: Never`，Completed 后未清理 GPU 上下文。

**02:16** — 根本原因确认：
1. PyTorch 训练脚本缺少显存清理（`torch.cuda.empty_cache()`）
2. 训练完成后 Pod 进入 `Completed` 状态，但 CUDA context 未被销毁
3. 显存持续占用，后续训练任务无法分配显存
4. 3 个 GPU 节点全部显存耗尽，训练队列积压

## 修复动作

**02:18** — 强制删除 Completed Pod 以释放显存：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
kubectl delete pods -n ml-training --field-selector status.phase=Succeeded
# Pod 删除后，kubelet 清理容器，显存释放
```

**02:22** — 验证显存释放：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl exec`：进入容器执行命令，可能改变容器状态

```bash
kubectl exec -n ml-training nvidia-device-plugin-xxx -- nvidia-smi
# |   0  NVIDIA A100-SXM4-40GB            ... |    512MiB / 40960MiB |      0%      Default |
# |   1  NVIDIA A100-SXM4-40GB            ... |    512MiB / 40960MiB |      0%      Default |
```

**02:25** — 重新提交训练任务：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源

```bash
kubectl apply -f training-jobs/resume-20260901.yaml
kubectl get pods -n ml-training
# NAME              READY   STATUS
# train-resnet-0    1/1     Running
# train-bert-1      1/1     Running
```

**02:30** — 修复训练脚本，添加显存清理：
```python
# 修复后的训练脚本
for epoch in range(num_epochs):
    for batch in dataloader:
        optimizer.zero_grad()
        loss = model(batch)
        loss.backward()
        optimizer.step()
    torch.cuda.empty_cache()  # 新增

# 训练结束后显式销毁模型和优化器
del model
del optimizer
torch.cuda.empty_cache()
```

**02:35** — 更新 Job 的 `ttlSecondsAfterFinished`：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl edit/patch`：修改运行中的资源

```bash
kubectl patch cronjob training-batch -n ml-training --type='merge' -p '
{
  "spec": {
    "jobTemplate": {
      "spec": {
        "ttlSecondsAfterFinished": 300
      }
    }
  }
}'
```

## 验证
- 02:38 — 训练任务正常运行，GPU 利用率为 95%+
- 02:40 — 模型训练进度正常，无显存泄漏
- 02:42 — ML 平台任务队列清空

## 复盘
- **直接原因**: PyTorch 训练脚本未释放显存 → Completed Pod 残留 CUDA context → GPU 显存耗尽 → 新任务无法调度
- **根本原因**: 
  1. 训练代码缺少 `torch.cuda.empty_cache()`
  2. 未设置 `ttlSecondsAfterFinished`，Completed Pod 长期占用显存
- **改进措施**:
  1. 所有 GPU 训练代码必须包含显存清理逻辑，Code Review 强制检查
  2. Job 配置 `ttlSecondsAfterFinished: 300`（训练完成后 5 分钟自动删除 Pod）
  3. 添加 GPU 显存监控：`nvidia_gpu_memory_used_bytes / nvidia_gpu_memory_total_bytes > 0.95` 触发告警
  4. 部署 GPU 显存清理 DaemonSet：定期检测无进程但显存占用的 GPU，执行 `nvidia-smi --gpu-reset`
  5. 训练镜像使用 `NVIDIA_VISIBLE_DEVICES` 限制设备可见性，训练结束后自动清理
- **相关 Skill**: [[ts-ai-ml-workloads]]
- **相关 FTA**: [[gpu-fta]]

```