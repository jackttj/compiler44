#include"front/lexical.h"

#include<map>
#include<cassert>
#include<string>

#define TODO assert(0 && "todo")

// #define DEBUG_DFA
// #define DEBUG_SCANNER


bool isoperator(char c)
{
    std::string s = "+-*/%<>:=;,()[]{}!<=>===!=&&||";
    return !(s.find(c) == std::string::npos);
}

frontend::TokenType get_op_type(std::string s)
{

    if (s == "+")
        return frontend::TokenType::PLUS;

    else if (s == "-")
        return frontend::TokenType::MINU;

    else if (s == "*")
        return frontend::TokenType::MULT;

    else if (s == "/")
        return frontend::TokenType::DIV;

    else if (s == "%")
        return frontend::TokenType::MOD;

    else if (s == "<")
        return frontend::TokenType::LSS;

    else if (s == ">")
        return frontend::TokenType::GTR;

    else if (s == ":")
        return frontend::TokenType::COLON;

    else if (s == "=")
        return frontend::TokenType::ASSIGN;

    else if (s == ";")
        return frontend::TokenType::SEMICN;

    else if (s == ",")
        return frontend::TokenType::COMMA;

    else if (s == "(")
        return frontend::TokenType::LPARENT;

    else if (s == ")")
        return frontend::TokenType::RPARENT;

    else if (s == "[")
        return frontend::TokenType::LBRACK;

    else if (s == "]")
        return frontend::TokenType::RBRACK;

    else if (s == "{")
        return frontend::TokenType::LBRACE;

    else if (s == "}")
        return frontend::TokenType::RBRACE;

    else if (s == "!")
        return frontend::TokenType::NOT;

    else if (s == "<=")
        return frontend::TokenType::LEQ;
    else if (s == ">=")
        return frontend::TokenType::GEQ;
    else if (s == "==")
        return frontend::TokenType::EQL;
    else if (s == "!=")
        return frontend::TokenType::NEQ;
    else if (s == "&&")
        return frontend::TokenType::AND;
    else if (s == "||")
        return frontend::TokenType::OR;
    else
        return frontend::TokenType::ASSIGN;
}

std::string frontend::toString(State s) {
    switch (s) {
    case State::Empty: return "Empty";
    case State::Ident: return "Ident";
    case State::IntLiteral: return "IntLiteral";
    case State::FloatLiteral: return "FloatLiteral";
    case State::op: return "op";
    default:
        assert(0 && "invalid State");
    }
    return "";
}

std::set<std::string> frontend::keywords= {
    "const", "int", "float", "if", "else", "while", "continue", "break", "return", "void"
};

frontend::DFA::DFA(): cur_state(frontend::State::Empty), cur_str() {}

frontend::DFA::~DFA() {}

