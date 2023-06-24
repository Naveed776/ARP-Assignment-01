# ARP Assignment No. 01
	Naveed Manzoor Afridi	| 5149575 |
	Abdul Rauf		| 5226212 |
	Group Name		| NR007   |

 ## GITHUB
 https://github.com/Naveed776/ARP-Assignment-01
 
## Description of the programs
The code to design, develop, test and deploy is an interactive simulator  of hoist with 2 d.o.f, in which two different consoles allow the user to activate the hoist.
In the octagonal box there are two motors mx and mz, which displace the hoist along the two respective axes. Motions along axes have their bounds, say 0 - max_x and 0 - max_z.

The hoist is controlled by a command window in which there are 6 clickable buttons:

- **_Vx+_** and **_Vx-_** to increment and decrement the speed along the horizontal axis
- **_Vz+_** and **_Vz-_** to increment and decrement the speed along the vertical axis
- two **_STP_** buttons to set the velocity along the two axis to zero

## Folders content

The repository is organized as follows:
- the `src` folder contains the source code for all the processes
- the `include` folder contains all the data structures and methods used within the ncurses framework to build the two GUIs

After compiling the program other two directories will be created:

- the `bin` folder contains all the executable files
- the `log` folder will contain all the log files of the processes after the program will be executed

##Processes
The simulator requires (at least) the following 5 processes:

> watchdog: it checks the previous 4 processes periodically, and sends a reset (like the R button) in case all processes did nothing (no computation, no motion, no input/output) for a certain time, say, 60 seconds.

> command console: reading the 6 commands, using mouse to click push buttons.

> motor x: simulating the motion along x axis, receiving command and sending to the world the position x.

> motor z: similar to motor x.

> world: receiving the position x and z and sending the real time position including simulated errors.

> inspection console: receiving from world the hoist positions while moving, and reporting on the screen; the inspection console manages the S ad R buttons as well (simulated with red and orange buttons).

## Requirements
The program requires the installation of the **konsole** program and of the **ncurses** library. To install the konsole program, simply open a terminal and type the following command:
```console
$ sudo apt-get install konsole
```
To install the ncurses library, type the following command:
```console
$ sudo apt-get install libncurses-dev
```
## How to compile and run it
I added two file .sh in order to simplify compiling and running all the processes.  
To compile it: execute ```compile.sh```;  
To run it: execute ```run.sh```. 
