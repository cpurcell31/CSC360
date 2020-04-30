### Multithreading Test Program - Train Scheduling Simulation

mts.c is a simple multithreading test program that simulates train scheduling using the pthread library.
Trains are initialized by a controller thread and must follow a list of rules to determine their ordering.
Trains have priority, load time, direction and traversal time that will determine their schedule.

#### Train Schedule Rule Set

1. Only one train is on the main track at any given time.
2. Only loaded trains can cross the main track.
3. If there are multiple loaded trains, the one with the high priority crosses.
4. If two loaded trains have the same priority, then:

  (a) If they are both traveling in the same direction, the train which finished loading first gets the clearance
  to cross first. If they finished loading at the same time, the one appeared first in the input file gets the
  clearance to cross first.
  
  (b) If they are traveling in opposite directions, pick the train which will travel in the direction opposite of
  which the last train to cross the main track traveled. If no trains have crossed the main track yet, the
  Eastbound train has the priority.
  
  (c) To avoid starvation, if there are three trains in the same direction traveled through the main track back
  to back, the trains waiting in the opposite direction get a chance to dispatch one train if any.

data.txt shows a sample input file for mts.c
