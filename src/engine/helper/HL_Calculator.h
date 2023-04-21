#ifndef __HL_CALCULATOR_H
#define __HL_CALCULATOR_H

#include <iostream>
#include <string>
#include <assert.h>

#include "helper/HL_VariantMap.h"
#include "helper/HL_String.h"
#include "helper/HL_InternetAddress.h"

namespace Calc
{
  class Parser;
  namespace Ast
  {
    enum class Type
    {
      None,
      And,
      Or,
      Equal,
      NotEqual,
      Less,
      LessOrEqual,
      Greater,
      GreatorOrEqual,
      Add,
      Sub,
      Mul,
      Div,
      Number,
      String,
      Var
    };

    class Item;
    typedef Item* PItem;

    class Item
    {
    friend class Calc::Parser;
    public:
      bool isVariable() const
      {
        return mType == Type::Var;
      }
      bool isFixed() const
      {
        return mType == Type::Number || mType == Type::String;
      }

      bool isOperation() const
      {
        return mType >= Type::And && mType <= Type::Div;
      }

      bool hasBrackets() const
      {
        return mHasBrackets;
      }

      int getOperatorLevel() const
      {
        switch (mType)
        {
        case Type::Or:
          return -2;

        case Type::And:
          return -1;

        case Type::Equal:
        case Type::NotEqual:
          return 0;

        case Type::Less:
        case Type::LessOrEqual:
        case Type::Greater:
        case Type::GreatorOrEqual:
          return 1;

        case Type::Add:
        case Type::Sub:
          return 2;

        case Type::Mul:
        case Type::Div:
          return 3;

        default:
          return 4;
        }
        assert(0);
      }

      Type getType() const
      {
        return mType;
      }

      std::string getName() const
      {
        return mName;
      }

      Variant& value()
      {
        return mValue;
      }

      std::vector<PItem>& children()
      {
        return mChildren;
      }

      typedef std::map<std::string, std::string> NameMap;

      std::ostream& print(std::ostream& oss, const NameMap& nm)
      {
        oss << " ( ";

        if (isOperation())
          mChildren.front()->print(oss, nm);

        oss << " ";
        switch (mType)
        {
        case Type::Number:          oss << mValue.asStdString(); break;
        case Type::String:          oss << '"' << mValue.asStdString() << '"'; break;
        case Type::Var:             { NameMap::const_iterator iter = nm.find(mName); oss << ((iter != nm.end()) ? iter->second : mName);} break;
        case Type::Add:             oss << "+"; break;
        case Type::Mul:             oss << "*"; break;
        case Type::Div:             oss << "/"; break;
        case Type::Sub:             oss << "-"; break;
        case Type::Equal:           oss << "=="; break;
        case Type::NotEqual:        oss << "!="; break;
        case Type::Less:            oss << "<"; break;
        case Type::LessOrEqual:     oss << "<="; break;
        case Type::Greater:         oss << ">"; break;
        case Type::GreatorOrEqual:  oss << ">="; break;
        case Type::Or:              oss << "or"; break;
        case Type::And:             oss << "and"; break;
        default:
          throw std::runtime_error("operator expected");
        }
        oss << " ";
        if (isOperation() && mChildren.size() == 2 && mChildren.back())
          mChildren.back()->print(oss, nm);

        oss << " ) ";

        return oss;
      }

      typedef std::map<std::string, Variant> ValueMap;

      Variant eval(const ValueMap& vm)
      {
        Variant result, left, right;
        if (isOperation())
        {
          left = mChildren.front()->eval(vm);
          right = mChildren.back()->eval(vm);
        }

        switch (mType)
        {
        case Type::Number:
        case Type::String:          result = mValue; break;

        case Type::Var:             { auto iter = vm.find(mName); if (iter != vm.end()) return iter->second; else throw std::runtime_error("Variable " + mName + " did not find."); }
                                    break;

        case Type::Add:             result = left + right; break;
        case Type::Mul:             result = left * right; break;
        case Type::Div:             result = left / right; break;
        case Type::Sub:             result = left - right; break;
        case Type::Equal:           result = left == right; break;
        case Type::NotEqual:        result = left != right; break;
        case Type::Less:            result = left < right; break;
        case Type::LessOrEqual:     result = left <= right; break;
        case Type::Greater:         result = left > right; break;
        case Type::GreatorOrEqual:  result = left >= right; break;
        case Type::Or:              result = left.asBool() || right.asBool(); break;
        case Type::And:             result = left.asBool() && right.asBool(); break;
        default:
          assert(0);
        }
        return result;
      }

      ~Item()
      {
        for (auto node: mChildren)
          delete node;
        mChildren.clear();
      }

    private:
      Type mType = Type::None;
      std::string mName;
      Variant mValue;
      std::vector<PItem> mChildren;
      bool mHasBrackets = false;
    };
  }

