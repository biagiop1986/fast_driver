# fast_driver

A fast Linux driver based on single-producer single-consumer ring buffers to control a sample accelerator.  

## 1 Architecture

### 1.1 kernel space driver

Kernel space and user space communicate through two single-producer single-consumer ring buffers deeply inspired by io_uring: a **submit** ring buffer and a **completion** ring buffer.  
Each user-space process has its own pair of ring buffers, so to avoid any contention between different processes. These are constructed once the process opens the device file and destroyed once it closes it. Ring buffers are stored into a hashtable indexed by proc-id.  
The **submit** ring buffer is used by a user-space process to enqueue commands (producer) and by the kernel to retrieve them (consumer). The **completion** ring buffer is used in a specular way: the kernel enqueus the output from executed commands (producer) and the user-space process reads them (consumer).  
Ring buffers are made available to user processes by memory-mapping them into the user memory space.  
The core of the kernel space driver is a kernel thread that waits in a waiting queue for commands to arrive. The notification to wake the thread up comes from a **ioctl** system call and must be called from the user-space once one or more commands have been enqueued in the **submit** ring buffer. Once the thread wakes up, it cycles over the **submit** ring buffers from all the processes and executes their commands. These are executed in sequence by draining each ring buffer, so commands from different processes do not interleave. This is necessary to ensure that computations that semantically require more than one command are executed completely without other processes' commands to interfere.

### 1.2 user space driver

User space driver is a C++17 **Accelerator** class that exposes some API methods to user space programs and communicates with the underlying kernel space drivers.  
The class embodies the RAII principle as it opens the device file when it is constructed and closes it when it is destroyed.  
It exposes two methods to submit commands and two to wait for command completion. In the "submit" methods, a parameter pack of **control** objects or a vector of **control** objects are used as input: each **control** object encapsulates the command intended for the accelerator and a few parameters. These are copied in sequence to the **submit** ring buffer and a signal to the kernel thread is sent at the end through a **ioctl** system call. The "wait" methods are blocking calls that fill a parameter pack of **output** objects or a vector of **output** objects as soon as its corresponding commands are executed and the outputs are ready in the **completion** ring buffer. 

## 2 Installation

From the main folder of the driver, execute the following commands on your terminal to create the device file and install the kernel space driver:
```
cd kernel_space_driver
make
sudo ./install_module.sh
```
Compile the user-space driver shared object:
```
cd user_space_driver
make
```
In order to leverage the driver functionalities in user-space programs, include the "user_driver.h" header and link the "libuser_driver.so" shared object into your programs. See any Makefile in any folder under "samples" to look at an example. Remember to add the shared object location to LD_LIBRARY_PATH - e.g., from user_space_driver:
```
export LD_LIBRARY_PATH=$(pwd):${LD_LIBRARY_PATH}
```

## 3 Customization

The driver can be customized to be used with real devices. The **control** and **output** structures can be changed in **common/common.h** header file. A quick-to-achieve modification is to change the maximum number of input parameters associated to a command by modifying the **PARAM_MAX** macro. The same can be done for output values by altering the **OUTPUT_MAX** macro. In both cases, **SUBMIT_RING_BUFFER_SIZE** and **COMPLETE_RING_BUFFER_SIZE** macros in **user_space_driver/user_driver.h** must be updated with the new sizes of the two ring buffers defined in **kernel_space_driver/fast_driver.c**.  
The **execute_command** function in **kernel_space_driver/fast_driver.c** file is where the input is processed and the output is produced. Here, the input data could be forwarded to a real device, possibly by copying them to memory-mapped registers and the output data retrieved accordingly.

## 4 Fairness

Currently, when the kernel thread is woken up in a **ioctl** system call, it scans the hashtable to retrieve **submit** ring buffers associated to the user processes that have work to do. These are retrieved by proc-id following whatever order is given to them by the **hash_for_each** macro. For this reason, the processes are not served in the same order as they signalled the kernel thread.  
A possible way to serve the processes in order is to add another ring buffer where the proc-id of signaling processes are enqueued. The wait condition in the **wait_event** macro is updated to let the kernel thread wake up as long as there are unprocessed signals in the ring buffer.    
This addition would improve the fairness of the driver, since processes would be served in FIFO order, but it would introduce an inter-process synchronization point in the newly added ring buffer.
