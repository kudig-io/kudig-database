---
title: Agent沙箱与隔离
description: 'Agent执行沙箱架构：Docker容器隔离、gVisor系统调用拦截、Firecracker microVM、云端沙箱服务与K8s安全策略'
summary: 'Agent执行沙箱架构：Docker容器隔离、gVisor系统调用拦截、Firecracker microVM、云端沙箱服务与K8s安全策略'
category: ai-ml-infra
tags:
- ai
- agent
- runtime
- sandbox
- security
- isolation
tier: supporting
created: '2026-07-02'
last_updated: 2026-07
difficulty: advanced
reading_level: advanced
audience:
- AI 工程师
- 平台工程师
- 架构师
estimated_read_time: 20min
intent_queries:
- Agent沙箱隔离 是什么
- 如何为Agent构建安全沙箱
- gVisor Firecracker Agent隔离
trigger_keywords:
- agent-sandbox
- isolation
- gvisor
- firecracker
- security
prerequisites:
- llm-basics
- kubernetes-basics
k8s_versions:
- '1.28'
- '1.29'
- '1.30'
- '1.31'
- '1.32'
authors:
- name: KUDIG Team
  role: contributor
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。


# Agent沙箱与隔离

## 概述

AI Agent的安全隔离是生产部署的核心挑战。与传统应用不同，Agent通常需要执行LLM生成的代码、调用外部API、访问文件系统，这些操作都带来了显著的安全风险。一个失控的Agent可能导致数据泄露、资源滥用甚至系统入侵。

本文档系统介绍Agent沙箱的多种实现方案：从轻量级的Docker容器隔离到强隔离的Firecracker microVM，以及E2B、Modal等云端沙箱服务，并提供K8s环境下的安全策略配置。

```
隔离级别对比:

级别          技术              隔离强度    启动时间    资源开销
────────────────────────────────────────────────────────────
进程级        Docker容器         中          ~100ms      低
系统调用级    gVisor             中高        ~200ms      中
硬件级        Firecracker microVM 高         ~125ms      中高
云端          E2B/Modal          高          ~500ms      按需
```

## Docker容器沙箱

### 基础容器配置

```dockerfile
# Agent沙箱基础镜像
FROM python:3.11-slim AS base

# 创建非root用户
RUN groupadd -r agent && useradd -r -g agent -d /home/agent agent

# 安装最小依赖
RUN apt-get update && apt-get install -y --no-install-recommends \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

# 设置工作目录
WORKDIR /workspace

# 切换到非root用户
USER agent

# 入口点
ENTRYPOINT ["python", "-m", "agent_executor"]
```

### 资源限制

```yaml
# K8s Pod资源配置
apiVersion: v1
kind: Pod
metadata:
  name: agent-sandbox
  labels:
    app: agent-executor
spec:
  securityContext:
    runAsNonRoot: true
    runAsUser: 1000
    runAsGroup: 1000
    fsGroup: 1000
    seccompProfile:
      type: RuntimeDefault

  containers:
    - name: agent
      image: registry.example.com/agent-sandbox:latest
      resources:
        requests:
          cpu: "250m"
          memory: "256Mi"
          ephemeral-storage: "1Gi"
        limits:
          cpu: "1"
          memory: "512Mi"
          ephemeral-storage: "5Gi"

      securityContext:
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        capabilities:
          drop:
            - ALL

      volumeMounts:
        - name: workspace
          mountPath: /workspace
        - name: tmp
          mountPath: /tmp

  volumes:
    - name: workspace
      emptyDir:
        sizeLimit: 1Gi
    - name: tmp
      emptyDir:
        medium: Memory
        sizeLimit: 100Mi
```

### seccomp配置

