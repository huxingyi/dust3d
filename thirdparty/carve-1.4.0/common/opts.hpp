// Begin License:
// Copyright (C) 2006-2008 Tobias Sargeant (tobias.sargeant@gmail.com).
// All rights reserved.
//
// This file is part of the Carve CSG Library (http://carve-csg.com/)
//
// This file may be used under the terms of the GNU General Public
// License version 2.0 as published by the Free Software Foundation
// and appearing in the file LICENSE.GPL2 included in the packaging of
// this file.
//
// This file is provided "AS IS" with NO WARRANTY OF ANY KIND,
// INCLUDING THE WARRANTIES OF DESIGN, MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE.
// End:


#pragma once

#include <string>
#include <list>
#include <vector>
#include <iomanip>

#include <iostream>

#include "stringfuncs.hpp"

namespace opt {
  struct Help {
    std::string long_opt;
    std::string short_opt;
    bool arg;
    std::string text;
    Help(const std::string &_long_opt, const std::string &_short_opt, bool _arg, const std::string &_text) :
        long_opt(_long_opt), short_opt(_short_opt), arg(_arg), text(_text) {
    }
  };



  struct Short {
    char ch;
    bool arg;
    std::string help;

    Short(char _ch, bool _arg, const std::string &_help) : ch(_ch), arg(_arg), help(_help) {}
  };



  struct Long {
    std::string str;
    bool arg;
    std::string help;

    Long(const std::string &_str, bool _arg, const std::string &_help) : str(_str), arg(_arg), help(_help) {}
  };



  struct exception {
  private:
    mutable std::string err;
    mutable std::ostringstream accum;

  public:
    exception(const std::string &e) : err(e), accum() { }
    exception() : err(), accum() { }
    exception(const exception &e) : err(e.str()), accum() { }

    const std::string &str() const {
      if (accum.tellp()) {
        err = accum.str();
        accum.str("");
      }
      return err;
    }

    template<typename T>
    exception &operator<<(const T &t) {
      accum << t;
      return *this;
    }
  };



  class Parser {
  protected:
    std::list<Short> short_opts;
    std::list<Long> long_opts;
    std::list<Help> help_text;

    std::string progname;

    virtual void help(std::ostream &out) {
      size_t max_long = 0;
      size_t max_short = 0;
      for (std::list<Help>::iterator i = help_text.begin(); i != help_text.end(); ++i) {
        max_long = std::max(max_long, (*i).long_opt.size());
        max_short = std::max(max_short, (*i).short_opt.size());
      }

      out << usageStr() << std::endl;
      out << std::setfill(' ');
      for (std::list<Help>::iterator i = help_text.begin(); i != help_text.end(); ++i) {
        out.setf(std::ios::left);
        out << std::setw(max_long + 1) << (*i).long_opt << std::setw(max_short + 1) << (*i).short_opt;
        if ((*i).arg) {
          out << "{arg} ";
        } else {
          out << "      ";
        }
        out << (*i).text << std::endl;
      }
    }

    virtual std::string usageStr() {
      return std::string ("Usage: ") + progname + std::string(" [options] [args]");
    };

    virtual void optval(const std::string &o, const std::string &v) {
    }

    virtual void arg(const std::string &a) {
    }

    void long_opt(std::vector<std::string>::const_iterator &i, const std::vector<std::string>::const_iterator &e) {
      const std::string &a(*i);
      std::string opt, val;
      bool has_argopt;

      std::string::size_type eq = a.find('=');
      has_argopt = eq != std::string::npos;
      if (has_argopt) {
        opt = a.substr(2, eq - 2);
        val = a.substr(eq + 1);
      } else {
        opt = a.substr(2);
        val = "";
      }
      for (std::list<Long>::iterator j = long_opts.begin(), je = long_opts.end(); j != je; ++j) {
        if ((*j).str == opt) {
          if (!(*j).arg && has_argopt) throw exception() << "unexpected argument for option --" << (*j).str << ".";
          if ((*j).arg) {
            if (++i == e) throw exception() << "missing argument for option --" << (*j).str << ".";
            val = *i;
          }
          optval("--" + opt, val);
          ++i;
          return;
        }
      }
      throw exception() << "unrecognised option --" << opt << ".";
    }

    void short_opt(std::vector<std::string>::const_iterator &i, const std::vector<std::string>::const_iterator &e) {
      const std::string &a(*i);
      
      for (std::string::size_type j = 1; j < a.size(); ++j) {
        for (std::list<Short>::iterator k = short_opts.begin(), ke = short_opts.end(); k != ke; ++k) {
          if ((*k).ch == a[j]) {
            if ((*k).arg) {
              if (j < a.size() - 1) {
                optval("-" + a.substr(j, 1), a.substr(j + 1));
                j = a.size() - 1;
              } else {
                if (++i == e) throw exception() << "missing argument for option -" << a[j] << ".";
                optval("-" + a.substr(j, 1), *i);
              }
            } else {
              optval("-" + a.substr(j, 1), "");
            }
            goto found;
          }
        }
        throw exception() << "unrecognised option -" << a[j] << ".";
      found:;
      }
      ++i;
    }

  public:
    Parser() {
    }

    virtual ~Parser() {
    }

    Parser &option(const std::string &str, char ch, bool arg, const std::string &help) {
      long_opts.push_back(Long(str, arg, help));
      short_opts.push_back(Short(ch, arg, help));
      help_text.push_back(Help("--" + str, std::string("-") + ch, arg, help));
      return *this;
    }

    Parser &option(const std::string &str, bool arg, const std::string &help) {
      long_opts.push_back(Long(str, arg, help));
      help_text.push_back(Help("--" + str, "", arg, help));
      return *this;
    }

    Parser &option(char ch, bool arg, const std::string &help) {
      short_opts.push_back(Short(ch, arg, help));
      help_text.push_back(Help("", std::string("-") + ch, arg, help));
      return *this;
    }

    bool parse(const std::string &pn, const std::vector<std::string> &opts) {
      try {
        progname = pn;
        std::vector<std::string>::const_iterator i = opts.begin();
        std::vector<std::string>::const_iterator e = opts.end();
        while (i != e) {
          const std::string &a(*i);
          if (a[0] == '-') {
            if (a == "-" || a == "--") { ++i; break; }
            if (a[1] == '-') {
              long_opt(i, e);
            } else {
              short_opt(i, e);
            }
          } else {
            break;
          }
        }
        while (i != e) { arg(*i++); }
        return true;
      } catch (exception e) {
        std::cerr << e.str() << std::endl;
        help(std::cerr);
        return false;
      }
    }

    bool parse(int argc, char **argv) {
      std::vector<std::string> opts(argc-1);
      for (int i = 1; i < argc; ++i) {
        opts[i-1] = argv[i];
      }
      return parse(argv[0], opts);
    }
  };

}
