import xml.etree.ElementTree as ET
import argparse
from collections import defaultdict
import copy

def parse_arguments():
    parser = argparse.ArgumentParser(description='Process vehicle XML data with specific parameters')
    parser.add_argument('--vehicleCount', type=int, required=True, help='Number of vehicles to track')
    parser.add_argument('--start_time', type=float, required=True, help='Start time of the simulation')
    parser.add_argument('--simulation_time', type=float, required=True, help='Duration of the simulation')
    parser.add_argument('--input_file', type=str, required=True, help='Input XML file path')
    parser.add_argument('--output_file', type=str, required=True, help='Output XML file path')
    return parser.parse_args()

def find_continuous_vehicles(root, start_time, end_time):
    # Track vehicle appearances in timesteps
    vehicle_appearances = defaultdict(list)
    
    # Collect all timesteps and their vehicles
    for timestep in root.findall('.//timestep'):
        time = float(timestep.get('time'))
        if time >= start_time and time <= end_time:
            for vehicle in timestep.findall('vehicle'):
                vehicle_id = vehicle.get('id')
                vehicle_appearances[vehicle_id].append(time)
    
    # Find vehicles that appear in all timesteps
    continuous_vehicles = []
    expected_timesteps = len([ts for ts in root.findall('.//timestep') 
                            if start_time <= float(ts.get('time')) <= end_time])
    
    for vehicle_id, appearances in vehicle_appearances.items():
        if len(appearances) == expected_timesteps:
            continuous_vehicles.append(vehicle_id)
            
    return continuous_vehicles

def create_new_xml(root, selected_vehicles, start_time, simulation_time):
    # Create new XML structure
    new_root = ET.Element('fcd-export')
    new_root.set('xmlns:xsi', "http://www.w3.org/2001/XMLSchema-instance")
    new_root.set('xsi:noNamespaceSchemaLocation', "http://sumo.dlr.de/xsd/fcd_file.xsd")
    
    # Copy relevant timesteps with time offset
    for timestep in root.findall('.//timestep'):
        time = float(timestep.get('time'))
        if start_time <= time < start_time + simulation_time:
            new_timestep = ET.SubElement(new_root, 'timestep')
            new_timestep.set('time', f"{time - start_time:.2f}")
            
            # Copy only selected vehicles
            for vehicle in timestep.findall('vehicle'):
                if vehicle.get('id') in selected_vehicles:
                    new_vehicle = copy.deepcopy(vehicle)
                    new_timestep.append(new_vehicle)
    
    return new_root

def main():
    args = parse_arguments()
    
    # Parse input XML
    tree = ET.parse(args.input_file)
    root = tree.getroot()
    
    # Find vehicles that appear continuously
    continuous_vehicles = find_continuous_vehicles(
        root, 
        args.start_time, 
        args.start_time + args.simulation_time
    )
    
    if len(continuous_vehicles) < args.vehicleCount:
        raise ValueError(
            f"Not enough continuous vehicles found. "
            f"Required: {args.vehicleCount}, Found: {len(continuous_vehicles)}"
        )
    
    # Select the requested number of vehicles
    selected_vehicles = continuous_vehicles[:args.vehicleCount]
    
    # Create new XML with selected vehicles
    new_root = create_new_xml(root, selected_vehicles, args.start_time, args.simulation_time)
    
    # Write to output file
    new_tree = ET.ElementTree(new_root)
    new_tree.write(args.output_file, encoding='utf-8', xml_declaration=True)
    
    print(f"Successfully processed XML. Selected vehicles: {', '.join(selected_vehicles)}")

if __name__ == "__main__":
    main()