---
title: cni-plugins v1.1 Release Notes
description: cni-plugins v1.1 Release Notes — Kubernetes 生产运维知识库
category: release-notes
tags:
- k8s
- release-notes
- changelog
- docker
- ingress
last_updated: 2026-05
difficulty: intermediate
reading_level: intermediate
audience:
- 所有工程师
estimated_read_time: 5min
intent_queries:
- cni-plugins v1.1 Release Notes 是什么
- 如何 cni-plugins v1.1 Release Notes
trigger_keywords:
- cni-plugins
- v1.1
- Release
- Notes
- release
- notes
---

# cni-plugins v1.1 Release Notes

Source: [v1.1.1](https://github.com/containernetworking/plugins/releases/tag/v1.1.1)

# Plugins release v1.1.1 
This is a patch release that fixes the following bugs in v1.1.0:

- #702 bridge: call ipam.ExecDel after clean up device in netns
- #709 ipam/dhcp: Fix client id in renew/release

# v1.1.0 Changelog:

One minor-but-major change is that we no longer wait for IPv6 Duplicate
Address Detection to complete. This reduces execution time by 2 seconds.

## New features:
- firewall: support ingressPolicy=(open|same-bridge) for isolating bridges as in Docker ([#584](https://github.com/containernetworking/plugins/pull/584))
- dhcp ipam: support customizing dhcp options from CNI args ([#670](https://github.com/containernetworking/plugins/pull/670))
- Allow setting sysctls on a particular interface ([#669](https://github.com/containernetworking/plugins/pull/669))
- bridge: Add macspoofchk support ([#639](https://github.com/containernetworking/plugins/pull/639)).

## Bug fixes:
- portmap: fix bug that new udp connection deletes all existing conntrack entries ([#705](https://github.com/containernetworking/plugins/pull/705))
- portmap: fix checkPorts result when chain does not exist ([#707](https://github.com/containernetworking/plugins/pull/707))
- dhcp: fixed DHCP problem that broke when fast retry was added ([#681](https://github.com/containernetworking/plugins/pull/681))
- ipvlan: Send Gratuitous ARP after IPs are set ([#675](https://github.com/containernetworking/plugins/pull/675))


## Improvements
- host-device: Bring interfaces up after moving into container ([#679](https://github.com/containernetworking/plugins/pull/679))
- Explicitly Disable Duplicate Address Detection For Container Side Veth ([#695](https://github.com/containernetworking/plugins/pull/695))
- Replace arping package with arp_notify ([#687](https://github.com/containernetworking/plugins/pull/687))
- host-device: add ipam support for dpdk device ([#642](https://github.com/containernetworking/plugins/pull/642))

## Other changes
- Ignore NetNS path errors on delete ([#686](https://github.com/containernetworking/plugins/pull/686))
- Fix confusing error msg invalid cidr ([#638](https://github.com/containernetworking/plugins/pull/638))
