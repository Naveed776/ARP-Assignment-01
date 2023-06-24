// Naveed Manzoor Afridi	| 5149575 |
// Abdul Rauf		        | 5226212 |
// Group Name		        | NR007   |

#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/select.h>

// FILE *log_file;

// Define random error
const float RANDOM_ERROR = 0.003; // 0.3%

// Define maximum and minimum positions
const float MIN_XPOS = 0.0;
const float MAX_XPOS = 40.0;
const float MIN_ZPOS = 0.0;
const float MAX_ZPOS = 10.0;

// Variable to store the error
int error = 0;

// variable to store the time for writing to the log file
int count = 0;

// File descriptor for log file
int log_filedes;

// Buffer to store the log message
char log_buffer[100];

// Function to write to the log file

int write_in_log(const char *to_write, char type) {
    // Log that in command_console process button has been pressed with date and time
    time_t t = time(NULL);
    struct tm *tm = localtime(&t);

    // Create a string for the type of log
    const char *log_type;
    switch (type) {
        case 'e':
            log_type = "error occurs";
            break;
        case 'x':
            log_type = "X position of hoist";
            break;
        case 'z':
            log_type = "Z position of hoist";
            break;
        default:
            log_type = "unknown";
    }

    // Open the log file in append mode
    FILE *log_file = fopen("log.txt", "a");
    if (log_file == NULL) {
        perror("Error opening log file");
        return 2;
    }

    // Use fprintf to format the log message
    int ret = fprintf(log_file, "%02d:%02d:%02d: World process %s: %s\n", tm->tm_hour, tm->tm_min, tm->tm_sec, log_type, to_write);
    if (ret < 0) {
        perror("Error writing to log file");
        fclose(log_file);
        return 2;
    }

    // Close the log file
    fclose(log_file);

    return 0;
}



// Function to get random number between two integers
int get_random(int a, int b) {
    return (rand() % (b - a + 1)) + a;
}

// Function to add the random error to the position
float random_error(float pos) {
    // Calculate the error range
    int error = (int)(pos * RANDOM_ERROR * 100); // Multiply by 100 to avoid floating-point errors

    // Calculate the minimum and maximum values
    int min = (int)(pos * 100) - error;
    int max = (int)(pos * 100) + error;

    // Generate a random value between the minimum and maximum
    int random_value = rand() % (max - min + 1) + min;

    // Return the random value divided by 100
    return (float)random_value / 100;
}

// Function to pick a random number between two integers
int any_random(int a, int b) {
    // Calculate the midpoint
    int midpoint = (a + b) / 2;

    // Calculate the absolute difference between midpoint and a, and between midpoint and b
    int diff_a = abs(midpoint - a);
    int diff_b = abs(midpoint - b);

    // Return the value that is closer to the midpoint
    if (diff_a < diff_b) {
        return a;
    } else {
        return b;
    }
}


// Function to read and return the real position of the axis
float real_pos(int *fd, char axis) {
    // Variable to store the position
    char pos[10];

    // Read the position from the file
    if (read(*fd, pos, 10) == -1) {
        // If error, return -1.0
        return -1.0; // Return a float to avoid errors
    }

    // Store the value in the real position variable and add the random error
    float real_pos = random_error(atof(pos));

    // Define the limits based on the axis
    float min_pos, max_pos;
    if (axis == 'x') {
        min_pos = MIN_XPOS;
        max_pos = MAX_XPOS;
    }
    else if (axis == 'z') {
        min_pos = MIN_ZPOS;
        max_pos = MAX_ZPOS;
    }
    else {
        // Invalid axis, return the original position
        return real_pos;
    }

    // Clamp the real position within the limits
    if (real_pos < min_pos) {
        real_pos = min_pos;
    }
    else if (real_pos > max_pos) {
        real_pos = max_pos;
    }

    // Return the real position
    return real_pos;
}


// Function to get the maximum between two integers
int max(int a, int b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}


