#include <SocketIoClient.h>

const String getEventName(const String msg) {
	return msg.substring(4, msg.indexOf("\"",4));
}

const String getEventPayload(const String msg) {
	String result = msg.substring(msg.indexOf("\"",4)+2,msg.length()-1);
	if(result.startsWith("\"")) {
		result.remove(0,1);
	}
	if(result.endsWith("\"")) {
		result.remove(result.length()-1);
	}
	return result;
}

void SocketIoClient::webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
	String msg;
	switch(type) {
        case WStype_TEXT:
            msg = String((char*)payload);
            if(msg.startsWith("42")) {
                trigger(getEventName(msg).c_str(), getEventPayload(msg).c_str(), length);
            } else if(msg.startsWith("2")) {
                _webSocket.sendTXT("3");
            } else if(msg.startsWith("40")) {
                trigger("connect", NULL, 0);
                connected = true;
            } else if(msg.startsWith("41")) {
                connected = false;
                trigger("disconnect", NULL, 0);
            }
            break;
		case WStype_DISCONNECTED:
            connected = false;
			SOCKETIOCLIENT_DEBUG("[SIoC] Disconnected!\n");
			break;
		case WStype_CONNECTED:
            connected = true;
			SOCKETIOCLIENT_DEBUG("[SIoC] Connected to url: %s\n",  payload);
			break;
		case WStype_BIN:
			SOCKETIOCLIENT_DEBUG("[SIoC] get binary length: %u\n", length);
			//hexdump(payload, length);
		break;
	}
}

void SocketIoClient::beginSSL(const char* host, const int port, const char* url, const char* fingerprint) {
	_webSocket.beginSSL(host, port, url, fingerprint);
    initialize();
}

void SocketIoClient::begin(const char* host, const int port, const char* url) {
	_webSocket.begin(host, port, url);
    initialize();
}

void SocketIoClient::initialize() {
    _webSocket.onEvent(std::bind(&SocketIoClient::webSocketEvent, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));
	_lastPing = millis();
}

void SocketIoClient::loop() {
	_webSocket.loop();
	
	// Process packets with size limit to prevent memory issues
	size_t maxProcessPerLoop = 5; // Limit processing to prevent blocking
	size_t processed = 0;
	
	for(auto packet=_packets.begin(); packet != _packets.end() && processed < maxProcessPerLoop;) {
		if(_webSocket.sendTXT(*packet)) {
			SOCKETIOCLIENT_DEBUG("[SIoC] packet \"%s\" emitted\n", packet->c_str());
			packet = _packets.erase(packet);
			processed++;
		} else {
			++packet;
		}
	}

	if(millis() - _lastPing > 5000) {
		_webSocket.sendTXT("2");
		//SOCKETIOCLIENT_DEBUG("ping \n");
		_lastPing = millis();
	}
}

void SocketIoClient::on(const char* event, std::function<void (const char * payload, size_t length)> func) {
	_events[event] = func;
}

void SocketIoClient::emit(const char* event, const char * payload) {
	// Prevent message queue overflow
	const size_t MAX_QUEUE_SIZE = 20;
	if (_packets.size() >= MAX_QUEUE_SIZE) {
		SOCKETIOCLIENT_DEBUG("[SIoC] Message queue full, dropping oldest packet\n");
		_packets.erase(_packets.begin()); // Remove oldest message
	}
	
	String msg = String("42[\"");
	msg += event;
	msg += "\"";
	if(payload) {
		msg += ",";
		msg += payload;
	}
	msg += "]";
	SOCKETIOCLIENT_DEBUG("[SIoC] add packet %s\n", msg.c_str());
	_packets.push_back(msg);
}

void SocketIoClient::trigger(const char* event, const char * payload, size_t length) {
	auto e = _events.find(event);
	if(e != _events.end()) {
		SOCKETIOCLIENT_DEBUG("[SIoC] trigger event %s\n", event);
		e->second(payload, length);
	} else {
		SOCKETIOCLIENT_DEBUG("[SIoC] event %s not found. %d events available\n", event, _events.size());
	}
}

void SocketIoClient::disconnect()
{
    connected = false;
	_webSocket.disconnect();
	trigger("disconnect", NULL, 0);
}

void SocketIoClient::setAuthorization(const char * user, const char * password) {
    _webSocket.setAuthorization(user, password);
}
