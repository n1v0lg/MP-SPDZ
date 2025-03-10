/*
 * ProcessorBase.cpp
 *
 */

#include "ProcessorBase.h"
#include "IntInput.h"
#include "FixInput.h"
#include "FloatInput.h"
#include "Exceptions/Exceptions.h"

#include <iostream>

void ProcessorBase::open_input_file(const string& name)
{
#ifdef DEBUG_FILES
    cerr << "opening " << name << endl;
#endif
    input_file.open(name);
    input_filename = name;
}

void ProcessorBase::open_input_file(int my_num, int thread_num)
{
    string input_file = "Player-Data/Input-P" + to_string(my_num) + "-" + to_string(thread_num);
    open_input_file(input_file);
}

template<class T>
T ProcessorBase::get_input(bool interactive, const int* params)
{
    if (interactive)
        return get_input<T>(cin, "standard input", params);
    else
        return get_input<T>(input_file, input_filename, params);
}

template<class T>
T ProcessorBase::get_input(istream& input_file, const string& input_filename, const int* params)
{
    T res;
    res.read(input_file, params);
    if (input_file.eof())
        throw IO_Error("not enough inputs in " + input_filename);
    if (input_file.fail())
        throw IO_Error("cannot read from " + input_filename);
    return res;
}

template IntInput ProcessorBase::get_input(bool, const int*);
template FixInput ProcessorBase::get_input(bool, const int*);
template FloatInput ProcessorBase::get_input(bool, const int*);
