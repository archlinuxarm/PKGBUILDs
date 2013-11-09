FILE(REMOVE_RECURSE
  "CMakeFiles/sclang-language.elc"
  "sclang-language.elc"
)

# Per-language clean rules from dependency scanning.
FOREACH(lang)
  INCLUDE(CMakeFiles/sclang-language.elc.dir/cmake_clean_${lang}.cmake OPTIONAL)
ENDFOREACH(lang)
