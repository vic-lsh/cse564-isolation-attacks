This attack tests contention on physical memory.

use\_mem.sh is a script that uses some amount of memory for some amount of time.
use\_all\_mem.sh uses 4/5 of available memory (according to `free`) and runs with $1 number of processes. It splits the available memory among all of the processes.

The victim runs use\_all\_mem.sh with 100 processes.
The attacker runs use\_all\_mem.sh with 2000 processes to try and increase contention on some sort of physical memory lock.

Thus, when they run together, the victim's `time` output should be longer if the attacker affects it.
