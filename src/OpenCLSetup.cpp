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

#include "OpenCLSetup.h"
#include "QString"
#include "QDebug"
#include "src/QSimulation/QSimulationModule.h"

#define __CL_ENABLE_EXCEPTIONS

/* test OpenCL */
#if defined(__APPLE__) || defined(__MACOSX)
#include <OpenCL/cl.hpp>
#else
#include <CL/cl.cpp>
#endif
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <fstream>
#include <QDebug>
#include "src/Globals.h"

using namespace std;
using namespace cl;

OpenCLSetup::OpenCLSetup(bool echo_off)
{
    kernel      = NULL;
    queue       = NULL;
    context     = NULL;
    source      = NULL;
    program     = NULL;
    platforms   = NULL;
    devices     = NULL;

        try {
            Devices.clear();
            vector<Platform> platforms;
            Platform::get(&platforms);

            int platform_id = 0;
            int i=0;

//            std::cout << "Number of Platforms: " << platforms.size() << std::endl;

            FoundClDevices.append("Number of Platforms: "+QString().number(platforms.size(),'f',0)+"\n");

            for(vector<Platform>::iterator it = platforms.begin(); it != platforms.end(); ++it){
                Platform platform(*it);

//                std::cout << "Platform ID: " << platform_id++ << std::endl;

                FoundClDevices.append("Platform ID: "+QString().number(platform_id,'f',0));

//                std::cout << "Platform Name: " << platform.getInfo<CL_PLATFORM_NAME>() << endl;

                FoundClDevices.append("Platform Name: " + QString().fromStdString(platform.getInfo<CL_PLATFORM_NAME>()).simplified());

//                std::cout << "Platform Vendor: " << platform.getInfo<CL_PLATFORM_VENDOR>() << endl;

                FoundClDevices.append("Platform Vendor: " + QString().fromStdString(platform.getInfo<CL_PLATFORM_VENDOR>()).simplified());


                vector<Device> devices;
                platform.getDevices(CL_DEVICE_TYPE_GPU | CL_DEVICE_TYPE_CPU, &devices);
                int j=0;
                for(vector<Device>::iterator it2 = devices.begin(); it2 != devices.end(); ++it2){
                    Device device(*it2);

                    QString type;

                    if (device.getInfo<CL_DEVICE_TYPE>() == CL_DEVICE_TYPE_GPU) type = "GPU";
                    else type = "CPU";

                    if (type == "GPU"){

                        QString cldeviceversion = QString().fromStdString(device.getInfo< CL_DEVICE_VERSION>()).replace('\0',' ').simplified();
                        QString cldevicename = QString().fromStdString(device.getInfo<CL_DEVICE_NAME>()).replace('\0',' ').simplified();

                        Devices.append(type+": "+cldeviceversion+" "+cldevicename);
                        QList<int> IDs;
                        IDs.append(i);
                        IDs.append(j);

                        if (type == QString("CPU")) IDs.append(0); //CPU = 0
                        if (type == QString("GPU")) IDs.append(1); //GPU = 1

                        DeviceIDs.append(IDs);

                        FoundClDevices.append("\tDevice ID: "+QString().number(j,'f',0));
                        FoundClDevices.append("\tDevice Name: " + QString().fromStdString(device.getInfo<CL_DEVICE_NAME>()).simplified());
                        FoundClDevices.append("\tDevice Type: " + type);
                        FoundClDevices.append("\tDevice Vendor: " + QString().fromStdString(device.getInfo<CL_DEVICE_VENDOR>()).simplified());
                        FoundClDevices.append("\tDevice Max Compute Units: " + QString().number(device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>(),'f',0).simplified());
                        FoundClDevices.append("\tDevice Global Memory: " + QString().number(device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>(),'f',0).simplified());
                        FoundClDevices.append("\tDevice Max Clock Frequency: " + QString().number(device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>(),'f',0).simplified());
                        FoundClDevices.append("\tDevice Max Allocateable Memory: " + QString().number(device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>(),'f',0).simplified());
                        FoundClDevices.append("\tDevice Local Memory: " + QString().number(device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>(),'f',0).simplified());
                        FoundClDevices.append("\tDevice Available: " + QString().number(device.getInfo<CL_DEVICE_AVAILABLE>(),'f',0).simplified());
                        FoundClDevices.append("\tDevice Version: " + QString().fromStdString(device.getInfo<CL_DEVICE_VERSION>()).simplified());

                        //                    cout << "Device " << j << ": " << std::endl;
                        //                    cout << "\tDevice Name: " << device.getInfo<CL_DEVICE_NAME>() << std::endl;
                        //                    cout << "\tDevice Type: " << device.getInfo<CL_DEVICE_TYPE>();
                        //                    cout << " (GPU: " << CL_DEVICE_TYPE_GPU << ", CPU: " << CL_DEVICE_TYPE_CPU << ")" << std::endl;
                        //                    cout << "\tDevice Vendor: " << device.getInfo<CL_DEVICE_VENDOR>() << std::endl;
                        //                    cout << "\tDevice Max Compute Units: " << device.getInfo<CL_DEVICE_MAX_COMPUTE_UNITS>() << std::endl;
                        //                    cout << "\tDevice Global Memory: " << device.getInfo<CL_DEVICE_GLOBAL_MEM_SIZE>() << std::endl;
                        //                    cout << "\tDevice Max Clock Frequency: " << device.getInfo<CL_DEVICE_MAX_CLOCK_FREQUENCY>() << std::endl;
                        //                    cout << "\tDevice Max Allocateable Memory: " << device.getInfo<CL_DEVICE_MAX_MEM_ALLOC_SIZE>() << std::endl;
                        //                    cout << "\tDevice Local Memory: " << device.getInfo<CL_DEVICE_LOCAL_MEM_SIZE>() << std::endl;
                        //                    cout << "\tDevice Available: " << device.getInfo< CL_DEVICE_AVAILABLE>() << std::endl;
                        //                    cout << "\tDevice Version: " << device.getInfo< CL_DEVICE_VERSION>() << std::endl;

                        j++;

                    }

                }

                FoundClDevices.append("");

                i++;

            }

        } catch(Error error) {
          qDebug() << error.what() << "(" << error.err() << ")" << endl;
        }


        if (isGUI){
            for (int i=0; i<Devices.size();i++){
//                qDebug() << Devices.at(i) << endl;
            }
        }
        else{
        #if defined QBLADE_LIBRARY
            qDebug().noquote() <<"Device List:";
            qDebug().noquote() <<"0"<< ": OpenMp (CPU)";
            for (int i=0; i<Devices.size();i++){
                QString device = QString().number(i+1,'f',0);
                qDebug().noquote() <<device<< ": " << Devices.at(i);
            }
            qDebug().noquote() <<endl<<"Specify the OpenCL group size with the parameter -gXX, where -g32 is the default";
        #else
            if (!echo_off) qDebug().noquote() <<"-d0"<< ": to use CPU: OpenMp (default)";
            for (int i=0; i<Devices.size();i++){
                QString device = "-d"+QString().number(i+1,'f',0);
                if (!echo_off) qDebug().noquote() <<device<< ": to use" << Devices.at(i);
            }
            if (!echo_off) qDebug().noquote() <<endl<<"Specify the OpenCL group size with the parameter -gXX, where -g32 is the default";
        #endif
        }

        if (isGUI){

            for (int i=0;i<Devices.size();i++) if(g_QSimulationModule) g_QSimulationModule->AddDeviceType(Devices.at(i));

            if(g_QSimulationModule){
                for (int i=0;i<g_QSimulationModule->GetDevices()->count();i++){
                    if (g_QSimulationModule->GetDevices()->findText(QString("GPU"),Qt::MatchContains) - 2 >= 0){
                        g_QSimulationModule->SetDeviceType(g_QSimulationModule->GetDevices()->findText(QString("GPU"),Qt::MatchContains));
                        CompileKernels(g_QSimulationModule->GetDevices()->findText(QString("GPU"),Qt::MatchContains)-2);
                    }
                }
            }
        }
}

