# Writes a C++ source whose function returns the text of a Metal header, so shaders compiled at
# run time see the same structs as default.metallib.
#
# cmake -DINPUT=<header> -DOUTPUT=<source.cpp> -DFUNCTION=<name> -P EmbedMetalSource.cmake

file(READ "${INPUT}" CONTENT)
file(WRITE "${OUTPUT}"
  "// Generated from ${INPUT} by EmbedMetalSource.cmake - do not edit.\n\n"
  "const char *${FUNCTION}() {\n"
  "  return R\"MSL(${CONTENT})MSL\";\n"
  "}\n")
