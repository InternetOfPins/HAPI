#pragma once
#include <hapi/hapi.h>
#include <stdint.h>
#ifdef ARDUINO
#include <WiFi.h>
#include <PubSubClient.h>

namespace oneIO::net {

  // Mqtt<Ssid,Pass,Broker,Port> -- WiFi + PubSubClient wrapper for IOP,
  // same shape as oneIO::storage::SDCard: wraps a vendor Arduino library,
  // not a from-scratch protocol stack. One transport for the ESP32 leg
  // of the IoT composition experiment, per HANDOFF's "one transport per
  // target" scope decision.
  //
  // Example-local, not promoted into OneIO proper: compiles and links
  // against the real framework-arduinoespressif32 core + real
  // knolleary/PubSubClient (see this example's README), but no live
  // broker round-trip has been verified yet -- see .RnD/iotComposition/
  // HANDOFF.md for the full provenance.
  //
  // Ssid/Pass/Broker: pointers to string literals (NTTPs -- same pattern
  // filenames use in oneIO::storage::SDCard).
  /// @brief WiFi + MQTT publish for ESP32; begin() connects both, publish(topic,payload) sends
  template<const char* Ssid, const char* Pass, const char* Broker, uint16_t Port = 1883>
  struct Mqtt {
    inline static WiFiClient wifiClient;
    inline static PubSubClient client{wifiClient};

    struct NetDef {
      NetDef() = delete;
      static void begin() {}
    };

    template<typename O>
    struct Part : O {
      static void begin() {
        WiFi.begin(Ssid, Pass);
        while (WiFi.status() != WL_CONNECTED) delay(200);
        client.setServer(Broker, Port);
        O::begin();
      }
      static bool publish(const char* topic, const char* payload) {
        if (!client.connected()) client.connect("iop-device");
        return client.publish(topic, payload);
      }
    };
  };

} // oneIO::net
#endif
