---
title: CNI (Container Network Interface)
description: '## 项目概述'
category: cncf-landscape
tags:
- k8s
- cncf
- cloud-native
- ecosystem
- kubelet
- cilium
- flannel
- calico
- containerd
- cri-o
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 架构师
- 技术决策者
- SRE
estimated_read_time: 5min
intent_queries:
- CNI (Container Network Interface) 是什么
- 如何 CNI (Container Network Interface)
- Kubernetes 34 cncf landscape 最佳实践
trigger_keywords:
- CNI
- Container
- Network
- Interface
- cncf
- landscape
---


# CNI (Container Network Interface)

> **成熟度**: Incubating | **加入时间**: 2017-05 | **最后更新**: 2026-03

## 基本信息

| 属性 | 值 |
|:---|:---|
| **官方网站** | https://www.cni.dev |
| **GitHub** | https://github.com/containernetworking/cni |
| **许可证** | Apache-2.0 |
| **主要语言** | Go |
| **CNCF 分类** | Cloud Native Network |

---

## 项目概述

CNI (Container Network Interface) 是一个定义容器网络配置的规范和库，用于在 Linux 容器中配置网络接口。它是 Kubernetes 和其他容器编排平台的网络基础，提供了插件化的网络解决方案。

## 核心特性

- **简单规范**: 清晰的 JSON 配置和二进制插件接口
- **可组合**: 支持多插件链式调用
- **容器运行时无关**: 支持 containerd、CRI-O、Podman 等
- **丰富插件生态**: 官方和第三方插件覆盖各种网络场景
- **版本兼容**: 规范版本向后兼容

---

## 架构设计

