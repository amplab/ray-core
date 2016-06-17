# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

"""Generates dart source files from a mojom.Module."""

import errno
import os
import re
import shutil
import sys

import mojom.fileutil as fileutil
import mojom.generate.constant_resolver as resolver
import mojom.generate.generator as generator
import mojom.generate.module as mojom
import mojom.generate.pack as pack
from mojom.generate.template_expander import UseJinja

GENERATOR_PREFIX = 'dart'

# CAUTION: To generate Dart-style names, and to avoid generating reserved words
# for identifiers, the template files should generate names using
# {{element|name}}, not {{element.name}}.

# Dart reserved words from:
# http://www.ecma-international.org/publications/files/ECMA-ST/ECMA-408.pdf
# We must not generate reserved words for identifiers.
# NB: async, await, and yield are not technically reserved words, but since
# they are not valid identifiers in all contexts, we include them here as well.
_dart_reserved_words = [
  "assert",
  "async",
  "await",
  "break",
  "case",
  "catch",
  "class",
  "const",
  "continue",
  "default",
  "do",
  "else",
  "enum",
  "extends",
  "false",
  "final",
  "finally",
  "for",
  "if",
  "in",
  "is",
  "new",
  "null",
  "rethrow",
  "return",
  "super",
  "switch",
  "this",
  "throw",
  "true",
  "try",
  "var",
  "void",
  "while",
  "with",
  "yield",
]

# These names are the class fields and methods of the Proxy class in
# lib/src/proxy.dart of Dart's mojo package. If these names appear in a .mojom
# they must be mangled to avoid name conflicts. They are mangled by appending
# an underscore ('_'), which is banned on names in mojom interfaces.
_reserved_words = _dart_reserved_words + [
  "close",
  "connectToService",
  "ctrl",
  "fromEndpoint",
  "fromHandle",
  "fromMock",
  "impl",
  "newFromEndpoint",
  "responseOrError",
  "serviceDescription",
  "serviceName",
  "unbound",
]

_kind_to_dart_default_value = {
  mojom.BOOL:                  "false",
  mojom.INT8:                  "0",
  mojom.UINT8:                 "0",
  mojom.INT16:                 "0",
  mojom.UINT16:                "0",
  mojom.INT32:                 "0",
  mojom.UINT32:                "0",
  mojom.FLOAT:                 "0.0",
  mojom.HANDLE:                "null",
  mojom.DCPIPE:                "null",
  mojom.DPPIPE:                "null",
  mojom.MSGPIPE:               "null",
  mojom.SHAREDBUFFER:          "null",
  mojom.NULLABLE_HANDLE:       "null",
  mojom.NULLABLE_DCPIPE:       "null",
  mojom.NULLABLE_DPPIPE:       "null",
  mojom.NULLABLE_MSGPIPE:      "null",
  mojom.NULLABLE_SHAREDBUFFER: "null",
  mojom.INT64:                 "0",
  mojom.UINT64:                "0",
  mojom.DOUBLE:                "0.0",
  mojom.STRING:                "null",
  mojom.NULLABLE_STRING:       "null"
}

_kind_to_dart_decl_type = {
  mojom.BOOL:                  "bool",
  mojom.INT8:                  "int",
  mojom.UINT8:                 "int",
  mojom.INT16:                 "int",
  mojom.UINT16:                "int",
  mojom.INT32:                 "int",
  mojom.UINT32:                "int",
  mojom.FLOAT:                 "double",
  mojom.HANDLE:                "core.MojoHandle",
  mojom.DCPIPE:                "core.MojoDataPipeConsumer",
  mojom.DPPIPE:                "core.MojoDataPipeProducer",
  mojom.MSGPIPE:               "core.MojoMessagePipeEndpoint",
  mojom.SHAREDBUFFER:          "core.MojoSharedBuffer",
  mojom.NULLABLE_HANDLE:       "core.MojoHandle",
  mojom.NULLABLE_DCPIPE:       "core.MojoDataPipeConsumer",
  mojom.NULLABLE_DPPIPE:       "core.MojoDataPipeProducer",
  mojom.NULLABLE_MSGPIPE:      "core.MojoMessagePipeEndpoint",
  mojom.NULLABLE_SHAREDBUFFER: "core.MojoSharedBuffer",
  mojom.INT64:                 "int",
  mojom.UINT64:                "int",
  mojom.DOUBLE:                "double",
  mojom.STRING:                "String",
  mojom.NULLABLE_STRING:       "String"
}

