.. SPDX-License-Identifier: GPL-2.0+

=================
Reclaim Cold Page
=================

Background
==========

The cgroup technology has been intensively deployed on contemporary servers
because of its capabilities to provide resource aggregation, separation in
light-weight cost. Various types of models are introduced to manage different
resources. For example, memory cgroup is used to manage the memory resources
consumed by the cgroup, meaning the consumed memory is charged to the memory
cgroup. Naturally, the available memory cgroups are presented using a hierarchy
tree, like below figure shows. In the mean while, the system scoped behaviours,
like page reclaim, are also applied to memory cgroups. the page reclaim could
be classified to asynchronous (indirect) and sychronous (direct) mechanisms,
which is similar to the system level behaviour. The memory cgroup is stateful
objects and states are maintained by the well-known reference count. There
are 2 primary states: online and offline. Memory cgroup usually stays in online
state for longest period and becomes offline after it's removed from system.
Huge amount of memory could be still hold by the memory cgroup when it becomes
offline.


                                  (root)
                                    |
                         +----------+----------+
                         |          |          |
                        (A)        (B)        (C)
                         |          |          |
                    +----+----+     +     +----+----+
                    |    |    |     |     |    |    |
                   (A1) (A2) (A3)  (B1)  (C1) (C2) (C3)


On the other hand, kidled has been enabled to periodically scan the consumed
pages, sorting out their access frequency (cold vs hot) and present the
statistics to users by cgroup file (``memory.idle_page_stats``). Unfortunately,
the statistics isn't fully exploited so far. Furthermore, memory is usually
blamed as bottleneck to improve the density of cgroups which are deployed on
one physical box. Besides, memory is relatively expensive, accounting for
heavy proportion of physical box cost. So it becomes serious that how to
improve the memory usage efficiency. The module sits on the position to fill
the gaps.

From the standpoint of usage, page frames (memory) are classified to page cache,
anonymous page etc. Page cache, especially clean one, accounts for most of the
system memory. So we come up with the cold page reclaim scheme, to leverage the
Kidled's statistics to reclaim cold and clean page cache on request. Besides, the
anonymous pages could contribute large memory consumption in some circumstances.
So the module also provides capability to reclaim the cold anonymous pages. The
module receives reclaim requests from user space through sysfs or cgroup files,
iterates the LRU lists of the target memory cgroup, then reclaims or migrates
the pages which meet the user-defined conditions.


To avoid kernel upgrade introduced by changes to this module, which is pretty
hard in production environment, we had the decision to make the functionality
(reclaim cold pages) as a kernel module. In this way, the module can be changed
and improved, without depending on the kernel upgrade in production environment.

Interfaces
==========

The module receives requests through sysfs and cgroup files. The global sysfs
files and per-memory-cgroup files are exported. The global sysfs files are
used to tell the global preference, while the per-memory-cgroup files are used
to convey parameters for (cold) page reclaim in that memory cgroup. The global
sysfs files include:

* ``/sys/kernel/mm/coldpgs/version``

  Exports the module version, usually used by user space program to adapt
  itself. The module's capabilities are identified through it. It's split
  up into 3 bytes: major, minor, revision respectively.

* ``/sys/kernel/mm/coldpgs/flags``

  Bitmask to specify the reclaim behaviours. Currently, bit#0, bit#1 is used
  only. The other bits have no effects on the reclaim behaviours.

  Bit[0]: specify if the mlock'ed pages are eligible to be reclaimed.
  Bit[1]: specify if the 0 age pages are eligible to be reclaimed.

* ``/sys/kernel/mm/coldpgs/batch``

  The LRU lock is hold and interrupt is disabled in middle of isolating or
  reclaiming pages. The CPU could be hogged in this situation. This specifies
  the number of cold pages to be isolated or reclaimed in one shot, prior to
  releasing LRU lock and enabling interrupt. It's 32 pages by default.

* ``/sys/kernel/mm/coldpgs/hierarchy``

  Specify the output statistics from ``memory.coldpgs.stats`` includes the
  that of the subordinate memory cgroups or not. It's 0 by default, meaing
  the statistics from the subordinate memory cgroup are excluded.

* ``/sys/kernel/mm/coldpgs/mode``

  The bitmask to indicate the page types to be reclaimed or migrated during
  global or per-mem-cgroup reclaim. Currently, the following settings are
  supported. Any combined modes are also supported.

  Bit[0] : Reclaim clean page cache from memory cgroup(s)
  Bit[1] : Reclaim anonymous pages from memory cgroup(s)
  Bit[2] : Reclaim reclaimable slab objects from memory cgroup(s),
           currently including dentries, inodes.

