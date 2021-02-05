# selvC
A simple IPC problem written in C.

## Description
A tribe of N savages eats in common from a pot that can hold up to M servings of
stew, it is assumed that the pot is initially full. When a savage is hungry he checks the pot:

i) if there are no portions, he wakes up the cook and waits until he has completely refilled the pot before serving himself;
ii) if the pot contains at least a portion, he appropriates it.

The cook checks that there are portions and, if there are, he falls asleep, otherwise he cooks M portions
and he puts them in the pot. Each savage must eat NGIRI times before finishing.
The number of n savages, of M portions and NGIRI should be requested as arguments from the line of
command to facilitate experiments when these parameters vary. The savages and the cook must be
implemented as separate processes working on shared variables and semaphores. Also define
a parameter that allows to count overall how many times the cook fills the pot before
the end of the program. The program ends when all the savages have completed theirs
cycle. During execution, print appropriate messages in order to determine the behavior of the
various processes.