_spec_to_decode_method = {
  mojom.BOOL.spec:                  'decodeBool',
  mojom.DCPIPE.spec:                'decodeConsumerHandle',
  mojom.DOUBLE.spec:                'decodeDouble',
  mojom.DPPIPE.spec:                'decodeProducerHandle',
  mojom.FLOAT.spec:                 'decodeFloat',
  mojom.HANDLE.spec:                'decodeHandle',
  mojom.INT16.spec:                 'decodeInt16',
  mojom.INT32.spec:                 'decodeInt32',
  mojom.INT64.spec:                 'decodeInt64',
  mojom.INT8.spec:                  'decodeInt8',
  mojom.MSGPIPE.spec:               'decodeMessagePipeHandle',
  mojom.NULLABLE_DCPIPE.spec:       'decodeConsumerHandle',
  mojom.NULLABLE_DPPIPE.spec:       'decodeProducerHandle',
  mojom.NULLABLE_HANDLE.spec:       'decodeHandle',
  mojom.NULLABLE_MSGPIPE.spec:      'decodeMessagePipeHandle',
  mojom.NULLABLE_SHAREDBUFFER.spec: 'decodeSharedBufferHandle',
  mojom.NULLABLE_STRING.spec:       'decodeString',
  mojom.SHAREDBUFFER.spec:          'decodeSharedBufferHandle',
  mojom.STRING.spec:                'decodeString',
  mojom.UINT16.spec:                'decodeUint16',
  mojom.UINT32.spec:                'decodeUint32',
  mojom.UINT64.spec:                'decodeUint64',
  mojom.UINT8.spec:                 'decodeUint8',
}

_spec_to_encode_method = {
  mojom.BOOL.spec:                  'encodeBool',
  mojom.DCPIPE.spec:                'encodeConsumerHandle',
  mojom.DOUBLE.spec:                'encodeDouble',
  mojom.DPPIPE.spec:                'encodeProducerHandle',
  mojom.FLOAT.spec:                 'encodeFloat',
  mojom.HANDLE.spec:                'encodeHandle',
  mojom.INT16.spec:                 'encodeInt16',
  mojom.INT32.spec:                 'encodeInt32',
  mojom.INT64.spec:                 'encodeInt64',
  mojom.INT8.spec:                  'encodeInt8',
  mojom.MSGPIPE.spec:               'encodeMessagePipeHandle',
  mojom.NULLABLE_DCPIPE.spec:       'encodeConsumerHandle',
  mojom.NULLABLE_DPPIPE.spec:       'encodeProducerHandle',
  mojom.NULLABLE_HANDLE.spec:       'encodeHandle',
  mojom.NULLABLE_MSGPIPE.spec:      'encodeMessagePipeHandle',
  mojom.NULLABLE_SHAREDBUFFER.spec: 'encodeSharedBufferHandle',
  mojom.NULLABLE_STRING.spec:       'encodeString',
  mojom.SHAREDBUFFER.spec:          'encodeSharedBufferHandle',
  mojom.STRING.spec:                'encodeString',
  mojom.UINT16.spec:                'encodeUint16',
  mojom.UINT32.spec:                'encodeUint32',
  mojom.UINT64.spec:                'encodeUint64',
  mojom.UINT8.spec:                 'encodeUint8',
}

# The mojom_types.mojom and service_describer.mojom files are special because
# they are used to generate mojom Type's and ServiceDescription implementations.
# They need to be imported, unless the file itself is being generated.
_service_describer_pkg_short = "service_describer"
_service_describer_pkg = "package:mojo/mojo/bindings/types/%s.mojom.dart" % \
  _service_describer_pkg_short
_mojom_types_pkg_short = "mojom_types"
_mojom_types_pkg = "package:mojo/mojo/bindings/types/%s.mojom.dart" % \
  _mojom_types_pkg_short

def GetDartType(kind):
  if kind.imported_from:
    return kind.imported_from["unique_name"] + "." + GetNameForElement(kind)
  return GetNameForElement(kind)

