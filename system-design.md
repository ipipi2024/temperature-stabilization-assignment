# System Design: Temperature Stabilization Assignment

## 1. Overview

This document describes the system design of a distributed temperature stabilization simulation system. The system demonstrates inter-process communication (IPC) using TCP sockets, where multiple external processes coordinate with a central server to reach temperature convergence through iterative updates.

### 1.1 System Purpose
- Educational project for learning network programming and distributed systems
- Simulates a temperature stabilization algorithm across multiple processes
- Demonstrates client-server architecture using POSIX TCP sockets in C
- Implements a convergence algorithm where distributed processes reach a stable state

### 1.2 Current Status
The codebase contains starter/template code with:
- Basic TCP socket setup and connection handling
- Message structure definitions
- Single send/receive cycle implementation
- Incomplete convergence detection logic

**Required Implementations:**
- Iterative temperature update loops
- Proper convergence detection
- Signal broadcasting for system completion
- Correct temperature calculation formulas

---

## 2. System Architecture

### 2.1 High-Level Architecture

```
┌─────────────────────────────────────────────────────────────┐
│                     Temperature Stabilization System         │
│                                                               │
│   ┌───────────────────────────────────────────────────┐     │
│   │         Central Server Process (tcp_server)        │     │
│   │                                                     │     │
│   │  - TCP Socket Listener (Port 2000)                 │     │
│   │  - Connection Manager (4 clients)                  │     │
│   │  - Temperature Aggregator                          │     │
│   │  - Convergence Detector                            │     │
│   └───────────────┬───────────────────────────────────┘     │
│                   │                                           │
│        ┌──────────┼──────────┬──────────┬──────────┐        │
│        │          │          │          │          │         │
│   ┌────▼───┐ ┌───▼────┐ ┌───▼────┐ ┌───▼────┐ ┌───▼────┐  │
│   │External│ │External│ │External│ │External│ │External│  │
│   │Process │ │Process │ │Process │ │Process │ │ ...    │  │
│   │   #1   │ │   #2   │ │   #3   │ │   #4   │ │        │  │
│   │(client)│ │(client)│ │(client)│ │(client)│ │        │  │
│   └────────┘ └────────┘ └────────┘ └────────┘ └────────┘  │
│                                                               │
│   Communication: TCP/IP over localhost (127.0.0.1:2000)      │
└─────────────────────────────────────────────────────────────┘
```

### 2.2 Process Model

**Number of Processes:** 5 total
- 1 Central Server Process
- 4 External Client Processes

**Communication Pattern:** Star Topology
- Central server acts as hub
- All external processes connect to central server
- No direct communication between external processes

---

## 3. Component Design

### 3.1 Central Server (`tcp_server.c`)

**Responsibilities:**
1. Network socket management and connection establishment
2. Temperature data aggregation from all external processes
3. Central temperature calculation
4. Broadcasting updates to all clients
5. Convergence detection and system termination

**Key Functions:**

#### `establishConnectionsFromExternalProcesses()`
```c
int *establishConnectionsFromExternalProcesses()
```
- Creates TCP socket on `127.0.0.1:2000`
- Configures socket for listening (backlog=5)
- Accepts exactly 4 client connections
- Returns array of client socket file descriptors
- Prints connection information for each client

**Socket Configuration:**
- Address Family: AF_INET (IPv4)
- Socket Type: SOCK_STREAM (TCP)
- Protocol: Default TCP (0)
- Port: 2000 (network byte order)
- Address: INADDR_ANY (localhost)

#### `main()`
**Algorithm:**
1. Initialize central temperature
2. Establish connections with 4 external processes
3. Enter stabilization loop:
   - Receive temperature messages from all 4 clients
   - Calculate updated central temperature
   - Check for convergence
   - Send updated temperature to all clients
4. Cleanup and terminate

**Temperature Update Formula:**
```
centralTemp_new = (2 * centralTemp_old + T1 + T2 + T3 + T4) / 6
```
Where T1, T2, T3, T4 are temperatures from external processes 1-4.

**Convergence Check (Incomplete):**
Current implementation has placeholder logic checking if `updatedTemp == 0.0`.
Should implement proper convergence based on tolerance threshold (EPS=1e-3).

**File Location:** `tcp_server.c:1-163`

---

### 3.2 External Process (`tcp_client.c`)

