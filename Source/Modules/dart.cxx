/* -----------------------------------------------------------------------------
 * See the LICENSE file for information on copyright, usage and redistribution
 * of SWIG, and the README file for authors - http://www.swig.org/release.html.
 *
 * dart.cxx
 *
 * Dart language module for SWIG.
 * ----------------------------------------------------------------------------- */

#include "swigmod.h"
#include "cparse.h"
#include <ctype.h>
#include <stdarg.h>

class DART:public Language {
protected:
  File *f_dart;
  File *f_c;
  File *f_dart_head;
  File *f_dart_body;
  File *f_dart_end;
  int indentation;

  // Users can pass in the location of the library file (normally .so on
  // linux, .dylib on mac. By default we assume that the library file is
  // named the same as the module.
  String *library_file;

  Hash *class_types;

public:
  DART():
    indentation(0),
    class_types(NULL){
  }


  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */
  virtual void main(int argc, char *argv[]) {
    Printf(stdout, "start running\n\n");

    for (int i = 1; i < argc; i++) {
      if (argv[i] && strcmp(argv[i], "-soname") == 0) {
        if (argv[i + 1]) {
          library_file = NewString(argv[i + 1]);
          Swig_mark_arg(i);
          Swig_mark_arg(i + 1);
          i++;
        } else {
          Swig_arg_error();
        }
      }
    }

    /* Set language-specific subdirectory in SWIG library */
    SWIG_library_directory("dart");

    /* Set language-specific preprocessing symbol */
    Preprocessor_define("SWIGDART 1", 0);

    /* Set language-specific configuration file */
    SWIG_config_file("dart.swg");

    /* Set typemap language (historical) */
    SWIG_typemap_lang("dart");

  }

  void indent() {
    indentation += 2;
  }

  void unindent() {
    if (indentation < 2) {
      Printf(stderr, "Trying to unindent when indentation < 2");
    	SWIG_exit(EXIT_FAILURE);
    }
    indentation -= 2;
  }

  void emit_indent(File *f) {
    for (int i = 0; i < indentation; i++) Printf(f, " ");
  }

  void emit_library_load(Node *n) {
    String *module;
    if (library_file) {
      module = library_file;
    } else {
      module = NewString("");
      Printf(module, "%s.so", Getattr(n, "name"));
    }
    Printf(f_dart_body,
           "final Uri __$uri__ = Uri.base.resolve('%s');\n",
           module);

    Printf(f_dart_body,
           "final __$library__ = new ForeignLibrary.fromName(__$uri__.path);\n\n",
           module);
  }

  virtual int top(Node *n) {
    class_types = NewHash();

    String *module = Getattr(n, "name");
    String *dart_filename = NewString("");

    Printf(dart_filename, "%s%s.dart", SWIG_output_directory(), module);

    String *c_filename = Getattr(n, "outfile");
    //    Printf(c_filename, "%s%s.c", SWIG_output_directory(), module);

    f_dart = NewFile(dart_filename, "w", SWIG_output_files());
    f_c = NewFile(c_filename, "w", SWIG_output_files());
    if (!f_dart) {
       FileErrorDisplay(dart_filename);
       SWIG_exit(EXIT_FAILURE);
    }

    File *f_null = NewString("");
    Swig_register_filebyname("runtime", f_null);

    f_dart_head = NewString("");
    f_dart_body = NewString("");
    f_dart_end = NewString("");

    Swig_register_filebyname("swigdart", f_dart);
    Swig_register_filebyname("dart_head", f_dart_head);
    Swig_register_filebyname("dart_body", f_dart_body);
    Swig_register_filebyname("dart_end", f_dart_end);
    Swig_banner(f_null);


    emit_library_load(n);
    /* Emit code for children */
    Language::top(n);

    /* Write all to the file */
    Dump(f_dart_head, f_dart);
    Dump(f_dart_body, f_dart);
    Dump(f_dart_end, f_dart);
    Dump(f_null, f_c);

    /* Cleanup files */
    Delete(f_c);
    Delete(f_dart);
    Delete(f_dart_head);
    Delete(f_dart_body);
    Delete(f_dart_end);
    Delete(f_null);
    assert(indentation == 0);
    return SWIG_OK;
  }

