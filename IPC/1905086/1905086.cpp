#include <iostream>
#include <vector>
#include <fstream>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <random>
#include <chrono>

std::fstream fin("input.txt");
std::fstream fout("output.txt");

#define READY 0
#define STANDING 1
#define USING 2
#define LEAVING 3

using namespace std;

vector<sem_t> student_semaphore;
pthread_mutex_t lock,db;
time_t start, endt;
int printer_station[5],binder_station[3],report_per_grp[100];
int n, m, pTime, bTime, lTime,rc,submission_count;
class info_of_students
{
private:
   int id;
   int group_id;
   int printer_id;
   int bind_station_id;
   int state;
   int delay;
   int getPoissionValue()
   {
      random_device randDev;
      mt19937 gen(randDev());
      poisson_distribution<int> distribution(10);
      return distribution(gen);
   }

public:
   info_of_students(int id)
   {
      this->id = id;
      this->group_id = (id - 1) / m + 1;        // 1->1 2->1 3->1 4->1 5->1 6->2
      this->printer_id = id % 4 + 1;            // 1->1 2->2 3->3 4->4 5->1
      this->state = READY;
      this->delay = getPoissionValue();
      this->bind_station_id = -1;
   }
   void setState(int state)
   {
      this->state = state;
   }
   int getState()
   {
      return this->state;
   }
   int getId()
   {
      return this->id;
   }
   int getGroupId()
   {
      return this->group_id;
   }
   int getPrinterId()
   {
      return this->printer_id;
   }
   int getDelay()
   {
      return this->delay;
   }
   bool isLeader()
   {
      return (this->id == (this->group_id) * m);
   }
   void setBindId(int id){
      this->bind_station_id = id;
   }
   int getBindId(){
      return this->bind_station_id;
   }
};
vector<info_of_students*> students;

void test(info_of_students* stinfo){

   if(stinfo->getState() == STANDING && printer_station[stinfo->getPrinterId()] == 0){
      stinfo->setState(USING);
      printer_station[stinfo->getPrinterId()] = 1;
      sem_post(&student_semaphore[stinfo->getId()]);
   }
}
void acquirePrinter(info_of_students *stinfo) {
   pthread_mutex_lock(&lock);
   time(&endt);
   fout << "Student " << stinfo->getId() << " has arrived at the print station at time " <<(endt-start) << endl;
   stinfo->setState(STANDING);
   test(stinfo);
   pthread_mutex_unlock(&lock);
   sem_wait(&student_semaphore[stinfo->getId()]);
}

void evaluateGroupPrints(info_of_students *stinfo) {
   int group = stinfo->getGroupId();
   int total = ++report_per_grp[group];
   if (total == m) {
      sem_post(&student_semaphore[group*m]);
   }
}
void test1(info_of_students* stinfo){
   if(stinfo->getState() == STANDING){
      if((binder_station[1] == 0 || binder_station[2] == 0)){
         stinfo->setState(USING);
         sem_post(&student_semaphore[stinfo->getId()]);
      }
   }
}

void writer(info_of_students* stinfo){
   pthread_mutex_lock(&db);
   time(&endt);
   fout << "Group " << stinfo->getGroupId() << " has submitted the report at time " << (endt-start) << endl;
   submission_count++;
   pthread_mutex_unlock(&db);
}

void acquireBindingStation(info_of_students *stinfo){
   pthread_mutex_lock(&lock);
   stinfo->setState(STANDING);
   test1(stinfo);
   pthread_mutex_unlock(&lock);
   sem_wait(&student_semaphore[stinfo->getId()]);
   pthread_mutex_lock(&lock);
   time(&endt);
   fout << "Group " << stinfo->getGroupId() << " has started binding at time " << (endt-start) << endl;
   int bId = -1;
   if(binder_station[1] == 0){
      bId = 1;
   }
   else if(binder_station[2] == 0){
      bId = 2;
   }
   binder_station[bId] = 1;
   stinfo->setBindId(bId);
   pthread_mutex_unlock(&lock);
   sleep(bTime);
   pthread_mutex_lock(&lock);
   time(&endt);
   fout << "Group " << stinfo->getGroupId() << " has finished binding at time " << (endt-start) << endl;
   binder_station[stinfo->getBindId()] = 0;
   stinfo->setState(LEAVING);
   for(int i = 1; i<= n/m; i++){
       if(stinfo->getState() == STANDING){
         sem_post(&student_semaphore[i*m]);
       }
   }
   pthread_mutex_unlock(&lock);
   writer(stinfo);
}

