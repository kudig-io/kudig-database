# cni-plugins v1.9 Release Notes

Source: [v1.9.1](https://github.com/containernetworking/plugins/releases/tag/v1.9.1)

This is a patch release with dependency updates and some minor fixes.

## Minor fixes
* bandwidth: handle nil bandwidth in CHECK by @squeed in https://github.com/containernetworking/plugins/pull/1222
* vrf: fix route filtering to preserve IPAM-configured routes by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1227
* CVE-2025-52881: Bump selinux to 1.13.0 by @sbiradar10 in https://github.com/containernetworking/plugins/pull/1231
* bridge: include attempted IP address in AddrAdd error message by @Amulyam24 in https://github.com/containernetworking/plugins/pull/1225

## New Contributors
* @sbiradar10 made their first contribution in https://github.com/containernetworking/plugins/pull/1231
* @Amulyam24 made their first contribution in https://github.com/containernetworking/plugins/pull/1225

**Full Changelog**: https://github.com/containernetworking/plugins/compare/v1.9.0...v1.9.1