

/* run using: ./load_gen localhost <server port> <number of concurrent users>
   <think time (in s)> <test duration (in s)> */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <cstring>
#include <netdb.h>
#include <pthread.h>
#include <sys/time.h>
#include <iostream>
#include <sstream>
#include <string>
using namespace std;
int time_up;
 char http_request[25000][25000];
FILE *log_file;  
 char buffer[4096];

// user info struct
struct user_info {
  // user id
  int id;

  // socket info
  int portno;
  char *hostname;
  float think_time;

  // user metrics
  int total_count;
  float total_rtt;
};

// error handling function
void error(char *msg) {
  perror(msg);
  //exit(1);
  
}

// time diff in seconds
float time_diff(struct timeval *t2, struct timeval *t1) {
  return (t2->tv_sec - t1->tv_sec) + (t2->tv_usec - t1->tv_usec) / 1e6;
}

// user thread function
void *user_function(void *arg) {
  /* get user info */
  struct user_info *info = (struct user_info *)arg;
  string m;
  int sockfd, n, x=0;
  int total=0;
  
  struct timeval start, end;
 int tid=(*info).id;
  struct sockaddr_in serv_addr;
  struct hostent *server;
  info->total_count=0;

  int l=m.length();
  char m1[l+1];

  while (1) {
    /* start timer */
    gettimeofday(&start, NULL);

    /* TODO: create socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
       m="ERROR opening socket";
       
       strcpy(m1, m.c_str());
        error(m1);
        continue;
      }


    /* TODO: set server attrs */
    bzero((char *)&serv_addr, sizeof(serv_addr));
 int portno = info->portno;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

    /* TODO: connect to server */
    if(connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0)
    {
       m="ERROR connecting";
       strcpy(m1, m.c_str());
        error(m1);
        close(sockfd);
        continue;
      }

    /* TODO: send message to server */
    
    n=write(sockfd,http_request[tid],strlen(http_request[tid]));
     if (n <= 0)
     {
     m="ERROR writing to socket";
      strcpy(m1, m.c_str());
        error(m1);
        close(sockfd);
      }
      bzero(buffer, 4096);    

    /* TODO: read reply from server */
     n=read(sockfd, buffer, 4096);
     if (n <= 0)
     {
     
       m="ERROR reading from socket";
       strcpy(m1,m.c_str());
       error(m1);
         //close(sockfd);
       continue;
   }
    printf("client number: %d\n", (*info).id);

    
    /* TODO: close socket */
    close(sockfd);

    /* end timer */
    gettimeofday(&end, NULL);

    /* if time up, break */
    if (time_up)
      break;

    /* TODO: update user metrics */
       info->total_rtt+=time_diff(&end,&start);
     //cout<<total<<endl;
  
     info->total_count+=1;
     

    /* TODO: sleep for think time */
    usleep((*info).think_time*1000000);
  }

  /* exit thread */
  
  fprintf(log_file, "User #%d finished\n", info->id);
  fflush(log_file);
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
  int user_count, portno, test_duration;
  float think_time;
  char *hostname;
  float response_time=0.0,x=0.0;
  int total=0,throughput=0;
  float totalrtt=0;

  if (argc != 6) {
    fprintf(stderr,
            "Usage: %s <hostname> <server port> <number of concurrent users> "
            "<think time (in s)> <test duration (in s)>\n",
            argv[0]);
    exit(0);
  }

  hostname = argv[1];
  portno = atoi(argv[2]);
  user_count = atoi(argv[3]);
  think_time = atof(argv[4]);
  test_duration = atoi(argv[5]);

  printf("Hostname: %s\n", hostname);
  printf("Port: %d\n", portno);
  printf("User Count: %d\n", user_count);
  printf("Think Time: %f s\n", think_time); 
  printf("Test Duration: %d s\n", test_duration);
  


  /* open log file */
  log_file = fopen("load_gen.log", "w");
    
  pthread_t threads[user_count];
  struct user_info info[user_count];
  struct timeval start, end;

  /* start timer */
  gettimeofday(&start, NULL);
  time_up = 0;
   for(int i=0;i<user_count;i=i+1)
  {
   string url="GET /index.html HTTP/1.1";
   strcpy(http_request[i],url.c_str());
  }
  
  for (int i = 0; i < user_count; ++i) {
  
    /* TODO: initialize user info */
      info[i].id=i;
      info[i].hostname=hostname;
      info[i].portno=portno;
      info[i].think_time= think_time;
      info[i].total_rtt=0;
      info[i].total_count=0;
    
    /* TODO: create user thread */
    pthread_create(&threads[i],NULL,user_function,(void *)&info[i]);
    fprintf(log_file, "Created thread %d\n", i);
  }

  /* TODO: wait for test duration */
   sleep(test_duration);

  fprintf(log_file, "Woke up\n");

  /* end timer */
  time_up = 1;
  gettimeofday(&end, NULL);

  /* TODO: wait for all threads to finish */
  for(int i=0;i<user_count;i++)
  {
  pthread_join(threads[i],NULL);
  printf("worker %d joined\n",i);
  }

  /* TODO: print results */
  for(int i=0;i<user_count;i++)
  { 
   total+=info[i].total_count;
   totalrtt+=info[i].total_rtt;
   
  }
 
  throughput=total/test_duration;
  printf("\n%d\t",throughput);
  response_time=((totalrtt*1000)/total);
  cout<<response_time;
  /* close log file */
  fclose(log_file);

  return 0;
}