```json
{
  "defaultAction": "SCMP_ACT_ERRNO",
  "architectures": ["SCMP_ARCH_X86_64"],
  "syscalls": [
    {
      "names": [
        "read", "write", "open", "close", "stat", "fstat",
        "lstat", "poll", "lseek", "mmap", "mprotect", "munmap",
        "brk", "ioctl", "access", "pipe", "select", "sched_yield",
        "mremap", "msync", "mincore", "madvise", "dup", "dup2",
        "nanosleep", "getpid", "clone", "fork", "vfork", "execve",
        "exit", "wait4", "kill", "uname", "fcntl", "flock",
        "fsync", "fdatasync", "truncate", "ftruncate", "getdents",
        "getcwd", "chdir", "rename", "mkdir", "rmdir", "link",
        "unlink", "symlink", "readlink", "chmod", "chown", "arch_prctl",
        "gettimeofday", "getuid", "getgid", "geteuid", "getegid",
        "getppid", "getpgrp", "setsid", "setuid", "setgid",
        "sigaltstack", "rt_sigaction", "rt_sigprocmask",
        "pread64", "pwrite64", "readv", "writev",
        "socket", "connect", "accept", "sendto", "recvfrom",
        "sendmsg", "recvmsg", "shutdown", "bind", "listen",
        "getsockname", "getpeername", "socketpair",
        "epoll_create", "epoll_ctl", "epoll_wait",
        "clock_gettime", "clock_getres", "exit_group",
        "futex", "set_robust_list", "get_robust_list",
        "epoll_create1", "pipe2", "dup3", "preadv", "pwritev",
        "recvmmsg", "sendmmsg", "getrandom", "memfd_create",
        "statx", "rseq", "clone3", "close_range",
        "epoll_pwait2", "faccessat2"
      ],
      "action": "SCMP_ACT_ALLOW"
    }
  ]
}
```

### AppArmor配置

```bash
# /etc/apparmor.d/agent-sandbox
#include <tunables/global>

profile agent-sandbox flags=(attach_disconnected,mediate_deleted) {
  #include <abstractions/base>
  #include <abstractions/python>
  #include <abstractions/openssl>

  # 允许读取系统库
  /usr/lib/** r,
  /lib/** r,

  # 工作目录读写
  /workspace/** rw,
  /tmp/** rw,

  # 禁止访问敏感目录
  deny /etc/shadow r,
  deny /etc/passwd w,
  deny /root/** rwx,
  deny /home/**/.* rwx,

  # 网络访问（限制出站）
  network inet stream,
  network inet dgram,
  deny network inet6,

  # 禁止挂载
  deny mount,
  deny umount,
  deny pivot_root,

  # 禁止加载内核模块
  deny /sbin/modprobe x,
  deny /sbin/insmod x,

  # 信号限制
  signal receive,
  signal send,
}
```

## gVisor沙箱

### gVisor原理

gVisor是一个用户空间内核，通过拦截系统调用提供强隔离：

```
传统容器:
  应用 → 系统调用 → 宿主机内核 → 硬件
  
gVisor:
  应用 → 系统调用 → Sentry(用户空间内核) → Gofer(文件代理) → 宿主机内核 → 硬件

gVisor核心组件:
  Sentry: 用户空间内核，实现Linux系统调用接口
  Gofer: 文件系统代理，限制主机文件访问
  Runsc: OCI兼容的容器运行时
```

### K8s集成gVisor

```yaml
# 安装gVisor RuntimeClass
apiVersion: node.k8s.io/v1
kind: RuntimeClass
metadata:
  name: gvisor
handler: runsc
scheduling:
  nodeSelector:
    node.kubernetes.io/gvisor: "true"
---
# 使用gVisor的Agent Pod
apiVersion: v1
kind: Pod
metadata:
  name: agent-gvisor-sandbox
spec:
  runtimeClassName: gvisor
  containers:
    - name: agent
      image: registry.example.com/agent-sandbox:latest
      resources:
        requests:
          cpu: "500m"
          memory: "512Mi"
        limits:
          cpu: "2"
          memory: "1Gi"
      securityContext:
        runAsNonRoot: true
        readOnlyRootFilesystem: true
```

### gVisor运行时配置

```json
// /etc/docker/daemon.json
{
  "runtimes": {
    "runsc": {
      "path": "/usr/local/bin/runsc",
      "runtimeArgs": [
        "--platform=systrap",
        "--network=sandbox",
        "--fsgofer-host-uds",
        "--overlay2=all:memory",
        "--file-access=exclusive",
        "--lisafs",
        "-fuse-overlayfs"
      ]
    }
  }
}
```

```yaml
# runsc配置文件
# /etc/runsc/config.toml
[runsc]
  # 网络隔离
  network = "sandbox"
  
  # 文件系统
  file-access = "exclusive"
  overlay2 = "all:memory"
  
  # 系统调用过滤
  platform = "systrap"
  
  # 内存限制
  total-memory-limit = "1Gi"
  
  # CPU限制
  cpu-rate-limit = 100000
  
  # 日志
  debug = false
  log = "/var/log/runsc/"
  log-packets = false
```

