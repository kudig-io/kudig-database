# cni-plugins v1.8 Release Notes

Source: [v1.8.0](https://github.com/containernetworking/plugins/releases/tag/v1.8.0)

The Bridge CNI plugin has removed limitations on VLAN trunk implementation. This aligns with recommended access and trunk port configurations, ensuring proper VLAN isolation and enhanced usability.

## What's Changed
* Allow vlan parameter to set native vlan on trunk ports by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1180
* Set default value of PreserveDefaultVlan to False by @mlguerrero12 in https://github.com/containernetworking/plugins/pull/1181
* remove duplicate route.Table and route.Scope assignments by @runsisi in https://github.com/containernetworking/plugins/pull/1192
* Set value of gw to nil for opt121 routes in DHCP by @omartin2010 in https://github.com/containernetworking/plugins/pull/1187

## New Contributors
* @runsisi made their first contribution in https://github.com/containernetworking/plugins/pull/1192
* @omartin2010 made their first contribution in https://github.com/containernetworking/plugins/pull/1187

**Full Changelog**: https://github.com/containernetworking/plugins/compare/v1.7.0...v1.8.0