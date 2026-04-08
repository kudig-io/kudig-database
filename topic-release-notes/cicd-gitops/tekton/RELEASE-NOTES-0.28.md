# tekton v0.28 Release Notes

Source: [v0.28.3](https://github.com/tektoncd/pipeline/releases/tag/v0.28.3)

# 🎉 Label Propagation Fix and Changes to Implicit Params 🎉

-[Docs @ v0.28.3](https://github.com/tektoncd/pipeline/tree/v0.28.3/docs)
-[Examples @ v0.28.3](https://github.com/tektoncd/pipeline/tree/v0.28.3/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.28.3/release.yaml
```

# Fixes

* #4478 Fix Pipeline/Task to *Run label/annotation propagation
* #4484 Implicit params: don't apply PipelineSpec params to TaskRefs
* #4511 Implicit params: Disable implicit param behavior for Pipeline Objects
* #4521 Update Dockerfiles using golang images to Go 1.16.13

## Thanks

Thanks to these contributors who contributed to v0.32.1!
* :heart: @vdemeester 
* :heart: @wlynch 
* :heart: @sbwsg