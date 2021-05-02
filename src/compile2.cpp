#include "compile2.hpp"

namespace fn {

using namespace fn_parse;
using namespace fn_bytes;

local_table::local_table(local_table* parent, func_stub* func)
    : vars{}
    , parent{parent}
    , cur_func{func}
    , sp{0} {
    if (parent != nullptr) {
        sp = parent->sp;
    }
}

u8 local_table::add_upvalue(u32 levels, u8 pos) {
    auto call = this;
    while (call->cur_func == nullptr && call != nullptr) {
        call = call->parent;
    }

    // levels == 1 => this is a direct upvalue, so add it and return
    if (levels == 1) {
        return call->cur_func->get_upvalue(pos, true);
    }

    // levels > 1 => need to get the upvalue from an enclosing function
    u8 slot = call->parent->add_upvalue(levels - 1, pos);
    return call->cur_func->get_upvalue(slot, false);
}

void compiler::compile_subexpr(local_table& locals, const ast_node* expr) {
    if (expr->kind == ak_atom) {
        compile_atom(locals, *expr->datum.atom, expr->loc);
    } else if (expr->kind == ak_list) {
        compile_list(locals, *expr->datum.list, expr->loc);
    } else {
        error("Parser error state.", expr->loc);
        return;
    }
}


optional<local_addr> compiler::find_local(local_table& locals,
                                          bool* is_upval,
                                          symbol_id name) {
    auto& l = locals;
    optional<u8*> res;
    u32 levels = 0;
    // keep track of how many enclosing functions we need to go into
    while (true) {
        res = l.vars.get(name);
        if (res.has_value()) {
            break;
        }

        // here we're about to ascend to an enclosing function, so we need an upvalue
        if (l.cur_func != nullptr) {
            levels += 1;
        }
        if (l.parent == nullptr) {
            break;
        }
    }

    if (levels > 0 && res.has_value()) {
        *is_upval = true;
        return locals.add_upvalue(levels, **res);
    } else if (res.has_value()) {
        *is_upval = false;
        return **res;
    }

    return std::nullopt;
}

void compiler::compile_atom(local_table& locals,
                            const ast_atom& atom,
                            const source_loc& loc) {
    const_id id;
    const symbol* s;
    switch (atom.type) {
    case at_number:
        id = dest->num_const(atom.datum.num);
        constant(id);
        ++locals.sp;
        break;
    case at_string:
        id = dest->str_const(*atom.datum.str);
        constant(id);
        ++locals.sp;
        break;
    case at_symbol:
        // TODO: check for special symbols
        s = &(*symtab)[atom.datum.sym];
        if (s->name == "null") {
            dest->write_byte(OP_NULL);
        } else if(s->name == "true") {
            dest->write_byte(OP_TRUE);
        } else if (s->name == "false") {
            dest->write_byte(OP_FALSE);
        } else {
            compile_var(locals, atom.datum.sym, loc);
        }
        break;
    }
}

void compiler::compile_var(local_table& locals,
                           symbol_id sym,
                           const source_loc& loc) {
    bool up;
    auto l = find_local(locals, &up, sym);
    if (l.has_value()) {
        dest->write_byte(up ? OP_UPVALUE : OP_LOCAL);
        dest->write_byte(*l);
    } else {
        auto id = dest->sym_const(sym);
        dest->write_byte(OP_CONST);
        dest->write_short(id);
        dest->write_byte(OP_GLOBAL);
    }
    ++locals.sp;
}

void compiler::compile_list(local_table& locals,
                            const vector<ast_node*>& list,
                            const source_loc& loc) {
    if (list.size() == 0) {
        error("Encountered empty list.", loc);
        return;
    }
    if (list[0]->kind == ak_atom
        && list[0]->datum.atom->type == at_symbol) {
        auto sym = list[0]->datum.atom->datum.sym;
        const string& name = (*symtab)[sym].name;
        if (name == "and") {
            compile_and(locals, list, list[0]->loc);
        // } else if (name == "cond") {
        //     compile_cond(locals, list, list[0]->loc);
        } else if (name == "def") {
            compile_def(locals, list, list[0]->loc);
        // } else if (name == "defmacro") {
        //     compile_defmacro(locals, list, list[0]->loc);
        // } else if (name == "defn") {
        //     compile_defn(locals, list, list[0]->loc);
        } else if (name == "do") {
            compile_do(locals, list, list[0]->loc);
        // } else if (name == "dot") {
        //     compile_dot(locals, list, list[0]->loc);
        // } else if (name == "dollar-fn") {
        //     compile_dollar_fn(locals, list, list[0]->loc);
        } else if (name == "if") {
            compile_if(locals, list, list[0]->loc);
        // } else if (name == "import") {
        //     compile_import(locals, list, list[0]->loc);
        // } else if (name == "fn") {
        //     compile_fn(locals, list, list[0]->loc);
        } else if (name == "let") {
            compile_let(locals, list, list[0]->loc);
        // } else if (name == "letfn") {
        //     compile_letfn(locals, list, list[0]->loc);
        } else if (name == "or") {
            compile_or(locals, list, list[0]->loc);
        // } else if (name == "quasiquote") {
        //     compile_quasiquote(locals, list, list[0]->loc);
        // } else if (name == "quote") {
        //     compile_quote(locals, list, list[0]->loc);
        // } else if (name == "unquote") {
        //     compile_unquote(locals, list, list[0]->loc);
        // } else if (name == "unquote-splicing") {
        //     compile_unquote_splicing(locals, list, list[0]->loc);
        // } else if (name == "set!") {
        //     compile_set(locals, list, list[0]->loc);
        // } else if (name == "with") {
        //     compile_with(locals, list, list[0]->loc);
        } else {
            compile_call(locals, list);
        }
    } else {
        compile_call(locals, list);
    }
}

void compiler::compile_call(local_table& locals,
                            const vector<ast_node*>& list) {
    auto base_sp = locals.sp;
    u32 num_args = -1;
    for (auto x : list) {
        compile_subexpr(locals, x);
        ++num_args;
    }
    if (num_args > 255) {
        error("Function call with more than 255 arguments.", list.back()->loc);
    }
    dest->write_byte(OP_CALL);
    dest->write_byte((u8)num_args);
    locals.sp = base_sp + 1;
}

void compiler::compile_and(local_table& locals,
                           const vector<fn_parse::ast_node*>& list,
                           const source_loc& loc) {
    forward_list<bc_addr> patch_locs;
    for (u32 i = 1; i < list.size(); ++i) {
        compile_subexpr(locals, list[i]);
        // skip to end jump on false
        dest->write_byte(OP_CJUMP);
        dest->write_short(0);
        --locals.sp;
        patch_locs.push_front(dest->get_size());
    }
    dest->write_byte(OP_TRUE);
    dest->write_byte(OP_JUMP);
    dest->write_short(1);
    auto end_addr = dest->get_size();
    for (auto u : patch_locs) {
        dest->patch_short(u - 2, end_addr - u);
    }
    dest->write_byte(OP_FALSE);
    ++locals.sp;
}

void compiler::compile_def(local_table& locals,
                           const vector<ast_node*>& list,
                           const source_loc& loc) {
    if (list.size() != 3) {
        error("Wrong number of arguments to def.", loc);
    }

    if (!list[1]->is_symbol()) {
        error("First argument to def must be a symbol.", loc);
    }

    auto sym = list[1]->datum.atom->datum.sym;
    // TODO: check for illegal symbol names
    constant(dest->sym_const(sym));
    ++locals.sp;
    compile_subexpr(locals, list[2]);
    dest->write_byte(OP_SET_GLOBAL);
    dest->write_byte(OP_NULL);
}

void compiler::compile_do(local_table& locals,
                          const vector<ast_node*>& list,
                          const source_loc& loc) {
    if (list.size() == 1) {
        dest->write_byte(OP_NULL);
        ++locals.sp;
        return;
    }

    u32 i;
    for (i = 1; i < list.size()-1; ++i) {
        compile_subexpr(locals, list[i]);
        dest->write_byte(OP_POP);
        --locals.sp;
    }
    compile_subexpr(locals, list[i]);
}

void compiler::compile_if(local_table& locals,
                          const vector<ast_node*>& list,
                          const source_loc& loc) {
    if (list.size() != 4) {
        error("Wrong number of arguments to if.", loc);
    }
    compile_subexpr(locals, list[1]);

    dest->write_byte(OP_CJUMP);
    dest->write_short(0);
    --locals.sp;

    auto then_addr = dest->get_size();
    compile_subexpr(locals, list[2]);
    dest->write_byte(OP_JUMP);
    dest->write_short(0);

    // put the stack pointer back since only one expression will be evaluated
    --locals.sp;
    auto else_addr = dest->get_size();
    compile_subexpr(locals, list[3]);

    auto end_addr = dest->get_size();
    dest->patch_short(then_addr - 2, else_addr - then_addr);
    dest->patch_short(else_addr - 2, end_addr - else_addr);
}

void compiler::compile_let(local_table& locals,
                           const vector<ast_node*>& list,
                           const source_loc& loc) {
    // TODO: fix something about this
    // if (locals.parent == nullptr) {
    //     error("Let cannot occur at the top level.", loc);
    // }


    // check we have an even number of arguments
    if ((list.size() & 1) != 1) {
        error("Wrong number of arguments to let.", loc);
    }

    for (u32 i = 1; i < list.size(); i += 2) {
        if (!list[i]->is_symbol()) {
            error("Names in let must be symbols.", list[i]->loc);
        }

        auto sym = list[i]->datum.atom->datum.sym;
        auto loc = locals.sp++;
        // initial value null (in case of recursive reads)
        dest->write_byte(OP_NULL);
        locals.vars.insert(sym, loc);
        compile_subexpr(locals, list[i+1]);
        dest->write_byte(OP_SET_LOCAL);
        dest->write_byte(loc);
        dest->write_byte(OP_NULL);
    }
}

void compiler::compile_or(local_table& locals,
                          const vector<fn_parse::ast_node*>& list,
                          const source_loc& loc) {
    forward_list<bc_addr> patch_locs;
    for (u32 i = 1; i < list.size(); ++i) {
        compile_subexpr(locals, list[i]);
        // skip the next jump on false
        dest->write_byte(OP_CJUMP);
        dest->write_short(3);
        --locals.sp;
        dest->write_byte(OP_JUMP);
        dest->write_short(0);
        patch_locs.push_front(dest->get_size());
    }
    dest->write_byte(OP_FALSE);
    dest->write_byte(OP_JUMP);
    dest->write_short(1);
    auto end_addr = dest->get_size();
    for (auto u : patch_locs) {
        dest->patch_short(u - 2, end_addr - u);
    }
    dest->write_byte(OP_TRUE);
    ++locals.sp;
}

void compiler::compile_expr() {
    local_table l;
    auto expr = parse_node(sc, symtab);
    compile_subexpr(l, expr);
    delete expr;
    dest->write_byte(OP_POP);
}

void compiler::compile_to_eof() {
    while (!sc->eof()) {
        compile_expr();
    }
}


}