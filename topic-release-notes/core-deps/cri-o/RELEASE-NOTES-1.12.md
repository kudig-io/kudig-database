# cri-o v1.12 Release Notes

Source: [v1.12.10](https://github.com/cri-o/cri-o/releases/tag/v1.12.10)

CRI-O 1.12.10

Welcome to the v1.12.10 release of CRI-O!



Please try out the release binaries and report any issues at
https://github.com/kubernetes-sigs/cri-o/issues.

### Contributors

* Giuseppe Scrivano
* Mrunal Patel
* Urvashi Mohnani

### Changes

* 2c94bb71 version: 1.12.10
* 4e37578a Merge pull request #2148 from giuseppe/race-fixes-1.12
* f7f31279 container_create: fix race with sandbox being stopped
* 81f3ac0c server: serialize StopPodSandbox for the same sandbox
* c507f147 sandbox: simplify if condition
* 47efa17d version: v1.12.10-dev

### Dependency Changes

Previous release can be found at [v1.12.9](https://github.com/kubernetes-sigs/cri-o/releases/tag/v1.12.9)