void OpenCLSetup::CompileKernels(int i){

    CleanUp();
    if (DeviceIDs.size() > i && i >= 0) Setup(DeviceIDs.at(i).at(0), DeviceIDs.at(i).at(1), DeviceIDs.at(i).at(2));

}

void OpenCLSetup::Setup(int platform, int device, int CPUGPU){

    try{
    // Get available platforms
    platforms = new vector<Platform>;
    Platform::get(platforms);

    // Select the default platform and create a context using this platform and the GPU
    cl_context_properties cps[3] = {
        CL_CONTEXT_PLATFORM,
        (cl_context_properties)(platforms->at(platform))(),
        0
    };
    if (CPUGPU == 0) {
        context = new Context( CL_DEVICE_TYPE_CPU, cps);
//        qDebug() << "CPU";
    }
    else {
//        qDebug() << "GPU";
        context = new Context( CL_DEVICE_TYPE_GPU, cps);
    }

    devices = new vector<Device>;
    // Get a list of devices on this platform
    *devices = context->getInfo<CL_CONTEXT_DEVICES>();

    // Create a command queue and use the first device
    queue = new CommandQueue(*context,devices->at(device));

    std::string sourceCode;

    FilamentKernel(sourceCode);

    source = new Program::Sources(1, std::make_pair(sourceCode.c_str(), sourceCode.length()+1));

    // Make program of the source code in the context
    program = new Program(*context, *source);

    // Build program for these specific devices
    program->build(*devices);

    // Make kernel
    kernel = new Kernel(*program, "FilamentKernel");

    } catch(Error error) {
      std::cout << error.what() << "(" << error.err() << ")" << std::endl;
    }

}

