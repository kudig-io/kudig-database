# cni-plugins v0.8 Release Notes

Source: [v0.8.7](https://github.com/containernetworking/plugins/releases/tag/v0.8.7)

This is a minor release with some bugfixes and minor improvements:

## New Features

- macvlan: set mac address from args and capabilities ([#480](https://github.com/containernetworking/plugins/pull/480)).

## Bugfixes & Cleanups
- flannel: remove net conf file after DEL succeed ([#449](https://github.com/containernetworking/plugins/pull/449)).
- portmap should not perform deletions if not portMapping config received ([#509](https://github.com/containernetworking/plugins/pull/509)). 
- portmap: don't use unspecified address as iptables rule destination  ([#487](https://github.com/containernetworking/plugins/pull/487)).
- Fix race condition in GetCurrentNS ([#523](https://github.com/containernetworking/plugins/pull/523)). 
- firewall: fix generate of admin chain comment ([#506](https://github.com/containernetworking/plugins/pull/506)).
- Fix handling of delay in acquiring lease with stp turned on ([#501](https://github.com/containernetworking/plugins/pull/501)).
- host-device: Bring interfaces down before moving ([#486](https://github.com/containernetworking/plugins/pull/486)).
