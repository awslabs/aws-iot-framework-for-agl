#pragma once

#include "mqtt/GreengrassMqttClient.hpp"
#include "NetworkConnection.hpp"
#include "discovery/DiscoveryResponse.hpp"

namespace awsiotsdk {
  class GGPubSub {
    protected:
      std::shared_ptr <NetworkConnection> p_network_connection_;
      std::shared_ptr <GreengrassMqttClient> p_iot_client_;

      static bool ConnectivitySortFunction(ConnectivityInfo info1, ConnectivityInfo info2);
      ResponseCode SubscribeCallback(util::String topic_name,
        util::String payload,
        std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data);
      ResponseCode DisconnectCallback(util::String topic_name,
        std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data);
        
    public:
      ResponseCode Publish(util::String topic, util::String message);
      ResponseCode Subscribe(util::String topic);
      ResponseCode Unsubscribe(util::String topic);
      ResponseCode Init();
      ResponseCode Disconnect();
  };
}
