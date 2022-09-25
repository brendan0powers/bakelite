import os
from dataclasses import dataclass
from typing import List, Tuple, Type, TypeVar

from lark import Lark
from lark.visitors import Transformer

from .types import *


_g_parser = None


class ValidationError(RuntimeError):
  pass


@dataclass
class _Array:
  value: int


@dataclass
class _Comment:
  value: str


@dataclass
class _Name:
  value: str


@dataclass
class _Value:
  value: str


@dataclass
class _Number:
  value: int


@dataclass
class _ProtoMessageIds:
  ids: List[ProtoMessageId]


TFilter = TypeVar("TFilter", bound=object)


def _filter(args: List[Any], class_type: Type[TFilter]) -> List[TFilter]:
  return [v for v in args if isinstance(v, class_type)]


def _find_one(args: List[Any], class_type: Type[object]) -> Any:
  args = _filter(args, class_type)
  if len(args) == 0:
    return None
  elif len(args) > 1:
    raise RuntimeError(f"Found more than one {class_type}")

  if hasattr(args[0], "value"):
    return args[0].value
  else:
    return args[0]


TMany = TypeVar("TMany")


def _find_many(args: List[Any], class_type: Type[TMany]) -> List[TMany]:
  return _filter(args, class_type)


class TreeTransformer(Transformer):
  def array(self, args: List[Any]) -> _Array:
    if len(args) > 0:
      return _Array(value=int(args[0]))
    else:
      return _Array(value=0)

  def argument_val(self, args: List[Any]) -> ProtoAnnotationArg:
    if len(args) == 1:
      return ProtoAnnotationArg(name=None, value=str(args[0]))
    elif len(args) == 2:
      return ProtoAnnotationArg(name=str(args[0]), value=str(args[1]))
    else:
      raise RuntimeError("Argument has more than three args")

  def annotation(self, args: List[Any]) -> ProtoAnnotation:
    return ProtoAnnotation(
        name=str(args[0]), arguments=_find_many(args, ProtoAnnotationArg)
    )

  def comment(self, args: List[Any]) -> _Comment:
    return _Comment(value=str(args[0]))

  def enum(self, args: List[Any]) -> ProtoEnum:
    return ProtoEnum(
        type=_find_one(args, ProtoType),
        name=_find_one(args, _Name),
        values=_find_many(args, ProtoEnumValue),
        comment=_find_one(args, _Comment),
        annotations=_find_many(args, ProtoAnnotation),
    )

  def enum_value(self, args: List[Any]) -> ProtoEnumValue:
    return ProtoEnumValue(
        name=_find_one(args, _Name),
        value=_find_one(args, _Value),
        comment=_find_one(args, _Comment),
        annotations=_find_many(args, ProtoAnnotation),
    )

  def name(self, args: List[Any]) -> _Name:
    return _Name(value=str(args[0]))

  def number(self, args: List[Any]) -> _Number:
    return _Number(value=int(args[0]))

  def prim(self, args: List[Any]) -> ProtoType:
    return ProtoType(name=str(args[0]), size=0)

  def prim_variable(self, args: List[Any]) -> ProtoType:
    if len(args) > 1:
      return ProtoType(name=str(args[0]), size=int(args[1]))
    else:
      return ProtoType(name=str(args[0]), size=0)

  def proto(self, args: List[Any]) -> Protocol:
    ids = _find_one(args, _ProtoMessageIds)

    return Protocol(
        options=_find_many(args, ProtoOption),
        message_ids=ids.ids if ids else [],
        comment=_find_one(args, _Comment),
        annotations=_find_many(args, ProtoAnnotation),
    )

  def proto_message_id(self, args: List[Any]) -> ProtoMessageId:
    return ProtoMessageId(
        name=_find_one(args, _Name),
        number=_find_one(args, _Number),
        comment=_find_one(args, _Comment),
        annotations=_find_many(args, ProtoAnnotation),
    )

  def proto_message_ids(self, args: List[Any]) -> _ProtoMessageIds:
    return _ProtoMessageIds(ids=_find_many(args, ProtoMessageId))

  def proto_member(self, args: List[Any]) -> ProtoOption:
    return ProtoOption(
        name=_find_one(args, _Name),
        value=_find_one(args, _Value),
        comment=_find_one(args, _Comment),
        annotations=_find_many(args, ProtoAnnotation),
    )

  def struct(self, args: List[Any]) -> ProtoStruct:
    return ProtoStruct(
        name=_find_one(args, _Name),
        members=_find_many(args, ProtoStructMember),
        comment=_find_one(args, _Comment),
        annotations=_find_many(args, ProtoAnnotation),
    )

  def struct_member(self, args: List[Any]) -> ProtoStructMember:
    return ProtoStructMember(
        name=_find_one(args, _Name),
        type=_find_one(args, ProtoType),
        value=_find_one(args, _Value),
        comment=_find_one(args, _Comment),
        annotations=_find_many(args, ProtoAnnotation),
        arraySize=_find_one(args, _Array),
    )

  def value(self, args: List[Any]) -> _Value:
    return _Value(value=str(args[0]))

  def type(self, args: List[Any]) -> ProtoType:
    if isinstance(args[0], ProtoType):
      return args[0]
    else:
      return ProtoType(name=str(args[0]), size=None)


def validate(enums: List[ProtoEnum], structs: List[ProtoStruct],
             protocol: Protocol, _comments: List[str]) -> None:
  _enum_map = {enum.name: enum for enum in enums}
  struct_map = {struct.name: struct for struct in structs}

  if not protocol:
    return

  # Validate Message IDs
  for msg_id in protocol.message_ids:
    if msg_id.number == 0:
      raise ValidationError("Message ID 0 is reverved for future use")
    if msg_id.name not in struct_map:
      raise ValidationError(f"{msg_id.name} assigned a message ID, but not declared")


def parse(text: str) -> Tuple[List[ProtoEnum], List[ProtoStruct], Optional[Protocol], List[str]]:
  global _g_parser

  if not _g_parser:
    with open(f"{os.path.dirname(__file__)}/protodef.lark", encoding='utf-8') as f:
      grammar = f.read()

    _g_parser = Lark(grammar)

  tree = _g_parser.parse(text)
  # print(tree.pretty())
  tree = TreeTransformer().transform(tree)

  # print(tree.pretty())

  items = list(tree.iter_subtrees_topdown())[0].children

  enums = _find_many(items, ProtoEnum)
  structs = _find_many(items, ProtoStruct)
  protocol = _find_one(items, Protocol)
  comments = [comment.value for comment in _find_many(items, _Comment)]

  validate(enums, structs, protocol, comments)

  return (enums, structs, protocol, comments)
