Tests run on Enigma (Ubuntu 19.04) - hardware listed in source file

| Parameters     | Sequential | Wait On All | Spinning | Semaphore |
| -------------- | ---------- | ----------- | -------- | --------- |
| 100M, 2, 50M+1 | 160ms      | 160ms       |  11ms    |   0ms     |
| 100M, 4, 75M+1 | 250ms      | 238ms       |  18ms    |   0ms     |
| 100M, 8, 88M   | 278ms      | 280ms       |  39ms    |  10ms     |
| 100M, 2, -1    | 317ms      | 317ms       | 477ms    | 319ms     |
| 100M, 4, -1    | 312ms      | 315ms       | 397ms    | 315ms     |
| 100M, 8, -1    | 336ms      | 328ms       | 366ms    | 321ms     |
--------------------------------------------------------------------

My tests seem to indicate that on this machine, multi-threading had no positive impact on performance unless threads
could exit early and stop computation. In fact, multi-threading may have had a negative impact on the tests that could
not exit early - however since all the results for tests with no zero were within a margin of error it's hard to be sure
of this.
This was initially surprising to me, until I realized that the machine I tested on has a single CPU with a single core
and a single thread. Therefore there could be no true parallel processing on this machine.
Using semaphores vs spin-locking has a clear performance advantage in this particular problem. Results with a zero
placed at the start of one of the thread regions consistently resulted in a 0ms search time for the semaphore search,
whereas spin-locking added between 10 and 40ms of wasted time.
