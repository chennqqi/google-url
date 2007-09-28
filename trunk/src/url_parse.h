// Copyright 2007, Google Inc.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef GOOGLEURL_SRC_URL_PARSE_H__
#define GOOGLEURL_SRC_URL_PARSE_H__

namespace url_parse {

// Component ------------------------------------------------------------------

// Represents a substring for URL parsing.
struct Component {
  Component() : begin(0), len(-1) {}

  // Normal constructor: takes an offset and a length.
  Component(int b, int l) : begin(b), len(l) {}

  int end() const {
    return begin + len;
  }

  // Returns true if this component is valid, meaning the length is given. Even
  // valid components may be empty to record the fact that they exist.
  bool is_valid() const {
    return (len != -1);
  }

  // Returns true if the given component is specified on false, the component
  // is either empty or invalid.
  bool is_nonempty() const {
    return (len > 0);
  }

  void reset() {
    begin = 0;
    len = -1;
  }

  bool operator==(const Component& other) const {
    return begin == other.begin && len == other.len;
  }

  int begin;  // Byte offset in the string of this component.
  int len;    // Will be -1 if the component is unspecified.
};

// Helper that returns a component created with the given begin and ending
// points. The ending point is non-inclusive.
inline Component MakeRange(int begin, int end) {
  return Component(begin, end - begin);
}

// Parsed ---------------------------------------------------------------------

// A structure that holds the identified parts of an input URL. This structure
// does NOT store the URL itself. The caller will have to store the URL text
// and its corresponding Parsed structure separately.
//
// Typical usage would be:
//
//    url_parse::Parsed parsed;
//    url_parse::Component scheme;
//    if (!url_parse::ExtractScheme(url, url_len, &scheme))
//      return I_CAN_NOT_FIND_THE_SCHEME_DUDE;
//
//    if (IsStandardScheme(url, scheme))  // Not provided by this component
//      url_parseParseStandardURL(url, url_len, &parsed);
//    else if (IsFileURL(url, scheme))    // Not provided by this component
//      url_parse::ParseFileURL(url, url_len, &parsed);
//    else
//      url_parse::ParsePathURL(url, url_len, &parsed);
//
struct Parsed {
  // Use a special initializer for the host since its length should be 0 when
  // not present. The default constructor is sufficient for the rest.
  Parsed() : host(0, 0) {
  }

  // Returns the length of the URL (the end of the last component).
  int Length() const;

  // Scheme without the colon: "http://foo"/ would have a scheme of "http".
  // The length will be -1 if no scheme is specified ("foo.com"), or 0 if there
  // is a colon but no scheme (":foo"). Note that the scheme is not guaranteed
  // to start at the beginning of the string if there are preceeding whitespace
  // or control characters.
  Component scheme;

  // Username. Specified in URLs with an @ sign before the host. See |password|
  Component username;

  // Password. The length will be -1 if unspecified, 0 if specified but empty.
  // Not all URLs with a username have a password, as in "http://me@host/".
  // The password is separated form the username with a colon, as in
  // "http://me:secret@host/"
  Component password;

  // Host name. The host name is never unspecified, and will have a length of
  // zero if empty.
  Component host;

  // Port number.
  Component port;

  // Path, this is everything following the host name. Length will be -1 if
  // unspecified. This includes the preceeding slash, so the path on
  // http://www.google.com/asdf" is "/asdf". As a result, it is impossible to
  // have a 0 length path, it will be -1 in cases like "http://host?foo".
  // Note that we treat backslashes the same as slashes.
  Component path;

  // Stuff between the ? and the # after the path. This does not include the
  // preceeding ? character. Length will be -1 if unspecified, 0 if there is
  // a question mark but no query string.
  Component query;

