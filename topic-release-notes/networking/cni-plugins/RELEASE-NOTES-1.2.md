# cni-plugins v1.2 Release Notes

Source: [v1.2.0](https://github.com/containernetworking/plugins/releases/tag/v1.2.0)

# Changelog:

## New plugins & features
- ([#743](https://github.com/containernetworking/plugins/pull/743)). dummy: Create a Dummy CNI plugin that creates a virtual interface 
- ([#725](https://github.com/containernetworking/plugins/pull/725)). V2 API support for win-overlay CNI
- ([#693](https://github.com/containernetworking/plugins/pull/693)). tuning Add sysctl allowList

## Bug fixes
- ([#809](https://github.com/containernetworking/plugins/pull/809)). bridge: refresh host-veth mac after port add
- ([#802](https://github.com/containernetworking/plugins/pull/802)). Add IPv6 support for AddDefaultRoute
- ([#779](https://github.com/containernetworking/plugins/pull/779)). Fix path substitution to enable setting sysctls on vlan interfaces
- ([#782](https://github.com/containernetworking/plugins/pull/782)). host-local: fix bug on getting NextIP of addresses with first byte
- ([#709](https://github.com/containernetworking/plugins/pull/709)). dhcp: Fix client id in renew/release

## Improvements & Cleanups:
- ([#772](https://github.com/containernetworking/plugins/pull/772)). portmap support masquerade all
- ([#733](https://github.com/containernetworking/plugins/pull/733)). bridge: support IPAM DNS settings
- ([#702](https://github.com/containernetworking/plugins/pull/702)). bridge:  call ipam.ExecDel after clean up device in netns #702 
- ([#768](https://github.com/containernetworking/plugins/pull/768)). dhcp: Cleanup Socket and Pidfile on exit
- ([#792](https://github.com/containernetworking/plugins/pull/792)). dhcp: Update Allocate method to reuse lease if present
- ([#755](https://github.com/containernetworking/plugins/pull/755)). dhcp: Use the same options for acquiring, renewing lease
- ([#730](https://github.com/containernetworking/plugins/pull/730)). tuning Check for duplicated sysctl keys
- ([#739](https://github.com/containernetworking/plugins/pull/739)). build: support riscv64
- ([#712](https://github.com/containernetworking/plugins/pull/712)). bug: return errors when iptables and ip6tables are unusable
- ([#719](https://github.com/containernetworking/plugins/pull/719)). Make description for `static` plugin more exact


As always, many thanks to our contributors.