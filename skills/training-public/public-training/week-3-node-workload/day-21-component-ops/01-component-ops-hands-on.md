---
title: 'Day 21: K8s 组件运维实操'
description: 'title: Day 21: K8s 组件运维实操'
category: learning
tags:
- k8s
- training
- hands-on
- etcd
- apiserver
- kubelet
- scheduler
- controller-manager
- prometheus
- containerd
last_updated: 2026-05
difficulty: beginner
reading_level: beginner
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- 'Day 21: K8s 组件运维实操 是什么'
- '如何 Day 21: K8s 组件运维实操'
trigger_keywords:
- Day
- '21:'
- K8s
- 组件运维实操
- learn
prerequisites:
- kubectl-basics
- gpu-ml-basics
- prometheus-basics
- etcd-basics
created: "2026-05-23"
---

---
title: Day 21: K8s 组件运维实操
last_updated: 2026-05-18
difficulty: advanced
intent_queries:
  - [[entities/kubernetes|[[Kubernetes|kubernetes]]]] 控制平面组件运维
  - API Server 故障排查
  - [[etcd|etcd]] 备份恢复
  - 证书管理与续期
trigger_keywords:
  - 控制平面
  - API Server
  - Scheduler
  - Controller Manager
  - etcd
  - [[kubelet|kubelet]]
  - 证书
  - 组件运维
reading_level: advanced
audience:
  - sre-engineer
  - ops-engineer
  - platform-engineer
estimated_read_time: 90min
related_domains:
  - domain-07-platform-engineering
  - domain-10-troubleshooting-diagnostics
related_topics:
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-15-node-basics/01-node-basics-hands-on
  - domain-11-production-operations/topic-learn/public-training/week-3-node-workload/day-16-node-advanced/01-node-advanced-hands-on
---

# Day 21: K8s 组件运维实操

> **日期**: Week 3 Day 7 | **主题**: 核心组件状态检查与故障处理 | **版本**: K8s 1.28-1.33

---

## 1. 控制平面组件概述

### 1.1 组件清单

| 组件 | 进程名 | 默认端口 | 关键文件 |
|------|--------|---------|---------|
| kube-apiserver | kube-apiserver | 6443 | /etc/kubernetes/manifests/kube-apiserver.yaml |
| kube-scheduler | kube-scheduler | 10259 | /etc/kubernetes/manifests/kube-scheduler.yaml |
| kube-controller-manager | kube-controller-manager | 10257 | /etc/kubernetes/manifests/kube-controller-manager.yaml |
| etcd | etcd | 2379/2380 | /var/lib/etcd |

### 1.2 组件健康检查

```bash
# 一键检查所有控制平面组件
for component in kube-apiserver kube-scheduler kube-controller-manager etcd; do
  echo "=== 检查 $component ==="
  kubectl get pods -n kube-system -l component=$component
done

# 检查组件日志
kubectl logs -n kube-system -l component=kube-apiserver --tail=20
kubectl logs -n kube-system -l component=kube-scheduler --tail=20
kubectl logs -n kube-system -l component=kube-controller-manager --tail=20
```

---

## 2. API Server 运维

### 2.1 状态检查

```bash
# 检查 API Server Pod
kubectl get pods -n kube-system -l component=kube-apiserver -o wide

# 检查 API Server 健康
curl -sk https://localhost:6443/healthz
curl -sk https://localhost:6443/healthz?verbose

# 检查 API Server 端口监听
ss -tlnp | grep 6443

# 检查 API Server 日志
kubectl logs -n kube-system kube-apiserver-<node-name> --tail=50 -f
```

### 2.2 API Server 故障排查

```bash
# 1. API Server 无法启动
# 检查静态 Pod manifest
ls -la /etc/kubernetes/manifests/
cat /etc/kubernetes/manifests/kube-apiserver.yaml | grep -E "image|port|host"

# 2. etcd 连接问题
curl -sk https://localhost:2379/health

# 检查 etcd 成员
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

# 3. 认证问题
kubectl auth whoami
openssl x509 -in /etc/kubernetes/pki/apiserver.crt -noout -dates
```

### 2.3 API Server 调优

```bash
# 增加 API 请求并发限制（/etc/kubernetes/manifests/kube-apiserver.yaml）
--max-mutating-requests-inflight=1000
--max-requests-inflight=2000
--target-ram-mb=1024
```

---

## 3. Scheduler 运维

### 3.1 状态检查

```bash
# 检查 Scheduler Pod
kubectl get pods -n kube-system -l component=kube-scheduler

# 查看 Scheduler 日志
kubectl logs -n kube-system kube-scheduler-<pod> --tail=50

# 测试调度
kubectl create -f test-pod.yaml --dry-run=client -o yaml | kubectl apply -f -
```

### 3.2 调度延迟排查

```bash
# 1. 确认调度器运行
kubectl get configmap -n kube-system kube-scheduler-configuration

# 2. 查看调度延迟指标
# (通过 Prometheus) scheduler scheduling duration P99

# 3. 查看调度失败事件
kubectl get events --sort-by='.lastTimestamp' | grep -i "schedul"

# 4. 测试调度算法
kubectl debug node/<node-name> -it --image=busybox -- ctr run
```

---

