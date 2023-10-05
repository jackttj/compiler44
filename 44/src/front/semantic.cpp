#include "front/semantic.h"

#include <cassert>
#include <iostream>

using ir::Function;
using ir::Instruction;
using ir::Operand;
using ir::Operator;

#define TODO assert(0 && "TODO");

#define GET_CHILD_PTR(node, type, index)                     \
    auto node = dynamic_cast<type *>(root->children[index]); \
    assert(node);
#define ANALYSIS(node, type, index)                          \
    auto node = dynamic_cast<type *>(root->children[index]); \
    assert(node);                                            \
    analysis##type(node, buffer);
#define COPY_EXP_NODE(from, to)              \
    to->is_computable = from->is_computable; \
    to->v = from->v;                         \
    to->t = from->t;

map<std::string, ir::Function *> *frontend::get_lib_funcs()
{
    static map<std::string, ir::Function *> lib_funcs = {
        {"getint", new Function("getint", Type::Int)},
        {"getch", new Function("getch", Type::Int)},
        {"getfloat", new Function("getfloat", Type::Float)},
        {"getarray", new Function("getarray", {Operand("arr", Type::IntPtr)}, Type::Int)},
        {"getfarray", new Function("getfarray", {Operand("arr", Type::FloatPtr)}, Type::Int)},
        {"putint", new Function("putint", {Operand("i", Type::Int)}, Type::null)},
        {"putch", new Function("putch", {Operand("i", Type::Int)}, Type::null)},
        {"putfloat", new Function("putfloat", {Operand("f", Type::Float)}, Type::null)},
        {"putarray", new Function("putarray", {Operand("n", Type::Int), Operand("arr", Type::IntPtr)}, Type::null)},
        {"putfarray", new Function("putfarray", {Operand("n", Type::Int), Operand("arr", Type::FloatPtr)}, Type::null)},
    };
    return &lib_funcs;
}

void frontend::SymbolTable::add_scope(Block *node)
{
}
void frontend::SymbolTable::exit_scope()
{
    scope_stack.pop_back();
}

string frontend::SymbolTable::get_scoped_name(string id) const
{
    return scope_stack[scope_stack.size() - 1].name;
}

Operand frontend::SymbolTable::get_operand(string id) const
{
    size_t len = scope_stack.size();
    for (size_t i = len - 1; i >= 0; --i)
    {
        if (scope_stack[i].table.find(id) != scope_stack[i].table.end())
        {
            auto stee = *scope_stack[i].table.find(id);
            return stee.second.operand;
        }
    }
}

frontend::STE frontend::SymbolTable::get_ste(string id) const
{
    size_t len = scope_stack.size();

    for (int i = len - 1; i >= 0; i--)
    {
        auto mp = scope_stack[i].table;

        if (scope_stack[i].table.find(id) != scope_stack[i].table.end())
        {
            auto stee = *(scope_stack[i].table.find(id));
            return scope_stack[i].table.find(id)->second;
        }
    }
    frontend::STE ste;
    ste.operand.name = "Do not find the val";
    return ste;
}

frontend::Analyzer::Analyzer() : tmp_cnt(0), symbol_table()
{
}

ir::Program frontend::Analyzer::get_ir_program(CompUnit *root)
{

    ir::Program program;
    Analyzer::tmp_cnt = 0;

    int cnt = Analyzer::tmp_cnt;
    frontend::ScopeInfo scopeInfo;
    scopeInfo.cnt = cnt;
    scopeInfo.name = std::to_string(cnt);
    frontend::Analyzer::symbol_table.scope_stack.push_back(scopeInfo);

    // start the analysis of program
    analysisCompUnit(root, program);

    // deal with the program func
    global_function.name = "gloabalfunc";
    global_function.returnType = ir::Type::null;

    ir::Instruction *globalreturn = new ir::Instruction(ir::Operand(),
                                                        ir::Operand(),
                                                        ir::Operand(), ir::Operator::_return);
    global_function.addInst(globalreturn);
    program.addFunction(global_function);

    return program;
}

