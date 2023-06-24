// Naveed Manzoor Afridi	| 5149575 |
// Abdul Rauf		        | 5226212 |
// Group Name		        | NR007   |

#include <stdio.h>
#include <string.h> 
#include <fcntl.h> 
#include <sys/stat.h> 
#include <sys/types.h> 
#include <unistd.h> 
#include <stdlib.h>
#include <errno.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>


// Variable to the time of the whatchdog
int const SLEEP_TIME = 60; // 60 seconds

// PIDs variables
pid_t pidCmd;
pid_t pidMotorX;
pid_t pidMotorZ;
pid_t pidWorld;
pid_t pidinspection;

// Variable for the status of the child processes
int status;

// function to fork and create a child process


pid_t spawn(const char *program, char *arg_list[]) {
  pid_t child_pid = fork();

  if (child_pid == -1) {
    perror("Error while forking...");
    return -1;
  }

  if (child_pid != 0) {
    return child_pid;
  }

  execvp(program, arg_list);
  perror("Error while executing...");
  return -1;
}


int generate_log_files() {
  const char *log_files[] = {
    "./log/log_command.txt",
    "./log/log_motor_x.txt",
    "./log/log_motor_z.txt",
    "./log/log_world.txt",
    "./log/log_inspection.txt"
  };

  for (int i = 0; i < sizeof(log_files) / sizeof(log_files[0]); i++) {
    if (remove(log_files[i]) != 0) {
      perror("Error while removing log files...");
    }
  }

  int fds[sizeof(log_files) / sizeof(log_files[0])] = {0};

  for (int i = 0; i < sizeof(log_files) / sizeof(log_files[0]); i++) {
    fds[i] = open(log_files[i], O_CREAT | O_RDWR, 0666);
    if (fds[i] == -1) {
      perror("Error while creating log files...");
      for (int j = 0; j < i; j++) {
        close(fds[j]);
      }
      return 1;
    }
    close(fds[i]);
  }

  return 0;
}


// function to kill all child processes
void kill_processes() {
  pid_t pids[] = {pidCmd, pidMotorX, pidMotorZ, pidWorld, pidinspection};
  int num_pids = sizeof(pids) / sizeof(pids[0]);

  for (int i = 0; i < num_pids; i++) {
    if (pids[i] > 0) {
      kill(pids[i], SIGKILL);
    }
  }
}

time_t last_modification(const char *file_name) {
  struct stat attrib;

  return (stat(file_name, &attrib) == 0) ? attrib.st_mtime : -1;
}


