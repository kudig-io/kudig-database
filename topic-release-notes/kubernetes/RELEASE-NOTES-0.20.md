# Kubernetes v0.20 Release Notes

Source: GitHub Release [v0.20.2](https://github.com/kubernetes/kubernetes/releases/tag/v0.20.2)

## Known issues
-  CPU usage of kubelet on nodes continually increases by 10-20% a day (#10659).   Will be fixed in 0.21.1.

## [Documentation](http://releases.k8s.io/v0.20.2/docs)

## [Examples](http://releases.k8s.io/v0.20.2/examples)

## Changes since 0.20.1
- Don't make kubelet systemd service depend on Docker #10400
- Kubelet doesn't fight apiserver for cputime on the master. #10471
- Wait until a token shows up to start addons (redux) #10542 

| binary | hash alg | hash |
| --- | --- | --- |
| `kubernetes.tar.gz` | md5 | `0eb67917ace28c5134b18dad88ae7451` |
| `kubernetes.tar.gz` | sha1 | `e58f7a0b7c0587d4f6678999f371be574d4b3c12` |