def DartDefaultValue(field):
  if field.default:
    if mojom.IsStructKind(field.kind):
      assert field.default == "default"
      return "new %s()" % GetDartType(field.kind)
    if mojom.IsEnumKind(field.kind):
      return ("new %s(%s)" %
          (GetDartType(field.kind), ExpressionToText(field.default)))
    return ExpressionToText(field.default)
  if field.kind in mojom.PRIMITIVES:
    return _kind_to_dart_default_value[field.kind]
  if mojom.IsStructKind(field.kind):
    return "null"
  if mojom.IsUnionKind(field.kind):
    return "null"
  if mojom.IsArrayKind(field.kind):
    return "null"
  if mojom.IsMapKind(field.kind):
    return "null"
  if mojom.IsInterfaceKind(field.kind) or \
     mojom.IsInterfaceRequestKind(field.kind):
    return "null"
  if mojom.IsEnumKind(field.kind):
    return "null"

def DartDeclType(kind):
  if kind in mojom.PRIMITIVES:
    return _kind_to_dart_decl_type[kind]
  if mojom.IsStructKind(kind):
    return GetDartType(kind)
  if mojom.IsUnionKind(kind):
    return GetDartType(kind)
  if mojom.IsArrayKind(kind):
    array_type = DartDeclType(kind.kind)
    return "List<" + array_type + ">"
  if mojom.IsMapKind(kind):
    key_type = DartDeclType(kind.key_kind)
    value_type = DartDeclType(kind.value_kind)
    return "Map<"+ key_type + ", " + value_type + ">"
  if mojom.IsInterfaceKind(kind):
    return ("%sInterface" % GetDartType(kind))
  if mojom.IsInterfaceRequestKind(kind):
    return ("%sInterfaceRequest" % GetDartType(kind.kind))
  if mojom.IsEnumKind(kind):
    return GetDartType(kind)

def NameToComponent(name):
  # insert '_' between anything and a Title name (e.g, HTTPEntry2FooBar ->
  # HTTP_Entry2_FooBar). Numbers terminate a string of lower-case characters.
  name = re.sub('([^_])([A-Z][^A-Z1-9_]+)', r'\1_\2', name)
  # insert '_' between non upper and start of upper blocks (e.g.,
  # HTTP_Entry2_FooBar -> HTTP_Entry2_Foo_Bar).
  name = re.sub('([^A-Z_])([A-Z])', r'\1_\2', name)
  return [x.lower() for x in name.split('_')]

def UpperCamelCase(name):
  return ''.join([x.capitalize() for x in NameToComponent(name)])

def CamelCase(name):
  uccc = UpperCamelCase(name)
  return uccc[0].lower() + uccc[1:]

def DotToUnderscore(name):
    return name.replace('.', '_')

def IsParamStruct(kind):
  assert(isinstance(kind, mojom.Struct))
  return kind.name.endswith('_Params')

# This may generate Dart reserved words. Call GetNameForElement to avoid
# generating reserved words.
def GetNameForElementUnsafe(element):
  if (mojom.IsInterfaceKind(element) or mojom.IsUnionKind(element)):
    return UpperCamelCase(element.name)
  if mojom.IsStructKind(element):
    if (IsParamStruct(element)):
      # Param Structs are library private.
      return '_' + UpperCamelCase(element.name)
    return UpperCamelCase(element.name)
  if mojom.IsInterfaceRequestKind(element):
    return GetNameForElement(element.kind)
  if isinstance(element, (mojom.Constant,
                          mojom.EnumField,
                          mojom.Field,
                          mojom.Method,
                          mojom.NamedValue,
                          mojom.Parameter,
                          mojom.UnionField)):
    return CamelCase(element.name)
  if mojom.IsEnumKind(element):
    # If the enum is nested in some other mojom element, then we
    # mangle the enum name by prepending it with the name of the containing
    # element.
    if element.parent_kind:
      return ("%s%s" % (GetNameForElement(element.parent_kind),
                        UpperCamelCase(element.name)))
    return UpperCamelCase(element.name)
  if isinstance(element, mojom.EnumValue):
    return (GetNameForElement(element.enum) + '.' + CamelCase(element.name))
  raise Exception('Unexpected element: %s' % element)

def GetNameForElement(element):
  name = GetNameForElementUnsafe(element)
  if name in _reserved_words:
    name = name + '_'
  return name

def GetInterfaceResponseName(method):
  return UpperCamelCase(method.name + 'Response')

