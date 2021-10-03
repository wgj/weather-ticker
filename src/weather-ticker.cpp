#include <string.h>
#include <set>

#include <Arduino.h>
#include <WiFi.h>

#define ARDUINOJSON_USE_DOUBLE 1
#include <ArduinoJson.h>
#include <SSLClient.h>
#include <StreamUtils.h>

#include "weather-ticker.h"
#include "trust_anchors.h"


WiFiClient wifiClient;
// An unused analog pin is needed by SSLClient for a random number.
const int rand_pin = 33;
SSLClient sslClient(wifiClient, TAs, (size_t)TAs_NUM, rand_pin, 1);
int sslClientStatus;
std::set<alert> previousAlerts;

void filterAlerts(SSLClient& cl, const char* _events[], std::set<alert>& alerts) {

  // Capacity is dependent on input JSON. See https://arduinojson.org/v6/assistant/ for details.
  DynamicJsonDocument doc(65536);
  // TODO(wgj): There are a few ways to clear sslClient's buffer. ReadBufferingClient might be fastest.
  //  Decorate sslClient with ReadBufferingClient: Works.
  //  Decorate sslClient with ReadBufferingStream: Not tested.
  //  Decorate loggingStream with ReadBufferingStream: Not tested.
  ReadBufferingClient bufferedClient(cl, 64);
  DeserializationError error = deserializeJson(doc, bufferedClient);

  if (error) {
    // TODO(wgj): Set global error here. If there's an IncompleteInput error here, we don't want to try and compare
    //  products to see if they should be printed.
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.f_str());
  }

  // Clear memory allocation in case we gave doc more capacity than we needed.
  doc.shrinkToFit();

  JsonArray graph = doc["@graph"];
  if (!graph.isNull()) {
    for (JsonVariant alert : graph) {
      const char* event = alert["event"];
      for ( auto * e : events) {
        if (strcmp(event, e) == 0) {
          char* id;
          id = "arst";
          char* headline = {};
          char* NWSheadline = {};
          char* description= {};
          struct alert a {
            .id = strcpy(id, alert["id"]),
            .headline = strcpy(headline, alert["headline"]),
            .NWSheadline = strcpy(NWSheadline, alert["parameters"]["NWSheadline"]),
            .description = strcpy(description, alert["description"])
          };
          //alerts.insert(a);
        }
      }
    }
  }
  // Free up memory used to store JSON object.
  if (debug) {
    Serial.println("filterAlerts: Iterate through alerts: ");
    for (alert a : alerts) {
        Serial.printf("a.id: %s\n", a.id);
        Serial.printf("a.NWSheadline: %s\n", a.NWSheadline);
        Serial.printf("a.headline: %s\n", a.headline);
      }
  Serial.println();
  }
  doc.clear();
}

// Request will query api.weather.gov with request string, leaving the response in cl for the caller.
// TODO(wgj): Ask Stephen about pros and cons/right or wrong to use global definition or to pass a sslClient pointer around.
void requestAlerts(SSLClient& cl, char* requestString) {
  Serial.printf("Connecting to \"%s\"...", nwsApiHost);

  // ls is used to write() and read() to SSLClient and Serial simultaneously.
  LoggingStream ls(cl, Serial);

  if (cl.connect(nwsApiHost, 443)) {
    Serial.printf(" Connected\n\n");

    if (debug) {
      ls.printf("GET %s HTTP/1.1\n", requestString);
      ls.printf("User-Agent: wgj.io/weather-ticker, contact %s\n", userAgentContact);
      ls.printf("accept: application/ld+json\n");
      ls.printf("Host: %s\n", nwsApiHost);
      ls.printf("Connection: close\n\n");
    } else {
      cl.printf("GET %s HTTP/1.1\n", requestString);
      cl.printf("User-Agent: wgj.io/weather-ticker, contact %s\n", userAgentContact);
      cl.printf("accept: application/ld+json\n");
      cl.printf("Host: %s\n", nwsApiHost);
      cl.printf("Connection: close\n\n");
    }
  } else {
    Serial.println(" Connection failed");
    }

  // Wait for SSLClient to have bytes available.
  while (!cl.available()) {}

  // Strip response headers and jump to start of response body.
  char startOfBody[] = "\r\n\r\n";
  cl.find(startOfBody);
}

void printAlert(alert a) {
  Serial.printf("id: %s\n", a.id);
  Serial.printf("...%s...\n", a.NWSheadline);
  Serial.println(a.headline);
  Serial.println(a.description);
}

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.printf("Connecting to \"%s\"", ssid);
  auto wifistatus = WiFi.begin(ssid, pass);
  while (wifistatus != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
    wifistatus = WiFiClass::status();
  }
  Serial.println(" Connected");

  for (const char* zone : zones) {
    // Search for alerts in zones[] of eventType.
    char requestString[256];
    sprintf(requestString, "/alerts/active/zone/%s", zone);
    requestAlerts(sslClient, requestString);
    // Seed previousAlerts[] with alerts when we first start up.
    filterAlerts(sslClient, events, previousAlerts);
  }
  if (debug) {
    Serial.printf("%i alerts found in setup()\n\n", previousAlerts.size());
  }
}

void loop() {
  // TODO(wgj): Add a button to print active alert(s).

  // Gather new alerts
  std::set<alert> newAlerts;
  for (const char* zone : zones) {
    // Search for alerts in zones[] of eventType.
    char requestString[256];
    sprintf(requestString, "/alerts/active/zone/%s", zone);
    requestAlerts(sslClient, requestString);
    filterAlerts(sslClient, events, newAlerts);
  }

  if (debug) {
    Serial.println("loop():");
    Serial.println("Contents of previousAlerts: ");
    Serial.printf("previousAlerts.size(): %i\n", previousAlerts.size());
    Serial.printf("previousAlerts.empty(): %i\n", previousAlerts.empty());
    Serial.printf("previousAlerts.max_size(): %i\n", previousAlerts.max_size());

    for (alert a : previousAlerts) {
      Serial.print("id: ");
      Serial.println(a.id);
      Serial.printf("headline: %s\n", a.headline);
      Serial.printf("\n\n");
    }

    Serial.println("Contents of newAlerts: ");
    Serial.printf("newAlerts.size(): %i\n", newAlerts.size());
    Serial.printf("newAlerts.empty(): %i\n", newAlerts.empty());
    Serial.printf("newAlerts.max_size(): %i\n", newAlerts.max_size());

    for (alert a : newAlerts) {
      Serial.print("id: ");
      Serial.println(a.id);
      Serial.printf("headline: %s\n", a.headline);
      Serial.printf("\n\n");
    }
  }

  // Print any new alerts not in the previous alerts.
  bool foundAlert = false;
  for (alert a : newAlerts) {
    auto iter = previousAlerts.find(a);
    if (iter == previousAlerts.end()) {
      // New alert wasn't found in previous alert.
      continue;
    } else {
      foundAlert = true;
      printAlert(a);
      Serial.println();
    }
  }

  if (!foundAlert) {
    if (debug) {
      Serial.println("No new alerts found");
    }
  }

  // Replace previous alerts with new alerts.
  //previousAlerts.swap(newAlerts);


  Serial.printf("\n\n");
  // Run once for debugging.
  // while(true) {}

  // Wait 60s before querying api.weather.gov again.
  delay(60000);
}