bool frontend::DFA::next(char input, Token& buf) {
#ifdef DEBUG_DFA
#include<iostream>
    std::cout << "in state [" << toString(cur_state) << "], input = \'" << input << "\', str = " << cur_str << "\t";
#endif
    if (input == ' ' || input == '\n') //
    {
        if (cur_state == State::Empty)
            return false;
        else
        {
            buf.value = cur_str;
            if (cur_state == State::op)
            {
                buf.type = get_op_type(cur_str);
            }
            else if (cur_state == State::IntLiteral)
            {
                buf.type = TokenType::INTLTR;
            }
            else if (cur_state == State::FloatLiteral)
            {
                buf.type = TokenType::FLOATLTR;
            }
            else if (cur_state == State::Ident)
            {
                buf.type = TokenType::IDENFR; // fix me
                if (frontend::keywords.find(cur_str) != frontend::keywords.end())
                {
                    buf.type = frontend::get_tk_type(cur_str);
                }
            }

            reset();

            return true;
        }
    }
    else if ((input >= '0' && input <= '9')) // if the input is a  number
    {
        if (cur_state == State::Empty || cur_state == State::IntLiteral)
        {

            cur_state = State::IntLiteral;
            cur_str += input;
            return false;
        }
        else if (cur_state == State::op)
        {
            buf.value = cur_str;

            buf.type = get_op_type(cur_str);

            reset();
            cur_state = State::IntLiteral;
            cur_str += input;
            return true;
        }
        else if(cur_state == State::Ident){
            cur_str += input;
            return false;
        }
        else if(cur_state == State::FloatLiteral){
            cur_str += input;
            return false;
        }
    }
    else if (isoperator(input))
    {
        if (cur_state == State::Empty)
        {
            cur_state = State::op;
            cur_str += input;
            if (input == '<' || input == '>' || input == '=' || input == '!' || input == '&' || input == '|')
            {
                return false;
            }
            buf.type = get_op_type(cur_str);
            buf.value = cur_str;
            reset();
            return true;
        }
        else if (cur_state == State::IntLiteral) // if the pre is op todo it other
        {
            buf.value = cur_str;
            if (cur_state == State::IntLiteral)
                buf.type = TokenType::INTLTR;
            else
                buf.type = get_op_type(cur_str);
            reset();
            cur_state = State::op;
            cur_str += input;
            return true;
        }
        else if (cur_state == State::FloatLiteral) // if the pre is op todo it other
        {
            buf.value = cur_str;
            if (cur_state == State::FloatLiteral)
                buf.type = TokenType::FLOATLTR;
            else
                buf.type = get_op_type(cur_str);
            reset();
            cur_state = State::op;
            cur_str += input;
            return true;
        }
        else if (cur_state == State::op) // if the pre is op todo it other
        {
            if (cur_str.size() == 1 && (cur_str + input == "<=" ||
                                        cur_str + input == ">=" ||
                                        cur_str + input == "==" ||
                                        cur_str + input == "!=" ||
                                        cur_str + input == "||" ||
                                        cur_str + input == "&&"))
            {

                cur_str += input;
                return false;
            } // two puple
            else
            {

                buf.value = cur_str;
                buf.type = get_op_type(cur_str);
                reset();
                cur_state = State::op;
                cur_str += input;
                return true;
            }
        }
        else if (cur_state == State::Ident)
        {
            if (keywords.find(cur_str) != keywords.end())
            {
                buf.type = get_tk_type(cur_str);
            }
            else
            {
                buf.type = frontend::TokenType::IDENFR;
            }
            buf.value = cur_str;
            reset();
            cur_state = State::op;
            cur_str += input;
            return true;
        }
    }
    else if ((input >= 'a' && input <= 'z') || (input >= 'A' && input <= 'Z') || input == '_')
    {
        // number,  indent ,
        if (cur_state == State::IntLiteral)
        {
            cur_str += input;
            return false;
        }
        else if (cur_state == State::Ident || cur_state == State::Empty) // Ident
        {
            cur_str += input;
            cur_state = State::Ident;
            return false; // false
        }
        else if (cur_state == State::op)
        {
            buf.type = get_op_type(cur_str);
            buf.value = cur_str;
            reset();
            cur_state = State::Ident;
            cur_str += input;
            return true;
        }
    }
    else if (input == '.')
    {
        if (cur_state == State::IntLiteral)
        {
            cur_state = State::FloatLiteral;
            cur_str += input;
            return false;
        }
        else if(cur_state == State::Empty){
            cur_state = State::FloatLiteral;
            cur_str += input;
            return false;
        }
        else {
            if(cur_state != State::Empty){
                buf.type = get_tk_type(cur_str);
                buf.value = cur_str;
                reset();
                cur_state = State::FloatLiteral;
                cur_str += input;
                return true;
            }
        }
    }
    return false;
#ifdef DEBUG_DFA
    std::cout << "next state is [" << toString(cur_state) << "], next str = " << cur_str << "\t, ret = " << ret << std::endl;
#endif
}

void frontend::DFA::reset() {
    cur_state = State::Empty;
    cur_str = "";
}

frontend::Scanner::Scanner(std::string filename): fin(filename) {
    if(!fin.is_open()) {
        assert(0 && "in Scanner constructor, input file cannot open");
    }
}

frontend::Scanner::~Scanner() {
    fin.close();
}

std::vector<frontend::Token> frontend::Scanner::run() {
    std::vector<frontend::Token> tokens;
    char input;
    frontend::DFA dfa;
    frontend::Token tk;

    fin >> std::noskipws;

    while (fin >> input)
    {
        // handle the comment
        if (input == '/')
        {

            if (fin >> input)
            {
                // case one comment ways of //
                if (input == '/')
                {
                    while (fin >> input)
                    {
                        if (input == '\n')
                            break;
                    }
                }
                else if (input == '*')
                {
                    while (fin >> input)
                    {
                        if (input == '*')
                        {
                            fin >> input;
                            if (input == '/')
                                break;
                        }
                    }
                    fin >> input;
                }
                else
                {
                    if (dfa.next('/', tk))
                    {

                        tokens.push_back(tk);
                    }
                }
            }
            else
                break;
        }

        // std::cout << input << std::endl;

        if (dfa.next(input, tk))
        {

            tokens.push_back(tk);
        }
    }
    

    return tokens;
#ifdef DEBUG_SCANNER
#include<iostream>
            std::cout << "token: " << toString(tk.type) << "\t" << tk.value << std::endl;
#endif

}
