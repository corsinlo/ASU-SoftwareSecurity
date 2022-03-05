#include <unistd.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <errno.h>
#include <time.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <ctype.h>
int _socket;

//Define a range to connecting ports
const int PORT_MAX = 65000l;
const int PORT_MIN = 0;

//Fuction definitions
void getCommand(char * url);
char* Welcome(void);
void acceptor(struct sockaddr_in _address);
void server_stop();

int main(int argc, char *argv[]){

  //Variable Decarations
  struct sockaddr_in _address;

  //Check Port range passed as a argument
  if (atoi(argv[1]) > PORT_MAX || (long) argv[1] < PORT_MIN) {
        printf("Please select port between %d - %d\n", PORT_MIN, PORT_MAX);
        exit(1);
    }

  if(argc < 2){
    printf("Port Missing Please Execute: ./normal_web_server <port_number>\n");
    exit(1);
  }
 
  //Define Ctrl + C ,i.e. SIGINT for exit
  signal(SIGINT, server_stop);

  //Create socket and check status
  if ((_socket = socket(AF_INET, SOCK_STREAM, 0)) <= 0) {
      perror("Socket creation failed");
      exit(1);
   }

  //Server socket struct
  _address.sin_family = AF_INET;
  _address.sin_addr.s_addr = INADDR_ANY;
  _address.sin_port = htons(atoi(argv[1]));

  //Binding socket to Server
  if(bind(_socket, (struct sockaddr *)&_address, sizeof(_address)) != 0){
    perror("Error in Binding, Port might be busy, try again later.");
    exit(1);
  }

  //Listening Process
      if(listen(_socket, 3) < 0){
          perror("Listen Failed");
          exit(1);
      }
          printf("The Client is connected!\n");
          acceptor(_address);
          return 0;
}

//Ctrl + C function
void server_stop() {
    printf("\nExit Handler\n");
    shutdown(_socket, 2);
    printf("Socket Closed\n");
    close(_socket);
    exit(0);
}

//Get command from url
void getCommand(char * url){
  char * _temp;
  _temp = malloc(strlen(url));
  int i, j=0;

  for(i = 0; i<strlen(url); i++){
    if(*(url+i) == '+'){
      _temp[j++] = ' ';
    }
    else if(*(url+i) != '%'){
      _temp[j++] = *(url+i);
    }
    else if(!isxdigit(*(url+i+1)) || !isxdigit(*(url+i+2))){
      _temp[j++] = *(url+i);
    }
    else{
      char first_char = tolower(*(url+i+1));
      char second_char = tolower(*(url+i+2));
      if(first_char <= '9'){
        first_char = first_char - '0';
      }
      else{
        first_char = first_char - 'a' + 10;
      }
      if(second_char <= '9'){
        second_char = second_char - '0';
      }
      else{
        second_char = second_char - 'a' + 10;
      }

      _temp[j++] = (16 * first_char + second_char);
      i += 2;
    }
  }
  _temp[j] = '\0';
  strcpy(url, _temp);

  char * exec = "/exec/";
  if(strlen(url) <= 6 || strncmp(exec, _temp, strlen(exec)) != 0){
    memset(_temp, 0, sizeof(_temp));
    strcpy(url, _temp);
    return;
  }

  char command[4096];
  strcpy(command, url+6);
  strcpy(url, command);
}

void acceptor(struct sockaddr_in _address){
  char * getdate = Welcome();
  char * response_200 = "HTTP/1.1 200 SUCCESS\r\n"
                              "Content-Type: text/html; charset=utf-8\r\n"
                              "Accept-Ranges: bytes\r\n"
                              "Connection: close\r\n"
                              "Date: Thu, 21 Jan 2021 15:28:06 GMT\r\n"
                              "Server: Normal Web Server\r\n"
                              "Content-Length: 30000\r\n"
                              "\r\n";                            
  char * response_404 = "HTTP/1.1 404 NOTFOUND\r\n"
                                    "Content-Type: text/html; charset=utf-8\r\n"
                                    "Accept-Ranges: bytes\r\n"
                                    "Connection: close\r\n"
                                    "Date: Thu, 21 Jan 2021 15:28:06 GMT\r\n"
                                    "Server: Normal Web Server\r\n"
                                    "Content-Length: 4096\r\n"
                                    "\r\n";  
  char * get = "GET";
  char * host = "Host:";

  while(1){
    //Accept incoming client requests
    long address_len = sizeof(_address);
    long socket_descriptor = accept(_socket, (struct sockaddr *)&_address, (socklen_t *)&address_len);
    char buffer[4096];
    char url[4096];
    read(socket_descriptor, buffer, 1000);
    char result[30000];
    char _header[4096];
    if(strlen(buffer)<= 10 || strncmp(get, buffer, strlen(get)) != 0){
      strcpy(_header, response_404);
      strcpy(result, "Wrong Method");
    }
    else if(!strstr(buffer,host))
    {
      if(!strstr(buffer,"http://"))
      {
      strcpy(_header, response_404);
      strcpy(result, "Wrong Method");
      }
    }
    else{
      int i, j=0;
      for(i = 4; i<strlen(buffer); i++){
        if(buffer[i] == ' '){
          break;
        }
        url[j++] = buffer[i];
      }
      url[j] = '\0';
      getCommand(url);
      printf("Command: %s\n", url);

      if(strlen(url) == 0){
        strcpy(_header, response_404);
        strcpy(result, "Invalid, Try again");
      }
      else{
        strcat(url, " 2>&1");
        char temp[4096];
        FILE* file = popen(url, "r");
        if(file == NULL){
          strcpy(_header, response_404);
          strcpy(result, "Invalid, Try again");
        }
        else{
          int flag = 0;
          while (fgets(temp, sizeof(temp), file) != NULL)
          { 
            if(flag == 0){
                strcpy(result, temp);
                flag++;
            }
            else{
                strcat(result, temp);
            }
          }
          strcpy(_header, response_200);
        }
        if (pclose (file) != 0)
        {
          strcpy(_header, response_404);
          file = NULL;
        }
        memset(temp, 0, sizeof(temp));
      }
    }
    char * response = NULL;
    response = malloc(strlen(_header) + strlen(result) + 10);
    strcpy(response, _header);
    strcat(response, result);
    send(socket_descriptor, response, strlen(response), 0);
    shutdown(socket_descriptor,2);
    close(socket_descriptor);
    printf("\nResponse Code:\n%s\n", response);

    //clearing all buffers
    memset(buffer, 0, sizeof(buffer));
    memset(url, 0, sizeof(url));
    memset(response, 0, sizeof(response));
    memset(result, 0, sizeof(result));
    memset(_header, 0, sizeof(_header));
  }
} 

char* Welcome(void)
{
    time_t current_time;
    char* time_;
    current_time = time(NULL);
    if (current_time == ((time_t)-1))
    {
        perror("Conversion Fail");
        exit(EXIT_FAILURE);
    }
    time_ = ctime(&current_time);
    if (time_ == NULL)
    {
        perror("Conversion Fail");
        exit(EXIT_FAILURE);
    }
    (void) printf("welcome It's: %s", time_);
    return time_;
    exit(EXIT_SUCCESS);
}
