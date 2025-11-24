# Documentation

This directory contains the Doxygen configuration for generating API documentation.

## Prerequisites

Install Doxygen and Graphviz (for dependency graphs):

**macOS:**
```bash
brew install doxygen graphviz
```

**Ubuntu/Debian:**
```bash
sudo apt-get install doxygen graphviz
```

**Windows:**
- Doxygen: Download from https://www.doxygen.nl/download.html
- Graphviz: Download from https://graphviz.org/download/

## Generating Documentation

From the project root directory, run:

```bash
doxygen docs/Doxyfile
```

This will generate HTML documentation in `docs/html/`.

## Viewing Documentation

Open the generated documentation:

```bash
open docs/html/index.html      # macOS
xdg-open docs/html/index.html  # Linux
start docs/html/index.html     # Windows
```

## Documentation Features

The generated documentation includes:

- **File Documentation**: All header files with descriptions
- **Function Documentation**: Complete API reference with parameters and return values
- **Structure Documentation**: All data structures with field descriptions
- **Source Browser**: Browse source code with cross-references
- **Dependency Graphs**: Visual diagrams showing file dependencies and relationships
  - Include dependency graphs (what files include this file)
  - Included-by graphs (what files are included by this file)
  - Call graphs (which functions call this function)
  - Caller graphs (which functions are called by this function)
  - Directory dependency graphs
  - Interactive SVG graphs (zoomable and clickable)
- **Class/Structure Graphs**: Visual representation of data structures
- **MISRA C Compliance**: Documentation includes MISRA C 2023 compliance notes

## Documentation Standards

This project follows Doxygen documentation standards:

- `@file` - File-level documentation
- `@brief` - Brief description
- `@param[in]` - Input parameter
- `@param[out]` - Output parameter
- `@param[in,out]` - Input/output parameter
- `@return` - Return value description
- `@note` - Important notes
- `/**< */` - Inline member documentation

## MISRA C 2023 Compliance

All headers include MISRA C 2023 compliance information:
- Rules addressed in each module
- Justification for design decisions
- Safety-critical coding practices

See `MISRA_C_2023_COMPLIANCE.md` for detailed compliance report.