void frontend::Analyzer::analysisBlock(Block *root, vector<ir::Instruction *> &vc)
{
    // Block -> '{' { BlockItem } '}'
    if (root->children.size() <= 2)
        return; // if there is no BlockItem

    size_t index_of_blockItem = 1;
    while (index_of_blockItem < root->children.size() - 1)
    {
        analysisBlockItem((BlockItem *)root->children[index_of_blockItem], vc);
        index_of_blockItem++;
    }
}
void frontend::Analyzer::analysisBlock(Block *root, frontend::ScopeInfo &)
{
}
void frontend::Analyzer::analysisCompUnit(CompUnit *root, ir::Program &program)
{

    auto nodes = root->children;
    auto node = nodes[0];
    if (node->type == frontend::NodeType::FUNCDEF) // func case
    {
        ir::Function func;
        analysisFuncDef((FuncDef *)(node), func);
        program.addFunction(func);
        std::cout << " i hava finish a  func\n";
    }
    else
    {
        /**
         * Decl -> ConstDecl | VarDecl
         * VarDecl -> BType VarDef { ',' VarDef } ';'
         * VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
         */

        if (node->children[0]->type == frontend::NodeType::VARDECL)
        {

            auto vdecl = node->children[0];
            size_t index_of_vdef = 1;
            auto type_of_globalval = ir::Type::Int;
            switch (((Term *)(vdecl->children[0]))->token.type)
            {
            case frontend::TokenType::FLOATTK:
                /* code */
                type_of_globalval = ir::Type::Float;
            case frontend::TokenType::INTTK:
                type_of_globalval = ir::Type::Int;
                break;

            default:
                break;
            }

            auto type1 = ((Term *)(vdecl->children[0]))->token.type == frontend::TokenType::FLOATLTR ? ir::Type::FloatLiteral : ir::Type::IntLiteral;

            while (index_of_vdef < vdecl->children.size())
            {
                // add global_var
                auto vdef = vdecl->children[index_of_vdef];
                auto name_of_var = ((Term *)(vdef->children[0]))->token.value;

                Instruction *inst = new Instruction();
                inst->des.type = type_of_globalval;
                inst->op1.name = "0";
                inst->op1.type = type1;
                inst->des.name = name_of_var + "_";
                inst->des.type = ir::Type::Int;
                inst->op = ir::Operator::def;
                if (vdef->children.size() == 1 ||
                    (vdef->children.size() == 3 && vdef->children.back()->type == frontend::NodeType::INITVAL))
                {
                    // if have a initialVal add to table
                    program.globalVal.emplace_back(ir::Operand(name_of_var + "_", type_of_globalval));
                    if (vdef->children.size() > 1 && vdef->children[vdef->children.size() - 1]->type == frontend::NodeType::INITVAL)
                    {
                        analysisInitialval((InitVal *)(vdef->children[vdef->children.size() - 1]), global_function.InstVec);
                        inst->op1 = global_function.InstVec.back()->des;

                        // add to symbolTable
                        // name;
                        auto value = ((InitVal *)(vdef->children[vdef->children.size() - 1]))->v;
                        ir::Operand operand(value, ir::Type::IntLiteral);
                        vector<int> a = vector<int>(1); // 写死
                        frontend::STE ste;
                        ste.operand = operand;
                        name_of_var += "_";

                        symbol_table.scope_stack[symbol_table.scope_stack.size() - 1].table.insert(std::make_pair(name_of_var, ste));
                    }

                    // add the inst to globalfunc
                    global_function.addInst(inst);
                }
                else
                {
                    std::cout << "I am a  hight arr\n";
                    type_of_globalval = ir::Type::IntPtr;
                    program.globalVal.emplace_back(ir::Operand(name_of_var + "_", type_of_globalval));
                    size_t index_lbrace = 1;
                    size_t weishu = 1;
                    std::cout << "the type of vardef is " << toString(vdef->type) << std::endl;
                    ir::Instruction *defInst = new ir::Instruction(ir::Operand("1", ir::Type::IntLiteral),
                                                                   ir::Operand(),
                                                                   ir::Operand("weishu", ir::Type::Int), ir::Operator::def);
                    global_function.addInst(defInst);
                    while (index_lbrace < vdef->children.size() && vdef->children[index_lbrace + 1]->type != frontend::NodeType::INITVAL)
                    {
                        // VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
                        analysisConstExp((ConstExp *)vdef->children[index_lbrace + 1], global_function.InstVec);

                        ir::Instruction *mulInst = new ir::Instruction(ir::Operand("weishu", ir::Type::Int),
                                                                       global_function.InstVec.back()->des,
                                                                       ir::Operand("weishu", ir::Type::Int), ir::Operator::mul);
                        global_function.addInst(mulInst);

                        if (global_function.InstVec.back()->op1.type == ir::Type::IntLiteral)
                        {
                            auto val = global_function.InstVec.back()->op1.name;
                            weishu *= std::stoi(val);
                        }
                        index_lbrace += 3;
                    }
                    auto demession = std::to_string(weishu);
                    ir::Instruction *allocInst = new ir::Instruction(ir::Operand("weishu", ir::Type::Int),
                                                                     ir::Operand(),
                                                                     ir::Operand(name_of_var + "_", ir::Type::IntPtr), ir::Operator::alloc);
                    global_function.addInst(allocInst);
                    //// VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
                    // if there is some initial val
                    // then add some store inst
                    if (vdef->children.back()->type == frontend::NodeType::INITVAL)
                    {
                        // InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'
                        auto node_initial = vdef->children.back();
                        if (node_initial->children.size() > 2)
                        { // there is some inital val
                            size_t index_val = 1;
                            int index_for_store = 0;
                            while (index_val < node_initial->children.size())
                            {
                                analysisInitialval((InitVal *)node_initial->children[index_val], global_function.InstVec);

                                ir::Instruction *storeInst = new ir::Instruction(ir::Operand(name_of_var + "_", ir::Type::IntPtr),
                                                                                 ir::Operand(std::to_string(index_for_store), ir::Type::IntLiteral),
                                                                                 global_function.InstVec.back()->des, ir::Operator::store);
                                global_function.addInst(storeInst);
                                index_for_store++;
                                index_val += 2;
                            }
                        }
                    }
                }
                // goto the next vardef or the end
                index_of_vdef += 2;
            }
        }
        else
        {
            /**
             * ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
             * ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
             * ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
             * ConstExp -> AddExp
             * ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
             */
            auto constdecl = node->children[0];
            size_t index_of_constdef = 2;
            auto type_of_globalval = ir::Type::Int;
            switch (((Term *)(constdecl->children[1]))->token.type)
            {
            case frontend::TokenType::FLOATTK:
                /* code */
                type_of_globalval = ir::Type::Float;
            case frontend::TokenType::INTTK:
                type_of_globalval = ir::Type::Int;
                break;

            default:
                break;
            }

            auto type1 = ((Term *)(constdecl->children[1]))->token.type == frontend::TokenType::FLOATLTR ? ir::Type::FloatLiteral : ir::Type::IntLiteral;

            while (index_of_constdef < constdecl->children.size())
            {
                // add global_var
                auto cosntdef = constdecl->children[index_of_constdef];
                auto name_of_var = ((Term *)(cosntdef->children[0]))->token.value;

                Instruction *inst = new Instruction();
                inst->des.type = type_of_globalval;
                inst->op1.name = "0";
                inst->op1.type = type1;
                inst->des.name = name_of_var + "_";
                inst->des.type = ir::Type::Int;
                inst->op = ir::Operator::def;
                if (cosntdef->children.size() == 1 ||
                    (cosntdef->children.size() == 3 && cosntdef->children.back()->type == frontend::NodeType::CONSTINITVAL))
                {
                    // if have a initialVal add to table
                    std::cout << " In const initial val of yiwei\n";
                    program.globalVal.emplace_back(ir::Operand(name_of_var + "_", type_of_globalval));
                    if (cosntdef->children.size() > 1 && cosntdef->children[cosntdef->children.size() - 1]->type == frontend::NodeType::CONSTINITVAL)
                    {
                        std::cout << " i am in\n";
                        analysisConstExp((ConstExp *)(cosntdef->children.back()->children[0]), global_function.InstVec);
                        inst->op1 = global_function.InstVec.back()->op1;

                        // add to symbolTable
                        // name;

                        auto node = cosntdef->children.back();
                        analysisConstExp((ConstExp *)node->children[0], global_function.InstVec);
                        ir::Operand operand("0", ir::Type::IntLiteral);
                        auto ins = global_function.InstVec.back()->op1;
                        if (ins.type == ir::Type::IntLiteral)
                            operand = ins;
                        vector<int> a = vector<int>(1); // 写死
                        frontend::STE ste;
                        ste.operand = operand;
                        name_of_var += "_";
                        std::cout << "yiwei :"
                                  << "name is " << name_of_var + "_" << std::endl;
                        symbol_table.scope_stack[symbol_table.scope_stack.size() - 1].table.insert(std::make_pair(name_of_var + "_", ste));
                    }

                    // add the inst to globalfunc
                    global_function.addInst(inst);
                }
                else
                {
                    std::cout << "I am a  hight arr\n";
                    //  ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';'
                    //  * ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
                    //  * ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
                    //  * ConstExp -> AddExp
                    type_of_globalval = ir::Type::IntPtr;
                    program.globalVal.emplace_back(ir::Operand(name_of_var + "_", type_of_globalval));
                    size_t index_lbrace = 1;
                    size_t weishu = 1;
                    vector<int> a = vector<int>();
                    while (index_lbrace < cosntdef->children.size() && ((Term *)cosntdef->children[index_lbrace])->token.value == "[")
                    {
                        analysisConstExp((ConstExp *)cosntdef->children[index_lbrace + 1], global_function.InstVec);
                        if (global_function.InstVec.back()->op1.type == ir::Type::IntLiteral)
                        {
                            auto val = global_function.InstVec.back()->op1.name;
                            weishu *= std::stoi(val);
                            a.push_back(std::stoi(val));
                            std::cout << "weishu = " << weishu << std::endl;
                        }
                        index_lbrace += 3;
                    }
                    auto demession = std::to_string(weishu);
                    ir::Instruction *allocInst = new ir::Instruction(ir::Operand(demession, ir::Type::IntLiteral),
                                                                     ir::Operand(),
                                                                     ir::Operand(name_of_var + "_", ir::Type::IntPtr), ir::Operator::alloc);
                    global_function.addInst(allocInst);
                    //// VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]

                    frontend::STE ste;
                    ste.dimension = a;
                    this->symbol_table.scope_stack.back().table.insert(std::make_pair(name_of_var + "_", ste));
                    // deal with the initial val
                    size_t index_store = 0;
                    size_t index_val = 1;
                    auto node_constInitial = cosntdef->children.back();
                    /**
                     * ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
                     * ConstExp -> AddExp
                     */
                    std::cout << " I hava get here\n";
                    if (node_constInitial->children.size() > 2)
                    {
                        while (index_val < node_constInitial->children.size())
                        {
                            analysisConstInitVal((ConstInitVal *)node_constInitial->children[index_val], global_function.InstVec);
                            auto oper = global_function.InstVec.back()->des;
                            ir::Instruction *storeInst = new ir::Instruction(ir::Operand(name_of_var + "_", ir::Type::IntPtr),
                                                                             ir::Operand(std::to_string(index_store), ir::Type::IntLiteral),
                                                                             oper, ir::Operator::store);

                            global_function.addInst(storeInst);
                            index_val += 2;
                            index_store++;
                        }
                    }
                }

                // goto the next vardef or the end
                index_of_constdef += 2;
                std::cout << "yes I have finished\n";
            }
        }
    }
    if (root->children.size() == 2)
    {
        analysisCompUnit((CompUnit *)nodes[1], program);
    }
}
void frontend::Analyzer::analysisConstIniVal(ConstInitVal *root, vector<ir::Instruction *> &vc)
{
    if (root->children.size() > 1)
        for (unsigned int i = 1; i < root->children.size(); i += 2)
        {
            analysisConstInitVal((ConstInitVal *)root->children[i], vc);
        }
    else
        analysisConstExp((ConstExp *)root->children[0], vc);
}