**Responsibilities:**
1. Connect to central server
2. Send current temperature to server
3. Receive updated temperature from server
4. Update local temperature based on received value
5. Iterate until receiving termination signal

**Command-Line Arguments:**
```bash
./client <externalIndex> <initialTemperature>
```
- `externalIndex`: Process identifier (1-4)
- `initialTemperature`: Initial temperature value (float)

**Key Functions:**

#### `main(int argc, char *argv[])`
**Algorithm:**
1. Parse command-line arguments
2. Create TCP socket
3. Connect to server at `127.0.0.1:2000`
4. Send temperature message to server
5. Receive updated temperature from server
6. (Should implement) Update local temperature and iterate

**Temperature Update Formula (Not Yet Implemented):**
```
externalTemp_new = (3 * externalTemp_old + 2 * centralTemp) / 5
```

**Current Limitation:**
Only performs single send/receive cycle. Needs implementation of iterative loop until convergence signal received.

**File Location:** `tcp_client.c:1-72`

---

### 3.3 Utility Module (`utils.c` / `utils.h`)

**Purpose:** Common data structures and helper functions for message handling.

**Data Structures:**

#### `struct msg`
```c
struct msg {
    float T;      // Temperature value
    int Index;    // Process identifier (1-4)
};
```
- Size: 8 bytes (4 bytes float + 4 bytes int)
- Used for serialization over TCP sockets
- Sent as raw binary data (network byte order considerations needed)

**Functions:**

#### `prepare_message(int i_Index, float i_Temperature)`
```c
struct msg prepare_message(int i_Index, float i_Temperature)
```
- Creates and initializes a `struct msg` instance
- Sets Index and Temperature fields
- Returns message ready for network transmission

**File Locations:**
- Header: `utils.h:1-17`
- Implementation: `utils.c:1-22`

---

## 4. Communication Protocol

### 4.1 Network Configuration

| Parameter | Value |
|-----------|-------|
| Protocol | TCP/IP (SOCK_STREAM) |
| Transport Layer | TCP |
| Network Layer | IPv4 (AF_INET) |
| IP Address | 127.0.0.1 (localhost) |
| Port | 2000 |
| Byte Order | Network byte order (big-endian) |

### 4.2 Message Format

**Message Structure:**
```
┌─────────────┬─────────────┐
│  Float T    │  Int Index  │
│  (4 bytes)  │  (4 bytes)  │
└─────────────┴─────────────┘
     Total: 8 bytes
```

**Fields:**
- `T` (Temperature): 32-bit floating-point value
- `Index` (Process ID): 32-bit integer (1-4)

**Serialization:** Raw binary transmission of `struct msg`

### 4.3 Communication Flow

#### Phase 1: Connection Establishment
```
Server                          Client
  │                               │
  │  socket() + bind() + listen() │
  │◄──────────────────────────────│ connect()
  │  accept()                     │
  │───────────────────────────────►│ Connected
```

#### Phase 2: Temperature Exchange (Per Iteration)
```
Server                                  External Process 1-4
  │                                            │
  │◄─────────────── send(msg) ────────────────│
  │  [Receive from all 4 clients]              │
  │                                            │
  │  [Aggregate & Calculate]                   │
  │                                            │
  │──────────────── send(msg) ────────────────►│
  │  [Broadcast to all 4 clients]              │
```

#### Phase 3: Convergence & Termination (To Be Implemented)
```
Server                                  External Process
  │                                            │
  │  [Detect Convergence]                      │
  │──────────────── send(done) ───────────────►│
  │                                            │
  │  close()                            close()│
```

---

## 5. Algorithm Design

### 5.1 Central Server Algorithm

```
ALGORITHM: Central Server Temperature Stabilization

INPUT: None (waits for 4 client connections)
OUTPUT: Convergence detection and system termination

1. Initialize:
   centralTemp ← 0.0
   client_sockets[4] ← establishConnectionsFromExternalProcesses()

2. WHILE NOT converged DO:
   a. FOR i = 0 TO 3 DO:
        recv(client_sockets[i], &temperatures[i])
      END FOR

   b. oldCentralTemp ← centralTemp

   c. Calculate new central temperature:
      centralTemp ← (2 × centralTemp + Σ temperatures[i]) / 6

   d. Check convergence:
      IF |centralTemp - oldCentralTemp| < EPS THEN
         converged ← TRUE
      END IF

   e. FOR i = 0 TO 3 DO:
        send(client_sockets[i], centralTemp)
      END FOR

   f. IF converged THEN
        FOR i = 0 TO 3 DO:
           send(client_sockets[i], DONE_SIGNAL)
        END FOR
      END IF
END WHILE

3. Cleanup:
   FOR i = 0 TO 3 DO:
      close(client_sockets[i])
   END FOR

4. RETURN
```

