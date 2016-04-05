#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>

typedef enum{false = 0, true = 1} bool;

#define BUFFER_LENGTH 1024

int son_pid = -1;
bool verbose;

/**
 * Get the number of CPU by reading /proc/cpuinfo.
 * Also, the function creates (at least tries) the file .sensor.conf, 
 *   for the future execution of the process. 
 */
int get_number_of_cpu_from_hardware(){
  int nb_cpu = 0;
  FILE *f;
  char buffer[BUFFER_LENGTH];

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
  if((f = fopen(".sensor.conf", "w")) == NULL){
    perror("An error occured while opening .sensor.conf");
    fprintf(stderr, "Cannot create conf file, will retry next time\n");
  }
  else{
    fprintf(f, "processors : %d", nb_cpu);
    fclose(f);
    fprintf(stdout, ".sensor.conf created\n");
  }
  
  return nb_cpu;
}

/**
 * Read the number of CPUs from the .sensor.conf file.
 *
 * The .sensor.conf file is closed at the end of this function.
 */
int get_number_of_cpu_from_file(FILE *f){
  int ret = 0;

  fscanf(f, "processors : %d", &ret);
  fclose(f);
  fprintf(stdout, "Done\n");
  return ret;
}

/**
 * Return the number of CPUs.
 * 
 * The function reads it from the file .sensor.conf created from a previous 
 * run of the process (the number of CPUs is unlikely to change between two runs ...)
 * or it creates this .sensor.conf file using /proc/cpuinfo to get the number 
 * of CPUs.
 */
int get_number_of_cpu(){
  FILE *f;

  fprintf(stdout, "Trying to recover data from .sensor.conf ... ");
  errno = 0;
  f = fopen(".sensor.conf", "r");
  if(errno == ENOENT){
    fprintf(stdout, "\n.sensor.conf does not exist. Trying to get data from hardware ...\n");
    return get_number_of_cpu_from_hardware();
  }
  else if(f == NULL){
    printf("%d", errno);
    perror("An error occured while opening file .sensor.conf");
    exit(EXIT_FAILURE);
  }
  else{
    return get_number_of_cpu_from_file(f);
  }
}

void send_out(int average_temperature){
  int pid;
  char command[BUFFER_LENGTH];

  sprintf(command, "'AT+SEND {\"date\":%ld,\"temperature\":%d}'\n", time(NULL), average_temperature);
  son_pid = pid = fork();
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
    while((pid = waitpid(son_pid, NULL, WUNTRACED|WCONTINUED)) != -1){
      ;
    }
  }
  
  printf("./serialcom -c %s", command);
  son_pid = -1;
}

void process_one_temperature(int nb_cpu){
  int second_between_measure = 10;
  int nb_measures = 60/second_between_measure;
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

void process_temperature(int nb_cpu){
  int toto = 10;

  while(toto >= 0){
    process_one_temperature(nb_cpu);
    toto--;
  }
}

int main(int argc, char **argv){
  FILE **f;
  int i, j, nb_cpu;
  char buffer[BUFFER_LENGTH];

  nb_cpu = get_number_of_cpu();

  printf("number of cpu : %d\n", nb_cpu);
  if(nb_cpu <= 0){
    fprintf(stderr, "It seems your PC doesn't have any processors ...\n");
    fprintf(stderr, "There's nothing I can do, sorry :(\n");
    exit(EXIT_FAILURE);
  }

  /*
  if((f = (FILE**)malloc(nb_cpu * (sizeof(FILE *)))) == NULL){
    sprintf(buffer, "Unable to allocate %d cpus\n", nb_cpu);
    perror(buffer);
    exit(EXIT_FAILURE);
  }

  for(i = 0; i < nb_cpu; i++){
    sprintf(buffer, "/sys/class/thermal/thermal_zone%d/temp", i); 
    if((f[i]=fopen(buffer, "r")) == NULL){
      fprintf(stderr, "Unable to open ");
      perror(buffer);
      for(j = 0; j < i; j++){
	fclose(f[j]);
      }
      free(f);
      exit(EXIT_FAILURE);
    }
  }
  */

  process_temperature(nb_cpu);

  /*
  for(i = 0; i < nb_cpu; i++){
    fclose(f[i]);
  }
  
  free(f);
  */
  
  return EXIT_SUCCESS;
}
