# prometheus v2.43 Release Notes

Source: [v2.43.1+stringlabels](https://github.com/prometheus/prometheus/releases/tag/v2.43.1%2Bstringlabels)

Special release build that incorporates performance improvements using
the stringlabels Go tag. This release aims to provide a more efficient and
faster solution for users managing large-scale deployments or facing performance
issues with the default Prometheus binaries.

The new labels data structure replaces the existing label/value storage with a
single string, reducing heap size and improving performance in most cases. It
enables Prometheus to use fewer system resources, particularly in
memory-intensive environments.