  // Indicated by a #, this is everything following the hash sign (not
  // including it). If there are multiple hash signs, we'll use the last one.
  // Length will be -1 if there is no hash sign, or 0 if there is one but
  // nothing follows it.
  Component ref;
};

// Initialization functions ---------------------------------------------------
//
// These functions parse the given URL, filling in all of the structure's
// components. These functions can not fail, they will always do their best
// at interpreting the input given.
//
// The string length of the URL MUST be specified, we do not check for NULLs
// at any point in the process, and will actually handle embedded NULLs.
//
// IMPORTANT: These functions do NOT hang on to the given pointer or copy it
// in any way. See the comment above the struct.
//
// The 8-bit versions require UTF-8 encoding.

// StandardURL is for when the scheme is known to be one that has an
// authority (host) like "http". This function will not handle weird ones
// like "about:" and "javascript:", or do the right thing for "file:" URLs.
void ParseStandardURL(const char* url, int url_len, Parsed* parsed);
void ParseStandardURL(const wchar_t* url, int url_len, Parsed* parsed);

// PathURL is for when the scheme is known not to have an authority (host)
// section but that aren't file URLs either. The scheme is parsed, and
// everything after the scheme is considered as the path. This is used for
// things like "about:" and "javascript:"
void ParsePathURL(const char* url, int url_len, Parsed* parsed);
void ParsePathURL(const wchar_t* url, int url_len, Parsed* parsed);

// FileURL is for file URLs. There are some special rules for interpreting
// these.
void ParseFileURL(const char* url, int url_len, Parsed* parsed);
void ParseFileURL(const wchar_t* url, int url_len, Parsed* parsed);

// Helper functions -----------------------------------------------------------

// Locates the scheme according to the URL  parser's rules. This function is
// designed so the caller can find the scheme and call the correct Init*
// function according to their known scheme types.
//
// It also does not perform any validation on the scheme.
//
// This function will return true if the scheme is found and will put the
// scheme's range into *scheme. False means no scheme could be found. Note
// that a URL beginning with a colon has a scheme, but it is empty, so this
// function will return true but *scheme will = (0,0).
//
// The scheme is found by skipping spaces and control characters at the
// beginning, and taking everything from there to the first colon to be the
// scheme. The character at scheme.end() will be the colon (we may enhance
// this to handle full width colons or something, so don't count on the
// actual character value). The character at scheme.end()+1 will be the
// beginning of the rest of the URL, be it the authority or the path (or the
// end of the string).
//
// The 8-bit version requires UTF-8 encoding.
bool ExtractScheme(const char* url, int url_len, Component* scheme);
bool ExtractScheme(const wchar_t* url, int url_len, Component* scheme);

// Returns true if ch is a character that terminates the authority segment
// of a URL.
bool IsAuthorityTerminator(wchar_t ch);

// Computes the integer port value from the given port component. The port
// component should have been identified by one of the init functions on
// |Parsed| for the given input url.
//
// The return value will be a positive integer between 0 and 64K, or one of
// the two special values below.
enum SpecialPort { PORT_UNSPECIFIED = -1, PORT_INVALID = -2 };
int ParsePort(const char* url, const Component& port);
int ParsePort(const wchar_t* url, const Component& port);

// Extracts the range of the file name in the given url. The path must
// already have been computed by the parse function, and the matching URL
// and extracted path are provided to this function. The filename is
// defined as being everything from the last slash/backslash of the path
// to the end of the path.
//
// The file name will be empty if the path is empty or there is nothing
// following the last slash.
//
// The 8-bit version requires UTF-8 encoding.
void ExtractFileName(const char* url,
                     const Component& path,
                     Component* file_name);
void ExtractFileName(const wchar_t* url,
                     const Component& path,
                     Component* file_name);

// Extract the next key / value from the range defined by query.
// Updates query to start at the end of the extracted key / value
// pair. If no key or / and no value are found query and key are
// unchanged.
void ExtractQueryFragment(const char* url,
                          Component* query,
                          Component* key,
                          Component* value);
void ExtractQueryFragment(const wchar_t* url,
                          Component* query,
                          Component* key,
                          Component* value);

}  // namespace url_parse

#endif  // GOOGLEURL_SRC_URL_PARSE_H__
