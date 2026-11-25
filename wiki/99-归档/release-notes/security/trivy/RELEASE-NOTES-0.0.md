---
title: runc v0.0 Release Notes
description: runc v0.0 Release Notes — Kubernetes 生产运维知识库
summary: runc v0.0 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- rag
tier: peripheral
created: '2026-05-23'
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- runc v0.0 Release Notes 是什么
- 如何 runc v0.0 Release Notes
trigger_keywords:
- runc
- v0.0
- Release
- Notes
- release
- notes
prerequisites:
- kubectl-basics
- cncf-ecosystem
---

> **生产环境安全提示**
>
> 本文档包含可直接执行的运维命令。执行前请确认：当前目标集群与 Namespace 是否正确；是否具备足够的 RBAC 权限；是否已在非生产环境验证。命令风险等级标注：🔴 高风险（可能造成数据丢失或服务中断）、🟡 中风险（会修改集群状态，但通常可回滚）、🟢 低风险/只读（信息收集，无副作用）。




# runc v0.0 Release Notes

Source: [v0.0.9](https://github.com/opencontainers/runc/releases/tag/v0.0.9)

## runc 0.0.9

This new release of runc includes the specification v0.4 changes.  The backwards incompatible changes includes moving process specific settings like capabilities, rlimits, apparmor, and selinux process label from the container configuration to the process configuration.  Be sure to update your `config.json` files for these changes or they will not be applied to the container.  You can always use the `runc spec` command to generate a compatible `config.json` based on the specification version that runc is currently using.  

### Updates:
- In this release runc has better support for errors and logging for use with the `--log` flag.  
- Improved namespace sharing for joining PID namespaces.
- Allow all mount types inside the container's mount namespace.
- Updated masked and readonly paths for container's /proc.
- Better IO handling for container's STDIO.
- Unique session keyring support for containers.
- Container label support.
- No new privileges support.
- Various bug fixes and performance improvements. 

```
NAME:
   runc - Open Container Initiative runtime

runc is a command line client for running applications packaged according to
the Open Container Format (OCF) and is a compliant implementation of the
Open Container Initiative specification.

runc integrates well with existing process supervisors to provide a production
container runtime environment for applications. It can be used with your
existing process monitoring tools and the container will be spawned as a
direct child of the process supervisor.

Containers are configured using bundles. A bundle for a container is a directory
that includes a specification file named "config.json" and a root filesystem.
The root filesystem contains the contents of the container. 

To start a new instance of a container:

    # runc start [ -b bundle ] <container-id>

Where "<container-id>" is your name for the instance of the container that you
are starting. The name you provide for the container instance must be unique on
your host. Providing the bundle directory using "-b" is optional. The default
value for "bundle" is the current directory.

USAGE:
   runc [global options] command [command options] [arguments...]

VERSION:
   0.0.9
spec version 0.4.0

COMMANDS:
   checkpoint   checkpoint a running container
   delete   delete any resources held by the container often used with detached containers
   events   display container events such as OOM notifications, cpu, memory, IO and network stats
   exec     execute new process inside the container
   init     init is used to initialize the containers namespaces and launch the users process.
    This command should not be called outside of runc.

   kill     kill sends the specified signal (default: SIGTERM) to the container's init process
   list     lists containers started by runc with the given root
   pause    pause suspends all processes inside the container
   restore  restore a container from a previous checkpoint
   resume   resumes all processes that have been previously paused
   spec     create a new specification file
   start    create and run a container
   state    output the state of a container
   help, h  Shows a list of commands or help for one command

GLOBAL OPTIONS:
   --debug      enable debug output for logging
   --log "/dev/null"    set the log file path where internal debug information is written
   --log-format "text"  set the format used by logs ('text' (default), or 'json')
   --root "/run/runc"   root directory for storage of container state (this should be located in tmpfs)
   --criu "criu"    path to the criu binary used for checkpoint and restore
   --help, -h       show help
   --version, -v    print the version

```


<!-- risk-assessed -->
