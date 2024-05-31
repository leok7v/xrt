#include "ut/ut.h"
#include "ut/ut_win32.h"

// Simplistic Win32 implementation of national language support.
// Windows NLS family of functions is very complicated and has
// difficult history of LANGID vs LCID etc... See:
// ResolveLocaleName()
// GetThreadLocale()
// SetThreadLocale()
// GetUserDefaultLocaleName()
// WM_SETTINGCHANGE lParam="intl"
// and many others...

enum {
    ut_nls_str_count_max = 1024,
    ut_nls_str_mem_max = 64 * ut_nls_str_count_max
};

static char  ut_nls_strings_memory[ut_nls_str_mem_max]; // increase if overflows
static char* ut_nls_strings_free = ut_nls_strings_memory;

static int32_t ut_nls_strings_count;

static const char* ut_nls_ls[ut_nls_str_count_max]; // localized strings
static const char* ut_nls_ns[ut_nls_str_count_max]; // neutral language strings

static uint16_t* ut_nls_load_string(int32_t strid, LANGID langid) {
    assert(0 <= strid && strid < countof(ut_nls_ns));
    uint16_t* r = null;
    int32_t block = strid / 16 + 1;
    int32_t index  = strid % 16;
    HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
        MAKEINTRESOURCE(block), langid);
//  traceln("FindResourceExA(block=%d langid=%04X)=%p", block, langid, res);
    uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
    uint16_t* ws = memory == null ? null : (uint16_t*)LockResource(memory);
//  traceln("LockResource(block=%d langid=%04X)=%p", block, langid, ws);
    if (ws != null) {
        for (int32_t i = 0; i < 16 && r == null; i++) {
            if (ws[0] != 0) {
                int32_t count = (int32_t)ws[0];  // String size in characters.
                ws++;
                assert(ws[count - 1] == 0, "use rc.exe /n command line option");
                if (i == index) { // the string has been found
//                  traceln("%04X found %s", langid, utf16to8(ws));
                    r = ws;
                }
                ws += count;
            } else {
                ws++;
            }
        }
    }
    return r;
}

static const char* ut_nls_save_string(uint16_t* utf16) {
    const int32_t bytes = ut_str.utf8_bytes(utf16);
    swear(bytes > 1);
    char* s = ut_nls_strings_free;
    uintptr_t left = (uintptr_t)countof(ut_nls_strings_memory) -
        (uintptr_t)(ut_nls_strings_free - ut_nls_strings_memory);
    fatal_if_false(left >= (uintptr_t)bytes, "string_memory[] overflow");
    ut_str.utf16to8(s, (int32_t)left, utf16);
    assert((int32_t)strlen(s) == bytes - 1, "utf16to8() does not truncate");
    ut_nls_strings_free += bytes;
    return s;
}

static const char* ut_nls_localize_string(int32_t strid) {
    assert(0 < strid && strid < countof(ut_nls_ns));
    const char* r = null;
    if (0 < strid && strid < countof(ut_nls_ns)) {
        if (ut_nls_ls[strid] != null) {
            r = ut_nls_ls[strid];
        } else {
            LCID lcid = GetThreadLocale();
            LANGID langid = LANGIDFROMLCID(lcid);
            uint16_t* ws = ut_nls_load_string(strid, langid);
            if (ws == null) { // try default dialect:
                LANGID primary = PRIMARYLANGID(langid);
                langid = MAKELANGID(primary, SUBLANG_NEUTRAL);
                ws = ut_nls_load_string(strid, langid);
            }
            if (ws != null && ws[0] != 0x0000) {
                r = ut_nls_save_string(ws);
                ut_nls_ls[strid] = r;
            }
        }
    }
    return r;
}

static int32_t ut_nls_strid(const char* s) {
    int32_t strid = 0;
    for (int32_t i = 1; i < ut_nls_strings_count && strid == 0; i++) {
        if (ut_nls_ns[i] != null && strcmp(s, ut_nls_ns[i]) == 0) {
            strid = i;
            ut_nls_localize_string(strid); // to save it, ignore result
        }
    }
    return strid;
}

static const char* ut_nls_string(int32_t strid, const char* defau1t) {
    const char* r = ut_nls_localize_string(strid);
    return r == null ? defau1t : r;
}

static const char* ut_nls_str(const char* s) {
    int32_t id = ut_nls_strid(s);
    return id == 0 ? s : ut_nls_string(id, s);
}

static const char* ut_nls_locale(void) {
    uint16_t wln[LOCALE_NAME_MAX_LENGTH + 1];
    LCID lcid = GetThreadLocale();
    int32_t n = LCIDToLocaleName(lcid, wln, countof(wln),
        LOCALE_ALLOW_NEUTRAL_NAMES);
    static char ln[LOCALE_NAME_MAX_LENGTH * 4 + 1];
    ln[0] = 0;
    if (n == 0) {
        // TODO: log error
    } else {
        ut_str.utf16to8(ln, countof(ln), wln);
    }
    return ln;
}

static void ut_nls_set_locale(const char* locale) {
    uint16_t wln[LOCALE_NAME_MAX_LENGTH + 1];
    ut_str.utf8to16(wln, countof(wln), locale);
    uint16_t rln[LOCALE_NAME_MAX_LENGTH + 1];
    int32_t n = (int32_t)ResolveLocaleName(wln, rln, (DWORD)countof(rln));
    if (n == 0) {
        // TODO: log error
    } else {
        LCID lcid = LocaleNameToLCID(rln, LOCALE_ALLOW_NEUTRAL_NAMES);
        if (lcid == 0) {
            // TODO: log error
        } else {
            fatal_if_false(SetThreadLocale(lcid));
            memset((void*)ut_nls_ls, 0, sizeof(ut_nls_ls)); // start all over
        }
    }
}

static void ut_nls_init(void) {
    static_assert(countof(ut_nls_ns) % 16 == 0, "countof(ns) must be multiple of 16");
    LANGID langid = MAKELANGID(LANG_ENGLISH, SUBLANG_NEUTRAL);
    for (int32_t strid = 0; strid < countof(ut_nls_ns); strid += 16) {
        int32_t block = strid / 16 + 1;
        HRSRC res = FindResourceExA(((HMODULE)null), RT_STRING,
            MAKEINTRESOURCE(block), langid);
        uint8_t* memory = res == null ? null : (uint8_t*)LoadResource(null, res);
        uint16_t* ws = memory == null ? null : (uint16_t*)LockResource(memory);
        if (ws == null) { break; }
        for (int32_t i = 0; i < 16; i++) {
            int32_t ix = strid + i;
            uint16_t count = ws[0];
            if (count > 0) {
                ws++;
                fatal_if_false(ws[count - 1] == 0, "use rc.exe /n");
                ut_nls_ns[ix] = ut_nls_save_string(ws);
                ut_nls_strings_count = ix + 1;
//              traceln("ns[%d] := %d \"%s\"", ix, strlen(ns[ix]), ns[ix]);
                ws += count;
            } else {
                ws++;
            }
        }
    }
}

ut_nls_if ut_nls = {
    .init       = ut_nls_init,
    .strid      = ut_nls_strid,
    .str        = ut_nls_str,
    .string     = ut_nls_string,
    .locale     = ut_nls_locale,
    .set_locale = ut_nls_set_locale,
};
