/*
  Wifi Configurator -
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#ifndef wificonfigurator_h
#define wificonfigurator_h

#include <WiFi.h>

#define MAX_REQUEST_LEN 512
#define MAX_PARAM_NUM 20

typedef struct ParamStruct
{
  const char *key;
  char *value;
} ParamStruct;

class WifiConfigurator
{

public:
  WifiConfigurator(const char *, const char *password = nullptr);
  void addParam(const char *, char *);
  const char *getParam(const char *);
  void setup();
  bool run();

  IPAddress local_ip;
  IPAddress gateway;
  IPAddress subnet;
  WiFiServer server;

private:
  char _request[MAX_REQUEST_LEN];

  const char *_ssid;
  const char *_password;
  ParamStruct _variables[MAX_PARAM_NUM];

  void _urlDecode(char *);
};

#endif