void OpenCLSetup::CleanUp(){

    if (kernel) delete kernel;
    if (queue) delete queue;
    if (context) delete context;
    if (source) delete source;
    if (program) delete program;
    if (platforms) delete platforms;
    if (devices) delete devices;


    kernel = NULL;
    queue = NULL;
    context = NULL;
    source = NULL;
    program = NULL;
    platforms = NULL;
    devices = NULL;

}

void OpenCLSetup::FilamentKernel(std::string &T)
{
    // This prepares the kernel for the NF evaluation

    SA(T,   "__kernel void FilamentKernel("                                                                                     );
    SA(T,   "__global const float3 *Positions,"                                                                                 );  //  Probe positions
    SA(T,   "__global const float4 *Vort1,"                                                                                     );  //  Vortex data
    SA(T,   "__global const float4 *Vort2,"                                                                                     );  //  Vortex data
    SA(T,   "__global float3 *Velocities,"                                                                                      );  //  Output data
    SA(T,   "__global const int *elems,"                                                                                        );
    SA(T,   "__global const int *pos,"                                                                                          );
    SA(T,   " __local float4* lVort1,"                                                                                          );  //  Internal
    SA(T,   " __local float4* lVort2)"                                                                                          );  //  Internal
    SA(T,   "{"                                                                                                                 );
    SA(T,       "const uint tid = get_local_id(0);"                                                                             );
    SA(T,       "unsigned int gid = get_global_id(0);"                                                                          );
    SA(T,       "const uint localSize = get_local_size(0);"                                                                     );
    SA(T,       "const uint globalSize = get_global_size(0);"                                                                   );
    SA(T,       "if (gid > *pos-1) gid = 0;"                                                                                    );
    SA(T,       "const uint numTilesVortices = *elems / localSize + 1;"                                                         );
    SA(T,       "const float3 position = Positions[gid];"                                                                       );
    SA(T,       "float3 acc = (float3)(0.0,0.0,0.0);"                                                                           );
    SA(T,       "for(int i=0;i<numTilesVortices;i++)"                                                                           );
    SA(T,       "{"                                                                                                             );
    SA(T,           "int idx = i * localSize + tid;"                                                                            );
    SA(T,           "if (idx > *elems-1){"                                                                                      );
    SA(T,               "lVort1[tid] = (float4)(1.0,2.0,3.0,10.0);"                                                             );
    SA(T,               "lVort2[tid] = (float4)(1.0,2.0,3.0,0.0);"                                                              );
    SA(T,           "}"                                                                                                         );
    SA(T,           "else{"                                                                                                     );
    SA(T,               "lVort1[tid] = Vort1[idx];"                                                                             );
    SA(T,               "lVort2[tid] = Vort2[idx];"                                                                             );
    SA(T,           "}"                                                                                                         );
    SA(T,           "float3 acc_aux = (float3)(0.0,0.0,0.0);"                                                                   );
    SA(T,           "barrier(CLK_LOCAL_MEM_FENCE);"                                                                             );
    SA(T,           "for (int j=0; j<localSize; j++)"                                                                           );  // Loop over sources
    SA(T,           "{"                                                                                                         );
    SA(T,               "float3 r1 = position - lVort1[j].xyz;"                                                                 );
    SA(T,               "float3 r2 = position - lVort2[j].xyz;"                                                                 );
    SA(T,               "float r1a = length(r1);"                                                                               );
    SA(T,               "float r2a = length(r2);"                                                                               );
    SA(T,               "float r1r2 = r1a*r2a;"                                                                                 );
    SA(T,               "float mag = lVort2[j].w/4.0/3.14159265359*(r1a+r2a)/(r1r2*(r1r2+dot(r1,r2))+lVort1[j].w);"             );
    SA(T,               "if (!isnan(mag) && !isinf(mag)) acc_aux += cross(r1,r2) * mag;"                                        );
    SA(T,           "}"                                                                                                         );
    SA(T,           "acc += acc_aux;"                                                                                           );
    SA(T,           "barrier(CLK_LOCAL_MEM_FENCE);"                                                                             );
    SA(T,       "}"                                                                                                             );
    SA(T,       "Velocities[gid] =  acc;"                                                                                       ); // Increment output
    SA(T,   "}"                                                                                                                 );
}

OpenCLSetup *g_OpenCl;
