#pragma once

#include <cstddef>
#include <string>
#include "aws-iot-hat.hpp"
#include "aws-gg-pubsub.hpp"

///
/// @brief Class representing a configuration attached to the binding.
///
///  It will be the reference point to needed objects.
///
class application_t
{
	private:
		std::shared_ptr<awsiotsdk::GGPubSub> pubsub_client_ = std::shared_ptr<awsiotsdk::GGPubSub>(new awsiotsdk::GGPubSub());
		std::vector<std::shared_ptr<afb_event> > afb_event_set_;

	public:
		static application_t& instance();

		std::shared_ptr<awsiotsdk::GGPubSub>& get_pubsub_client();
		std::vector<std::shared_ptr<afb_event>>& get_afb_event_set();		
};
