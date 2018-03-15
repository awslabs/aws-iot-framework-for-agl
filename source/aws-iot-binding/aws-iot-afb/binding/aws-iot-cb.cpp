#include "aws-iot-hat.hpp"
#include "application.hpp"
#include <json-c/json.h>
#include <systemd/sd-event.h>
#include <cstring>


static int make_subscription_unsubscription(struct afb_req request, struct afb_event event, bool subscribe)
{
  awsiotsdk::ResponseCode rc;
  const std::string& topic(afb_event_name(event));
  std::shared_ptr<awsiotsdk::GGPubSub> pubsub_client = application_t::instance().get_pubsub_client();

  if (subscribe) {
    AFB_DEBUG("subscribe to topic: %s", topic.c_str());
    rc = pubsub_client->Subscribe(topic);
  } else {
    AFB_DEBUG("unsubscribe from topic: %s", topic.c_str());
    rc = pubsub_client->Unsubscribe(topic);
  }

  if (awsiotsdk::ResponseCode::SUCCESS == rc) {
    if (((subscribe ? afb_req_subscribe : afb_req_unsubscribe)(request, event)) < 0)
    {
      AFB_ERROR("Failed to subscribe/unscribe the afb_event : %s", topic.c_str());
      return -1;
    }
  } else {
    AFB_ERROR("Failed to subscribe/unscribe to the AWS IoT topic: %s", topic.c_str());
    return -1;
  }

  return 0;
}

static int one_subscribe_unsubscribe(struct afb_req request, bool subscribe, const std::string& topic)
{
  std::vector<std::shared_ptr<afb_event>>& afb_events = application_t::instance().get_afb_event_set();

  AFB_DEBUG("Searching for afb_event: %s in the list", topic.c_str());

  for ( auto event : afb_events ) {
    if (std::strcmp(topic.c_str(), afb_event_name(*event) ) == 0) {
      AFB_DEBUG("Found afb_event: %s", topic.c_str());
      //return make_subscription_unsubscription(request, *event, subscribe);
      return 0;
    }
  }

  AFB_DEBUG("Making new afb_event: %s", topic.c_str());
  struct afb_event _event = afb_daemon_make_event(topic.c_str());

  if (!afb_event_is_valid(_event))
  {
    AFB_ERROR("Can't create an event for %s, something went wrong.", topic.c_str());
    return -1;
  }

  AFB_DEBUG("Adding new afb_event: %s to the list", topic.c_str());
  afb_events.push_back(std::make_shared<afb_event>(_event));
  return make_subscription_unsubscription(request, _event, subscribe);
}

static void do_subscribe_unsubscribe(struct afb_req request, bool subscribe)
{
  int rc, n, i, rc2;
  struct json_object *args, *event, *x;

  args = afb_req_json(request);
  rc = -1;

  if (args != NULL)
  {
    json_object_object_get_ex(args, "topic", &event);

    if (json_object_is_type(event, json_type_string)) {
      rc = one_subscribe_unsubscribe(request, subscribe, json_object_get_string(event));
    }
    else if (json_object_is_type(event, json_type_array)) {
      rc = 0;
      n = json_object_array_length(event);
      for (i = 0 ; i < n ; i++)
      {
        x = json_object_array_get_idx(event, i);
        rc2 = one_subscribe_unsubscribe(request, subscribe, json_object_get_string(x));
        if (rc >= 0)
        rc = rc2 >= 0 ? rc + rc2 : rc2;
      }

    }

    if (rc >= 0)
      afb_req_success(request, NULL, NULL);
    else
      afb_req_fail(request, "error", NULL);
  }
  else
    afb_req_fail(request, "error", NULL);
}


void publish(struct afb_req request)
{
  AFB_DEBUG("publish called");

  awsiotsdk::ResponseCode rc;
  struct json_object* args = nullptr,
  *topic = nullptr,
  *message = nullptr;

  args = afb_req_json(request);

  if (args != NULL &&
    (json_object_object_get_ex(args, "topic", &topic) && json_object_is_type(topic, json_type_string) ) &&
    (json_object_object_get_ex(args, "message", &message) ))
    {

      std::string topic_str = json_object_get_string(topic);
      std::string message_str;

      if (json_object_is_type(message, json_type_string))
      {
        message_str = json_object_get_string(message);
      } else  if (json_object_is_type(message, json_type_object)){
        message_str = json_object_to_json_string(message);
      }

      std::shared_ptr<awsiotsdk::GGPubSub> pubsub_client = application_t::instance().get_pubsub_client();
      rc = pubsub_client->Publish(topic_str, message_str);

      if (awsiotsdk::ResponseCode::SUCCESS == rc)
        afb_req_success(request, NULL, NULL);
      else
        afb_req_fail(request, "error", NULL);
    }
    else
    {
      AFB_ERROR("Invalid JSON payload format. It is missing topic and/or message tag(s).");
      afb_req_fail(request, "error", NULL);
    }
}

void subscribe(struct afb_req request)
{
	do_subscribe_unsubscribe(request, true);
}

void unsubscribe(struct afb_req request)
{
	do_subscribe_unsubscribe(request, false);
}

void disconnect(struct afb_req request)
{
  awsiotsdk::ResponseCode rc;
  AFB_DEBUG("disconnect called");

  std::shared_ptr<awsiotsdk::GGPubSub> pubsub_client = application_t::instance().get_pubsub_client();
  rc = pubsub_client->Disconnect();

  if (awsiotsdk::ResponseCode::SUCCESS == rc)
    afb_req_success(request, NULL, NULL);
  else
    afb_req_fail(request, "error", NULL);

}
