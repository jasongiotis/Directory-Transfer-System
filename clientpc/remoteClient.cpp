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
#include <experimental/filesystem>
#include <queue>
#include <iostream>
#include <unordered_map>
#include <fcntl.h>
#include <fstream>
std::string foldername;
std::string ip;
int port_adr;
namespace fs = std::experimental::filesystem;
int main(int argc, char *argv[]) /* Client with Internet stream sockets */
{
   int port, sock;
   unsigned int serverlen;
   // Finding where each argument is
   for (int i = 1; i <= 5; i = i + 2)
   {
      if (!strcmp(argv[i], "-p"))
      {
         port_adr = atoi(argv[i + 1]);
      }
      if (!strcmp(argv[i], "-i"))
      {
         ip = std::string(argv[i + 1]);
      }
      if (!strcmp(argv[i], "-d"))
      {
         foldername = std::string(argv[i + 1]);
      }
   }
   printf(" we got %d %s %s \n", port_adr, ip.c_str(), foldername.c_str());
   struct sockaddr_in server;
   struct sockaddr *serverptr;
   struct hostent *rem;

   if ((sock = socket(PF_INET, SOCK_STREAM, 0)) < 0)
   { /* Create socket */
      perror("socket");
      exit(1);
   }
   if ((rem = gethostbyname(ip.c_str())) == NULL)
   { /* Find server address */
      perror("gethostbyname");
      exit(1);
   }
   port = port_adr;             /* Convert port number to integer */
   server.sin_family = PF_INET; /* Internet domain */
   bcopy((char *)rem->h_addr, (char *)&server.sin_addr,
         rem->h_length);
   server.sin_port = htons(port); /* Server's Internet address and port */
   serverptr = (struct sockaddr *)&server;
   serverlen = sizeof(server);
   if (connect(sock, serverptr, serverlen) < 0)
   { /* Request connection */
      perror("connect");
      exit(1);
   }
   printf("Requested connection to host %s port %d\n", argv[2], port);

   printf("directory :%s\n", foldername.c_str());
   if (write(sock, foldername.c_str(), foldername.length()) < 0)
   { // sending the requested directory to the client
      perror("write");
      exit(1);
   }

   int x;
   int buffer_size;

   if (read(sock, &buffer_size, sizeof(int)) < 0)
   { // recieving buffer size from the client
      perror("read");
      exit(1);
   }

   if (read(sock, &x, sizeof(int)) < 0)
   { /* recieving the number of files the server will send us*/
      perror("read");
      exit(1);
   }
   printf("files to be received : %d \n", x);
   for (int i = 0; i < x; i++)
   { // then for each file we take the data
      int f_size;
      char buf[buffer_size];
      bzero(buf, sizeof(buf));
      if (read(sock, buf, sizeof(buf)) < 0)
      {
         perror("read");
         exit(1);
      } // getting file name from server
      if (read(sock, &f_size, sizeof(int)) < 0)
      {
         perror("read");
         exit(1);
      } // getting file size from server

      printf("Received :  %s . Size is %d bytes \n", buf, f_size);
      std::string open_name(buf);
      std::string file_name(buf);
      file_name = file_name.substr(0, file_name.find_last_of('/')); // we take the directories that contain the file
      std::string dir = file_name;
      if (file_name.length() != 0)
      {
         fs::create_directories(file_name);
      }                                // we create the directories that contain the file if they dont exist
      std::ofstream myfile(open_name); // opening the file to clients local filesystem
      bzero(buf, sizeof(buf));         // clearing the buffer

      int w;

      float loops = f_size / buffer_size;
      if (f_size <= buffer_size)
      {
         loops = 1;
      }
      else
      {
         loops++;
      }
      int lps = loops; // the number of blocks we will receive
      int its;
      its = 0;

      bzero(buf, sizeof(buf));
      while (w = read(sock, buf, sizeof(buf)) > 0) // getting the block
      {

         myfile.write(buf, strlen(buf)); // writing the block to the file
         bzero(buf, sizeof(buf));
         its++;
         if (its == lps)
         {
            break;
         }
      }

      myfile.close();
   }

   close(sock);
   exit(0);
} /* Close socket and exit */