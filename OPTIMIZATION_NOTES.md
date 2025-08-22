# WiFi and Socket Communication Optimizations for Pouring Operations

## Problem
The ESP32 was experiencing crashes during pouring operations due to excessive socket communication, particularly when sending frequent status updates to the server.

## Root Cause Analysis
1. **Frequent Status Updates**: Status updates were sent every 2-5% increase, resulting in 20-50 messages per pour
2. **Unbounded Message Queue**: SocketIoClient used unlimited vector storage for pending messages
3. **Resource Competition**: Socket task and WiFi reconnection competed with critical pouring operations
4. **Memory Pressure**: High frequency messaging could cause memory fragmentation and stack overflow

## Optimizations Implemented

### 1. Rate Limiting for Status Updates
- **Normal Operation**: Max 1 update every 500ms
- **Pouring Operation**: Max 1 update every 2 seconds
- **Duplicate Prevention**: Skip updates with same value during pouring
- **Reduced Frequency**: Increased percentage incrementor from 2-5% to 5-10%

### 2. Message Queue Management
- **Queue Size Limit**: Maximum 10 pending messages in SocketIO wrapper
- **Client Queue Limit**: Maximum 20 messages in SocketIoClient library
- **Overflow Protection**: Drop oldest messages when queue is full
- **Processing Limit**: Process max 5 messages per loop cycle

### 3. Resource Protection During Pouring
- **WiFi Reconnection**: Disabled during active pouring (`screen.isServing`)
- **Socket Connection Checks**: Suspended during pouring operations
- **Task Scheduling**: Increased socket task delay from 100ms to 250ms during pouring

### 4. State Management
- **Pouring Mode**: `setPouring(true/false)` to control optimization behavior
- **Automatic Cleanup**: Restore normal operation when pouring ends
- **Multiple Exit Points**: Handle all pouring termination scenarios

## Code Changes Summary

### Modified Files:
1. `src/SocketComm.h` - Added optimization methods and rate limiting variables
2. `src/SocketComm.cpp` - Implemented rate limiting and queue management
3. `src/main.cpp` - Integrated pouring state management and resource protection
4. `lib/SocketIoClient/SocketIoClient.h` - Added queue management methods
5. `lib/SocketIoClient/SocketIoClient.cpp` - Implemented overflow protection

### Key Methods Added:
- `SocketIO::setPouring(bool)` - Control optimization mode
- `SocketIO::isPouring()` - Check current mode
- `SocketIoClient::getPendingMessageCount()` - Monitor queue size
- `SocketIoClient::clearMessageQueue()` - Emergency queue clear

## Performance Impact

### Before Optimization:
- Status updates: Every 2-5% (20-50 messages per pour)
- Socket task frequency: 100ms
- WiFi reconnection: Continuous during pouring
- Queue management: None (unbounded growth)

### After Optimization:
- Status updates: Every 5-10% with 2s minimum interval (5-10 messages per pour)
- Socket task frequency: 250ms during pouring
- WiFi reconnection: Suspended during pouring
- Queue management: Limited to 10-20 messages max

## Expected Results
- **Reduced CPU Load**: 60-80% fewer status messages during pouring
- **Memory Stability**: Bounded message queues prevent memory issues
- **Improved Reliability**: Critical operations protected from interruption
- **Maintained Functionality**: Server still receives sufficient status updates

## Backward Compatibility
All changes are backward compatible. Normal operation (non-pouring) maintains existing behavior with minimal performance impact.