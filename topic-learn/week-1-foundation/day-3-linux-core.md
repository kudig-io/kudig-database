# Day 3: Linux 核心基础

> **学习时间**: 4-5 小时 | **主题**: Linux 系统架构与进程管理

---

## 今日目标

- [ ] 理解 Linux 系统架构 (内核、系统调用)
- [ ] 掌握进程管理 (ps、top、kill)
- [ ] 深入理解 namespace 和 cgroup (容器隔离基础)

---

## 理论学习 (2h)

### 必读文档

1. **Linux 系统架构**
   - 文件: `../../domain-14-linux/01-linux-system-architecture.md`
   - 重点: 内核空间/用户空间、系统调用、cgroups/namespace 与 K8s 的关系

2. **进程管理**
   - 文件: `../../domain-14-linux/02-linux-process-management.md`
   - 重点: 进程树、信号机制、僵尸进程 (排障必备)

### 补充阅读

3. **容器基础原理**
   - 文件: `../../domain-14-linux/08-linux-container-fundamentals.md`
   - 重点: namespace 类型、cgroup 资源控制

---

## 实践任务 (2.5h)

### 任务 1: 进程管理命令练习 (45min)

```bash
# 查看进程树
pstree -p

# ps 命令详解
ps aux                    # 所有进程
ps -ef                    # 全格式
ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%cpu | head -10  # 按 CPU 排序

# top 实时监控
top -c                    # 显示完整命令
# 在 top 中按:
# P - 按 CPU 排序
# M - 按内存排序
# k - 杀死进程
# q - 退出

# htop (如果已安装，更友好的界面)
htop

# 进程信号
kill -l                   # 列出所有信号
kill -15 <PID>            # 优雅终止 (SIGTERM)
kill -9 <PID>             # 强制终止 (SIGKILL)
kill -HUP <PID>           # 重新加载配置

# 后台进程管理
sleep 100 &               # 后台运行
jobs                      # 查看后台任务
fg %1                     # 切换到前台
bg %1                     # 切换到后台
```

### 任务 2: 系统资源监控 (45min)

```bash
# 内存信息
free -h
cat /proc/meminfo | head -20

# CPU 信息
lscpu
cat /proc/cpuinfo | grep "model name" | head -1

# 磁盘信息
df -h
du -sh /var/log

# 系统负载
uptime
cat /proc/loadavg

# 网络连接
ss -tuln                  # 监听端口
ss -tunp                  # 连接及进程
netstat -anp | head -20   # 传统命令

# 打开文件
lsof -i :80               # 查看占用 80 端口的进程
lsof -p <PID>             # 查看进程打开的文件
```

### 任务 3: Namespace 实验 (30min)

```bash
# 查看当前进程的 namespace
ls -la /proc/$$/ns/

# 创建新的网络 namespace
sudo ip netns add test-ns
sudo ip netns list

# 在新 namespace 中执行命令
sudo ip netns exec test-ns ip addr

# 查看 Docker 容器的 namespace
docker run -d --name test-ns-container alpine sleep 3600
CONTAINER_PID=$(docker inspect -f '{{.State.Pid}}' test-ns-container)
sudo ls -la /proc/$CONTAINER_PID/ns/

# 清理
sudo ip netns delete test-ns
docker rm -f test-ns-container
```

### 任务 4: Cgroup 实验 (30min)

```bash
# 查看 cgroup 挂载点
mount | grep cgroup

# 查看 cgroup 子系统
cat /proc/cgroups

# 查看 Docker 容器的 cgroup
docker run -d --name cg-test --memory=100m --cpus=0.5 alpine sleep 3600
CONTAINER_ID=$(docker ps -q -f name=cg-test)

# 查看内存限制 (cgroup v2)
cat /sys/fs/cgroup/docker/$CONTAINER_ID/memory.max 2>/dev/null || \
cat /sys/fs/cgroup/memory/docker/$CONTAINER_ID/memory.limit_in_bytes

# 查看 CPU 限制
cat /sys/fs/cgroup/docker/$CONTAINER_ID/cpu.max 2>/dev/null || \
cat /sys/fs/cgroup/cpu/docker/$CONTAINER_ID/cpu.cfs_quota_us

# 清理
docker rm -f cg-test
```

### 任务 5: 排障命令练习 (30min)

参考 `../../domain-14-linux/99-linux-commands-reference.md`:

```bash
# strace - 跟踪系统调用
strace -p <PID> -e trace=open,read,write

# lsof - 查看打开的文件
lsof -p <PID>
lsof +D /var/log          # 查看目录被哪些进程打开

# 查找高 CPU 进程
ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%cpu | head -5

# 查找高内存进程
ps -eo pid,ppid,cmd,%mem,%cpu --sort=-%mem | head -5

# 查看进程限制
cat /proc/<PID>/limits

# 查看进程的文件描述符
ls -la /proc/<PID>/fd/
```

---

## 费曼复述 (0.5h)

用自己的语言回答:

1. **Linux namespace 有哪几种类型？各自隔离什么资源？**
   - 提示: pid, net, mnt, uts, ipc, user, cgroup

2. **cgroup 可以限制哪些资源？在 K8s 中如何体现？**
   - 提示: CPU、内存、IO；Pod 的 resources.limits

3. **什么是僵尸进程？如何产生？如何处理？**
   - 提示: 子进程退出但父进程未 wait

---

## 今日检验

- [ ] 能够使用 ps/top 找出高资源占用的进程
- [ ] 能够解释 namespace 和 cgroup 的作用
- [ ] 能够使用 lsof/ss 排查端口占用问题
- [ ] 理解容器是如何通过 namespace+cgroup 实现隔离的

---

## 重要概念回顾

| 概念 | 说明 | K8s 关联 |
|------|------|----------|
| namespace | 资源隔离 | Pod 网络/PID 隔离 |
| cgroup | 资源限制 | resources.limits/requests |
| 进程信号 | 进程通信 | Pod 终止流程 (SIGTERM -> SIGKILL) |

---

## 明日预告

Day 4 将学习 Linux 网络配置和性能调优，这是理解 K8s 网络的基础。
