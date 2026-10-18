#include "pool.h"
#include <ArduinoJson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Helpers ---
static const char* nextLine(const char* line) {
  const char* nl = strchr(line, '\n');
  return nl ? nl + 1 : nullptr;
}

// --- ckpool ---
bool parseCkpoolStatus(const char* body, PoolStats& out) {
  PoolStats stats;
  JsonDocument doc;

  // deserializeJson() stops at the end of the first object on the line
  // line 1: users and workers
  const char* line = body;
  if (deserializeJson(doc, line) || doc["Users"].isNull()) return false;
  stats.users = doc["Users"] | 0L;
  stats.workers = doc["Workers"] | 0L;

  // line 2: hashrates
  line = nextLine(line);
  if (!line || deserializeJson(doc, line) || doc["hashrate1m"].isNull()) return false;
  stats.hashrate = parseSuffixed(doc["hashrate1m"] | "0");

  out = stats;
  return true;
}

bool parseCkpoolUser(const char* body, UserStats& out) {
  JsonDocument filter;
  filter["hashrate1m"] = true;
  filter["workers"] = true;
  filter["shares"] = true;
  filter["bestshare"] = true;

  JsonDocument doc;
  if (deserializeJson(doc, body, DeserializationOption::Filter(filter)) || doc["hashrate1m"].isNull()) return false;

  UserStats stats;
  stats.hashrate = parseSuffixed(doc["hashrate1m"] | "0");
  stats.workers = doc["workers"] | 0L;
  stats.shares = doc["shares"] | 0.0;
  stats.bestShare = doc["bestshare"] | 0.0;

  out = stats;
  return true;
}

// --- Units ---
double parseSuffixed(const char* text) {
  char* end;
  double value = strtod(text, &end);
  switch (*end) {
    case 'E': return value * 1e18;
    case 'P': return value * 1e15;
    case 'T': return value * 1e12;
    case 'G': return value * 1e9;
    case 'M': return value * 1e6;
    case 'K': return value * 1e3;
    case 'm': return value * 1e-3;
    case 'u': return value * 1e-6;
    case 'n': return value * 1e-9;
    default:  return value;
  }
}

void formatSuffixed(double value, const char* unit, char* out, size_t size) {
  static const char prefixes[] = "KMGTPEZY";
  const char* sep = unit[0] ? " " : "";

  // also catches NaN
  if (!(value > 0)) {
    snprintf(out, size, "0%s%s", sep, unit);
    return;
  }

  // switch prefix before rounding could print "1000"
  int prefix = -1;
  while (value >= 999.5 && prefixes[prefix + 1]) {
    value /= 1000;
    prefix++;
  }

  int decimals = value >= 99.95 ? 0 : value >= 9.995 ? 1 : 2;
  if (prefix < 0) {
    snprintf(out, size, "%.*f%s%s", decimals, value, sep, unit);
  } else {
    snprintf(out, size, "%.*f%s%c%s", decimals, value, sep, prefixes[prefix], unit);
  }
}
