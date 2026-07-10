.. SPDX-License-Identifier: GPL-2.0+

======
kidled
======

Introduction
============

kidled uses a kernel thread to scan the pages and slab objects on LRU list
respectively, and supports to output statistics for each memory cgroup
(process is not supported yet). Kidled scans pages round to round indexed
by pfn, but scan slab objects is different.  Slab lru list is not stable as
time goes, hence we regards the first accessed slab lru size as the real
size that kidled should scan the numbers of the specified slab in a round.
Kidled scanning will try to finish each round in a fixed duration which
is named as scan period. Of course, users can set the scan period whose
unit is seconds. Scanned objects has an attribute named as 'idle age',
which represents how long the object is kept in idle state, the age's unit
is in one scan period. The idle aging information (field) of the page consumes
one byte, which is stored in dynamically allocated array, tied with the NUMA
node or flags field of page descriptor (struct page). Meanwhile, Slab objects
use two bytes to store the information, its lower bytes to store the idle aging
information and upper bytes to make an mark to avoid accessing an object more
than one time. So the maximal age is 255. kidled eventually shows the histogram
statistics through memory cgroup files (``memory.idle_page_stats``). The statistics
could be used to evaluate the working-set size of that memory cgroup or the hierarchy.

Especially, we add a switch to control whether slab scan or not. That isolate
page scan and slab scan effectively to avoid too many slab objects interfering
with page scan. Because it is important for us to reap cold userspace page, which
reclaim more memory at the lower cost.

Note: The implementation of kidled had referred to Michel Lespinasse's patch:
https://lore.kernel.org/lkml/20110922161448.91a2e2b2.akpm@google.com/T/
Thanks for Michel Lespinasse's idea about page age and buckets!

Usage
=====

There are two sysfs files and two memory cgroup file, exported by kidled.
Here are their functions:

* ``/sys/kernel/mm/kidled/scan_period_in_seconds``

  It controls the scan period for the kernel thread to do the scanning.
  Higher resolution will be achieved with smaller value, but more CPU
  cycles will be consumed to do the scanning. The scanning won't be
  issued if 0 is set for the parameter and it's default setting. Writing
  to the file clears all statistics collected previously, even the scan
  period isn't changed.

.. note::
   A rare race exists! ``scan_period_in_seconds`` is only visible thing to
   users. duration and sequence number are internal representation for
   developers, and they'd better not be seen by users to avoid be confused.
   When user updates ``scan_period_in_seconds`` file, the sequence number
   is increased and the duration is updated synchronously, as below figure
   shows:

        OP           |       VALUE OF SCAN_PERIOD
   Initial value     | seq = 0,     duration = 0
   user update 120s  | seq = 1,     duration = 120 <---- last value kidled sees
   user update 120s  | seq = 2,     duration = 120 ---+
   ....              |                                | kidled may miss these
   ....              |                                | updates because busy
   user update 300s  | seq = 65536, duration = 300    |
   user update 300s  | seq = 0,     duration = 300 ---+
   user update 120s  | seq = 1,     duration = 120 <---- next value kidled sees

   The race happens when ``scan_period_in_seconds`` is updated very fast in a
   very short period of time and kidled misses just 65536 * N (N = 1,2,3...)
   updates and the duration keeps the same. kidled won't clear previous
   statistics, but it won't be very odd due to the duration are the same at
   least.

* ``/sys/kernel/mm/kidled/scan_target``

  It controls which type kidled will scan, there are three kinds of type
  could be selected: scan page only, scan slab only, scan both page and
  slab. The users can enable them as follows. Other value will be invalid.

  To scan user page only
        echo 1 > ``/sys/kernel/mm/kidled/scan_target``
  To scan slab only
        echo 2 > ``/sys/kernel/mm/kidled/scan_target``
  Both scan page and slab
        echo 3 > ``/sys/kernel/mm/kidled/scan_target``

  By default, kidled will not scan slab because the cpu load will very
  high if the system has a lot of reclaimable slabs. But we need to enable
  it when userspace pages have been reclaimed and a lot of reclaimable
  slabs is in the system. We'd better mark and reclaim the cold slab in
  front of the memory reclaim triggered by allocating memory request.


  It shows histogram of idle statistics for the corresponding memory cgroup.

