/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set ts=8 sts=2 et sw=2 tw=80: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef Tokenizer_h__
#define Tokenizer_h__

#include "nsString.h"
#include "mozilla/CheckedInt.h"

namespace mozilla {

/**
 * This is a simple implementation of a lexical analyzer or maybe better
 * called a tokenizer.  It doesn't allow any user dictionaries or
 * user define token types.
 *
 * It is limited only to ASCII input for now. UTF-8 or any other input
 * encoding must yet be implemented.
 */
class Tokenizer {
public:
  /**
   * The analyzer works with elements in the input cut to a sequence of token
   * where each token has an elementary type
   */
  enum TokenType {
    TOKEN_UNKNOWN,
    TOKEN_ERROR,
    TOKEN_INTEGER,
    TOKEN_WORD,
    TOKEN_CHAR,
    TOKEN_WS,
    TOKEN_EOL,
    TOKEN_EOF
  };

  /**
   * Class holding the type and the value of a token.  It can be manually created
   * to allow checks against it via methods of Tokenizer or are results of some of
   * the Tokenizer's methods.
   */
  class Token {
    TokenType mType;
    nsDependentCSubstring mWord;
    char mChar;
    uint64_t mInteger;

    // If this token is a result of the parsing process, this member is referencing
    // a sub-string in the input buffer.  If this is externally created Token this
    // member is left an empty string.
    nsDependentCSubstring mFragment;

    friend class Tokenizer;
    void AssignFragment(nsACString::const_char_iterator begin,
                        nsACString::const_char_iterator end);

  public:
    Token() : mType(TOKEN_UNKNOWN), mChar(0), mInteger(0) {}
    Token(const Token& aOther);
    Token& operator=(const Token& aOther);

    // Static constructors of tokens by type and value
    static Token Word(const nsACString& aWord);
    static Token Char(const char aChar);
    static Token Number(const uint64_t aNumber);
    static Token Whitespace();
    static Token NewLine();
    static Token EndOfFile();
    static Token Error();

    // Compares the two tokens, type must be identical and value
    // of one of the tokens must be 'any' or equal.
    bool Equals(const Token& aOther) const;

    TokenType Type() const { return mType; }
    char AsChar() const;
    nsDependentCSubstring AsString() const;
    uint64_t AsInteger() const;

    nsDependentCSubstring Fragment() const { return mFragment; }
  };

public:
  /**
   * @param aSource
   *    The string to parse.
   *    IMPORTANT NOTE: Tokenizer doesn't ensure the input string buffer lifetime.
   *    It's up to the consumer to make sure the string's buffer outlives the Tokenizer!
   * @param aWhitespaces
   *    If non-null Tokenizer will use this custom set of whitespaces for CheckWhite()
   *    and SkipWhites() calls.
   *    By default the list consists of space and tab.
   * @param aAdditionalWordChars
   *    If non-null it will be added to the list of characters that consist a word.
   *    This is useful when you want to accept e.g. '-' in HTTP headers.
   *    By default a word character is consider any character for which upper case
   *    is different from lower case.
   *
   * If there is an overlap between aWhitespaces and aAdditionalWordChars, the check for
   * word characters is made first.
   */
  explicit Tokenizer(const nsACString& aSource,
                     const char* aWhitespaces = nullptr,
                     const char* aAdditionalWordChars = nullptr);
  explicit Tokenizer(const char* aSource,
                     const char* aWhitespaces = nullptr,
                     const char* aAdditionalWordChars = nullptr);

  /**
   * When there is still anything to read from the input, tokenize it, store the token type
   * and value to aToken result and shift the cursor past this just parsed token.  Each call
   * to Next() reads another token from the input and shifts the cursor.
   * Returns false if we have passed the end of the input.
   */
  MOZ_WARN_UNUSED_RESULT
  bool Next(Token& aToken);

  /**
   * Parse the token on the input read cursor position, check its type is equal to aTokenType
   * and if so, put it into aResult, shift the cursor and return true.  Otherwise, leave
   * the input read cursor position intact and return false.
   */
  MOZ_WARN_UNUSED_RESULT
  bool Check(const TokenType aTokenType, Token& aResult);
  /**
   * Same as above method, just compares both token type and token value passed in aToken.
   * When both the type and the value equals, shift the cursor and return true.  Otherwise
   * return false.
   */
  MOZ_WARN_UNUSED_RESULT
  bool Check(const Token& aToken);

  /**
   * Return false iff the last Check*() call has returned false or when we've read past
   * the end of the input string.
   */
  MOZ_WARN_UNUSED_RESULT
  bool HasFailed() const;

  /**
   * SkipWhites method (below) may also skip new line characters automatically.
   */
  enum WhiteSkipping {
    /**
     * SkipWhites will only skip what is defined as a white space (default).
     */
    DONT_INCLUDE_NEW_LINE = 0,
    /**
     * SkipWhites will skip definited white spaces as well as new lines
     * automatically.
     */
    INCLUDE_NEW_LINE = 1
  };

  /**
   * Skips any occurence of whitespaces specified in mWhitespaces member,
   * optionally skip also new lines.
   */
  void SkipWhites(WhiteSkipping aIncludeNewLines = DONT_INCLUDE_NEW_LINE);

  // These are mostly shortcuts for the Check() methods above.

