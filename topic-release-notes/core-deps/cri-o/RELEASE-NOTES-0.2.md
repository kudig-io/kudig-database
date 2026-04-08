# cri-o v0.2 Release Notes

Source: [v0.2](https://github.com/cri-o/cri-o/releases/tag/v0.2)

With this release, we have made good progress on passing the node conformance tests. 
 
Highlights of the release:

1. Logging support
2. 115/121 (95%) node conformance tests pass  (https://github.com/kubernetes-incubator/cri-o/issues/441)
3. gpg check on image pull
4. Lots of bug fixes
5. Supports latest runc v1.0.0-rc3 and runtime-spec v1.0.0-rc5


Features that don't work yet:
Streaming (exec), attach and port forward.