### 5.2 External Process Algorithm

```
ALGORITHM: External Process Temperature Updates

INPUT:
  - externalIndex: Process identifier (1-4)
  - initialTemperature: Starting temperature value

OUTPUT: Converged temperature value

1. Initialize:
   externalTemp ← initialTemperature
   socket_fd ← connectToServer("127.0.0.1", 2000)

2. WHILE TRUE DO:
   a. Prepare message:
      msg ← {T: externalTemp, Index: externalIndex}

   b. Send temperature to server:
      send(socket_fd, msg)

   c. Receive response from server:
      recv(socket_fd, &response)

   d. IF response == DONE_SIGNAL THEN:
        BREAK
      END IF

   e. Update local temperature:
      centralTemp ← response.T
      oldExternalTemp ← externalTemp
      externalTemp ← (3 × externalTemp + 2 × centralTemp) / 5

   f. Print iteration info (optional):
      PRINT "Iteration: Index=", externalIndex,
            " Temp=", externalTemp
END WHILE

3. Cleanup:
   close(socket_fd)

4. RETURN externalTemp
```

### 5.3 Convergence Detection

**Tolerance Threshold:** EPS = 1e-3 (0.001)

**Convergence Criteria:**
The system is considered converged when:
1. Central temperature change: `|centralTemp_new - centralTemp_old| < EPS`
2. All external temperature changes: `|externalTemp_new - externalTemp_old| < EPS`

**Implementation Strategy:**
- Central server checks its own convergence
- Optionally: External processes could send convergence status
- Server broadcasts termination signal when all converged

---

## 6. Data Flow Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                     Initialization Phase                     │
└─────────────────────────────────────────────────────────────┘
                              │
                              ▼
              ┌───────────────────────────┐
              │  Server: Create Socket    │
              │  Bind to 127.0.0.1:2000   │
              │  Listen for connections   │
              └───────────┬───────────────┘
                          │
              ┌───────────▼───────────┐
              │ Accept 4 Connections  │
              └───────────┬───────────┘
                          │
┌─────────────────────────────────────────────────────────────┐
│                    Stabilization Loop                        │
└─────────────────────────────────────────────────────────────┘
                          │
              ┌───────────▼───────────────┐
              │  Clients Send Temps       │
              │  T1, T2, T3, T4           │
              └───────────┬───────────────┘
                          │
              ┌───────────▼────────────────────┐
              │  Server Receives 4 Messages    │
              │  Extract T1, T2, T3, T4        │
              └───────────┬────────────────────┘
                          │
              ┌───────────▼────────────────────┐
              │  Calculate Central Temp        │
              │  Tc = (2×Tc + ΣTi) / 6         │
              └───────────┬────────────────────┘
                          │
              ┌───────────▼────────────────────┐
              │  Check Convergence             │
              │  |Tc_new - Tc_old| < EPS?      │
              └───────────┬────────────────────┘
                          │
                ┌─────────┴─────────┐
                │                   │
               YES                 NO
                │                   │
                ▼                   │
    ┌───────────────────┐           │
    │ Set Done Flag     │           │
    └─────────┬─────────┘           │
              │                     │
              └─────────┬───────────┘
                        │
            ┌───────────▼───────────────┐
            │  Broadcast Tc to Clients  │
            └───────────┬───────────────┘
                        │
                ┌───────┴────────┐
                │                │
             Done?              NO
                │                │
               YES               │
                │                └───────┐
                ▼                        │
    ┌───────────────────────┐            │
    │  Send Done Signals    │            │
    │  Close Connections    │            │
    └───────────────────────┘            │
                                         │
                        ┌────────────────┘
                        │
            ┌───────────▼──────────────┐
            │  Clients Receive Tc      │
            │  Update Local Temps      │
            │  Ti = (3×Ti + 2×Tc) / 5  │
            └───────────┬──────────────┘
                        │
                        └──────► Loop Back
