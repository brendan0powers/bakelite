#include <iostream>
#include <iomanip>
#include "struct.h"

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

using namespace std;
using namespace Bakelite;

string hexString(const char *data, int length, bool space = false) {
  stringstream ss;
  
  ss << std::hex;

  for(int i = 0; i < length; i++) {
    ss << std::setw(2) << std::setfill('0') << (unsigned int)(data[i] & 0xffu);
    if(space) {
      ss << ' ';
    }
  }
  
  return ss.str();
}

void printHex(const char *data, int length) {
  cout << hexString(data, length, false) << endl;
}


TEST_CASE("simple struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  Ack t1 = {
    123
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 1);
  CHECK(hexString(data, stream.pos()) == "7b");

  Ack t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.code == 123);
}

TEST_CASE("complex struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  TestStruct t1 = {
    5,
    -1234,
    31,
    1234,
    -1.23,
    true,
    true,
    false,
    {1, 2, 3, 4},
    "hey",
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 24);
  CHECK(hexString(data, stream.pos()) == "052efbffff1fd204a4709dbf010100010203046865790000");

  TestStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.int1 == 5);
  CHECK(t2.int2 == -1234);
  CHECK(t2.uint1 == 31);
  CHECK(t2.uint2 == 1234);
  CHECK(t2.float1 == doctest::Approx(-1.23));
  CHECK(t2.b1 == true);
  CHECK(t2.b2 == true);
  CHECK(t2.b3 == false);
  CHECK(string(t2.str) == "hey");
}

TEST_CASE("enum struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  EnumStruct t1 = {
    Direction::Left,
    Speed::Fast
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 2);
  CHECK(hexString(data, stream.pos()) == "02ff");

  EnumStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK_EQ(t2.direction, Direction::Left);
  CHECK_EQ(t2.speed, Speed::Fast);
}

TEST_CASE("nested struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  NestedStruct t1 = {
    {true, false},
    { 127 },
    -4
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 4);
  CHECK(hexString(data, stream.pos()) == "01007ffc");

  NestedStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.a.b1 == true);
  CHECK(t2.a.b2 == false);
  CHECK(t2.b.num == 127);
  CHECK(t2.num == -4);
}

TEST_CASE("deeply nested struct") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  DeeplyNestedStruct t1 = {
    { { false, true } }
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 2);
  CHECK(hexString(data, stream.pos()) == "0001");

  DeeplyNestedStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.c.a.b1 == false);
  CHECK(t2.c.a.b2 == true);
}

TEST_CASE("struct with fixed arrays") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);

  ArrayStruct t1 = {
    { Direction::Left, Direction::Right, Direction::Down },
    { { 127 }, { 64 } },
    { "abc", "def", "ghi" }
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 17);
  CHECK(hexString(data, stream.pos()) == "0203017f40616263006465660067686900");

  ArrayStruct t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);
  
  CHECK(t2.a[0] == Direction::Left);
  CHECK(t2.a[1] == Direction::Right);
  CHECK(t2.a[2] == Direction::Down);
  CHECK(t2.b[0].code == 127);
  CHECK(t2.b[1].code == 64);
  CHECK(string(t2.c[0]) == "abc");
  CHECK(string(t2.c[1]) == "def");
  CHECK(string(t2.c[2]) == "ghi");
}

TEST_CASE("struct with variable types") {
  char *data = new char[256];
  char *heap = new char[256];
  BufferStream stream(data, 256, heap, 256);
  uint8_t byteData[11] = { 0x68, 0x65, 0x6C, 0x6C, 0x6F,
        0,
        0x57, 0x6F, 0x72, 0x6C, 0x64 };
  uint8_t bytes[4] = { 1, 2, 3, 4 };
  uint8_t nestedBytesA[3] = { 4, 5, 6 };
  uint8_t nestedBytesB[3] = { 7, 8, 9 };
  SizedArray<uint8_t> bytesList[2] = {
    { nestedBytesA, 3 },
    { nestedBytesB, 3 },
  };
  const char *stringList[3] = { "abc", "def", "ghi" };

  VariableLength t1 = {
    { byteData, 11 },
    (char *)"This is a test string!",
    { bytes, 4},
    { bytesList, 2 },
    { (char **)stringList, 3 }
  };
  REQUIRE(t1.pack(stream) == 0);

  CHECK(stream.pos() == 62);
  CHECK(hexString(data, stream.pos()) == "0b68656c6c6f00576f726c64546869732069732061207465737420737472696e672100040102030402030405060307080903616263006465660067686900");

  VariableLength t2;
  stream.seek(0);
  REQUIRE(t2.unpack(stream) == 0);

  CHECK(t2.a.size == 11);
  CHECK(vector<uint8_t>(t2.a.data, t2.a.data+t2.a.size) == vector<uint8_t>(byteData, byteData+11));
  CHECK(string(t2.b) == "This is a test string!");
  CHECK(t2.c.size == 4);
  CHECK(vector<uint8_t>(t2.c.data, t2.c.data+t2.c.size) == vector<uint8_t>({1, 2, 3, 4}));
  CHECK(t2.d.size == 2);
  CHECK(t2.d.data[0].size == 3);
  CHECK(t2.d.data[0].data[0] == 4);
  CHECK(t2.d.data[0].data[1] == 5);
  CHECK(t2.d.data[0].data[2] == 6);
  CHECK(t2.d.data[1].size == 3);
  CHECK(t2.d.data[1].data[0] == 7);
  CHECK(t2.d.data[1].data[1] == 8);
  CHECK(t2.d.data[1].data[2] == 9);
  CHECK(t2.e.size == 3);
  CHECK(string(t2.e.data[0]) == "abc");
  CHECK(string(t2.e.data[1]) == "def");
  CHECK(string(t2.e.data[2]) == "ghi");
}