## Firecracker microVM

### Firecracker原理

Firecracker是AWS开发的轻量级虚拟机监视器，提供硬件级隔离：

```
Firecracker架构:

传统VM:
  应用 → Guest OS → Hypervisor(KVM) → 宿主机内核 → 硬件
  
Firecracker microVM:
  应用 → 精简Guest OS → Firecracker VMM → KVM → 宿主机内核 → 硬件

特点:
  - 启动时间: ~125ms
  - 内存开销: <5MB per microVM
  - 支持>4000个microVM/主机
  - 最小化攻击面（约50K行Rust代码）
```

### Firecracker Agent沙箱

```python
import firectl
from firectl import FirecrackerClient

class FirecrackerAgentSandbox:
    """基于Firecracker的Agent沙箱"""

    def __init__(self, socket_path: str):
        self.client = FirecrackerClient(socket_path)

    async def create_sandbox(
        self,
        agent_id: str,
        config: SandboxConfig,
    ) -> str:
        """创建Firecracker microVM沙箱"""
        # 配置VM
        vm_config = {
            "boot-source": {
                "kernel_image_path": config.kernel_path,
                "boot_args": "console=ttyS0 reboot=k panic=1 pci=off",
            },
            "drives": [
                {
                    "drive_id": "rootfs",
                    "path_on_host": config.rootfs_path,
                    "is_root_device": True,
                    "is_read_only": True,
                },
            ],
            "machine-config": {
                "vcpu_count": config.vcpus,
                "mem_size_mib": config.memory_mb,
                "smt": False,
            },
            "network-interfaces": [
                {
                    "iface_id": "eth0",
                    "guest_mac": self._generate_mac(),
                    "host_dev_name": f"tap-{agent_id[:8]}",
                },
            ],
        }

        # 启动microVM
        await self.client.create_vm(vm_config)

        # 配置cgroup限制
        await self._setup_cgroups(agent_id, config)

        return agent_id

    async def execute_in_sandbox(
        self,
        agent_id: str,
        command: str,
    ) -> ExecutionResult:
        """在microVM中执行命令"""
        # 通过API执行命令
        result = await self.client.api_put(
            f"/actions",
            {
                "action_type": "SendCtrlAltDel",
            },
        )
        return result

    async def _setup_cgroups(
        self,
        agent_id: str,
        config: SandboxConfig,
    ):
        """配置cgroup资源限制"""
        cgroup_path = f"/sys/fs/cgroup/firecracker/{agent_id}"

        # CPU限制
        with open(f"{cgroup_path}/cpu.max", "w") as f:
            f.write(f"{config.cpu_quota} {config.cpu_period}")

        # 内存限制
        with open(f"{cgroup_path}/memory.max", "w") as f:
            f.write(str(config.memory_limit_bytes))

        # I/O限制
        with open(f"{cgroup_path}/io.max", "w") as f:
            f.write(f"8:0 rbps={config.read_bps} wbps={config.write_bps}")
```

### Kata Containers (Firecracker集成)

```yaml
# K8s使用Kata Containers (Firecracker后端)
apiVersion: v1
kind: Pod
metadata:
  name: agent-kata-sandbox
spec:
  runtimeClassName: kata-fc
  containers:
    - name: agent
      image: registry.example.com/agent-sandbox:latest
      resources:
        requests:
          cpu: "500m"
          memory: "512Mi"
        limits:
          cpu: "2"
          memory: "2Gi"
      securityContext:
        privileged: false
        runAsNonRoot: true
```

## E2B云端沙箱

### E2B概述

E2B（Environment to Build）提供托管的云端代码执行沙箱：

