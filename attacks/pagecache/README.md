TODO: rewrite attack to use tmpfs, so that we know the issue isn't contention from the disk

This attack tests pagecache contention.

The victim loads a file twice to use the whole pagecache.
The attacker clobbers the pagecache.
When they run together, the victim's second load should run slower.
