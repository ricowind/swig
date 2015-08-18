/* ------------------------------------------------------------
 * dart.swg
 *
 * Dart configuration module.
 * ------------------------------------------------------------ */


%typemap(darttype) int "int";
%typemap(darttype) double "double";
%typemap(darttype) void "void";

%insert(dart_head) %{
// This is generated code, only touch this if you know what you are\n
// doing.

import 'dart:fletch.ffi';
%}

%insert(dart_end) %{
void main() {
  print("Calling GetCount: ${GetCount()}");
  print("Calling setup: ${setup()}");
  print("Calling SetCount(42, 42): ${SetCount(42, 42)}");
  print("Calling GetCount: ${GetCount()}");
  var mynode = new node(42, 42.42);
  print("node first val: ${mynode.ivalue}");
  print("node first val: ${mynode.dvalue}");
  print("Calling get_i_value: ${get_i_value(mynode)}");
}

%}