```
┌─────────────────────────────────────────────────────────────────┐
│                   Container Runtime (kubelet)                    │
├─────────────────────────────────────────────────────────────────┤
│                              │                                   │
│                    CNI 调用流程                                  │
│                              │                                   │
│  ┌───────────────────────────┴────────────────────────────┐     │
│  │                     CRI Runtime                         │     │
│  │              (containerd / CRI-O / etc.)                │     │
│  └───────────────────────────┬────────────────────────────┘     │
│                              │                                   │
│              reads config    │    executes plugins               │
│                              ▼                                   │
│  ┌────────────────┐    ┌─────────────────────────────────┐     │
│  │ /etc/cni/net.d │───▶│        CNI Plugin Chain         │     │
│  │                │    │                                  │     │
│  │  10-calico.    │    │  ┌──────┐ ┌──────┐ ┌────────┐  │     │
│  │  conflist      │    │  │bridge│→│ IPAM │→│firewall│  │     │
│  │                │    │  └──────┘ └──────┘ └────────┘  │     │
│  └────────────────┘    └─────────────────────────────────┘     │
│                                       │                          │
│                                       ▼                          │
│  ┌─────────────────────────────────────────────────────────┐    │
│  │                   Container Network                      │    │
│  │  ┌─────────────┐              ┌─────────────┐           │    │
│  │  │ Container A │──────────────│ Container B │           │    │
│  │  │  eth0: IP1  │   Network    │  eth0: IP2  │           │    │
│  │  └─────────────┘              └─────────────┘           │    │
│  └─────────────────────────────────────────────────────────┘    │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

### CNI 操作流程

```
┌──────────────────────────────────────────────────────────────┐
│                    CNI Plugin Operations                      │
├──────────────────────────────────────────────────────────────┤
│                                                               │
│   ADD (容器创建)                                              │
│   ┌─────────┐    ┌─────────┐    ┌─────────┐    ┌─────────┐  │
│   │ Create  │───▶│  Setup  │───▶│ Assign  │───▶│ Return  │  │
│   │ veth    │    │ Bridge  │    │   IP    │    │ Result  │  │
│   └─────────┘    └─────────┘    └─────────┘    └─────────┘  │
│                                                               │
│   DEL (容器删除)                                              │
│   ┌─────────┐    ┌─────────┐    ┌─────────┐                 │
│   │ Release │───▶│ Remove  │───▶│ Cleanup │                 │
│   │   IP    │    │  veth   │    │  Route  │                 │
│   └─────────┘    └─────────┘    └─────────┘                 │
│                                                               │
│   CHECK (健康检查)                                            │
│   ┌─────────┐    ┌─────────┐                                │
│   │ Verify  │───▶│ Return  │                                │
│   │ Config  │    │ Status  │                                │
│   └─────────┘    └─────────┘                                │
│                                                               │
└──────────────────────────────────────────────────────────────┘
```

---

## 核心概念

### 配置文件格式

```json
// /etc/cni/net.d/10-mynet.conflist
{
  "cniVersion": "1.0.0",
  "name": "mynet",
  "plugins": [
    {
      "type": "bridge",
      "bridge": "cni0",
      "isGateway": true,
      "ipMasq": true,
      "ipam": {
        "type": "host-local",
        "subnet": "10.22.0.0/16",
        "routes": [
          { "dst": "0.0.0.0/0" }
        ]
      }
    },
    {
      "type": "portmap",
      "capabilities": { "portMappings": true }
    },
    {
      "type": "firewall"
    }
  ]
}
```

### 环境变量

| 变量 | 说明 |
|------|------|
| CNI_COMMAND | 操作类型: ADD, DEL, CHECK, VERSION |
| CNI_CONTAINERID | 容器 ID |
| CNI_NETNS | 网络命名空间路径 |
| CNI_IFNAME | 接口名称 (如 eth0) |
| CNI_ARGS | 额外参数 |
| CNI_PATH | 插件搜索路径 |

---

## 常用插件

### 主插件 (Main Plugins)

| 插件 | 功能 | 适用场景 |
|------|------|----------|
| bridge | 创建网桥，连接容器 | 单节点网络 |
| macvlan | 直接使用物理网卡 MAC | 需要物理网络可达 |
| ipvlan | 共享 MAC 不同 IP | 类似 macvlan |
| ptp | 点对点 veth | 简单互联 |
| host-device | 移动网卡到容器 | 直通网卡 |

### IPAM 插件

| 插件 | 功能 |
|------|------|
| host-local | 本地 IP 分配 |
| dhcp | DHCP 获取 IP |
| static | 静态 IP 配置 |

### Meta 插件

| 插件 | 功能 |
|------|------|
| portmap | 端口映射 (DNAT) |
| bandwidth | 带宽限制 |
| firewall | iptables 规则 |
| tuning | sysctl 调优 |

---

## 快速开始

### 安装 CNI 插件

```bash
# 下载 CNI 插件
CNI_VERSION=v1.4.0
curl -LO https://github.com/containernetworking/plugins/releases/download/${CNI_VERSION}/cni-plugins-linux-amd64-${CNI_VERSION}.tgz

# 安装到标准路径
sudo mkdir -p /opt/cni/bin
sudo tar -xzf cni-plugins-linux-amd64-${CNI_VERSION}.tgz -C /opt/cni/bin

# 验证
ls /opt/cni/bin
```

### 创建网络配置

```bash
# 创建配置目录
sudo mkdir -p /etc/cni/net.d

# Bridge 网络配置
cat << 'EOF' | sudo tee /etc/cni/net.d/10-bridge.conflist
{
  "cniVersion": "1.0.0",
  "name": "bridge-network",
  "plugins": [
    {
      "type": "bridge",
      "bridge": "cni0",
      "isGateway": true,
      "ipMasq": true,
      "ipam": {
        "type": "host-local",
        "subnet": "10.88.0.0/16",
        "routes": [
          { "dst": "0.0.0.0/0" }
        ]
      }
    },
    {
      "type": "portmap",
      "capabilities": { "portMappings": true }
    }
  ]
}
EOF
```

### 手动测试 CNI

```bash
# 创建网络命名空间
sudo ip netns add test-ns

# 设置环境变量
export CNI_PATH=/opt/cni/bin
export CNI_CONTAINERID=test-container
export CNI_NETNS=/var/run/netns/test-ns
export CNI_IFNAME=eth0
export CNI_COMMAND=ADD

# 执行 CNI 插件
cat /etc/cni/net.d/10-bridge.conflist | sudo -E /opt/cni/bin/bridge

# 查看容器网络
sudo ip netns exec test-ns ip addr
sudo ip netns exec test-ns ip route

