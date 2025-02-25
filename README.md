# aesd-assignments
This repo contains public starter source code, scripts, and documentation for Advanced Embedded Software Development (ECEN-5713) and Advanced Embedded Linux Development assignments University of Colorado, Boulder.

## Setting Up Git

Use the instructions at [Setup Git](https://help.github.com/en/articles/set-up-git) to perform initial git setup steps. For AESD you will want to perform these steps inside your Linux host virtual or physical machine, since this is where you will be doing your development work.

## Setting up SSH keys

See instructions in [Setting-up-SSH-Access-To-your-Repo](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-SSH-Access-To-your-Repo) for details.

## Specific Assignment Instructions

Some assignments require further setup to pull in example code or make other changes to your repository before starting.  In this case, see the github classroom assignment start instructions linked from the assignment document for details about how to use this repository.

## Testing

The basis of the automated test implementation for this repository comes from [https://github.com/cu-ecen-aeld/assignment-autotest/](https://github.com/cu-ecen-aeld/assignment-autotest/)

The assignment-autotest directory contains scripts useful for automated testing  Use
```
git submodule update --init --recursive
```
to synchronize after cloning and before starting each assignment, as discussed in the assignment instructions.

As a part of the assignment instructions, you will setup your assignment repo to perform automated testing using github actions.  See [this page](https://github.com/cu-ecen-aeld/aesd-assignments/wiki/Setting-up-Github-Actions) for details.

Bhakti Ramani

# Socket Server Implementation

## Overview
This project implements a socket server in C that handles client connections, stores received data, and provides echo functionality. The server listens on port 9000, accepts incoming connections, and manages data transfer between clients and a persistent storage file.

## Features
- TCP Socket Server listening on port 9000
- Support for daemon mode operation
- Signal handling (SIGTERM and SIGINT)
- Dynamic buffer management for received data
- Persistent data storage in `/var/tmp/aesdsocketdata`
- IPv4 and IPv6 client support
- Comprehensive error handling and logging
- Clean resource management

## Technical Specifications

### Build Requirements
- GCC compiler
- Linux environment
- Required headers:
  - stdio.h
  - sys/socket.h
  - sys/types.h
  - netdb.h
  - arpa/inet.h
  - syslog.h
  - string.h
  - stdlib.h
  - stdbool.h
  - signal.h
  - unistd.h
  - fcntl.h
  - sys/stat.h

### Compilation
```bash
gcc -o aesdsocket aesdsocket.c -Wall -Werror
```

## Usage

### Starting the Server
Normal mode:
```bash
./aesdsocket
```

Daemon mode:
```bash
./aesdsocket -d
```

### Server Operation
1. The server creates a socket and binds to port 9000
2. Listens for incoming connections (max 20 in queue)
3. Creates `/var/tmp/aesdsocketdata` for data storage
4. Handles client connections and data:
   - Accepts connection and logs client IP
   - Receives data until newline character
   - Stores data in socket data file
   - Sends complete file contents back to client
   - Continues accepting new connections

### Signal Handling
- SIGINT (Ctrl+C): Graceful shutdown
- SIGTERM: Graceful shutdown
- During shutdown:
  - Closes all open sockets
  - Closes and truncates data file
  - Frees allocated memory
  - Closes syslog
  - Exits cleanly

### Daemon Mode
When started with `-d` flag:
- Forks and parent exits
- Creates new session
- Changes working directory to root
- Redirects standard file descriptors to /dev/null
- Continues operation in background

## Implementation Details

### Data Reception
- Initial buffer size: 1024 bytes
- Dynamic buffer expansion when needed
- Receives data until newline character found
- Handles partial receives
- Proper memory management with error handling

### Data Storage
- All received data appended to `/var/tmp/aesdsocketdata`
- File opened with O_RDWR | O_APPEND | O_CREAT flags
- Permissions set to 0666
- Uses fdatasync to ensure data persistence

### Data Transmission
- Reads complete file content
- Sends to client in chunks
- Handles partial sends
- Manages EINTR interruptions
- Proper error handling and recovery

### Logging
Uses syslog for the following events:
- Socket creation/binding
- Client connections/disconnections
- Error conditions
- Daemon operations
- Cleanup operations

## Error Handling
Comprehensive error handling for:
- Socket operations
- Memory allocation
- File operations
- Client communications
- Signal handling
- Daemon creation

## Known Limitations
1. Fixed initial buffer size (1024 bytes)
2. No configuration file for server parameters
3. Single-threaded implementation
4. No SSL/TLS support

## Security Considerations
1. File permissions set to 0666 (consider restricting)
2. No authentication mechanism
3. No encryption for data in transit
4. No rate limiting implemented
5. No input validation beyond newline detection

## Testing
Test the server using netcat:
```bash
echo "Hello, World" | nc localhost 9000
```

Expected behavior:
1. Server accepts connection
2. Receives "Hello, World\n"
3. Stores data in /var/tmp/aesdsocketdata
4. Sends all stored data back to client
5. Logs connection details to syslog

## Debugging
- Check syslog for operational messages:
```bash
tail -f /var/log/syslog | grep aesdsocket
```
- Monitor socket data file:
```bash
tail -f /var/tmp/aesdsocketdata
```

## Memory Management
- All dynamically allocated memory is properly freed
- Buffer management uses realloc for growth
- Cleanup function ensures no memory leaks
- Error paths include proper memory cleanup


