#pragma once
#define AST_H

#include "orbit.h"

typedef struct {
    token* start;
    token* end;
    type* type;
} ast_base;

#define AST_NODES \
    /* primary exprs */ \
    ast_node(identifier, \
        struct { \
            bool is_attribute; \
            bool is_object; \
            bool is_function; \
            bool is_member; \
            bool is_type; \
            bool is_label; \
        };) \
    ast_node(int_constant, \
        union { \
            int int_value; \
            long long_value; \
            long long long_long_value; \
            u64 bit_precise_value; \
        };) \
    ast_node(float_constant, \
        union { \
            float float_value; \
            double double_value; \
            long double long_double_value; \
        };) \
    ast_node(string_literal, ) \
    /* - ( expression )*/ \
    /* shim nodes */ \
    ast_node(primary_expr, AST* expr;) \
    ast_node(postfix_expr, AST* expr;) \
    ast_node(unary_expr, AST* expr;) \
    ast_node(cast_expr, AST* expr;) \
    /* postfix_expr */ \
    /* - primary_expr */ \
    ast_node(array_index_expr, \
        type* type; \
        AST* lhs; \
        AST* rhs; \
        ) \
    ast_node(function_call_expr, \
        type* type; \
        AST* lhs; \
        Vec(AST) rhs; \
        ) \
    ast_node(aggregate_access_expr, \
        type* type; \
        AST* lhs; \
        AST* rhs; \
        bool through_pointer; \
    ) \
    ast_node(postfix_inc_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(postfix_dec_expr, \
        type* type; \
        AST* lhs; \
    ) \
    /* unary_expr */ \
    /* - postfix_expr */ \
    ast_node(prefix_inc_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(prefix_dec_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(addr_of_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(deref_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(unary_plus_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(negate_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(complement_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(not_expr, \
        type* type; \
        AST* lhs; \
    ) \
    ast_node(sizeof_expr, \
        type* type; \
        bool is_type_name; \
        AST* expr; \
    ) \
    ast_node(alignof_expr, \
        type* type; \
        AST* type_name; \
    ) \
    /* cast_expr */ \
    /* - unary_expr */ \
    ast_node(cast_expr, \
        type* type; \
        AST* expr; \
    ) \
    


typedef enum {
    AST_invalid,
#define ast_node(x, ...) AST_##x,
    AST_NODES
#undef ast_node
    AST_COUNT,
} ast_type;

typedef struct AST {
    union {
        void* rawptr;
        ast_base* base;
#define ast_node(ident, definition) struct ast_##ident* as_##ident;
        AST_NODES
#undef ast_node
    };
    ast_type type;
} AST;

typedef struct {

} type;

Vec_typedef(AST);

#define ast_node(ident, def) typedef struct ast_##ident { ast_base base; def } ast_##ident;
AST_NODES
#undef ast_node

