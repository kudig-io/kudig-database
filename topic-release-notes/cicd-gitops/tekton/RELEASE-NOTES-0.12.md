# tekton v0.12 Release Notes

Source: [v0.12.1](https://github.com/tektoncd/pipeline/releases/tag/v0.12.1)

# 🎉 Bug Fixes 🎉

-[Docs @ v0.12.1](https://github.com/tektoncd/pipeline/tree/v0.12.1/docs)
-[Examples @ v0.12.1](https://github.com/tektoncd/pipeline/tree/v0.12.1/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.12.1/release.yaml
```

## Upgrade Notices

N/A

## Changes

# Fixes

* :bug: Add PodSecurityPolicy access to webhook's clusterrole (#2620)
* :bug: Fix typo introduced in git-init  (#2620)

[Fill list here]

# Misc

* :hammer: Revert "config: prefixes image names with ko:// scheme 📠" (#2625)
* :hammer: Revert "config: prefixes image names with ko:// scheme" (#2624)
* :hammer: Update golangci configuration (#2620)
* :hammer: Replace devel on all yamls (#2620)

## Thanks

Thanks to these contributors who contributed to v0.12.1!

- :heart: @ad22
- :heart: @afrittoli
- :heart: @sbwsg 
- :heart: @vdemeester

Extra shout-out for awesome release notes:

* :heart_eyes: @afrittoli
* :heart_eyes: @sbwsg