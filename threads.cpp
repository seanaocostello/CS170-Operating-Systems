#include<stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>
#include <semaphore.h>
#include <queue>

using namespace std;

int numThreads = 0;

//pthread scheduler [128];                                                                               
int maxThread = 0;
int currentThread = 0;

struct pthread{
  pthread_t id;
  int status;
  int sem;
  char * memoryLocation;
  jmp_buf state;
  void * (*start)(void*);
  void * args;
  void * returnValue;
  void * joinValue;
};

struct semaphore{
  int count;
  queue <int> * blocked = new queue<int>;
};

//static stack <int> * ids = new stack<int>;                                                             

//static int a3 = 0;                                                                                     
//static int b3 = 0;                                                                                     
static pthread scheduler [129];
static semaphore sems [65536];
static int countSems = 0;
static struct sigaction sig;
static long int i64_ptr_mangle(long int p)
{
  long int ret;
  asm(" mov %1, %%rax;\n"
      " xor %%fs:0x30, %%rax;"
      " rol $0x11, %%rax;"
      " mov %%rax, %0;"
      : "=r"(ret)
      : "r"(p)
      : "%rax"
      );
  return ret;
}

int findNextThread(){
  int i = currentThread+1;
  i %= maxThread+1;
  while (scheduler[i].status >= -1 || scheduler[i].sem == 1){
    if (scheduler[i].status >= 0){
      cout << scheduler[i].status << scheduler[scheduler[i].status].status << endl;
      if (scheduler[scheduler[i].status].status == -1){
        scheduler[i].status = -2;
        cout << "Join: " << i << endl;
        return i;
      }
    }
    //cout << scheduler[i].status << scheduler[i].sem << endl;                                           
    i++;
    if (maxThread){
    	i %= maxThread+1;
    }
    else{
      i %= maxThread+1;
    }
  }
  return i;
}

void lock(){
  sigemptyset(&sig.sa_mask);
  sigset_t signal;
  sigemptyset(&signal);
  sigaddset(&signal,SIGALRM);
  sigprocmask(SIG_BLOCK,&signal,&sig.sa_mask);
}

void unlock(){
  sigemptyset(&sig.sa_mask);
  sigset_t signal;
  sigemptyset(&signal);
  sigaddset(&signal,SIGALRM);
  sigprocmask(SIG_UNBLOCK,&signal,&sig.sa_mask);
}

void alarm_handler(int){
  cout << currentThread << endl;
  lock();
  int v = setjmp(scheduler[currentThread].state);
  if (!v){
    currentThread = findNextThread();
    unlock();
    longjmp(scheduler[currentThread].state,1);
  }
  else{
    unlock();
    cout << "LLL" << endl;
    //ualarm(50000);                                                                                     
  }
}

void init(){
	for (int i = 0; i < 129; i++){
    scheduler[i].id = i;
    scheduler[i].status = -1;
    scheduler[i].sem = -1;
  }
  scheduler[0].memoryLocation = 0;
  scheduler[0].status = -2;
  numThreads = 1;
  maxThread = 1;
  sig.sa_handler = &alarm_handler;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags = SA_NODEFER;
  sigaction(SIGALRM, &sig, 0);
  ualarm(50000, 50000);
}

int create_thread(){
  void * val = scheduler[currentThread].start(scheduler[currentThread].args);
  pthread_exit(val);
}

int pthread_create(pthread_t *thread,  const pthread_attr_t * attr, void *(*start_routine)(void*), void \
* arg){
  if (!numThreads){
    init();
}
  if (numThreads < 129){
    int newThreadId = 0;
    for (int i = currentThread; i <= maxThread+1; i++){
      if (scheduler[i].status == -1 && !newThreadId){
        newThreadId = i;
        if (newThreadId == maxThread+1){
          maxThread = newThreadId;
        }
      }
    }
    lock();
    *thread = newThreadId;
    char * s = (char *)malloc(32767*sizeof(char));
    /*    if (scheduler[newThreadId].memoryLocation){                                                    
      free(scheduler[newThreadId].memoryLocation);                                                       
      }*/
    scheduler[newThreadId].memoryLocation = s;
    int val = setjmp(scheduler[newThreadId].state);
    if (!val){
      *((long int*)&(scheduler[newThreadId].state)+6) = i64_ptr_mangle((long int)s);
      *((long int*)&(scheduler[newThreadId].state)+7) = i64_ptr_mangle((long int)create_thread);
      scheduler[newThreadId].start = start_routine;
      scheduler[newThreadId].args = arg;
      return scheduler[currentThread].id;
}

int pthread_join(pthread_t thread, void ** value_ptr){
  if (scheduler[thread].status == -1){ return ESRCH;}
  if (thread == currentThread){return EDEADLK;}
  lock();
  scheduler[currentThread].status = thread;
  unlock();
  alarm_handler(0);
  if (value_ptr){*value_ptr = scheduler[thread].returnValue;}
  return 0;
}

void pthread_exit(void * value_ptr){
  lock();
  scheduler[currentThread].returnValue = value_ptr;
  void * n = scheduler[currentThread].memoryLocation;
  //free(n);                                                                                             
  scheduler[currentThread].memoryLocation = 0;
  scheduler[currentThread].status = -1;
  numThreads--;
  if (currentThread == maxThread){
    while (scheduler[maxThread].status == -1 && numThreads != 0){
    	maxThread--;
    }
  }
  currentThread = findNextThread();
  unlock();
  //cout << "apples" << endl;                                                                            
  if (numThreads == 0){
    exit(0);
  }
  else{
    longjmp(scheduler[currentThread].state,1);
  }
  exit(0);
}

int sem_init(sem_t * sem, int pshared, unsigned int value){
  if (value > 65536){
    return -1;
  }
  if (numThreads == 0){init();}
  lock();
  sems[countSems].count = value;
  //sems[countSems].blocked = new queue <int>;                                                           
  sem->__align = (long int)countSems;
  countSems++;
  unlock();
  return 1;
}

int sem_destroy(sem_t * sem){
  long int s = sem->__align;
  while (!sems[s].blocked->empty()){sems[s].blocked->pop();}
  sems[s].count = 0;
  sem->__align = (long int)NULL;
  sem = NULL;
  return 1;
}

int sem_wait(sem_t * sem){
  long int s = sem->__align;
  if (sems[s].count > 0){
    sems[s].count--;
    return 0;
  }
  else{
    lock();
    sems[s].blocked->push(currentThread);
    scheduler[currentThread].sem = 1;
    unlock();
    alarm_handler(0);
    sems[s].count--;
    return 0;
  }
}

int sem_post(sem_t * sem){
  long int s = sem->__align;
  lock();
  sems[s].count++;
  if (!sems[s].blocked->empty()){
    int t = sems[s].blocked->front();
    cout << "t: " << t << endl;
    sems[s].blocked->pop();
    scheduler[t].sem = -1;
  }
  unlock();
  return 0;
}