* ``/sys/kernel/mm/coldpgs/threshold``

  Accept page coldness value (0 - 255) to start global reclaim. The existing
  memory cgroups will be iterated and its page cache or anonymous pages, which
  are colder than this value will be reclaimed. Note that the amount of pages
  to be reclaimed isn't limited in global reclaim scheme.

* ``/sys/kernel/mm/coldpgs/threshold_nonrot``

  When reclaiming cold anonymous pages, the reclaimed memory could be backed
  up by rotational swap device, SSD swap device or zSwap. The zSwap has priority
  over the left media. This specifies the anonymous pages, which are less
  colder than the value in ``threshold_nonrot`` will be backed by zSwap. The
  other anonymous pages will be backed by rotational or SSD swap device.

* ``/sys/kernel/mm/coldpgs/swapin``

  "swapin" is only accepted by this. The string is simply returned on reading.
  On writing, all memory cgroups will be walked through and swap in all pages,
  which have been backed by zSwap or traditional swap device. It should be
  assured that the corresponding memory cgroup's memory limit won't be broken,
  prior to issuing the operation.

There are bundle of cgroup files populated for every memory cgroup, shown as
below:

* ``memory.coldpgs.flags``

  Currently, the field is used as three different purpose, its low 32bit is
  designed to determine the coldness threshold for the corresponding memory
  cgroup by userland daemon (idlemd). Meanwhile, it also indicates if the
  memory cgroup is involved in reclaiming cold pages. The low 8bit of high
  32bit represent the reclaim type of the cgroup. It means that the cgroup
  allows to reclaim the available page controlled by user.

  This 64bit field is divided as follows.

                 FFFFFF | 111 |     1     |  1  | 111 | FFFFFFFF
                 ---+---|--+--|-----+-----|--+--|--+--|----+----|
                    |      |        |     |     |     |
                reserved  rsv  ignore_age  mlock  mode  coldness

  ignore_age bit: Ignoring age of pages to reclaim unconditonally. With this
  bit is set, memory.coldpgs.threshold is allowed to be 0 to reclaim pages
  without checking pages' age.

* ``memory.coldpgs.threshold``

  The coldness threshold used in memory cgroup scoped reclaim. The memory
  cgroup's page cache or anonymous pages, which are colder than the threshold,
  will be reclaimed. The amount (memory in bytes) to be reclaimed is limited
  by ``memory.coldpgs.size``. The corresponding memory cgroup might have
  subordinate offline memory cgroups. During the memory cgroup scoped reclaim,
  the subordinate offline memory cgroups will experience page reclaim as well.
  The coldness threshold is inherited from the parent, but the amount isn't
  limited.

* ``memory.coldpgs.size``

  The amount of memory that can be reclaimed from the corresponding memory
  cgroup. The coldness threshold is given by ``memory.coldpgs.threshold``
  in memory cgroup scoped reclaim. The corresponding memory cgroup might have
  subordinate offline memory cgroups. During the memory cgroup scoped reclaim,
  the subordinate offline memory cgroups will experience page reclaim as well.
  The coldness threshold is inherited from the parent, but the amount isn't
  limited.

* ``memory.coldpgs.stats``

  Statistics to reveal the activities about reclaiming cold pages.

* ``memory.coldpgs.swapin``

  "swapin" is simply returned on reading. It's the only content accepted by
  this. On writing, the anonymous pages, which are charged to the memory
  cgroup or its subordinate memory cgroups will be swapped in. Note that
  memory cgroup's memory limit won't be broken, prior to issuing the operation.

Examples
========

Prior to do page reclaim through the module, we need make sure Kidled has been
enabled and working properly:

   # echo 1 > /sys/kernel/mm/kidled/use_hierarchy
   # echo 15 > /sys/kernel/mm/kidled/scan_period_in_seconds

1) Global scoped page cache reclaim

   The global scoped page cache reclaim simply starts by feeding the coldness
   threshold to ``/sys/kernel/mm/coldpgs/threshold``. Also, the mode must be
   specified by writing to ``/sys/kernel/mm/coldpgs/mode``.

   # dd if=/dev/zero of=/ext4/test.data bs=1M count=128
   # mkdir /cgroup/memory/test;                                \
     echo $$ > /cgroup/memory/test/tasks;                      \
     dd if=/ext4/test.data of=/dev/null bs=1M count=128
   # < hold for a while, wait the page cache to become cold enough >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfei
     cfei 67108864 67108864 0 0 0 0 0 0
   # echo 0x1 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /sys/kernel/mm/coldpgs/threshold
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfei
     cfei 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 32768
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 0

