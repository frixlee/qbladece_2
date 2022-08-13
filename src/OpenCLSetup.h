/**********************************************************************

    Copyright (C) 2014 David Marten <david.marten@qblade.org>

    This program is licensed under the Academic Public License
    (APL) v1.0; You can use, redistribute and/or modify it in
    non-commercial academic environments under the terms of the
    APL as published by the QBlade project; See the file 'LICENSE'
    for details; Commercial use requires a commercial license
    (contact info@qblade.org).

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

***********************************************************************/

#ifndef OPENCLSETUP_H
#define OPENCLSETUP_H

#include <QList>
#define __CL_ENABLE_EXCEPTIONS

#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.cpp>
#endif
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>

class OpenCLSetup
{
public:

    OpenCLSetup(bool echo_off = false);
    void Setup(int platform, int device, int CPUGPU);
    void CompileKernels(int i);
    void CleanUp();

    void FilamentKernel(std::string &T);

    QStringList FoundClDevices;

    QList<QString> Devices;
    QList< QList < int > > DeviceIDs;
    cl::Kernel *kernel;
    cl::CommandQueue *queue;
    cl::Context *context;
    std::vector<cl::Platform> *platforms;
    cl::Program *program;
    std::vector<cl::Device> *devices;
    cl::Program::Sources *source;


    void SA(std::string &S_orig, std::string S_new) {S_orig.append(S_new);}
    void SA(std::string &S_orig, std::string SL,int SN, std::string SR)
    {   S_orig.append(SL);
        S_orig.append(std::to_string(SN));
        S_orig.append(SR);
    }
    void SA(std::string &S_orig, std::string SL, double SN, std::string SR)
    {   S_orig.append(SL);
        S_orig.append(std::to_string(SN));
        S_orig.append(SR);
    }
    void SA(std::string &S_orig, int S_new) {S_orig.append(std::to_string(S_new));}
};



extern OpenCLSetup *g_OpenCl;


#endif // OPENCLSETUP_H
