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

#include "QVelocityCutPlane.h"
#include "src/Params.h"
#include "../GlobalFunctions.h"
#include "src/Serializer.h"
#include "../MainFrame.h"
#include "../Globals.h"
#include "src/ImportExport.h"

QVelocityCutPlane::QVelocityCutPlane(QString name, StorableObject *parent)
: StorableObject (name, parent)
{
    is_computed = false;
    rotateRotor = 0;
}

QVelocityCutPlane* QVelocityCutPlane::newBySerialize() {
    QVelocityCutPlane* plane = new QVelocityCutPlane ();
    plane->serialize();
    return plane;
}

void QVelocityCutPlane::exportPlane(QString fileName, bool debugout) {

    if (debugout) qDebug().noquote() << "...exporting cut plane: " << fileName;

    QFile file (fileName);

    int prec = 5;

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);

        ExportFileHeader(stream);
        stream << "# Simulation Time " << m_time << " [s]" <<endl;
        stream << "# Position Vector\t\tVelocity Vector " << endl;
        stream << "        X\t        Y\t        Z\t        X\t        Y\t        Z" << endl;

        for (int i=0;i<m_points.size();i++){
            for (int j=0;j<m_points.at(i).size();j++){

             stream << QString().number(m_points[i][j].x,'f',prec)<<"\t"<<QString().number(m_points[i][j].y,'f',prec)<<"\t"<<QString().number(m_points[i][j].z,'f',prec)<<"\t";
             stream << QString().number(m_velocities[i][j].x,'f',prec)<<"\t"<<QString().number(m_velocities[i][j].y,'f',prec)<<"\t"<<QString().number(m_velocities[i][j].z,'f',prec)<<endl;

            }
        }
    }
    file.close();
}

