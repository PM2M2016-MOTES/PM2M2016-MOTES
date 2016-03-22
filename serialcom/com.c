#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <termios.h>
#include <string.h>
#include <errno.h>

#define DEFAULT_PATH "/dev/ttyUSB0"
#define LENGTH_BUFFER 255

typedef enum bool{false = 0, true = 1} bool;

static void interactive_shell(int fd){
  char c, buffer[LENGTH_BUFFER];
  struct termios raw;  
  struct timeval tv;
  fd_set rfds;
  int ret;

  memset((void*)&raw, 0, sizeof(struct termios));
  cfmakeraw(&raw);
  
  /* We set the input terminal to catch every character on the fly */
  tcsetattr(STDIN_FILENO, TCSANOW, &raw);

  /* We assume everything is okay so far */
  write(STDOUT_FILENO, "OK\n\r", 4*sizeof(char));
    
  while(1){
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    FD_ZERO(&rfds);
    FD_SET(STDIN_FILENO, &rfds);

    select(STDIN_FILENO + 1, &rfds, NULL, NULL, &tv);

    /* If input is available, read and proceed it */
    if(FD_ISSET(STDIN_FILENO, &rfds)){
      read(STDIN_FILENO, &c, sizeof(char));
    }
    /* Otherwise, we try to read the input from the mote */
    else{
      while((ret = read(fd, buffer, LENGTH_BUFFER * sizeof(char))) > 0){
	  write(STDOUT_FILENO, buffer, ret * sizeof(char));
	}
	/* There wasn't any input hence we don't process it */
	continue;
    }

    /* Input processing */
    /* Send command to the mote (note the \n\r sent to the mote) */
    if(c == '\n'){
      write(STDOUT_FILENO, "\n", sizeof(char));
      write(fd, "\n\r", 2 * sizeof(char));
    }
    /* Program terminaison */
    else if(c == 'q'){
      break;
    }
    /* Just sending a character to the mote with a local echo */
    else{
      write(STDOUT_FILENO, &c, sizeof(char));
      write(fd, &c, sizeof(char));
    }
  }

  return;
}

int send_char(char const * const c, const int fd){
  int r;

  r = write(fd, c, sizeof(char));
  /* It seems the mote need some time to proceed a character, so let's
     give it some rest before we send the next character */
  usleep((unsigned int)50000);
  return r;
}

void send_command(char const * const command, const int fd){
  char const *c = command;
  char buffer[LENGTH_BUFFER];
  struct timeval tv;
  fd_set rfds;
  int ret;

  printf("*** Sending command %s ***\n", command);
  
  while(*c){
    if(*c == '\n'){
      send_char("\n", fd);
      send_char("\r", fd);
      break;
    }
    else{
      send_char(c, fd);
      c++;
    }
  }
  /* \n\r to signal the end of the command to the mote */
  send_char("\n", fd);
  send_char("\r", fd);

  /* Reading output from the mote for the given command */
  /* First, we're waiting for the beginning of the data */
  ret = read(fd, buffer, LENGTH_BUFFER * sizeof(char));
  /* Print it */
  write(STDOUT_FILENO, buffer, ret * sizeof(char));
  /* While there's data to be read, */
  while(ret > 0){  
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    
    FD_ZERO(&rfds);
    FD_SET(fd, &rfds);

    /* We're looking if data arrived */
    select(fd + 1, &rfds, NULL, NULL, &tv);

    /* If some data is readeable, we read and display it. */
    if(FD_ISSET(fd, &rfds)){
      ret = read(fd, buffer, LENGTH_BUFFER * sizeof(char));
      write(STDOUT_FILENO, buffer, ret * sizeof(char));
    }
    /* If we'we been timed out, we assume there's no data to be read anymore,
       and we leave the function. */
    else{
      ret = 0;
    }
  }

  return;
}

void process_file(char const * const input_file_path, int fd){
  FILE *f;
  char buffer[LENGTH_BUFFER];
  int i;

  if((f = fopen(input_file_path, "r")) == NULL){
    sprintf(buffer, "Cannot open %s", input_file_path);
    perror(buffer);
    return;
  }

  i = 0;
  /*
  while(feof(f) == false){
    fscanf(f, "%s", buffer);
    send_command(buffer, fd);
  }
  */
  while(feof(f) == false){
    fscanf(f, "%c", &buffer[i]);
    if(buffer[i] == '\n'){
      buffer[i] = 0;
      send_command(buffer, fd);
      i = 0;
    }
    else{
      i++;
    }
  }

  fclose(f);
  
  return;
}

