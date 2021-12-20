# Primitive Types
#################
# uint[bits]             unsigned integer
# int[bits]              signed integer
# float[bits]            iEEE floating point number
# flag:                  One bit boolean value
# bits[bits]             Some number of bits,
# bytes[bytes]           Some number of bytes.
# bytes[]                series of bytes, length is variable
# string[bytes]          fixed length string, may be ascii or utf8. Unused characters are indicated as null
# string[]               variable length string
# unused[bits]           indicates data that is not used for any purpose, or reserved

enum Direction: uint[2] {
  Up = 0
  Down = 1
  Left = 2
  Right = 3
}

enum Speed: uint[8] {
  Stopped = 0
  Slow = 80
  Fast = 255
}

struct EnumStruct {
  direction: Direction
  speed: Speed
}

struct TestStruct {
  int1: int[4]
  int2: int[32]
  uint1: uint[5]
  uint2: uint[16]
  float1: float[32]
  f1: flag
  f2: flag
  f3: flag
  b1: bits[5]
  b2: bytes[4]
  s1: string[5]
}

struct Ack {
  code: uint[8]
}

struct SubA {
  flag: flag
  flag2: flag
}

struct SubB {
  num: uint[8]
}

struct NestedStruct {
  a: SubA
  b: SubB
  num: int[4]
}

struct SubC {
  a: SubA
}

struct DeeplyNestedStruct {
  c: SubC
}

struct ArrayStruct {
  a[3]: Direction
  b[2]: Ack
}