/* tcp-server.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>

#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

#include <dirent.h>
#include <math.h>
#include <ctype.h>




#define BUFFER_SIZE 40


/*

  Will need fwrite for STORE
  Will need file pointers for READ
  Will need DIRENT for LIST

*/
char buffer[ BUFFER_SIZE ];

int store_file(char*, char*, char**, int, int);
int read_file(char*, char*, char**, int);
int list_file(int);

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

  if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int)) < 0)
    perror("setsockopt(SO_REUSEADDR) failed");

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
          //buffer[n] = '\0';  /* assuming text.... */
          printf( "[child %d] Received %s\n",
                  getpid(),
                  buffer );
          fflush(NULL);

          char delim[3];
          delim[0] = '\n';
          delim[1] = ' ';
          delim[2] = 92;

          char** saveptr = (char**)calloc(BUFFER_SIZE, sizeof(char*));;
          
          char * message;
          message = strtok_r(buffer, delim, saveptr);


          /* Check for STORE */
          if (strcmp(message, "STORE") == 0) {
            fprintf(stderr, "Executing %s\n", message );

            /* Call store_file function */
            if (store_file(message, delim, saveptr, newsock, n) == 0) {
              fprintf(stderr, "Success\n");
            } else { fprintf(stderr, "Fail\n"); }

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
            /* Run list_file function */
            if (list_file(newsock) == 0) {
              fprintf(stderr, "Success\n");
            } else { fprintf(stderr, "Fail\n"); }

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

int store_file(char * message, char* delim, char** saveptr, int newsock, int moreMsg) {

  /* Allocate memory for message */
  char** parsed = (char**)calloc(2, sizeof(char*));

  /* Initialize offset */
  int offset = strlen(message);

  /* Get filename (parsed[0]), # of bytes (parsed[1]) */
  int i;
  for(i = 0; i < 2; ++i) 
  {
    message = strtok_r(NULL, delim, saveptr);
    parsed[i] = (char*)calloc(strlen(message), 1);
    strcpy(parsed[i], message);
    fprintf(stderr, "Message[%d] = %s\n", i, message);
  }
  
  /* Check for non integer byte number */
  for (i = 0; i < strlen(parsed[1]) - 1; ++i) {
    if (isdigit(parsed[1][i]) == 0) {
      fprintf(stderr, "Sending: ERROR INVALID REQUEST\n");
      send( newsock, "ERROR INVALID REQUEST\n", 22, 0 );
      return 1;
    }
  }

  // char* filename = parsed[0];
  int numBytes = atoi(parsed[1]);

  /* Create file pointer, open file to read */
  FILE* fp = fopen(parsed[0], "rb");

  /* Check file validity */
  if(!(NULL == fp)) {
    fprintf(stderr, "Sending: ERROR FILE EXISTS\n");
    send( newsock, "ERROR FILE EXISTS\n", 18, 0 );
    return 1;
  } 

  /* Create file pointer, file to write */
  fp = fopen(parsed[0], "wb");

  /* Check file validity */
  if(NULL == fp) {
      fprintf(stderr, "Error opening file\n");
      return 1;
  } else { fprintf(stderr, "File opened for write\n"); }

  /* Stop using delimiters in remainder of message */
  char emptyDelim[1];

  /* Get remainder of message */
  message = strtok_r(NULL, emptyDelim, saveptr);
  fprintf(stderr, "BufferRemainder: Message = %s\n", message);

  
  /* Find offset based off of length of previous 
    string arguments and number of delimiters */
  offset += (strlen(parsed[0]) + strlen(parsed[1]));
  offset = moreMsg - offset - 3;

  /* Write remainder of buffer */
  fwrite(message, 1, offset, fp);

  fprintf(stderr, "preLoop = %d\n", offset);
  numBytes -= offset;
  

  while (numBytes > 0) {
    /* Set buffer to uninitialized values */
    bzero(buffer, BUFFER_SIZE);

    /* Keep receiving */
    moreMsg = recv( newsock, buffer, BUFFER_SIZE, 0 );
    numBytes -= moreMsg;
    fprintf(stderr, "BufferRemainder: Message = %s\n", buffer);
    fprintf(stderr, "writing %d bytes\n", moreMsg);
    fwrite(buffer, 1, moreMsg, fp);
  }

  /* Send ack back to the client */
  int n = send( newsock, "ACK\n", 4, 0 );
  fflush( NULL );
  if ( n != 4 )
  {
    perror( "STORE ACK FAILED" );
  }

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

  /* Check for non integer byte number */
  for (i = 0; i < strlen(parsed[2]) - 1; ++i) {
    if (isdigit(parsed[2][i]) == 0) {
      fprintf(stderr, "Sending: ERROR INVALID REQUEST\n");
      send( newsock, "ERROR INVALID REQUEST\n", 22, 0 );
      return 1;
    }
  }

  //char* filename = parsed[0];
  int offset = atoi(parsed[1]);
  int readLength = atoi(parsed[2]);

  /* Create file pointer, open file to read */
  FILE* fp = fopen(parsed[0], "rb");

  /* Check file validity */
  if(NULL == fp) {
      fprintf(stderr, "Sending: ERROR NO SUCH FILE\n");
      send( newsock, "ERROR NO SUCH FILE\n", 19, 0 );
      return 1;
  } else { fprintf(stderr, "File opened for read\n"); }

  /* Check byte range validity */
  fseek(fp, 0L, SEEK_END);
  long fileSize = ftell(fp);

  fseek(fp, offset, 0);
  int offsetLocation = ftell(fp) + (long)readLength;

  if (offsetLocation < 0 || (offsetLocation > fileSize)) {
    fprintf(stderr, "ERROR INVALID BYTE RANGE\n");
    send( newsock, "ERROR INVALID BYTE RANGE\n", 25, 0 );
    return 1;
  }

  /* Skip to requested offset */
  fseek(fp, offset, 0);

  /* Write ACK into send buffer */
  snprintf(buffer, (6 + strlen(parsed[2])),"ACK %d\n", readLength);
  
  /* Read # of bytes */
  fread(buffer + (6 + strlen(parsed[2])), 1, readLength, fp);

  /* Send message back to the client */
  int n = send( newsock, buffer, (readLength + 5 + strlen(parsed[2])), 0 );
  fflush( NULL );
  if ( n != readLength )
  {
    perror( "READ FAILED" );
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

/* Helper function for strlen(itoa(int)) */
int numLen(int i)
{
  if (i < 0) i = -i;
  if (i <         10) return 1;
  if (i <        100) return 2;
  if (i <       1000) return 3;
  if (i <      10000) return 4;
  if (i <     100000) return 5;
  if (i <    1000000) return 6;      
  if (i <   10000000) return 7;
  if (i <  100000000) return 8;
  if (i < 1000000000) return 9;
  return 10;
}

int list_file(int newsock) {
      
  struct dirent **namelist;
  int numFiles;

  /* numFiles - TOTAL number of entries in directory */
  numFiles = scandir(".", &namelist, NULL, alphasort);
 

  /* Initialize buffers and variables */
  struct stat buf;
  int dBufferLength = 0;
  int fileCount = 0;
  int i;

  /* count number of files and add to dBufferLength */
  for (i = 0; i < numFiles; ++i) {
    /* Get file information */
    int lstatCheck = lstat( namelist[i]->d_name, &buf );

    if ( lstatCheck == -1 ) {
      perror( "lstat() failed" );
      return EXIT_FAILURE;
    }

    /* Only count regular files */
    if ( S_ISREG( buf.st_mode ) ) {
      fprintf ( stdout, "In Directory: %s -- regular file\n", namelist[i]->d_name);
      dBufferLength += strlen(namelist[i]->d_name) + 1;
      ++fileCount;
    }  
  }

  /* calculate length of fileCount as a string and increment dBufferLength */
  int nDigits;
  nDigits = numLen(fileCount);
  dBufferLength += nDigits + 1;

  /* allocate dBuffer memory */
  char* dBuffer = (char*)calloc(1, dBufferLength);

  /* printf to dBuffer */
  snprintf(dBuffer, nDigits + 1,"%d", fileCount);

  /* copy files into dynamic buffer and free namelist memory */
  for (i = 0; i < numFiles; ++i) {
    /* Get file information */
    int lstatCheck = lstat( namelist[i]->d_name, &buf );

    if ( lstatCheck == -1 ) {
      perror( "lstat() failed" );
      return EXIT_FAILURE;
    }

    /* Only add regular files */
    if ( S_ISREG( buf.st_mode ) ) {
      
      snprintf(dBuffer + strlen(dBuffer), (strlen(namelist[i]->d_name) + 2), " %s", namelist[i]->d_name);
      fprintf ( stdout, "Adding to dBuffer: %s -- regular file\n", namelist[i]->d_name);
      fprintf(stderr, "dBuffer = %s\n", dBuffer);

      /* Free namelist element memory */
      free(namelist[i]);
    }
  }
  /* Free namelist pointer memory */
  free(namelist);      

  snprintf(dBuffer + strlen(dBuffer), 2, "\n");
  fprintf(stderr, "dBufferLength = %d\n", dBufferLength);
  fprintf(stderr, "dBuffer = %s\n", dBuffer);
  /* Send message back to the client */
  int n = send( newsock, dBuffer, dBufferLength, 0 );
  fflush( NULL );
  if ( n != dBufferLength )
  {
    perror( "LIST FAILED" );
  }

  return 0;
}