void help(char const * const argv){
  fprintf(stderr, "Usage : %s -c <COMMAND> | -f <FILE> | -i <FILE> | -s\n", argv);
  fprintf(stderr, "\tUse -c to execute the given COMMAND\n");
  fprintf(stderr, "\tUse -f to change the block device to open to communicate with the mote (default is %s)\n", DEFAULT_PATH);
  fprintf(stderr, "At least one option between -c -f -i or -s is required.\n");
  
  return;
}

/* -s : interactif
   -i : fichier d'input (pour les commandes à effectuer)
   -f : fichier à ouvrir (au lieu /dev/ttyUSB0)
   -h : help
Rien : lit une commande sur la ligne d'input
*/
/** 
    TODO : Implémenter la lecture de fichier
    TODO : Implémenter l'aide.
    TODO : Implémenter le parsage de l'input.
    TODO : Doc ?
 */
int main(int argc, char **argv){
  char *path, *default_path = DEFAULT_PATH, *input_file_path, *command;
  char buffer[LENGTH_BUFFER];
  int fd, flags, opt;
  struct termios old_t, new_t, old_in;
  
  bool shell = false;
  bool input_file = false;
  bool device_file = false;
  bool command_bool = false;
  /*
  path = argv[1];
  if(path == NULL){
    path = default_path;
  }
  else if(strcmp(path, "-i") == 0){
    interactive = 
  }
  */
  
  path = default_path;
  flags = O_RDWR;
  /*
  if(argv[1] == NULL){
    shell = false;
  }
  else if(strcmp(argv[1], "-s") == 0){
    shell = true;
  }
  */
  while((opt = getopt(argc, argv, "c:f:hi:s")) != -1){
    switch(opt){
    case 'f':
      device_file = true;
      path = optarg;
      break;
    case 'h':
      help(argv[0]);
      return(EXIT_SUCCESS);
    case 'i':
      input_file = true;
      input_file_path = optarg;
      break;
    case 's':
      shell = true;
      break;
    case 'c':
      command = optarg;
      command_bool = true;
      break;
    default:
      /* case '?' */
      printf("[%c] : %s\n", opt, optarg);
      help(argv[0]);
      return(EXIT_FAILURE);
    }
  }

  if(shell == false &&
     device_file == false &&
     input_file == false &&
     command_bool == false){
    fprintf(stderr, "At least one option needed\n");
    return(EXIT_FAILURE);
  }
  
  if(shell){
    flags |= (O_NONBLOCK | O_NDELAY);
  }
  
  fd = open(path, flags);
  if(fd < 0){
    sprintf(buffer, "Cannot open %s", path);
    perror(buffer);
    return(EXIT_FAILURE);
  }

  /* Save current serial port settings */
  tcgetattr(fd, &old_t);
  tcgetattr(STDIN_FILENO, &old_in);

  /* Clear new settings */
  memset((void*)&new_t, 0, sizeof(struct termios));
  
  /* Ok, let's setup everything ... */
  /* Raw mode */
  cfmakeraw(&new_t);

  /* Set Baud rate */
  cfsetospeed(&new_t, (speed_t)B19200);
  cfsetispeed(&new_t, (speed_t)B19200);
  /* 8 Data bits */
  new_t.c_cflag = (new_t.c_cflag & ~CSIZE) | CS8;
  /* No flow control */
  new_t.c_iflag &= ~(IXON | IXOFF | IXANY);
  /* No parity */
  new_t.c_cflag &= ~(PARENB | PARODD);
  /* And 1 stop bit */
  new_t.c_cflag &= ~CSTOPB;

  /* Setting the mote device */
  tcsetattr(fd, TCSANOW, &new_t);

  /* First, process the given command or the file. */
  if(input_file){
    process_file(input_file_path, fd);
  }
  /* First the commad, then the file */
  if(command_bool){
      send_command(command, fd);
  }
  
  /* Then, execute the shell */
  if(shell){
    interactive_shell(fd);
  }
  
  /* Restore serial port settings */
  tcsetattr(fd, TCSANOW, &old_t);
  /* Restore previous terminal settings */
  tcsetattr(STDIN_FILENO, TCSANOW, &old_in);
  /* Close connection */
  close(fd);

  /* And ... We're done :) */
  return EXIT_SUCCESS;
}
