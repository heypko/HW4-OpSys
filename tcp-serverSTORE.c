/* tcp-server.c */

#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 1024


/*

  Will need fwrite for SEND
  Will need file pointers for READ
  Will need DIRENT for LIST

*/

int main()
{
  /* Create the listener socket as TCP socket */
  int sd = socket( PF_INET, SOCK_STREAM, 0 );
  /* sd is the socket descriptor */
  /*  and SOCK_STREAM == TCP       */

  if ( sd < 0 )
  {
    perror( "socket() failed" );
    exit( EXIT_FAILURE );
  }

  /* socket structures */
  struct sockaddr_in server;

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  unsigned short port = 8127;

  /* htons() is host-to-network-short for marshalling */
  /* Internet is "big endian"; Intel is "little endian" */
  server.sin_port = htons( port );
  int len = sizeof( server );

  if ( bind( sd, (struct sockaddr *)&server, len ) < 0 )
  {
    perror( "bind() failed" );
    exit( EXIT_FAILURE );
  }

  listen( sd, 5 );   /* 5 is the max number of waiting clients */
  printf( "Started server; listening on port: %d\n", port );

  struct sockaddr_in client;
  int fromlen = sizeof( client );

  int pid;
  char buffer[ BUFFER_SIZE ];

  while ( 1 )
  {
    /*printf( "PARENT: Blocked on accept()\n" );*/
    int newsock = accept( sd, (struct sockaddr *)&client,
                          (socklen_t*)&fromlen );
    printf( "Received incoming connection from: %s\n",
                inet_ntoa( (struct in_addr)client.sin_addr ));

    /* handle new socket in a child process,
       allowing the parent process to immediately go
       back to the accept() call */
    pid = fork();

    if ( pid < 0 )
    {
      perror( "fork() failed" );
      exit( EXIT_FAILURE );
    }
    else if ( pid == 0 )
    {
      int n;
      #if 0
      sleep( 10 );
      #endif

      do
      {
            //printf( "**childDEBUG %d Blocked on recv()\n", getpid() );

            /* can also use read() and write()..... */
            n = recv( newsock, buffer, BUFFER_SIZE, 0 );

            if ( n < 0 )
            {
              perror( "recv() failed" );
            }
            else if ( n == 0 )
            {
              printf( "[child %d] Client disconnected\n",
                      getpid() );
            }
            else
            {
              buffer[n] = '\0';  /* assuming text.... */
              printf( "[child %d] Received %s\n",
                      getpid(),
                      buffer );
              fflush(NULL);

              char delim[2];
              delim[0] = '\n';
              delim[1] = ' ';
              
              char * message;

              message = strtok(buffer, delim);



              /* Check for STORE */
              if (strcmp(message, "STORE") == 0) {
                fprintf(stderr, "Executing %s\n", message );
                /* Run send_file function */
                /* walk through other tokens */
                while( message != NULL ) 
                {
                  printf( " %s\n", message );
                  message = strtok(NULL, delim);
                }
              }

              /* Check for READ */
              else if (strcmp(message, "READ") == 0) {
                fprintf(stderr, "Executing %s\n", message );
                /* Run send_file function */
              }

              /* Check for LIST */
              else if (strcmp(message, "LIST") == 0) {
                fprintf(stderr, "Executing %s\n", message );
                /* Run send_file function */
              }

              else {
                fprintf(stderr, "Invalid Command\n");
                /* Send Error Back */
              }


              /* can also use read() and write()..... */

              /* send ack message back to the client */
              n = send( newsock, "ACK\n", 4, 0 );
              fflush( NULL );
              if ( n != 4 )
              {
                perror( "send() failed" );
              }

              /* Create file to write */
              FILE* fp = fopen("test.txt", "w");

              /* Check file validity */
              if(NULL == fp) {
                  printf("Error opening file");
                  return 1;
              } else { fprintf(stderr, "File opened for write\n"); }

              /* Load entire file */
              while (n = recv( newsock, buffer, BUFFER_SIZE, 0 ) != 0) {
                printf("Bytes received %d\n", n);
                fprintf(stderr, "buffer Contents: %s\n", buffer);
                fwrite(buffer, 1, n, fp);
              }

              /* send ack message back to the client */
              n = send( newsock, "ACK\n", 4, 0 );
              fflush( NULL );
              if ( n != 4 )
              {
                perror( "send() failed" );
              }
            }
      }
      while ( n > 0 );
      /* this do..while loop exits when the recv() call
         returns 0, indicating the remote/client side has
         closed its socket */

            //printf( "**childDEBUG %d Bye!\n", getpid() );
            close( newsock );
            exit( EXIT_SUCCESS );  /* child terminates here! */

            /* TO DO: add code to handle zombies! */
    }
    else /* pid > 0   PARENT */
    {
      /* parent simply closes the new client socket (endpoint) */
      close( newsock );
    }
  }

  close( sd );

  return EXIT_SUCCESS;
}