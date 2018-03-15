#include "aws-iot-hat.hpp"
#include "application.hpp"

extern "C"
{
  static int init();

  static const struct afb_verb_v2 verbs[]=
  {
    { .verb= "subscribe", .callback= subscribe, .auth= NULL, .info="Let subscribe to topics", .session= AFB_SESSION_NONE},
    { .verb= "unsubscribe", .callback= unsubscribe, .auth= NULL, .info="Let unsubscribe topics", .session= AFB_SESSION_NONE},
    { .verb= "publish", .callback= publish, .auth= NULL, .info="Publish message to topic", .session= AFB_SESSION_NONE},
    { .verb= "disconnect", .callback= disconnect, .auth= NULL, .info="Disconnect from IoT", .session= AFB_SESSION_NONE},
    { .verb= NULL, .callback= NULL, .auth= NULL, .info=NULL, .session= 0}
  };

  const struct afb_binding_v2 afbBindingV2 {
    .api = "aws-iot-service",
    .specification = NULL,
    .info = "API to pubish and subscribe message on topics on AWS IoT",
    .verbs = verbs,
    .preinit = NULL,
    .init = init,
    .onevent = NULL,
    .noconcurrency = 0
  };

  static int init()
  {
    AFB_INFO("Loading the AWS IoT client config...");
    awsiotsdk::ResponseCode rc = awsiotsdk::ConfigCommon::InitializeCommon("data/aws-iot-service-config/config/config.json");
    if (awsiotsdk::ResponseCode::SUCCESS == rc) {
      std::shared_ptr<awsiotsdk::GGPubSub> pubsub_client = application_t::instance().get_pubsub_client();
      AFB_INFO("Inititalizing the AWS IoT....");

      if (awsiotsdk::ResponseCode::SUCCESS == rc) {
        rc = pubsub_client->Init();

        if (awsiotsdk::ResponseCode::SUCCESS == rc) {
          AFB_INFO("AWS IoT client initialized");
          return 0;
        }
      }
    }
    AFB_ERROR("Failed to initialize AWS IoT client, return code: %d", static_cast<int>(rc));
    return static_cast<int>(rc);
  }
};
