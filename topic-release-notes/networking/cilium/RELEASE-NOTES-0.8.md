# cilium v0.8 Release Notes

Source: [v0.8.2](https://github.com/cilium/cilium/releases/tag/v0.8.2)

- Separate state directory inside runtime directory (#537)
- Fix all remaining testsuites and have Jenkins fail properly on all failures (#513)
- policy: Support carrying part of the path in the name (#533)
- Temporary fix: Set net.ipv6.conf.all.disable_ipv6=1 as Docker disables it by mistake (#544)