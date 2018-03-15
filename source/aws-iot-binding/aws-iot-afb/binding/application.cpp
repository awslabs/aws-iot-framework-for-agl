#include "application.hpp"

/// @brief Return singleton instance of configuration object.
application_t& application_t::instance()
{
	static application_t config;
	return config;
}

std::shared_ptr<awsiotsdk::GGPubSub>& application_t::get_pubsub_client()
{
	return pubsub_client_;
}

std::vector<std::shared_ptr<afb_event>>& application_t::get_afb_event_set()
{
	return afb_event_set_;
}