```

---

## 7. Technology Stack

### 7.1 Programming Language
- **C (ANSI C / C99)**
  - Systems programming language
  - Direct socket API access
  - Efficient binary data handling

### 7.2 Libraries & APIs

| Library | Purpose |
|---------|---------|
| `stdio.h` | Standard input/output operations |
| `stdlib.h` | Memory allocation, type conversions |
| `unistd.h` | POSIX API for socket operations |
| `string.h` | String manipulation |
| `sys/types.h` | Data types for system calls |
| `sys/socket.h` | Socket API (TCP/IP) |
| `arpa/inet.h` | Internet address conversions |
| `stdbool.h` | Boolean type support |

### 7.3 Build System
- **Compiler:** GCC (GNU Compiler Collection)
- **Build Commands:**
  ```bash
  gcc utils.c tcp_server.c -o server
  gcc -o client tcp_client.c utils.c
  ```
- **No automated build system** (Makefile could be added)

### 7.4 Platform Requirements
- **Operating System:** Linux/Unix/macOS (POSIX-compliant)
- **Not compatible:** Windows (without POSIX compatibility layer)
- **Alternative:** CSE4001 Docker container for standardized environment

---

## 8. System Constraints & Assumptions

### 8.1 Design Constraints
1. **Fixed Number of Processes:** Exactly 4 external processes (hardcoded)
2. **Single Machine:** All processes run on localhost
3. **Sequential Connections:** Server accepts clients sequentially (not concurrent)
4. **Blocking I/O:** Uses blocking socket operations
5. **No Authentication:** No security or authentication mechanism
6. **No Fault Tolerance:** No handling for client disconnection or crash

### 8.2 Assumptions
1. All processes start in a coordinated manner
2. Network communication is reliable (localhost TCP)
3. No network delays or packet loss (local communication)
4. Processes don't crash during execution
5. Initial temperatures are valid floating-point numbers
6. Socket operations succeed without error handling for edge cases

### 8.3 Limitations of Current Implementation
1. **No iterative loop in clients** - Single send/receive cycle only
2. **Incomplete convergence detection** - Placeholder logic
3. **No proper termination signal** - Clients don't know when to stop
4. **Limited error handling** - Minimal error checking
5. **No timeout mechanisms** - Infinite blocking on recv()
6. **Hardcoded network parameters** - No configuration file
7. **No logging or debugging output** - Minimal observability

---

## 9. Execution Model

### 9.1 System Startup Sequence

**Step 1: Start Central Server**
```bash
./server
```
- Server initializes and starts listening
- Waits for 4 client connections
- Blocks until all clients connected

**Step 2: Start External Processes (4 separate terminals)**
```bash
./client 1 100.0    # Terminal 1
./client 2 200.0    # Terminal 2
./client 3 150.0    # Terminal 3
./client 4 180.0    # Terminal 4
```
- Each client connects to server
- Sends initial temperature
- Begins stabilization process

### 9.2 Runtime Behavior

**Single Iteration Timeline:**
```
t0: Clients send temperatures to server
    ├─ Client 1: {T: 100.0, Index: 1}
    ├─ Client 2: {T: 200.0, Index: 2}
    ├─ Client 3: {T: 150.0, Index: 3}
    └─ Client 4: {T: 180.0, Index: 4}

t1: Server aggregates data
    └─ Receives all 4 messages

t2: Server calculates new central temperature
    └─ centralTemp = (2×0 + 100 + 200 + 150 + 180) / 6 = 105.0

t3: Server broadcasts update
    ├─ Send 105.0 to Client 1
    ├─ Send 105.0 to Client 2
    ├─ Send 105.0 to Client 3
    └─ Send 105.0 to Client 4

t4: Clients update local temperatures
    ├─ Client 1: (3×100 + 2×105) / 5 = 102.0
    ├─ Client 2: (3×200 + 2×105) / 5 = 162.0
    ├─ Client 3: (3×150 + 2×105) / 5 = 132.0
    └─ Client 4: (3×180 + 2×105) / 5 = 150.0