void frontend::Analyzer::analysisConstExp(ConstExp *root, vector<ir::Instruction *> &vc)
{
    analysisAddExp((AddExp *)root->children[0], vc);
}
void frontend::Analyzer::analysisFuncDef(FuncDef *root, ir::Function &func)
{
    ir::CallInst *callGlobal = new ir::CallInst(ir::Operand("gloabalfunc", ir::Type::null),
                                                ir::Operand("t0", ir::Type::null));

    auto chis = root->children;
    func.returnType = ir::Type::Int; // FIXME
    if (((Term *)root->children[0]->children[0])->token.value == "void")
    {
        func.returnType = ir::Type::null;
    }
    func.name = root->n;
    if (root->n == "main")
        func.addInst(callGlobal);
    // if has args
    int index_of_params = 3;
    if (chis[index_of_params]->type == frontend::NodeType::FUNCFPARAMS)
    { // 如果是参数 就加入到 参数列表中
        auto params = chis[index_of_params]->children;
        size_t index_of_param = 0;
        while (index_of_param < params.size())
        {
            // add every param  into list

            auto func_param = params[index_of_param];
            ir::Operand operand;
            auto s = (Term *)func_param->children[1];
            operand.name = s->token.value + "_";
            switch (((Term *)func_param->children[0]->children[0])->token.type)
            {
            case frontend::TokenType::INTLTR:
            case frontend::TokenType::INTTK:
                operand.type = ir::Type::Int;
                break;
            case frontend::TokenType::FLOATLTR:
            case frontend::TokenType::FLOATTK:
                operand.type = ir::Type::Float;
                break;

            default:
                break;
            }
            func.ParameterList.push_back(operand);
            // operand.type = s->token.type;
            // struct Operand {
            // std::string name;
            // Type type;
            // Operand(std::string = "null", Type = Type::null);
            // };

            // jump to next param
            index_of_param += 2;
        }
    }

    // struct Instruction {
    // Operand op1;
    // Operand op2;
    // Operand des;
    // Operator op;
    // Instruction();
    // Instruction(const Operand& op1, const Operand& op2, const Operand& des, const Operator& op);
    // virtual std::string draw() const;

    // if hava instruction the add it into
    //  std::cout << "start the inst \n" << std::endl;
    std::cout << "start the anslysis of stmt\n";
    auto block = chis[chis.size() - 1];
    auto items = block->children;
    for (size_t index_of_block_item = 1; index_of_block_item < items.size() - 1; ++index_of_block_item)
    {
        // two case  decl or stmt
        auto item = items[index_of_block_item];

        if (item->children[0]->type == frontend::NodeType::STMT)
            analysisStmt((Stmt *)item->children[0], func.InstVec);
        else if (item->children[0]->type == frontend::NodeType::DECL)
            analysisDecl((Decl *)(item->children[0]), func.InstVec);
    }
    if (func.returnType == ir::Type::null)
    {
        ir::Instruction *returnInst = new ir::Instruction(ir::Operand(),
                                                          ir::Operand(),
                                                          ir::Operand(), ir::Operator::_return);
        func.addInst(returnInst);
    }
};
void frontend::Analyzer::analysisStmt(Stmt *root, vector<ir::Instruction *> &vc)
{
    //     struct Instruction {
    //     Operand op1;
    //     Operand op2;
    //     Operand des;
    //     Operator op;
    //     Instruction();
    //     Instruction(const Operand& op1, const Operand& op2, const Operand& des, const Operator& op);
    //     virtual std::string draw() const;
    // };
    // case return
    /**
     * Stmt -> LVal '=' Exp ';' | Block | 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
     *  | 'while' '(' Cond ')' Stmt | 'break' ';' | 'continue' ';'
     * | 'return' [Exp] ';' | [Exp] ';'
     */

    if (root->children[0]->type == frontend::NodeType::TERMINAL &&
        ((Term *)(root->children[0]))->token.value == "return")
    {
        // auto val = root -> children[1].
        // case return ;
        if (root->children.size() == 3)
        {
            analysisExp((Exp *)(root->children[1]), vc);
            ir::Instruction *inst = new ir::Instruction(vc.back()->des,
                                                        ir::Operand(),
                                                        ir::Operand(), ir::Operator::_return);
            vc.push_back(inst);
        }
        else
        {
            ir::Instruction *inst = new ir::Instruction(ir::Operand(),
                                                        ir::Operand(),
                                                        ir::Operand(), ir::Operator::_return);
            vc.push_back(inst);
        }
    }
    else if (root->children[0]->type == frontend::NodeType::TERMINAL &&
             ((Term *)(root->children[0]))->token.type == frontend::TokenType::COMMA)
    {
        // none inst
    }
    else if (root->children[0]->type == frontend::NodeType::EXP)
    {
        analysisExp((Exp *)root->children[0], vc);
    }
    else if (root->children[0]->type == frontend::NodeType::TERMINAL &&
             ((Term *)(root->children[0]))->token.value == "continue")
    {

        size_t now_ins = vc.size();
        size_t goto_ins = 0;
        if (this->index_for_continue.size() > 0)
        {
            goto_ins = this->index_for_continue.back(); // the index before con  pc xxxx
        }
        int index_ = (now_ins - goto_ins) * (-1);
        auto index_ins = std::to_string(index_);
        ir::Instruction *gotoInst1 = new ir::Instruction(ir::Operand(),
                                                         ir::Operand(),
                                                         ir::Operand(index_ins, ir::Type::IntLiteral), ir::Operator::_goto);
        vc.push_back(gotoInst1);
    }
    else if (root->children[0]->type == frontend::NodeType::TERMINAL &&
             ((Term *)(root->children[0]))->token.value == "while")
    {
        /**
         * 'while' '(' Cond ')' Stmt
         */
        this->index_of_goto.push_back(1);
        size_t pre_con = vc.size();
        this->index_for_continue.push_back(vc.size());

        auto con = (Cond *)(root->children[2]);
        analysisCond(con, vc);
        ir::Instruction *gotoInst = new ir::Instruction(ir::Operand("t3", ir::Type::Int),
                                                        ir::Operand(),
                                                        ir::Operand("2", ir::Type::IntLiteral), ir::Operator::_goto);

        size_t pre = vc.size();
        auto stm = (Stmt *)(root->children[4]);
        analysisStmt(stm, vc);

        size_t las = vc.size();
        size_t p_ = las - pre + 1;
        for (size_t i = 0; i < p_ - 1; ++i)
            vc.pop_back();

        vc.push_back(gotoInst);
        p_++;

        std::string p = std::to_string(p_);
        ir::Instruction *gotoInst1 = new ir::Instruction(ir::Operand(),
                                                         ir::Operand(),
                                                         ir::Operand(p, ir::Type::IntLiteral), ir::Operator::_goto);
        vc.push_back(gotoInst1);
        this->index_of_goto.pop_back();
        this->index_of_goto.push_back(vc.size()); // for break use

        analysisStmt(stm, vc);
        //     analysisCond(con, vc);
        size_t lass = vc.size();
        int pp = lass - pre_con;
        pp = pp * -1;

        std::string sp = std::to_string(pp);

        ir::Instruction *gotoInst2 = new ir::Instruction(ir::Operand(),
                                                         ir::Operand(),
                                                         ir::Operand(sp, ir::Type::IntLiteral), ir::Operator::_goto);

        vc.push_back(gotoInst2);
        this->index_of_goto.pop_back();
        this->index_for_continue.pop_back();
    }
    else if (root->children[0]->type == frontend::NodeType::TERMINAL &&
             ((Term *)(root->children[0]))->token.value == "if")
    {
        // 'if' '(' Cond ')' Stmt [ 'else' Stmt ]
        auto con = (Cond *)(root->children[2]);
        analysisCond(con, vc);

        size_t pre = vc.size();

        ir::Instruction *gotoInst = new ir::Instruction(ir::Operand("t3", ir::Type::Int),
                                                        ir::Operand(),
                                                        ir::Operand("2", ir::Type::IntLiteral), ir::Operator::_goto);

        auto stm = (Stmt *)(root->children[4]);

        analysisStmt(stm, vc);

        size_t las = vc.size();

        size_t pianyi = las - pre; //

        for (size_t i = 0; i < pianyi; ++i)
            vc.pop_back();
        vc.push_back(gotoInst);
        if (root->children.size() > 5)
            pianyi += 2;
        else
            pianyi++;

        std::string p = std::to_string(pianyi);

        ir::Instruction *gotoInst1 = new ir::Instruction(ir::Operand(),
                                                         ir::Operand(),
                                                         ir::Operand(p, ir::Type::IntLiteral), ir::Operator::_goto);

        vc.push_back(gotoInst1);

        analysisStmt(stm, vc); // this is  if's stmt
        size_t pre1 = vc.size();

        if (root->children.size() > 5)
        {
            auto stm1 = (Stmt *)(root->children[6]);
            analysisStmt(stm1, vc);
            size_t las1 = vc.size();

            int pianyi1 = las1 - pre1;
            for (int i = 0; i < pianyi1; ++i)
            {
                vc.pop_back();
            }

            pianyi1 += 1; // for getthe inst of goto itsel
            std::string p1 = std::to_string(pianyi1);

            ir::Instruction *gotoInst12 = new ir::Instruction(ir::Operand(),
                                                              ir::Operand(),
                                                              ir::Operand(p1, ir::Type::IntLiteral), ir::Operator::_goto);

            vc.push_back(gotoInst12);
            analysisStmt(stm1, vc);
        }
    }
    else if (root->children[0]->type == frontend::NodeType::BLOCK) // block
    {
        analysisBlock((Block *)(root->children[0]), vc);
    }
    else if (root->children[0]->type == frontend::NodeType::LVAL)
    {
        // Stmt -> LVal '=' Exp ';'

        auto val = ((Exp *)root->children[2])->v;

        auto sval = ((Term *)(root->children[0]->children[0]))->token.value;
        if (root->children[0]->children.size() == 1)
        {
            analysisExp((Exp *)root->children[2], vc);
            auto lastinst = vc[vc.size() - 1];
            ir::Instruction *inst = new ir::Instruction(lastinst->des,
                                                        ir::Operand(),
                                                        ir::Operand(sval + "_", ir::Type::Int), ir::Operator::mov);

            vc.push_back(inst);
        }
        else
        {
            // deal with the case arr[]
            // LVal -> Ident {'[' Exp ']'}
            auto node_lval = root->children[0];
            int index_of_load = 0;
            size_t index_of_exp = 2;
            int index_ = 0;
            vector<int> demo;
            auto demession = this->symbol_table.get_ste(sval + "_").dimension;
            demo.resize(demession.size());
            demo[demo.size() - 1] = 1;
            for (int i = (int)demession.size() - 2; i >= 0; --i)
            {
                demo[i] = demession[i + 1] * demo[i + 1];
            }

            std::cout << "I hava come here\n";
            ir::Instruction *assignInst = new ir::Instruction(ir::Operand("0", ir::Type::IntLiteral),
                                                              ir::Operand(),
                                                              ir::Operand("index_val", ir::Type::Int), ir::Operator::def);
            int index_name = 1;
            vc.push_back(assignInst);

            while (index_of_exp < node_lval->children.size())
            {
                analysisExp((Exp *)node_lval->children[index_of_exp], vc);
                std::string val;
                std::cout << " done\n";
                auto some = demo[index_];
                ir::Instruction *mulInst = new ir::Instruction(vc.back()->des,
                                                               ir::Operand(std::to_string(some), ir::Type::IntLiteral),
                                                               ir::Operand("t5", ir::Type::Int), ir::Operator::mul);
                Instruction *movInst3 = new ir::Instruction(ir::Operand("t5", ir::Type::Int),
                                                            ir::Operand("index_val", ir::Type::Int),
                                                            ir::Operand("index_val", ir::Type::Int), ir::Operator::add);
                vc.push_back(mulInst);
                vc.push_back(movInst3);

                index_++;
                index_of_exp += 3;
            }

            analysisExp((Exp *)root->children[2], vc);
            if (vc.back()->op1.type == ir::Type::IntLiteral)
            {
                auto val_exp = vc.back()->op1.name;
                ir::Instruction *storeInst = new ir::Instruction(ir::Operand(sval + "_", ir::Type::IntPtr),
                                                                 ir::Operand("index_val", ir::Type::Int),
                                                                 ir::Operand(val_exp, ir::Type::IntLiteral), ir::Operator::store);
                vc.push_back(storeInst);
            }
        }
    }
    else if (root->children[0]->type == frontend::NodeType::TERMINAL &&
             ((Term *)(root->children[0]))->token.value == "break")
    {

        size_t now_ins = vc.size();
        size_t goto_ins = 0;
        if (this->index_of_goto.size() > 0)
        {
            goto_ins = this->index_of_goto.back();
        }
        std::cout << "now-ins = " << now_ins << " goto-ins = " << goto_ins << std::endl;
        int index_ = (now_ins - goto_ins + 1) * (-1);
        auto index_ins = std::to_string(index_); // ?????? can not calcite in to_string ?????
        ir::Instruction *gotoInst1 = new ir::Instruction(ir::Operand(),
                                                         ir::Operand(),
                                                         ir::Operand(index_ins, ir::Type::IntLiteral), ir::Operator::_goto);
        vc.push_back(gotoInst1);
    }
}

