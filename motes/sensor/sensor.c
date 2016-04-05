#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

typedef enum{false = 0, true = 1} bool;
struct conf{
  int cpu;
  int seconds;
};

#define BUFFER_LENGTH 1024

bool verbose;

/**
 * Get the number of CPU by reading /proc/cpuinfo.
 * Also, the function creates (at least tries) the file .sensor.conf, 
 *   for the future execution of the process. 
 */
struct conf get_number_of_cpu_from_hardware(bool delay_bool, int delay){
  int nb_cpu = 0;
  FILE *f;
  char buffer[BUFFER_LENGTH];
  struct conf c;

  if((f = fopen("/proc/cpuinfo", "r")) == NULL){
    perror("An error occured while opening /proc/cpuinfo");
    exit(EXIT_FAILURE);
  }
  
  while(feof(f) == 0){
    fgets(buffer, BUFFER_LENGTH, f);
    if(strncmp("processor", buffer, (size_t)(9 * sizeof(char))) == 0){
      nb_cpu++;
    }
  }
  fclose(f);
  
  fprintf(stdout, "%d processors detected\n", nb_cpu);
  c.cpu = nb_cpu;
  c.seconds = (delay_bool) ? delay : 10;
  
  if((f = fopen(".sensor.conf", "w")) == NULL){
    perror("An error occured while opening .sensor.conf");
    fprintf(stderr, "Cannot create conf file, will retry next time\n");
  }
  else{
    fprintf(f, "processors : %d\n", c.cpu);
    fprintf(f, "number-of-seconds-between-measures : %d\n", c.seconds);
    fclose(f);
    fprintf(stdout, ".sensor.conf created\n");
  }
  
  return c;
}

/**
 * Read the number of CPUs from the .sensor.conf file.
 *
 * The .sensor.conf file is closed at the end of this function.
 */
struct conf get_number_of_cpu_from_file(bool delay_bool, int delay, FILE *f){
  int cpu = 0, seconds = 0;
  struct conf c;

  fscanf(f, "processors : %d\n", &cpu);
  fscanf(f, "number-of-seconds-between-measures : %d\n", &seconds);
  fclose(f);
  fprintf(stdout, "Done\n");

  c.cpu = cpu;
  c.seconds = (delay_bool) ? delay : seconds;
  return c;
}

/**
 * Return the number of CPUs.
 * 
 * The function reads it from the file .sensor.conf created from a previous 
 * run of the process (the number of CPUs is unlikely to change between two runs ...)
 * or it creates this .sensor.conf file using /proc/cpuinfo to get the number 
 * of CPUs.
 */
struct conf get_number_of_cpu(bool delay_bool, int delay){
  FILE *f;

  fprintf(stdout, "Trying to recover data from .sensor.conf ... ");
  errno = 0;
  f = fopen(".sensor.conf", "r");
  if(errno == ENOENT){
    fprintf(stdout, "\n.sensor.conf does not exist. Trying to get data from hardware ...\n");
    return get_number_of_cpu_from_hardware(delay_bool, delay);
  }
  else if(f == NULL){
    printf("%d", errno);
    perror("An error occured while opening file .sensor.conf");
    exit(EXIT_FAILURE);
  }
  else{
    return get_number_of_cpu_from_file(delay_bool, delay, f);
  }
}

void send_out(int average_temperature){
  int pid;
  char command[BUFFER_LENGTH];

  sprintf(command, "'AT+SEND {\"date\":%ld,\"temperature\":%d}'\n", time(NULL), average_temperature);
  pid = fork();
  if(pid < 0){
    perror("An error occured while creating the communication process");
    return;
  }
  else if(pid == 0){
    /* Let's call serialcom -c */
    execlp("./serialcom", "./serialcom", "-c", command, NULL);
  }
  else{
    /* Wait until the end of the process communication */
    while((pid = waitpid(-1, NULL, WUNTRACED|WCONTINUED)) != -1){
      ;
    }
  }
  
  printf("./serialcom -c %s", command);
}

 void process_one_temperature(int nb_cpu, int nb_seconds){
  int second_between_measure = nb_seconds;
  int nb_measures = (second_between_measure == 0) ? 1 : 60/second_between_measure;
  int limit, i;
  unsigned int raw_heat;
  double total_temperatures = 0;
  FILE *f;
  char path[BUFFER_LENGTH];
  
  for(limit = 0; limit < nb_measures; limit++){
    for(i = 0; i < nb_cpu; i++){
      sprintf(path, "/sys/class/thermal/thermal_zone%d/temp", i);
      /* We re-open all the file descriptor at each loop since it seems the temperature
	 is cached otherwise */
      if((f = fopen(path, "r")) == NULL){
	fprintf(stderr, "Unable to open ");
	perror(path);
	/* Ignore this file */
	continue;
      }
      fscanf(f, "%d", &raw_heat);
      fclose(f);
      printf("Passe %d, coeur %d : %lf\n", limit, i, ((double)raw_heat/1000.00));
      total_temperatures += (((double)raw_heat) / 1000.00);
    }
    sleep(second_between_measure);
  }

  send_out((int)(total_temperatures / (i*limit)));
}

void process_temperature(int nb_cpu, int nb_seconds){
  while(true){
    process_one_temperature(nb_cpu, nb_seconds);
  }
}

int main(int argc, char **argv){
  struct conf c;
  int nb_cpu, nb_seconds;
  bool delay_bool = false, verbose = false;
  int opt, delay = -1;

  while((opt = getopt(argc, argv, "d:v")) != -1){
    switch(opt){
    case 'd':
      delay_bool = true;
      delay = strtol(optarg, NULL, 10);
      break;
    case 'v':
      verbose = true;
      break;
    default:
      fprintf(stderr, "Unknown option : [%c] : %s\n", opt, optarg);
      return(EXIT_FAILURE);
    }
  }

  /* Redirecting output in quiet mode */
  if(verbose == false){
    if(freopen("/dev/null", "w", stdout) == NULL){
      perror("Cannot set quiet mode");
      fprintf(stderr, "Please retry in verbose mode (option -v)\n");
      return(EXIT_FAILURE);
    }
    /* Let's keep a trace of the possible errors */
    if(freopen("./sensor.error", "w", stderr) == NULL){
      perror("Cannot set quiet mode");
      fprintf(stderr, "Please retry in verbose mode (option -v)\n");
      return(EXIT_FAILURE);
    }
  }
  
  c = get_number_of_cpu(delay_bool, delay);

  nb_cpu = c.cpu;
  nb_seconds = c.seconds;

  printf("number of cpu : %d\n", nb_cpu);
  printf("number of seconds : %d\n", nb_seconds);
  if(nb_cpu <= 0){
    fprintf(stderr, "It seems your PC doesn't have any processors ...\n");
    fprintf(stderr, "There's nothing I can do, sorry :(\n");
    exit(EXIT_FAILURE);
  }
  if(nb_seconds < 0 || nb_seconds > 60){
    fprintf(stderr, "The number of seconds between each measure MUST be between 0 and 60 seconds\n");
    exit(EXIT_FAILURE);
  }
  
  process_temperature(nb_cpu, nb_seconds);
  
  return EXIT_SUCCESS;
}