TEST_CASE("encode frame") {
  const int buffSize = 259;
  char buffer[buffSize + COBS_ENCODE_SRC_OFFSET(buffSize)];
  memset(buffer, 0xFF, sizeof(buffer));
  char *srcPtr = buffer + COBS_ENCODE_SRC_OFFSET(buffSize);
  const char srcData[] = "\x00\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\xEE\x00\xAA\xBB";
  memcpy(srcPtr, srcData, sizeof(srcData));

  auto result = cobs_encode((uint8_t *)buffer, sizeof(buffer), (const uint8_t *)srcPtr, sizeof(srcData)-1);
  CHECK(result.status == 0);
  CHECK(result.out_len == 260);
  
  memset(buffer+result.out_len, 0xFF, sizeof(buffer) - result.out_len);
  CHECK(hexString(buffer, sizeof(buffer)) == "01ffeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee0103aabbff");
}

TEST_CASE("decode frame") {
  char buffer[260];
  memset(buffer, 0xFF, sizeof(buffer));

  buffer[0] = 0x01;
  buffer[1] = 0xFF;
  for(int i = 0; i < 254; i++) {
    buffer[i+2] = 0xEE;
  }
  buffer[256] = 0x01;
  buffer[257] = 0x03;
  buffer[258] = 0xaa;
  buffer[259] = 0xbb;

  auto result = cobs_decode((uint8_t *)buffer, sizeof(buffer), (const uint8_t *)buffer, sizeof(buffer));
  CHECK(result.status == 0);
  CHECK(result.out_len == 258);
  
  memset(buffer+result.out_len, 0xFF, sizeof(buffer) - result.out_len);
  //printHex(buffer, sizeof(buffer));

  CHECK(hexString(buffer, sizeof(buffer)) == "00eeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeeee00aabbffff");
}

TEST_CASE("cobs framer encode") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.encodeFrame((const uint8_t *)"\x11\x22\x33\x44", 4);
  CHECK(result.status == 0);
  CHECK(result.length == 6);
  CHECK(hexString((const char *)result.data, result.length) == "051122334400");
}

TEST_CASE("cobs framer decode") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.readFrameByte(0x05);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x11);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x22);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x33);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x44);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x00);
  CHECK(result.status == CobsDecodeState::Decoded);
  CHECK(result.length == 4);
  CHECK(hexString((const char *)result.data, result.length) == "11223344");
}

TEST_CASE("cobs framer decode more bytes") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.readFrameByte(0x05);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x11);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x22);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x33);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x44);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x00);
  CHECK(result.status == CobsDecodeState::Decoded);
  CHECK(result.length == 4);
  CHECK(hexString((const char *)result.data, result.length) == "11223344");

  result = framer.readFrameByte(0x05);
  CHECK(result.status == CobsDecodeState::NotReady);
}

TEST_CASE("cobs framer decode two null bytes") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.readFrameByte(0x05);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x11);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x22);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x33);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x44);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x00);
  CHECK(result.status == CobsDecodeState::Decoded);
  CHECK(result.length == 4);
  CHECK(hexString((const char *)result.data, result.length) == "11223344");

  result = framer.readFrameByte(0x00);
  CHECK(result.status == CobsDecodeState::DecodeFailure);
}

TEST_CASE("cobs buffer overrun") {
  CobsFramer<CrcNoop, 2> framer;
  auto result = framer.readFrameByte(0x05);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x11);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x22);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x33);
  CHECK(result.status == CobsDecodeState::BufferOverrun);
}

TEST_CASE("cobs decode failure") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.readFrameByte(0x01);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x11);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x22);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x33);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x44);
  CHECK(result.status == CobsDecodeState::NotReady);
  result = framer.readFrameByte(0x00);
  CHECK(result.status == CobsDecodeState::DecodeFailure);
}

TEST_CASE("cobs framer roundtrip") {
  CobsFramer<CrcNoop, 256> framer;
  auto result = framer.encodeFrame((const uint8_t *)"\x11\x22\x33\x44", 4);
  REQUIRE(result.status == 0);
  for(int i = 0; i < result.length - 1; i++) {
    auto decodeResult = framer.readFrameByte(result.data[i]);
    CHECK(decodeResult.status == CobsDecodeState::NotReady);
  }

  auto decodeResult = framer.readFrameByte(result.data[result.length - 1]);
  CHECK(decodeResult.status == CobsDecodeState::Decoded);
  CHECK(decodeResult.length == 4);
  CHECK(hexString((const char *)decodeResult.data, decodeResult.length) == "11223344");
}