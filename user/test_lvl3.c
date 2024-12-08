/*
 Handle the case where a process terminates accidently (faults). 
  Release any mutexes it holds, and ensure that it is no longer blocked on conditions.
*/

