#!/usr/bin/env python3
"""
Generate JSON files from .ini configuration and OpenCL kernel files.

This script:
1. Reads kernel configuration from .ini files
2. Parses OpenCL kernel function signatures from .cl files
3. Generates JSON files with algorithm metadata and kernel signatures
"""

import os
import re
import json
import configparser
import sys
from pathlib import Path


def parse_opencl_function_signature(cl_file_path, function_name):
    """
    Parse an OpenCL function signature from a .cl file.

    Args:
        cl_file_path: Path to the .cl kernel file
        function_name: Name of the kernel function to find

    Returns:
        List of dictionaries with 'type' and 'name' for each parameter
    """
    with open(cl_file_path, 'r') as f:
        content = f.read()

    # Match kernel function signature
    # Pattern: __kernel void function_name(params...)
    pattern = rf'__kernel\s+void\s+{re.escape(function_name)}\s*\((.*?)\)'
    match = re.search(pattern, content, re.DOTALL)

    if not match:
        print(f"Warning: Could not find function '{function_name}' in {cl_file_path}", file=sys.stderr)
        return []

    params_str = match.group(1)

    # Split parameters by comma (handling multi-line)
    params_str = re.sub(r'\s+', ' ', params_str)  # Normalize whitespace
    param_list = []

    # Split by comma but be careful with nested types
    params = [p.strip() for p in params_str.split(',')]

    for param in params:
        if not param:
            continue

        # Parse parameter: type + name
        # Handle qualifiers: __global, __constant, __local, __private, const
        # Extract the base type and parameter name

        # Remove qualifiers and extract type and name
        param_clean = param.strip()

        # Pattern: (qualifiers*) (type) (name)
        # Example: "__global const uchar* input" -> type="uchar*", name="input"
        # Example: "int width" -> type="int", name="width"

        # Remove address space qualifiers
        param_clean = re.sub(r'\b__global\b', '', param_clean)
        param_clean = re.sub(r'\b__constant\b', '', param_clean)
        param_clean = re.sub(r'\b__local\b', '', param_clean)
        param_clean = re.sub(r'\b__private\b', '', param_clean)
        param_clean = re.sub(r'\bconst\b', '', param_clean)
        param_clean = param_clean.strip()

        # Now extract type and name
        # The name is the last token, everything else is the type
        tokens = param_clean.split()
        if len(tokens) < 2:
            print(f"Warning: Could not parse parameter: {param}", file=sys.stderr)
            continue

        param_name = tokens[-1]
        param_type = ' '.join(tokens[:-1])

        # Normalize type (remove extra spaces around *)
        param_type = re.sub(r'\s*\*\s*', '*', param_type)

        param_list.append({
            'type': param_type,
            'name': param_name
        })

    return param_list


def generate_json_from_ini(ini_file_path, output_dir='batch'):
    """
    Generate JSON file from an .ini configuration file.

    Args:
        ini_file_path: Path to the .ini file
        output_dir: Output directory for JSON files
    """
    config = configparser.ConfigParser(inline_comment_prefixes=('#', ';'))
    config.read(ini_file_path)

    # Extract algorithm name from filename (e.g., gaussian5x5.ini -> gaussian5x5)
    algo_name = Path(ini_file_path).stem

    # Find all kernel variants
    kernel_sections = [s for s in config.sections() if s.startswith('kernel.')]

    if not kernel_sections:
        print(f"Warning: No kernel sections found in {ini_file_path}", file=sys.stderr)
        return

    # Build JSON structure
    json_data = {
        'algorithm': algo_name,
        'variants': []
    }

    for section in sorted(kernel_sections):
        kernel_file = config.get(section, 'kernel_file')
        kernel_function = config.get(section, 'kernel_function')

        # Get absolute path to kernel file
        ini_dir = Path(ini_file_path).parent.parent  # Go up from config/ to root
        kernel_path = ini_dir / kernel_file

        if not kernel_path.exists():
            print(f"Warning: Kernel file not found: {kernel_path}", file=sys.stderr)
            continue

        # Parse function signature
        parameters = parse_opencl_function_signature(kernel_path, kernel_function)

        variant_data = {
            'function_name': kernel_function,
            'parameters': parameters
        }

        json_data['variants'].append(variant_data)

    # Create output directory if it doesn't exist
    output_path = Path(output_dir)
    output_path.mkdir(exist_ok=True)

    # Write JSON file with custom formatting (compact parameters)
    output_file = output_path / f"{algo_name}.json"
    with open(output_file, 'w') as f:
        f.write('{\n')
        f.write(f'  "algorithm": "{json_data["algorithm"]}",\n')
        f.write('  "variants": [\n')

        for i, variant in enumerate(json_data['variants']):
            f.write('    {\n')
            f.write(f'      "function_name": "{variant["function_name"]}",\n')
            f.write('      "parameters": [\n')

            for j, param in enumerate(variant['parameters']):
                comma = ',' if j < len(variant['parameters']) - 1 else ''
                f.write(f'        {json.dumps(param)}{comma}\n')

            f.write('      ]\n')
            comma = ',' if i < len(json_data['variants']) - 1 else ''
            f.write(f'    }}{comma}\n')

        f.write('  ]\n')
        f.write('}\n')

    print(f"Generated: {output_file}")
    return output_file


def main():
    """Main entry point."""
    if len(sys.argv) < 2:
        print("Usage: python generate_kernel_json.py <ini_file> [output_dir]")
        print("Example: python generate_kernel_json.py config/gaussian5x5.ini batch/")
        sys.exit(1)

    ini_file = sys.argv[1]
    output_dir = sys.argv[2] if len(sys.argv) > 2 else 'batch'

    if not os.path.exists(ini_file):
        print(f"Error: File not found: {ini_file}", file=sys.stderr)
        sys.exit(1)

    generate_json_from_ini(ini_file, output_dir)


if __name__ == '__main__':
    main()