  static bool ishex(int c)
  {
    if (isdigit(c))
      return true;
    return (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
  }

  class Parser
  {
  private:
    enum class LexemType
    {
      None,
      Hex,
      Dec,
      Float,
      Str,
      Oper,
      Var,
      OpenBracket,
      CloseBracket
    };

    struct Lexem
    {
      LexemType mType = LexemType::None;
      std::string mValue;

      operator bool () const
      {
        return mType != LexemType::None;
      }

      std::string toString() const
      {
        return std::to_string((int)mType) + " : " + mValue;
      }
    };
    Lexem mCurrentLexem;

    Lexem processNewLexem(int c)
    {
      Lexem result;

      if (c == '(')
        mCurrentLexem.mType = LexemType::OpenBracket;
      else
      if (c == ')')
        mCurrentLexem.mType = LexemType::CloseBracket;
      else
      if (isdigit(c))
        mCurrentLexem.mType = LexemType::Dec;
      else
      if (isalpha(c))
        mCurrentLexem.mType = LexemType::Var;
      else
      if (c == '+' || c == '-' || c == '/' || c == '*' || c == '=' || c == '<' || c == '>' || c == '&' || c == '|')
        mCurrentLexem.mType = LexemType::Oper;
      else
      if (c == '"')
        mCurrentLexem.mType = LexemType::Str;
      else
        return Lexem();

      mCurrentLexem.mValue.push_back(c);

      // Can we return result here already ?
      if (mCurrentLexem.mType == LexemType::OpenBracket || mCurrentLexem.mType == LexemType::CloseBracket)
      {
        // Lexem finished
        result = mCurrentLexem;
        mCurrentLexem = Lexem();
        return result;
      }

      if (mCurrentLexem.mType == LexemType::Oper)
      {
        if (mCurrentLexem.mValue == "+" ||
            mCurrentLexem.mValue == "-" ||
            mCurrentLexem.mValue == "*" ||
            mCurrentLexem.mValue == "/" ||
            mCurrentLexem.mValue == ">=" ||
            mCurrentLexem.mValue == "<=" ||
            mCurrentLexem.mValue == "==" ||
            mCurrentLexem.mValue == "||" ||
            mCurrentLexem.mValue == "&&")
        {
          // Lexem finished
          result = mCurrentLexem;
          mCurrentLexem = Lexem();
          return result;
        }
      }
      return Lexem();
    }

    void checkNumericLexem()
    {
      if (mCurrentLexem.mType != LexemType::Dec)
        return;

      // Check if there is ".:" characters
      if (mCurrentLexem.mValue.find('.') != std::string::npos)
      {
        // Dot is here - is it float
        bool isFloat = false;
        strx::toFloat(mCurrentLexem.mValue, 0.0f, &isFloat);
        if (isFloat)
          mCurrentLexem.mType = LexemType::Float;
        else
        {
          // Maybe it is IP4/6 address ?
          InternetAddress addr(mCurrentLexem.mValue, 8000);
          if (!addr.isEmpty())
          {
            mCurrentLexem.mValue = "\"" + mCurrentLexem.mValue + "\"";
            mCurrentLexem.mType = LexemType::Str;
          }
        }
      }
    }

    Lexem getLexem(std::istream& input)
    {
      Lexem result;

      // Iterate while characters avaialbe from input stream & lexem is not finished
      bool putback = false;
      int c = input.get();
      while (!input.eof() && c && result.mType == LexemType::None)
      {
        switch (mCurrentLexem.mType)
        {
        case LexemType::None:
          result = processNewLexem(c);
          break;

        case LexemType::Hex:
          if (!ishex(c))
          {
            // Finish Hex lexem
            result = mCurrentLexem;
            putback = true;
          }
          else
            mCurrentLexem.mValue.push_back(c);
          break;

        case LexemType::Dec:
          if (c == 'x' && mCurrentLexem.mValue == "0")
            mCurrentLexem.mType = LexemType::Hex;
          else
          if (isdigit(c) || c == '.')
          {
            mCurrentLexem.mValue.push_back(c);
          }
          else
          {
            checkNumericLexem();
            result = mCurrentLexem;
            putback = true;
          }
          break;

        case LexemType::Oper:
          // It must be one of two-characters operations
          if (c == '<' || c == '>' || c == '=' || c == '&' || c == '|')
          {
            mCurrentLexem.mValue.push_back(c);
            result = mCurrentLexem;
            mCurrentLexem = Lexem();
          }
          else
          {
            result = mCurrentLexem;
            putback = true;
          }
          break;

        case LexemType::Var:
          if (isdigit(c) || isalpha(c) || c == '.' || c == '_')
          {
            mCurrentLexem.mValue.push_back(c);
          }
          else
          {
            result = mCurrentLexem;
            putback = true;
          }
          break;

        case LexemType::Str:
          mCurrentLexem.mValue.push_back(c);
          if (c == '"')
          {
            result = mCurrentLexem;
            // String lexem is finished
            mCurrentLexem.mType = LexemType::None;
            putback = false;
          }
          break;

        default:
          assert(0);
        }

        if (putback)
          input.putback(c);
        else
        if (!result)
          c = input.get();
      }

      checkNumericLexem();

      // Recover partially processed lexem - maybe we finish processing at all but there is dec / float / string / variable
      if (mCurrentLexem.mType != LexemType::None && result.mType == LexemType::None)
        result = mCurrentLexem;

      // Reset current lexem
      mCurrentLexem = Lexem();

      return result;
    }

    // Make AST node from lexem
    Ast::PItem makeAst(const Lexem& l)
    {
      Ast::PItem result(new Ast::Item());

      switch (l.mType)
      {
      case LexemType::Oper:
        if (l.mValue == "-")
          result->mType = Ast::Type::Sub;
        else
        if (l.mValue == "+")
          result->mType = Ast::Type::Add;
        else
        if (l.mValue == "*")
          result->mType = Ast::Type::Mul;
        else
        if (l.mValue == "/")
          result->mType = Ast::Type::Div;
        else
        if (l.mValue == "<")
          result->mType = Ast::Type::Less;
        else
        if (l.mValue == "<=")
          result->mType = Ast::Type::LessOrEqual;
        else
        if (l.mValue == ">")
          result->mType = Ast::Type::Greater;
        else
        if (l.mValue == ">=")
          result->mType = Ast::Type::GreatorOrEqual;
        else
        if (l.mValue == "==")
          result->mType = Ast::Type::Equal;
        else
        if (l.mValue == "!=")
          result->mType = Ast::Type::NotEqual;
        else
        if (l.mValue == "&&")
          result->mType = Ast::Type::And;
        else
        if (l.mValue == "||")
          result->mType = Ast::Type::Or;
        break;

      case LexemType::Var:
        result->mType = Ast::Type::Var;
        result->mName = l.mValue;
        break;

      case LexemType::Dec:
        result->mType = Ast::Type::Number;
        result->mValue = (int64_t)atoll(l.mValue.c_str());
        break;

      case LexemType::Hex:
        result->mType = Ast::Type::Number;
        result->mValue = strx::fromHex2Int(l.mValue);
        break;

      case LexemType::Float:
        result->mType = Ast::Type::Number;
        result->mValue = (float)atof(l.mValue.c_str());
        break;

      case LexemType::Str:
        result->mType = Ast::Type::String;
        result->mValue = l.mValue.substr(1, l.mValue.size() - 2);
        break;

      default:
        throw std::runtime_error("Unexpected lexem.");
      }

      return result;
    }

    Lexem mLexem;
  public:
    Ast::PItem parseExpression(std::istream& input)
    {
      Ast::PItem operationNode(nullptr), leftNode(nullptr), rightNode(nullptr),
          currentOperation(nullptr);

      // While we have lexem
      while (mLexem = getLexem(input))
      {
        std::cout << "Returned lexem: " << mLexem.toString() << std::endl;

        if (!leftNode)
        {
          // It must be first operand!
          switch (mLexem.mType)
          {
          case LexemType::OpenBracket:
            leftNode = parseExpression(input);
            leftNode->mHasBrackets = true;
            break;

          case LexemType::CloseBracket:
            throw std::runtime_error("Expected +/-/constant/variable here.");

          case LexemType::Dec:
          case LexemType::Hex:
          case LexemType::Str:
          case LexemType::Var:
          case LexemType::Float:
            leftNode = makeAst(mLexem);
            break;

          default:
            throw std::runtime_error("Open bracket or constant / number / string / variable expected.");
          }
        }
        else
        if (!operationNode)
        {
          // Well, there is left node already
          // See operation here
          switch (mLexem.mType)
          {
          case LexemType::Oper:
            operationNode = makeAst(mLexem);
            break;

          case LexemType::None:
            // Finish the tree building
            break;

          case LexemType::CloseBracket:
            // Finish the tree building in this level
            if (leftNode)
              return leftNode;
            break;

          default:
            throw std::runtime_error("Expected operation.");
          }

          // Parse rest of expression
          rightNode = parseExpression(input);

          // If right part of expression is operation - make left side child of right part - to allow calculation in right order
          if (operationNode)
          {
            if (rightNode->isOperation() && rightNode->getOperatorLevel() <= operationNode->getOperatorLevel() && !rightNode->hasBrackets())
            {
              // Get left child of right expression - make it our right child
              operationNode->children().push_back(leftNode);
              operationNode->children().push_back(rightNode->children().front());
              rightNode->children().front() = operationNode;
              currentOperation = rightNode;
            }
            else
            {
              operationNode->children().push_back(leftNode);
              operationNode->children().push_back(rightNode);
              currentOperation = operationNode;
            }
          }
          if (mLexem.mType == LexemType::CloseBracket)
            break; // Exit from loop
        }
      }
      return currentOperation ? currentOperation : leftNode;
    }

  public:
    Ast::PItem parse(std::istream& input)
    {
      return nullptr;
    }

    void testLexemParser(const std::string& test)
    {
      std::istringstream iss(test);

      for (Lexem l = getLexem(iss); l.mType != LexemType::None; l = getLexem(iss))
      {
        std::cout << "Lexem type: " << (int)l.mType << ", value: " << l.mValue << std::endl;
      }
    }
  };

  class Worker
  {
  public:
    Variant eval(Ast::PItem ast);
  };
}


#endif
