/* File: int_str_server.c */
#include <sys/types.h>                                   /* For sockets */
#include <sys/socket.h>                                  /* For sockets */
#include <netinet/in.h>                         /* For Internet sockets */
#include <netdb.h>                                 /* For gethostbyaddr */
#include <stdio.h>                                           /* For I/O */
#include <stdlib.h>                                         /* For exit */
#include <string.h>                                /* For strlen, bzero */
#include <unistd.h>
#include <pthread.h>
#include <queue>
#include <iostream>
#include <unordered_map>
#include <fcntl.h>
#include<fstream>

int input_size(char* buff,int size){
    if(buff[size-1]!='\0'){
        return size;
    }
    else{
        for(int i=0;i<size;i++){
            if(buff[i]=='\0'){
                return i;
            }
        }
    }
    return -100;
}




int main(void){

    char buf[2];
    int fd=open("mpezos.txt",O_RDWR);
    bzero(buf, sizeof(buf));  
    int i=0;

   while(read(fd,buf,sizeof(buf))>0){
       i++;
       if(i==5){
           printf("%s\n",buf);
       }
         bzero(buf, sizeof(buf));  
   }
    printf("%d\n",i);

//     std::ifstream in_file("./asdas/ultrampezosbro", std::ios::binary);
//    in_file.seekg(0, std::ios::end);
//    int file_size = in_file.tellg();
//    std::cout<<"Size of the file is"<<" "<< file_size<<" "<<"bytes";


    return 0;



}