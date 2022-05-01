#define DIMMABLE
#ifdef DIMMABLE
#define HUE_TYPE "Dimmable light"
#define HUE_MODEL_ID "LWB010"
#define MANUFACTURERNAME "Philips"
#define HUE_CAPABILITIES "{\"certified\":false}"
#endif
#ifdef COLOR_TEMP
#define HUE_TYPE "Color temperature light"
#define HUE_MODEL_ID "LTW001"
#define MANUFACTURERNAME "Philips"
#define HUE_CAPABILITIES "{\"certified\":false,\"streaming\":{\"renderer\":true,\"proxy\":false}}"
#endif
#ifdef COLOR_TEMP_EXT
#define HUE_TYPE "Extended color light"
#define HUE_MODEL_ID "LCT015"
#define MANUFACTURERNAME "Philips"
#define HUE_CAPABILITIES "{\"certified\":false,\"streaming\":{\"renderer\":true,\"proxy\":false}}"
#endif
#ifdef OTHER
#define HUE_TYPE "On/Off plug-in unit"
#define HUE_MODEL_ID "TRADFRI control outlet"
#define MANUFACTURERNAME "IKEA of Sweden"
#define HUE_CAPABILITIES "{\"certified\":false,\"control\": {},\"streaming\":{\"renderer\":false,\"proxy\":false}},\"productname\": \"On/Off plug\""
#endif

const char FAUXMO_TCP_HEADERS[] =
    "HTTP/1.1 %d %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "Connection: close\r\n";

const char FAUXMO_TCP_STATE_RESPONSE[] = "["
    "{\"success\":{\"/lights/%d/state/on\":%s}}"
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
    ",{\"success\":{\"/lights/%d/state/bri\":%d}}"
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
    ",{\"success\":{\"/lights/%d/state/hue\":%d}},"
    "{\"success\":{\"/lights/%d/state/sat\":%d}},"
    "{\"success\":{\"/lights/%d/state/ct\":%d}}"   // not needed?
#endif
#endif
"]";

const char FAUXMO_TCP_STATE_TEMPLATE[] = "{"
    "\"on\":%s"
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
    ",\"bri\":%d"   // not needed?
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
    ",\"hue\":%d,"   // not needed?
    "\"sat\":%d,"   // not needed?
    "\"ct\":%d"   // not needed?
#endif
#endif
"}";

// Working with gen1 and gen3, ON/OFF/%, gen3 requires TCP port 80
const char FAUXMO_DEVICE_JSON_TEMPLATE[] = "{"
    "\"type\":\"" HUE_TYPE "\","
    "\"name\":\"%s\","
    "\"uniqueid\":\"" MACSTR ":00:11-%02x\","
    "\"modelid\":\"" HUE_MODEL_ID "\","
    "\"state\":{\"on\":%s"
#if defined(DIMMABLE) || defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
    ",\"bri\":%u,"
#if defined(COLOR_TEMP) || defined(COLOR_TEMP_EXT)    
    "\"hue\": %d, \"sat\": %d, \"xy\": [0.0, 0.0], \"ct\": %d, \"effect\": \"none\", \"colormode\": \"%s\","
#endif
#endif
    "\"alert\":\"none\",\"mode\":\"homeautomation\",\"reachable\":true}%s}";

const char FAUXMO_DESCRIPTION_TEMPLATE[] =
"<?xml version=\"1.0\" ?>"
"<root xmlns=\"urn:schemas-upnp-org:device-1-0\">"
    "<specVersion><major>1</major><minor>0</minor></specVersion>"
    "<URLBase>http://" IPSTR ":%d/</URLBase>"
    "<device>"
        "<deviceType>urn:schemas-upnp-org:device:Basic:1</deviceType>"
        "<friendlyName>Philips hue (" IPSTR ":%d)</friendlyName>"
        "<manufacturer>Royal Philips Electronics</manufacturer>"
        "<manufacturerURL>http://www.philips.com</manufacturerURL>"
        "<modelDescription>Philips hue Personal Wireless Lighting</modelDescription>"
        "<modelName>Philips hue bridge 2012</modelName>"
        "<modelNumber>929000226503</modelNumber>"
        "<modelURL>http://www.meethue.com</modelURL>"
        "<serialNumber>%s</serialNumber>"
        "<UDN>uuid:2f402f80-da50-11e1-9b23-%s</UDN>"
        "<presentationURL>index.html</presentationURL>"
    "</device>"
"</root>";

const char FAUXMO_UDP_RESPONSE_TEMPLATE[] =
    "HTTP/1.1 200 OK\r\n"
    "EXT:\r\n"
    "CACHE-CONTROL: max-age=100\r\n" // SSDP_INTERVAL
    "LOCATION: http://" IPSTR ":%d/description.xml\r\n"
    "SERVER: FreeRTOS/6.0.5, UPnP/1.0, IpBridge/1.17.0\r\n" // _modelName, _modelNumber
    "hue-bridgeid: %02X%02X%02XFFFE%02X%02X%02X\r\n"
    "ST: urn:schemas-upnp-org:device:basic:1\r\n"  // _deviceType
    "USN: uuid:2f402f80-da50-11e1-9b23-%02x%02x%02x%02x%02x%02x::upnp:rootdevice\r\n" // _uuid::_deviceType
    "\r\n";

const char WEMO_UDP_RESPONSE_TEMPLATE[] =
    "HTTP/1.1 200 OK\r\n"
    "CACHE-CONTROL: max-age=86400\r\n"
    "DATE: Fri, 15 Apr 2016 04:56:29 GMT\r\n"
    "EXT:\r\n"
    "LOCATION: http://" IPSTR ":80/setup.xml\r\n"
    "OPT: \"http://schemas.upnp.org/upnp/1/0/\"; ns=01\r\n"
    "01-NLS: b9200ebb-736d-4b93-bf03-835149d13983\r\n"
    "SERVER: Unspecified, UPnP/1.0, Unspecified\r\n"
    "ST: upnp:rootdevice\r\n"
    "USN: uuid:%s::urn:upnp:rootdevice\r\n"
    "X-User-Agent: redsonic\r\n\r\n";

const char WEMO_UPNP_RESPONSE_TEMPLATE[] =
  "<s:Envelope xmlns:s=\"http://schemas.xmlsoap.org/soap/envelope/\" s:encodingStyle=\"http://schemas.xmlsoap.org/soap/encoding/\">" \
    "<s:Body>" \
      "<u:%cetBinaryStateResponse xmlns:u=\"urn:Belkin:service:basicevent:1\">" \
        "<BinaryState>%d</BinaryState>" \
      "</u:%cetBinaryStateResponse>" \
    "</s:Body>" \
  "</s:Envelope>\r\n";
