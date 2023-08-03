/* File: int_str_server.c */
#include <sys/types.h>  /* For sockets */
#include <sys/socket.h> /* For sockets */
#include <netinet/in.h> /* For Internet sockets */
#include <netdb.h>      /* For gethostbyaddr */
#include <stdio.h>      /* For I/O */
#include <stdlib.h>     /* For exit */
#include <string.h>     /* For strlen, bzero */
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <iostream>
#include <unordered_map>
#include <experimental/filesystem>
#include <fstream>
#include <fcntl.h>
namespace fs = std::experimental::filesystem;

pthread_mutex_t queue_usage; // prevents threads from using the queue the same time
pthread_cond_t full_queue;   // prevents queue from passing the allowed size
pthread_cond_t emty_queue;   // prevents poping from an emty queue
int max_allowed_size;        // queue size
int workers;                 // thread pool size
int buffer_size;             // block size
int port_adr;

typedef struct service_args
{ // arguments for the communication threads
   int buf_size;
   int newsock; // socket id

} service_args;

typedef struct job
{                    // our queue will contain job objects
   std::string name; // filename
   int sock;         // socket for the data to be sent

} job;

std::queue<job> service_queue;                         // a queue that contains all the jobs for the workers
std::unordered_map<int, pthread_mutex_t *> socket_map; // we map each socket to a mutex to prevent 2 threads from writing to the same socket
std::unordered_map<int, int> socket_total_services;    // we map each socket to the number of files we will need to send to it
std::unordered_map<int, int> socket_services;          // we map each socket to the number of files we've sent so far to it

// the fucntion for the communication thread

void *service(void *arg)
{
   int buf_size = ((service_args *)arg)->buf_size; // taking buffer size
   int newsock = ((service_args *)arg)->newsock;   // taking socket number
   int file_count;                                 // the number of files our client requires
   if (socket_map.find(newsock) == socket_map.end())
   { // creating a mutex for each socket if
      socket_map[newsock] = new pthread_mutex_t;
      pthread_mutex_init(socket_map[newsock], NULL);
   }
   else
   {
      pthread_mutex_destroy(socket_map[newsock]);
      socket_map[newsock] = new pthread_mutex_t;
      pthread_mutex_init(socket_map[newsock], NULL);
   }
   socket_services[newsock] = 0;
   char buf[buffer_size];
   bzero(buf, sizeof(buf)); /* Initialize buffer */

   pthread_mutex_lock(&queue_usage);
   pthread_mutex_lock(socket_map[newsock]);
   if (read(newsock, buf, sizeof(buf)) < 0)
   { /* Get message */
      perror("read");
      exit(1);
   }
   if (write(newsock, &buffer_size, sizeof(buffer_size)) < 0)
   {
      perror("write");
      exit(1);
   }

   pthread_mutex_unlock(socket_map[newsock]);
   printf("[Thread: %ld ]: Received foler %s: \n", pthread_self(), buf);
   std::string dir(buf);
   std::cout << "keep in mind" << dir << std::endl;

   for (const fs::directory_entry &file :
        fs::recursive_directory_iterator(dir))
   {
      if (!is_regular_file(file.status()))
         continue;
      file_count++;
   }
   pthread_mutex_lock(socket_map[newsock]);
   write(newsock, &file_count, sizeof(int));
   pthread_mutex_unlock(socket_map[newsock]);
   socket_total_services[newsock] = file_count;

   for (const fs::directory_entry &file :
        fs::recursive_directory_iterator(dir))
   {
      if (!is_regular_file(file.status()))
         continue;
      std::string name = file.path().string();

      if (service_queue.size() == max_allowed_size)
      {
         pthread_cond_wait(&full_queue, &queue_usage);
      }
      job s = {
          .name = std::string(name),
          .sock = newsock};
      printf("[Thread: %ld ]: adding file  %s  to queue : \n", pthread_self(), s.name.c_str());
      service_queue.push(s);
      if (service_queue.size() == 1)
      {
         pthread_cond_signal(&emty_queue);
      }
   }

   pthread_mutex_unlock(&queue_usage);

   return NULL;
}
void *worker(void *argu)
{
   int s;
   job j;
   while (1)
   {
      pthread_mutex_lock(&queue_usage);
      if (service_queue.size() == 0)
      {
         pthread_cond_wait(&emty_queue, &queue_usage);
      }

      j = service_queue.front();
      s = j.sock;
      service_queue.pop();
      printf("[Thread: %ld ]: Recieved task  %s  to queue : \n", pthread_self(), j.name.c_str());
      if (service_queue.size() == max_allowed_size - 1)
      {
         pthread_cond_signal(&full_queue);
      }
      pthread_mutex_unlock(&queue_usage);
      pthread_mutex_lock(socket_map[s]);
      printf("[Thread: %ld ]: about to read  %s  to queue : \n", pthread_self(), j.name.c_str());
      char name[buffer_size];
      strcpy(name, j.name.c_str());
      std::ifstream in_file(j.name, std::ios::binary);
      in_file.seekg(0, std::ios::end);
      int file_size = in_file.tellg();
      if (write(s, name, sizeof(name)) < 0)
      {
         perror("write");
         exit(1);
      }
      if (write(s, &file_size, sizeof(file_size)) < 0)
      {
         perror("write");
         exit(1);
      }
      float loops = (file_size / buffer_size);
      if (buffer_size >= file_size)
      {
         loops = 1;
      }
      else
      {
         loops++;
      }
      char buff[buffer_size];

      std::ifstream file;
      file.open(name);

      int lps = loops;

      for (int i = 0; i < lps; i++)
      {
         bzero(buff, sizeof(buff));
         //  if(read(fd,buff,sizeof(buff))<0){perror("read");}
         file.read(buff, sizeof(buff));
         if (write(s, buff, sizeof(buff)) < 0)
         {
            perror("write");
         }
      }
      file.close();
      //  close(fd);
      socket_services[s]++;
      if (socket_services[s] == socket_total_services[s])
      {
         socket_services[s] = 0;
         close(s);
      }

      pthread_mutex_unlock(socket_map[s]);
   }
   return NULL;
}

