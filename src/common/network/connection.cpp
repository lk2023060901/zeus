#include "common/network/connection.h"
#include "common/network/network_logger.h"
#include "common/network/network_events.h"

namespace common {
namespace network {

Connection::Connection(boost::asio::any_io_executor executor, const std::string& connection_id)
    : executor_(executor), connection_id_(connection_id) {
    
    NETWORK_LOG_DEBUG("Connection created with ID: {}", connection_id_);
}

Connection::~Connection() {
    StopHeartbeat();
    NETWORK_LOG_DEBUG("Connection destroyed: {}", connection_id_);
}

void Connection::AsyncSend(const std::string& data, SendCallback callback) {
    std::vector<uint8_t> binary_data(data.begin(), data.end());
    AsyncSend(binary_data, callback);
}

void Connection::SetHeartbeat(bool enable, uint32_t interval_ms) {
    heartbeat_enabled_ = enable;
    heartbeat_interval_ms_ = interval_ms;
    
    if (enable && IsConnected()) {
        StartHeartbeat();
    } else {
        StopHeartbeat();
    }
    
    NETWORK_LOG_DEBUG("Heartbeat {} for connection {}, interval: {}ms", 
                     enable ? "enabled" : "disabled", connection_id_, interval_ms);
}

void Connection::UpdateState(ConnectionState new_state) {
    ConnectionState old_state = state_.exchange(new_state);
    
    if (old_state != new_state) {
        NETWORK_LOG_DEBUG("Connection {} state changed: {} -> {}", 
                         connection_id_, static_cast<int>(old_state), static_cast<int>(new_state));
        
        // Handle heartbeat based on state
        if (new_state == ConnectionState::CONNECTED && heartbeat_enabled_) {
            StartHeartbeat();
        } else if (new_state != ConnectionState::CONNECTED) {
            StopHeartbeat();
        }
        
        // Fire state change event
        NetworkEvent event(NetworkEventType::CONNECTION_ESTABLISHED);
        event.connection_id = connection_id_;
        event.endpoint = GetRemoteEndpoint();
        event.protocol = GetProtocol();
        event.connection = shared_from_this();
        
        if (new_state == ConnectionState::CONNECTED) {
            event.type = NetworkEventType::CONNECTION_ESTABLISHED;
        } else if (new_state == ConnectionState::DISCONNECTED || new_state == ConnectionState::ERROR) {
            event.type = NetworkEventType::CONNECTION_CLOSED;
        }
        
        FireEvent(event);
        
        // Call user handler
        if (state_change_handler_) {
            try {
                state_change_handler_(old_state, new_state);
            } catch (const std::exception& e) {
                NETWORK_LOG_ERROR("Exception in state change handler for connection {}: {}", 
                                connection_id_, e.what());
            }
        }
    }
}

void Connection::HandleDataReceived(const std::vector<uint8_t>& data) {
    // Update statistics
    stats_.bytes_received.fetch_add(data.size());
    stats_.messages_received.fetch_add(1);
    stats_.last_activity = std::chrono::steady_clock::now();
    
    NETWORK_LOG_TRACE("Data received on connection {}: {} bytes", connection_id_, data.size());
    
    // Fire data received event
    NetworkEvent event(NetworkEventType::DATA_RECEIVED);
    event.connection_id = connection_id_;
    event.endpoint = GetRemoteEndpoint();
    event.protocol = GetProtocol();
    event.connection = shared_from_this();
    event.data = data;
    event.bytes_transferred = data.size();
    
    FireEvent(event);
    
    // Call user handler
    if (data_handler_) {
        try {
            data_handler_(data);
        } catch (const std::exception& e) {
            NETWORK_LOG_ERROR("Exception in data handler for connection {}: {}", 
                            connection_id_, e.what());
        }
    }
}

void Connection::HandleError(boost::system::error_code error) {
    stats_.errors_count.fetch_add(1);
    
    NETWORK_LOG_ERROR("Error on connection {}: {} ({})", 
                     connection_id_, error.message(), error.value());
    
    // Update state to error
    UpdateState(ConnectionState::ERROR);
    
    // Fire error event
    NetworkEvent event(NetworkEventType::CONNECTION_ERROR);
    event.connection_id = connection_id_;
    event.endpoint = GetRemoteEndpoint();
    event.protocol = GetProtocol();
    event.connection = shared_from_this();
    event.error_code = error;
    event.error_message = error.message();
    
    FireEvent(event);
    
    // Call user handler
    if (error_handler_) {
        try {
            error_handler_(error);
        } catch (const std::exception& e) {
            NETWORK_LOG_ERROR("Exception in error handler for connection {}: {}", 
                            connection_id_, e.what());
        }
    }
}

void Connection::FireEvent(const NetworkEvent& event) {
    try {
        NetworkEventManager::Instance().FireEventAsync(event, executor_);
    } catch (const std::exception& e) {
        NETWORK_LOG_ERROR("Failed to fire network event for connection {}: {}", 
                         connection_id_, e.what());
    }
}

void Connection::UpdateStats() {
    stats_.last_activity = std::chrono::steady_clock::now();
}

void Connection::StartHeartbeat() {
    if (!heartbeat_enabled_ || heartbeat_interval_ms_ == 0) {
        return;
    }
    
    StopHeartbeat(); // Stop existing timer
    
    heartbeat_timer_ = std::make_unique<boost::asio::steady_timer>(executor_);
    
    auto self = shared_from_this();
    heartbeat_timer_->expires_after(std::chrono::milliseconds(heartbeat_interval_ms_));
    heartbeat_timer_->async_wait([self](boost::system::error_code ec) {
        if (!ec && self->IsConnected() && self->heartbeat_enabled_) {
            self->SendHeartbeat();
            self->StartHeartbeat(); // Schedule next heartbeat
        }
    });
    
    NETWORK_LOG_TRACE("Started heartbeat for connection {}", connection_id_);
}

void Connection::StopHeartbeat() {
    if (heartbeat_timer_) {
        try {
            heartbeat_timer_->cancel();
        } catch (const boost::system::system_error& e) {
            NETWORK_LOG_WARN("Failed to cancel heartbeat timer: {}", e.what());
        }
        heartbeat_timer_.reset();
        
        NETWORK_LOG_TRACE("Stopped heartbeat for connection {}", connection_id_);
    }
}

} // namespace network
} // namespace common