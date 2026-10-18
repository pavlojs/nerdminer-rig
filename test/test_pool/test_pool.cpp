#include <math.h>
#include <string.h>
#include <unity.h>
#include "pool.h"

// --- Responses captured from the pools on 2026-10-18 ---
static const char* NERDMINERS_STATUS =
  R"({"runtime": 18933, "lastupdate": 1792341645, "Users": 8630, "Workers": 12683, "Idle": 999, "Disconnected": 23625})" "\n"
  R"({"hashrate1m": "9.88865G", "hashrate5m": "9.89782G", "hashrate15m": "9.91460G", "hashrate1hr": "9.84900G", "hashrate6hr": "5.76632G", "hashrate1d": "1.94864G", "hashrate7d": "310.601M"})" "\n"
  R"({"diff": 0.05, "accepted": 6.90056e10, "rejected": 1.09142e10, "bestshare": 2.12214e8, "SPS1m": 2302.38, "SPS5m": 2304.52, "SPS15m": 2308.42, "SPS1h": 2303.17})" "\n";

static const char* CKPOOL_STATUS =
  R"({"runtime":1010248,"lastupdate":1792342090,"Users":23179,"Workers":37436,"Idle":12876,"Disconnected":4920})" "\n"
  R"({"hashrate1m":"95.5P","hashrate5m":"95.3P","hashrate15m":"95.2P","hashrate1hr":"95.1P","hashrate6hr":"125P","hashrate1d":"183P","hashrate7d":"146P"})" "\n"
  R"({"diff":161.3,"accepted":226775970721113,"rejected":8846191217025317325,"bestshare":57242715681716,"SPS1m":5501.7,"SPS5m":5492.7,"SPS15m":5485.6,"SPS1h":5480.1})" "\n"
  R"({"clients":31,"channels":29,"jdclients":0})" "\n";

// solo.ckpool.org, wallet address replaced
static const char* CKPOOL_USER_IDLE = R"({
    "hashrate1m": "0",
    "hashrate5m": "0",
    "hashrate1hr": "0",
    "hashrate1d": "279M",
    "hashrate7d": "447M",
    "lastshare": 1792245146,
    "workers": 0,
    "shares": 15593000,
    "bestshare": 33282598.74076409,
    "bestever": 33282598,
    "authorised": 1761881707,
    "worker": [
        {
            "workername": "bc1qexampleaddress.bitaxe",
            "hashrate1m": "0",
            "hashrate5m": "0",
            "hashrate1hr": "0",
            "hashrate1d": "279M",
            "hashrate7d": "447M",
            "lastshare": 1792245146,
            "shares": 909000,
            "bestshare": 33282598.74076409,
            "bestever": 33282598
        }
    ]
})";

// same layout with four NerdMiners mining
static const char* CKPOOL_USER_MINING = R"({
    "hashrate1m": "219K",
    "hashrate5m": "218K",
    "hashrate1hr": "218K",
    "hashrate1d": "217K",
    "hashrate7d": "217K",
    "lastshare": 1792345146,
    "workers": 4,
    "shares": 1250,
    "bestshare": 1532.25,
    "bestever": 48210,
    "authorised": 1761881707,
    "worker": []
})";

static const char* NGINX_404 = "<html>\r\n<head><title>404 Not Found</title></head>\r\n</html>\r\n";

static const char* PUBLIC_POOL_INFO =
  R"({"totalHashRate":50795113923210080,"blockHeight":972025,"totalMiners":26197,"blocksFound":[],"fee":0})";

void setUp() {}
void tearDown() {}

// --- ckpool status ---
void test_ckpool_status_nerdminers() {
  PoolStats stats;
  TEST_ASSERT_TRUE(parseCkpoolStatus(NERDMINERS_STATUS, stats));
  TEST_ASSERT_EQUAL(8630, stats.users);
  TEST_ASSERT_EQUAL(12683, stats.workers);
  TEST_ASSERT_DOUBLE_WITHIN(1, 9.88865e9, stats.hashrate);
}

void test_ckpool_status_with_extra_lines() {
  PoolStats stats;
  TEST_ASSERT_TRUE(parseCkpoolStatus(CKPOOL_STATUS, stats));
  TEST_ASSERT_EQUAL(23179, stats.users);
  TEST_ASSERT_EQUAL(37436, stats.workers);
  TEST_ASSERT_DOUBLE_WITHIN(1e6, 95.5e15, stats.hashrate);
}

void test_ckpool_status_rejects_other_responses() {
  PoolStats stats;
  stats.users = 7;
  TEST_ASSERT_FALSE(parseCkpoolStatus(NGINX_404, stats));
  TEST_ASSERT_FALSE(parseCkpoolStatus(PUBLIC_POOL_INFO, stats));
  TEST_ASSERT_FALSE(parseCkpoolStatus("", stats));
  // first line alone is not enough
  TEST_ASSERT_FALSE(parseCkpoolStatus(R"({"Users": 1, "Workers": 2})", stats));
  // a failed parse keeps the previous stats
  TEST_ASSERT_EQUAL(7, stats.users);
}

