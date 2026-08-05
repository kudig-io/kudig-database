---
title: 03 - 运行时安全防护与威胁检测
description: 'title: 运行时安全防护与威胁检测'
summary: 'title: 运行时安全防护与威胁检测'
category: general
tags:
- k8s
- scheduler
- prometheus
- containerd
- cri-o
- harbor
- falco
- hpa
- daemonset
- rbac
tier: supporting
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 25min
intent_queries:
- 运行时安全防护与威胁检测 是什么
- 如何 运行时安全防护与威胁检测
- Kubernetes 05 security compliance 最佳实践
trigger_keywords:
- 运行时安全防护与威胁检测
- security
- compliance
prerequisites:
- kubectl-basics
- rbac-basics
- prometheus-basics
- ebpf-basics
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




---
title: 运行时安全防护与威胁检测
description: 深入解析 K8s 运行时安全：Seccomp、AppArmor、SELinux、[[falco|Falco]] 规则、容器逃逸检测、供应链安全与安全上下文配置
category: 安全
tags:
- k8s
- runtime
- security
- seccomp
- apparmor
- falco
- container
- security-context
- scheduler
- [[prometheus|prometheus]]
- rbac
- best-practice
last_updated: 2026-05
difficulty: advanced
reading_level: advanced
audience:
- 安全工程师
- SRE
- 架构师
estimated_read_time: 5min
intent_queries:
- 运行时安全防护与威胁检测 是什么
- 如何 运行时安全防护与威胁检测
- [[kubernetes|Kubernetes]] 7 security 最佳实践
trigger_keywords:
- 运行时安全防护与威胁检测
- security
k8s_versions:
- '1.25'
- '1.26'
- '1.27'
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
related_docs:
- path: 01-authentication-authorization-system.md
  type: depth
  desc: 认证授权体系
- path: 02-network-security-policies.md
  type: depth
  desc: 网络安全策略
- path: ../安全/
  type: depth
  desc: 云原生安全
cross_refs:
- type: domain
  path: ../集群基础/
  label: '相关知识域: 集群基础'
- type: domain
  path: ../可观测性/
  label: '相关知识域: 可观测性'
- type: cheatsheet
  path: ../系统基础/topic-cheat-sheet/tls-pki.md
  label: '速查卡: tls-pki'

tier: peripheral---


# 03 - 运行时安全防护与威胁检测

