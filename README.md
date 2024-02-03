# Hashtable Opertaions using a Client-Server Architecture

This program creates 2 processes:
* Server process: Processes hash table requests from the client and updates the hashtable accordingly.
* Client process: Sends requests to the server.

The server and client are synchronized with the use of a **named semaphore** and share the data via a **shared memory region**. The semaphore is incremented by the client after successfully writing a message into the shared memory region and decremented by the server after reading a message. Thus, the value of the semaphore represents the number of messages still in the queue.

### Operations

The client can request the following operations from the server:
* `INSERT val` - insert a value into the hashtable.
* `DELETE val` - delete a value from the hashtable.
* `FIND val` - search for a value in the hashtable.
* `PRINT` - print the current state of the hashtable.
* `QUIT` - tell the server to quit.

### Shared memory buffer

The shared memory buffer is used to pass messages between the server and the client. When the client reaches the end of the buffer, it waits for the server to finish processing all in-flight requests before starting writing from the beginning of the buffer.

### Server internals

The server reads the messages from the queue and starts a thread for each operation. Read-write locks are used for each hashtable bucket to ensure that race conditions are avoided. 

However, the server guarantees that the requests from the buffer will be processed in the order in which the client wrote them; therefore, the server waits until the recently spawned thread acquires the read-write lock for its bucket before spawning the next operation.

Furthermore, the server will spawn threads only for `INSERT`, `DELETE` and `FIND` operations. `QUIT` and `PRINT` messages will cause the server to wait until all the currently executing threads have finished.

The hashtable resolves collisions by chaining elements in a linked list.

### Assumptions / Simplifications

The following assumptions have been made:
* There exist only 1 server and 1 client interacting with each other.
* The messages are 8 bytes long (4 bytes for the operation and 4 bytes for the payload).
* All messages have the same size, therefore `QUIT` and `PRINT` messages will have an empty payload.
* The performance of the hashing function is beyond the scope of this project. The server uses a very simple hashing function that is not optimized to reduce collisions. However, custom hashing functions can easily be provided to the hashtable.

NOTE: To simulate variable workloads per thread, each thread will spin a random number of times before executing the actual hashtable instruction. 

## Installation

After cloning the repository, compile the code using the Makefile

```
make
```

Then, open 2 terminals. Start the server first in one terminal, and then start the client in the other terminal.

Terminal 1:
```
./server <hashtable-size>
```

Terminal 2:
```
./client
```

Once both processes have been started, the client will send a few predefined messages and then tell the server to quit. 