void releasePrinter(info_of_students *stinfo){
   pthread_mutex_lock(&lock);
   time(&endt);

   fout << "Student " << stinfo->getId() <<  " has finished printing at time " << (endt-start) << endl;

   stinfo->setState(LEAVING);
   printer_station[stinfo->getPrinterId()] = 0;

   int gId = stinfo->getGroupId();
   int mem_init = gId*m - m + 1;
   int mem_last = gId*m;

   for(int i = mem_init; i <= mem_last; i++ ){
      if(stinfo->getPrinterId() == students[i]->getPrinterId()){
         test(students[i]);
      }
   }

   for(int i = 1; i <= n; i++){
      if(i>=mem_init && i <= mem_last) continue;
      if(stinfo->getPrinterId() == students[i]->getPrinterId()){
         test(students[i]);
      }
   }
   evaluateGroupPrints(stinfo);

   pthread_mutex_unlock(&lock);

   if (stinfo->isLeader() == true) {
      sem_wait(&student_semaphore[stinfo->getId()]);
      acquireBindingStation(stinfo);
   }
}

void* shrd_function(void *args){
   info_of_students *stinfo = (info_of_students*) args;

   sleep(stinfo->getDelay());
   acquirePrinter(stinfo);
   sleep(pTime);
   releasePrinter(stinfo);

   return NULL;
}
int getStaffDelay(){
   random_device randDev;
   mt19937 gen(randDev());
   poisson_distribution<int> distribution(10);
   return distribution(gen);
}
void reader(int staffID){
   while(true){
      if(submission_count == (n/m)){
         return;
      }
      sleep(getStaffDelay());
      pthread_mutex_lock(&lock);
      rc++;
      if(rc == 1) pthread_mutex_lock(&db);
      time(&endt);
      fout << "Staff " << staffID << " has started reading the entry book at time " << (endt-start) << ". No. of submission = " << submission_count << endl;
      pthread_mutex_unlock(&lock);
      sleep(lTime);

      pthread_mutex_lock(&lock);
      rc--;
      if(rc == 0)pthread_mutex_unlock(&db);

      pthread_mutex_unlock(&lock);
   }
}
void* shrd_function1(void *args){
   int staffID =  *(int*) args;
   reader(staffID);
   return NULL;
}

int main()
{
   int x = 1;
   int y = 2;
   printer_station[0]= binder_station[0] = 1;
   time(&start);
   fin >> n >> m;
   fin >> pTime >> bTime >> lTime;
   pthread_t sTh[n],lab_staff[2];
   pthread_mutex_init(&lock, 0);
   pthread_mutex_init(&db,0);
   student_semaphore.push_back(*(new sem_t));
   for (int i = 0; i < n; i++){
      sem_t* temp = new sem_t;
      sem_init(temp,0,0);
      student_semaphore.push_back(*temp);
   }
   students.push_back(new info_of_students(0));
   for(int i = 0; i < n; i++){
      info_of_students *st = new info_of_students(i+1);
      students.push_back(st);
      pthread_create(&sTh[i],NULL,&shrd_function,(void*)(students[i+1]));
   }

   pthread_create(&lab_staff[0],NULL,&shrd_function1,(void*)(&x));
   pthread_create(&lab_staff[1],NULL,&shrd_function1,(void*)(&y));
   
   for(int i = 0; i < n; i++){
      pthread_join(sTh[i], NULL);
   }
   for(int i = 0; i < 2; i++){
      pthread_join(lab_staff[i], NULL);
   }
   pthread_mutex_destroy(&lock);
   for(int i = 1; i <= n; i++){
      sem_destroy(&student_semaphore[i]);
   }
   fin.close();
   fout.close();
   return 0;
}