void frontend::Analyzer::analysisExp(Exp *root, vector<ir::Instruction *> &vc)
{
    analysisAddExp((AddExp *)root->children[0], vc);
}

void frontend::Analyzer::analysisAddExp(AddExp *root, vector<ir::Instruction *> &vc)
{
    // AddExp -> MulExp { ('+' | '-') MulExp }
    // a - -5 + b + -5

    analysisMulExp((MulExp *)(root->children[0]), vc);

    size_t index_of_mulexp = 2;
    int name_index = 1;

    while (index_of_mulexp < root->children.size())
    {

        auto pre = vc[vc.size() - 1];
        auto name = "sub_" + std::to_string(name_index) + std::to_string(vc.size());
        name_index++;

        ir::Instruction *movInst = new ir::Instruction(pre->des,
                                                       ir::Operand(),
                                                       ir::Operand(name, ir::Type::Int), ir::Operator::mov);
        vc.push_back(movInst);

        analysisMulExp((MulExp *)(root->children[index_of_mulexp]), vc);

        Instruction *last = vc[vc.size() - 1];
        auto name1 = "sub_" + std::to_string(name_index) + std::to_string(vc.size());
        name_index++;
        ir::Instruction *movInst1 = new ir::Instruction(last->des,
                                                        ir::Operand(),
                                                        ir::Operand(name1, ir::Type::Int), ir::Operator::mov);
        vc.push_back(movInst1);
        auto operator1 = ((Term *)(root->children[index_of_mulexp - 1]))->token.type ==
                                 frontend::TokenType::PLUS
                             ? ir::Operator::add
                             : ir::Operator::sub;

        ir::Instruction *subInst = new ir::Instruction(
            ir::Operand(name, ir::Type::Int),
            ir::Operand(name1, ir::Type::Int),
            ir::Operand("t2", ir::Type::Int), operator1);

        vc.push_back(subInst);
        index_of_mulexp += 2;
    }
}
void frontend::Analyzer::analysisUnaryExp(UnaryExp *root, vector<ir::Instruction *> &vc)
{
    // UnaryExp -> PrimaryExp | Ident '(' [FuncRParams] ')' | UnaryOp UnaryExp
    auto chi1 = root->children[0];

    if (chi1->type == frontend::NodeType::PRIMARYEXP)
    {
        analysisPrimaryExp((PrimaryExp *)root->children[0], vc);
    }
    else
    {

        if (chi1->type == frontend::NodeType::UNARYOP)
        {

            analysisUnaryExp((UnaryExp *)root->children[1], vc);
            auto lastInst = vc[vc.size() - 1];

            auto op = (Term *)(root->children[0]->children[0]);
            if (op->token.value == "-")
            {
                ir::Instruction *mulInst = new ir::Instruction(lastInst->des,
                                                               ir::Operand("-1", ir::Type::IntLiteral),
                                                               lastInst->des, ir::Operator::mul);

                vc.push_back(mulInst);
            }
            else if (op->token.value == "!")
            {
                ir::Instruction *mulInst = new ir::Instruction(lastInst->des,
                                                               ir::Operand("-1", ir::Type::Int),
                                                               lastInst->des, ir::Operator::_not);

                vc.push_back(mulInst);
            }
            lastInst = vc[vc.size() - 1];
        }
        else
        { // call a func

            auto rparams = root->children[2];
            vector<ir::Operand> v;
            if (root->children.size() > 3)
            {
                size_t index_of_param = 0;
                auto params = rparams->children;
                while (index_of_param < params.size())
                {
                    // add every param  into list

                    auto func_param = params[index_of_param];
                    ir::Operand operand;
                    analysisExp((Exp *)func_param, vc);
                    auto lastinst = vc[vc.size() - 1];

                    v.push_back(lastinst->des);

                    // jump to next param
                    index_of_param += 2;
                }
            }
            // ir::CallInst call1(op1, v, op2);
            auto name = ((Term *)root->children[0])->token.value;

            auto m = get_lib_funcs();

            ir::CallInst *callInstd = new ir::CallInst(ir::Operand(((Term *)root->children[0])->token.value, ir::Type::null), ir::Operand("t2", ir::Type::Int));
            if (v.size() > 0)
                callInstd->argumentList = v;
            if (m->find(name) != m->end())
            {

                auto func = m->find(name)->second;
                if (func->returnType == ir::Type::null)
                {
                    callInstd->des = ir::Operand();
                }
            }
            vc.push_back(callInstd);
        }
    }
}
void frontend::Analyzer::analysisPrimaryExp(PrimaryExp *root, vector<ir::Instruction *> &vc)
{
    // PrimaryExp -> '(' Exp ')' | LVal | Number

    if (root->children[0]->type == frontend::NodeType::NUMBER)
    {
        auto val = ((Number *)(root->children[0]))->v;

        ir::Instruction *defInst = new ir::Instruction(ir::Operand(val, ir::Type::IntLiteral),
                                                       ir::Operand(),
                                                       ir::Operand("z", ir::Type::Int), ir::Operator::def);
        vc.push_back(defInst);
    }
    else if (root->children[0]->type == frontend::NodeType::LVAL)
    {
        // LVal -> Ident {'[' Exp ']'}
        auto node_lval = root->children[0];

        if (node_lval->children.size() == 1)
        {
            auto val = ((Number *)(node_lval))->v;
            auto name = ((LVal *)(node_lval))->v;
            // auto val_laval = symbol_table.get_ste(name + "_").operand.name;
            ir::Instruction *defInst = new ir::Instruction(ir::Operand(name + "_", ir::Type::Int),
                                                           ir::Operand(),
                                                           ir::Operand(name + "_", ir::Type::Int), ir::Operator::mov);
            //    std::cout << "this value is get from table\n";
            //    std::cout << name << " = " << val_laval << std::endl;
            vc.push_back(defInst);
        }
        else
        {
            vector<int> idx;
            size_t index_of_exp = 2;
            std::cout << "here" << std::endl;
            auto name = ((Term *)(node_lval->children[0]))->token.value;
            auto d = this->symbol_table.get_ste(name + "_").dimension;
            vector<int> de(d.size());
            std::cout << "finished get the table \n";
            for (size_t i = 0; i < d.size(); ++i)
                de[i] = d[i];
                de[de.size() - 1] = 1;

                std::cout << "one\n";
            for (int t = (int)(d.size()) - 2; t >= 0; t--)
            {
                de[t] = de[t + 1] * d[t + 1];
                std::cout << de[t] << std::endl;
            }
            int index = 0;
            ir::Instruction *assignInst = new ir::Instruction(ir::Operand("0", ir::Type::IntLiteral),
                                                              ir::Operand(),
                                                              ir::Operand("index_load", ir::Type::Int), ir::Operator::def);
            vc.push_back(assignInst);
            size_t i = 0;
            std::cout << " the wrong is here?\n";
            while (index_of_exp < node_lval->children.size())
            {
                analysisExp((Exp *)node_lval->children[index_of_exp], vc);
                 ir::Instruction *mulInst = new ir::Instruction(vc.back()->des,
                                     ir::Operand(std::to_string(de[i]),ir::Type::IntLiteral),
                                     ir::Operand("t5",ir::Type::Int),ir::Operator::mul);
                                     Instruction* addInst3 = new ir::Instruction(ir::Operand("t5",ir::Type::Int),
                                    ir::Operand("index_load", ir::Type::Int),
                                     ir::Operand("index_load", ir::Type::Int),ir::Operator::add);
                                     vc.push_back(mulInst); 
                                     vc.push_back(addInst3);

                index_of_exp += 3;
                i++;
            }
           
            std::cout << " yes I have come here\n";
            ir::Instruction *loadInst = new ir::Instruction(ir::Operand(name + "_", ir::Type::IntPtr),
                                                          ir::Operand("index_load", ir::Type::Int),
                                                            ir::Operand("t2", ir::Type::Int), ir::Operator::load);

            vc.push_back(loadInst);
        }
    }
    else
    {
        analysisExp((Exp *)root->children[1], vc);
    }
}
void frontend::Analyzer::analysisMulExp(MulExp *root, vector<ir::Instruction *> &vc)
{
    // MulExp -> UnaryExp { ('*' | '/' | '%') UnaryExp }

    auto node = root->children[0];

    analysisUnaryExp((UnaryExp *)(node), vc);

    size_t index_of_unaryexp = 2;
    int i = 0;
    while (index_of_unaryexp < root->children.size())
    {
        auto pre = vc[vc.size() - 1];
        auto name = "name_" + std::to_string(i);
        i++;
        ir::Instruction *assignInst = new ir::Instruction(pre->des,
                                                          ir::Operand(),
                                                          ir::Operand(name, ir::Type::Int), ir::Operator::mov);
        vc.push_back(assignInst);

        analysisUnaryExp((UnaryExp *)(root->children[index_of_unaryexp]), vc);
        Instruction *last = vc[vc.size() - 1];
        auto name1 = "name_" + std::to_string(i);
        i++;
        ir::Instruction *assignInst1 = new ir::Instruction(last->des,
                                                           ir::Operand(),
                                                           ir::Operand(name1, ir::Type::Int), ir::Operator::mov);
        vc.push_back(assignInst1);

        auto operator1 = ((Term *)(root->children[index_of_unaryexp - 1]))->token.type ==
                                 frontend::TokenType::MULT
                             ? ir::Operator::mul
                             : ir::Operator::div;

        ir::Instruction *subInst = new ir::Instruction(ir::Operand(name, ir::Type::Int),
                                                       ir::Operand(name1, ir::Type::Int),
                                                       ir::Operand("multy_val", ir::Type::Int), operator1);

        vc.push_back(subInst);
        index_of_unaryexp += 2;
    }
}
void frontend::Analyzer::analysisBlockItem(BlockItem *root, vector<ir::Instruction *> &vc)
{
    std::cout << " start the blockitem\n";
    if (root->children[0]->type == frontend::NodeType::DECL)
        analysisDecl((Decl *)root->children[0], vc);
    else
        analysisStmt((Stmt *)root->children[0], vc);
}
void frontend::Analyzer::analysisCond(Cond *root, vector<ir::Instruction *> &vc)
{
    // Cond -> LOrExp

    analysisLorExp((LOrExp *)root->children[0], vc);
    std::cout << "yes i am finished the con analysis\n";
}

