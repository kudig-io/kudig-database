# tekton v0.24 Release Notes

Source: [v0.24.3](https://github.com/tektoncd/pipeline/releases/tag/v0.24.3)

# 🎉 Align to v0.24.1,  Setup WorkingDir for place-tools (#3978) 🎉

-[Docs @ v0.24.3](https://github.com/tektoncd/pipeline/tree/v0.24.3/docs)
-[Examples @ v0.24.3](https://github.com/tektoncd/pipeline/tree/v0.24.3/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.24.3/release.yaml
```

## Changes

# Fixes

* :hammer: Setup WorkingDir for place-tools init container to avoid permission error (#4025)

Fixes bug #3978

* :hammer: Release v0.24.3 (#4024)

This is a minor release to fix the delta between v0.24.1 and v0.24.2.

Release v0.24.1 included by mistake three extra commits.
The next minor release v0.24.2 did not include them, but the difference is problematic: https://github.com/tektoncd/pipeline/issues/4017

Release v0.24.3 builds on top of v0.24.2 and restores the three extra commits:

* https://github.com/tektoncd/pipeline/commit/d3666c263237a88bb02f68d9173a9fff7b5fd50e
```
`WhenExpressions` no longer support PascalCase fields, they only support lowercase fields

action required: if you applied a `Pipeline` with `WhenExpressions` in v0.16, you have to reapply it 
```

* https://github.com/tektoncd/pipeline/commit/bf511ddbb8392e3be8036ef697315f7d36089f7f
* https://github.com/tektoncd/pipeline/commit/b31327bd83512658fdcc61f12806dfad41353a40


## Thanks

Thanks to these contributors who contributed to v0.24.3!
* :heart: @barthy1 
* :heart: @jerop
* :heart: @sbwsg 
* :heart: @afrittoli

Extra shout-out for awesome release notes:
* :heart_eyes: @jerop
* :heart_eyes: @afrittoli
