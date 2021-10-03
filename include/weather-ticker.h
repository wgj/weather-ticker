#ifndef WEATHER_TICKER_WEATHER_TICKER_H
#define WEATHER_TICKER_WEATHER_TICKER_H

bool debug = true;

// WiFi settings
const char* ssid = "I pronounce WiFi as WiFi";
const char* pass = "anime was a mistake";

// NWS settings
const char* userAgentContact = "wgj@futureprefect.com";
const char* nwsApiHost = "api.weather.gov";
// The NWS issues weather alerts by county and Public Forecast zones.
// You can find yours at https://www.weather.gov/pimar/PubZone
// TODO(wgj): Call BOU and ask what the difference is between COZ039 and COZ239, 303-494-4221.
//  COZ039 NWS zone for Boulder below 6000'.
//  COZ239 is *also* the NWS zone for Boulder below 6000'.
const char* zones[] = {
                          //"COZ039",
                          "COC013"
                       };
const char* events[] = {
                          "Air Quality Alert",
                          "Winter Weather Warnings / Watches / Advisories"
                          // TODO(wgj): Add more alert types.
                        };

struct alert {
  char* id;
  char* headline;
  char* NWSheadline;
  char* description;
};
inline bool operator<(const alert& lhs, const alert& rhs)
{
  return lhs.id < rhs.id;
}

char debugRequestString[] = "/alerts/active/area/CO";
// char requestString[256];
// sprintf(requestString, "/alerts/active/zone/%s", nwsZone);

#endif
