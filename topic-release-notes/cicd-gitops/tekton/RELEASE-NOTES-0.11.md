# tekton v0.11 Release Notes

Source: [v0.11.3](https://github.com/tektoncd/pipeline/releases/tag/v0.11.3)

# 🎉 Timeout and Result Fixes 🎉

-[Docs @ v0.11.3](https://github.com/tektoncd/pipeline/tree/v0.11.3/docs)
-[Examples @ v0.11.3](https://github.com/tektoncd/pipeline/tree/v0.11.3/examples)

0.11.3 is likely to be the final patch release before 0.12.  Included here is a fix for Timeouts in PipelineTasks and several fixes for Task Results.  This release also adds support for using Task Results in the parameters of Conditions.

## Upgrade Notices

🚨 If you are upgrading from a version of Tekton Pipelines older than v0.11.0 then you may need to delete your existing tekton-pipeline deployments before applying v0.11.3.

## Changes

### Fixes

* :bug: Fix 3 bugs with Task Results #2471 
* :bug: Fix PipelineTask timeout not correctly set #2468 

## Thanks

Thanks to these contributors who contributed to v0.11.3!

- :heart: @bobcatfish
- :heart: @othomann 
- :heart: @vdemeester