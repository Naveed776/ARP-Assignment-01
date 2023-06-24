// Naveed Manzoor Afridi	| 5149575 |
// Abdul Rauf		        | 5226212 |
// Group Name		        | NR007   |


#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h>

// Unique flag for RESET and STOP
// 0 = no flag
// 1 = reset flag
// 2 = stop flag
int flag = 0;

// Variable to store the errors
// 0 = no error
// 1 = system call error
// 2 = error while writing on log file
volatile int error = 0;

// Constant to store the minimum x position
const float X_MIN = 0.0;
// Constant to store the maximum x position
const float X_MAX = 39.0;
// Constant to store the abs velocity
int const VELOCITY = 2;

// File descriptor for log file
int log_fd;
// File descriptor for pipes
int fd_vx, fdx_pos;

// Variables to store position
float x_pos = 0.0;
// Variable to store velocity
int vx = 0;

// Buffer to store the log message
char log_buffer[100];
// Function to write on log
int write_log(const char* to_write, char type) {
    // Log the button press in the command_console process with the current time
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // Create a string for the log type
    const char* log_type = NULL;

    switch (type) {
        case 'e':
            log_type = "error";
            break;
        case 'i':
            log_type = "info";
            break;
        case 's':
            log_type = "signal received";
            break;
        case 'c':
            log_type = "velocity change";
            break;
        case 'm':
            log_type = "velocity bounds";
            break;
        default:
            return 2;  // Return an error code for unsupported log types
    }

    // Use snprintf to format the log message
    int written = snprintf(log_buffer, sizeof(log_buffer), "%02d:%02d:%02d: Motor Z process %s: %s\n",
                           tm.tm_hour, tm.tm_min, tm.tm_sec, log_type, to_write);

    if (written < 0 || written >= sizeof(log_buffer)) {
        return 2;  // Return an error code for buffer overflow or snprintf failure
    }

    // Write and check for errors
    if (write(log_fd, log_buffer, written) == -1) {
        return 2;  // Return an error code for write failure
    }

    return 0;  // Return success code
}



// Function to handle the signals
void myhandler(int signo) {
    const char* logMsg = "";
    int flagValue = 0;

    switch (signo) {
        case SIGUSR1:
            logMsg = "STOP";
            flagValue = 2;
            break;
        case SIGUSR2:
            logMsg = "RESET";
            flagValue = 1;
            break;
        default:
            return;
    }

    // Set the flag
    flag = flagValue;

    // Write log message
    if (error = write_log(logMsg, 's')) {
        return;
    }

    // Set signal handlers
    if (signal(SIGUSR1, myhandler) == SIG_ERR || signal(SIGUSR2, myhandler) == SIG_ERR) {
        // If an error occurs while setting the signal handler, log the error
        error = write_log(strerror(errno), 'e');
        return;
    }
}


// Function to reset the machine after a signal
void reset_for_signal(int flag_sig){
    // If 'reset' signal, reset the position and velocity
    if (flag_sig == 1){
        flag = 0;
        vx = 0;
        x_pos = 0.0;
    }
    // If 'stop' signal, stop the motor
    else if (flag_sig == 2){
        flag = 0;
        vx = 0;
    }
}

// Function to check max e min velocity
int check_velocity(int velocity) {
    const char* write_to;
    int return_value = velocity;

    if (velocity > VELOCITY) {
        write_to = "MAXIMUM SPEED";
        return_value = VELOCITY;
    } else if (velocity < -VELOCITY) {
        write_to = "MINIMUM SPEED";
        return_value = -VELOCITY;
    }

    write_log(write_to, 'm');
    return return_value;
}


// Function to check max e min position
float check_position(float position) {
    float new_position = position;

    if (position > X_MAX) {
        new_position = X_MAX;
        vx = 0;
    }
    else if (position < X_MIN) {
        new_position = X_MIN;
        vx = 0;
    }

    return new_position;
}

// Function to open a file and log the error
int open_and_log_on_error(const char* file, int flags, const char* operation) {
    int fd = open(file, flags);
    if (fd == -1) {
        write_log(strerror(errno), 'e');
        close(log_fd);
        exit(errno);
    }
    return fd;
}

// Function to signal and log the error
void signal_and_log_on_error(int signum, void (*handler)(int), const char* operation) {
    if (signal(signum, handler) == SIG_ERR) {
        write_log(strerror(errno), 'e');
        close(log_fd);
        close(fd_vx);
        close(fdx_pos);
        exit(errno);
    }
}

