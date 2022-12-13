/*
 * Copyright (C) 2022 Apertium
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 */

#include <string>
#include <vector>
#include <map>
#include <iostream>

class CLI {
private:
  struct CLIOption {
    char short_opt;
    std::string long_opt;
    std::string desc;
    bool is_bool;
    std::string var;
  };

  std::string description;
  std::string version;
  std::string epilog;

  std::vector<CLIOption> options;
  std::vector<std::pair<std::string, bool>> file_args;
  size_t min_file_args = 0;

  std::map<std::string, std::vector<std::string>> strs;
  std::map<std::string, bool> bools;
  std::vector<std::string> files;

  std::string prog_name;

public:
  CLI(std::string desc, std::string version);
  CLI(std::string desc);
  ~CLI();
  void add_str_arg(char short_flag, std::string long_flag, std::string desc,
                   std::string arg);
  void add_bool_arg(char short_flag, std::string long_flag, std::string desc);
  void add_file_arg(std::string name, bool optional = true);
  void set_epilog(std::string e);
  void print_usage(std::ostream& out = std::cerr);
  void parse_args(int argc, char* argv[]);
  std::map<std::string, std::vector<std::string>>& get_strs();
  std::map<std::string, bool>& get_bools();
  std::vector<std::string>& get_files();
};
