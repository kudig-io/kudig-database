# kops v1.5 Release Notes

Source: [1.5.3](https://github.com/kubernetes/kops/releases/tag/1.5.3)

* **Important for Terraform Users** Make ELB naming unambiguous by including the full cluster name.  This will cause the ELBs to be recreated if using Terraform with private topologies, causing disruption of external access to the API and of external access to the bastion (if enabled).  Expected disruption is less than 5 minutes.  Use `export KOPS_FEATURE_FLAGS=+UseLegacyELBName` to keep the legacy naming and avoid disruption.  Fix #1899

* Fix terraform output of shared subnets.  Fix #1977
* Add support for i3 instances (thanks @geojaz)

* Experimental drain rolling-update, 
* Experimental GCE support

* Update Weave to v1.9.3
* Put flannel in guaranteed class (thanks @mihok)
* DNS autoscaler fixes (thanks @MrHohn)
* Remove legacy flags (thanks @mtaufen)
* Add route53 mapper addon (thanks @itskingori)
* Build fixes (thanks @zmerlynn)
* Disable cloudformation delete (thanks @kris-nova)

* Docs fixes (thanks @bowei, @jonchiu, @dosullivan, @DualSpark, @foxylion, @kris-nova
