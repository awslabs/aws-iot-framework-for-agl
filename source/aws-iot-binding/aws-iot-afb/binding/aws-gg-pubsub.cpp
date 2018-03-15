#include <chrono>
#include <cstring>
#include <fstream>
#include <algorithm>
#include <json-c/json.h>
#include "aws-iot-hat.hpp"
#include "application.hpp"
#include "OpenSSLConnection.hpp"
#include "ConfigCommon.hpp"
#include "aws-gg-pubsub.hpp"

#define DISCOVER_ACTION_RETRY_COUNT 10

namespace awsiotsdk {

  bool GGPubSub::ConnectivitySortFunction(ConnectivityInfo info1, ConnectivityInfo info2) {
      if (0 > info1.id_.compare(info2.id_)) {
          return true;
      }
      return false;
  }

  ResponseCode GGPubSub::Publish(util::String topic, util::String message) {

    AFB_DEBUG("Publising message (%s) on IoT topic (%s)", message.c_str(), topic.c_str());

    ResponseCode rc;
    uint16_t packet_id = 0;

    do {
        std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(topic);
        rc = p_iot_client_->PublishAsync(std::move(p_topic_name), false, false, mqtt::QoS::QOS0,
                                         message, nullptr, packet_id);
        if (ResponseCode::SUCCESS == rc) {
            AFB_DEBUG("Publish Packet Id : %d", packet_id);
        } else if (ResponseCode::ACTION_QUEUE_FULL == rc) {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } while (ResponseCode::ACTION_QUEUE_FULL == rc);

    return rc;
  }

	ResponseCode GGPubSub::Disconnect() {
		ResponseCode rc;
		AFB_DEBUG("IoT Client Disconnecting!!");

		rc = p_iot_client_->Disconnect(ConfigCommon::mqtt_command_timeout_);
    if (ResponseCode::SUCCESS != rc) {
        AFB_INFO("Disconnect failed : %s", ResponseHelper::ToString(rc).c_str());
	  }

	  return rc;
	}

  ResponseCode GGPubSub::SubscribeCallback(util::String topic_name,
                                         util::String payload,
                                         std::shared_ptr<mqtt::SubscriptionHandlerContextData> p_app_handler_data)
  {
    AFB_DEBUG("Received message (%s) on topic (%s) ", payload.c_str(), topic_name.c_str());

    std::vector<std::shared_ptr<afb_event>>& afb_events = application_t::instance().get_afb_event_set();

    for ( auto event : afb_events )
    {
      if (std::strcmp(topic_name.c_str(), afb_event_name(*event) ) == 0)
      {
        struct json_object* json_payload = json_tokener_parse(payload.c_str());
        AFB_DEBUG("Pushing event (%s) to subscriber", topic_name.c_str());
     		afb_event_push(*event, json_payload);
      }
    }

    return ResponseCode::SUCCESS;
  }

  ResponseCode GGPubSub::DisconnectCallback(util::String client_id,
                                          std::shared_ptr<DisconnectCallbackContextData> p_app_handler_data) {
    AFB_INFO("%s Disconnected!!", client_id.c_str());
    return ResponseCode::SUCCESS;
  }

  ResponseCode GGPubSub::Subscribe(util::String topic) {
      AFB_INFO("Subscribing to IoT topic (%s)", topic.c_str());

      std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(topic);
      mqtt::Subscription::ApplicationCallbackHandlerPtr p_sub_handler = std::bind(&GGPubSub::SubscribeCallback,
                                                                                  this,
                                                                                  std::placeholders::_1,
                                                                                  std::placeholders::_2,
                                                                                  std::placeholders::_3);

      std::shared_ptr<mqtt::Subscription> p_subscription =
          mqtt::Subscription::Create(std::move(p_topic_name), mqtt::QoS::QOS0, p_sub_handler, nullptr);

      util::Vector<std::shared_ptr<mqtt::Subscription>> topic_vector;
      topic_vector.push_back(p_subscription);

      ResponseCode rc = p_iot_client_->Subscribe(topic_vector, ConfigCommon::mqtt_command_timeout_);

      std::this_thread::sleep_for(std::chrono::seconds(3));

      return rc;
  }

  ResponseCode GGPubSub::Unsubscribe(util::String topic) {
    AFB_INFO("Unsubscribing from IoT topic (%s)", topic.c_str());

    std::unique_ptr<Utf8String> p_topic_name = Utf8String::Create(topic);
    util::Vector<std::unique_ptr<Utf8String>> topic_vector;
    topic_vector.push_back(std::move(p_topic_name));

    ResponseCode rc = p_iot_client_->Unsubscribe(std::move(topic_vector), ConfigCommon::mqtt_command_timeout_);
    std::this_thread::sleep_for(std::chrono::seconds(1));

    return rc;
  }

  ResponseCode GGPubSub::Init() {
    ResponseCode rc = ResponseCode::SUCCESS;

    AFB_INFO("Initialize OpenSSL Connection ");

    std::shared_ptr <network::OpenSSLConnection> p_openssl_connection =
    std::make_shared<network::OpenSSLConnection>(ConfigCommon::endpoint_,
      ConfigCommon::endpoint_greengrass_discovery_port_,
      ConfigCommon::root_ca_path_,
      ConfigCommon::client_cert_path_,
      ConfigCommon::client_key_path_,
      ConfigCommon::tls_handshake_timeout_,
      ConfigCommon::tls_read_timeout_,
      ConfigCommon::tls_write_timeout_,
      true);
    rc = p_openssl_connection->Initialize();

    if (ResponseCode::SUCCESS != rc) {
      AFB_ERROR("Failed to initialize Network Connection with rc : %d", static_cast<int>(rc));
      return ResponseCode::FAILURE;
    } else {
      p_network_connection_ = std::dynamic_pointer_cast<NetworkConnection>(p_openssl_connection);
    }

    AFB_INFO("Run discovery to find Greengrass core endpoint to connect to");

    ClientCoreState::ApplicationDisconnectCallbackPtr p_disconnect_handler =
    std::bind(&GGPubSub::DisconnectCallback, this, std::placeholders::_1, std::placeholders::_2);

    p_iot_client_ = std::shared_ptr<GreengrassMqttClient>(GreengrassMqttClient::Create(p_network_connection_,
                                                                               ConfigCommon::mqtt_command_timeout_,
                                                                               p_disconnect_handler, nullptr));


    if (nullptr == p_iot_client_) {
      AFB_ERROR("Failed to initialize the GG MQTT Client");
      return ResponseCode::FAILURE;
    }

    std::unique_ptr <Utf8String> p_thing_name = Utf8String::Create(ConfigCommon::thing_name_);

    DiscoveryResponse discovery_response;
    int max_retries = 0;

    do {
      std::unique_ptr <Utf8String> p_thing_name = Utf8String::Create(ConfigCommon::thing_name_);
      rc = p_iot_client_->Discover(std::chrono::milliseconds(ConfigCommon::discover_action_timeout_),
      std::move(p_thing_name), discovery_response);
      if (rc != ResponseCode::DISCOVER_ACTION_SUCCESS) {
        max_retries++;
        if (rc != ResponseCode::DISCOVER_ACTION_NO_INFORMATION_PRESENT) {
          AFB_DEBUG("Discover Request failed with response code: %d.  Trying again...", static_cast<int>(rc));
          std::this_thread::sleep_for(std::chrono::seconds(5));
        } else {
          AFB_ERROR("No GGC connectivity information present for this Device: %d", static_cast<int>(rc));
          return rc;
        }
      }
    } while (max_retries != DISCOVER_ACTION_RETRY_COUNT && rc != ResponseCode::DISCOVER_ACTION_SUCCESS);

    if (max_retries == DISCOVER_ACTION_RETRY_COUNT) {
      AFB_ERROR("Discover failed after max retries, exiting");
      return rc;
    }

    AFB_DEBUG("GGC connectivity information found for this Device! %d\n", static_cast<int>(rc));

    util::String current_working_directory = ConfigCommon::GetCurrentPath();

    #ifdef WIN32
      current_working_directory.append("\\");
    #else
      current_working_directory.append("/");
    #endif

    util::String discovery_response_output_path = current_working_directory;
    discovery_response_output_path.append("discovery_output.json");
    rc = discovery_response.WriteToPath(discovery_response_output_path);

    util::Vector <ConnectivityInfo> parsed_response;
    util::Map <util::String, util::Vector<util::String>> ca_map;
    rc = discovery_response.GetParsedResponse(parsed_response, ca_map);

    // sorting in ascending order of endpoints wrt ID
    std::sort(parsed_response.begin(), parsed_response.end(), std::bind(GGPubSub::ConnectivitySortFunction,
      std::placeholders::_1,
      std::placeholders::_2));

    for (auto ca_map_itr: ca_map) {
      util::String ca_output_path_base = current_working_directory;
      ca_output_path_base.append(ca_map_itr.first);
      ca_output_path_base.append("_root_ca");
      int suffix_itr = 1;
      for (auto ca_list_itr: ca_map_itr.second) {
        util::String ca_output_path = ca_output_path_base;
        ca_output_path.append(std::to_string(suffix_itr));
        ca_output_path.append(".pem");
        std::ofstream ca_output_stream(ca_output_path, std::ios::out | std::ios::trunc);
        ca_output_stream << ca_list_itr;
        suffix_itr++;
      }
    }

    AFB_DEBUG("Found the GGC endpoint, connecting....");

    for (auto connectivity_info_itr : parsed_response) {
      p_openssl_connection->SetEndpointAndPort(connectivity_info_itr.host_address_,
        connectivity_info_itr.port_);

      auto ca_map_itr = ca_map.find(connectivity_info_itr.group_name_);

      util::String ca_output_path_base = current_working_directory;
      ca_output_path_base.append(connectivity_info_itr.group_name_);
      ca_output_path_base.append("_root_ca");
      int suffix_itr = 1;

      AFB_DEBUG("Attempting Connect with:\nGGC Endpoint : %s\nGGC Endpoint Port : %u\n",
      connectivity_info_itr.host_address_.c_str(), connectivity_info_itr.port_);

      for (auto ca_list_itr: ca_map_itr->second) {
        util::String core_ca_file_path = ca_output_path_base;
        core_ca_file_path.append(std::to_string(suffix_itr));
        core_ca_file_path.append(".pem");
        p_openssl_connection->SetRootCAPath(core_ca_file_path);

        AFB_DEBUG("Using CA at : %s\n", core_ca_file_path.c_str());

        std::unique_ptr <Utf8String> p_client_id = Utf8String::Create(ConfigCommon::base_client_id_);

        rc = p_iot_client_->Connect(ConfigCommon::mqtt_command_timeout_,
          ConfigCommon::is_clean_session_, mqtt::Version::MQTT_3_1_1,
          ConfigCommon::keep_alive_timeout_secs_, std::move(p_client_id),
          nullptr, nullptr, nullptr);
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
        if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
            break;
        }
        AFB_ERROR("Connect attempt failed with this CA with return code: %d", static_cast<int>(rc));
        suffix_itr++;
      }

      if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
        AFB_INFO("Connected to GGC %s in Group %s!!",
        connectivity_info_itr.ggc_name_.c_str(),
        connectivity_info_itr.group_name_.c_str());
        break;
      }

      AFB_ERROR("Connect attempt failed for GGC %s in Group %s!!",
      connectivity_info_itr.ggc_name_.c_str(),
      connectivity_info_itr.group_name_.c_str());
    }

    if (ResponseCode::MQTT_CONNACK_CONNECTION_ACCEPTED == rc) {
      AFB_INFO("Connected to the local GGC endpoint");
      return ResponseCode::SUCCESS;
    }

    return ResponseCode::FAILURE;
  }
}
