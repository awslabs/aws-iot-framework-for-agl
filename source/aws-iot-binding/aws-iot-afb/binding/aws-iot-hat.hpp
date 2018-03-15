#pragma once

#include <cstddef>
#include <string>
#include <systemd/sd-event.h>
#include "ConfigCommon.hpp"

extern "C"
{
	#define AFB_BINDING_VERSION 2
	#include <afb/afb-binding.h>
};

void subscribe(struct afb_req request);
void unsubscribe(struct afb_req request);
void publish(struct afb_req request);
void disconnect(struct afb_req request);