2) Global scoped locked page cache reclaim

   The locked page cache won't be reclaimed successfully without setting
   ``/sys/kernel/mm/coldpgs/flags`` correctly.

   # < Run program to mmap 128-MB file and lock the area >
   # < hold for a while, wait the page cache to become cold enough >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfui
     cfui 0 134217728 0 0 0 0 0 0
   # echo 0x1 > /sys/kernel/mm/coldpgs/flags;                  \
     echo 0x1 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /sys/kernel/mm/coldpgs/threshold
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfui
     cfui 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 32768
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 0

3) Memory cgroup scoped page cache reclaim

   The memory cgroup scoped page cache reclaim starts by writing cgroup files:
   ``memory.coldpgs.threshold`` and ``memory.coldpgs.size``

   # dd if=/dev/zero of=/ext4/test.data bs=1M count=128
   # mkdir /cgroup/memory/test;                                \
     echo $$ > /cgroup/memory/test/tasks;                      \
     dd if=/ext4/test.data of=/dev/null bs=1M count=128
   # < hold for a while, wait the page cache to become cold enough >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfei
     cfei 67108864 67108864 0 0 0 0 0 0
   # echo 0x1 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /cgroup/memory/test/memory.coldpgs.threshold;    \
     echo 1000000 > /cgroup/memory/test/memory.coldpgs.size
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfei
     cfei 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 32768
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 0

4) Memory cgroup scoped locked page cache reclaim

   ``/sys/kernel/mm/coldpgs/flags`` must be configured properly. Otherwise
   the locked page cache won't be reclaimed successfully.

   # < Run program to mmap 128-MB file and lock the area >
   # < hold for a while, wait the page cache to become cold enough >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfui
     cfui 0 134217728 0 0 0 0 0 0
   # echo 0x1 > /sys/kernel/mm/coldpgs/flags;                  \
     echo 0x1 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /cgroup/memory/test/memory.coldpgs.threshold;    \
     echo 1000000 > /cgroup/memory/test/memory.coldpgs.size
   # cat /cgroup/memory/test/memory.idle_page_stats | grep cfui
     cfui 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 32768
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 0

5) Global scoped anonymous page reclaim

   Similar to what we do for pagecache, global scoped anonymous page relcaim
   will be started by writing to ``/sys/kernel/mm/coldpgs/threshold``.

   # < Run test program to consume 128MB anonymous memory >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 134311936 0 0 0 0 0 0
   # echo 0x2 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /sys/kernel/mm/coldpgs/threshold
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 0
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 32791
   # cat /proc/swaps
     Filename      Type        Size     Used    Priority
     /dev/nvme1n1  partition   1048572  145152  -2

6) Global scoped locked anonymous page reclaim

   # < Run test program to consume 128MB anonymous memory and lock it >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csui
     csui 0 134311936 0 0 0 0 0 0
   # echo 0x1 > /sys/kernel/mm/coldpgs/flags;                  \
     echo 0x2 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /sys/kernel/mm/coldpgs/threshold
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csui
     csui 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 0
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 32791
   # cat /proc/swaps
     Filename      Type        Size     Used    Priority
     /dev/nvme1n1  partition   1048572  145152  -2

7) Memory cgroup scoped anonymous page reclaim

   Similar to what we do for pagecache, reclaiming anonymous pages will be
   started by writing ``memory.coldpgs.threshold`` and ``memory.coldpgs.size``.

   # < Run test program to consume 128MB anonymous memory >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 134311936 0 0 0 0 0 0
   # echo 0x2 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /cgroup/memory/test/memory.coldpgs.threshold;    \
     echo 100000 > /cgroup/memory/test/memory.coldpgs.size
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 0
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 32791
   # cat /proc/swaps
     Filename      Type        Size     Used    Priority
     /dev/nvme1n1  partition   1048572  145152  -2

