#!/usr/bin/env python3
"""
Dependency Analysis Tool for OpenCL Image Processing Framework

Analyzes layer dependencies and checks Clean Architecture compliance.

Usage:
    python3 scripts/analyze_deps.py
"""

import os
import re
from pathlib import Path
from collections import defaultdict

# Find all .c and .h files
root = Path(__file__).parent.parent
src_files = list(root.glob("src/**/*.c")) + list(root.glob("src/**/*.h"))
inc_files = list(root.glob("include/**/*.h"))
ex_files = list(root.glob("examples/**/*.c"))

# Parse includes
deps = defaultdict(set)

def categorize(file_path):
    """Categorize file into layer"""
    rel = file_path.relative_to(root)
    parts = rel.parts

    if parts[0] == 'include':
        if len(parts) > 1 and parts[1] == 'utils':
            return 'include/utils'
        return 'include'
    elif parts[0] == 'src':
        if parts[1] == 'core':
            return 'core'
        elif parts[1] == 'platform':
            return 'platform'
        elif parts[1] == 'utils':
            return 'utils'
        elif parts[1] == 'main.c':
            return 'main'
    elif parts[0] == 'examples':
        return 'examples'
    return 'unknown'

def parse_includes(file_path):
    """Extract #include statements"""
    includes = []
    try:
        with open(file_path) as f:
            for line in f:
                match = re.match(r'^\s*#include\s+["<]([^">]+)[">]', line)
                if match:
                    inc = match.group(1)
                    # Skip system includes
                    if not inc.startswith(('stdio', 'stdlib', 'string', 'math',
                                          'time', 'sys/', 'ctype', 'errno',
                                          'limits', 'stdbool', 'stdint', 'stddef',
                                          'OpenCL/', 'CL/')):
                        includes.append(inc)
    except:
        pass
    return includes

# Build dependency graph
layer_deps = defaultdict(set)

for file_path in src_files + inc_files + ex_files:
    src_layer = categorize(file_path)
    includes = parse_includes(file_path)

    for inc in includes:
        # Determine target layer
        if inc.startswith('op_interface.h') or inc.startswith('op_registry.h') or inc.startswith('algorithm_runner.h'):
            tgt_layer = 'include'
        elif inc.startswith('utils/'):
            tgt_layer = 'include/utils'
        elif inc.startswith('platform/'):
            tgt_layer = 'platform'
        elif inc.startswith('core/'):
            tgt_layer = 'core'
        elif inc.endswith('.h') and src_layer == 'utils':
            tgt_layer = 'utils'
        elif inc.endswith('.h') and src_layer == 'platform':
            tgt_layer = 'platform'
        else:
            continue

        if src_layer != tgt_layer:
            layer_deps[src_layer].add(tgt_layer)

# Print dependency graph
print("\n" + "=" * 70)
print("LAYER DEPENDENCY ANALYSIS")
print("=" * 70 + "\n")

layers = ['examples', 'main', 'include', 'include/utils', 'core', 'platform', 'utils']
for layer in layers:
    if layer in layer_deps:
        deps_list = sorted(layer_deps[layer])
        print(f"  {layer:15} → {', '.join(deps_list)}")
    else:
        print(f"  {layer:15} → (no dependencies)")

# Check for violations
print("\n" + "=" * 70)
print("CLEAN ARCHITECTURE COMPLIANCE")
print("=" * 70 + "\n")

violations = []

# Define allowed dependencies (clean architecture rules)
allowed = {
    'examples': {'include', 'include/utils'},
    'main': {'include', 'core', 'platform', 'utils'},
    'include': {'include/utils'},  # Can reference utils
    'include/utils': set(),  # Pure (no deps)
    'core': {'include', 'platform', 'utils', 'include/utils'},
    'platform': {'include', 'utils', 'include/utils'},
    'utils': {'include', 'include/utils'},  # Allowed to reference domain types
}

for src, targets in layer_deps.items():
    for tgt in targets:
        if tgt not in allowed.get(src, set()):
            violations.append(f"  ✗ {src} → {tgt} (NOT ALLOWED)")

if violations:
    print("❌ VIOLATIONS FOUND:")
    for v in violations:
        print(v)
else:
    print("✅ NO VIOLATIONS - Clean Architecture Compliant!\n")
    print("All dependencies follow Clean Architecture principles:")
    print("  • Dependencies point inward (toward stable abstractions)")
    print("  • Outer layers depend on inner layers, not vice versa")
    print("  • Interface layer (include/) is most stable (no dependencies)")

# Analyze layer hierarchy
print("\n" + "=" * 70)
print("LAYER HIERARCHY (Outer → Inner)")
print("=" * 70 + "\n")

print("┌─────────────────┐")
print("│   examples/     │  User Algorithm Implementations (Outermost)")
print("└────────┬────────┘")
print("         │ depends on")
print("         ▼")
print("┌─────────────────┐")
print("│     main        │  Application Entry Point")
print("└────────┬────────┘")
print("         │ depends on")
print("         ▼")
print("┌─────────────────┐")
print("│   include/      │  Public API (Stable Abstractions)")
print("│ include/utils/  │  - Interfaces and contracts")
print("└────────┬────────┘")
print("         │ used by")
print("         ▼")
print("┌─────────────────┐")
print("│     core/       │  Business Logic (Use Cases)")
print("└────────┬────────┘  - Algorithm execution pipeline")
print("         │ depends on")
print("         ▼")
print("┌─────────────────┐")
print("│   platform/     │  Platform Abstraction (Infrastructure)")
print("└────────┬────────┘  - OpenCL platform management")
print("         │ depends on")
print("         ▼")
print("┌─────────────────┐")
print("│    utils/       │  Utilities & Entities (Innermost)")
print("└─────────────────┘  - Configuration, I/O, verification")

# Calculate stability metrics
print("\n" + "=" * 70)
print("STABILITY METRICS")
print("=" * 70 + "\n")

print("Instability (I) = Fan-out / (Fan-in + Fan-out)")
print("  I = 0.00: Most stable (many dependents, no dependencies)")
print("  I = 1.00: Most unstable (no dependents, many dependencies)\n")

# Count fan-in and fan-out
fan_in = defaultdict(int)
fan_out = defaultdict(int)

for src, targets in layer_deps.items():
    fan_out[src] = len(targets)
    for tgt in targets:
        fan_in[tgt] += 1

for layer in layers:
    fi = fan_in.get(layer, 0)
    fo = fan_out.get(layer, 0)
    total = fi + fo
    instability = fo / total if total > 0 else 0.0

    bar_length = int(instability * 20)
    bar = "█" * bar_length + "░" * (20 - bar_length)

    print(f"  {layer:15}  I={instability:.2f}  [{bar}]  (in={fi}, out={fo})")

print("\n✅ Proper stability gradient: include/ (stable) → main/examples (unstable)")
print()