void QVelocityCutPlane::exportPlaneVTK(QString fileName, bool debugout) {

    if (debugout) qDebug().noquote() << "...exporting cut plane: " << fileName;

//    QDate date = QDate::currentDate();
//    QTime time = QTime::currentTime();
    QFile file (fileName);

    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream stream (&file);
//VTK LEGACY FORMAT
//        stream << "# vtk DataFile Version 2.0" << endl;
//        stream << "Export File Created with " << g_mainFrame->m_VersionName << " on "<<date.toString("dd.MM.yyyy") << " at " << time.toString("hh:mm:ss") << "\n";
//        stream << "ASCII" <<endl;
//        stream << "DATASET UNSTRUCTURED_GRID" << endl;
//        stream << "POINTS " <<QString().number(m_points.size()*m_points.at(0).size(),'f',0)<<" float"<< endl;

//        for (int i=0;i<m_points.size();i++){
//            for (int j=0;j<m_points.at(i).size();j++){
//             stream << QString("%1 %2 %3").arg(m_points[i][j].x, 4).arg(m_points[i][j].y, 4).arg(m_points[i][j].z, 4)<<endl;
//            }
//        }

//        stream << endl;
//        stream << "CELLS " <<QString().number((m_points.size()-1)*(m_points.at(0).size()-1),'f',0)<<" "<<QString().number((m_points.size()-1)*(m_points.at(0).size()-1)*5,'f',0)<< endl;

//        for (int i=0;i<m_points.size()-1;i++){
//            for (int j=0;j<m_points.at(i).size()-1;j++){
//             stream << "4 "<<QString().number(i+j*m_points.at(i).size(),'f',0)<<" "<<QString().number(i+j*m_points.at(i).size()+1,'f',0)<<" "<<QString().number(i+j*m_points.at(i).size()+m_points.size()+1,'f',0)<<" "<<QString().number(i+j*m_points.at(i).size()+m_points.size(),'f',0)<<endl;
//            }
//        }

//        stream << "CELL_TYPES " <<QString().number((m_points.size()-1)*(m_points.at(0).size()-1),'f',0)<< endl;
//        for (int i=0;i<m_points.size()-1;i++){
//            for (int j=0;j<m_points.at(i).size()-1;j++){
//             stream << "9"<<endl;
//            }
//        }

//        stream <<endl;
//        stream << "POINT_DATA " <<QString().number((m_points.size())*(m_points.at(0).size()),'f',0)<< endl;
//        stream << "VECTORS velocity float"<<endl;

//        for (int i=0;i<m_points.size();i++){
//            for (int j=0;j<m_points.at(i).size();j++){
//             stream << QString("%1 %2 %3").arg(m_velocities[i][j].x, 4).arg(m_velocities[i][j].y, 4).arg(m_velocities[i][j].z, 4)<<endl;
//            }
//        }

        int prec = 5;


        stream << "<?xml version=\"1.0\"?>" <<endl;
        stream << "<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\" compressor=\"vtkZLibDataCompressor\">" <<endl;
        stream << "<UnstructuredGrid>" <<endl;
        stream << "<Piece NumberOfPoints=\""<<QString().number(m_points.size()*m_points.at(0).size(),'f',0)<<"\" NumberOfCells=\"" << QString().number((m_points.size()-1)*(m_points.at(0).size()-1),'f',0)<<"\">" <<endl;
        stream << "<Points>" <<endl;
        stream << "<DataArray type=\"Float32\" NumberOfComponents=\"3\" format=\"ascii\">" <<endl;
        for (int i=0;i<m_points.size();i++){
            for (int j=0;j<m_points.at(i).size();j++){
                stream << QString().number(m_points[i][j].x,'f', prec)<<" "<<QString().number(m_points[i][j].y,'f', prec)<<" "<<QString().number(m_points[i][j].z,'f', prec) << " ";
            }
        }
        stream << endl << "</DataArray>" <<endl;
        stream << "</Points>" <<endl;
        stream << "<Cells>" <<endl;
        stream << "<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">" <<endl;
        for (int i=0;i<m_points.size()-1;i++){
            for (int j=0;j<m_points.at(i).size()-1;j++){
             stream <<QString().number(j+i*m_points.at(i).size(),'f',0)<<" "<<QString().number(j+i*m_points.at(i).size()+1,'f',0)<<" "<<QString().number(j+(i+1)*m_points.at(i).size()+1,'f',0)<<" "<<QString().number(j+(i+1)*m_points.at(i).size(),'f',0)<<" ";
            }
        }
        stream << endl << "</DataArray>" <<endl;
        stream << "<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">" <<endl;
        int offset = 4;
        for (int i=0;i<m_points.size()-1;i++){
            for (int j=0;j<m_points.at(i).size()-1;j++){
             stream <<QString().number(offset,'f',0)<<" ";
             offset+=4;
            }
        }
        stream << endl << "</DataArray>" <<endl;
        stream << "<DataArray type=\"UInt8\" Name=\"types\" format=\"ascii\">" <<endl;
        for (int i=0;i<m_points.size()-1;i++){
            for (int j=0;j<m_points.at(i).size()-1;j++){
             stream << "9 ";
            }
        }
        stream << endl << "</DataArray>" <<endl;
        stream << "</Cells>" <<endl;
        stream << "<PointData Vectors=\"velocity\">" <<endl;
        stream << "<DataArray type=\"Float32\" Name=\"velocity\" NumberOfComponents=\"3\" format=\"ascii\">" <<endl;
        for (int i=0;i<m_points.size();i++){
            for (int j=0;j<m_points.at(i).size();j++){
             stream << QString().number(m_velocities[i][j].x,'f', prec)<<" "<<QString().number(m_velocities[i][j].y,'f', prec)<<" "<<QString().number(m_velocities[i][j].z,'f', prec) << " ";
            }
        }
        stream << endl << "</DataArray>" <<endl;
        stream << "</PointData>" <<endl;
        stream << "</Piece>" <<endl;
        stream << "</UnstructuredGrid>" <<endl;
        stream << "</VTKFile>" <<endl;

    }
    file.close();
}

void QVelocityCutPlane::serialize() {
    StorableObject::serialize();

    g_serializer.readOrWriteDouble(&m_length);
    g_serializer.readOrWriteDouble(&m_width);
    g_serializer.readOrWriteDouble(&m_X);
    g_serializer.readOrWriteDouble(&m_Y);
    g_serializer.readOrWriteDouble(&m_Z);
    g_serializer.readOrWriteDouble(&m_X_rot);
    g_serializer.readOrWriteDouble(&m_Y_rot);
    g_serializer.readOrWriteDouble(&m_Z_rot);
    g_serializer.readOrWriteDouble(&m_time);
    g_serializer.readOrWriteDouble(&rotateRotor);

    g_serializer.readOrWriteInt(&m_X_res);
    g_serializer.readOrWriteInt(&m_Y_res);
    g_serializer.readOrWriteInt(&m_timeIndex);

    g_serializer.readOrWriteBool(&is_computed);
    g_serializer.readOrWriteCVectorfVector2D(&m_velocities);

    m_meanHubHeightVelocity.serialize();
    m_Hub.serialize();
    m_Axis.serialize();

    if (g_serializer.isReadMode()) CreatePoints();

}

