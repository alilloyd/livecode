/* Copyright (C) 2003-2013 Runtime Revolution Ltd.
 
 This file is part of LiveCode.
 
 LiveCode is free software; you can redistribute it and/or modify it under
 the terms of the GNU General Public License v3 as published by the Free
 Software Foundation.
 
 LiveCode is distributed in the hope that it will be useful, but WITHOUT ANY
 WARRANTY; without even the implied warranty of MERCHANTABILITY or
 FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 for more details.
 
 You should have received a copy of the GNU General Public License
 along with LiveCode.  If not see <http://www.gnu.org/licenses/>.  */

#include "literal.h"
#include "report.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

////////////////////////////////////////////////////////////////////////////////

// typedef struct Name *NameRef;
struct Name
{
    NameRef next;
    char *token;
};

typedef struct Binding *BindingRef;
struct Binding
{
    BindingRef next;
    NameRef name;
    unsigned int has_meaning : 1;
    long meaning;
};

typedef struct Scope *ScopeRef;
struct Scope
{
    ScopeRef outer;
    BindingRef bindings;
};

static NameRef s_names;
static ScopeRef s_scopes;

////////////////////////////////////////////////////////////////////////////////

void InitializeLiterals(void)
{
    s_names = NULL;
}

void FinalizeLiterals(void)
{
}

////////////////////////////////////////////////////////////////////////////////

void MakeIntegerLiteral(const char *p_token, long *r_literal)
{
    *r_literal = atoi(p_token);
}

void MakeDoubleLiteral(const char *p_token, long *r_literal)
{
    double *t_value;
    t_value = (double *)malloc(sizeof(double));
    if (t_value == NULL)
        Fatal_OutOfMemory();
    *t_value = atof(p_token);
    *r_literal = (long)t_value;
}

static int char_to_nibble(char p_char, int *r_nibble)
{
    if (isdigit(p_char))
        *r_nibble = p_char - '0';
    else if (p_char >= 'a' && p_char <= 'f')
        *r_nibble = (p_char - 'a') + 10;
    else if (p_char >= 'A' && p_char <= 'F')
        *r_nibble = (p_char - 'A') + 10;
    else
        return 0;
    
    return 1;
}

void append_utf8_char(char *p_string, int *x_index, int p_char)
{
    if (p_char < 128)
    {
        p_string[*x_index] = p_char;
        (*x_index) += 1;
    }
    else if (p_char >= 0x10000)
    {
        p_string[*x_index] = 0xf0 | (p_char >> 18);
        p_string[*x_index + 1] = 0x80 | ((p_char >> 12) & 0x3f);
        p_string[*x_index + 2] = 0x80 | ((p_char >> 6) & 0x3f);
        p_string[*x_index + 3] = 0x80 | ((p_char >> 0) & 0x3f);
        (*x_index) += 4;
    }
    else if (p_char >= 0x0800)
    {
        p_string[*x_index] = 0xe0 | (p_char >> 12);
        p_string[*x_index + 1] = 0x80 | ((p_char >> 6) & 0x3f);
        p_string[*x_index + 2] = 0x80 | ((p_char >> 0) & 0x3f);
        (*x_index) += 3;
    }
    else
    {
        p_string[*x_index] = 0xc0 | (p_char >> 6);
        p_string[*x_index + 1] = 0x80 | ((p_char >> 0) & 0x3f);
        (*x_index) += 2;
    }
}

