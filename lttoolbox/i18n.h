#ifndef _I18N_
#define _I18N_

#include <vector>
#include <memory>
#include <unordered_map>
#include <filesystem>
#include <fstream>
#include <iostream>

#include <unicode/ustream.h>
#include <unicode/resbund.h>
#include <unicode/ustring.h>
#include <unicode/msgfmt.h>
#include <unicode/locid.h>
#include <unicode/udata.h>

class I18n
{
private:
    static std::unordered_map<std::string, std::unique_ptr<char[]>> locales_data;
    static icu::Locale locale;
    icu::ResourceBundle resource;
    UErrorCode status;
public:
    I18n(const char *locales_path, std::string package_name);
    icu::UnicodeString format(const char* key, const std::vector<icu::UnicodeString> arg_names,
                                               const std::vector<icu::Formattable> arg_values);
    icu::UnicodeString format(const char* key);
    void error(const char* key, const std::vector<icu::UnicodeString> arg_names,
                                const std::vector<icu::Formattable> arg_values, bool quit);
    void error(const char* key, bool quit);
};

// Store .dat files and map it to package name of it.
inline std::unordered_map<std::string, std::unique_ptr<char[]>> I18n::locales_data;

// Store default language of the system before
// being overriden by LtLocale::tryToSetLocale() function.
inline icu::Locale I18n::locale = icu::Locale().getName();

inline I18n::I18n(const char *locales_path, std::string package_name) : resource("", "", status)
{
    // Initialize status.
    status = U_ZERO_ERROR;

    // Check if .dat file that is linked to this package_name has already been loaded or not.
    if (locales_data.find(package_name) == locales_data.end()) {
        // Open .dat file and loads it into pointer that is mapped in hashtable to string that represents
        // package name for easy looking later in case of being used again to avoid reloading it.

        std::ifstream file;
        file.open(locales_path);

        if (!file.is_open()) {
            std::cerr << "Error in opening data file!" << std::endl;
            std::cerr << "File: " << locales_path << std::endl;
            std::cerr << "Package Name: " << package_name << std::endl;
            exit(EXIT_FAILURE);
        }

        std::streamsize file_size = std::filesystem::file_size(std::filesystem::path{locales_path});

        locales_data[package_name] = std::make_unique<char[]>(file_size);

        file.read(locales_data[package_name].get(), file_size);

        // Link pointer address that points to .dat file
        // in memory to package_name that will be used later in Resource Bundles.
        udata_setAppData(package_name.c_str(), locales_data[package_name].get(), &status);

        if (!U_SUCCESS(status)) {
            std::cerr << "Error in loading data!" << std::endl;
            std::cerr << "Package Name: " << package_name << std::endl;
            std::cerr << u_errorName(status) << std::endl;
            exit(EXIT_FAILURE);
        }
    }
    // Initialize ResourceBundle with package_name that is linked by udata_setAppData function to .dat file
    // that contains i18n messages and default locale of the system for use in internationaliztion later
    // through format and error functions.
    resource = icu::ResourceBundle(package_name.c_str(), locale, status);

    if (!U_SUCCESS(status)) {
        std::cerr << "Error in initializing resource bundle" << std::endl;
        std::cerr << "Package Name: " << package_name << std::endl;
        std::cerr << u_errorName(status) << std::endl;
        exit(EXIT_FAILURE);
    }
}

inline icu::UnicodeString I18n::format(const char* key, const std::vector<icu::UnicodeString> arg_names,
                                                 const std::vector<icu::Formattable> arg_values)
{
    icu::UnicodeString pattern;
    icu::UnicodeString output;
    
    icu::ResourceBundle resource_object = resource.get(key, status);
    if (!U_SUCCESS(status)) {
        std::cerr << "Error: key not found!" << std::endl;
        std::cerr << "Key: " << key << std::endl;
        std::cerr << u_errorName(status) << std::endl;
        exit(EXIT_FAILURE);
    }

    pattern = resource_object.getString(status);
    if (!U_SUCCESS(status)) {
        std::cerr << "Error in getting key text!" << std::endl;
        std::cerr << "Key: " << key << std::endl;
        std::cerr << u_errorName(status) << std::endl;
        exit(EXIT_FAILURE);
    }

    icu::MessageFormat formatter {pattern, status};
    if (!U_SUCCESS(status)) {
        std::cerr << "Error in initializing MessageFormat class!" << std::endl;
        std::cerr << "Key: " << key << std::endl;
        std::cerr << "Pattern: " << pattern << std::endl;
        std::cerr << u_errorName(status) << std::endl;
        exit(EXIT_FAILURE);
    }

    formatter.format(arg_names.data(), arg_values.data(), arg_values.size(), output, status);
    
    if (!U_SUCCESS(status)) {
        std::cerr << "Error in formatting!" << std::endl;
        std::cerr << "Key: " << key << std::endl;

        std::cerr << "Argument names: ";
        for (int i = 0; i < arg_names.size(); i++)
            std::cerr << arg_names[i] << std::endl;
        
        std::cerr << "Argument values: ";
        for (int i = 0; i < arg_values.size(); i++)
            std::cerr << arg_values[i].getString() << std::endl;
        
        std::cerr << u_errorName(status) << std::endl;
        exit(EXIT_FAILURE);
    }

    return output;
}

inline icu::UnicodeString I18n::format(const char* key)
{
    return format(key, {}, {});
}

inline void I18n::error(const char* key, const std::vector<icu::UnicodeString> arg_names,
                                  const std::vector<icu::Formattable> arg_values, bool quit)
{
    std::cerr << format(key, arg_names, arg_values) << std::endl;
    if (quit) {
        exit(EXIT_FAILURE);
    }
}
inline void I18n::error(const char* key, bool quit)
{
    error(key, {}, {}, quit);
}

#endif
