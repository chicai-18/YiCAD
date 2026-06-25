# YiCAD Support Resources

## User-Provided Resource Files

YiCAD source code and official release packages do not include third-party SHX fonts, PAT hatch patterns, or LIN linetype definitions.

To use these resources, users may prepare files they have legally obtained and whose license permits such use, and place them in the following directories:

| File Type | Development Directory | Installed Runtime Directory |
|-----------|----------------------|-----------------------------|
| SHX Fonts | `YiCAD/support/fonts/` | `resources/fonts/` |
| PAT Hatch Patterns | `YiCAD/support/patterns/` | `resources/patterns/` |
| LIN Linetypes | `YiCAD/support/linetypes/` | `resources/linetypes/` |

YiCAD only provides the ability to read and use these file formats. It does not provide, host, license, or recommend any specific vendor's resource files.

Users are responsible for ensuring that their use, copying, or conversion of these files complies with the files' own licenses and applicable law. Do not submit resource files without clear redistribution permission to the YiCAD repository, Issues, Pull Requests, or official release packages.

Third-party products and trademarks are the property of their respective owners. YiCAD has no affiliation with, authorization from, or endorsement by any third-party vendor.

## Directory Contents

- `fonts/` - SHX font files (user-provided, not tracked in version control)
- `patterns/` - PAT hatch pattern files (user-provided, not tracked in version control)
- `linetypes/` - LIN linetype definition files (user-provided, not tracked in version control)
- `config/` - Application configuration files (project-owned)
- `ai/` - AI assistant documentation (project-owned)
