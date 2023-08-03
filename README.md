    Directory Transfer System in C++ using sockets and threads

    A very simple make file is provided.
    Use make compile to create the executables
    Use make clean to delete them 

    Local Execution Example ( you must change the ip addresses accordingly if you want to run it in different machines) :
    ./dataServer -s 1 -p 4037 -b 512 -q 1

    To execute the client make sure you are in the clientpc directory 

    if you want to transfer all the Lyrics folder:
        ./remoteClient -p 4030 -i 127.0.0.1 -d ./Lyrics 

    if you want to transfer only one subfolder of Lyrics folder :
        ./remoteClient -p 4030 -i 127.0.0.1 -d ./Lyrics/Metallica



    Design choices :
    I used  pthread for the threads , mutexed and conditions

    I used Experimental file system for recursive directory iterattion and for creating 
    the required directories before we open the file

    I used open and write for  socket I/O  

    I used < fstream > for files 

    I used c++ stl queue for the workers queue and the mutexes and conditions  used
    make the queue thread safe

    I used c++ stl unordered map to implement the socket maps needed 

    Important notes : 
    If you try to  transfer a directory that already exists in the client's filesystem the directory contents  will be overwritten

    Client must be run in a different directory than server . If client can see the server directories when running 
    there will be no results  . So after you use make compile place dataClient to clientpc1 or to any other directory you would like 
    so everything works fine 


    dataServer executable must be in the same directory with the server root folder/filesystem
    the metadata that server sends to client for each file is:
    file name
    file size
    full path to the file ( so we can create the folders needed)