void frontend::Analyzer::analysisLorExp(LOrExp *root, vector<ir::Instruction *> &vc)
{
    // LOrExp -> LAndExp [ '||' LOrExp ]
    analysisLAndExp((LAndExp *)root->children[0], vc);
    if (root->children.size() == 1)
    {
        ir::Instruction *assignInst = new ir::Instruction(vc.back()->des,
                                                          ir::Operand(),
                                                          ir::Operand("t3", ir::Type::Int), ir::Operator::def);
        vc.push_back(assignInst);
    }
    else
    {

        ir::Instruction *assignInst = new ir::Instruction(vc.back()->des,
                                                          ir::Operand(),
                                                          ir::Operand("or1", ir::Type::Int), ir::Operator::def);
        vc.push_back(assignInst);

        analysisLorExp((LOrExp *)root->children[2], vc);

        ir::Instruction *assignInst2 = new ir::Instruction(vc.back()->des,
                                                           ir::Operand(),
                                                           ir::Operand("or2", ir::Type::Int), ir::Operator::def);
        vc.push_back(assignInst2);

        ir::Instruction *or_Inst = new ir::Instruction(ir::Operand("or1", ir::Type::Int),
                                                       ir::Operand("or2", ir::Type::Int),
                                                       ir::Operand("t3", ir::Type::Int), ir::Operator::_or);
        vc.push_back(or_Inst);
    }
}
void frontend::Analyzer::analysisLAndExp(LAndExp *root, vector<ir::Instruction *> &vc)
{
    // LAndExp -> EqExp [ '&&' LAndExp ]
    analysisEqExp((EqExp *)root->children[0], vc);
    // if(root -> children.size() > 1){
    //     analysisLAndExp((LAndExp*)root->children[2], vc);
    // }

    if (root->children.size() == 1)
    {
        ir::Instruction *assignInst = new ir::Instruction(vc.back()->des,
                                                          ir::Operand(),
                                                          ir::Operand("v_l_and_exp", ir::Type::Int), ir::Operator::mov);
        vc.push_back(assignInst);
    }
    else
    {
        ir::Instruction *assignInst = new ir::Instruction(vc.back()->des,
                                                          ir::Operand(),
                                                          ir::Operand("t_eqexp", ir::Type::Int), ir::Operator::mov);
        vc.push_back(assignInst);

        analysisLAndExp((LAndExp *)root->children.back(), vc);
        ir::Instruction *assignInst1 = new ir::Instruction(vc.back()->des,
                                                           ir::Operand(),
                                                           ir::Operand("v_l_and_exp", ir::Type::Int), ir::Operator::mov);
        vc.push_back(assignInst1);

        ir::Instruction *andinst = new ir::Instruction(ir::Operand("t_eqexp", ir::Type::Int),
                                                       ir::Operand("v_l_and_exp", ir::Type::Int),
                                                       ir::Operand("v_l_and_exp", ir::Type::Int), ir::Operator::_and);
        vc.push_back(andinst);
    }
}

