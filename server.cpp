#include "http_server.hh"
#include <vector>
#include <sstream>
#include <sys/stat.h>
#include <string>
#include <vector>
#include <sys/socket.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fstream>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <cstring>
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <stdlib.h>
#include <queue>
using namespace std;
#define SIZE 100
#define QSIZE 100
pthread_t thread_pool[SIZE];
pthread_mutex_t mutex;
pthread_cond_t full;
pthread_cond_t emp;
queue <int> clientque;

void *handle_connection(void *p_client_socket);
void error(char *msg)
{
  perror(msg);
  
}
/* main function*/
int main(int argc, char *argv[])
{
  int sockfd, newsockfd, portno;
  socklen_t clilen;
  string m;
  pthread_mutex_init(&mutex, NULL);
  pthread_cond_init(&full, NULL);
  pthread_cond_init(&emp, NULL);
  int l = m.length();
  char m1[l + 1];

  struct sockaddr_in serv_addr, cli_addr;

  if (argc < 2)
  {
    fprintf(stderr, "ERROR, no port provided\n");
    exit(1);
  }
  // creation of thread pool
  for (int i = 0; i < SIZE; i++)
  {
    pthread_create(&thread_pool[i], NULL, handle_connection, NULL);
  }

  /* create socket */

  sockfd = socket(AF_INET, SOCK_STREAM, 0);
 
  if (sockfd < 0)
  {
    m = "Error opening socket";
    strcpy(m1, m.c_str());
    error(m1);
  }

  /* fill in port number to listen on. IP address can be anything (INADDR_ANY)
   */

  bzero((char *)&serv_addr, sizeof(serv_addr));
  portno = atoi(argv[1]);
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);

  /* bind socket to this port number on this machine */

  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    m = "ERROR on binding";
    strcpy(m1, m.c_str());
    error(m1);
  }

  /* listen for incoming connection requests */

  if (listen(sockfd, 10000) == 0)
    cout << "Listening" << endl;
  else
    cout << "Error" << endl;

  while (1)
  {
    socklen_t addr_len;
    newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
  
    // adding fd of clients to queue
    // locking before accessing shared queue
    pthread_mutex_lock(&mutex);
    while(clientque.size()==QSIZE)
    { 
      pthread_cond_wait(&full, &mutex);
    }
    clientque.push(newsockfd);
    pthread_cond_signal(&emp);
    pthread_mutex_unlock(&mutex); // unlock
  }
}
void *handle_connection(void *p_client_socket)
{
  
  string m;
  int l = m.length();
  char m1[l + 1];
  int client_socket;
  char buffer[256];
  int n;
  while(1)
  {
  pthread_mutex_lock(&mutex); // locking before accessing shared queue

  while (clientque.empty())
  {
    pthread_cond_wait(&emp,&mutex);
   
  }
  client_socket=clientque.front();
  clientque.pop();
   pthread_cond_signal(&full);
  pthread_mutex_unlock(&mutex);

  bzero(buffer, 256);
  n = read(client_socket, buffer, 255);
  
  if (n <= 0)
  {
    m = "ERROR reading from socket";
    strcpy(m1, m.c_str());
    error(m1);
    close(client_socket);
  return 0;
  }

  cout << "Here is the message:" << buffer << endl;

  HTTP_Response *file1 = handle_request(buffer);

  
  string message = file1->get_string();
// cout << message<<endl;
   cout<<"hello"<<endl;
  char msg[message.length() + 1];
  strcpy(msg, message.c_str());
  n = write(client_socket, msg, message.size());
  if (n < 0)
    {
      m = "ERROR writing to socket";
        strcpy(m1, m.c_str());

        error(m1);
    }
  close(client_socket);
  }
  return 0;
}
vector<string> split(const string &s, char delim)
{
  vector<string> elems;

  stringstream ss(s);
  string item;

  while (getline(ss, item, delim))
  {
    if (!item.empty())
      elems.push_back(item);
  }

  return elems;
}

HTTP_Request::HTTP_Request(string request)
{
  vector<string> lines = split(request, '\n');
  vector<string> first_line = split(lines[0], ' ');

  this->HTTP_version = "1.0"; // We'll be using 1.0 irrespective of the request

  /*
   TODO : extract the request method and URL from first_line here
  */
  this->method = first_line[0];
  this->url = first_line[1];

  if (this->method != "GET")
  {
    cerr << "Method '" << this->method << "' not supported" << endl;
    exit(1);
  }
}

HTTP_Response *handle_request(string req)
{


  HTTP_Request *request = new HTTP_Request(req);

  HTTP_Response *response = new HTTP_Response();
  
   string url = string("html_files/") + request->url;

  response->HTTP_version = "HTTP/1.0";

  struct stat sb;
  if (stat(url.c_str(), &sb) == 0) // requested path exists
  {  
    response->status_code = "200";
    response->status_text = "OK";
    response->content_type = "text/html";

    

    if (S_ISDIR(sb.st_mode))
    {
      /*
      In this case, requested path is a directory.
      TODO : find the index.html file in that directory (modify the url
      accordingly)
      */
      url = url + "/index.html";
    }

    /*
    TODO : open the file and read its contents
    */
    ofstream file1;
    file1.open(url, ios::in);
    string data;
    if (file1)
    {
      ostringstream ss;
      ss << file1.rdbuf();
      data = ss.str();
    }
    response->body = data;
    response->content_length = to_string(data.length());
    file1.close();
  }

  else
  {

    response->status_code = "404";
    response->status_text = "Not Found";
    response->content_type = "text/html";

    string html_file = "<!DOCTYPE html><head> <title>Document</title></head><body><h1> Page Not Found </h1></body></html>";
    response->body = html_file;
    response->content_length = to_string(response->body.length());

    /*
    TODO : set the remaining fields of response appropriately
    */
  }

  delete request;

  return response;
}

string HTTP_Response::get_string()
{
  /*
  TODO : implement this function
  */
  string msg = "\n" + this->HTTP_version + " " + this->status_code + " " + this->status_text + " " + "\n" +
               "Content-Type:" + this->content_type + "\n" + "Content-length:" + this->content_length + "\n" + "\n" +
               this->body;

  return msg;
}
