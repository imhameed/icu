#include <stdio.h>

#include <unicode/uchar.h>
#include <unicode/ustring.h>
#include <unicode/ucol.h>
#include <unicode/ucoleitr.h>
#include <unicode/usearch.h>

#include <unicode/ulocdata.h>
#include <unicode/udata.h>

#include <stdlib.h> //malloc

extern "C" {
    extern char const icudt67_dat = 0;
}

#if 0
int
main(int, char **) {
    UChar f;
    #if 0
        auto const F = static_cast<UChar32>('I');
        f = u_tolower(F);
    #else
        auto const F = static_cast<UChar>('I');
        UErrorCode err;
        u_strToLower(&f, 1, &F, 1, "tr-TR", &err);
    #endif
    auto const fc = static_cast<char>(f);
    printf("hello! %c\n", fc);
    return 0;
}
#else

#if 1
int32_t UErrorCodeToBool(UErrorCode status)
{
    if (U_SUCCESS(status))
    {
        return true;
    }

    // assert errors that should never occur
    assert(status != U_BUFFER_OVERFLOW_ERROR);
    assert(status != U_INTERNAL_PROGRAM_ERROR);

    // add possible SetLastError support here

    return false;
}

int32_t GetLocale(const char* localeName,
                  char* localeNameResult,
                  int32_t localeNameResultLength,
                  UBool canonicalize,
                  UErrorCode* err)
{
    char localeNameTemp[ULOC_FULLNAME_CAPACITY] = {0};
    int32_t localeLength;

    if (U_FAILURE(*err))
    {
        return 0;
    }

    // Convert ourselves instead of doing u_UCharsToChars as that function considers '@' a variant and stops.
    for (int i = 0; i < ULOC_FULLNAME_CAPACITY - 1; i++)
    {
        UChar c = localeName[i];

        // Some versions of ICU have a bug where '/' in name can cause infinite loop, so we preemptively
        // detect this case for CultureNotFoundException (as '/' is anyway illegal in locale name and we
        // expected ICU to return this error).

        if (c > (UChar)0x7F || c == (UChar)'/')
        {
            *err = U_ILLEGAL_ARGUMENT_ERROR;
            return ULOC_FULLNAME_CAPACITY;
        }

        localeNameTemp[i] = (char)c;

        if (c == (UChar)0x0)
        {
            break;
        }
    }

    if (canonicalize)
    {
        localeLength = uloc_canonicalize(localeNameTemp, localeNameResult, localeNameResultLength, err);
    }
    else
    {
        localeLength = uloc_getName(localeNameTemp, localeNameResult, localeNameResultLength, err);
    }

    if (U_SUCCESS(*err))
    {
        // Make sure the "language" part of the locale is reasonable (i.e. we can fetch it and it is within range).
        // This mimics how the C++ ICU API determines if a locale is "bogus" or not.

        char language[ULOC_LANG_CAPACITY];
        uloc_getLanguage(localeNameTemp, language, ULOC_LANG_CAPACITY, err);

        if (*err == U_BUFFER_OVERFLOW_ERROR || *err == U_STRING_NOT_TERMINATED_WARNING)
        {
            // ULOC_LANG_CAPACITY includes the null terminator, so if we couldn't extract the language with the null
            // terminator, the language must be invalid.

            *err = U_ILLEGAL_ARGUMENT_ERROR;
        }
    }

    return localeLength;
}

void u_charsToUChars_safe(const char* str, UChar* value, int32_t valueLength, UErrorCode* err)
{
    if (U_FAILURE(*err))
    {
        return;
    }

    size_t len = strlen(str);
    if (len >= (size_t)valueLength)
    {
        *err = U_BUFFER_OVERFLOW_ERROR;
        return;
    }

    u_charsToUChars(str, value, (int32_t)(len + 1));
}

int32_t FixupLocaleName(UChar* value, int32_t valueLength)
{
    int32_t i = 0;
    for (; i < valueLength; i++)
    {
        if (value[i] == (UChar)'\0')
        {
            break;
        }
        else if (value[i] == (UChar)'_')
        {
            value[i] = (UChar)'-';
        }
    }

    return i;
}

int32_t GlobalizationNative_GetLocaleName(const char* localeName, UChar* value, int32_t valueLength)
{
    UErrorCode status = U_ZERO_ERROR;

    char localeNameBuffer[ULOC_FULLNAME_CAPACITY];
    GetLocale(localeName, localeNameBuffer, ULOC_FULLNAME_CAPACITY, true, &status);
    u_charsToUChars_safe(localeNameBuffer, value, valueLength, &status);

    if (U_SUCCESS(status))
    {
        FixupLocaleName(value, valueLength);
    }

    return UErrorCodeToBool(status);
}
#endif

#if 1
template <typename... ts>
static void
log_icu_error(ts... args) {
    printf(args...);
}

template <typename... ts>
static void
log_shim_error(ts... args) {
    printf(args...);
}