void frontend::Analyzer::analysisEqExp(EqExp *root, vector<ir::Instruction *> &vc)
{
    // EqExp -> RelExp { ('==' | '!=') RelExp }
    analysisRelExp((RelExp *)root->children[0], vc);

    if (root->children.size() == 1)
    {
        ir::Instruction *assignInst = new ir::Instruction(vc.back()->des,
                                                          ir::Operand(),
                                                          ir::Operand("t4", ir::Type::Int), ir::Operator::def);
        vc.push_back(assignInst);
    }
    size_t index_of_relexp = 2;
    int name_index = 0;

    while (index_of_relexp < root->children.size())
    {

        auto pre = vc.back();
        auto name = "name_" + std::to_string(name_index);
        name_index++;

        ir::Instruction *movInst = new ir::Instruction(pre->des,
                                                       ir::Operand(),
                                                       ir::Operand(name, ir::Type::Int), ir::Operator::mov);

        vc.push_back(movInst);
        analysisRelExp((RelExp *)root->children[index_of_relexp], vc);
        auto lst = vc.back();
        auto name1 = "name_" + std::to_string(name_index);
        name_index++;

        ir::Instruction *movInst1 = new ir::Instruction(lst->des,
                                                        ir::Operand(),
                                                        ir::Operand(name1, ir::Type::Int), ir::Operator::mov);

        vc.push_back(movInst1);
        if (((Term *)root->children[index_of_relexp - 1])->token.value == "==")
        {
            ir::Instruction *eqInst2 = new ir::Instruction(ir::Operand(name, ir::Type::Int),
                                                           ir::Operand(name1, ir::Type::Int),
                                                           ir::Operand("t4", ir::Type::Int), ir::Operator::eq);
            vc.push_back(eqInst2);
        }
        index_of_relexp += 2;
    }
}
void frontend::Analyzer::analysisRelExp(RelExp *root, vector<ir::Instruction *> &vc)
{
    // RelExp -> AddExp { ('<' | '>' | '<=' | '>=') AddExp }
    analysisAddExp((AddExp *)root->children[0], vc);

    if (root->children.size() == 1)
    {
        ir::Instruction *assignInst = new ir::Instruction(vc.back()->des,
                                                          ir::Operand(),
                                                          ir::Operand("t3", ir::Type::Int), ir::Operator::def);
        vc.push_back(assignInst);
    }
    else if (root->children.size() == 3)

    {
        auto pre = vc[vc.size() - 1];
        ir::Instruction *assignInst = new ir::Instruction(pre->des,
                                                          ir::Operand(),
                                                          ir::Operand("a11", ir::Type::Int), ir::Operator::mov);

        vc.push_back(assignInst);
        analysisAddExp((AddExp *)root->children[2], vc);
        auto las = vc[vc.size() - 1];
        ir::Instruction *assignInst1 = new ir::Instruction(las->des,
                                                           ir::Operand(),
                                                           ir::Operand("a22", ir::Type::Int), ir::Operator::mov);

        vc.push_back(assignInst1);
        auto opNode = (Term *)(root->children[1]);
        auto value = opNode->token.value;
        ir::Operator oper;
        if (value == "<")
        {
            oper = ir::Operator::lss;
        }
        else if (value == "<=")
        {
            oper = ir::Operator::leq;
        }
        else if (value == ">=")
        {
            oper = ir::Operator::geq;
        }
        else if (value == ">")
        {
            oper = ir::Operator::gtr;
        }

        ir::Instruction *lssInst = new ir::Instruction(ir::Operand("a11", ir::Type::Int),
                                                       ir::Operand("a22", ir::Type::Int),
                                                       ir::Operand("t3", ir::Type::Int), oper);

        vc.push_back(lssInst);
    }
}
void frontend::Analyzer::analysisDecl(Decl *root, std::vector<ir::GlobalVal> &vc)
{

    // Decl -> ConstDecl | VarDecl
    // VarDecl -> BType VarDef { ',' VarDef } ';'
    // VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
    auto chi = root->children[0];

    if (chi->type == frontend::NodeType::VARDECL)
    {
        auto vardecl = root->children[0];
        auto type = ((Term *)(vardecl->children[0]->children[0]))->token.type == frontend::TokenType::INTTK ? ir::Type::Int : ir::Type::Float;
        for (size_t j = 1; j < vardecl->children.size(); j += 2)
        {
            auto vdef = vardecl->children[j];
            auto term_node = (Term *)(vdef->children[0]);
            std::string names = term_node->token.value;

            Operand ope(names, type);

            ir::GlobalVal glo(ope);
            vc.push_back(glo);

            // do the val
        }
    }
}

