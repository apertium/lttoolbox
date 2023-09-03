#include <iostream>
#include <lttoolbox/i18n.h>

int main(int argc, char* argv[])
{
    if (argc < 4 || argc % 2 != 0) {
        std::cout << "USAGE: formatmsg <data-path> <package-name> <key> <arg-names> <arg-values>\n";
        return 0;
    }

    I18n i18n {argv[1], argv[2]};
    
    std::vector<icu::UnicodeString> arg_names;
    std::vector<icu::Formattable> arg_values;
    if (argc > 4) {
        int arg_values_start = (argc - 4) / 2 + 4;

        for (int i = 4; i < arg_values_start; i++) {
            arg_names.push_back(argv[i]);
        }
        
        for (int i = arg_values_start; i < argc; i++) {
            arg_values.push_back(argv[i]);
        }
    }
    std::cout << i18n.format(argv[3], arg_names, arg_values) << std::endl;
    return 0;
}
