# cri-o v0.1 Release Notes

Source: [v0.1](https://github.com/cri-o/cri-o/releases/tag/v0.1)

We are able to run a k8s cluster with cri-o and run basic pods with this release. 

Features:
1. Pod lifecycle
2. Container lifecycle
3. Image lifecycle
4. CNI networking integration
5. Exec (sync)
6. Support for restarting the daemon
7. Support for multiple storage backend drivers (overlay, devicemapper, aufs, btrfs)
8. SELinux support
9. Seccomp support (partial)
10. Clear Containers support

Features that don’t work yet:
Logging (should be available shortly in 0.2)
Exec (streaming)
Attach
Port-forwarding
