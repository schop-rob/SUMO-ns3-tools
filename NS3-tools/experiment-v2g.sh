#!/bin/bash

densities=(50 100 200 400)
for i in "${densities[@]}"
do
    CXXFLAGS="-std=c++17 -Wno-error=deprecated-declarations -Wno-error=unused-but-set-variable" ./waf --run "test-v2g --tcl_file=/path/to/your/ns2_mobility_${i}density.tcl"
done