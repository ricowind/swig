/* File : example.i */
%module(directors="1") example

%include "std_string.i"

%header %{
#include "director.h"
%}

%feature("director") FooBarAbs;
%include "director.h"
