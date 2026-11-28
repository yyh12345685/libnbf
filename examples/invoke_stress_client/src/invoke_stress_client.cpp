#include "invoke_stress_client.h"

#include <cstdlib>
#include <unistd.h>

#include "client/client_mgr.h"
#include "common/rate_limiter.h"
#include "message.h"

namespace {

LOGGER_STATIC_DECL_IMPL(logger, "root");

std::atomic<uint64_t> in_flight{0};
std::atomic<uint64_t> completed{0};
std::atomic<uint64_t> failed{0};

uint64_t ReadEnvUnsigned(const char* name, uint64_t default_value) {
  const char* value = std::getenv(name);
  if (value == nullptr || *value == '\0') {
    return default_value;
  }
  return std::strtoull(value, nullptr, 10);
}

void RecordInvokeResult(bdf::EventMessage* message, const char* metric_name) {
  if (message == nullptr) {
    failed.fetch_add(1);
    in_flight.fetch_sub(1);
    return;
  }

  if (message->status == bdf::MessageBase::kStatusOK) {
    completed.fetch_add(1);
  } else {
    failed.fetch_add(1);
    // Callbacks run on the service-handler threads: one log line per failure
    // slows them down exactly when failures pile up.
    static bdf::common::RateLimiter failure_log;
    uint64_t suppressed = 0;
    if (failure_log.Allow(&suppressed)) {
      WARN(logger, metric_name << " invoke failed,status:" << (int)message->status
        << ",suppressed since last log:" << suppressed);
    }
  }

  bdf::MessageFactory::Destroy(message);
  in_flight.fetch_sub(1);
}

void HttpInvokeCallback(bdf::EventMessage* message) {
  RecordInvokeResult(message, "http_test_client");
}

void RapidInvokeCallback(bdf::EventMessage* message) {
  RecordInvokeResult(message, "rapid_test_client");
}

bool SubmitHttp(bdf::ClientMgr& client_mgr) {
  auto* message = bdf::MessageFactory::Allocate<bdf::HttpMessage>();
  message->InitRequest("POST", true);
  message->http_info.headers.emplace("Content-Type", "text/html");
  message->http_info.url = "/invoke-stress";
  message->http_info.body = "invoke http stress request";
  return client_mgr.Invoke("http_test_client", message, &HttpInvokeCallback);
}

bool SubmitRapid(bdf::ClientMgr& client_mgr) {
  auto* message = bdf::MessageFactory::Allocate<bdf::RapidMessage>();
  message->body = "invoke rapid stress request";
  return client_mgr.Invoke("rapid_test_client", message, &RapidInvokeCallback);
}

}  // namespace

void RunInvokeStress(bdf::ClientMgr& client_mgr,
                     const std::atomic<bool>& stop_requested) {
  const uint64_t max_requests = ReadEnvUnsigned("LIBNBF_INVOKE_COUNT", 0);
  const uint64_t max_in_flight = ReadEnvUnsigned("LIBNBF_INVOKE_MAX_IN_FLIGHT", 4096);
  const uint64_t batch_size = ReadEnvUnsigned("LIBNBF_INVOKE_BATCH", 100);
  const uint64_t interval_us = ReadEnvUnsigned("LIBNBF_INVOKE_INTERVAL_US", 0);
  const bool send_http = ReadEnvUnsigned("LIBNBF_INVOKE_HTTP", 1) != 0;
  const bool send_rapid = ReadEnvUnsigned("LIBNBF_INVOKE_RAPID", 1) != 0;
  const bool once = ReadEnvUnsigned("LIBNBF_INVOKE_ONCE", 0) != 0;
  const uint64_t startup_delay_ms = ReadEnvUnsigned("LIBNBF_INVOKE_START_DELAY_MS", 1000);
  if (batch_size == 0 || max_in_flight == 0 || (!send_http && !send_rapid)) {
    WARN(logger, "Invoke stress requires positive batch and in-flight limits and an enabled protocol");
    return;
  }
  usleep(static_cast<useconds_t>(startup_delay_ms * 1000));

  uint64_t submitted = 0;
  bool next_http = send_http;
  while (!stop_requested.load() && (max_requests == 0 || submitted < max_requests)) {
    for (uint64_t attempt = 0; attempt < batch_size && !stop_requested.load()
         && (max_requests == 0 || submitted < max_requests); ++attempt) {
      while (!stop_requested.load() && in_flight.load() >= max_in_flight) {
        usleep(100);
      }
      if (stop_requested.load()) break;

      // Invoke may deliver an immediate failure on this thread.
      in_flight.fetch_add(1);
      const bool accepted = next_http ? SubmitHttp(client_mgr) : SubmitRapid(client_mgr);
      if (accepted) ++submitted;
      if (send_http && send_rapid) next_http = !next_http;
      if (interval_us != 0) usleep(static_cast<useconds_t>(interval_us));
    }
    if (once) break;
    if (0 == (submitted % 30)) {
      usleep(1000);
    }
  }

  INFO(logger, "Invoke submission stopped, submitted:" << submitted
    << ",completed:" << completed.load() << ",failed:" << failed.load()
    << ",in_flight:" << in_flight.load());
}
