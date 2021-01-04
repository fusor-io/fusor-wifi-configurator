/*
  Wifi Configurator -
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#include <Arduino.h>
#include <EEPROM.h>
#include "WifiConfigurator.h"
#include "WifiConfigurator-html.h"

WifiConfigurator::WifiConfigurator() : _server(80),
                                       _local_ip(192, 168, 1, 1),
                                       _gateway(192, 168, 1, 1),
                                       _subnet(255, 255, 255, 0)
{
}

void WifiConfigurator::init()
{
  memset(_variables, 0, sizeof(_variables));
  _loadFromEEPROM();
}

void WifiConfigurator::addParam(const char *name, const char *value)
{
  int id = _getParamId(name);
  if (id < 0)
  {
    // create new param
    _variables[_varCount].key = name;
    _variables[_varCount].value = value;
    _varCount++;
  }
  // Param already exist, do nothing
  // Motivation:
  //   when configurator is created, we read params from EEPROM
  //   then caller program calls addParam and we dont want to overrwrite EEPROM value
  // Use setParam to update/create.
}

void WifiConfigurator::setParam(const char *name, const char *value)
{
  if (name[0] == 0 || value[0] == 0)
    return;

  int id = _getParamId(name);
  if (id < 0)
  {
    // create new param
    _variables[_varCount].key = name;
    _variables[_varCount].value = value;
    _varCount++;
  }
  else
  {
    // overwrite existing
    _variables[id].value = value;
  }
}

int WifiConfigurator::_getParamId(const char *name)
{
  ParamStruct *variable = _variables;

  for (int i = 0; i < _varCount; i++, variable++)
    if (strcmp(variable->key, name) == 0)
      return i;

  return -1;
}

const char *WifiConfigurator::getParam(const char *name)
{
  int id = _getParamId(name);
  return id < 0 ? nullptr : _variables[id].value;
}

void WifiConfigurator::runServer(const char *ssid, const char *password)
{
  _ssid = ssid;

  WiFi.persistent(false);

  if (password == nullptr || !password[0])
    WiFi.softAP(_ssid);
  else
    WiFi.softAP(_ssid, password);

  WiFi.softAPConfig(_local_ip, _gateway, _subnet);

  _server.begin();

  while (!_serve())
  {
  }

  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);
}

bool WifiConfigurator::_serve()
{
  WiFiClient client = _server.available();

  if (client)
  {
    memset(_buffer, 0, CONFIGURATOR_BUFFER_SIZE);

    uint8_t len = 0;
    bool skipTheRest = false;
    char prev = '\0';

    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.print(c);

        if (len < CONFIGURATOR_BUFFER_SIZE - 1 && !skipTheRest)
        {
          if (c == '\n')
            skipTheRest = true;
          else
            _buffer[len++] = c;
        }

        if (c == '\r' && prev == '\n')
          break;

        prev = c;
      }
    }

    if (strncmp("GET / ", _buffer, 6) == 0)
    {
      client.println(http200);
      client.println(ctHTML);
      client.println(httpClose);
      client.println();
#ifdef ESP32
      client.println(indexHtml);
#else
      client.println(F(INDEX_HTML));
#endif

      client.print(F("var title='"));
      client.print(_ssid);
      client.print(F("',params={"));

      for (int i = 0; i < CONFIGURATOR_MAX_PARAM_COUNT; i++)
      {
        if (_variables[i].key)
        {
          client.print(_variables[i].key);
          client.print(":'");
          client.print(_variables[i].value);
          if (i < CONFIGURATOR_MAX_PARAM_COUNT - 1)
          {
            client.print("',");
          }
        }
        else
        {
          break;
        }
      }
      client.println("};");

#ifdef ESP32
      client.println(indexHtmlEnd);
#else
      client.println(F(INDEX_HTML_END));
#endif


    }
    else if (strncmp("GET /?", _buffer, 6) == 0)
    {
      char *query = &_buffer[6];

      _urlDecode(query);
      _saveToEEPROM();

      client.println(http200);
      client.println(ctHTML);
      client.println(httpClose);
      client.println();
#ifdef ESP32
      client.println(successHtml);
#else
      client.println(F(SUCCESS_HTML));
#endif

      return true;
    }
    else
    {
      client.println(http404);
      client.println(httpClose);
      client.println();
    }

    // give the web browser time to receive the data
    delay(1000);

    // close the connection:
    client.stop();
  }

  return false;
}

void WifiConfigurator::_urlDecode(char *url)
{
  char *leader = url;
  char *follower = leader;
  char *paramName = url;
  char *paramValue = nullptr;
  bool paramPending = false;

  while (*leader)
  {
    switch (*leader)
    {

    case '%':
    {
      leader++;
      if (!*leader)
        return;
      char high = *leader;
      leader++;
      if (!*leader)
        return;
      char low = *leader;

      if (high > 0x39)
        high -= 7;

      high &= 0x0f;

      if (low > 0x39)
        low -= 7;
      low &= 0x0f;

      *follower = (high << 4) | low;
      break;
    }

    case '+':
    {
      *follower = ' ';
      break;
    }

    case '=':
    {
      *follower = 0;
      paramValue = follower + 1;
      if (*paramValue)
        paramPending = true;
      break;
    }

    case '&':
    case ' ': // end of url
    {
      *follower = 0;
      if (paramPending)
      {

        setParam(_allocateValue(paramName), _allocateValue(paramValue));
        paramName = follower + 1;
        paramPending = false;
        if (*leader == ' ')
          return;
      }
      break;
    }

    default:
      *follower = *leader;
    }

    leader++;
    follower++;
  }

  *follower = 0;
}

const char *WifiConfigurator::_allocateValue(char *value)
{
  char *buff = new char[strlen(value)];
  strcpy(buff, value);
  return buff;
}

void WifiConfigurator::_saveToEEPROM()
{
  uint16_t address = 0;

  EEPROM.begin(CONFIGURATOR_EEPROM_SIZE);

  // Write signature
  _writeEEPROM(address, (uint8_t *)_signature, sizeof(_signature) - 1);
  // Write number of variables to store
  _writeEEPROM(address, &_varCount, sizeof(_varCount));

  // Write variables
  ParamStruct *variable = _variables;
  for (uint8_t i = 0; i < _varCount; i++, variable++)
  {
    _writeEEPROMString(address, variable->key, CONFIGURATOR_MAX_PARAM_LEN);
    _writeEEPROMString(address, variable->value, CONFIGURATOR_MAX_VALUE_LEN);
  }

  EEPROM.commit();
}

void WifiConfigurator::_writeEEPROMString(uint16_t &address, const char *str, size_t maxLen)
{
  size_t len = strlen(str);
  uint8_t size = (uint8_t)(len > maxLen ? maxLen : len);
  _writeEEPROM(address, &size, sizeof(size));
  _writeEEPROM(address, (uint8_t *)str, size);
}

void WifiConfigurator::_writeEEPROM(uint16_t &address, uint8_t *data, uint8_t size)
{
  for (uint8_t i = 0; i < size; i++, data++, address++)
    EEPROM.write(address, (char)*data);
}

void WifiConfigurator::_loadFromEEPROM()
{
  uint16_t address = 0;
  uint8_t count;

  char sig[sizeof(_signature)];

  EEPROM.begin(CONFIGURATOR_EEPROM_SIZE);

  _readEEPROM(address, (uint8_t *)sig, sizeof(sig) - 1);
  if (strncmp(sig, _signature, sizeof(_signature) - 1) != 0)
    return;

  _readEEPROM(address, &count, sizeof(count));

  char *buff = _buffer;
  for (uint8_t i = 0; i < count; i++)
  {
    const char *varName = _readEEPROMString(address);
    const char *varValue = _readEEPROMString(address);
    setParam(varName, varValue);
  }
}

void WifiConfigurator::_readEEPROM(uint16_t &address, uint8_t *buffer, uint8_t size)
{
  for (uint8_t i = 0; i < size; i++, buffer++, address++)
    *buffer = (uint8_t)EEPROM.read(address);
}

const char *WifiConfigurator::_readEEPROMString(uint16_t &address)
{
  uint8_t size = EEPROM.read(address++);
  char *buffer = new char[size + 1];

  char *p = buffer;
  for (uint8_t i = 0; i < size; i++, p++, address++)
    *p = EEPROM.read(address);

  buffer[size] = 0;

  return buffer;
}