void frontend::Analyzer::analysisDecl(Decl *root, std::vector<ir::Instruction *> &ins)
{
    //     struct Instruction {
    //     Operand op1;
    //     Operand op2;
    //     Operand des;
    //     Operator op;
    //     Instruction();
    //     Instruction(const Operand& op1, const Operand& op2, const Operand& des, const Operator& op);
    //     virtual std::string draw() const;
    // };
    if (root->children[0]->type == frontend::NodeType::CONSTDECL)
    {

        analysisConstDecl((ConstDecl *)root->children[0], ins);
    }
    else
    {
        analysisVarDecl((VarDecl *)root->children[0], ins);
    }
}

// Done
void frontend::Analyzer::analysisVarDecl(VarDecl *root, vector<ir::Instruction *> &vc)
{
    // VarDecl -> BType VarDef { ',' VarDef } ';'
    // VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
    size_t index_of_varDef = 1;
    auto chis = root->children;

    while (index_of_varDef < chis.size())
    {
        // des
        auto des_name_node = (Term *)(chis[index_of_varDef]->children[0]);
        auto des_name = des_name_node->token.value + "_";

        auto des_type_node = (BType *)chis[0];
        auto des_type = des_type_node->t;

        // val
        std::string val = "1";
        auto var_def = chis[index_of_varDef];
        int index_of_initial = var_def->children.size() - 1;

        // not  an arr
        if (var_def->children.size() == 1 ||
            (var_def->children.size() == 3 && var_def->children.back()->type == frontend::NodeType::INITVAL))
        {
            ir::Instruction *assignInst = new ir::Instruction(ir::Operand(val, ir::Type::IntLiteral),
                                                              ir::Operand(),
                                                              ir::Operand(des_name, des_type),
                                                              ir::Operator::def);
            if (index_of_initial > 0 && var_def->children[index_of_initial]->type == frontend::NodeType::INITVAL)
            {
                std::cout << " i start the initial analysis\n";
                auto initial_node = ((InitVal *)(var_def->children[index_of_initial]));
                analysisInitialval(initial_node, vc);
                assignInst->op1 = vc.back()->des;
            }

            // opr

            vc.push_back(assignInst);
            vector<int> a = vector<int>(1);
            frontend::STE ste;
            ste.operand = ir::Operand(val, ir::Type::IntLiteral);
            ste.dimension = a;
            this->symbol_table.scope_stack.back().table[des_name] = ste;
        }
        else
        {

            // find the weishu
            //// VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
            vector<int> a = vector<int>();
            frontend::STE ste;
            ste.operand = ir::Operand(val, ir::Type::IntLiteral);

            size_t index_lbrace = 1;
            size_t weishu = 1;
            size_t index_of_demession = 0;
            while (index_lbrace < var_def->children.size() &&
                   ((Term *)(var_def->children[index_lbrace]))->token.value == "[")
            {
                analysisExp((Exp *)var_def->children[index_lbrace + 1], vc);
                
                if (vc.back()->op1.type == ir::Type::IntLiteral)
                {
                    auto val = vc.back()->op1.name;
                    int x = std::stoi(val);
                    weishu *= x;
                    a.push_back(x);
                }
                Instruction *defInst = new ir::Instruction(vc.back()->des,
                                     ir::Operand(),
                                     ir::Operand(des_name + "demession_of_" + std::to_string(index_of_demession),ir::Type::Int),ir::Operator::def);
                vc.push_back(defInst);
                index_lbrace += 3;
            }
            auto demession = std::to_string(weishu);
            ir::Instruction *allocInst = new ir::Instruction(ir::Operand(demession, ir::Type::IntLiteral),
                                                             ir::Operand(),
                                                             ir::Operand(des_name, ir::Type::IntPtr), ir::Operator::alloc);
            vc.push_back(allocInst);
            std::cout << "success put the allocInst into vc\n;";
            //// VarDef -> Ident { '[' ConstExp ']' } [ '=' InitVal ]
            /**
             *  ir::Instruction storeInst(ir::Operand("arr",ir::Type::IntPtr),
                              ir::Operand("0",ir::Type::IntLiteral),
                              ir::Operand("2",ir::Type::IntLiteral),ir::Operator::store);
            */
            if (var_def->children.back()->type == frontend::NodeType::INITVAL)
            {
                std::cout << " I am come in in the  initial\n";

                int initial_index = 0;
                auto node_initial = var_def->children.back();
                if (node_initial->children.size() == 1)
                {
                    analysisExp((Exp *)node_initial->children[0], vc);
                    if (vc.back()->op1.type == ir::Type::IntLiteral)
                    {
                        ir::Instruction *storeInst = new ir::Instruction(ir::Operand(des_name, ir::Type::IntPtr),
                                                                         ir::Operand(std::to_string(initial_index), ir::Type::IntLiteral),
                                                                         vc.back()->op1, ir::Operator::store);
                        initial_index++;
                        vc.push_back(storeInst);
                    }
                }
                else if (node_initial->children.size() == 2)
                {
                }
                else
                {
                    // InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'
                    size_t index_of_val = 1;
                    while (index_of_val < node_initial->children.size())
                    {
                        std::cout << " start the high arr initial val analysis\n";

                        auto node = node_initial->children[index_of_val];
                        if (node->children[0]->type == frontend::NodeType::EXP)
                        {
                            analysisExp((Exp *)node->children[0], vc);
                            if (vc.back()->op1.type == ir::Type::IntLiteral)
                            {
                                ir::Instruction *storeInst = new ir::Instruction(ir::Operand(des_name, ir::Type::IntPtr),
                                                                                 ir::Operand(std::to_string(initial_index), ir::Type::IntLiteral),
                                                                                 vc.back()->op1, ir::Operator::store);
                                initial_index++;
                                vc.push_back(storeInst);
                            }
                        }

                        index_of_val += 2;
                    }
                }
            }
            ste.dimension = a;
            std::cout << "there ???????\n";
            this->symbol_table.scope_stack.back().table[des_name] = ste;
        }

        index_of_varDef += 2;
    }
}