  int getSize(String *type) {
    // TODO(ricow): refactor
    if (Strcmp(type, "double") == 0) {
      return 8;
    } else if (Strcmp(type, "int") == 0) {
      return 4;
    } else {
      Printf(stderr, "Don't know size of %s\n", type);
      SWIG_exit(EXIT_FAILURE);
      return 0;
    }
  }

  String *getFFIGetter(String *type) {
    // TODO(ricow): refactor
    if (Strcmp(type, "double") == 0) {
      return NewString("getFloat64");
    } else if (Strcmp(type, "int") == 0) {
      return NewString("getInt32");
    } else {
      Printf(stderr, "Don't know size of %s\n", type);
      SWIG_exit(EXIT_FAILURE);
      return 0;

    }
  }

  String *getFFISetter(String *type) {
    // TODO(ricow): refactor
    if (Strcmp(type, "double") == 0) {
      return NewString("setFloat64");
    } else if (Strcmp(type, "int") == 0) {
      return NewString("setInt32");
    } else {
      Printf(stderr, "Don't know size of %s\n", type);
      SWIG_exit(EXIT_FAILURE);
      return 0;

    }
  }

  /* We explicitly catch this to not have the class expanded into constructors
     All we really need is to know the type and name of the members
  */
  virtual int classDeclaration(Node *n) {
    // STRUCTS ONLY FOR NOW
    String *name = Getattr(n, "name");
    Swig_print(n);
    Printf(stdout, "Calling classDeclaration %s\n", nodeType(n));
    Printf(f_dart_body, "class %s {\n", name);
    Printf(f_dart_body, "  ForeignMemory __$fmem__;\n");

    Node *c;
    String *constructor_parameters = NewString("");
    String *constructor_initializers = NewString("");
    int struct_size = 0;
    for (c = firstChild(n); c; c = nextSibling(c)) {
      Printf(stdout, "  Sibling %s\n", nodeType(c));
      String *variable_type = Getattr(c, "type");
      String *variable_name = Getattr(c, "name");
      Printf(f_dart_body, "  %s get %s => ", variable_type, variable_name);
      Printf(f_dart_body, "__$fmem__.%s(%d);\n", getFFIGetter(variable_type),
             struct_size);
      Printf(f_dart_body, "  set %s(%s val) => ", variable_name, variable_type);
      Printf(f_dart_body, "__$fmem__.%s(%d, val);\n",
             getFFISetter(variable_type),
             struct_size);
      Printf(constructor_initializers, "    __$fmem__.%s(%d, %s);\n",
             getFFISetter(variable_type),
             struct_size,
             variable_name);

      if (struct_size == 0) {
        Printf(constructor_parameters, "%s %s", variable_type, variable_name);
      } else {
        Printf(constructor_parameters, ", %s %s", variable_type, variable_name);
      }

      struct_size += getSize(variable_type);
    }
    Printf(f_dart_body, "  %s(%s) {\n", name, constructor_parameters);
    Printf(f_dart_body, "    __$fmem__ = new ForeignMemory.allocated(%d);\n",
           struct_size);
    Printf(f_dart_body, "%s\n",
           constructor_initializers);

    Printf(f_dart_body, "  }\n");
    Printf(f_dart_body, "}\n\n");
    // Register the type so that we can used it for declarations later on.
    Parm *pattern = NewParm(name, NULL, n);
    Swig_typemap_register("darttype", pattern, name, NULL, NULL);

    return SWIG_OK;
  }


  virtual int constantWrapper(Node *n) {
    Printf(stdout, "Calling constantWrapper %s\n", nodeType(n));
    return SWIG_OK;
  }
  virtual int variableWrapper(Node *n) {
    Printf(stdout, "Calling variableWrapper %s\n", nodeType(n));
    Printf(stdout, "Name %s\n", Getattr(n, "name"));
    return SWIG_OK;
  }
  virtual int nativeWrapper(Node *n) {
    Printf(stdout, "Calling nativeWrapper %s\n", nodeType(n));
    return SWIG_OK;
  }

  virtual int classDirector(Node *n) {
    Printf(stdout, "Calling classDirector %s\n", nodeType(n));
    return SWIG_OK;
  }