  /**
   * Check whitespace character is present.
   */
  MOZ_WARN_UNUSED_RESULT
  bool CheckWhite() { return Check(Token::Whitespace()); }
  /**
   * Check there is a single character on the read cursor position.  If so, shift the read
   * cursor position and return true.  Otherwise false.
   */
  MOZ_WARN_UNUSED_RESULT
  bool CheckChar(const char aChar) { return Check(Token::Char(aChar)); }
  /**
   * This is a customizable version of CheckChar.  aClassifier is a function called with
   * value of the character on the current input read position.  If this user function
   * returns true, read cursor is shifted and true returned.  Otherwise false.
   * The user classifiction function is not called when we are at or past the end and
   * false is immediately returned.
   */
  MOZ_WARN_UNUSED_RESULT
  bool CheckChar(bool (*aClassifier)(const char aChar));
  /**
   * Check for a whole expected word.
   */
  MOZ_WARN_UNUSED_RESULT
  bool CheckWord(const nsACString& aWord) { return Check(Token::Word(aWord)); }
  /**
   * Shortcut for literal const word check with compile time length calculation.
   */
  template <uint32_t N>
  MOZ_WARN_UNUSED_RESULT
  bool CheckWord(const char (&aWord)[N]) { return Check(Token::Word(nsDependentCString(aWord, N - 1))); }
  /**
   * Checks \r, \n or \r\n.
   */
  MOZ_WARN_UNUSED_RESULT
  bool CheckEOL() { return Check(Token::NewLine()); }
  /**
   * Checks we are at the end of the input string reading.  If so, shift past the end
   * and returns true.  Otherwise does nothing and returns false.
   */
  MOZ_WARN_UNUSED_RESULT
  bool CheckEOF() { return Check(Token::EndOfFile()); }

  /**
   * These are shortcuts to obtain the value immediately when the token type matches.
   */
  bool ReadChar(char* aValue);
  bool ReadChar(bool (*aClassifier)(const char aChar), char* aValue);
  bool ReadWord(nsACString& aValue);
  bool ReadWord(nsDependentCSubstring& aValue);

  /**
   * This is an integer read helper.  It returns false and doesn't move the read
   * cursor when any of the following happens:
   *  - the token at the read cursor is not an integer
   *  - the final number doesn't fit the T type
   * Otherwise true is returned, aValue is filled with the integral number
   * and the cursor is moved forward.
   */
  template <typename T>
  bool ReadInteger(T* aValue)
  {
    MOZ_RELEASE_ASSERT(aValue);

    nsACString::const_char_iterator rollback = mRollback;
    nsACString::const_char_iterator cursor = mCursor;
    Token t;
    if (!Check(TOKEN_INTEGER, t)) {
      return false;
    }

    mozilla::CheckedInt<T> checked(t.AsInteger());
    if (!checked.isValid()) {
      // Move to a state as if Check() call has failed
      mRollback = rollback;
      mCursor = cursor;
      mHasFailed = true;
      return false;
    }

    *aValue = checked.value();
    return true;
  }

  /**
   * Returns the read cursor position back as it was before the last call of any parsing
   * method of Tokenizer (Next, Check*, Skip*) so that the last operation can be repeated.
   * Rollback cannot be used multiple times, it only reverts the last successfull parse
   * operation.  It also cannot be used before any parsing operation has been called
   * on the Tokenizer.
   */
  void Rollback();

  /**
   * Record() and Claim() are collecting the input as it is being parsed to obtain
   * a substring between particular syntax bounderies defined by any recursive
   * descent parser or simple parser the Tokenizer is used to read the input for.
   * Inlucsion of a token that has just been parsed can be controlled using an arguemnt.
   */
  enum ClaimInclusion {
    /**
     * Include resulting (or passed) token of the last lexical analyzer operation in the result.
     */
    INCLUDE_LAST,
    /**
     * Do not include it.
     */
    EXCLUDE_LAST
  };

  /**
   * Start the process of recording.  Based on aInclude value the begining of the recorded
   * sub-string is at the current position (EXCLUDE_LAST) or at the position before the last
   * parsed token (INCLUDE_LAST).
   */
  void Record(ClaimInclusion aInclude = EXCLUDE_LAST);
  /**
   * Claim result of the record started with Record() call before.  Depending on aInclude
   * the ending of the sub-string result includes or excludes the last parsed or checked
   * token.
   */
  void Claim(nsACString& aResult, ClaimInclusion aInclude = EXCLUDE_LAST);
  void Claim(nsDependentCSubstring& aResult, ClaimInclusion aInclude = EXCLUDE_LAST);

protected:
  // false if we have already read the EOF token.
  bool HasInput() const;
  // Main parsing function, it doesn't shift the read cursor, just returns the next
  // token position.
  nsACString::const_char_iterator Parse(Token& aToken) const;
  // Is read cursor at the end?
  bool IsEnd(const nsACString::const_char_iterator& caret) const;
  // Is read cursor on a character that is a word start?
  bool IsWordFirst(const char aInput) const;
  // Is read cursor on a character that is an in-word letter?
  bool IsWord(const char aInput) const;
  // Is read cursor on a character that is a valid number?
  // TODO - support multiple radix
  bool IsNumber(const char aInput) const;

  // true iff we have already read the EOF token
  bool mPastEof;
  // true iff the last Check*() call has returned false, reverts to true on Rollback() call
  bool mHasFailed;

  // Customizable list of whitespaces
  const char* mWhitespaces;
  // Additinal custom word characters
  const char* mAdditionalWordChars;

  // All these point to the original buffer passed to the Tokenizer
  nsACString::const_char_iterator mRecord; // Position where the recorded sub-string for Claim() is
  nsACString::const_char_iterator mRollback; // Position of the previous token start
  nsACString::const_char_iterator mCursor; // Position of the current (actually next to read) token start
  nsACString::const_char_iterator mEnd; // End of the input position

private:
  Tokenizer() = delete;
  Tokenizer(const Tokenizer&) = delete;
  Tokenizer(Tokenizer&&) = delete;
  Tokenizer(const Tokenizer&&) = delete;
  Tokenizer &operator=(const Tokenizer&) = delete;
};

} // mozilla

#endif // Tokenizer_h__
