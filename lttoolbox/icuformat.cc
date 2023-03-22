#include <iostream>
#include <vector>
#include <unicode/ustream.h>
#include <unicode/resbund.h>
#include <unicode/locid.h>
#include <unicode/ustring.h>
#include <unicode/msgfmt.h>

#define LOCALES_DIR "./../share/lttoolbox/locales"

int main(int argc, char* argv[]) {

    if (argc == 1) {
        std::cout << "USAGE: icuformat <key> <args>\n";
        return 0;
    }

    UErrorCode status1 = U_ZERO_ERROR;
    UErrorCode status2 = U_ZERO_ERROR;

    icu::UnicodeString output;

    icu::Locale system_local {};

    icu::ResourceBundle resource_bundle(LOCALES_DIR, system_local.getName(), status1);
    if (!U_SUCCESS(status1)) {
        std::cout << "Error in accessing locales directory!" << std::endl;
        return 1;
    }

    icu::ResourceBundle pattern = resource_bundle.get(argv[1], status1);
    if (!U_SUCCESS(status1)) {
        std::cout << "Error: key not found!" << std::endl;
        return 1;
    }

    std::vector<icu::Formattable> arguments;
    for (int i = 2; i < argc; i++) {
        arguments.push_back(argv[i]);
    }

    icu::MessageFormat::format(pattern.getString(status1), arguments.data(), (argc - 2), output, status2);
    if(!U_SUCCESS(status1)) {
        std::cout << "Error in getting key text!" << std::endl;
        return 1;
    }
    if (!U_SUCCESS(status2)) {
        std::cout << "Error in formatting output!" << std::endl;
        return 1;
    }

    std::cout << output;
    return 0;
}
