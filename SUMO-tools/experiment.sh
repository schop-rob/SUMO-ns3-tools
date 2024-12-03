
#!/bin/bash

for i in $(seq 50 50 800)
do
    # generate the traffic
    python "/usr/local/Cellar/sumo/1.20.0/share/sumo/tools/randomTrips.py" -n osm.net.xml.gz --fringe-factor 5 --insertion-density $i -o osm.passenger.trips.xml -r osm.passenger.rou.xml -b 0 -e 150 --trip-attributes "departLane=\"best\"" --fringe-start-attributes "departSpeed=\"max\"" --validate --remove-loops --via-edge-types highway.motorway,highway.motorway_link,highway.trunk_link,highway.primary_link,highway.secondary_link,highway.tertiary_link --vehicle-class passenger --vclass passenger --prefix veh --min-distance 300 --min-distance.fringe 10 --allow-fringe.min-length 1000 --lanes

    # run simulation
    ./run.bat

    python constant_vehicles.py --vehicleCount ${i} --start_time 250 --simulation_time 200 --input_file veh_positions.xml --output_file perfect.xml

    #convert tracefile
    python ../traceExporter.py -i perfect.xml --ns2mobility-output=ns2_mobility_${i}density.tcl
done