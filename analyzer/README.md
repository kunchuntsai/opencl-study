# C/C++ Dependency Analyzer

Analyzes C/C++ header dependencies and Clean Architecture compliance.

## Requirements

- Python 3.6.3+
- No external dependencies (uses only standard library)

## Usage

```bash
python3 cdep_analyzer.py /path/to/project
```

## Workflow

1. **First run**: Auto-detects dependencies and generates `ca_layers.json` with suggested layer assignments
2. **Edit config**: Modify `ca_layers.json` to match your design concept
3. **Run again**: Checks for Clean Architecture violations based on your configuration

## Output Files (in analyzer directory)

- `ca_layers.json` - Layer configuration
- `dep_report.html` - Interactive visualization

## Offline Mode

Generated HTML reports require D3.js for visualization. By default, D3.js is loaded from CDN.

For offline environments, download D3.js and place it in the analyzer directory:

```bash
# On a machine with internet access:
curl -o d3.v7.min.js https://d3js.org/d3.v7.min.js
# Or: wget https://d3js.org/d3.v7.min.js

# Copy to analyzer directory
cp d3.v7.min.js /path/to/analyzer/
```

Once `d3.v7.min.js` is present, generated HTML reports will embed it inline and work offline.

## Clean Architecture Layers

Order: outermost to innermost

| Layer | Description |
|-------|-------------|
| Presentation | Entry points, UI, drivers |
| Application | Use cases, orchestration |
| Domain | Business logic, interfaces |
| Infrastructure | External services, utilities |

**Violation**: Inner layer depending on outer layer (e.g., Infrastructure -> Presentation)

## Configuration Format

```json
{
  "layers": [...],
  "directory_layers": {
    "Presentation": ["src"],
    "Application": ["src/core"],
    "Domain": ["include"],
    "Infrastructure": ["src/utils", "examples"]
  },
  "file_overrides": {}
}
```
