# cni-plugins v1.7 Release Notes

Source: [v1.7.1](https://github.com/containernetworking/plugins/releases/tag/v1.7.1)

## What's Changed

(Administrative note: the GitHub release v1.7.0 somehow got split in to two immutable releases. v1.7.1 is a re-release that corrects the issue. Apologies for the trouble).

### New features / options
* bridge: Add option to enable port isolation by @ormergi in https://github.com/containernetworking/plugins/pull/1141
* Add a new firewall ingress-policy "isolated" by @swagatbora90 in https://github.com/containernetworking/plugins/pull/1140

### Other improvements
* host-device: Return interface name in result by @sriramy in https://github.com/containernetworking/plugins/pull/1147
* Add retries for netlink calls that may return a EINTR by @adrianmoisey in https://github.com/containernetworking/plugins/pull/1154
* Enable KeepAddrOnDown for ipv6 addresses by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1155
* Implement exponential backoff in vrf plugin by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1156

### Bug fixes
* DHCP lease maintenance should terminate when interface no longer exists. by @dougbtv in https://github.com/containernetworking/plugins/pull/1143
* Fix addresses and routes reinserted to the VRF by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1151
* Check error returned by ipv6 SettleAddresses by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1168


## New Contributors
* @sriramy made their first contribution in https://github.com/containernetworking/plugins/pull/1147
* @swagatbora90 made their first contribution in https://github.com/containernetworking/plugins/pull/1140
* @dougbtv made their first contribution in https://github.com/containernetworking/plugins/pull/1143
* @adrianmoisey made their first contribution in https://github.com/containernetworking/plugins/pull/1154

**Full Changelog**: https://github.com/containernetworking/plugins/compare/v1.6.2...v1.7.1