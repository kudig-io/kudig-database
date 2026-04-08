# tekton v0.20 Release Notes

Source: [v0.20.1](https://github.com/tektoncd/pipeline/releases/tag/v0.20.1)

# 🎉 fix task result validation with "status" 🎉

-[Docs @ v0.20.1](https://github.com/tektoncd/pipeline/tree/v0.20.1/docs)
-[Examples @ v0.20.1](https://github.com/tektoncd/pipeline/tree/v0.20.1/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.20.1/release.yaml
```

## Changes

# Fixes

* :hammer: [cherry-pick] validate execution status variable (#3697)

Avoid validating task results while validating context variable to access execution status since it follows similar pattern $(tasks.taskname.results.status) where status is result of some task compared to context variable for referencing execution status $(tasks.taskname.status).



## Thanks

Thanks for the bug report @r0bj 😻 !!

Thanks for the review @sbwsg, @vdemeester, @GregDritschler, @souleb, @afrittoli !!!  

Thanks to these contributors who contributed to v0.20.1!

* :heart: @pritidesai

Extra shout-out for awesome release notes:

* :heart_eyes: @pritidesai

