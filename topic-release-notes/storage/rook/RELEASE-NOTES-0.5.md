# rook v0.5 Release Notes

Source: [v0.5.1](https://github.com/rook/rook/releases/tag/v0.5.1)

Rook v0.5.1 is a patch release limited in scope and focusing on bug fixes and build improvements.

### Improvements
* Ceph Luminous has been upgraded to [12.1.3](http://ceph.com/releases/v12-1-3-luminous-rc-released/)
* Helm charts are now built and published as part of the continuous integration pipeline.  Details can be found in the [Helm Chart readme](https://rook.io/docs/rook/v0.5/helm-operator.html)
* Improve initial monitor quorum performance so a Rook cluster can be bootstrapped more quickly
* Rook's metrics and monitoring via Prometheus is now fully compatible with Ceph Luminous
* Allow [placement policy](https://rook.io/docs/rook/v0.5/cluster-crd.html#placement-configuration-settings) to be applied to manager pods