// --- ckpool user ---
void test_ckpool_user_idle() {
  UserStats stats;
  TEST_ASSERT_TRUE(parseCkpoolUser(CKPOOL_USER_IDLE, stats));
  TEST_ASSERT_EQUAL(0, stats.workers);
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 0, stats.hashrate);
  TEST_ASSERT_DOUBLE_WITHIN(0.5, 15593000, stats.shares);
  TEST_ASSERT_DOUBLE_WITHIN(0.01, 33282598.74, stats.bestShare);
}

void test_ckpool_user_mining() {
  UserStats stats;
  TEST_ASSERT_TRUE(parseCkpoolUser(CKPOOL_USER_MINING, stats));
  TEST_ASSERT_EQUAL(4, stats.workers);
  TEST_ASSERT_DOUBLE_WITHIN(1, 219e3, stats.hashrate);
  TEST_ASSERT_DOUBLE_WITHIN(0.5, 1250, stats.shares);
  TEST_ASSERT_DOUBLE_WITHIN(0.01, 1532.25, stats.bestShare);
}

void test_ckpool_user_rejects_other_responses() {
  UserStats stats;
  TEST_ASSERT_FALSE(parseCkpoolUser(NGINX_404, stats));
  TEST_ASSERT_FALSE(parseCkpoolUser(PUBLIC_POOL_INFO, stats));
}

// --- Units ---
void test_parse_suffixed() {
  TEST_ASSERT_DOUBLE_WITHIN(1, 9.88865e9, parseSuffixed("9.88865G"));
  TEST_ASSERT_DOUBLE_WITHIN(1, 310.601e6, parseSuffixed("310.601M"));
  TEST_ASSERT_DOUBLE_WITHIN(1e6, 95.5e15, parseSuffixed("95.5P"));
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 0, parseSuffixed("0"));
  TEST_ASSERT_DOUBLE_WITHIN(0.001, 123, parseSuffixed("123"));
  // ckpool prints "%.3g", which turns 999.95K into "1e+03K"
  TEST_ASSERT_DOUBLE_WITHIN(1, 1e6, parseSuffixed("1e+03K"));
}

static void assertFormat(const char* expected, double value, const char* unit) {
  char buf[16];
  formatSuffixed(value, unit, buf, sizeof(buf));
  TEST_ASSERT_EQUAL_STRING(expected, buf);
}

void test_format_suffixed() {
  assertFormat("0 H/s", 0, "H/s");
  assertFormat("0 H/s", NAN, "H/s");
  assertFormat("999 H/s", 999, "H/s");
  assertFormat("219 KH/s", 218734, "H/s");
  assertFormat("9.89 GH/s", 9.88865e9, "H/s");
  assertFormat("95.5 PH/s", 95.5e15, "H/s");
  assertFormat("33.3M", 33282598.74, "");
  assertFormat("15.6M", 15593000, "");
  assertFormat("294T", 294141974944674.8, "");
  assertFormat("1.25K", 1250.4, "");
}

void test_format_suffixed_rounds_into_next_prefix() {
  // never "1000 H/s" or "100.0"
  assertFormat("1.00 KH/s", 999.6, "H/s");
  assertFormat("100 KH/s", 99.96e3, "H/s");
  assertFormat("10.0 MH/s", 9.996e6, "H/s");
}

// The hashrate page prints at text size 2, where no Org_01 character used here is wider than 12 px:
// nine characters keep it inside the 128 px screen
void test_format_suffixed_fits_display() {
  char buf[32];
  for (int exp = -2; exp <= 24; exp++) {
    for (double mantissa = 1.0; mantissa < 10.0; mantissa += 0.0137) {
      double value = mantissa * pow(10, exp);
      formatSuffixed(value, "H/s", buf, sizeof(buf));
      TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(9, strlen(buf), buf);
      formatSuffixed(value, "", buf, sizeof(buf));
      TEST_ASSERT_LESS_OR_EQUAL_MESSAGE(5, strlen(buf), buf);
    }
  }
}

int main() {
  UNITY_BEGIN();
  RUN_TEST(test_ckpool_status_nerdminers);
  RUN_TEST(test_ckpool_status_with_extra_lines);
  RUN_TEST(test_ckpool_status_rejects_other_responses);
  RUN_TEST(test_ckpool_user_idle);
  RUN_TEST(test_ckpool_user_mining);
  RUN_TEST(test_ckpool_user_rejects_other_responses);
  RUN_TEST(test_parse_suffixed);
  RUN_TEST(test_format_suffixed);
  RUN_TEST(test_format_suffixed_rounds_into_next_prefix);
  RUN_TEST(test_format_suffixed_fits_display);
  return UNITY_END();
}