void QVelocityCutPlane::CreatePoints(){


    for (int i=0;i<m_points.size();i++){
    m_points[i].clear();
    }
    m_points.clear();

    QVector< Vec3f > line;

    for (int i=0;i<m_X_res;i++){
        line.clear();
        for (int j=0;j<m_Y_res;j++){
        Vec3f vec;
        vec.Set(m_X-m_length/2+m_length/(m_X_res-1)*i,m_Y-m_width/2+m_width/(m_Y_res-1)*j,m_Z);
        if (m_X_res == 1) vec.x = m_X;
        if (m_Y_res == 1) vec.y = m_Y;
        line.append(vec);
        }
        m_points.append(line);
    }

    Vec3f O;

    O.x = m_X;
    O.y = m_Y;
    O.z = m_Z;

    for (int i=0;i<m_points.size();i++){
        for (int j=0;j<m_points.at(i).size();j++){
            m_points[i][j].Rotate(O,Vec3f(1,0,0),m_X_rot);
            m_points[i][j].Rotate(O,Vec3f(0,1,0),m_Y_rot);
            m_points[i][j].Rotate(O,Vec3f(0,0,1),m_Z_rot);
        }
    }

    O.x = m_Hub.x;
    O.y = m_Hub.y;
    O.z = m_Hub.z;

    for (int i=0;i<m_points.size();i++){
        for (int j=0;j<m_points.at(i).size();j++){
            m_points[i][j].Rotate(O,Vec3f(1,0,0),rotateRotor);
        }
    }


}

void QVelocityCutPlane::Update(){

    for (int i=0;i<m_points.size();i++){
    m_points[i].clear();
    m_velocities[i].clear();
    }
    m_points.clear();
    m_velocities.clear();

    QVector< Vec3f > line;
    QVector< Vec3f > vel;

    for (int i=0;i<m_X_res;i++){
        line.clear();
        vel.clear();
        for (int j=0;j<m_Y_res;j++){
        Vec3f vec;
        vec.Set(m_X-m_length/2+m_length/(m_X_res-1)*i,m_Y-m_width/2+m_width/(m_Y_res-1)*j,m_Z);
        if (m_X_res == 1) vec.x = m_X;
        if (m_Y_res == 1) vec.y = m_Y;
        line.append(vec);
        vel.append(Vec3f(0,0,0));
        }
        m_points.append(line);
        m_velocities.append(vel);
    }

    Vec3f O;

    O.x = m_X;
    O.y = m_Y;
    O.z = m_Z;

    for (int i=0;i<m_points.size();i++){
        for (int j=0;j<m_points.at(i).size();j++){
            m_points[i][j].Rotate(O,Vec3f(1,0,0),m_X_rot);
            m_points[i][j].Rotate(O,Vec3f(0,1,0),m_Y_rot);
            m_points[i][j].Rotate(O,Vec3f(0,0,1),m_Z_rot);
        }
    }

    O.x = m_Hub.x;
    O.y = m_Hub.y;
    O.z = m_Hub.z;

    for (int i=0;i<m_points.size();i++){
        for (int j=0;j<m_points.at(i).size();j++){
            m_points[i][j].Rotate(O,Vec3f(1,0,0),rotateRotor);
        }
    }

}
void QVelocityCutPlane::drawFrame(){

    glColor3d(0,0,0);
    glLineWidth(1);
    glBegin(GL_LINE_STRIP);
    {
        glVertex3d(m_points.at(0).at(0).x,m_points.at(0).at(0).y,m_points.at(0).at(0).z);
        glVertex3d(m_points.at(m_points.size()-1).at(0).x,m_points.at(m_points.size()-1).at(0).y,m_points.at(m_points.size()-1).at(0).z);
        glVertex3d(m_points.at(m_points.size()-1).at(m_points.at(0).size()-1).x,m_points.at(m_points.size()-1).at(m_points.at(0).size()-1).y,m_points.at(m_points.size()-1).at(m_points.at(0).size()-1).z);
        glVertex3d(m_points.at(0).at(m_points.at(0).size()-1).x,m_points.at(0).at(m_points.at(0).size()-1).y,m_points.at(0).at(m_points.at(0).size()-1).z);
        glVertex3d(m_points.at(0).at(0).x,m_points.at(0).at(0).y,m_points.at(0).at(0).z);
    }
    glEnd();

}

