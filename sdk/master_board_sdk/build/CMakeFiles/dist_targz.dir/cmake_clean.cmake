file(REMOVE_RECURSE
  "doc/master-board-sdk.doxytag"
  "doc/doxygen.log"
  "doc/doxygen-html"
  "example/example.pyc"
  "CMakeFiles/dist_targz"
)

# Per-language clean rules from dependency scanning.
foreach(lang )
  include(CMakeFiles/dist_targz.dir/cmake_clean_${lang}.cmake OPTIONAL)
endforeach()
