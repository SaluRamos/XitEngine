#ifndef XITENGINE_ENUMS
#define XITENGINE_ENUMS

//ID, STR NAME
#define MEMORY_TYPE_LIST \
    X(MEM_INT, "int") \
    X(MEM_FLOAT, "float") \
    X(MEM_DOUBLE, "double") \
    X(MEM_LONG, "long") \
    X(MEM_BYTE, "byte")

enum MemoryType {
    #define X(id, str) id,
        MEMORY_TYPE_LIST
    #undef X
        MEM_COUNT
};

static const char* const* GetMemoryTypeNameList() {
    static const char* names[] = {
        #define X(id, str) str,
                MEMORY_TYPE_LIST
        #undef X
    };
    return names;
}

MemoryType ToMemoryType(int value) {
    if (value < 0 || value >= MEM_COUNT)
        return MEM_COUNT;
    return static_cast<MemoryType>(value);
}

//ID, STR NAME, ALLOWED_IN_FIRST_SCAN
#define SCAN_TYPE_LIST \
    X(EXACT_VALUE, "EXACT VALUE", true) \
    X(BIGGER_THAN, "BIGGER THAN", true) \
    X(SMALLER_THAN, "SMALLER THAN", true) \
    X(VALUE_BETWEEN, "VALUE BETWEEN", true) \
    X(INCREASED, "INCREASED", false) \
    X(DECREASED, "DECREASED", false) \
    X(CHANGED, "CHANGED", false) \
    X(UNCHANGED, "UNCHANGED", false)

enum ScanType {
    #define X(id, str, allowedInFirstScan) id,
        SCAN_TYPE_LIST
    #undef X
        SCAN_COUNT
};

static const char* const ScanTypeNames[] = {
    #define X(id, str, allowed) str,
        SCAN_TYPE_LIST
    #undef X
};

static const bool ScanTypeAllowedOnFirstScan[] = {
#define X(id, str, allowed) allowed,
    SCAN_TYPE_LIST
#undef X
};

static int allowedInFirstScanCount = 0;

static const char* const* GetScanTypeNameListInFirstScan() {
    static const char* names[SCAN_COUNT];
    static bool initialized = false;

    if (!initialized) {
        for (int i = 0; i < SCAN_COUNT; i++) {
            if (ScanTypeAllowedOnFirstScan[i]) {
                names[allowedInFirstScanCount++] = ScanTypeNames[i];
            }
        }
        initialized = true;
    }

    return names;
}

static const char* const* GetScanTypeNameListInNextScan() {
    return ScanTypeNames;
}

#endif