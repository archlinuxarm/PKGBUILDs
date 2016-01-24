#!/bin/sh -f

export PATH="$PATH:$CASROOT/bin"
export LD_LIBRARY_PATH="${LD_LIBRARY_PATH:+$LD_LIBRARY_PATH:}$CASROOT/lib"

export CSF_LANGUAGE=us
export MMGT_CLEAR=1
export CSF_EXCEPTION_PROMPT=1

export CSF_SHMessage="$CASROOT"/src/SHMessage
export CSF_MDTVTexturesDirectory="$CASROOT"/src/Textures
export CSF_XSMessage="$CASROOT"/src/XSMessage
export CSF_StandardDefaults="$CASROOT"/src/StdResource
export CSF_PluginDefaults="$CASROOT"/src/StdResource
export CSF_XCAFDefaults="$CASROOT"/src/StdResource
export CSF_StandardLiteDefaults="$CASROOT"/src/StdResource
export CSF_UnitsLexicon="$CASROOT"/src/UnitsAPI/Lexi_Expr.dat
export CSF_UnitsDefinition="$CASROOT"/src/UnitsAPI/Units.dat
export CSF_IGESDefaults="$CASROOT"/src/XSTEPResource
export CSF_STEPDefaults="$CASROOT"/src/XSTEPResource
export CSF_XmlOcafResource="$CASROOT"/src/XmlOcafResource
export CSF_GraphicShr="$CASROOT"/lib/libTKOpenGl.so