static int32_t load_icu_data(void* pData)
{

    UErrorCode status = (UErrorCode) 0;
    udata_setCommonData(pData, &status);

    if (U_FAILURE(status)) {
        log_icu_error("udata_setCommonData", status);
        return 0;
    } else {

#if defined(ICU_TRACING)
        // see https://github.com/unicode-org/icu/blob/master/docs/userguide/icu_data/tracing.md
        utrace_setFunctions(0, 0, 0, icu_trace_data);
        utrace_setLevel(UTRACE_VERBOSE);
#endif
        return 1;
    }
}

int32_t GlobalizationNative_LoadICUData(const char* path)
{
    int32_t ret = -1;
    char* icu_data;

    FILE *fp = fopen(path, "rb");
    if (fp == NULL) {
        log_shim_error("Unable to load ICU dat file '%s'.", path);
        return ret;
    }

    if (fseek(fp, 0L, SEEK_END) != 0) {
        fclose(fp);
        log_shim_error("Unable to determine size of the dat file");
        return ret;
    }

    long bufsize = ftell(fp);

    if (bufsize == -1) {
        fclose(fp);
        log_shim_error("Unable to determine size of the ICU dat file.");
        return ret;
    }

    icu_data = (char *) malloc(sizeof(char) * (bufsize + 1));

    if (icu_data == NULL) {
        fclose(fp);
        log_shim_error("Unable to allocate enough to read the ICU dat file");
        return ret;
    }

    if (fseek(fp, 0L, SEEK_SET) != 0) {
        fclose(fp);
        log_shim_error("Unable to seek ICU dat file.");
        return ret;
    }

    fread(icu_data, sizeof(char), bufsize, fp);
    if (ferror( fp ) != 0 ) {
        fclose(fp);
        log_shim_error("Unable to read ICU dat file");
        return ret;
    }

    fclose(fp);

    if (load_icu_data(icu_data) == 0) {
        log_shim_error("ICU BAD EXIT %d.", ret);
        return ret;
    }

    return 1;
}
#endif

UChar ** volatile mystery_buffer;
UCollationResult volatile res;

void *ptrs[] = {
    (void *) &ucol_open,
    (void *) &ucol_strcoll,
    (void *) &ucol_getStrength,
    (void *) &ucol_safeClone,
    (void *) &ucol_getRules,
#if 1
    (void *) &ucol_openRules,
    (void *) &ucol_setAttribute,
    (void *) &ucol_setMaxVariable,
    (void *) &ucol_setVariableTop,
    (void *) &ucol_close,
    (void *) &ucol_getVersion,
    (void *) &ucol_getSortKey,
#endif

#if 1
    (void *) &ucol_openElements,
    (void *) &ucol_closeElements,
    (void *) &ucol_next,
    (void *) &ucol_previous,
#endif

#if 1
    (void *) &usearch_close,
    (void *) &usearch_first,
    (void *) &usearch_getBreakIterator,
    (void *) &usearch_getMatchedLength,
    (void *) &usearch_last,
    (void *) &usearch_openFromCollator,
    (void *) &usearch_setPattern,
    (void *) &usearch_setText,
#endif

#if 1
    (void *) &u_toupper,
    (void *) &u_tolower,
#endif
};

int
main(int, char **) {
    GlobalizationNative_LoadICUData("/Users/imhameed/ms/dotnet-icu/icu/eng/prebuilts/mobile/icudt.dat");

#if 0
    UErrorCode status = U_ZERO_ERROR;
    auto const coll = ucol_open("en_US", &status);
    auto const buf1 = *mystery_buffer;
    auto const buf2 = *mystery_buffer;
    auto const result = ucol_strcoll(coll, buf1, 2, buf2, 2);
    ucol_setMaxVariable(coll, UCOL_REORDER_CODE_CURRENCY, &status);
    for (size_t i = 0; i < (sizeof(ptrs) / sizeof(ptrs[0])); ++i) {
        printf("%p\n", ptrs[i]);
    }
#else
    auto const result = res;
#endif
    for (size_t i = 0; i < (sizeof(ptrs) / sizeof(ptrs[0])); ++i) {
        printf("%p\n", ptrs[i]);
    }
    auto const str
        = result == UCOL_EQUAL ? "equal"
        : result == UCOL_GREATER ? "greater"
        : "less";
    printf("%s\n", str);

    auto const count_available = uloc_countAvailable();
    printf("count_available = %d\n", (int) count_available);
    for (int i = 0; i < count_available; ++i) {
        auto const name = uloc_getAvailable(i);
        UChar fullname[157] = { };
        GlobalizationNative_GetLocaleName(name, fullname, 157);
        char fullname_ch[157] = { };
        for (int i = 0; i < 157; ++i) {
            fullname_ch[i] = fullname[i];
        }
        fullname_ch[156] = 0;
        printf("%s \t\t %s\n", name, fullname_ch);
    }


    return 0;
}
#endif