> **适用版本**: v1.25 - v1.32 | **最后更新**: 2026-02 | **参考**: [kubernetes.io/docs/concepts/security](https://kubernetes.io/docs/concepts/security/)

<!-- chunk: 运行时安全架构全景 -->
## 运行时安全架构全景

```
┌─────────────────────────────────────────────────────────────────────────────────────┐
│                        Kubernetes 运行时安全防护体系                               │
├─────────────────────────────────────────────────────────────────────────────────────┤
│                                                                                      │
│  ┌────────────────────────────────────────────────────────────────────────────────┐ │
│  │                      Container Runtime Security                                │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │   Seccomp    │  │  AppArmor    │  │   SELinux    │  │ Capabilities │       │ │
│  │  │ 系统调用过滤 │  │ 应用程序防护 │  │ 强制访问控制 │  │ 权限管理     │       │ │
│  │  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘  └──────┬───────┘       │ │
│  │         │                 │                 │                 │                │ │
│  │         └─────────────────┼─────────────────┼─────────────────┘                │ │
│  │                           │                 │                                  │ │
│  │                    ┌──────▼─────────────────▼──────┐                          │ │
│  │                    │     Security Context          │                          │ │
│  │                    │  容器安全上下文配置           │                          │ │
│  │                    └───────────────────────────────┘                          │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                      Runtime Threat Detection                                  │ │
│  │  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐  ┌──────────────┐       │ │
│  │  │    Falco     │  │  Tetragon   │  │  KubeArmor   │  │   Sysdig     │       │ │
│  │  │              │  │              │  │              │  │              │       │ │
│  │  │ • Syscall    │  │ • eBPF      │  │ • LSM/BPF   │  │ • 商业平台   │       │ │
│  │  │ • 规则引擎   │  │ • 网络追踪   │  │ • 策略防护   │  │ • 合规检测   │       │ │
│  │  │ • 实时告警   │  │ • 进程跟踪   │  │ • 告警阻断   │  │ • 取证分析   │       │ │
│  │  └──────────────┘  └──────────────┘  └──────────────┘  └──────────────┘       │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                    │                                                │
│  ┌─────────────────────────────────▼──────────────────────────────────────────────┐ │
│  │                    Advanced Runtime Protection                                │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                        Sandboxed Runtimes                                │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │  gVisor    │  │   Kata     │  │   Firecracker│  │   Nabla     │         │   │ │
│  │  │  │            │  │            │  │             │  │             │         │   │ │
│  │  │  │ • 用户态   │  │ • 虚拟机   │  │ • 微型VM   │  │ •unikernel  │         │   │ │
│  │  │  │ • 沙箱     │  │ • 硬件隔离 │  │ • 快速启动  │  │ • 应用隔离  │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  │  ┌─────────────────────────────────────────────────────────────────────────┐   │ │
│  │  │                      Image Security Scanning                            │   │ │
│  │  │  ┌────────────┐  ┌────────────┐  ┌────────────┐  ┌────────────┐         │   │ │
│  │  │  │   Trivy    │  │   Clair    │  │   Anchore  │  │   Snyk     │         │   │ │
│  │  │  │            │  │            │  │            │  │            │         │   │ │
│  │  │  │ • 多源CVE  │  │ • Harbor   │  │ • 企业级   │  │ • 开发者   │         │   │ │
│  │  │  │ • 配置检查 │  │ • 集成     │  │ • 合规     │  │ • 体验     │         │   │ │
│  │  │  └────────────┘  └────────────┘  └────────────┘  └────────────┘         │   │ │
│  │  └─────────────────────────────────────────────────────────────────────────┘   │ │
│  └────────────────────────────────────────────────────────────────────────────────┘ │
│                                                                                      │
└─────────────────────────────────────────────────────────────────────────────────────┘
```

<!-- chunk: 容器安全上下文配置 -->
## 容器安全上下文配置

### SecurityContext 核心配置项

| 配置项 | 说明 | 安全影响 | 推荐值 |
|-------|------|---------|-------|
| **runAsNonRoot** | 以非root用户运行 | 防止容器逃逸 | `true` |
| **runAsUser** | 指定运行用户ID | 用户隔离 | `1000` |
| **runAsGroup** | 指定运行组ID | 组隔离 | `1000` |
| **fsGroup** | 文件系统组ID | 文件访问控制 | `1000` |
| **readOnlyRootFilesystem** | 只读根文件系统 | 防止恶意文件写入 | `true` |
| **allowPrivilegeEscalation** | 允许权限提升 | 防止权限滥用 | `false` |
| **privileged** | 特权模式 | 完整主机访问 | `false` |

### 完整SecurityContext配置示例

```yaml
# 01-secure-pod.yaml
apiVersion: v1
kind: Pod
metadata:
  name: secure-app
  namespace: production
spec:
  securityContext:
    # Pod级别安全配置
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 1000
    fsGroup: 1000
    supplementalGroups: [2000]
    seccompProfile:
      type: RuntimeDefault
    seLinuxOptions:
      level: "s0:c123,c456"
    
  containers:
  - name: app
    image: myapp:v1.0
    securityContext:
      # 容器级别安全配置
      allowPrivilegeEscalation: false
      readOnlyRootFilesystem: true
      runAsNonRoot: true
      runAsUser: 1000
      runAsGroup: 1000
      
      # 能力管理
      capabilities:
        drop:  # 删除所有不必要的能力
        - ALL
        add:   # 仅添加必需的能力
        - NET_BIND_SERVICE  # 允许绑定低端口
        
      # Seccomp配置
      seccompProfile:
        type: Localhost
        localhostProfile: profiles/custom-profile.json
        
    # 只读挂载必要的卷
    volumeMounts:
    - name: app-data
      mountPath: /app/data
    - name: tmp-storage
      mountPath: /tmp
    - name: logs
      mountPath: /var/log
      
    resources:
      requests:
        cpu: "100m"
        memory: "128Mi"
      limits:
        cpu: "500m"
        memory: "256Mi"
        
  volumes:
  - name: app-data
    emptyDir: {}
  - name: tmp-storage
    emptyDir: {}
  - name: logs
    emptyDir: {}
```

<!-- chunk: Seccomp 配置详解 -->
## Seccomp 配置详解

### Seccomp Profile 配置

```json
{
  "defaultAction": "SCMP_ACT_ERRNO",
  "architectures": [
    "SCMP_ARCH_X86_64",
    "SCMP_ARCH_X86",
    "SCMP_ARCH_X32"
  ],
  "syscalls": [
    {
      "names": [
        "accept",
        "accept4",
        "access",
        "alarm",
        "bind",
        "brk",
        "capget",
        "capset",
        "chdir",
        "chmod",
        "chown",
        "chown32",
        "clock_getres",
        "clock_gettime",
        "clock_nanosleep",
        "close",
        "connect",
        "copy_file_range",
        "creat",
        "dup",
        "dup2",
        "dup3",
        "epoll_create",
        "epoll_create1",
        "epoll_ctl",
        "epoll_ctl_old",
        "epoll_pwait",
        "epoll_wait",
        "epoll_wait_old",
        "eventfd",
        "eventfd2",
        "execve",
        "execveat",
        "exit",
        "exit_group",
        "faccessat",
        "fadvise64",
        "fadvise64_64",
        "fallocate",
        "fanotify_mark",
        "fchdir",
        "fchmod",
        "fchmodat",
        "fchown",
        "fchown32",
        "fchownat",
        "fcntl",
        "fcntl64",
        "fdatasync",
        "fgetxattr",
        "flistxattr",
        "flock",
        "fork",
        "fremovexattr",
        "fsetxattr",
        "fstat",
        "fstat64",
        "fstatat64",
        "fstatfs",
        "fstatfs64",
        "fsync",
        "ftruncate",
        "ftruncate64",
        "futex",
        "futimesat",
        "getcpu",
        "getcwd",
        "getdents",
        "getdents64",
        "getegid",
        "getegid32",
        "geteuid",
        "geteuid32",
        "getgid",
        "getgid32",
        "getgroups",
        "getgroups32",
        "getitimer",
        "getpeername",
        "getpgid",
        "getpgrp",
        "getpid",
        "getppid",
        "getpriority",
        "getrandom",
        "getresgid",
        "getresgid32",
        "getresuid",
        "getresuid32",
        "getrlimit",
        "get_robust_list",
        "getrusage",
        "getsid",
        "getsockname",
        "getsockopt",
        "get_thread_area",
        "gettid",
        "gettimeofday",
        "getuid",
        "getuid32",
        "getxattr",
        "inotify_add_watch",
        "inotify_init",
        "inotify_init1",
        "inotify_rm_watch",
        "io_cancel",
        "ioctl",
        "io_destroy",
        "io_getevents",
        "ioprio_get",
        "ioprio_set",
        "io_setup",
        "io_submit",
        "ipc",
        "kill",
        "lchown",
        "lchown32",
        "lgetxattr",
        "link",
        "linkat",
        "listen",
        "listxattr",
        "llistxattr",
        "_llseek",
        "lremovexattr",
        "lseek",
        "lsetxattr",
        "lstat",
        "lstat64",
        "madvise",
        "memfd_create",
        "mincore",
        "mkdir",
        "mkdirat",
        "mknod",
        "mknodat",
        "mlock",
        "mlock2",
        "mlockall",
        "mmap",
        "mmap2",
        "mprotect",
        "mq_getsetattr",
        "mq_notify",
        "mq_open",
        "mq_timedreceive",
        "mq_timedsend",
        "mq_unlink",
        "mremap",
        "msgctl",
        "msgget",
        "msgrcv",
        "msgsnd",
        "msync",
        "munlock",
        "munlockall",
        "munmap",
        "nanosleep",
        "newfstatat",
        "_newselect",
        "open",
        "openat",
        "pause",
        "pipe",
        "pipe2",
        "poll",
        "ppoll",
        "prctl",
        "pread64",
        "preadv",
        "prlimit64",
        "pselect6",
        "pwrite64",
        "pwritev",
        "read",
        "readahead",
        "readlink",
        "readlinkat",
        "readv",
        "recv",
        "recvfrom",
        "recvmmsg",
        "recvmsg",
        "remap_file_pages",
        "removexattr",
        "rename",
        "renameat",
        "renameat2",
        "restart_syscall",
        "rmdir",
        "rt_sigaction",
        "rt_sigpending",
        "rt_sigprocmask",
        "rt_sigqueueinfo",
        "rt_sigreturn",
        "rt_sigsuspend",
        "rt_sigtimedwait",
        "rt_tgsigqueueinfo",
        "sched_getaffinity",
        "sched_getattr",
        "sched_getparam",
        "sched_get_priority_max",
        "sched_get_priority_min",
        "sched_getscheduler",
        "sched_rr_get_interval",
        "sched_setaffinity",
        "sched_setattr",
        "sched_setparam",
        "sched_setscheduler",
        "sched_yield",
        "seccomp",
        "select",
        "semctl",
        "semget",
        "semop",
        "semtimedop",
        "send",
        "sendfile",
        "sendfile64",
        "sendmmsg",
        "sendmsg",
        "sendto",
        "setfsgid",
        "setfsgid32",
        "setfsuid",
        "setfsuid32",
        "setgid",
        "setgid32",
        "setgroups",
        "setgroups32",
        "setitimer",
        "setpgid",
        "setpriority",
        "setregid",
        "setregid32",
        "setresgid",
        "setresgid32",
        "setresuid",
        "setresuid32",
        "setreuid",
        "setreuid32",
        "setrlimit",
        "set_robust_list",
        "setsid",
        "setsockopt",
        "set_thread_area",
        "set_tid_address",
        "setuid",
        "setuid32",
        "setxattr",
        "shmat",
        "shmctl",
        "shmdt",
        "shmget",
        "shutdown",
        "sigaltstack",
        "signalfd",
        "signalfd4",
        "sigreturn",
        "socket",
        "socketcall",
        "socketpair",
        "splice",
        "stat",
        "stat64",
        "statfs",
        "statfs64",
        "symlink",
        "symlinkat",
        "sync",
        "sync_file_range",
        "syncfs",
        "sysinfo",
        "tee",
        "tgkill",
        "time",
        "timer_create",
        "timer_delete",
        "timerfd_create",
        "timerfd_gettime",
        "timerfd_settime",
        "timer_getoverrun",
        "timer_gettime",
        "timer_settime",
        "times",
        "tkill",
        "truncate",
        "truncate64",
        "ugetrlimit",
        "umask",
        "uname",
        "unlink",
        "unlinkat",
        "utime",
        "utimensat",
        "utimes",
        "vfork",
        "vmsplice",
        "wait4",
        "waitid",
        "waitpid",
        "write",
        "writev"
      ],
      "action": "SCMP_ACT_ALLOW"
    }
  ]
}
```

<!-- chunk: Falco 运行时威胁检测 -->
## Falco 运行时威胁检测

### Falco 部署配置

```yaml
# 02-falco-deployment.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: falco
  namespace: falco
spec:
  selector:
    matchLabels:
      app: falco
  template:
    metadata:
      labels:
        app: falco
    spec:
      serviceAccountName: falco
      hostPID: true
      hostNetwork: true
      containers:
      - name: falco
        image: falcosecurity/falco-no-driver:0.37.0
        args:
        - /usr/bin/falco
        - --cri=/run/containerd/containerd.sock
        - --cri=/run/crio/crio.sock
        - -K=/var/run/secrets/kubernetes.io/serviceaccount/token
        - -k=https://kubernetes.default
        - --k8s-node=$(FALCO_K8S_NODE_NAME)
        - -pk
        
        env:
        - name: FALCO_K8S_NODE_NAME
          valueFrom:
            fieldRef:
              fieldPath: spec.nodeName
            
        securityContext:
          privileged: true
          
        volumeMounts:
        - name: dev
          mountPath: /host/dev
          readOnly: true
        - name: proc
          mountPath: /host/proc
          readOnly: true
        - name: boot
          mountPath: /host/boot
          readOnly: true
        - name: lib-modules
          mountPath: /host/lib/modules
          readOnly: true
        - name: usr-src
          mountPath: /host/usr
          readOnly: true
        - name: containerd-socket
          mountPath: /run/containerd/containerd.sock
          readOnly: true
        - name: crio-socket
          mountPath: /run/crio/crio.sock
          readOnly: true
        - name: falco-config
          mountPath: /etc/falco
          
      volumes:
      - name: dev
        hostPath:
          path: /dev
      - name: proc
        hostPath:
          path: /proc
      - name: boot
        hostPath:
          path: /boot
      - name: lib-modules
        hostPath:
          path: /lib/modules
      - name: usr-src
        hostPath:
          path: /usr
      - name: containerd-socket
        hostPath:
          path: /run/containerd/containerd.sock
      - name: crio-socket
        hostPath:
          path: /run/crio/crio.sock
      - name: falco-config
        configMap:
          name: falco-config
```

### Falco 规则配置

```yaml
# 03-falco-rules.yaml
apiVersion: v1
kind: ConfigMap
metadata:
  name: falco-rules
  namespace: falco
data:
  falco_rules.local.yaml: |
    - rule: Detect crypto miners
      desc: Detect crypto mining activity
      condition: spawned_process and proc.name in (xmrig, cgminer, bfgminer)
      output: Crypto miner detected (user=%user.name command=%proc.cmdline)
      priority: CRITICAL
      tags: [process, mitre_execution]

    - rule: Detect privilege escalation
      desc: Detect attempts to escalate privileges
      condition: >
        spawned_process and proc.pname in (sudo, su) and not proc.name in (sudo, su)
      output: Privilege escalation attempt (user=%user.name parent=%proc.pname command=%proc.cmdline)
      priority: WARNING
      tags: [privilege_escalation, mitre_privilege_escalation]

    - rule: Detect port scanning
      desc: Detect port scanning activity
      condition: >
        evt.type in (connect, accept) and fd.typechar = 4 and fd.l4proto = tcp and
        (evt.rawres >= 0 or evt.res = EINPROGRESS)
      output: Port scanning detected (connection=%fd.name user=%user.name command=%proc.cmdline)
      priority: WARNING
      tags: [network, reconnaissance]

    - rule: Detect suspicious file access
      desc: Detect access to sensitive files
      condition: >
        open_read and fd.name startswith /etc/passwd or fd.name startswith /etc/shadow
      output: Suspicious file access detected (file=%fd.name user=%user.name command=%proc.cmdline)
      priority: WARNING
      tags: [file, credential_access]

    - rule: Detect container escape attempts
      desc: Detect attempts to escape from container
      condition: >
        spawned_process and proc.name in (nsenter, chroot) and container
      output: Container escape attempt detected (command=%proc.cmdline container=%container.id)
      priority: CRITICAL
      tags: [container, mitre_escape]
```

<!-- chunk: KubeArmor 安全策略 -->
## KubeArmor 安全策略

### KubeArmor 部署

```yaml
# 04-kubearmor-deployment.yaml
apiVersion: apps/v1
kind: DaemonSet
metadata:
  name: kubearmor
  namespace: kubearmor
spec:
  selector:
    matchLabels:
      kubearmor-app: kubearmor
  template:
    metadata:
      labels:
        kubearmor-app: kubearmor
    spec:
      hostPID: true
      hostNetwork: true
      containers:
      - name: kubearmor
        image: kubearmor/kubearmor:stable
        args:
        - -enableKubeArmorHostPolicy
        - -enableKubeArmorPolicy
        securityContext:
          privileged: true
        volumeMounts:
        - name: bpf
          mountPath: /sys/fs/bpf
        - name: containerd
          mountPath: /run/containerd/containerd.sock
        - name: cri-o
          mountPath: /run/crio/crio.sock
      volumes:
      - name: bpf
        hostPath:
          path: /sys/fs/bpf
      - name: containerd
        hostPath:
          path: /run/containerd/containerd.sock
      - name: cri-o
        hostPath:
          path: /run/crio/crio.sock
```

### KubeArmor 安全策略

```yaml
# 05-kubearmor-policy.yaml
apiVersion: security.kubearmor.com/v1
kind: KubeArmorPolicy
metadata:
  name: app-security-policy
  namespace: production
spec:
  selector:
    matchLabels:
      app: web
      
  file:
    matchDirectories:
    - dir: /app/
      recursive: true
      readOnly: true  # 应用目录只读
      
    matchPaths:
    - path: /tmp/
      readOnly: false  # 允许写入临时目录
      
  process:
    matchPaths:
    - path: /app/application
      ownerOnly: true  # 仅允许所有者执行
      
  network:
    matchProtocols:
    - protocol: TCP
      localPortStart: 8080
      localPortEnd: 8080  # 仅允许监听8080端口
      
  capabilities:
    matchCapabilities:
    - capability: NET_BIND_SERVICE  # 仅允许绑定服务端口能力
    
  action:
    Allow  # 默认允许，显式拒绝
```

<!-- chunk: 安全沙箱运行时 -->
## 安全沙箱运行时

### gVisor 配置

```yaml
# 06-gvisor-runtime.yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc

---
apiVersion: v1
kind: Pod
metadata:
  name: sandboxed-app
spec:
  runtimeClassName: gvisor
  containers:
  - name: app
    image: myapp:v1.0
    securityContext:
      runAsNonRoot: true
      runAsUser: 1000
```

### Kata Containers 配置

```yaml
# 07-kata-runtime.yaml
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: kata
handler: kata

---
apiVersion: v1
kind: Pod
metadata:
  name: kata-secured-app
spec:
  runtimeClassName: kata
  containers:
  - name: app
    image: myapp:v1.0
    resources:
      limits:
        kata.peerpods.io/vm: "1"  # 分配一个VM
```

<!-- chunk: 运行时安全监控告警 -->
## 运行时安全监控告警

### Prometheus 告警规则

```yaml
# 08-runtime-security-alerts.yaml
apiVersion: monitoring.coreos.com/v1
kind: PrometheusRule
metadata:
  name: runtime-security-alerts
  namespace: monitoring
spec:
  groups:
  - name: runtime-security
    rules:
    # Falco告警
    - alert: FalcoCriticalAlert
      expr: |
        sum(rate(falco_events_total{priority="Critical"}[5m])) > 0
      for: 1m
      labels:
        severity: critical
      annotations:
        summary: "Falco检测到严重安全威胁"
        
    # KubeArmor违规
    - alert: KubeArmorPolicyViolation
      expr: |
        sum(rate(kubearmor_alerts_total{action="Block"}[5m])) > 0
      for: 1m
      labels:
        severity: warning
      annotations:
        summary: "KubeArmor策略违规"
        
    # 容器逃逸尝试
    - alert: ContainerEscapeAttempt
      expr: |
        sum(rate(container_runtime_operations_total{operation="exec",container_state="running"}[5m])) > 10
      for: 5m
      labels:
        severity: critical
      annotations:
        summary: "检测到容器逃逸尝试"
        
    # 异常系统调用
    - alert: AbnormalSyscallActivity
      expr: |
        sum(rate(security_syscalls_total[5m])) by (syscall) > 1000
      for: 10m
      labels:
        severity: warning
      annotations:
        summary: "检测到异常系统调用活动 ({{ $labels.syscall }})"
```

<!-- chunk: 安全最佳实践检查清单 -->
## 安全最佳实践检查清单

| 检查项 | 命令/方法 | 安全要求 | 优先级 |
|-------|---------|---------|-------|
| 非root运行 | `kubectl get pods -n <ns> -o jsonpath='{.spec.securityContext.runAsNonRoot}'` | 所有容器必须为true | P0 |
| 只读文件系统 | 检查readOnlyRootFilesystem | 生产环境设为true | P0 |
| 权限提升禁用 | 检查allowPrivilegeEscalation | 必须为false | P0 |
| 特权容器禁用 | 检查privileged | 必须为false | P0 |
| 能力最小化 | 检查capabilities.drop | 必须drop ALL | P0 |
| Seccomp启用 | 检查seccompProfile | 使用RuntimeDefault | P1 |
| 运行时防护 | 部署Falco/KubeArmor | 实时威胁检测 | P0 |
| 沙箱运行时 | 配置gVisor/Kata | 敏感应用使用 | P1 |
| 安全策略 | 配置NetworkPolicy | 网络隔离 | P0 |

---
**运行时安全原则**: 深度防御 + 实时检测 + 最小权限 + 沙箱隔离
---
**表格底部标记**: Kusheet Project, 作者 Allen Galler (allengaller@gmail.com)

---

<!-- chunk: Obsidian 相关文档 -->
## Obsidian 相关文档

- 安全 MOC
- [[08-安全/README.md|Security Domain]]
- [[08-安全/00-总览/00-open-source-projects-index.md|Domain-7 安全 — 开源项目索引]]
- Kubernetes 认证授权体系详解
- 网络安全策略与零信任架构
- 04 - 审计日志与合规性管理
- 05 - 策略校验与准入控制工具 (Policy Validation)
- 06 - Pod安全标准详解
- 07 - RBAC权限矩阵表
- 08 - 安全最佳实践表
- Kubernetes 安全加固
- 证书管理与 TLS 配置

## Related

- 认证授权体系
- 网络安全策略
- 相关知识域: 集群基础
- 相关知识域: 可观测性
- [[17-系统基础/05-速查卡/tls-pki.md|速查卡: tls-pki]]

- [[08-安全/README.md|返回目录]]- [[21-生态参考/03-领域索引/security-index.md|Security 安全知识图谱索引]]

## See Also

- 01-authentication-authorization-system
- 02-network-security-policies
- 04-audit-logging-compliance
- 05-policy-validation-tools


<!-- risk-assessed -->