int main(int argc, char const *argv[]){
    // Open log file 
    if ((log_filedes = open("log/log_world.txt", O_WRONLY | O_APPEND | O_CREAT, 0666)) == -1){
        // If error occurs while opening the log file
        exit(errno);
    }

    // Variables to store the real values of the positions
    float real_x_pos = 0.0;
    float real_z_pos = 0.0;

    // File descriptors
    int fdx_pos, fdz_pos, fd_inspx, fd_inspz;

    // FIFOs locations
    char *x_pos_fifo = "/tmp/x_world";
    char *z_pos_fifo = "/tmp/z_world";
	char *fifo_inspx = "/tmp/inspx";
	char *fifo_inspz = "/tmp/inspz";

    // Create the FIFOs
    mkfifo(x_pos_fifo, 0666);
    mkfifo(z_pos_fifo, 0666);
	mkfifo(fifo_inspx, 0666);
	mkfifo(fifo_inspz, 0666);

    // Open the FIFOs for reading and writing
    if ((fdx_pos = open(x_pos_fifo, O_RDWR)) == -1){
        // If error occurs while opening the FIFO 
        // Log the error 
        int ret = write_in_log(strerror(errno), 'e');

        // Close the log file 
        close(log_filedes);
        if (ret){
            // If error occurs while writing on log file
            exit(errno);
        }
        exit(1);
    }

    if ((fdz_pos = open(z_pos_fifo, O_RDWR)) == -1){
        // If error occurs while opening the FIFO
        // Log the error
        int ret = write_in_log(strerror(errno), 'e');

        // Close the file descriptors
        close(fdx_pos);
        close(log_filedes);
        if (ret){
            // If error occurs while writing on log file
            exit(errno);
        }
        exit(1);
    }

    if((fd_inspx = open(fifo_inspx, O_WRONLY)) == -1){
        // If error occurs while opening the FIFO
        // Log the error
        int ret = write_in_log(strerror(errno), 'e');

        // Close the file descriptors
        close(fdx_pos);
        close(fdz_pos);
        close(log_filedes);
        if (ret){
            // If error occurs while writing on log file
            exit(errno);
        }
        exit(1);
    }

    if((fd_inspz = open(fifo_inspz, O_WRONLY)) == -1){
        // If error occurs while opening the FIFO
        // Log the error
        int ret = write_in_log(strerror(errno), 'e');

        // Close the file descriptors
        close(fdx_pos);
        close(fdz_pos);
        close(log_filedes);
        if (ret){
            // If error occurs while writing on log file
            exit(errno);
        }
        exit(1);
    }

    // initialize times for write on the log file
    time_t current_time;
    time_t last_log_time = 0;

    // infinite loop
    while (1){
        // Variable to store the read values
        char x_pos[10];
        char z_pos[10];

        // Setting parameters for select function
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(fdx_pos, &readfds);
        FD_SET(fdz_pos, &readfds);
        int max_fd = max(fdx_pos, fdz_pos) + 1;

        // Setting timeout for select function
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100000; // 100 ms

        // Wait for the file descriptor to be ready
        int ready = select(max_fd, &readfds, NULL, NULL, &timeout);
        // if select returns 0 it means that the timeout has expired and no file descriptor is ready
        // we can continue the loop and check again

        // Check if the file descriptor is ready
        if (ready > 0){
            // If both file descriptors are ready
            if (FD_ISSET(fdx_pos, &readfds) && FD_ISSET(fdz_pos, &readfds)){

                // Randomly select one of the file descriptors
                if (any_random(fdx_pos, fdz_pos) == fdx_pos){

                    // Read and store the value in the x position variable
                    real_x_pos = real_pos(&fdx_pos, 'x');
                }
                else{

                    // Read and store the value in the z position variable
                    real_z_pos = real_pos(&fdz_pos, 'z');
                }
            }
            // If only the x position file descriptor is ready
            else if (FD_ISSET(fdx_pos, &readfds)){

                // Read and store the value in the x position variable
                real_x_pos = real_pos(&fdx_pos, 'x');
            }
            // If only the z position file descriptor is ready
            else if (FD_ISSET(fdz_pos, &readfds)){

                // Read and store the value in the z position variable
                real_z_pos = real_pos(&fdz_pos, 'z');
            }

            // If error occurs while reading the position
            if (real_x_pos == -1.0 || real_z_pos == -1.0){

                error = 1;
                break;
            }
            
            char x_pos[20];
            char z_pos[20];
            sprintf(x_pos, "%f", real_x_pos);
            sprintf(z_pos, "%f", real_z_pos);

            // Write the real position to the FIFO
            int m = write(fd_inspx, x_pos, strlen(x_pos));
            if (m == -1 || m != strlen(x_pos)){
                // If error occurs while writing on the FIFO
                error = 1;
                break;
            }

            // Write the real position to the FIFO
            int n = write(fd_inspz, z_pos, strlen(z_pos));
            if (n == -1 || n != strlen(z_pos)){
                // If error occurs while writing on the FIFO
                error = 1;
                break;
            }

            // Get the current time
            time(&current_time);

            // write the position to the log file only one time every more than 25 seconds
            // to avoid to write too many times on the log file
            if (difftime(current_time, last_log_time) > 25){

                // write the position to the log file
                int ret = write_in_log(x_pos, 'x');
                if (ret){
                    // If error occurs while writing on log file
                    error = 2;
                    break;
                }

                // write the position to the log file
                ret = write_in_log(z_pos, 'z');
                if (ret){
                    // If error occurs while writing on log file
                    error = 2;
                    break;

                    // Update the last log time
                    last_log_time = current_time;
                }
            }

        }
        else if (ready < 0){
            // If error occurs while waiting for the file descriptor
            error = 1;
            break;
        }
    }

    // Close the FIFOs
    close(fdx_pos);
    close(fdz_pos);
    close(fd_inspx);
    close(fd_inspz);

    if (error == 1){
        // If error occurs while reading the position
        // Log the error
        int ret = write_in_log(strerror(errno), 'e');

        // Close the log file
        close(log_filedes);
        if (ret){
            // If error occurs while writing on log file
            exit(errno);
        }
        exit(1);
    }

    // Close the log file
    close(log_filedes);

    if (error == 2){
        // If error occurs while writing on log file
        exit(errno);
    }

    exit(0);
}