// watchdog function
// it configure allarm signal with 60 seconds of timeout
// and if the child process is not alive, it kills all child processes
// it also check if the child processes are writing on the log files
// if they are not, it kills all child processes                        
int main() {
  // File descriptor for the log file
  int log_fd;

  // create the log file where write all happenings to the program
  log_fd = open("./log/log_master.txt", O_WRONLY | O_APPEND | O_CREAT, 0666);
  if (log_fd == -1) {
    // if the system call fails, it returns -1
    return -1;
  }

  // variable to write on the log file
  char buffer[100];

  // char variable in order to convert the PID of the processes to string
  char pidMaster[10];
  char pidMotorXstr[10];
  char pidMotorZstr[10];

  // log that the MASTER process has started at the current time
  time_t now = time(NULL);
  struct tm t = *localtime(&now);
  sprintf(buffer, "Master process has been started at %d:%d:%d\n", t.tm_hour, t.tm_min, t.tm_sec);
  write(log_fd, buffer, strlen(buffer));

  // check if there are errors while writing on the log file
  if (errno != 0) {
    // if there are errors, print the error message
    fprintf(stderr, "MASTER: There was error while writing on log file: %s\n", strerror(errno));
    close(log_fd);
    return -1;
  }

  // start the COMMAND CONSOLE process passing the PID of the MASTER process as argument
  char *arg_list_command[] = {"/usr/bin/konsole", "-e", "./bin/command_console", NULL};
  pidCmd = spawn("/usr/bin/konsole", arg_list_command);

  if (pidCmd == -1) {
    // if the spawn() function fails, it returns -1
    goto spawn_error;
  }

  // start the MOTOR_X process
  char *arg_list_motor_x[] = {"./bin/motor_x", NULL};
  pidMotorX = spawn("./bin/motor_x", arg_list_motor_x);

  if (pidMotorX == -1) {
    // if the spawn() function fails, it returns -1
    goto spawn_error;
  }

  // convert the PID of the MOTOR_X process to string
  sprintf(pidMotorXstr, "%d", pidMotorX);

  // start the MOTOR_Z process
  char *arg_list_motor_z[] = {"./bin/motor_z", NULL};
  pidMotorZ = spawn("./bin/motor_z", arg_list_motor_z);

  if (pidMotorZ == -1) {
    // if the spawn() function fails, it returns -1
    goto spawn_error;
  }

  // convert the PID of the MOTOR_Z process to string
  sprintf(pidMotorZstr, "%d", pidMotorZ);

  // start the WORLD process
  char *arg_list_world[] = {"./bin/world", NULL};
  pidWorld = spawn("./bin/world", arg_list_world);

  if (pidWorld == -1) {
    // if the spawn() function fails, it returns -1
    goto spawn_error;
  }

  // start the INSPECTION process
  char *arg_list_insp[] = {"/usr/bin/konsole", "-e", "./bin/inspection_console", pidMotorXstr, pidMotorZstr, NULL};
  pidinspection = spawn("/usr/bin/konsole", arg_list_insp);

  if (pidinspection == -1) {
    // if the spawn() function fails, it returns -1
    goto spawn_error;
  }

  // write on the log file the time when the child processes have been started
  now = time(NULL);
  t = *localtime(&now);
  sprintf(buffer, "All processes have been started at %d:%d:%d\n", t.tm_hour, t.tm_min, t.tm_sec);
  write(log_fd, buffer, strlen(buffer));

  // chekc if there are errors while writing on the log file
  if (errno != 0) {
    // if there are errors, print the error message
    fprintf(stderr, "MASTER: There was error while writing on log file: %s\n", strerror(errno));
    close(log_fd);
    return -1;
  }

  // if there are NOT errors while starting the child processes, the MASTER process continues its execution
  goto no_spawn_error;

// if there are errors while starting the child processes, the MASTER process terminates
spawn_error:
  // Print error message
  fprintf(stderr, "MASTER: There was error while starting the child processes... \n");
  // kill all child processes
  kill_processes();
  close(log_fd);
  return -1;

// if there are NOT errors while starting the child processes, the MASTER process continues its execution
no_spawn_error:
  // if the spawn() function is successful, the software is running normally
  // create all log files
  int spy = generate_log_files();

  if (spy == -1) {
    // if the generate_log_files() function fails, it returns -1
    // Print error message
    fprintf(stderr, "MASTER: Error while creating the log files... \n");
    // kill all child processes
    kill_processes();
    close(log_fd);
    return -1;
  }

  // set an array of the name of all log files
  char *log_array[5] = {
      "./log/log_command.txt",
      "./log/log_motor_x.txt",
      "./log/log_motor_z.txt",
      "./log/log_world.txt",
      "./log/log_inspection.txt"};

  // set an array of the PID of all child processes
  pid_t pid_array[5] = {pidCmd, pidMotorX, pidMotorZ, pidWorld, pidinspection};

  // FLAG to check if the file is modified
  // if it's 1, it means that the file is not modified
  // if it's 0, it means that the file is modified in the last SLEEP_TIME seconds
  int flag_mod = 0;

  // variable to store the time of the last modification of the file
  time_t last_mod;

  // set the variable to store the changing time
  int seconds_since_mod = 0;

  // infinite loop
  while (1) {
    // get the current time
    time_t now = time(NULL);

    // for every log file check if it was modified in the last SLEEP_TIME/2 seconds
    for (int i = 0; i < 5; i++) {
      // get the last modification time of the log file
      last_mod = last_modification(log_array[i]);

      // if the last_mod is -1, it means that the system call failed
      if (last_mod == -1) {
        // kill all child processes
        kill_processes();
        // Print the error message
        fprintf(stderr, "MASTER: Error while getting the LAST MODIFICATION TIME of the file %s in the watchdog function.\n", log_array[i]);
        return -1;
      }

      if (difftime(now, last_mod) > SLEEP_TIME / 2) {
        // if the file was not modified
        flag_mod += 1;
      }
    }

    // if the file was not modified increase the seconds since the last modification
    if (flag_mod > 0) {
      // if the file was not modified
      seconds_since_mod++;

      // set the flag to 0
      flag_mod = 0;
    } else {
      // if the file was modified, set the seconds since the last modification to 0
      seconds_since_mod = 0;
    }

    for (int i = 0; i < 5; i++) {
      // if the child process is not alive
      if (waitpid(pid_array[i], &status, WNOHANG) == pid_array[i]) {
        // kill all child processes
        kill_processes();
        // Print error message and child process exit status
        fprintf(stderr, "MASTER: Error the CHILD PROCESS is NOT ALIVE in the watchdog function.\n");
        return -1;
      }
    }
    // if the file was not modified for more than SLEEP_TIME/2 seconds (
    //  add to the SLEEP_TIME/2 seconds of the alarm it results in SLEEP_TIME seconds)
    if (seconds_since_mod > SLEEP_TIME / 2) {
      // kill all child processes
      kill_processes();

      return 1;
    }

    // sleep for 1 second
    sleep(1);
  }

  // close the log file
  close(log_fd);

  return 0;
}
