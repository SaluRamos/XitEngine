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

constexpr uint8_t sizeOfMemType(MemoryType t) {
    switch (t) {
        case MEM_INT:    return sizeof(int32_t);
        case MEM_FLOAT:  return sizeof(float);
        case MEM_DOUBLE: return sizeof(double);
        case MEM_LONG:   return sizeof(int64_t);
        case MEM_BYTE:   return sizeof(uint8_t);
        default:         return 0;
    }
}

static const char* const* GetMemoryTypeNameList() {
    static const char* names[] = {
        #define X(id, str) str,
                MEMORY_TYPE_LIST
        #undef X
    };
    return names;
}

inline MemoryType ToMemoryType(int value) {
    if (value < 0 || value >= MEM_COUNT)
        return MEM_COUNT;
    return static_cast<MemoryType>(value);
}

//ID, STR NAME, ALLOWED_IN_FIRST_SCAN
#define SCAN_FILTER_TYPE_LIST \
    X(EXACT_VALUE, "EXACT VALUE", true) \
    X(BIGGER_THAN, "BIGGER THAN", true) \
    X(SMALLER_THAN, "SMALLER THAN", true) \
    X(VALUE_BETWEEN, "VALUE BETWEEN", true) \
    X(INCREASED, "INCREASED", false) \
    X(DECREASED, "DECREASED", false) \
    X(CHANGED, "CHANGED", false) \
    X(UNCHANGED, "UNCHANGED", false)

enum ScanFilterType {
    #define X(id, str, allowedInFirstScan) id,
        SCAN_FILTER_TYPE_LIST
    #undef X
        SCAN_FILTER_COUNT
};

inline ScanFilterType ToScanFilterType(int value) {
    if (value < 0 || value >= SCAN_FILTER_COUNT)
        return SCAN_FILTER_COUNT;
    return static_cast<ScanFilterType>(value);
}

static const char* const ScanFilterTypeNames[] = {
    #define X(id, str, allowed) str,
        SCAN_FILTER_TYPE_LIST
    #undef X
};

static const bool ScanFilterTypeAllowedOnFirstScan[] = {
#define X(id, str, allowed) allowed,
    SCAN_FILTER_TYPE_LIST
#undef X
};

static int allowedInFirstScanCount = 0;

static const char* const* GetScanFilterTypeNameListInFirstScan() {
    static const char* names[SCAN_FILTER_COUNT];
    static bool initialized = false;

    if (!initialized) {
        for (int i = 0; i < SCAN_FILTER_COUNT; i++) {
            if (ScanFilterTypeAllowedOnFirstScan[i]) {
                names[allowedInFirstScanCount++] = ScanFilterTypeNames[i];
            }
        }
        initialized = true;
    }

    return names;
}

static const char* const* GetScanFilterTypeNameListInNextScan() {
    return ScanFilterTypeNames;
}

#endif