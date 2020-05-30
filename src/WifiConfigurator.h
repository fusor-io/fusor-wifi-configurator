/*
  Wifi Configurator -
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#ifndef wificonfigurator_h
#define wificonfigurator_h

#include <WiFi.h>

#define CONFIGURATOR_BUFFER_SIZE 512
#define CONFIGURATOR_EEPROM_SIZE 512
#define CONFIGURATOR_MAX_PARAM_COUNT 20
#define CONFIGURATOR_MAX_PARAM_LEN 20
#define CONFIGURATOR_MAX_VALUE_LEN 40

typedef struct ParamStruct
{
  const char *key;
  const char *value;
} ParamStruct;

class WifiConfigurator
{

public:
  WifiConfigurator(const char *, const char *password = nullptr);
  void addParam(const char *, const char *);
  void setParam(const char *, const char *);
  const char *getParam(const char *);
  void runServer();

private:
  IPAddress _local_ip;
  IPAddress _gateway;
  IPAddress _subnet;
  WiFiServer _server;

  char _buffer[CONFIGURATOR_BUFFER_SIZE];

  const char *_ssid;
  const char *_password;
  ParamStruct _variables[CONFIGURATOR_MAX_PARAM_COUNT];
  uint8_t _varCount = 0;

  int _getParamId(const char *);
  const char *_allocateValue(char *);

  bool _serve();
  void _urlDecode(char *);

  const char _signature[5] = "CFG ";

  void _saveToEEPROM();
  void _writeEEPROM(uint16_t &address, uint8_t *, uint8_t);
  void _writeEEPROMString(uint16_t &address, const char *, size_t);

  void _loadFromEEPROM();
  void _readEEPROM(uint16_t &address, uint8_t *, uint8_t);
  const char *_readEEPROMString(uint16_t &address);
};

#endif
