/*
  Wifi Configurator -
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#include <Arduino.h>
#include "WifiConfigurator.h"
#include "WifiConfigurator-html.h"

WifiConfigurator::WifiConfigurator(const char *ssid, const char *password) : server(80),
                                                                             local_ip(192, 168, 1, 1),
                                                                             gateway(192, 168, 1, 1),
                                                                             subnet(255, 255, 255, 0)
{
  _ssid = ssid;
  _password = password;
  memset(_variables, 0, sizeof(_variables));
}

void WifiConfigurator::addParam(const char *name, char *value)
{
  for (int i = 0; i < MAX_PARAM_NUM; i++)
  {
    if (!_variables[i].key)
    {
      _variables[i].key = name;
      _variables[i].value = value;
      return;
    }
    if (strcmp(_variables[i].key, name) == 0)
    {
      _variables[i].value = value;
      return;
    }
  }
}

const char *WifiConfigurator::getParam(const char *name)
{
  for (int i = 0; i < MAX_PARAM_NUM; i++)
  {
    if (!_variables[i].key)
      return nullptr;
    if (strcmp(_variables[i].key, name) == 0)
    {
      return _variables[i].value;
    }
  }
}

void WifiConfigurator::setup()
{
  WiFi.persistent(false);

  if (_password == nullptr || !_password[0])
    WiFi.softAP(_ssid);
  else
    WiFi.softAP(_ssid, _password);

  WiFi.softAPConfig(local_ip, gateway, subnet);

  server.begin();

  while (!run())
  {
  }
}

bool WifiConfigurator::run()
{
  WiFiClient client = server.available();

  if (client)
  {
    memset(_request, 0, MAX_REQUEST_LEN);

    uint8_t len = 0;
    bool skipTheRest = false;
    char prev = '\0';

    Serial.println("new client");
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        Serial.write(c);

        if (len < MAX_REQUEST_LEN - 1 && !skipTheRest)
        {
          if (c == '\n')
            skipTheRest = true;
          else
            _request[len++] = c;
        }

        if (c == '\r' && prev == '\n')
          break;

        prev = c;
      }
    }

    if (strncmp("GET / ", _request, 6) == 0)
    {
      client.println(http200);
      client.println(ctHTML);
      client.println(httpClose);
      client.println();
      client.println(indexHtml);
    }
    else if (strncmp("GET /vars.js ", _request, 13) == 0)
    {
      client.println(http200);
      client.println(ctJavaScript);
      client.println(httpClose);
      client.println();
      client.print("var title='");
      client.print(_ssid);
      client.print("',params={");

      for (int i = 0; i < MAX_PARAM_NUM; i++)
      {
        if (_variables[i].key)
        {
          client.print(_variables[i].key);
          client.print(":'");
          client.print(_variables[i].value);
          client.print("',");
        }
        else
        {
          break;
        }
      }
      client.print("}");
    }
    else if (strncmp("GET /?", _request, 6) == 0)
    {
      char *query = &_request[6];
      _urlDecode(query);

      client.println(http200);
      client.println(ctHTML);
      client.println(httpClose);
      client.println();
      client.println("DONE");

      return true;
    }
    else
    {
      client.println(http404);
      client.println("Connection: close");
      client.println();

      Serial.print("404:");
      Serial.println(_request);
    }

    // give the web browser time to receive the data
    delay(5);

    // close the connection:
    client.stop();
    Serial.println("Close connection");
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
        addParam((const char *)paramName, paramValue);
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