def GetDartTrueFalse(value):
  return 'true' if value else 'false'

def GetArrayNullabilityFlags(kind):
    """Returns nullability flags for an array type, see codec.dart.

    As we have dedicated decoding functions for arrays, we have to pass
    nullability information about both the array itself, as well as the array
    element type there.
    """
    assert mojom.IsArrayKind(kind)
    ARRAY_NULLABLE   = 'bindings.kArrayNullable'
    ELEMENT_NULLABLE = 'bindings.kElementNullable'
    NOTHING_NULLABLE = 'bindings.kNothingNullable'

    flags_to_set = []
    if mojom.IsNullableKind(kind):
        flags_to_set.append(ARRAY_NULLABLE)
    if mojom.IsNullableKind(kind.kind):
        flags_to_set.append(ELEMENT_NULLABLE)

    if not flags_to_set:
        flags_to_set = [NOTHING_NULLABLE]
    return ' | '.join(flags_to_set)

def AppendDecodeParams(initial_params, kind, bit):
  """ Appends standard parameters for decode calls. """
  params = list(initial_params)
  if (kind == mojom.BOOL):
    params.append(str(bit))
  if mojom.IsReferenceKind(kind):
    if mojom.IsArrayKind(kind):
      params.append(GetArrayNullabilityFlags(kind))
    else:
      params.append(GetDartTrueFalse(mojom.IsNullableKind(kind)))
  if mojom.IsInterfaceKind(kind):
    params.append('%sProxy.newFromEndpoint' % GetDartType(kind))
  if mojom.IsArrayKind(kind) and mojom.IsInterfaceKind(kind.kind):
    params.append('%sProxy.newFromEndpoint' % GetDartType(kind.kind))
  if mojom.IsInterfaceRequestKind(kind):
    params.append('%sStub.newFromEndpoint' % GetDartType(kind.kind))
  if mojom.IsArrayKind(kind) and mojom.IsInterfaceRequestKind(kind.kind):
    params.append('%sStub.newFromEndpoint' % GetDartType(kind.kind.kind))
  if mojom.IsArrayKind(kind):
    params.append(GetArrayExpectedLength(kind))
  return params

def AppendEncodeParams(initial_params, kind, bit):
  """ Appends standard parameters shared between encode and decode calls. """
  params = list(initial_params)
  if (kind == mojom.BOOL):
    params.append(str(bit))
  if mojom.IsReferenceKind(kind):
    if mojom.IsArrayKind(kind):
      params.append(GetArrayNullabilityFlags(kind))
    else:
      params.append(GetDartTrueFalse(mojom.IsNullableKind(kind)))
  if mojom.IsArrayKind(kind):
    params.append(GetArrayExpectedLength(kind))
  return params

def DecodeMethod(kind, offset, bit):
  def _DecodeMethodName(kind):
    if mojom.IsArrayKind(kind):
      return _DecodeMethodName(kind.kind) + 'Array'
    if mojom.IsInterfaceRequestKind(kind):
      return 'decodeInterfaceRequest'
    if mojom.IsInterfaceKind(kind):
      return 'decodeServiceInterface'
    return _spec_to_decode_method[kind.spec]
  methodName = _DecodeMethodName(kind)
  params = AppendDecodeParams([ str(offset) ], kind, bit)
  return '%s(%s)' % (methodName, ', '.join(params))

def EncodeMethod(kind, variable, offset, bit):
  def _EncodeMethodName(kind):
    if mojom.IsStructKind(kind):
      return 'encodeStruct'
    if mojom.IsUnionKind(kind):
      return 'encodeUnion'
    if mojom.IsArrayKind(kind):
      return _EncodeMethodName(kind.kind) + 'Array'
    if mojom.IsEnumKind(kind):
      return 'encodeEnum'
    if mojom.IsInterfaceRequestKind(kind):
      return 'encodeInterfaceRequest'
    if mojom.IsInterfaceKind(kind):
      return 'encodeInterface'
    return _spec_to_encode_method[kind.spec]
  methodName = _EncodeMethodName(kind)
  params = AppendEncodeParams([ variable, str(offset) ], kind, bit)
  return '%s(%s)' % (methodName, ', '.join(params))