## 4. Controller Manager 运维

### 4.1 状态检查

```bash
# 检查 Controller Manager Pod
kubectl get pods -n kube-system -l component=kube-controller-manager

# 查看 Controller Manager 日志
kubectl logs -n kube-system kube-controller-manager-<pod> --tail=50

# 查看控制器循环延迟
# (通过 Prometheus) namespace_sync_duration etc
```

### 4.2 控制器故障排查

```bash
# 1. Deployment 控制器问题
kubectl get events --sort-by='.lastTimestamp' | grep -i "deployment"

# 2. ReplicaSet 控制器问题
kubectl get rs -A
kubectl describe rs <rs-name>

# 3. Node 控制器问题
kubectl get nodes -o wide
kubectl describe node <node-name> | grep -A20 "Conditions"

# 4. 控制器 Leader 选举
kubectl get endpoints kube-controller-manager -n kube-system -o yaml | grep -i leader
```

---

## 5. Kubelet 运维

### 5.1 节点级别组件检查

```bash
# SSH 到节点
ssh <node-ip>

# 检查 kubelet 状态
sudo systemctl status kubelet

# 查看 kubelet 日志
sudo journalctl -u kubelet --since "30m" | tail -100

# 检查 container runtime
sudo systemctl status docker    # Docker
sudo systemctl status containerd  # containerd

# 检查 kubelet 连接 API Server
sudo crictl info
```

### 5.2 Kubelet 故障排查

```bash
# 1. 节点 NotReady
sudo journalctl -u kubelet --since "10m" | grep -E "error|failed|refused"

# 2. Pod 无法创建
sudo crictl ps -a | grep -v "Running"

# 3. 镜像拉取失败
sudo crictl images
sudo crictl pull <image>

# 4. 磁盘压力
df -h /var/lib/kubelet
sudo docker system df

# 5. 重启 kubelet
sudo systemctl restart kubelet
sudo systemctl status kubelet
```

---

## 6. etcd 运维

### 6.1 状态检查

```bash
# 检查 etcd Pod
kubectl get pods -n kube-system -l component=etcd -o wide

# 检查 etcd 健康
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

# 检查 etcd leader
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint status
```

### 6.2 etcd 故障排查

```bash
# 1. etcd 无法启动
sudo systemctl status etcd
sudo journalctl -u etcd --since "10m" | tail -50

# 2. 磁盘空间不足
du -sh /var/lib/etcd/
ETCDCTL_API=3 etcdctl defrag --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key

# 3. Leader 选举问题
ping -c 10 <other-etcd-ip>

# 4. 备份与恢复
ETCDCTL_API=3 etcdctl snapshot save /backup/etcd.db \
  --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key
```

---

## 7. 组件证书管理

### 7.1 证书检查与续期

```bash
# 检查证书过期时间
for cert in /etc/kubernetes/pki/apiserver.crt /etc/kubernetes/pki/etcd/server.crt; do
  echo "=== $cert ==="
  openssl x509 -in $cert -noout -dates
done

# 检查 kubelet 证书
openssl x509 -in /var/lib/kubelet/pki/kubelet.crt -noout -dates

# 续期 kubeadm 证书（控制平面节点）
sudo kubeadm certs renew apiserver
sudo kubeadm certs renew apiserver-kubelet-client
sudo kubeadm certs renew all

# 重启 API Server 加载新证书
sudo systemctl restart kube-apiserver
```

---

## 8. 综合故障处理 SOP

### 8.1 控制平面故障处理流程

```bash
# ========== 控制平面故障 SOP ==========

echo "[1] 检查所有控制平面 Pod"
kubectl get pods -n kube-system | grep -E "apiserver|scheduler|controller|etcd"

echo "[2] 检查 API Server 健康"
curl -sk https://localhost:6443/healthz

echo "[3] 检查 etcd 健康"
ETCDCTL_API=3 etcdctl --endpoints=https://127.0.0.1:2379 \
  --cacert=/etc/kubernetes/pki/etcd/ca.crt \
  --cert=/etc/kubernetes/pki/etcd/healthcheck-client.crt \
  --key=/etc/kubernetes/pki/etcd/healthcheck-client.key \
  endpoint health

echo "[4] 检查节点状态"
kubectl get nodes -o wide

echo "[5] 检查事件"
kubectl get events -A --sort-by='.lastTimestamp' | tail -50 | grep -i "error\|failed"

echo "[6] 查看组件日志"
kubectl logs -n kube-system -l component=kube-apiserver --tail=20

# ========== 升级控制平面 ==========
# 升级 kubeadm
sudo kubeadm upgrade plan
sudo kubeadm upgrade apply v1.30.0

# 升级节点组件
sudo apt-get update && sudo apt-get install -y kubeadm=1.30.0-*
sudo kubeadm upgrade node

# 升级 kubelet
sudo apt-get install -y kubelet=1.30.0-*
sudo systemctl restart kubelet
```

---

## 9. 实战练习

**练习 1**: 检查 API Server 健康状态，模拟 API Server 故障并恢复

**练习 2**: 使用 `kubeadm certs renew` 续期所有证书

**练习 3**: etcd 备份与恢复操作

**练习 4**: 模拟节点 NotReady，排查 kubelet 故障

---