```python
from e2b_code_interpreter import Sandbox

class E2BAgentSandbox:
    """基于E2B的Agent代码执行沙箱"""

    def __init__(self, api_key: str):
        self.api_key = api_key

    async def execute_code(
        self,
        code: str,
        language: str = "python",
    ) -> ExecutionResult:
        """在E2B沙箱中执行代码"""
        sandbox = Sandbox(api_key=self.api_key)

        try:
            # 执行代码
            execution = sandbox.run_code(code)

            return ExecutionResult(
                stdout=execution.logs.stdout,
                stderr=execution.logs.stderr,
                exit_code=execution.exit_code,
                artifacts=execution.results,
            )
        finally:
            sandbox.kill()

    async def execute_with_files(
        self,
        code: str,
        files: dict[str, bytes],
    ) -> ExecutionResult:
        """上传文件并在沙箱中执行"""
        sandbox = Sandbox(api_key=self.api_key)

        try:
            # 上传文件
            for filename, content in files.items():
                sandbox.files.write(filename, content)

            # 执行代码
            execution = sandbox.run_code(code)

            # 下载结果文件
            output_files = {}
            for path in sandbox.files.list("/workspace"):
                if path.endswith(".out") or path.endswith(".result"):
                    output_files[path] = sandbox.files.read(path)

            return ExecutionResult(
                stdout=execution.logs.stdout,
                stderr=execution.logs.stderr,
                exit_code=execution.exit_code,
                output_files=output_files,
            )
        finally:
            sandbox.kill()
```

### E2B自定义模板

```dockerfile
# E2B自定义沙箱模板
# e2b.Dockerfile
FROM e2bdev/code-interpreter:latest

# 安装额外依赖
RUN pip install pandas numpy matplotlib scikit-learn

# 安装Node.js
RUN apt-get update && apt-get install -y nodejs npm

# 复制自定义工具
COPY tools/ /usr/local/bin/tools/

# 配置环境
ENV PYTHONUNBUFFERED=1
ENV E2B_TEMPLATE_ID="custom-agent-sandbox"
```

## Modal无服务器沙箱

### Modal概述

Modal提供无服务器的代码执行环境，支持GPU加速：

```python
import modal

app = modal.App("agent-sandbox")

# 定义沙箱镜像
sandbox_image = (
    modal.Image.debian_slim(python_version="3.11")
    .pip_install("pandas", "numpy", "scikit-learn")
    .apt_install("git")
)

@app.function(
    image=sandbox_image,
    timeout=300,
    cpu=2,
    memory=1024,
    # GPU支持
    # gpu="A10G",
)
def execute_agent_code(code: str, context: dict) -> dict:
    """在Modal沙箱中执行Agent生成的代码"""
    import io
    import sys

    # 捕获输出
    old_stdout = sys.stdout
    old_stderr = sys.stderr
    sys.stdout = io.StringIO()
    sys.stderr = io.StringIO()

    try:
        # 注入上下文变量
        exec_globals = {"__builtins__": __builtins__}
        exec_globals.update(context)

        # 执行代码
        exec(code, exec_globals)

        return {
            "stdout": sys.stdout.getvalue(),
            "stderr": sys.stderr.getvalue(),
            "exit_code": 0,
        }
    except Exception as e:
        return {
            "stdout": sys.stdout.getvalue(),
            "stderr": f"{type(e).__name__}: {str(e)}",
            "exit_code": 1,
        }
    finally:
        sys.stdout = old_stdout
        sys.stderr = old_stderr


# Modal Sandbox API (推荐)
@app.function()
async def run_in_sandbox(code: str) -> dict:
    """使用Modal Sandbox API"""
    sb = modal.Sandbox.create(
        image=sandbox_image,
        timeout=300,
        cpu=2,
        memory=1024,
    )

    # 执行命令
    process = sb.exec("python", "-c", code)

    return {
        "stdout": process.stdout.read(),
        "stderr": process.stderr.read(),
        "exit_code": process.wait(),
    }
```

## 代码执行安全策略

### 代码静态分析

