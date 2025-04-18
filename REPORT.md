# xv6 Containers Final Report

...

###
Test Files are in the integration Branch for Module #1,2,3
###


###
Test Files are in the main branch for Module #4
###

# xv6 Shared Memory Final Report
Levels 0, 1, 2, 3 were completed and tested.
**All these levels were done by Dane Van Horn.**
- lvl 0: Testing is done implicity in shmem_test files
- lvl 1: Testing is included in shmem_test1. 
  - The first test ensures that a parent and child can seperately call and use shm_get to communicate, showing shared memory exists between seperate processes.
- lvl 2: Testing is included in shmem_test2.
  - The first test tests that a shared memory region mapped before calling fork will allow a parent and child to communicate; a parent writes as a child briefly sleeps, then the child reads. 
  - The second test tests that three shared memory regions will pass a test similar to the one below, comparing three unique texts copied over shared memory. **This also tests level three, as exit is called and the shmem is unmapped.** There is a short initial test to ensure that memory is blank between the sleep in the child and the wait in the parents, explained as memory being blank before the child writes.
  - The third test, which might actually belond in lvl3, tests that memory can be created up to the max number of shared_memory, and that creating another shmem will not map memory over. 
  - The second part of the third test involves Then, it goes through and unmaps every other chunk of memory and then just makes sure that it can read from those that should be read from and can't from those that are deallocated.
- lvl 3: Testing is included in shmem_test3.
  - NOTE: SHM_REM returns 1 if it is the last process to process to remove reference to the memory, and then deallocs. 0 is if it isn't the last, nothing will be deallocated
  - The first test is just a control that makes sure that shmem will not dealloc or return one if at least one process still holds it.
  - The second test tests that memory is deallocated (1 is returned) after the child exits, and the parent removes the memory.
  - The third test does a similar thing, just with exec.
  - The fourth test tests that undeclared memory will return an error (-1) when free a chunk of memory that never was shm_got.
  - Fifth test tests that a first and third chunk of memory can be used when the second test is freed. It makes sure the children are blank before reads. Then, it just makes sure the process continues (no kernel trap).
  - The sixth test tests a double free across a forked process, making sure it's freed and then returns an error when nothing is remaining. 
  - The final test makes sure if a process calls the same char* of memory twice, it returns the same address.

#  xv6 Synchronization Final Report
**lvl 0,1,2,3 WERE DONE BY ROHAN MUMMALANENI**

-lvl0 testing done in test_lvl0 testfile. Makes sure that you can't lock twice, unlock twice or lock something else

-lvl1 testing done in test_lvl1 testfile. Forks process, tests
the lock and cv_wait and cv_signal to make sure that you cannot overwrite a signal by just locking immediately

-lvl2 testing done in test_lvl2. Has condition variables which wait on condition variables. Once the condition variable is locked another condition variable waits till it exits and reaches the parent process then it looks and signals to unlock. Happens 5 different times, meaning that you ahve 5 parents and 5 children.

-lvl3 testing done in test_lvl3. What I did was simulate a fault where I exited and killed a child process while the mutex was locked. I then locked that mutex proving that the mutex was in fact able to be locked and was released upon termination.

# xv6 Scheduling Final Report
**lvl 0,1,2,3 were done by Thomas (Tommy) Gillespie**

- lvl0 testing is done with sched_lvl0. This attempts to set a priority using the prio_set syscall. This will fail as it attempts to set the priority of the current process to 0, which it cannot as it is >= the priority it already has.
  
- lvl1 testing is done wtih sched_lvl1. This will take the process with pid=1 and set it to the max priority 7. Another syscall "prio_get" will get its priority to confirm
  
- lvl2 testing is done with sched_lvl2. This test will set the priority of the current process to the max priority. Then, a loop checks this priority 100 times, making sure that it is the same and that there is no demotion or promotion, as it should be fixed.
  
- lvl3 testing is done with sched_lvl3. This test will get the current pid and attempt to set its priority to zero. It will then check on the priority has it traverses the HFS structure, demoting its priority until it reaches the highest (value) priority of 7.
  
#  xv6 Container Manager Final Report
**lvl 0,1,2,3 were all done by Malik Gomez.**

### Level 0

Level 0 has 8 tests. 

The first two tests parse two different JSON files. They call **read_file(const char * filename)** which simply opens the file, reads it, and returns a **char *buffer** which contains the contents of the JSON file. Afterwards, **parse_json(const char *js)** is called in order to parse the content. It utilizes a the JMSN implementation to tokenize the content into an expected 7 tokens. The token values are stored into an array of strings which is returned as a **`char **token_values`**. The 2nd, 4th, and 6th token are stored as the init, root, and maxproc specs for the container and checked to make sure they are valid. For these two tests, token 0 is simply printed out since it is the entire JSON content.

