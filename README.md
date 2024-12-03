# SUMO and NS-3 tools for VANET simulation

We assume basic unix knowledge. There are many hardcoded paths (sadly), it is best to replace them with actual paths before running.
Code quality was not a priority, neither was reusability. This is a collection of scripts that were used to generate data for a research project.

## SUMO related files

In the `SUMO-tools`directory 

To make SUMO export vehicle positions to an XML, add the following to the generated osm.sumocfg inside the <configuration> tags:

```
<output>
    <fcd-output value="veh_positions.xml"/>
</output>   
```

`constant_vehicles.py` is used to filter an existing vehicle positions XML file to only a selection of continously existing vehicles across a timespan.

## NS3 related files

Any of the `scratch` subdirectories can be placed in the `scratch` directory of ns-3. They represent different scenarios that can be run with the `waf` command. The `waf` command is used in legacy versions of ns-3, we used 3.35. We did not use more up to date versions of ns-3 for convenience. The older version allows us to simply place the subdirectories into `scratch` and run the simulation.
