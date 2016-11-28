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

int store_file(char*, char*, char**, int);
int read_file(char*, char*, char**, int);

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

              char** saveptr;
              
              char * message;
              message = strtok_r(buffer, delim, saveptr);


              /* Check for STORE */
              if (strcmp(message, "STORE") == 0) {
                fprintf(stderr, "Executing %s\n", message );

                /* Call store_file function */
                if (store_file(message, delim, saveptr, newsock) == 0) {
                  fprintf(stderr, "Success\n");
                } else { fprintf(stderr, "Fail\n"); }

                /* Send ack back to the client */
                n = send( newsock, "ACK\n", 4, 0 );
                fflush( NULL );
                if ( n != 4 )
                {
                  perror( "STORE ACK FAILED" );
                }

              }

              /* Check for READ */
              else if (strcmp(message, "READ") == 0) {
                fprintf(stderr, "Executing %s\n", message );
                /* Call read_file function */
                if (read_file(message, delim, saveptr, newsock) == 0) {
                  fprintf(stderr, "Success\n");
                } else { fprintf(stderr, "Fail\n"); }


              }

              /* Check for LIST */
              else if (strcmp(message, "LIST") == 0) {
                fprintf(stderr, "Executing %s\n", message );
                /* Run send_file function */
              }

              else {
                fprintf(stderr, "ERROR INVALID REQUEST\n");
                /* Send error message back to the client */
                n = send( newsock, "ERROR INVALID REQUEST\n", 22, 0 );
                fflush( NULL );
                if ( n != 22 )
                {
                  perror( "ERROR SEND FAILED" );
                }
              }


              /* can also use read() and write()..... */


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

int store_file(char * message, char* delim, char** saveptr, int newsock) {

  /* Allocate memory for message */
  char** parsed = (char**)calloc(2, sizeof(char*));

  /* Get filename (parsed[0]), # of bytes (parsed[1]) */
  int i;
  for(i = 0; i < 2; ++i) 
  {
    message = strtok_r(NULL, delim, saveptr);
    parsed[i] = (char*)calloc(strlen(message), 1);
    strcpy(parsed[i], message);
  }

  /* Write file contents */
  message = strtok_r(NULL, delim, saveptr);

  /* Create file pointer, file to write */
  FILE* fp = fopen(parsed[0], "w");

  /* Check file validity */
  if(NULL == fp) {
      fprintf(stderr, "Error opening file\n");
      return 1;
  } else { fprintf(stderr, "File opened for write\n"); }

  /* Load entire file */
  fwrite(message, 1, atoi(parsed[1]), fp);

  /* Free message memory */
  for (i = 0; i < 2; ++i) {
    free(parsed[i]);
  }
  free(parsed);
  
  /* Close file pointer */
  fclose(fp);

  /* Success! */
  return 0;
}

int read_file(char * message, char* delim, char** saveptr, int newsock) {

  /* Allocate memory for message */
  char** parsed = (char**)calloc(2, sizeof(char*));

  /* Get filename (parsed[0]), 
    byte offset(parsed[1]) 
    # of bytes (parsed[2]) */
  int i;
  for(i = 0; i < 3; ++i) 
  {
    message = strtok_r(NULL, delim, saveptr);
    parsed[i] = (char*)calloc(strlen(message), 1);
    strcpy(parsed[i], message);
  }

  /* Create file pointer, open file to read */
  FILE* fp = fopen(parsed[0], "r");

  /* Check file validity */
  if(NULL == fp) {
      fprintf(stderr, "Error opening file\n");
      return 1;
  } else { fprintf(stderr, "File opened for write\n"); }

  /* Load entire file */
  fwrite(message, 1, atoi(parsed[1]), fp);

  /* Send message back to the client */
  int n = send( newsock, "READ completed.\n", 17, 0 );
  fflush( NULL );
  if ( n != 17 )
  {
    perror( "STORE ACK FAILED" );
  }

  /* Free message memory */
  for (i = 0; i < 3; ++i) {
    free(parsed[i]);
  }
  free(parsed);
  
  /* Close file pointer */
  fclose(fp);

  /* Success! */
  return 0;

}