  virtual int functionHandler(Node *n) {
    if (!Strcmp(nodeType(n), "cdecl") == 0) {
      Printf(stdout, "Not handling: %s\n", nodeType(n));
      return SWIG_OK;
    }

    String *kind = Getattr(n, "kind");

    // Currently no support for variables.
    if (Strcmp(kind, "variable") == 0) return SWIG_OK;

    String *funcname = Getattr(n, "sym:name");
    ParmList *pl = Getattr(n, "parms");
    Parm *p;
    String *raw_return_type = Swig_typemap_lookup("darttype", n, "", 0);
    // Create lazy initialized function lookup
    Printf(f_dart_body, "final __$%s__ = __$library__.lookup('%s');\n",
           funcname, funcname);

    Printf(f_dart_body, "%s ", raw_return_type);
    int argnum = 0;
    Printf(f_dart_body, "%s(", funcname);
    String *arguments = NewString("");
    for (p = pl; p; p = nextSibling(p), argnum++) {
      String *arg_type = Swig_typemap_lookup("darttype", , "", 0);
      Printf(stdout, "foobar: %s\n", arg_type);
      Swig_print(p);
      String *name  = Getattr(p,"name");
      if (argnum > 0) Printf(f_dart_body, ", ");
      Printf(f_dart_body, "%s %s", arg_type, name);

      // We later pass these on to the icall
      if (argnum > 0) Printf(arguments, ", ");

      // If there was no arg type we assume that this is a wrapped foreign
      // memory.
      if (!arg_type) {
        Printf(arguments, "%s.__$fmem__", name);
      } else {
        Printf(arguments, "%s", name);
      }

    }
    Printf(f_dart_body, ") {\n");
    if (Strcmp(raw_return_type, "int") == 0) {
      // Printf(f_dart_body,
      //        "  ForeignFunction f = __$wrapper__.getFunction('%s');\n",
      //        funcname);
      Printf(f_dart_body, "  return __$%s__.icall$%d(%s);\n",
             funcname, argnum, arguments);
    }

    Printf(f_dart_body, "}\n\n");
    // String *name = Getattr(, "name");
    // String *iname   = Getattr(n,"sym:name");
    // SwigType *type   = Getattr(n,"type");
    // ParmList *parms  = Getattr(n,"parms");


    return SWIG_OK;
  }

  virtual int functionWrapper(Node *n) {
    if (!Strcmp(nodeType(n), "cdecl") == 0) {
      Printf(stdout, "Not handling: %s\n", nodeType(n));
      return SWIG_OK;
    }

    String *kind = Getattr(n, "kind");

    // Currently no support for variables.
    if (Strcmp(kind, "variable") == 0) return SWIG_OK;

    String *funcname = Getattr(n, "sym:name");
    ParmList *pl = Getattr(n, "parms");
    Parm *p;
    String *raw_return_type = Swig_typemap_lookup("darttype", n, "", 0);
    // Create lazy initialized function lookup
    Printf(f_dart_body, "final __$%s__ = __$library__.lookup('%s');\n",
           funcname, funcname);

    Printf(f_dart_body, "%s ", raw_return_type);
    int argnum = 0;
    Printf(f_dart_body, "%s(", funcname);
    String *arguments = NewString("");
    for (p = pl; p; p = nextSibling(p), argnum++) {
      String *arg_type = Swig_typemap_lookup("darttype", p, "", 0);
      //      Swig_print(p);
      String *name  = Getattr(p,"name");
      if (argnum > 0) Printf(f_dart_body, ", ");
      Printf(f_dart_body, "%s %s", arg_type, name);

      // We later pass these on to the icall
      if (argnum > 0) Printf(arguments, ", ");

      // If there was no arg type we assume that this is a wrapped foreign
      // memory.
      if (!arg_type) {
        Printf(arguments, "%s.__$fmem__", name);
      } else {
        Printf(arguments, "%s", name);
      }

    }
    Printf(f_dart_body, ") {\n");
    if (Strcmp(raw_return_type, "int") == 0) {
      // Printf(f_dart_body,
      //        "  ForeignFunction f = __$wrapper__.getFunction('%s');\n",
      //        funcname);
      Printf(f_dart_body, "  return __$%s__.icall$%d(%s);\n",
             funcname, argnum, arguments);
    }

    Printf(f_dart_body, "}\n\n");
    // String *name = Getattr(, "name");
    // String *iname   = Getattr(n,"sym:name");
    // SwigType *type   = Getattr(n,"type");
    // ParmList *parms  = Getattr(n,"parms");


    return SWIG_OK;
  }

};

extern "C" Language *swig_dart(void) {
  return new DART();
}