int main(int argc, char const *argv[]){
    // variable to know if the velocity is changed: we confront directly this string with the one read from the pipe
    char * Vp = "i";
    char * Vm = "d";
    char * Vzero = "s";

    // Open the log file
    if ((log_fd = open("log/log_motor_x.txt", O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1){
        // If error occurs while opening the log file
        exit(errno);
    }

    // Create the FIFOs
    char *vx_fifo = "/tmp/velx";
    char *x_pos_fifo = "/tmp/x_world";
    mkfifo(vx_fifo, 0666);
    mkfifo(x_pos_fifo, 0666);

    // Open the FIFOs
    int fd_vx = open_and_log_on_error(vx_fifo, O_RDWR, "opening vx_fifo");
    int fdx_pos = open_and_log_on_error(x_pos_fifo, O_WRONLY, "opening x_pos_fifo");
    // Set the signal handlers
    signal_and_log_on_error(SIGUSR1, myhandler, "setting signal handler for SIGUSR1");
    signal_and_log_on_error(SIGUSR2, myhandler, "setting signal handler for SIGUSR2");

    // Variable to change the velocity/position
    float delta_x;
    float new_x_pos;
    // Loop until handler_error is set to true
    while (!error){
        // Set the file descriptors to be monitored
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fd_vx, &readfds);

        // Set the timeout
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 ms

        // Wait for the file descriptor to be ready
        int ready = select(fd_vx + 1, &readfds, NULL, NULL, &timeout);

        // Check if the file descriptor is ready
        if (ready > 0)
        {
            // Read the velocity increment
            char str[2];
            int nbytes;
            if ((nbytes = read(fd_vx, str, 2)) == -1)
            {
                // If error occurs while reading from the FIFO
                error = 1;
                break;
            }
            // Need to add the null character at the end of the string to be able to compare it
            str[nbytes - 1] = '\0';
            
            // Confront the string with the possible commands
            if (strcmp(str, Vp) == 0){
                vx += 1;
                vx = check_velocity(vx);
                // If velocity is changed, log the change
                char write_to[100] = "SPEED INCREASING";
                int ret = write_log(write_to, 'c');
            } 
            else if (strcmp(str, Vm) == 0){
                vx -= 1;
                vx = check_velocity(vx);
                // If velocity is changed, log the change
                char write_to[100] = "SPEED DECREASING";
                int ret = write_log(write_to, 'c');
            } 
            else if (strcmp(str, Vzero) == 0){
                vx = 0;
            }
            else{
                // If the string is not recognized
                error = 1;
                break;
            }
        }
        // Error handling
        else if (ready < 0 && errno != EINTR)
        {
            // If error occurs while waiting for the file descriptor to be ready
            error = 1;
            break;
        }

        // Update the position
        delta_x = vx * 0.25;
        new_x_pos = x_pos + delta_x;

        // Check if the position is out of bounds
        new_x_pos = check_position(new_x_pos);

        // Check if the position has changed
        if (new_x_pos != x_pos)
        {
            // Update the position
            x_pos = new_x_pos;
            // Write the position
            char x_pos_str[10];
            sprintf(x_pos_str, "%f", x_pos);
            if ((write(fdx_pos, x_pos_str, strlen(x_pos_str) + 1)) == -1)
            {
                // If error occurs while writing to the FIFO
                error = 1;
                break;
            }
        }

        // If reset signal was received,
        if (flag == 1){
            // char to send to the FIFO
            char x_pos_str[10];
            // Reset the position
            while (x_pos >= X_MIN){
                vx = -VELOCITY;
                x_pos += vx * 0.1;
                // do not allow the position to go below X_MIN 
                // we do not want to write to the FIFO
                if (x_pos <= X_MIN){
                    break;
                }

                // Write the position
                sprintf(x_pos_str, "%f", x_pos);
                if ((write(fdx_pos, x_pos_str, strlen(x_pos_str) + 1)) == -1){
                    // If error occurs while writing to the FIFO
                    error = 1;
                    break;
                }

                // Wait for 0.5 seconds to simulate the motor
                sleep(0.5);
            }

            // Reset the machine and write to the pipe
            reset_for_signal(flag);

            // Write the position
            sprintf(x_pos_str, "%f", x_pos);
            if ((write(fdx_pos, x_pos_str, strlen(x_pos_str) + 1)) == -1){
                // If error occurs while writing to the FIFO
                error = 1;
                break;
            }
        }

        // If stop signal was received
        if (flag == 2){
            // Reset the machine
            reset_for_signal(flag);
        }
    }

    // Close the FIFOs
    close(fd_vx);
    close(fdx_pos);

    if (error == 1)
    {
        // If error occurs while reading the position, log the error
        int ret = write_log(strerror(errno), 'e');
        // Close the log file
        close(log_fd);
        if (ret)
        {
            // If error occurs while writing on log file
            exit(errno);
        }
        exit(1);
    }
    
    // Close the log file
    close(log_fd);

    if(error == 2){
        // If error occurs while writing on log file
        exit(errno);
    }

    exit(0);
}
