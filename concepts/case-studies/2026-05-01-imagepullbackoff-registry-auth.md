---
title: "[2026-05-01] [P1] ImagePullBackOff 由于镜像仓库认证失败"
category: case-study
tags: [production, incident, workloads, image, registry, secret]
date: "2026-05-01"
severity: P1
mttr: "15min"
status: resolved
created: "2026-05-23"
updated: "2026-05-23"
last_updated: 2026-05-23
---

# [2026-05-01] 镜像仓库认证 Secret 过期导致全量 Pod ImagePullBackOff

## 工单信息
- **工单编号**: INC-2026-0501-010
- **发现时间**: 2026-05-01 09:00 UTC
- **恢复时间**: 2026-05-01 09:15 UTC
- **影响范围**: 所有使用私有镜像仓库的 namespace（8 个），156 个 Pod
- **业务影响**: 新 Pod 无法创建，滚动更新全部卡住，CI/CD 部署流水线失败

## 问题现象
09:00，CI/CD 流水线大量失败，ArgoCD 显示大量 Pod 状态为 `ImagePullBackOff`：
```bash
kubectl get pods -A | grep ImagePullBackOff | wc -l
# 156
```

Dev 团队反馈：任何新的部署都无法启动，已有 Deployment 的滚动更新也卡住。

## 诊断过程

**09:02** — 查看一个问题 Pod：
```bash
kubectl describe pod order-api-7d9f4b8c5a-abc12 -n prod-order
# Events:
#   Warning  Failed     5m    kubelet  
#     Failed to pull image "registry.example.com/order-api:v2.5.1": 
#     rpc error: code = Unknown desc = failed to pull and unpack image 
#     "registry.example.com/order-api:v2.5.1": failed to resolve reference 
#     "registry.example.com/order-api:v2.5.1": pull access denied, 
#     repository does not exist or may require authorization: 
#     authorization failed: no basic auth credentials
```

**09:04** — 检查 imagePullSecrets：
```bash
kubectl get sa default -n prod-order -o yaml | grep -A5 imagePullSecrets
# imagePullSecrets:
# - name: regcred

kubectl get secret regcred -n prod-order -o json | jq '.data.".dockerconfigjson"' | base64 -d | jq .
# {
#   "auths": {
#     "registry.example.com": {
#       "auth": "...",
#       "email": "k8s-pull@example.com"
#     }
#   }
# }
```

**09:06** — 验证认证信息：
```bash
# 解码 auth 字段
echo "..." | base64 -d
# k8s-pull:expired_token_xxx

# 测试直接拉取
docker login registry.example.com -u k8s-pull -p expired_token_xxx
# Error response from daemon: Get "https://registry.example.com/v2/": 
#   unauthorized: authentication required
```

**09:08** — 检查仓库 token 有效期：
```bash
# Harbor 管理员后台显示：
# 机器人账户 k8s-pull 的 token 创建于 2025-05-01，有效期 1 年
# 已于 2026-05-01 00:00 UTC 过期
```

## 根因
Harbor 机器人账户 `k8s-pull` 的 token 于 2026-05-01 00:00 UTC 过期。所有 namespace 的 `regcred` Secret 均使用该 token。token 过期后，kubelet 无法从私有仓库拉取镜像，所有新创建的 Pod 进入 `ImagePullBackOff`。

## 修复动作

**09:10** — 在 Harbor 生成新 token：
```bash
# Harbor UI → 系统管理 → 机器人账户 → k8s-pull → 重置密钥
# 新 token: new_token_abc123
```

**09:11** — 更新所有 namespace 的 regcred Secret：

> ⚠️ **🟡 中危变更** — 变更集群资源状态，建议先 --dry-run 或 diff 确认
> - `kubectl apply/create/replace`：创建/变更集群资源
> - `kubectl delete`：删除资源（可由声明式清单重建）

```bash
# 创建新的 dockerconfigjson
kubectl create secret docker-registry regcred \
  --docker-server=registry.example.com \
  --docker-username=k8s-pull \
  --docker-password=new_token_abc123 \
  --docker-email=k8s-pull@example.com \
  --dry-run=client -o yaml | base64 -w0 > /tmp/new_secret.yaml

# 批量更新（使用 kubed 或脚本）
for ns in $(kubectl get ns -o jsonpath='{.items[*].metadata.name}' | tr ' ' '\n' | grep -v kube-); do
  kubectl delete secret regcred -n $ns --ignore-not-found
  kubectl create secret docker-registry regcred \
    --docker-server=registry.example.com \
    --docker-username=k8s-pull \
    --docker-password=new_token_abc123 \
    --docker-email=k8s-pull@example.com \
    -n $ns
done
```

**09:13** — 验证拉取：
```bash
kubectl get pods -A | grep ImagePullBackOff | wc -l
# 0

kubectl get pods -n prod-order -l app=order-api
# NAME                          READY   STATUS    RESTARTS   AGE
# order-api-7d9f4b8c5a-abc12   1/1     Running   0          2m
```

## 验证
- 09:14 — 所有 ImagePullBackOff Pod 恢复 Running
- 09:15 — CI/CD 流水线恢复正常，新部署成功

## 复盘
- **直接原因**: Harbor 机器人账户 token 过期 → kubelet 无法认证 → ImagePullBackOff
- **根本原因**: 缺少镜像仓库 token 过期监控和自动轮换机制
- **改进措施**:
  1. 部署 token 过期监控：`harbor_robot_token_expiry_days < 30` 触发 P1 告警
  2. 使用 External Secrets Operator 自动同步 Harbor token 到 K8s Secret
  3. Harbor 机器人账户 token 有效期设为 90 天，配合自动轮换脚本
  4. 所有私有镜像仓库认证统一通过 ServiceAccount 的 imagePullSecrets 管理，禁止 Pod 级别硬编码
- **相关 Skill**: [[k8s-pod-security-guide]]
- **相关 FTA**: [[pod-fta]]
