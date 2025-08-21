# MQTT Migration Notes

## Configuration Required

After migrating from Socket.IO to MQTT, you'll need to configure the MQTT broker:

1. **MQTT Broker Setup**: Deploy an MQTT broker (like Mosquitto) accessible from your ESP32 devices
2. **Update Broker IP**: In `src/main.cpp`, line 381, replace `wifi.getIP().c_str()` with your MQTT broker IP address
3. **Topic Structure**: The device publishes/subscribes to topics under `tapometer/{deviceMAC}/`

## MQTT Topics

### Incoming (Subscribe):
- `connect` - Connection events
- `claim_beer` - Beer claim requests  
- `disconnect` - Disconnection events
- `change_line` - Line change notifications
- `remote_sell` - Remote selling requests
- `device_info` - Device information updates
- `validated_client` - Client validation responses
- `validated_user` - User validation responses
- `disconnected_line` - Line disconnection events
- `add_emergency_card` - Emergency card additions
- `start_pour` - Start pouring commands
- `stop_pour` - Stop pouring commands
- `request_device` - Device request events

### Outgoing (Publish):
- `redeem_beer` - Beer redemption requests
- `set_up` - Setup/configuration data
- `get_worker` - Worker information requests
- `get_client` - Client information requests
- `update_status` - Status updates (volume, line state)
- `finished_pour` - Pour completion notifications
- `line_not_available` - Line unavailability notifications
- `confirm_order` - Order confirmations

## Server-Side Changes Required

Your MQTT broker and backend services will need to:
1. Handle the new topic structure instead of Socket.IO events
2. Parse the same JSON payloads (format unchanged)
3. Implement pub/sub pattern instead of event emission