t5: Loop continues until convergence...
```

---

## 10. Implementation Roadmap

### 10.1 Current Implementation Status

✅ **Completed:**
- Basic TCP socket setup (server and client)
- Connection establishment mechanism
- Message structure definition
- Single message exchange
- Utility functions for message preparation

❌ **Not Implemented:**
- Iterative temperature update loop
- Proper convergence detection
- Termination signal mechanism
- External process temperature recalculation
- Error handling and edge cases
- Testing with multiple scenarios

### 10.2 Required Implementations

**Priority 1: Core Functionality**
1. Implement iterative loop in `tcp_server.c`:
   - Replace single recv/send with while loop
   - Add convergence check with EPS threshold
   - Send termination signal when converged

2. Implement iterative loop in `tcp_client.c`:
   - Add while loop for continuous updates
   - Implement temperature recalculation formula
   - Add termination signal handling

3. Fix temperature calculation:
   - Verify formula implementation in server
   - Implement formula in client

**Priority 2: Robustness**
1. Add error handling:
   - Check socket operation return values
   - Handle connection failures
   - Add timeout mechanisms

2. Improve convergence detection:
   - Track convergence across multiple iterations
   - Implement proper EPS-based checking

**Priority 3: Testing & Documentation**
1. Test with various initial conditions
2. Verify convergence behavior
3. Document execution results
4. Create sample output screenshots

---

## 11. Potential Improvements

### 11.1 Architecture Enhancements
1. **Configuration File:** Externalize hardcoded values (port, IP, client count)
2. **Concurrent Connections:** Use `select()`, `poll()`, or threads for simultaneous handling
3. **Dynamic Client Count:** Support variable number of external processes
4. **Distributed Deployment:** Enable running on multiple machines
5. **Message Protocol:** Add message types (DATA, ACK, DONE) for clarity

### 11.2 Code Quality
1. **Error Handling:** Comprehensive error checking and recovery
2. **Logging System:** Add structured logging for debugging
3. **Unit Tests:** Test individual components
4. **Makefile:** Automate build process
5. **Code Documentation:** Add detailed comments and function documentation

### 11.3 Features
1. **Visualization:** Real-time temperature graph
2. **Performance Metrics:** Track iteration count, convergence time
3. **Configurable Formulas:** Allow different stabilization algorithms
4. **Graceful Shutdown:** Handle SIGINT and cleanup
5. **State Persistence:** Save/restore system state

---

## 12. References & Resources

### 12.1 Documentation
- **README.md:** Assignment specifications and setup instructions
- **Beej's Guide to Network Programming:** Reference for socket programming

### 12.2 Source Files
- `tcp_server.c:1-163` - Central server implementation
- `tcp_client.c:1-72` - External process implementation
- `utils.c:1-22` - Utility functions
- `utils.h:1-17` - Message structure definitions

### 12.3 Network Parameters
- **Server Address:** 127.0.0.1
- **Server Port:** 2000
- **Protocol:** TCP/IP (IPv4)
- **Message Size:** 8 bytes

---

## 13. Glossary

| Term | Definition |
|------|------------|
| **Central Server** | Process that aggregates temperatures and broadcasts updates |
| **External Process** | Client process that reports and updates temperature |
| **Convergence** | State where temperature changes fall below threshold (EPS) |
| **EPS** | Epsilon threshold (1e-3) for detecting convergence |
| **TCP Socket** | Transmission Control Protocol communication endpoint |
| **File Descriptor** | Integer handle representing an open socket |
| **Blocking I/O** | Operation that waits until completion before returning |
| **localhost** | Loopback network interface (127.0.0.1) |
| **Port 2000** | TCP port number used for server listening |

---

## 14. Conclusion

This temperature stabilization system demonstrates fundamental concepts of distributed systems and network programming. The current implementation provides a solid foundation with basic TCP socket communication, but requires completion of the iterative stabilization loop and convergence detection mechanism.

**Key Takeaways:**
- Client-server architecture using POSIX TCP sockets
- Message-based communication with binary serialization
- Iterative algorithm for distributed convergence
- Educational focus on IPC and network programming concepts

**Next Steps:**
1. Complete the iterative update loops in both server and client
2. Implement proper convergence detection logic
3. Add termination signaling mechanism
4. Test with multiple initial temperature configurations
5. Document execution results with screenshots

The system architecture is straightforward and suitable for learning purposes, with clear separation between server coordination logic and client update logic. Once the missing implementations are completed, the system will successfully demonstrate a distributed temperature stabilization algorithm through inter-process communication.
