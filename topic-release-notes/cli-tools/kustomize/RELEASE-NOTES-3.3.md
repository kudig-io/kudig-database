# kustomize v3.3 Release Notes

Source: [v3.3.1](https://github.com/kubernetes-sigs/kustomize/releases/tag/v3.3.1)

Test of new API goreleaser-driven release process.  LGTM.

Ignore the assets, as there's just a binary that prints the API version number.
The important thing with this release is that one may
```
require sigs.k8s.io/kustomize/v3 v3.3.1
```
from your `go.mod` file.

## Changelog

78d14d0d Introduce dummy program to help with API releases.
40ed9e6a fix zh-doc
3cf6b8ec v3.3.0 release notes
281f9328 zh example:chart,secret generator plugin

