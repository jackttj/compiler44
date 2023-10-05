#include "front/syntax.h"

#include <iostream>
#include <cassert>

using frontend::Parser;

// #define DEBUG_PARSER
#define TODO assert(0 && "todo")
#define CUR_TOKEN_IS(tk_type) (token_stream[index].type == TokenType::tk_type)
#define PARSER_TOKEN(tk_type) root->children.push_back(parserTerm(root, TokenType::tk_type))
// #define PARSE(name, type) auto name = new type(root); assert(parse##type(name)); root->children.push_back(name);

#define PARSER(name, type)      \
    auto name = new type();     \
    assert(parser##type(name)); \
    root->children.push_back(name);
Parser::Parser(const std::vector<frontend::Token> &tokens) : index(0), token_stream(tokens)
{
}

Parser::~Parser() {}

frontend::CompUnit *Parser::get_abstract_syntax_tree()
{
    CompUnit *root = new CompUnit();
    log(root);
    Parser::parserCompUnit(root);

    return root;
}

void Parser::log(AstNode *node)
{
#ifdef DEBUG_PARSER
    std::cout << "in parse" << toString(node->type) << ", cur_token_type::" << toString(token_stream[index].type) << ", token_val::" << token_stream[index].value << '\n';
#endif
}

bool Parser::parserCompUnit(frontend::CompUnit *root)
{

    log(root);
    /**
     * CompUnit -> (Decl | FuncDef) [CompUnit]
     */

    if (index + 2 < token_stream.size() && token_stream[index + 2].type == frontend::TokenType::LPARENT)
    {

        PARSER(root1, FuncDef);
    }
    else
    {

        PARSER(root1, Decl);
    }
    while (index < token_stream.size())
    {
        PARSER(roo2, CompUnit);
    }

    return true;
}
bool Parser::parserDecl(frontend::Decl *root)
{

    /**
     * Decl -> ConstDecl | VarDecl
     */

    log(root);

    if (CUR_TOKEN_IS(CONSTTK))
    {
        PARSER(root1, ConstDecl);
        return true;
    }
    else
    {
        PARSER(root2, VarDecl);
        return true;
    }

    return false;
}

bool Parser::parserConstDecl(frontend::ConstDecl *root)
{
    /**
     * ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
     */
    log(root);

    PARSER_TOKEN(CONSTTK);
    // PARSER(root1, BType)

    auto root1 = new BType();
    assert(parserBType(root1));
    root->children.push_back(root1);

    root->t = root1->t;
    PARSER(root2, ConstDef)

    while (!CUR_TOKEN_IS(SEMICN))
    {
        PARSER_TOKEN(COMMA);
        PARSER(root3, ConstDef)
    }
    PARSER_TOKEN(SEMICN);

    return true;
}

bool Parser::parserVarDecl(frontend::VarDecl *root)
{
    /**
     * VarDecl -> BType VarDef { ',' VarDef } ';'
     */
    log(root);
    PARSER(root1, BType);

    PARSER(root2, VarDef);

    while (index < token_stream.size() && CUR_TOKEN_IS(COMMA))
    {
        PARSER_TOKEN(COMMA);
        PARSER(root3, VarDef);
    }

    PARSER_TOKEN(SEMICN);

    return true;
}

bool Parser::parserBType(frontend::BType *root)
{
    /**
     * BType -> 'int' | 'float'
     */
    log(root);
    if (CUR_TOKEN_IS(INTTK))
    {
        PARSER_TOKEN(INTTK);
        root->t = ir::Type::Int;
        return true;
    }
    else if (CUR_TOKEN_IS(FLOATTK))
    {
        PARSER_TOKEN(FLOATTK);
        return true;
    }

    return false;
}

bool Parser::parserConstDef(frontend::ConstDef *root)
{
    /**
     * ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
     */
    log(root);
    /**
     * Ident
     */

    PARSER_TOKEN(IDENFR);

    /**
     * { '[' ConstExp ']' }
     */

    while (!CUR_TOKEN_IS(ASSIGN))
    {

        PARSER_TOKEN(LBRACK);

        PARSER(root1, ConstExp);

        PARSER_TOKEN(RBRACK);
    }

    PARSER_TOKEN(ASSIGN);

    /**
     * ConstInitVal
     */
    PARSER(root2, ConstInitVal);

    return true;
}

bool Parser::parserConstExp(frontend::ConstExp *root)
{

    /**
     * ConstExp -> AddExp
     */
    log(root);
    PARSER(root1, AddExp);
    root -> v = root1 -> v;

    return true;
}

bool Parser::parserConstInitVal(frontend::ConstInitVal *root)
{
    /**
     * ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
     */
    log(root);
    if (CUR_TOKEN_IS(LBRACE))
    {
        PARSER_TOKEN(LBRACE);
        if (CUR_TOKEN_IS(RBRACE))
        {
            PARSER_TOKEN(RBRACE);
            return true;
        }
        else
        {
            PARSER(root1, ConstInitVal);
            while (index < token_stream.size() && CUR_TOKEN_IS(COMMA))
            {
                PARSER_TOKEN(COMMA);
                PARSER(root2, ConstInitVal);
            }
            PARSER_TOKEN(RBRACE);
            return true;
        }
    }
    else
    {
        PARSER(root1, ConstExp);
        root -> v = root1 -> v;
        return true;
    }

    return true;
}

bool Parser::parserVarDef(frontend::VarDef *root)
{
    /**
     * VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
     */
    log(root);
    PARSER_TOKEN(IDENFR);

    while (CUR_TOKEN_IS(LBRACK))
    {
        PARSER_TOKEN(LBRACK);
        PARSER(root1, ConstExp);
        PARSER_TOKEN(RBRACK);
    }
    if (index < token_stream.size() && CUR_TOKEN_IS(ASSIGN))
    {
        PARSER_TOKEN(ASSIGN);
        PARSER(root2, InitVal);

        return true;
    }

    return true;
}

bool Parser::parserInitVal(frontend::InitVal *root)
{
    /**
     * InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'
     */
    log(root);
    if (CUR_TOKEN_IS(LBRACE))
    {
        PARSER_TOKEN(LBRACE);
        if (CUR_TOKEN_IS(RBRACE))
        {
            PARSER_TOKEN(RBRACE);
            return true;
        }
        else
        {
            PARSER(root2, InitVal);
            while (index < token_stream.size() && CUR_TOKEN_IS(COMMA))
            {
                PARSER_TOKEN(COMMA);
                PARSER(root3, InitVal);
            }
            PARSER_TOKEN(RBRACE);
            return true;
        }
    }
    else
    {
        PARSER(root3, Exp);
        root->v = root3->v;

        return true;
    }

    return true;
}

bool Parser::parserFuncDef(frontend::FuncDef *root)
{
    log(root);
    /**
     * FuncDef -> FuncType Ident '(' [FuncFParams] ')' Block
     */

    log(root);
    PARSER(root1, FuncType);
    root->n = token_stream[index].value;
    PARSER_TOKEN(IDENFR);
    PARSER_TOKEN(LPARENT);
    if (CUR_TOKEN_IS(RPARENT))
    {
        PARSER_TOKEN(RPARENT);
        PARSER(root3, Block);
        return true;
    }
    else
    {
        PARSER(root4, FuncFParams);
        PARSER_TOKEN(RPARENT);
        PARSER(root3, Block);
        return true;
    }
    return true;
}

bool Parser::parserFuncType(frontend::FuncType *root)
{
    /**
     * FuncType -> 'void' | 'int' | 'float'
     */
    log(root);
    if (CUR_TOKEN_IS(VOIDTK))
    {
        PARSER_TOKEN(VOIDTK);
        return true;
    }
    else if (CUR_TOKEN_IS(INTTK))
    {
        PARSER_TOKEN(INTTK);
        return true;
    }
    else if (CUR_TOKEN_IS(FLOATTK))
    {
        PARSER_TOKEN(FLOATTK);
        return true;
    }

    return false;
}

bool Parser::parserFuncFParams(frontend::FuncFParams *root)
{
    /**
     * FuncFParams -> FuncFParam { ',' FuncFParam }
     */
    log(root);
    PARSER(root1, FuncFParam);

    while (index < token_stream.size() && CUR_TOKEN_IS(COMMA))
    {
        PARSER_TOKEN(COMMA);
        PARSER(root2, FuncFParam);
    }
    return true;
}

bool Parser::parserBlock(frontend::Block *root)
{
    /**
     * Block -> '{' { BlockItem } '}'
     *
     */
    log(root);
    PARSER_TOKEN(LBRACE);

    if (CUR_TOKEN_IS(RBRACE))
    {
        PARSER_TOKEN(RBRACE);
        return true;
    }
    else
    {

        PARSER(root1, BlockItem);
    }
    while (!CUR_TOKEN_IS(RBRACE))
    {

        PARSER(root2, BlockItem);
    }
    PARSER_TOKEN(RBRACE);

    return true;
}

bool Parser::parserFuncFParam(frontend::FuncFParam *root)
{
    /**
     * FuncFParam -> BType Ident ['[' ']' { '[' Exp ']' }]
     */
    log(root);
    PARSER(root1, BType);
    PARSER_TOKEN(IDENFR);
    if (CUR_TOKEN_IS(LBRACK))
    {
        PARSER_TOKEN(LBRACK);
        PARSER_TOKEN(RBRACK);
        while (index < token_stream.size() && CUR_TOKEN_IS(LBRACK))
        {
            PARSER_TOKEN(LBRACK);
            PARSER(root2, Exp);
            PARSER_TOKEN(RBRACK);
        }
        return true;
    }
    else
        return true;

    return true;
}

bool Parser::parserBlockItem(frontend::BlockItem *root)
{
    /**
     * BlockItem -> Decl | Stmt
     *
     */
    log(root);
    if (CUR_TOKEN_IS(CONSTTK) || CUR_TOKEN_IS(INTTK) || CUR_TOKEN_IS(FLOATTK))
    {
        PARSER(root1, Decl);
        return true;
    }
    else
    {
        PARSER(root2, Stmt);
        return true;
    }

    return true;
}

bool Parser::parserLVal(frontend::LVal *root)
{
    /**
     * LVal -> Ident {'[' Exp ']'}
     */
    log(root);
    root -> v = token_stream[index].value;
    PARSER_TOKEN(IDENFR);

    while (index < token_stream.size() && CUR_TOKEN_IS(LBRACK))
    {
        PARSER_TOKEN(LBRACK);
        PARSER(root1, Exp);
        PARSER_TOKEN(RBRACK);
    }

    return true;
}

bool Parser::parserCond(frontend::Cond *root)
{
    /**
     * Cond -> LOrExp
     */
    log(root);
    PARSER(root1, LOrExp);

    return true;
}

bool Parser::parserExp(frontend::Exp *root)
{
    /**
     * Exp -> AddExp
     */
    log(root);
    PARSER(root1, AddExp);
    root->v = root1->v;

    return true;
}

bool Parser::parserAddExp(frontend::AddExp *root)
{
    /**
     * AddExp -> MulExp { ('+' | '-') MulExp }
     */
    log(root);

    PARSER(root1, MulExp);
    root->v = root1->v;

    while (CUR_TOKEN_IS(PLUS) || CUR_TOKEN_IS(MINU))
    {

        if (CUR_TOKEN_IS(PLUS))
        {
            PARSER_TOKEN(PLUS);
            PARSER(root2, MulExp);
            root -> v = std::to_string(stoi(root -> v) + stoi(root2 -> v));
        }
        else if (CUR_TOKEN_IS(MINU))
        {
            PARSER_TOKEN(MINU);
            PARSER(root2, MulExp);
            root -> v = std::to_string(stoi(root -> v) - stoi(root2 -> v));
        }
    }

    return true;
}

bool Parser::parserNumber(frontend::Number *root)
{

    /**
     * Number -> IntConst | floatConst
     */

    log(root);
    if (CUR_TOKEN_IS(INTLTR))
    {
        root->v = token_stream[index].value;
        PARSER_TOKEN(INTLTR);

        return true;
    }
    else if (CUR_TOKEN_IS(FLOATLTR))
    {
        PARSER_TOKEN(FLOATLTR);
        return true;
    }
    return true;
}

bool Parser::parserPrimaryExp(frontend::PrimaryExp *root)
{
    /**
     * PrimaryExp -> '(' Exp ')' | LVal | Number
     */
    log(root);

    if (CUR_TOKEN_IS(LPARENT))
    {
        PARSER_TOKEN(LPARENT);
        PARSER(root1, Exp);
        root -> v = root1 -> v;

        PARSER_TOKEN(RPARENT);
        return true;
    }
    else if (CUR_TOKEN_IS(INTLTR) || CUR_TOKEN_IS(FLOATLTR))
    {

        root->v = token_stream[index].value;
        PARSER(root1, Number);

        return true;
    }
    else
    {

        PARSER(root2, LVal);
        root -> v = "1";
        return true;
    }

    return true;
}

bool Parser::parserUnaryOp(frontend::UnaryOp *root)
{
    /**
     * UnaryOp -> '+' | '-' | '!'
     */
    log(root);

    if (CUR_TOKEN_IS(PLUS))
    {
        PARSER_TOKEN(PLUS);
        return true;
    }
    else if (CUR_TOKEN_IS(MINU))
    {
        PARSER_TOKEN(MINU);
        return true;
    }
    else if (CUR_TOKEN_IS(NOT))
    {
        PARSER_TOKEN(NOT);
        return true;
    }

    return false;
}

bool Parser::parserUnaryExp(frontend::UnaryExp *root)
{
    /**
     * UnaryExp -> PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
     *
     */

    log(root);

    if (CUR_TOKEN_IS(PLUS))
    {
        PARSER(root2, UnaryOp);
        PARSER(root1, UnaryExp);
        root -> v = root1 -> v;
        return true;
    }
    else if (CUR_TOKEN_IS(MINU))
    {
        PARSER(root2, UnaryOp);
        PARSER(root1, UnaryExp);
        root -> v = std::to_string(stoi(root1 -> v) * (-1));
        return true;
    }
    else if (CUR_TOKEN_IS(NOT)) // the up three is UnaryOp
    {
        PARSER(root2, UnaryOp);
        PARSER(root1, UnaryExp);
        if(root1 -> v == "0") root -> v = "1";
        else root -> v = "0";
        return true;
    }
    else if (CUR_TOKEN_IS(IDENFR) && token_stream[index + 1].type == frontend::TokenType::LPARENT)
    {
        PARSER_TOKEN(IDENFR);
        PARSER_TOKEN(LPARENT);

        if (CUR_TOKEN_IS(RPARENT))
        {
            PARSER_TOKEN(RPARENT);
            root -> v = "1";

            return true;
        }
        else
        {
            PARSER(root2, FuncRParams);
            PARSER_TOKEN(RPARENT);
            root -> v = 1;
            return true;
        }
    }
    else // Ident !!!!!!!wrong
    {
        PARSER(root3, PrimaryExp);
        // PARSER_TOKEN(RPARENT); //fuck bug in there
        root->v = root3->v;

        return true;
    }

    return false;
}

bool Parser::parserFuncRParams(frontend::FuncRParams *root)
{
    /**
     * FuncRParams -> Exp { ',' Exp }
     */
    log(root);
    PARSER(root1, Exp);
    while (index < token_stream.size() && CUR_TOKEN_IS(COMMA))
    {
        PARSER_TOKEN(COMMA);
        PARSER(root2, Exp);
    }

    return true;
}

bool Parser::parserRelExp(frontend::RelExp *root)
{
    /**
     * RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
     */
    log(root);

    PARSER(root1, AddExp);
    while (index < token_stream.size())
    {
        if (CUR_TOKEN_IS(LSS))
        {
            PARSER_TOKEN(LSS);
            PARSER(root2, AddExp);
        }
        else if (CUR_TOKEN_IS(GTR))
        {
            PARSER_TOKEN(GTR);
            PARSER(root2, AddExp);
        }
        else if (CUR_TOKEN_IS(LEQ))
        {
            PARSER_TOKEN(LEQ);
            PARSER(root2, AddExp);
        }
        else if (CUR_TOKEN_IS(GEQ))
        {
            PARSER_TOKEN(GEQ);
            PARSER(root2, AddExp);
        }
        else
            break;
    }

    return true;
}

bool Parser::parserEqExp(frontend::EqExp *root)
{
    /**
     * EqExp -> RelExp { ('==' | '!=') RelExp }
     */
    log(root);
    PARSER(root1, RelExp);
    while (index < token_stream.size())
    {
        if (CUR_TOKEN_IS(EQL))
        {
            PARSER_TOKEN(EQL);
            PARSER(root2, RelExp);
        }
        else if (CUR_TOKEN_IS(NEQ))
        {
            PARSER_TOKEN(NEQ);
            PARSER(root2, RelExp);
        }
        else
            break;
    }

    return true;
}

bool Parser::parserLAndExp(frontend::LAndExp *root)
{
    /**
     * LAndExp -> EqExp [ '&&' LAndExp ]
     */
    log(root);

    PARSER(root1, EqExp);
    if (index < token_stream.size() && CUR_TOKEN_IS(AND))
    {
        PARSER_TOKEN(AND);
        PARSER(root2, LAndExp);
    }

    return true;
}

bool Parser::parserLOrExp(frontend::LOrExp *root)
{
    /**
     * LOrExp -> LAndExp [ '||' LOrExp ]
     */
    log(root);

    PARSER(root1, LAndExp);
    if (index < token_stream.size() && CUR_TOKEN_IS(OR))
    {
        PARSER_TOKEN(OR);
        PARSER(root2, LOrExp);
    }

    return true;
}
bool Parser::parserStmt(frontend::Stmt *root)
{
    /**
     * Stmt -> LVal '=' Exp ';' | Block |
     *  'if' '(' Cond ')' Stmt [ 'else' Stmt ] |
     * 'while' '(' Cond ')' Stmt | 'break' ';' |
     *  'continue' ';' | 'return' [Exp] ';' | [Exp] ';'
     *
     *
     */
    log(root);

    if (CUR_TOKEN_IS(IDENFR))
    { // not the next
        if (index + 1 < token_stream.size() && token_stream[index + 1].type == frontend::TokenType::LPARENT)
        {
            // exp

            PARSER(root2, Exp);
            PARSER_TOKEN(SEMICN);
            return true;
        }
        else
        {

            uint32_t pre = index;
            PARSER(roo2, LVal);
            if (CUR_TOKEN_IS(ASSIGN))
            {
                PARSER_TOKEN(ASSIGN);
                PARSER(root3, Exp);
                ((LVal*)(root ->children[root ->children.size() - 3])) -> v = "3";
                PARSER_TOKEN(SEMICN);
                return true;
            }
            else
            {
                root->children.pop_back();
                index = pre;
                PARSER(root3, Exp);
                PARSER_TOKEN(SEMICN);
                return true;
            }

            return true;
        }
    }
    else if (CUR_TOKEN_IS(LBRACE))
    {

        PARSER(root1, Block);
        return true;
    }
    else if (CUR_TOKEN_IS(IFTK))
    {
        PARSER_TOKEN(IFTK);
        PARSER_TOKEN(LPARENT);

        PARSER(root1, Cond);
        PARSER_TOKEN(RPARENT);

        PARSER(root2, Stmt);

        if (CUR_TOKEN_IS(ELSETK))
        {
            PARSER_TOKEN(ELSETK);
            PARSER(root3, Stmt);
            return true;
        }
        else
            return true;
    }
    else if (CUR_TOKEN_IS(WHILETK))
    {
        PARSER_TOKEN(WHILETK);
        PARSER_TOKEN(LPARENT);

        PARSER(root1, Cond);
        PARSER_TOKEN(RPARENT);

        PARSER(root2, Stmt);
        return true;
    }
    else if (CUR_TOKEN_IS(BREAKTK))
    {
        PARSER_TOKEN(BREAKTK);
        PARSER_TOKEN(SEMICN);
        return true;
    }
    else if (CUR_TOKEN_IS(CONTINUETK))
    {
        PARSER_TOKEN(CONTINUETK);
        PARSER_TOKEN(SEMICN);
        return true;
    }
    else if (CUR_TOKEN_IS(RETURNTK))
    {
        PARSER_TOKEN(RETURNTK);
        if (CUR_TOKEN_IS(SEMICN))
        {
            PARSER_TOKEN(SEMICN);
            return true;
        }
        else
        {
            PARSER(root1, Exp);

            PARSER_TOKEN(SEMICN);
            return true;
        }
        return true;
    }
    else
    {
        if (CUR_TOKEN_IS(SEMICN))
        {
            PARSER_TOKEN(SEMICN);
            return true;
        }
        PARSER(root1, Exp);
        PARSER_TOKEN(SEMICN);
        return true;
    }
    return true;
}

bool Parser::parserMulExp(frontend::MulExp *root)
{
    /**
     * MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }
     *
     */
    log(root);
    PARSER(root1, UnaryExp);
    root->v = root1->v;
    while (CUR_TOKEN_IS(MULT) || CUR_TOKEN_IS(DIV) || CUR_TOKEN_IS(MOD))
    {
        if (CUR_TOKEN_IS(MULT))
        {
            PARSER_TOKEN(MULT);
            PARSER(root2, UnaryExp);
            root -> v = std::to_string(stoi(root -> v) * stoi(root2 -> v));
            
        }
        else if (CUR_TOKEN_IS(DIV))
        {
            PARSER_TOKEN(DIV);
            PARSER(root2, UnaryExp);

            root -> v = std::to_string(stoi(root -> v) / stoi(root2 -> v));
        }
        else if (CUR_TOKEN_IS(MOD))
        {
            PARSER_TOKEN(MOD);
            PARSER(root2, UnaryExp);
            root -> v = std::to_string(stoi(root -> v) % stoi(root2 -> v));
        }
        else
        {
            break;
        }
    }

    return true;
}

/**
 * FIX ME
 * which is better for parserTerm's return type
 * bool or Term?
 */

frontend::Term *Parser::parserTerm(frontend::AstNode *root, frontend::TokenType type)
{

    if (token_stream[index].type == type)
    {

        frontend::Term *chi = new frontend::Term(token_stream[index]);
        chi->token = token_stream[index];
        index++;
        return chi;
    }
    else
    {

        throw "error";
    }
}
