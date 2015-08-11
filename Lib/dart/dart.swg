/* ------------------------------------------------------------
 * dart.swg
 *
 * Dart configuration module.
 * ------------------------------------------------------------ */


%typemap(darttype) int "int";
%typemap(darttype) double "double";
%typemap(darttype) void "void";

%insert(dart_head) %{
// This is generated code, only touch this if you know what you are\n"
// doing

import 'dart:fletch.ffi';

class __FFIWrapper__ {
  Map<String, ForeignFunction> foreignFunctions =
      new Map<String, ForeignFunction>();
  ForeignLibrary library;

  __FFIWrapper__(String libraryFile) {
    Uri uri = Uri.base.resolve(libraryFile);
    library = new ForeignLibrary.fromName(uri.path);
  }

  // TODO(ricow): add support for different signatures for the same name (for
  // c++)
  ForeignFunction getFunction(String name) =>
      foreignFunctions.putIfAbsent(name, () => library.lookup(name));

}
%}

%insert(dart_end) %{
void main() {
  print("Calling GetCount: ${GetCount()}");
  print("Calling setup: ${setup()}");
  print("Calling SetCount(42, 42): ${SetCount(42, 42)}");
  print("Calling GetCount: ${GetCount()}");
}

%}
