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
   File *f_dart_head;
   File *f_dart_body;
   File *f_dart_end;
   int indentation;

public:
  DART():
    indentation(0) {
  }


  /* ------------------------------------------------------------
   * main()
   * ------------------------------------------------------------ */
  virtual void main(int argc, char *argv[]) {
    for (int i = 1; i < argc; i++) {
      if (argv[i] && strcmp(argv[i],"-library_file") == 0) {
        printf("Arg %d: %s\n", i, argv[i]);
        Swig_mark_arg(i);
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
    String *module = Getattr(n, "name");
    Printf(f_dart_body,
           "__FFIWrapper__ __wrapper__ = new __FFIWrapper__('%s.so');\n\n",
           module);
  }

  virtual int top(Node *n) {
    String *module = Getattr(n, "name");
    String *dart_filename = NewString("");

    Printf(dart_filename, "%s%s.dart", SWIG_output_directory(), module);
    f_dart = NewFile(dart_filename, "w", SWIG_output_files());
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

    emit_library_load(n);
    /* Emit code for children */
    Language::top(n);

    /* Write all to the file */
    Dump(f_dart_head, f_dart);
    Dump(f_dart_body, f_dart);
    Dump(f_dart_end, f_dart);

    /* Cleanup files */
    Delete(f_dart);
    Delete(f_dart_head);
    Delete(f_dart_body);
    Delete(f_dart_end);
    Delete(f_null);
    printf("node %p\n", n);
    assert(indentation == 0);
    return SWIG_OK;
  }

  virtual int functionWrapper(Node *n) {
    String *funcname = Getattr(n, "sym:name");
    ParmList *pl = Getattr(n, "parms");
    Parm *p;
    String *raw_return_type = Swig_typemap_lookup("darttype", n, "", 0);
    Printf(f_dart_body, "%s ", raw_return_type);
    int argnum = 0;
    Printf(f_dart_body, "%s(", funcname);
    String *arguments = NewString("");
    for (p = pl; p; p = nextSibling(p), argnum++) {
      String *arg_type = Swig_typemap_lookup("darttype", p, "", 0);
      String *name  = Getattr(p,"name");
      if (argnum > 0) Printf(f_dart_body, ", ");
      Printf(f_dart_body, "%s %s", arg_type, name);

      // We later pass these on to the icall
      if (argnum > 0) Printf(arguments, ", ");
      Printf(arguments, "%s", name);
    }
    Printf(f_dart_body, ") {\n");
    if (Strcmp(raw_return_type, "int") == 0) {
      Printf(f_dart_body,
             "  ForeignFunction f = __wrapper__.getFunction('%s');\n",
             funcname);
      Printf(f_dart_body, "  return f.icall$%d(%s);\n", argnum, arguments);
    }

    Printf(f_dart_body, "}\n");
    // String *name = Getattr(n, "name");
    // String *iname   = Getattr(n,"sym:name");
    // SwigType *type   = Getattr(n,"type");
    // ParmList *parms  = Getattr(n,"parms");


    return SWIG_OK;
  }

};

extern "C" Language *swig_dart(void) {
  return new DART();
}

