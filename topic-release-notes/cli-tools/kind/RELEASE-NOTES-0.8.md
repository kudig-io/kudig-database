# kind v0.8 Release Notes

Source: [v0.8.1](https://github.com/kubernetes-sigs/kind/releases/tag/v0.8.1)

**This is a tiny patch release to pick up the fix for [Can't create ipv4 clusters if ipv6 is disabled at kernel level](https://github.com/kubernetes-sigs/kind/issues/1544).**

**For full release notes please see [v0.8.0](https://github.com/kubernetes-sigs/kind/releases/tag/v0.8.0).**

**Most users will not need to upgrade to this release, this bug is only known to occur on hosts with the `ipv6.disable=1` kernel parameter.**