int UnescapeStringLiteral(const char *p_string, long *r_unescaped_string)
{
    // Allocate enough room for the length of the string including a NUL char.
    // This is more than enough to handle any escapes as escaped chars are always
    // greater in length than their unescaped versions.
    char *t_value;
    const char *t_limit;
    int t_length;
    const char *t_ptr;
    
    t_value = (char *)malloc(strlen(p_string) + 1);
    
    t_limit = p_string + strlen(p_string);
    t_length = 0;
    
    for(t_ptr = p_string; t_ptr < t_limit; t_ptr++)
    {
        if (*t_ptr == '\\')
        {
            if (t_ptr + 1 < t_limit)
            {
                t_ptr += 1;
                
                if (*t_ptr == 'q')
                    t_value[t_length++] = '"';
                else if (*t_ptr == 'n')
                    t_value[t_length++] = '\n';
                else if (*t_ptr == 'r')
                    t_value[t_length++] = '\r';
                else if (*t_ptr == 't')
                    t_value[t_length++] = '\t';
                else if (*t_ptr == '\\')
                    t_value[t_length++] = '\\';
                else if (*t_ptr == 'x')
                {
                    t_ptr += 1;
                    
                    if (t_ptr + 1 < t_limit)
                    {
                        int t_nibble_1, t_nibble_2;
                        int t_char;
                        
                        if (!char_to_nibble(*t_ptr, &t_nibble_1))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_2))
                            goto error_exit;
                        
                        t_char = (t_nibble_1 << 4) | t_nibble_2;
                        append_utf8_char(t_value, &t_length, t_char);
                    }
                }
                else if (*t_ptr == 'u')
                {
                    t_ptr += 1;
                    
                    if (t_ptr + 3 < t_limit)
                    {
                        int t_nibble_1, t_nibble_2, t_nibble_3, t_nibble_4;
                        int t_char;
                        
                        if (!char_to_nibble(*t_ptr, &t_nibble_1))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_2))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_3))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_4))
                            goto error_exit;
                        
                        t_char = (t_nibble_1 << 12) | (t_nibble_2 << 8) | (t_nibble_3  << 4) | t_nibble_4;
                        append_utf8_char(t_value, &t_length, t_char);
                    }
                }
                else if (*t_ptr == 'U')
                {
                    t_ptr += 1;
                    
                    if (t_ptr + 5 < t_limit)
                    {
                        int t_nibble_1, t_nibble_2, t_nibble_3, t_nibble_4, t_nibble_5, t_nibble_6;
                        int t_char;
                        
                        if (!char_to_nibble(*t_ptr, &t_nibble_1))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_2))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_3))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_4))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_5))
                            goto error_exit;
                        t_ptr += 1;
                        if (!char_to_nibble(*t_ptr, &t_nibble_6))
                            goto error_exit;
                        
                        t_char = (t_nibble_1 << 20) | (t_nibble_2 << 16) | (t_nibble_3 << 12) | (t_nibble_4 << 8) | (t_nibble_5 << 4) | t_nibble_6;
                        append_utf8_char(t_value, &t_length, t_char);
                    }
                }
                else
                    goto error_exit;
            }
        }
        else
            t_value[t_length++] = *t_ptr;
    }
    
    t_value[t_length++] = '\0';
    
    *r_unescaped_string = (long)t_value;

    return 1;
    
error_exit:
    free(t_value);
    return 0;
}

void MakeStringLiteral(const char *p_token, long *r_literal)
{
    
    char *t_value;
    t_value = strdup(p_token + 1);
    if (t_value == NULL)
        Fatal_OutOfMemory();
    t_value[strlen(t_value) - 1] = '\0';
    *r_literal = (long)t_value;
}

                    
void MakeNameLiteral(const char *p_token, NameRef *r_literal)
{
    NameRef t_name;
    for(t_name = s_names; t_name != NULL; t_name = t_name -> next)
        if (strcmp(p_token, t_name -> token) == 0)
            break;
    
    if (t_name == NULL)
    {
        t_name = (NameRef)malloc(sizeof(struct Name));
        if (t_name == NULL)
            Fatal_OutOfMemory();
        
        t_name -> token = strdup(p_token);
        if (t_name -> token == NULL)
            Fatal_OutOfMemory();
        
        t_name -> next = s_names;
        s_names = t_name;
    }
    
    *r_literal = t_name;
}

void GetStringOfNameLiteral(NameRef p_literal, const char **r_string)
{
    *r_string = ((NameRef)p_literal) -> token;
}

int IsNameEqualToString(NameRef p_name, const char *p_string)
{
    return strcmp(p_name -> token, p_string) == 0;
}

void NegateReal(long p_real, long *r_real)
{
    double *t_value;
    t_value = (double *)malloc(sizeof(double));
    if (t_value == NULL)
        Fatal_OutOfMemory();
    *t_value = -*(double *)p_real;
    *r_real = (long)t_value;
}

