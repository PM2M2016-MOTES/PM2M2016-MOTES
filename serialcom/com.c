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
  fprintf(stderr, "Usage : %s -c <COMMAND> | -f <FILE> | -h | -i <FILE> | -s\n", argv);
  fprintf(stderr, "\tUse -c to execute the given COMMAND\n");
  fprintf(stderr, "\tUse -f to change the block device to open to communicate with the mote (default is %s)\n", DEFAULT_PATH);
  fprintf(stderr, "\tUse -h to print this help\n");
  fprintf(stderr, "\tUse -i to read command from the given FILE\n");
  fprintf(stderr, "\tUse -s to use the shell\n");
  fprintf(stderr, "At least one option between -c -i or -s is required.\n");
  fprintf(stderr, "If many options ar given, they will be exectued in this order : \n");
  fprintf(stderr, "\t- command\n");
  fprintf(stderr, "\t- file\n");
  fprintf(stderr, "\t- shell\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "Available commands : \n");
  fprintf(stderr, "\tAT&V - Display status/settings\n");
  fprintf(stderr, "\tAT&W - Save configuration\n");
  fprintf(stderr, "\tATZ - Reset CPU\n");
  fprintf(stderr, "\tAT+ENTER - Returns OK\n");
  fprintf(stderr, "\tAT+EXIT - Exit the AT mode\n");
  fprintf(stderr, "\tAT+TIMEOUT=000 - Set the Timeout Value of the AT MODE (Value between 0 and 998 - 999 to disable it)\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\tAT+AUTOTEST - Executes an auto test board\n");
  fprintf(stderr, "\tAT+BVO - Display board voltage\n");
  fprintf(stderr, "\n");
  fprintf(stderr, "\tAT+DI | AT+DI=<key> - Display|Set the device ID. key is : 8 bytes length in hexadecimal in OTA mode, 3 bytes length in hexadecimal in ABP mode\n");
  fprintf(stderr, "\t(Only in ABP mode) AT+NA | AT+NA=00112233 - Display|Set the Network Address. 00112233 must be 4 bytes length in hexadecimal\n");
  fprintf(stderr, "\t(Only in ABP mode) AT+NSK | AT+NSK=FFEEDDCCBBAA9900 - Display|Set the Network Session Key. The given key must be 8 bytes length in hexadecimal\n");
  fprintf(stderr, "\t(Only in ABP mode) AT+DSK | AT+DSK=<key> - Display|Set Data Session Key. The given key must be 8 bytes length in hexadecimal\n");
  fprintf(stderr, "\tAT+NI | AT+NI=<key> - Display|Set Network ID. key must be : 8 bytes length in hexadecimal in OTA mode, 3 bytes length in hexadecimal in ABP mode\n");
  fprintf(stderr, "\t(Only in OTA mode)AT+AK | AT+AK=<key> - Display|Set Application Key. The given key must be 8 bytes length in hexadecimal\n");
  fprintf(stderr, "\tAT+JOIN - Join Network\n");
  fprintf(stderr, "\tAT+NJM | AT+NJM=(0 or 1) - Set/Display Network Join Mode. 0: ABP, 1: OTA\n");
  fprintf(stderr, "\tAT+NJS - Network Join Status\n");
  fprintf(stderr, "\n");

  fprintf(stderr, "\tAT+TXDR - Set Tx Datarate. See doc for more information\n");
  fprintf(stderr, "\tAT+TXP - Set Tx Power. See doc for more information\n");
  fprintf(stderr, "\tAT+ADR=(0 or 1) - Adaptive Data Rate. 0: off, 1: on\n");
  fprintf(stderr, "\tAT+RXF=<frequency> - Set Rx Frequency. frequency in Hz\n");
  fprintf(stderr, "\tAT+RXDR=<spread factor>(SPACE)<bandwidth> - Set Rx Datarate. 0 <= bandwidth <= 2, 6 <= spread factor <= 12. See doc.\n");
  fprintf(stderr, "\tAT+RXI=(0 or 1) - Set Rx Inverted. 0: off, 1: on (default on)\n");
  fprintf(stderr, "\n");

  fprintf(stderr, "\tAT+SEND <c> - Send Once in LoraWan. c in ASCII\n");
  fprintf(stderr, "\tAT+SENDB <h> - Send Once in Lorawan. h in hexadecimal\n");
  fprintf(stderr, "\tAT+SENDI 00 <c> - Send Interval in LoraWan. Interval must have 2 digits. c in ASCII. Type '+++' to quit (you should enable the -s option).\n");
  fprintf(stderr, "\tAT+SENDBI 00 <h> - Send Interval in LoraWan. Interval must have 2 digits. h in hexadecimal. Type '+++' to quit (you should enable the -s option).\n");
  fprintf(stderr, "\tAT+RECVC - Recieve continuous. Type '+++' to quit (you should enable the -s option).\n");
  fprintf(stderr, "\n");

  fprintf(stderr, "\tATD - Execute a simultaneous Chirp Emission/Reception\n");
  fprintf(stderr, "\n");
  return;
}

/* -s : interactif
   -i : fichier d'input (pour les commandes à effectuer)
   -f : fichier à ouvrir (au lieu /dev/ttyUSB0)
   -h : help
Rien : lit une commande sur la ligne d'input
*/
/** 
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
  bool command_bool = false;
 
  path = default_path;
  flags = O_RDWR;
  
  while((opt = getopt(argc, argv, "c:f:hi:s")) != -1){
    switch(opt){
    case 'f':
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
     input_file == false &&
     command_bool == false){
    fprintf(stderr, "At least one option needed between -c, -i and -s\n");
    help(argv[0]);
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
  /* First the commad, then the file */
  if(command_bool){
      send_command(command, fd);
  }
  if(input_file){
    process_file(input_file_path, fd);
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
