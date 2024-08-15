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

#include <lttoolbox/cli.h>

#include <cstdlib>
#include <iostream>
#include <libgen.h>
#include <string>
#include <getopt.h>

CLI::CLI(std::string desc, std::string ver)
{
  description = desc;
  version = ver;
}

CLI::CLI(std::string desc)
{
  description = desc;
}

CLI::~CLI()
{
}

void CLI::add_str_arg(char short_flag, std::string long_flag,
                         std::string desc, std::string arg)
{
  options.push_back({.short_opt=short_flag, .long_opt=long_flag,
                     .desc=desc, .is_bool=false, .var=arg});
}

void CLI::add_bool_arg(char short_flag, std::string long_flag,
                       std::string desc)
{
  options.push_back({.short_opt=short_flag, .long_opt=long_flag,
                     .desc=desc, .is_bool=true, .var=""});
}

void CLI::add_file_arg(std::string name, bool optional)
{
  file_args.push_back(std::make_pair(name, optional));
  if (!optional) min_file_args++;
}

void CLI::set_epilog(std::string e)
{
  epilog = e;
}

void CLI::print_usage(std::ostream& out)
{
  if (!prog_name.empty()) {
    out << prog_name;
    if (!version.empty()) {
      out << " v" << version;
    }
    out << ": " << description << std::endl;
    out << "USAGE: " << prog_name;
    std::string bargs;
    std::string sargs;
    for (auto& it : options) {
      if (it.is_bool) {
        bargs += it.short_opt;
      } else {
        sargs += " [-";
        sargs += it.short_opt;
        sargs += ' ';
        sargs += it.var;
        sargs += ']';
      }
    }
    if (!bargs.empty()) {
      out << " [-" << bargs << "]";
    }
    out << sargs;
    int depth = 0;
    for (auto& it : file_args) {
      out << ' ';
      if (it.second) {
        out << '[';
        depth += 1;
      }
      out << it.first;
    }
    while (depth-- > 0) out << "]";
    out << std::endl;
    for (auto& it : options) {
      out << "  -" << it.short_opt;
#if HAVE_GETOPT_LONG
      out << ", --" << it.long_opt << ':';
      for (size_t i = it.long_opt.size(); i < 20; i++) {
        out << ' ';
      }
#else
      out << ":    ";
#endif
      out << it.desc << std::endl;
    }
    if (!epilog.empty()) {
      out << epilog << std::endl;
    }
  }
  exit(EXIT_FAILURE);
}

void CLI::parse_args(int argc, char* argv[])
{
  prog_name = basename(argv[0]);
  std::string arg_str;
#if HAVE_GETOPT_LONG
  std::vector<struct option> long_options(options.size());
  int option_index = 0;
#endif
  for (size_t i = 0; i < options.size(); i++) {
    arg_str += options[i].short_opt;
    if (!options[i].is_bool) arg_str += ':';
#if HAVE_GETOPT_LONG
    long_options[i].name = options[i].long_opt.c_str();
    long_options[i].has_arg = (options[i].is_bool ? no_argument : required_argument);
    long_options[i].flag = 0;
    long_options[i].val = options[i].short_opt;
#endif
  }

  while (true) {
#if HAVE_GETOPT_LONG
    int cnt = getopt_long(argc, argv, arg_str.c_str(), long_options.data(), &option_index);
#else
    int cnt = getopt(argc, argv, arg_str.c_str());
#endif
    if (cnt == -1) break;

    bool found = false;
    for (auto& it : options) {
      if (it.short_opt == cnt) {
        found = true;
        if (it.short_opt == 'v' && it.long_opt == "version") {
          std::cout << prog_name << " version " << version << std::endl;
          exit(EXIT_SUCCESS);
        }
        if (it.is_bool) {
          bools[it.long_opt] = true;
        } else {
          strs[it.long_opt].push_back(optarg);
        }
        break;
      }
    }
    if (!found) {
      print_usage(std::cerr);
    }
    else if (cnt == 'h') {
      print_usage(std::cout);
    }
  }
  while (optind < argc) {
    files.push_back(argv[optind++]);
  }
  if (files.size() < min_file_args || files.size() > file_args.size()) {
    print_usage();
  }
  while (files.size() < file_args.size()) {
    files.push_back("");
  }
}

std::map<std::string, std::vector<std::string>>& CLI::get_strs()
{
  return strs;
}

std::map<std::string, bool>& CLI::get_bools()
{
  return bools;
}

std::vector<std::string>& CLI::get_files()
{
  return files;
}
