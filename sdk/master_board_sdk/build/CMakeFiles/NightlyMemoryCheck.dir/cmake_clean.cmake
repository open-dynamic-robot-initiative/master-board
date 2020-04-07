file(REMOVE_RECURSE
  "doc/master-board-sdk.doxytag"
  "doc/doxygen.log"
  "doc/doxygen-html"
  "example/example.pyc"
  "CMakeFiles/NightlyMemoryCheck"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/NightlyMemoryCheck.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
