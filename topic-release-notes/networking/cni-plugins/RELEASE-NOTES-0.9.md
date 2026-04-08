# cni-plugins v0.9 Release Notes

Source: [v0.9.1](https://github.com/containernetworking/plugins/releases/tag/v0.9.1)

This is a minor update to the CNI plugins that bumps a few dependencies and includes some small behavior tweaks.

New behavior:
- DHCP timeout is configurable ([#565](https://github.com/containernetworking/plugins/pull/565)). 
- host-device: Add support for DPDK device ([#490](https://github.com/containernetworking/plugins/pull/490)). Host-device plugin is a noop for DPDK devices

Fixes:
- vlan: fix error message text by removing ptp references ([#566](https://github.com/containernetworking/plugins/pull/566)). Fixing a few error messages that the vlan plugin returns. These appear to be mistaken references to the ptp plugin.
- vlan: Fix error handling for delegate IPAM plugin ([#568](https://github.com/containernetworking/plugins/pull/568)).
- deps: bump coreos/go-iptables ([#563](https://github.com/containernetworking/plugins/pull/563)). Closes #544
