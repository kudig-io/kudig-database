# tekton v0.17 Release Notes

Source: [v0.17.3](https://github.com/tektoncd/pipeline/releases/tag/v0.17.3)

# 🎉 Bug-fix release 🎉

-[Docs @ v0.17.3](https://github.com/tektoncd/pipeline/tree/v0.17.3/docs)
-[Examples @ v0.17.3](https://github.com/tektoncd/pipeline/tree/v0.17.3/examples)

## Installation one-liner

```
kubectl apply -f https://storage.googleapis.com/tekton-releases/pipeline/previous/v0.17.3/release.yaml
```

## Upgrade Notices

N/A

## Changes

# Fixes

* :bug: [cherry-pick] Avoid dangling symlinks in git-init (#3485)

Fixed a bug in `git-init` that allowed a circular symlink to be created from `/root/.ssh` to itself if no SSH credentials are present in the service account and the `disable-home-env-overwrite` flag is set to `"true"`.

* :bug: [cherry-pick] pkg/git: fix ssh credentials detection 🦀 (#3484)

fix ssh credential wrong detection in git-init

## Thanks

Thanks to these contributors who contributed to v0.17.3!
* :heart: @vdemeester
* :heart: @sbwsg 

Extra shout-out for awesome release notes:
* :heart_eyes: @vdemeester
* :heart_eyes: @sbwsg  