def TranslateConstants(token):
  if isinstance(token, mojom.BuiltinValue):
    if token.value == "double.INFINITY" or token.value == "float.INFINITY":
      return "double.INFINITY";
    if token.value == "double.NEGATIVE_INFINITY" or \
       token.value == "float.NEGATIVE_INFINITY":
      return "double.NEGATIVE_INFINITY";
    if token.value == "double.NAN" or token.value == "float.NAN":
      return "double.NAN";

  # Strip leading '+'.
  if token[0] == '+':
    token = token[1:]

  return token

def ExpressionToText(token):
  if isinstance(token, (mojom.EnumValue, mojom.NamedValue)):
    return str(token.resolved_value)
  return TranslateConstants(token)

def GetArrayKind(kind, size = None):
  if size is None:
    return mojom.Array(kind)
  else:
    array = mojom.Array(kind, 0)
    array.dart_map_size = size
    return array

def GetArrayExpectedLength(kind):
  if mojom.IsArrayKind(kind) and kind.length is not None:
    return getattr(kind, 'dart_map_size', str(kind.length))
  else:
    return 'bindings.kUnspecifiedArrayLength'

def IsPointerArrayKind(kind):
  if not mojom.IsArrayKind(kind):
    return False
  sub_kind = kind.kind
  return mojom.IsObjectKind(sub_kind)

def IsEnumArrayKind(kind):
  return mojom.IsArrayKind(kind) and mojom.IsEnumKind(kind.kind)

def IsImportedKind(kind):
  return hasattr(kind, 'imported_from') and kind.imported_from

def ParseStringAttribute(attribute):
  assert isinstance(attribute, basestring)
  return attribute

def GetPackage(module):
  if module.attributes and 'DartPackage' in module.attributes:
    package = ParseStringAttribute(module.attributes['DartPackage'])
    package = package.strip()
    if package != '':
      return package
  # Default package.
  return 'mojom'

def GetImportUri(module):
  package = GetPackage(module);
  elements = module.namespace.split('.')
  elements.append("%s" % module.name)
  return os.path.join(package, *elements)

def RaiseHelper(msg):
    raise Exception(msg)

