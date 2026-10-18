#pragma once
#include <stddef.h>

// Parsing and formatting of pool API responses, kept free of Arduino code so it runs in native tests

// --- Stats shown on the display ---
struct PoolStats {
  long users = 0;
  long workers = 0;
  double hashrate = 0;  // H/s
};

struct UserStats {
  long workers = 0;
  double hashrate = 0;  // H/s
  double shares = 0;
  double bestShare = 0;
};

// --- ckpool (pool.nerdminers.org, solo.ckpool.org) ---
// /pool/pool.status holds one JSON object per line
bool parseCkpoolStatus(const char* body, PoolStats& out);
// /users/<address>
bool parseCkpoolUser(const char* body, UserStats& out);

// Reads a ckpool value with a unit prefix, ex. "9.88865G" -> 9.88865e9
double parseSuffixed(const char* text);

// Three significant digits with a unit prefix, ex. (9.88865e9, "H/s") -> "9.89 GH/s", (33282598, "") -> "33.3M"
void formatSuffixed(double value, const char* unit, char* out, size_t size);
