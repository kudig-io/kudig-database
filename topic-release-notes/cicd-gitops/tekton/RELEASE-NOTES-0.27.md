# tekton v0.27 Release Notes

Source: [v0.27.3](https://github.com/tektoncd/pipeline/releases/tag/v0.27.3)

# 🎉 Support for Kubernetes 1.22 🎉

-[Docs @ v0.27.3](https://github.com/tektoncd/pipeline/tree/v0.27.3/docs)
-[Examples @ v0.27.3](https://github.com/tektoncd/pipeline/tree/v0.27.3/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.27.3/release.yaml
```

## Upgrade Notices

This is the first Tekton Pipelines release with support for Kubernetes 1.22. There are no other changes.

## Changes

# Fixes

* :bug: Patch vendor/ apimachinery to work on 1.22 (#4164)

Backport adding Subresource field to  ManagedField entries in our `vendor/` folder to make tektoncd/pipeline work on k8s 1.22.

## Thanks

Thanks to these contributors who contributed to v0.27.3!
* :heart: @vdemeester 