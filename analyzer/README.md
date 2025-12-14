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

## Offline Mode (for local networks)

By default, HTML reports load D3.js from CDN and require internet access.
To enable offline viewing:

### Option 1: Use the helper HTML (recommended)
1. Open `get_d3_offline.html` in a browser (on a machine with internet)
2. Click "Download d3.v7.min.js"
3. Copy the file to the `analyzer/` directory

### Option 2: Python script
```bash
python3 download_d3.py
```

### Option 3: Manual download
Download https://d3js.org/d3.v7.min.js and save as `analyzer/d3.v7.min.js`

After adding the file, generated HTML reports will embed D3.js inline and work offline.

## Clean Architecture Layers

Order: outermost to innermost

| Layer | Description |
|-------|-------------|
| Presentation | Entry points, UI, drivers |
| Application | Use cases, orchestration |
| Core | Business logic, interfaces |
| Infrastructure | External services, utilities |

**Violation**: Inner layer depending on outer layer (e.g., Infrastructure -> Presentation)

## Configuration Format

```json
{
  "layers": [...],
  "directory_layers": {
    "Presentation": ["src"],
    "Application": ["src/core"],
    "Core": ["include"],
    "Infrastructure": ["src/utils", "examples"]
  },
  "file_overrides": {}
}
```
