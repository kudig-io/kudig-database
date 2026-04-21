# tekton v0.16 Release Notes

Source: [v0.16.3](https://github.com/tektoncd/pipeline/releases/tag/v0.16.3)

# 🎉  Fix nil pointer with timeouts  🎉

-[Docs @ v0.16.3](https://github.com/tektoncd/pipeline/tree/v0.16.3/docs)
-[Examples @ v0.16.3](https://github.com/tektoncd/pipeline/tree/v0.16.3/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.16.3/release.yaml
```

## Changes

# Fixes

* :bug: Fix `nil` pointer exception in case the PipelineRun timeout is not specified (nor default applied)⏲ (#3241)

## Thanks

Thanks for the bug report @dghubble 😻 !!

Thanks to these contributors who contributed to v0.16.3!

- :heart: @vdemeester