8) Memory cgroup scoped locked anonymous page reclaim

   ``/sys/kernel/mm/coldpgs/flags`` must be configured correctly. Otherwise,
   the locked anonymous pages won't be reclaimed successfully.

   # < Run test program to consume 128MB anonymous memory >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csui
     csui 0 134311936 0 0 0 0 0 0
   # echo 0x1 > /sys/kernel/mm/coldpgs/flags;                  \
     echo 0x2 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /cgroup/memory/test/memory.coldpgs.threshold;    \
     echo 100000 > /cgroup/memory/test/memory.coldpgs.size
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csui
     csui 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 0
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 32791
   # cat /proc/swaps
     Filename      Type        Size     Used    Priority
     /dev/nvme1n1  partition   1048572  145152  -2

9) Memory cgroup scoped balanced anonymous page reclaim

   ``/sys/kernel/mm/coldpgs/threshold_nonrot`` must be configured correctly
   so that the anonymous pages will be partially backed by zSwap. Also, the
   zSwap should be enabled properly.

   # < Run test program to consume 128MB anonymous memory >
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 68186112 65118208 0 0 0 0 0 0
   # echo Y > /sys/module/zswap/parameters/enabled;            \
     echo 0x2 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /sys/kernel/mm/coldpgs/threshold_nonrot;         \
     echo 1 > /cgroup/memory/test/memory.coldpgs.threshold;    \
     echo 100000 > /cgroup/memory/test/memory.coldpgs.size
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 0
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 8
     anon swap out           : 32783
   # cat /proc/swaps
     Filename      Type        Size     Used    Priority
     /dev/nvme1n1  partition   1048572  145152  -2
   # cat /sys/kernel/debug/zswap/stored_pages
     8

10) Swap in anonymous pages

   Forcely swap in anonymous pages will be carried out by writing to
   ``/sys/kernel/mm/coldpgs/swapin`` or ``memory.coldpgs.swapin``.

   # < Run test program to consume 128MB anonymous memory >
   # cat /cgroup/memory/test/memory.stat | grep anon
     active_anon 134311936
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 134287360 0 0 0 0 0 0
   # echo 0x2 > /sys/kernel/mm/coldpgs/mode;                   \
     echo 1 > /cgroup/memory/test/memory.coldpgs.threshold;    \
     echo 100000 > /cgroup/memory/test/memory.coldpgs.size
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 0 0 0 0 0 0 0
   # cat /cgroup/memory/test/memory.coldpgs.stats
     pagecache migrate in    : 0
     pagecache migrate out   : 0
     pagecache dropped       : 0
     anon migrate in         : 0
     anon zswap in           : 0
     anon swap in            : 0
     anon migrate out        : 0
     anon zswap out          : 0
     anon swap out           : 32791
   # cat /proc/swaps
     Filename      Type        Size     Used    Priority
     /dev/nvme1n1  partition   1048572  131328  -2
   # echo swapin > /cgroup/memory/test/memory.coldpgs.swapin
   # cat /cgroup/memory/test/memory.idle_page_stats | grep csea
     csea 0 134287360 0 0 0 0 0 0
   # cat /proc/swaps
     Filename       Type        Size      Used    Priority
     /dev/nvme1n1   partition   1048572   256	  -2

11) Global scoped slab object reclaim
    Similar to what we do for global pagecache、anonymous page reclaim.
    We need to set ``/sys/kernel/mm/coldpgs/threshold`` and specified
    mode to work.

    # < Run test program to produce a lot of cold slab objects >
    # cat /sys/fs/cgroup/memory/memory.idle_page_stats | grep slab
      slab               5824           7280            416            416         112384     3584597744              0              0
    # echo 0x4 > /sys/kernel/mm/coldpgs/mode
    # echo 120 > sys/kernel/mm/coldpgs/threshold
    # cat /sys/fs/cgroup/memory/memory.idle_page_stats
      pagecache migrate in            :                    0 kB
      pagecache migrate out           :                    0 kB
      pagecache dropped               :                    0 kB
      anon migrate in                 :                    0 kB
      anon zswap in                   :                    0 kB
      anon swap in                    :                    0 kB
      anon migrate out                :                    0 kB
      anon zswap out                  :                    0 kB
      anon swap out                   :                    0 kB
      slab drop                       :              3099108 kB
      Total pagecache migrate in      :                    0 kB
      Total pagecache migrate out     :                    0 kB
      Total pagecache dropped         :                    0 kB
      Total anon migrate in           :                    0 kB
      Total anon zswap in             :                    0 kB
      Total anon swap in              :                    0 kB
      Total anon migrate out          :                    0 kB
      Total anon zswap out            :                    0 kB
      Total anon swap out             :                    0 kB
      Total slab drop                 :              3099108 kB