The following 5 tests simply use conditionals and JSMN to make sure that the maxproc doesn't exceed the maximum or subceed the minimum number of processes allowed for a container, there are exactly 7 JSON tokens, and checks for invalid JSON formatting/characters.

The final test is an integration test of the Container Manager, Shared Memory, and Synchronization. It simulates a simplfied interaction between the container manager and dockv6. The test uses two **fork()** calls in order to **exec()** a **cm.c** and **dockv6.c** simulation called **test_cm.c** and **test_dockv6.c** in the parent and child. These test files are simplified versions of the actual **cm.c** and **dockv6.c** and emulate the container manager running continously waiting for dockv6 to send it JSON specs. The container manager first uses **shm_get(SHARED_MEMORY)**  in order to get a page of memory and **mutex_create(SHARED_MEMORY)** in order to get a mutex id. It then calls **fork()** and has the child run in **while(1)**. Inside this loop, the 3 crucial lines are as follows:

<p align="center">
mutex_lock(mutex);
char * json_name = shm_get(SHARED_MEMORY);
mutex_unlock(mutex);
</p>

Before accessing memory, the process makes sure to lock to ensure no other processes can write to this memory. The first byte is a flag bit and if it isn't 1, then the process goes to sleep, and does this again. Meanwhile, dockv6 calls the same **shm_get(SHARED_MEMORY)** in order to get the same memory as the container manager. Here, it sets the memory to **"example.json"** and sets the flag bit to 1 to tell the container manager it is ready to be read. If successful, container manager will see the newly written data in the memory. 

Level 1 has 29 tests.

The first 3 tests take place in container 0. It changes the root to be c0 and then attempts to run files outside of the container using a relative path. The first time it does not use **cd ..** and the second time it does. It then exits the container.

The next 7 tests are done in container 1. It changes the root to c1 and then runs a file that is in the container. Afterwards, it attempts to run a file that is inside the container but not outside the container after **cd ..**. Afterwards, the program forks and attempts to run that same file. This should succeed since forked processes in a container should still share the same root. Next, the test attempts to change the root to a new container 'c2' which should not succeed. The next test attempts to create a file using an absolute path while the test after that attempts to create a file using a relative path. They should fail and pass respectively. These all ensure that **`int cm_setroot(char *path, int path_length)`**indeed changed the root of the system through **`static struct inode *namex(char *path, int nameiparent, char *name)`**It then exists the container.

The next 7 tests are done in container 2. c2 attempts to access a file in c2 and fails. It creates a file with the same name as the file in c1 and checks to make sure the content is different. The program then does the same things it did in c1. It then exists the container.

The next 2 tests attempt to change the root to invalid roots. Afterwards, we create a container c3, get our shared memory and mutex, mark our memory, and exit the container.

The next 3 test are in container 4. One test grabs shared memoery using the same name that c3 used. It should return different shared memory since containers have their own shared memeory. Next, it checks to make sure c3 and c4 mutexes are unique as well. 

The final tests are in container 5. We simply try to run any files from previous containers as well as use system calls such as **link** and **mkdir** to make sure our root change is real.

Level 2 has 7 tests. 

The first test attempts to set the max number of processes outside of a container using **cm_maxproc()**. Afterwards, we enter a container and call **max_proc()** twice. The first time should succeed while the second should not. Then, we **fork()** twice so that the second **fork()** is called at the process limit to make sure **fork()** fails. The first **fork()** is used to verify that **cm_maxproc()** cannot be called in any subprocesses. Lastly, we call **exec()** at the process limit which should not affect the total process count.

Level 3 has 1 test and a live demonstration.

The CMlvl3.c file simply sets up two unique text files for each container. 
The user can input the following into the terminal:

$
$ CMlvl3
$ cm &
$ dockv6 create demo1.json

After files are copied, the user has multiple programs to play with:
c_write: writes to shared memory
c_read: reads from shared memory
demo1.txt: can use **cat** to get text from file
mkdir: make directories that can only be accessed in that container
cat: read text files
forkbomb: run multiple times until max processes is reached
exit_container: allows the user to exit the container
c_mutex: returns a mutex lock

$ dockv6 create demo2.json

The user will have the same programs in the demo2 container. They will notice that the shared memoery and mutex are unique despite having the same name due to the fact that containers have their own. 


![an example run](image.png)