////////////////////////////////////////////////////////////////////////////////

static void FreeScope(ScopeRef p_scope)
{
    while(p_scope -> bindings != NULL)
    {
        BindingRef t_binding;
        t_binding = p_scope -> bindings;
        p_scope -> bindings = p_scope -> bindings -> next;
        
        free(t_binding);
    }
    
    free(p_scope);
}

static int FindNameInScope(ScopeRef p_scope, NameRef p_name, BindingRef *r_binding)
{
    BindingRef t_binding;

	for (t_binding = p_scope -> bindings; t_binding != NULL; t_binding = t_binding -> next)
        if (t_binding -> name == p_name)
        {
            *r_binding = t_binding;
            return 1;
        }
    return 0;
}

void InitializeScopes(void)
{
    s_scopes = NULL;
}

void FinalizeScopes(void)
{
}

void EnterScope(void)
{
    ScopeRef t_new_scope;
    t_new_scope = (ScopeRef)malloc(sizeof(struct Scope));
    if (t_new_scope == NULL)
        Fatal_OutOfMemory();
    
    t_new_scope -> outer = s_scopes;
    t_new_scope -> bindings = NULL;
    s_scopes = t_new_scope;
}

void LeaveScope(void)
{
    ScopeRef t_scope;

	if (s_scopes == NULL)
        Fatal_InternalInconsistency("Scope stack underflow");
    
    t_scope = s_scopes;
    s_scopes = s_scopes -> outer;
    
    FreeScope(t_scope);
}

void DefineMeaning(NameRef p_name, long p_meaning)
{
    BindingRef t_binding;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");
    
    if (FindNameInScope(s_scopes, (NameRef)p_name, &t_binding) == 0)
    {
        t_binding = (BindingRef)malloc(sizeof(struct Binding));
        if (t_binding == NULL)
            Fatal_OutOfMemory();
        
        t_binding -> next = s_scopes -> bindings;
        t_binding -> name = (NameRef)p_name;
        
        s_scopes -> bindings = t_binding;
    }
    
    t_binding -> meaning = p_meaning;
    t_binding -> has_meaning = 1;
}

void UndefineMeaning(NameRef p_name)
{
    BindingRef t_binding;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");
    
    if (FindNameInScope(s_scopes, (NameRef)p_name, &t_binding) == 1)
    {
        t_binding -> meaning = 0;
        t_binding -> has_meaning = 0;
    }
}

int HasLocalMeaning(NameRef p_name, long *r_meaning)
{
    BindingRef t_binding;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when manipulating meaning");

    if (FindNameInScope(s_scopes, (NameRef)p_name, &t_binding) == 1 &&
        t_binding -> has_meaning == 1)
    {
        *r_meaning = t_binding -> meaning;
        return 1;
    }
    
    return 0;
}

int HasMeaning(NameRef p_name, long *r_meaning)
{
    ScopeRef t_scope;
	
	if (s_scopes == NULL)
        Fatal_InternalInconsistency("No scope when checking for meaning");
    
    for(t_scope = s_scopes; t_scope != NULL; t_scope = t_scope -> outer)
    {
        BindingRef t_binding;
        if (FindNameInScope(t_scope, (NameRef)p_name, &t_binding) == 1 &&
            t_binding -> has_meaning == 1)
        {
            *r_meaning = t_binding -> meaning;
            return 1;
        }
    }
    
    return 0;
}

void DumpScopes(void)
{
    int t_depth;
	ScopeRef t_scope;

    t_depth = 0;
    for (t_scope = s_scopes; t_scope != NULL; t_scope = t_scope -> outer)
    {
        BindingRef t_binding;

		for (t_binding = t_scope -> bindings; t_binding != NULL; t_binding = t_binding -> next)
            if (t_binding -> has_meaning)
                fprintf(stderr, "[%d] %s = %ld\n", t_depth, t_binding -> name -> token, t_binding -> meaning);
        t_depth += 1;
    }
}

////////////////////////////////////////////////////////////////////////////////
