// Copyright 2006 Tobias Sargeant (toby@permuted.net).
// All rights reserved.

#pragma once

#include <string>
#include <sstream>
namespace gloop {
  namespace str {

    class format {
      mutable std::ostringstream accum;

    public:
      template<typename T>
      format &operator<<(const T &t) {
        accum << t;
        return *this;
      }
      operator std::string() const { return accum.str(); }
    };


    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline bool startswith(
        const std::basic_string<_CharT, _Traits, _Alloc> &a,
        const std::basic_string<_CharT, _Traits, _Alloc> &b) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      if (b.size() > a.size()) return false;
      return a.compare(0, b.size(), b) == 0;
    }

    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline bool startswith(
        const std::basic_string<_CharT, _Traits, _Alloc> &a,
        const _CharT *b) {
      return startswith(a, std::basic_string<_CharT, _Traits, _Alloc>(b));
    }

    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline bool endswith(
        const std::basic_string<_CharT, _Traits, _Alloc> &a,
        const std::basic_string<_CharT, _Traits, _Alloc> &b) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      if (b.size() > a.size()) return false;
      return a.compare(a.size() - b.size(), b.size(), b) == 0;
    }

    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline bool endswith(
        const std::basic_string<_CharT, _Traits, _Alloc> &a,
        const _CharT *b) {
      return endswith(a, std::basic_string<_CharT, _Traits, _Alloc>(b));
    }

    template<typename _CharT, typename _Traits, typename _Alloc, typename strip_t>
    static inline std::basic_string<_CharT, _Traits, _Alloc> rstrip(
        const std::basic_string<_CharT, _Traits, _Alloc> &a,
        const strip_t stripchars) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      typename str_t::size_type p = a.find_last_not_of(stripchars);
      if (p == str_t::npos) return "";
      return a.substr(0, p + 1);
    }

    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline std::basic_string<_CharT, _Traits, _Alloc> rstrip(
        const std::basic_string<_CharT, _Traits, _Alloc> &a) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      typename str_t::size_type p = a.size();
      while (p && isspace(a[p-1])) p--;
      if (!p) return "";
      return a.substr(0, p);
    }

    template<typename _CharT, typename _Traits, typename _Alloc, typename strip_t>
    static inline std::basic_string<_CharT, _Traits, _Alloc> lstrip(
        const std::basic_string<_CharT, _Traits, _Alloc> &a,
        const strip_t stripchars) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      typename str_t::size_type p = a.find_first_not_of(stripchars);
      if (p == str_t::npos) return "";
      return a.substr(p);
    }

    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline std::basic_string<_CharT, _Traits, _Alloc> lstrip(
        const std::basic_string<_CharT, _Traits, _Alloc> &a) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      typename str_t::size_type p = 0;
      while (p < a.size() && isspace(a[p])) p++;
      if (p == a.size()) return "";
      return a.substr(p);
    }

    template<typename _CharT, typename _Traits, typename _Alloc, typename strip_t>
    static inline std::basic_string<_CharT, _Traits, _Alloc> strip(
        const std::basic_string<_CharT, _Traits, _Alloc> &a,
        const strip_t stripchars) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      typename str_t::size_type p = a.find_first_not_of(stripchars);
      if (p == str_t::npos) return "";
      typename str_t::size_type q = a.find_last_not_of(stripchars);
      return a.substr(p, q-p);
    }

    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline std::basic_string<_CharT, _Traits, _Alloc> strip(
        const std::basic_string<_CharT, _Traits, _Alloc> &a) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;
      typename str_t::size_type p = 0;
      while (p < a.size() && isspace(a[p])) p++;
      if (p == a.size()) return "";
      typename str_t::size_type q = a.size();
      while (q>p && isspace(a[q-1])) q--;
      return a.substr(p, q-p);
    }
    
    template<typename _CharT, typename _Traits, typename _Alloc, typename _OutputIter>
    static inline void split(
        _OutputIter iter,
        const std::basic_string<_CharT, _Traits, _Alloc> &s,
        _CharT split,
        int nsplits = -1) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;

      int x = 0, rx = 0;
      while (x < (int)s.size()) {
        if (s[x] == (_CharT)split) {
          *iter++ = str_t(s, rx, x - rx);
          rx = x + 1;
          if (!--nsplits) {
            *iter++ = str_t(s, rx);
            return;
          }
        }
        x++;
      }
      if (rx < (int)s.size()) {
        *iter++ = str_t(s, rx);
      }
    }

    template<typename _CharT, typename _Traits, typename _Alloc, typename _OutputIter>
    static inline void split(
        _OutputIter iter,
        const std::basic_string<_CharT, _Traits, _Alloc> &s,
        int nsplits = -1) {
      typedef std::basic_string<_CharT, _Traits, _Alloc> str_t;

      int x = 0, rx = 0;

      int y = (int)s.size() - 1;
      while (isspace(s[x])) x++;
      rx = x;
      while (y >= 0 && isspace(s[y])) y--;
      y++;

      while (x < y) {
        if (isspace(s[x])) {
          *iter++ = str_t(s, rx, x - rx);
          while (x < y && isspace(s[x])) x++;
          rx = x;
          if (!--nsplits) {
            *iter++ = str_t(s, rx);
            return;
          }
        } else {
          x++;
        }
      }
      if (rx != y) {
        *iter++ = str_t(s, rx, y - rx);
      }
    }




    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline std::vector<std::basic_string<_CharT, _Traits, _Alloc> > split(
        const std::basic_string<_CharT, _Traits, _Alloc> &s,
        _CharT split,
        int nsplits = -1) {
      std::vector<std::basic_string<_CharT, _Traits, _Alloc> > r;
      if (nsplits != -1) r.reserve(nsplits + 1);
      split(std::back_inserter(r), s, split, nsplits);
      return r;
    }

    template<typename _CharT, typename _Traits, typename _Alloc>
    static inline std::vector<std::basic_string<_CharT, _Traits, _Alloc> > split(
        const std::basic_string<_CharT, _Traits, _Alloc> &s,
        int nsplits = -1) {
      std::vector<std::basic_string<_CharT, _Traits, _Alloc> > r;
      if (nsplits != -1) r.reserve(nsplits + 1);
      split(std::back_inserter(r), s, nsplits);
      return r;
    }

  }
}