int main(int argc, char *argv[]) /* Server with Internet stream sockets */
{
   int port, sock, newsock;
   unsigned int serverlen, clientlen;
   struct sockaddr_in server, client;
   struct sockaddr *serverptr, *clientptr;
   struct hostent *rem;
   // Finding where each argument is
   for (int i = 1; i <= 7; i = i + 2)
   {
      if (!strcmp(argv[i], "-p"))
      {
         port_adr = atoi(argv[i + 1]);
      }
      if (!strcmp(argv[i], "-s"))
      {
         workers = atoi(argv[i + 1]);
      }
      if (!strcmp(argv[i], "-q"))
      {
         max_allowed_size = atoi(argv[i + 1]);
      }
      if (!strcmp(argv[i], "-b"))
      {
         buffer_size = atoi(argv[i + 1]);
      }
   }

   if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
   { /* Create socket */
      perror("socket");
      exit(1);
   }
   port = port_adr;                            /* Convert port number to integer */
   server.sin_family = PF_INET;                /* Internet domain */
   server.sin_addr.s_addr = htonl(INADDR_ANY); /* My Internet address */
   server.sin_port = htons(port);              /* The given port */
   serverptr = (struct sockaddr *)&server;
   serverlen = sizeof server;
   if (bind(sock, serverptr, serverlen) < 0)
   { /* Bind socket to address */
      perror("bind");
      exit(1);
   }
   if (listen(sock, 5) < 0)
   { /* Listen for connections */
      perror("listen");
      exit(1);
   }
   printf("Listening for connections to port %d\n", port);
   pthread_mutex_init(&queue_usage, NULL);
   pthread_cond_init(&full_queue, NULL);
   pthread_cond_init(&emty_queue, NULL);
   pthread_t worker_ths[workers];
   for (int i = 0; i < workers; i++)
   {
      if (pthread_create(&worker_ths[i], NULL, &worker, NULL) < 0)
      { /* Create child for serving the client */
         perror("pthread");
         exit(1); /* Child process */
      }
   }

   while (1)
   {
      clientptr = (struct sockaddr *)&client;
      clientlen = sizeof client;
      if ((newsock = accept(sock, clientptr, &clientlen)) < 0)
      {
         perror("accept");
         exit(1);
      } /* Accept connection */
      if ((rem = gethostbyaddr((char *)&client.sin_addr.s_addr,
                               sizeof client.sin_addr.s_addr, /* Find client's address */
                               client.sin_family)) == NULL)
      {
         perror("gethostbyaddr");
         exit(1);
      }
      printf("Accepted connection from %s\n", rem->h_name);
      pthread_t th;
      service_args s = {
          .buf_size = 256,
          .newsock = newsock};
      if (pthread_create(&th, NULL, &service, &s) < 0)
      { /* Create child for serving the client */
         perror("pthread");
         exit(1); /* Child process */
      }
   }
   return 0;
}