```python
import ast
from typing import Optional

class CodeSafetyAnalyzer:
    """代码安全性静态分析器"""

    # 禁止的模块
    BLOCKED_MODULES = {
        "os", "subprocess", "shutil", "sys",
        "socket", "http", "urllib", "requests",
        "ctypes", "importlib", "code",
        "compile", "exec", "eval",
    }

    # 禁止的内置函数
    BLOCKED_BUILTINS = {
        "exec", "eval", "compile",
        "__import__", "globals", "locals",
        "getattr", "setattr", "delattr",
    }

    # 危险的AST节点类型
    DANGEROUS_NODE_TYPES = {
        ast.Import,
        ast.ImportFrom,
        ast.Exec,
        ast.Yield,  # 可能用于生成器攻击
    }

    def analyze(self, code: str) -> SafetyReport:
        """分析代码安全性"""
        try:
            tree = ast.parse(code)
        except SyntaxError as e:
            return SafetyReport(
                safe=False,
                violations=[f"语法错误: {str(e)}"],
            )

        violations = []

        for node in ast.walk(tree):
            # 检查导入
            if isinstance(node, ast.Import):
                for alias in node.names:
                    if alias.name.split(".")[0] in self.BLOCKED_MODULES:
                        violations.append(
                            f"禁止导入模块: {alias.name} (行 {node.lineno})"
                        )

            if isinstance(node, ast.ImportFrom):
                if node.module and node.module.split(".")[0] in self.BLOCKED_MODULES:
                    violations.append(
                        f"禁止从模块导入: {node.module} (行 {node.lineno})"
                    )

            # 检查危险函数调用
            if isinstance(node, ast.Call):
                if isinstance(node.func, ast.Name):
                    if node.func.id in self.BLOCKED_BUILTINS:
                        violations.append(
                            f"禁止调用: {node.func.id}() (行 {node.lineno})"
                        )

            # 检查属性访问
            if isinstance(node, ast.Attribute):
                if node.attr.startswith("__"):
                    violations.append(
                        f"禁止访问魔术属性: {node.attr} (行 {node.lineno})"
                    )

        return SafetyReport(
            safe=len(violations) == 0,
            violations=violations,
        )
```

### 运行时沙箱

```python
import resource
import signal

class RuntimeSandbox:
    """运行时代码执行沙箱"""

    def __init__(
        self,
        max_memory_mb: int = 256,
        max_cpu_seconds: int = 30,
        max_output_bytes: int = 1024 * 1024,
    ):
        self.max_memory_mb = max_memory_mb
        self.max_cpu_seconds = max_cpu_seconds
        self.max_output_bytes = max_output_bytes

    def execute(self, code: str, context: dict) -> ExecutionResult:
        """在沙箱中执行代码"""
        import io
        import sys

        # 设置资源限制
        self._set_resource_limits()

        # 设置超时信号
        signal.signal(signal.SIGALRM, self._timeout_handler)
        signal.alarm(self.max_cpu_seconds)

        # 捕获输出
        stdout_capture = io.StringIO()
        stderr_capture = io.StringIO()

        old_stdout = sys.stdout
        old_stderr = sys.stderr
        sys.stdout = stdout_capture
        sys.stderr = stderr_capture

        try:
            # 创建受限的命名空间
            safe_builtins = self._create_safe_builtins()
            exec_globals = {"__builtins__": safe_builtins}
            exec_globals.update(context)

            # 执行代码
            exec(code, exec_globals)

            return ExecutionResult(
                stdout=stdout_capture.getvalue()[:self.max_output_bytes],
                stderr=stderr_capture.getvalue()[:self.max_output_bytes],
                exit_code=0,
            )
        except TimeoutError:
            return ExecutionResult(
                stdout="",
                stderr="执行超时",
                exit_code=124,
            )
        except MemoryError:
            return ExecutionResult(
                stdout="",
                stderr="内存超限",
                exit_code=137,
            )
        except Exception as e:
            return ExecutionResult(
                stdout=stdout_capture.getvalue(),
                stderr=f"{type(e).__name__}: {str(e)}",
                exit_code=1,
            )
        finally:
            sys.stdout = old_stdout
            sys.stderr = old_stderr
            signal.alarm(0)

    def _set_resource_limits(self):
        """设置系统资源限制"""
        # 内存限制
        memory_bytes = self.max_memory_mb * 1024 * 1024
        resource.setrlimit(
            resource.RLIMIT_AS,
            (memory_bytes, memory_bytes),
        )

        # CPU时间限制
        resource.setrlimit(
            resource.RLIMIT_CPU,
            (self.max_cpu_seconds, self.max_cpu_seconds),
        )

        # 文件大小限制
        resource.setrlimit(
            resource.RLIMIT_FSIZE,
            (100 * 1024 * 1024, 100 * 1024 * 1024),  # 100MB
        )

    def _timeout_handler(self, signum, frame):
        raise TimeoutError("执行超时")

    def _create_safe_builtins(self) -> dict:
        """创建安全的内置函数集合"""
        import builtins

        safe = {}
        allowed = [
            "abs", "all", "any", "bin", "bool", "chr", "dict",
            "divmod", "enumerate", "filter", "float", "format",
            "frozenset", "hash", "hex", "id", "int", "isinstance",
            "issubclass", "iter", "len", "list", "map", "max",
            "min", "next", "oct", "ord", "pow", "print", "range",
            "repr", "reversed", "round", "set", "slice", "sorted",
            "str", "sum", "tuple", "type", "zip",
        ]

        for name in allowed:
            if hasattr(builtins, name):
                safe[name] = getattr(builtins, name)

        return safe
```