# 清理
export CNI_COMMAND=DEL
cat /etc/cni/net.d/10-bridge.conflist | sudo -E /opt/cni/bin/bridge
sudo ip netns del test-ns
```

---

## 自定义 CNI 插件开发

### Go 插件示例

```go
// main.go
package main

import (
    "encoding/json"
    "fmt"
    "net"
    
    "github.com/containernetworking/cni/pkg/skel"
    "github.com/containernetworking/cni/pkg/types"
    current "github.com/containernetworking/cni/pkg/types/100"
    "github.com/containernetworking/cni/pkg/version"
    bv "github.com/containernetworking/plugins/pkg/utils/buildversion"
)

type PluginConf struct {
    types.NetConf
    MyOption string `json:"myOption"`
}

func parseConfig(stdin []byte) (*PluginConf, error) {
    conf := &PluginConf{}
    if err := json.Unmarshal(stdin, conf); err != nil {
        return nil, fmt.Errorf("failed to parse config: %v", err)
    }
    return conf, nil
}

func cmdAdd(args *skel.CmdArgs) error {
    conf, err := parseConfig(args.StdinData)
    if err != nil {
        return err
    }
    
    // 实现网络配置逻辑
    // 1. 创建 veth pair
    // 2. 配置 IP 地址
    // 3. 设置路由
    
    result := &current.Result{
        CNIVersion: conf.CNIVersion,
        Interfaces: []*current.Interface{
            {
                Name:    args.IfName,
                Sandbox: args.Netns,
            },
        },
        IPs: []*current.IPConfig{
            {
                Address: net.IPNet{
                    IP:   net.ParseIP("10.88.0.2"),
                    Mask: net.CIDRMask(24, 32),
                },
                Gateway: net.ParseIP("10.88.0.1"),
            },
        },
    }
    
    return types.PrintResult(result, conf.CNIVersion)
}

func cmdDel(args *skel.CmdArgs) error {
    // 清理网络配置
    return nil
}

func cmdCheck(args *skel.CmdArgs) error {
    // 验证网络配置
    return nil
}

func main() {
    skel.PluginMain(cmdAdd, cmdCheck, cmdDel, version.All, bv.BuildString("my-cni"))
}
```

---

## Kubernetes 中的 CNI

### Kubelet CNI 配置

```yaml
# /var/lib/kubelet/config.yaml
apiVersion: kubelet.config.k8s.io/v1beta1
kind: KubeletConfiguration
networkPlugin: cni
cniConfDir: /etc/cni/net.d
cniBinDir: /opt/cni/bin
```

### 常见 CNI 实现

| 实现 | 特点 | 适用场景 |
|------|------|----------|
| Calico | BGP 路由、Network Policy | 大规模生产 |
| Cilium | eBPF、L7 策略 | 高性能、可观测性 |
| Flannel | 简单 Overlay | 小规模、学习 |
| Weave | 加密、多播 | 安全要求高 |
| Canal | Flannel + Calico 策略 | 混合需求 |

---

## 调试技巧

```bash
# 查看 CNI 日志
journalctl -u kubelet | grep cni

# 检查网络配置
cat /etc/cni/net.d/*.conflist

# 验证插件
ls -la /opt/cni/bin/

# 检查网桥
ip link show type bridge
brctl show

# 网络命名空间
ip netns list
```

---

## 最佳实践

1. **版本兼容**: 使用与 Kubernetes 版本匹配的 CNI 规范
2. **配置优先级**: 配置文件按字典序加载，用数字前缀控制顺序
3. **IPAM 选择**: 生产环境推荐使用 host-local 或 Calico IPAM
4. **网络策略**: 结合 NetworkPolicy 实现细粒度访问控制
5. **监控**: 监控网络插件健康状态和 IP 池使用情况

---

## 参考资源

- [官方文档](https://www.cni.dev/docs)
- [GitHub Repo](https://github.com/containernetworking/cni)
- [CNI Plugins](https://github.com/containernetworking/plugins)
- [CNI 规范](https://www.cni.dev/docs/spec/)
- [Kubernetes 网络模型](https://kubernetes.io/docs/concepts/cluster-administration/networking/)

---

**维护者**: Kudig Team | **许可证**: MIT