class Generator(generator.Generator):

  dart_filters = {
    'array_expected_length': GetArrayExpectedLength,
    'array': GetArrayKind,
    'decode_method': DecodeMethod,
    'default_value': DartDefaultValue,
    'encode_method': EncodeMethod,
    'is_imported_kind': IsImportedKind,
    'is_array_kind': mojom.IsArrayKind,
    'is_map_kind': mojom.IsMapKind,
    'is_numerical_kind': mojom.IsNumericalKind,
    'is_any_handle_kind': mojom.IsAnyHandleKind,
    'is_string_kind': mojom.IsStringKind,
    'is_nullable_kind': mojom.IsNullableKind,
    'is_pointer_array_kind': IsPointerArrayKind,
    'is_enum_array_kind': IsEnumArrayKind,
    'is_struct_kind': mojom.IsStructKind,
    'is_union_kind': mojom.IsUnionKind,
    'is_enum_kind': mojom.IsEnumKind,
    'is_interface_kind': mojom.IsInterfaceKind,
    'is_interface_request_kind': mojom.IsInterfaceRequestKind,
    'dart_true_false': GetDartTrueFalse,
    'dart_type': DartDeclType,
    'name': GetNameForElement,
    'interface_response_name': GetInterfaceResponseName,
    'dot_to_underscore': DotToUnderscore,
    'is_cloneable_kind': mojom.IsCloneableKind,
    'upper_camel': UpperCamelCase,
    'lower_camel': CamelCase,
    'raise': RaiseHelper,
  }

  # If set to True, then mojom type information will be generated.
  should_gen_mojom_types = False

  def GetParameters(self, args):
    package = self.module.name.split('.')[0]

    # True if handles are used anywhere in the mojom.
    has_handles = any(not mojom.IsCloneableKind(kind)
                      for kind in (self.GetStructs() +
                                   self.GetStructsFromMethods() +
                                   self.GetUnions()))

    parameters = {
      "namespace": self.module.namespace,
      "imports": self.GetImports(args),
      "kinds": self.module.kinds,
      "enums": self.module.enums,
      "module": resolver.ResolveConstants(self.module, ExpressionToText),
      "structs": self.GetStructs() + self.GetStructsFromMethods(),
      "unions": self.GetUnions(),
      "interfaces": self.GetInterfaces(),
      "imported_interfaces": self.GetImportedInterfaces(),
      "imported_from": self.ImportedFrom(),
      "typepkg": '%s.' % _mojom_types_pkg_short,
      "descpkg": '%s.' % _service_describer_pkg_short,
      "mojom_types_import": 'import \'%s\' as %s;' % \
        (_mojom_types_pkg, _mojom_types_pkg_short),
      "service_describer_import": 'import \'%s\' as %s;' % \
        (_service_describer_pkg, _service_describer_pkg_short),
      "has_handles": has_handles,
    }

    # If this is the mojom types package, clear the import-related params.
    if package == _mojom_types_pkg_short:
      parameters["typepkg"] = ""
      parameters["mojom_types_import"] = ""

    # If this is the service describer package, clear the import-related params.
    if package == _service_describer_pkg_short:
      parameters["descpkg"] = ""
      parameters["service_describer_import"] = ""

    # If no interfaces were defined, the service describer import isn't needed.
    if len(self.module.interfaces) == 0:
      parameters["service_describer_import"] = ""

    return parameters

  def GetGlobals(self):
    return {
      'should_gen_mojom_types': self.should_gen_mojom_types,
    }

  @UseJinja("dart_templates/module.lib.tmpl", filters=dart_filters)
  def GenerateLibModule(self, args):
    return self.GetParameters(args)


  def GenerateFiles(self, args):
    self.should_gen_mojom_types = "--generate_type_info" in args

    elements = self.module.namespace.split('.')
    elements.append("%s.dart" % self.module.name)

    lib_module = self.GenerateLibModule(args)

    # List of packages with checked in bindings.
    # TODO(johnmccutchan): Stop generating bindings as part of build system
    # and then remove this.
    packages_with_checked_in_bindings = [
      '_mojo_for_test_only',
      'mojo',
      'mojo_apptest',
      'mojo_services',
      'mojo_sdk',
      'mojom',
    ]
    package_name = GetPackage(self.module)
    if not (package_name in packages_with_checked_in_bindings):
      pkg_path = os.path.join("dart-pkg", package_name, "lib", *elements)
      self.Write(lib_module, pkg_path)

    gen_path = os.path.join("dart-gen", package_name, "lib", *elements)
    full_gen_path = os.path.join(self.output_dir, gen_path)
    self.Write(lib_module, gen_path)

    link = self.MatchMojomFilePath("%s.dart" % self.module.name)
    full_link_path = os.path.join(self.output_dir, link)
    try:
      os.unlink(full_link_path)
    except OSError, exc:
      # If the file does not exist, ignore the error.
      if errno.ENOENT == exc.errno:
        pass
      else:
        raise
    fileutil.EnsureDirectoryExists(os.path.dirname(full_link_path))
    try:
      if sys.platform == "win32":
        shutil.copy(full_gen_path, full_link_path)
      else:
        os.symlink(full_gen_path, full_link_path)
    except OSError as e:
      # Errno 17 is file already exists. If the link fails because file already
      # exists assume another instance of this script tried to create the same
      # file and continue on.
      if e.errno != 17:
        raise e

  def GetImports(self, args):
    used_imports = self.GetUsedImports(self.module)
    used_names = set()
    for each_import in used_imports.values():
      simple_name = each_import["module_name"].split(".")[0]

      # Since each import is assigned a library in Dart, they need to have
      # unique names.
      unique_name = simple_name
      counter = 0
      while unique_name in used_names:
        counter += 1
        unique_name = simple_name + str(counter)

      used_names.add(unique_name)
      each_import["unique_name"] = unique_name + '_mojom'
      counter += 1

      each_import["rebased_path"] = GetImportUri(each_import['module'])
    return sorted(used_imports.values(), key=lambda x: x['rebased_path'])

  def GetImportedInterfaces(self):
    interface_to_import = {}
    for each_import in self.module.imports:
      for each_interface in each_import["module"].interfaces:
        name = each_interface.name
        interface_to_import[name] = each_import["unique_name"] + "." + name
    return interface_to_import

  def ImportedFrom(self):
    interface_to_import = {}
    for each_import in self.module.imports:
      for each_interface in each_import["module"].interfaces:
        name = each_interface.name
        interface_to_import[name] = each_import["unique_name"] + "."
    return interface_to_import
