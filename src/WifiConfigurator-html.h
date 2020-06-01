/*
  Wifi Configurator -
  web client based configuration for you IOT project
  Copyright Giedrius Lukosevicius 2020
  MIT License
*/

#ifndef wificonfiguratorhtml_h
#define wificonfiguratorhtml_h

#include <Arduino.h>

// Source in template.html
// Used http://minifycode.com/html-minifier to minify

const char indexHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"/><style>body{font-family:Arial;text-align:center;color:#666}form{display:inline-block}.f{margin:0 0 9px;text-align:left}.l{text-transform:capitalize}#b{padding:8px 18px;margin:15px}input{font-size:1em;padding:3px;color:#666}</style><script src="vars.js"></script></head><body><h2 id="h"></h2><form action="/"><div id="d"></div><input type="submit" value="Save" id="b"/></form> <script>var d=document,g=d.getElementById.bind(d),s=(e,v)=>((e.innerHTML=v),e),f=g('d');s(g('h'),title);Object.keys(params).map(p=>{f.appendChild(s(d.createElement('div'),`<div class="f"><div class="l">${p.replace(/[_-]/g,' ')}</div><div><input type="text"id="${p}"name="${p}"></div></div>`));g(p).value=params[p];});</script></body></html>
)rawliteral";
const char successHtml[] PROGMEM = R"rawliteral(<!DOCTYPE html><html><head><meta name="viewport" content="width=device-width,initial-scale=1"/><style>body{font-family:Arial;text-align:center;color:#666}</style></head><body><h2>Configuration saved</h2></body></html>
)rawliteral";
const char htmlContentType[] PROGMEM = "text/html";
const char jsContentType[] PROGMEM = "text/javascript";

const char http200[] PROGMEM = "HTTP/1.1 200 OK";
const char http404[] PROGMEM = "HTTP/1.1 404 Not Found";

const char ctHTML[] PROGMEM = "Content-Type: text/html";
const char ctJavaScript[] PROGMEM = "Content-Type: text/javascript";

const char httpClose[] PROGMEM = "Connection: close";

#endif