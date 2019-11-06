#include<stdlib.h>
#include <sys/types.h>
#include <pthread.h>
#include <setjmp.h>
#include <unistd.h>
#include <signal.h>
#include <iostream>


using namespace std;

int numThreads = 0;

//pthread scheduler [128];                                                                               
int maxThread = 0;
int currentThread = 0;

struct pthread{
  pthread_t id;
  int status;
  char * memoryLocation;
  jmp_buf state;
  void * (*start)(void*);
  void * args;
};

//static stack <int> * ids = new stack<int>;                                                             

//static int a3 = 0;                                                                                     
//static int b3 = 0;                                                                                     
static pthread scheduler [129];

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
  while (!scheduler[i].status){
    i++;
    if (maxThread){
      i %= maxThread;
    }
    else{
      i %= maxThread+1;
    }
  }
  return i;
}

void alarm_handler(int){
  int v = setjmp(scheduler[currentThread].state);
  //int v = setjmp(scheduler[currentThread].state);                                                      
  if (!v){
    currentThread = findNextThread();
    cout << currentThread << endl;
    longjmp(scheduler[currentThread].state,1);
  }
  else{
    cout << "Back it up: " <<currentThread << endl;
  }
 }

void init(){
  for (int i = 0; i < 129; i++){
    scheduler[i].id = i;
    scheduler[i].status = 0;
  }
  scheduler[0].memoryLocation = 0;
  scheduler[0].status = 1;
  numThreads = 1;
  maxThread = 1;
  struct sigaction sig;
  sig.sa_handler = &alarm_handler;
  sigemptyset(&sig.sa_mask);
  sig.sa_flags = SA_NODEFER;
  sigaction(SIGALRM, &sig, 0);
  ualarm(50000, 50000);
}

int create_thread(){
  scheduler[currentThread].start(scheduler[currentThread].args);
  pthread_exit(0);
}

int pthread_create(pthread_t *thread,  const pthread_attr_t * attr, void *(*start_routine)(void*), void \
* arg){
  if (!numThreads){
    init();
  }
  if (numThreads < 129){
    int newThreadId = 0;
    for (int i = currentThread; i <= maxThread+1; i++){
      if (!scheduler[i].status && !newThreadId){
        newThreadId = i;
        if (newThreadId == maxThread+1){
          maxThread = newThreadId;
        }
      }
    }
    *thread = newThreadId;
    char * s = (char *)malloc(32767*sizeof(char));
    scheduler[newThreadId].memoryLocation = s;
    int val = setjmp(scheduler[newThreadId].state);
    if (!val){
      *((long int*)&(scheduler[newThreadId].state)+6) = i64_ptr_mangle((long int)s);
      *((long int*)&(scheduler[newThreadId].state)+7) = i64_ptr_mangle((long int)create_thread);
      scheduler[newThreadId].start = start_routine;
      scheduler[newThreadId].args = arg;
      scheduler[newThreadId].status = 1;
      numThreads++;
    }
    return 0;
  }
  return -1;
}

pthread_t pthread_self(void){
  return scheduler[currentThread].id;
}

void pthread_exit(void * value_ptr){
  cout << currentThread << endl;
  void * n = scheduler[currentThread].memoryLocation;
  free(n);
  scheduler[currentThread].memoryLocation = 0;
  scheduler[currentThread].status = 0;
  numThreads--;
  if (currentThread == maxThread){
    maxThread--;
  }
  currentThread = findNextThread();
  //cout << numThreads << endl;                                                                          
  if (numThreads == 0){
    exit(0);
  }
  else{
    longjmp(scheduler[currentThread].state,1);
  }
  exit(0);
}

static int a4;
static void* _thread_inc4(void* arg){
    for(int i = 0; i < 10; i++){
        a4++;
    }
    pthread_exit(0);
}

int main(){

   pthread_t tid1; pthread_t tid2;

   pthread_create(&tid1, NULL,  &_thread_inc4, NULL);
   pthread_create(&tid2, NULL,  &_thread_inc4, NULL);


   for(int i = 0; i < 10; i++){
     a4++;
   }

   while(a4 != 30);

   cout << "Apples" << endl;
}