void frontend::Analyzer::analysisInitialval(InitVal *root, vector<ir::Instruction *> &vc)
{
    // InitVal -> Exp | '{' [ InitVal { ',' InitVal } ] '}'

    if (root->children.size() == 1)
    {

        analysisExp((Exp *)root->children[0], vc);

        ir::Instruction *movInst = new ir::Instruction(vc.back()->des,
                                                       ir::Operand(),
                                                       ir::Operand("initial_val", ir::Type::Int), ir::Operator::mov);
        vc.push_back(movInst);
    }
}
void frontend::Analyzer::analysisConstDecl(ConstDecl *root, vector<ir::Instruction *> &vc)
{
    // ConstDecl -> 'const' BType ConstDef { ',' ConstDef } ';
    /**
     * ConstDef -> Ident { '[' ConstExp ']' } '=' ConstInitVal
     * ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
     * ConstExp -> AddExp
     * ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
     */
    size_t index_of_constDef = 2;
    auto chis = root->children;
    while (index_of_constDef < chis.size())
    {
        // des
        auto des_name_node = (Term *)(chis[index_of_constDef]->children[0]);
        auto des_name = des_name_node->token.value;

        auto des_type_node = (BType *)chis[1];
        auto des_type = des_type_node->t;

        // val
        std::string val = "1";
        int index_of_initial = chis[index_of_constDef]->children.size() - 1;

        if (index_of_initial > 0 && chis[index_of_constDef]->children[index_of_initial]->type == frontend::NodeType::CONSTINITVAL)
        {

            val = ((ConstInitVal *)(chis[index_of_constDef]->children[index_of_initial]))->v;
        }

        // opr

        ir::Instruction *assignInst = new ir::Instruction(ir::Operand(val, ir::Type::IntLiteral),
                                                          ir::Operand(),
                                                          ir::Operand(des_name + "_", des_type),
                                                          ir::Operator::def);
        vc.push_back(assignInst);
        vector<int> a = vector<int>(1);
        frontend::STE ste;
        ste.operand = ir::Operand(val, ir::Type::IntLiteral);
        ste.dimension = a;
        this->symbol_table.scope_stack.back().table[des_name + "_"] = ste;
        index_of_constDef += 2;
    }
}
void frontend::Analyzer::analysisBType(BType *, vector<ir::Instruction *> &)
{
}
void frontend::Analyzer::analysisConstDef(ConstDef *, vector<ir::Instruction *> &)
{
}
void frontend::Analyzer::analysisConstInitVal(ConstInitVal * root, vector<ir::Instruction *> & vc)
{
    //ConstInitVal -> ConstExp | '{' [ ConstInitVal { ',' ConstInitVal } ] '}'
    if(root -> children.size() == 1) analysisConstExp((ConstExp*)(root->children[0]), vc);
    else {
        size_t index_val = 1; 
        while(index_val < root -> children.size()){
            analysisConstExp((ConstExp*)root->children[index_val], vc);
            index_val += 2;
        }
    }
    std::cout << "finished one \n";
}