## K8s Pod Security Standards

### PSS/PSA配置

```yaml
# Pod Security Standards - Restricted级别
apiVersion: v1
kind: Namespace
metadata:
  name: agent-sandbox
  labels:
    pod-security.kubernetes.io/enforce: restricted
    pod-security.kubernetes.io/audit: restricted
    pod-security.kubernetes.io/warn: restricted
```

### NetworkPolicy

```yaml
# Agent网络隔离策略
apiVersion: networking.k8s.io/v1
kind: NetworkPolicy
metadata:
  name: agent-network-policy
  namespace: agent-sandbox
spec:
  podSelector:
    matchLabels:
      app: agent-executor
  policyTypes:
    - Ingress
    - Egress
  ingress:
    # 只允许来自API Gateway的入站流量
    - from:
        - namespaceSelector:
            matchLabels:
              name: api-gateway
          podSelector:
            matchLabels:
              app: api-gateway
      ports:
        - port: 8080
          protocol: TCP
  egress:
    # 允许DNS查询
    - to:
        - namespaceSelector: {}
          podSelector:
            matchLabels:
              k8s-app: kube-dns
      ports:
        - port: 53
          protocol: UDP
        - port: 53
          protocol: TCP
    # 允许访问LLM API（限制IP范围）
    - to:
        - ipBlock:
            cidr: 0.0.0.0/0
            except:
              - 10.0.0.0/8      # 禁止访问内网
              - 172.16.0.0/12
              - 192.168.0.0/16
      ports:
        - port: 443
          protocol: TCP
```

### SecurityContext约束

```yaml
# 完整的安全约束Pod模板
apiVersion: v1
kind: Pod
metadata:
  name: agent-hardened
  namespace: agent-sandbox
  annotations:
    container.apparmor.security.beta.kubernetes.io/agent: localhost/agent-sandbox
spec:
  automountServiceAccountToken: false
  hostNetwork: false
  hostPID: false
  hostIPC: false
  
  securityContext:
    runAsNonRoot: true
    runAsUser: 65534
    runAsGroup: 65534
    fsGroup: 65534
    seccompProfile:
      type: Localhost
      localhostProfile: profiles/agent-sandbox.json
  
  containers:
    - name: agent
      image: registry.example.com/agent-sandbox:latest@sha256:abc123...
      
      securityContext:
        allowPrivilegeEscalation: false
        readOnlyRootFilesystem: true
        capabilities:
          drop:
            - ALL
        seccompProfile:
          type: RuntimeDefault
      
      resources:
        requests:
          cpu: "250m"
          memory: "256Mi"
          ephemeral-storage: "1Gi"
        limits:
          cpu: "1"
          memory: "512Mi"
          ephemeral-storage: "5Gi"
      
      volumeMounts:
        - name: workspace
          mountPath: /workspace
        - name: tmp
          mountPath: /tmp
        - name: cache
          mountPath: /home/agent/.cache
      
      env:
        - name: PYTHONUNBUFFERED
          value: "1"
        - name: PYTHONPYCACHEPREFIX
          value: "/tmp/pycache"
      
      livenessProbe:
        httpGet:
          path: /health
          port: 8080
        initialDelaySeconds: 5
        periodSeconds: 10
      
      readinessProbe:
        httpGet:
          path: /ready
          port: 8080
        initialDelaySeconds: 5
        periodSeconds: 5
  
  volumes:
    - name: workspace
      emptyDir:
        sizeLimit: 1Gi
    - name: tmp
      emptyDir:
        medium: Memory
        sizeLimit: 100Mi
    - name: cache
      emptyDir:
        sizeLimit: 500Mi
```

---

*Agent沙箱是安全执行LLM生成代码的关键基础设施，选择合适的隔离级别需要在安全性、性能和成本之间找到平衡。*


<!-- risk-assessed -->
