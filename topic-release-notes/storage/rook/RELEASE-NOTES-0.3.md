# rook v0.3 Release Notes

Source: [v0.3.1](https://github.com/rook/rook/releases/tag/v0.3.1)

* Deeper Kubernetes integration with the Rook Operator, including new [Storage Pool Third Party Resource](https://github.com/rook/rook/blob/master/Documentation/pool-tpr.md)
  * New [Cluster TPR settings](https://github.com/rook/rook/blob/master/Documentation/cluster-tpr.md) as well
* Ceph Monitor failover when the monitor is determined unhealthy
* [Monitoring of Rook via Prometheus integration](https://github.com/rook/rook/blob/master/Documentation/k8s-monitoring.md)
* New build option to build with Ceph Kraken or Luminous
* Reliability and general bug fixes