Vec3f QVelocityCutPlane::CorrespondingAxisPoint(Vec3f Point, Vec3f Line1, Vec3f Line2)
{
     Vec3f v = Line1 - Line2;
     Vec3f w = Point - Line1;

     double c1 = w.dot(v);
     double c2 = v.dot(v);
     double b = c1 / c2;

     Vec3f Pb = Line1 + v * b;
     return Pb;
}

void QVelocityCutPlane::render(bool redblue, bool vectors, int component, double meanFrac){

    hsv hs;
    hs.s = 1;
    hs.v = 1;

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glEnable(GL_POLYGON_OFFSET_FILL);
    glPolygonOffset(1.0, 0);


    if (!is_computed){

        glPointSize(1.0);

        for (int i=0;i<m_points.size();i++){
            for (int j=0;j<m_points.at(i).size();j++){
                glBegin(GL_POINTS);

                glVertex3d(m_points.at(i).at(j).x, m_points.at(i).at(j).y, m_points.at(i).at(j).z);

                glEnd();
            }
        }
    }
    else{
         double  range = 2 * m_meanHubHeightVelocity.VAbs() * meanFrac;

         Vec3f PointOnAxis, RadialVector, TangentVector;

         double vel;
         double fac = 1 + 10*(1-meanFrac);
            for (int i=0;i<m_points.size()-1;i++){
                for (int j=0;j<m_points.at(i).size()-1;j++){
                    glBegin(GL_QUADS);

                    if (!redblue){
                        if (component == 0){
                            double vel = (m_velocities[i][j].VAbs()/m_meanHubHeightVelocity.VAbs());
                            if (vel > 1) hs.h = 112-(vel-1)*fac * 112;
                            else hs.h = 112 + (1-vel)*fac*112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 1){
                            hs.h = (m_velocities[i][j].x*(-1)+m_meanHubHeightVelocity.x)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 2){
                            hs.h = (m_velocities[i][j].y*(-1)+m_meanHubHeightVelocity.y)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 3){
                            hs.h = (m_velocities[i][j].z*(-1)+m_meanHubHeightVelocity.z)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 4){
                            hs.h = (m_meanHubHeightVelocity.dot(m_Axis)-m_velocities[i][j].dot(m_Axis))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 5){

                            PointOnAxis = CorrespondingAxisPoint(m_points[i][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(TangentVector)-m_velocities[i][j].dot(TangentVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j]-PointOnAxis);
                            RadialVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(RadialVector)-m_velocities[i][j].dot(RadialVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                    }
                    else{
                        if (component == 0){
                            vel = m_velocities[i][j].VAbs()/m_meanHubHeightVelocity.VAbs();
                            if (vel > 1) glColor4d(1,0,0,(vel-1)*fac);
                            else glColor4d(0,0,1,(1-vel)*fac);
                        }
                        if (component == 1){
                            vel = (m_meanHubHeightVelocity.x*(-1)+m_velocities[i][j].x)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 2){
                            vel = (m_meanHubHeightVelocity.y*(-1)+m_velocities[i][j].y)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 3){
                            vel = (m_meanHubHeightVelocity.z*(-1)+m_velocities[i][j].z)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 4){
                            vel = (m_meanHubHeightVelocity.dot(m_Axis)*(-1)+m_velocities[i][j].dot(m_Axis))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 5){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(TangentVector)*(-1)+m_velocities[i][j].dot(TangentVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j]-PointOnAxis);
                            RadialVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(RadialVector)*(-1)+m_velocities[i][j].dot(RadialVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                    }
                    glVertex3d(m_points.at(i).at(j).x, m_points.at(i).at(j).y, m_points.at(i).at(j).z);


                    if (!redblue){
                        if (component == 0){
                            double vel = (m_velocities[i+1][j].VAbs()/m_meanHubHeightVelocity.VAbs());
                            if (vel > 1) hs.h = 112-(vel-1)*fac * 112;
                            else hs.h = 112 + (1-vel)*fac * 112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 1){
                            hs.h = (m_velocities[i+1][j].x*(-1)+m_meanHubHeightVelocity.x)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 2){
                            hs.h = (m_velocities[i+1][j].y*(-1)+m_meanHubHeightVelocity.y)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 3){
                            hs.h = (m_velocities[i+1][j].z*(-1)+m_meanHubHeightVelocity.z)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 4){
                            hs.h = (m_meanHubHeightVelocity.dot(m_Axis)-m_velocities[i+1][j].dot(m_Axis))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 5){

                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(TangentVector)-m_velocities[i+1][j].dot(TangentVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j]-PointOnAxis);
                            RadialVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(RadialVector)-m_velocities[i+1][j].dot(RadialVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                    }
                    else{
                        if (component == 0){
                            vel = m_velocities[i+1][j].VAbs()/m_meanHubHeightVelocity.VAbs();
                            if (vel > 1) glColor4d(1,0,0,(vel-1)*fac);
                            else glColor4d(0,0,1,(1-vel)*fac);
                        }
                        if (component == 1){
                            vel = (m_meanHubHeightVelocity.x*(-1)+m_velocities[i+1][j].x)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 2){
                            vel = (m_meanHubHeightVelocity.y*(-1)+m_velocities[i+1][j].y)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 3){
                            vel = (m_meanHubHeightVelocity.z*(-1)+m_velocities[i+1][j].z)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 4){
                            vel = (m_meanHubHeightVelocity.dot(m_Axis)*(-1)+m_velocities[i+1][j].dot(m_Axis))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 5){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(TangentVector)*(-1)+m_velocities[i+1][j].dot(TangentVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j]-PointOnAxis);
                            RadialVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(RadialVector)*(-1)+m_velocities[i+1][j].dot(RadialVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                    }
                    glVertex3d(m_points.at(i+1).at(j).x, m_points.at(i+1).at(j).y, m_points.at(i+1).at(j).z);


                    if (!redblue){
                        if (component == 0){
                            double vel = (m_velocities[i+1][j+1].VAbs()/m_meanHubHeightVelocity.VAbs());
                            if (vel > 1) hs.h = 112-(vel-1)*fac * 112;
                            else hs.h = 112 + (1-vel)*fac * 112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 1){
                            hs.h = (m_velocities[i+1][j+1].x*(-1)+m_meanHubHeightVelocity.x)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 2){
                            hs.h = (m_velocities[i+1][j+1].y*(-1)+m_meanHubHeightVelocity.y)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 3){
                            hs.h = (m_velocities[i+1][j+1].z*(-1)+m_meanHubHeightVelocity.z)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 4){

                            hs.h = (m_meanHubHeightVelocity.dot(m_Axis)-m_velocities[i+1][j+1].dot(m_Axis))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 5){

                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j+1],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j+1]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(TangentVector)-m_velocities[i+1][j+1].dot(TangentVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j+1],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j+1]-PointOnAxis);
                            RadialVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(RadialVector)-m_velocities[i+1][j+1].dot(RadialVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                    }
                    else{
                        if (component == 0){
                            vel = m_velocities[i+1][j+1].VAbs()/m_meanHubHeightVelocity.VAbs();
                            if (vel > 1) glColor4d(1,0,0,(vel-1)*fac);
                            else glColor4d(0,0,1,(1-vel)*fac);
                        }
                        if (component == 1){
                            vel = (m_meanHubHeightVelocity.x*(-1)+m_velocities[i+1][j+1].x)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 2){
                            vel = (m_meanHubHeightVelocity.y*(-1)+m_velocities[i+1][j+1].y)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 3){
                            vel = (m_meanHubHeightVelocity.z*(-1)+m_velocities[i+1][j+1].z)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 4){
                            vel = (m_meanHubHeightVelocity.dot(m_Axis)*(-1)+m_velocities[i+1][j+1].dot(m_Axis))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 5){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j+1],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j+1]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(TangentVector)*(-1)+m_velocities[i+1][j+1].dot(TangentVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i+1][j+1],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i+1][j+1]-PointOnAxis);
                            RadialVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(RadialVector)*(-1)+m_velocities[i+1][j+1].dot(RadialVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                    }
                    glVertex3d(m_points.at(i+1).at(j+1).x, m_points.at(i+1).at(j+1).y, m_points.at(i+1).at(j+1).z);


                    if (!redblue){
                        if (component == 0){
                            double vel = (m_velocities[i][j+1].VAbs()/m_meanHubHeightVelocity.VAbs());
                            if (vel > 1) hs.h = 112-(vel-1)*fac * 112;
                            else hs.h = 112 + (1-vel)*fac * 112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 1){
                            hs.h = (m_velocities[i][j+1].x*(-1)+m_meanHubHeightVelocity.x)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 2){
                            hs.h = (m_velocities[i][j+1].y*(-1)+m_meanHubHeightVelocity.y)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 3){
                            hs.h = (m_velocities[i][j+1].z*(-1)+m_meanHubHeightVelocity.z)/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 4){

                            hs.h = (m_meanHubHeightVelocity.dot(m_Axis)-m_velocities[i][j+1].dot(m_Axis))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 5){

                            PointOnAxis = CorrespondingAxisPoint(m_points.at(i).at(j+1),m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j+1]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(TangentVector)-m_velocities[i][j+1].dot(TangentVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                        else if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points.at(i).at(j+1),m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j+1]-PointOnAxis);
                            RadialVector.Normalize();

                            hs.h = (m_meanHubHeightVelocity.dot(RadialVector)-m_velocities[i][j+1].dot(RadialVector))/range*225+112;
                            if (hs.h>225)hs.h = 225;
                            else if (hs.h < 0) hs.h = 0;
                            glColor4d(hsv2rgb(hs).r, hsv2rgb(hs).g, hsv2rgb(hs).b,0.7);
                        }
                    }
                    else{
                        if (component == 0){
                            vel = m_velocities[i][j+1].VAbs()/m_meanHubHeightVelocity.VAbs();
                            if (vel > 1) glColor4d(1,0,0,(vel-1)*fac);
                            else glColor4d(0,0,1,(1-vel)*fac);
                        }
                        if (component == 1){
                            vel = (m_meanHubHeightVelocity.x*(-1)+m_velocities[i][j+1].x)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 2){
                            vel = (m_meanHubHeightVelocity.y*(-1)+m_velocities[i][j+1].y)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 3){
                            vel = (m_meanHubHeightVelocity.z*(-1)+m_velocities[i][j+1].z)/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 4){
                            vel = (m_meanHubHeightVelocity.dot(m_Axis)*(-1)+m_velocities[i][j+1].dot(m_Axis))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 5){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i][j+1],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j+1]-PointOnAxis);
                            TangentVector = m_Axis * RadialVector;
                            TangentVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(TangentVector)*(-1)+m_velocities[i][j+1].dot(TangentVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                        if (component == 6){
                            PointOnAxis = CorrespondingAxisPoint(m_points[i][j+1],m_Hub,m_Hub+m_Axis);
                            RadialVector = Vec3f(m_points[i][j+1]-PointOnAxis);
                            RadialVector.Normalize();

                            vel = (m_meanHubHeightVelocity.dot(RadialVector)*(-1)+m_velocities[i][j+1].dot(RadialVector))/range;
                            if (vel > 0) glColor4d(1,0,0,vel);
                            else glColor4d(0,0,1,fabs(vel));
                        }
                    }
                    glVertex3d(m_points.at(i).at(j+1).x, m_points.at(i).at(j+1).y, m_points.at(i).at(j+1).z);

                    glEnd();
                }
            }

            if (vectors){
                glDisable(GL_DEPTH_TEST);
                double norm;
                Vec3f V;
                glLineWidth(0.5);
                glPointSize(0.5);
                for (int i=0;i<m_points.size()-1;i++){
                    for (int j=0;j<m_points.at(i).size()-1;j++){
                        glBegin(GL_LINES);
                        glPolygonOffset(1.0, 1.0);

                        V = m_velocities.at(i).at(j);
                        norm = V.VAbs();
                        norm *= 1 / m_meanHubHeightVelocity.VAbs();
                        norm = pow(norm,2);
                        if (norm>2){
                            norm =2;
                        }
                        V.Normalize();
                        if (m_width/m_Y_res < m_length / m_X_res)  V *= m_width/m_Y_res;
                        else V *= m_length/m_X_res;
                        glColor4d(0,0,0,1);

                        glVertex3d(m_points.at(i).at(j).x, m_points.at(i).at(j).y, m_points.at(i).at(j).z);

                        glVertex3d(m_points.at(i).at(j).x+V.x*norm, m_points.at(i).at(j).y+V.y*norm, m_points.at(i).at(j).z+V.z*norm);

                        glEnd();
                    }
                }
                glEnable(GL_DEPTH_TEST);
            }
    }
}