* ``memory.idle_page_stats`` (memory cgroup v1/v2)

  It shows histogram of accumulated idle statistics for the corresponding
  memory cgroup.

  ``memory.idle_page_stats.local`` and ``memory.idle_page_stats`` share the
  same output format, as shown below.

  ----------------------------- snapshot start -----------------------------
  # version: 1.0
  # page_scans: 92
  # slab_scans: 92
  # scan_period_in_seconds: 120
  # buckets: 1,2,5,15,30,60,120,240
  #
  #   _-----=> clean/dirty
  #  / _----=> swap/file
  # | / _---=> evict/unevict
  # || / _--=> inactive/active
  # ||| / _-=> slab
  # |||| /
  # |||||              [1,2)          [2,5)         [5,15)        [15,30)        [30,60)       [60,120)      [120,240)     [240,+inf)
    csei                  0              0              0              0              0              0              0              0
    dsei                  0          16384              0              0              0         360448              0              0
    cfei             774144        3624960        1744896        1298432       20676608      161087488              0              0
    dfei                  0              0          16384              0          24576              0              0              0
    csui                  0              0              0              0              0              0              0              0
    dsui                  0              0              0              0              0              0              0              0
    cfui                  0              0              0              0              0              0              0              0
    dfui                  0              0              0              0              0              0              0              0
    csea             278528        3510272         389120         872448         806912       22716416              0              0
    dsea                  0          12288              0              0              0         196608              0              0
    cfea            1298432       12115968        3510272       10518528       78409728     1503793152              0              0
    dfea                  0              0              0              0              0           4096              0              0
    csua                  0              0              0              0              0              0              0              0
    dsua                  0              0              0              0              0              0              0              0
    cfua                  0              0              0              0              0              0              0              0
    dfua                  0              0              0              0              0              0              0              0
    slab               2704            832          15600          20800          70720      763819160              0              0
  ----------------------------- snapshot end -----------------------------

  ``page_scans`` means how many rounds current cgroup's pagecache has been scanned.
  ``slab_scans`` means how many rounds current cgroup's slab has been scanned.
  ``scan_period_in_seconds`` means kidled will take how long to finish
  one round. ``buckets`` is to allow scripts parsing easily. The table
  shows how many bytes are in idle state, the row is indexed by idle
  type and column is indexed by idle ages.

  e.g. it shows 331776 bytes are idle at column ``[2,5)`` and row ``csea``,
  ``csea`` means the pages are clean && swappable && evictable && active,
  ``[2,5)`` means pages keep idle at least 240 seconds and less than 600
  seconds (get them by [2, 5) * scan_period_in_seconds). The last column
  ``[240,+inf)`` means pages keep idle for a long time, greater than 28800
  seconds.

  Each memory cgroup can have its own histogram sampling different from
  others by echo a monotonically increasing array to either
  ``memory.idle_page_stats.local`` or ``memory.idle_page_stats``, each number
  should be less than 256 and the write operation will clear previous stats
  even buckets have not been changed. The number of bucket values must be
  less or equal than 8. The default setting is "1,2,5,15,30,60,120,240".
  Null bucket values (i.e. a null string) means no need account to current
  memcg (NOTE it will still account to parent memcg if parent memcg exists
  and has non-null buckets), non-accounting's snapshot looks like below:

  ----------------------------- snapshot start -----------------------------
  $ sudo bash -c "echo '' > /sys/fs/cgroup/memory/test/memory.idle_page_stats"
  $ cat /sys/fs/cgroup/memory/test/memory.idle_page_stats
  # version: 1.0
  # page_scans: 0
  # slab_scans: 0
  # scan_period_in_seconds: 1
  # buckets: no valid bucket available
  ----------------------------- snapshot end -----------------------------
