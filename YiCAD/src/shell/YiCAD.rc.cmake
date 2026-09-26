// Icon
//
// Icon with lowest ID value placed first to ensure application icon
// remains consistent on all systems.
IDI_ICON1               ICON    DISCARDABLE     "icon.ico"

// File info for the YiCAD.exe
//
1 VERSIONINFO 
FILEVERSION ${PACKAGE_VERSION_MAJOR},${PACKAGE_VERSION_MINOR},${YICAD_VERSION_PATCH},${PACKAGE_VERSION_PATCH}
BEGIN
    BLOCK "StringFileInfo"
    BEGIN
        BLOCK "080404b0" 
        BEGIN
            VALUE "CompanyName", "${PROJECT_NAME} Team"
            VALUE "FileDescription", "${PROJECT_NAME} main executable"
            VALUE "InternalName", "YiCAD.exe"
            VALUE "LegalCopyright", "Copyright (C) 2023"
            VALUE "OriginalFilename", "YiCAD.exe"
            VALUE "ProductName", "${PROJECT_NAME}"
            VALUE "ProductVersion", "${YICAD_VERSION}.${YICAD_VERSION_PATCH}${PACKAGE_VERSION_SUFFIX}"
        END
    END
    BLOCK "VarFileInfo"
    BEGIN
        VALUE "Translation", 0x804, 1200 //
    END
END
